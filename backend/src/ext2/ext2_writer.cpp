#include "../../include/ext2/ext2_writer.h"
#include "../../include/disk/session.h"
#include "../../include/ext2/journal.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <regex>

Ext2Writer::Ext2Writer() : Ext2Reader() {}

Ext2Writer::~Ext2Writer() {
    close();
}

bool Ext2Writer::openForWrite(const std::string& path, int partitionStart) {
    // Primero abrir en modo lectura
    if (!open(path, partitionStart)) {
        return false;
    }
    
    // Reabrir en modo lectura/escritura
    file.close();
    file.open(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) {
        return false;
    }
    
    // Releer SuperBlock
    file.seekg(partitionStart, std::ios::beg);
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBlock));
    
    if (sb.s_magic != 0xEF53) {
        file.close();
        return false;
    }
    
    isOpen = true;
    return true;
}

// ============================================
// BITMAPS
// ============================================

int Ext2Writer::findFreeInode() {
    for (int i = 0; i < sb.s_inodes_count; i++) {
        int byteIndex = i / 8;
        int bitOffset = i % 8;
        int offset = partitionStart + sb.s_bm_inode_start + byteIndex;
        
        file.seekg(offset, std::ios::beg);
        char byte;
        file.read(&byte, 1);
        
        if (!(byte & (1 << bitOffset))) {
            return i;
        }
    }
    return -1;
}

int Ext2Writer::findFreeBlock() {
    for (int i = 0; i < sb.s_blocks_count; i++) {
        int byteIndex = i / 8;
        int bitOffset = i % 8;
        int offset = partitionStart + sb.s_bm_block_start + byteIndex;
        
        file.seekg(offset, std::ios::beg);
        char byte;
        file.read(&byte, 1);
        
        if (!(byte & (1 << bitOffset))) {
            return i;
        }
    }
    return -1;
}

bool Ext2Writer::markInodeOccupied(int inodeIndex) {
    int byteIndex = inodeIndex / 8;
    int bitOffset = inodeIndex % 8;
    int offset = partitionStart + sb.s_bm_inode_start + byteIndex;
    
    file.seekg(offset, std::ios::beg);
    char byte;
    file.read(&byte, 1);
    
    byte |= (1 << bitOffset);
    
    file.seekp(offset, std::ios::beg);
    file.write(&byte, 1);
    
    return file.good();
}

bool Ext2Writer::markBlockOccupied(int blockIndex) {
    int byteIndex = blockIndex / 8;
    int bitOffset = blockIndex % 8;
    int offset = partitionStart + sb.s_bm_block_start + byteIndex;
    
    file.seekg(offset, std::ios::beg);
    char byte;
    file.read(&byte, 1);
    
    byte |= (1 << bitOffset);
    
    file.seekp(offset, std::ios::beg);
    file.write(&byte, 1);
    
    return file.good();
}

std::string Ext2Writer::getLastName(const std::string& path) const {
    size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

int Ext2Writer::findParentInode(const std::string& path) {
    size_t slash = path.find_last_of('/');
    if (slash == std::string::npos || slash == 0) return 0;
    return findFolderByPath(path.substr(0, slash));
}

int Ext2Writer::findInodeByPath(const std::string& path) {
    if (path.empty() || path == "/") return 0;
    int current = 0;
    std::stringstream parts(path);
    std::string part;
    while (std::getline(parts, part, '/')) {
        if (part.empty()) continue;
        int next = -1;
        if (!findFileInFolder(current, part, next)) return -1;
        current = next;
    }
    return current;
}

bool Ext2Writer::hasPermission(int inodeIndex, char operation) const {
    if (g_activeSession.uid == 1) return true;
    return inodeIndex >= 0;
}

bool Ext2Writer::freeInode(int inodeIndex) {
    int byteIndex = inodeIndex / 8;
    int bitOffset = inodeIndex % 8;
    int offset = partitionStart + sb.s_bm_inode_start + byteIndex;
    file.seekg(offset, std::ios::beg);
    char byte = 0;
    file.read(&byte, 1);
    byte &= static_cast<char>(~(1 << bitOffset));
    file.clear();
    file.seekp(offset, std::ios::beg);
    file.write(&byte, 1);
    return file.good();
}

bool Ext2Writer::removeEntryFromFolder(int folderInode, const std::string& name) {
    Inode parent = readInode(folderInode);
    for (int i = 0; i < 12 && parent.i_block[i] != -1; ++i) {
        BlockFolder block;
        if (!readBlockFolder(parent.i_block[i], block)) continue;
        for (Content& entry : block.b_content) {
            if (std::string(entry.b_name) == name) {
                entry = Content();
                return writeBlockFolder(parent.i_block[i], block);
            }
        }
    }
    return false;
}

bool Ext2Writer::matchPattern(const std::string& value, const std::string& pattern) const {
    std::string expression;
    for (char c : pattern) {
        if (c == '*') expression += ".*";
        else if (c == '?') expression += '.';
        else expression += std::regex_replace(std::string(1, c), std::regex("([.^$|()\\\\+])"), "\\\\$1");
    }
    return std::regex_match(value, std::regex(expression));
}

// ============================================
// JOURNALING (REAL)
// ============================================

bool Ext2Writer::writeJournal(const std::string& operation, 
                               const std::string& path, 
                               const std::string& content) {
    // Solo escribir si es EXT3
    if (sb.s_filesystem_type != 3) {
        return true;  // Silenciosamente ignorar en EXT2
    }
    
    // El área de journaling está justo después del SuperBlock
    int journalStart = partitionStart + sizeof(SuperBlock);
    
    // Buscar el primer slot libre
    for (int i = 0; i < sb.s_inodes_count; i++) {
        Journal journal;
        int offset = journalStart + i * sizeof(Journal);
        
        file.clear();
        file.seekg(offset, std::ios::beg);
        file.read(reinterpret_cast<char*>(&journal), sizeof(Journal));
        
        // Si está vacío, usar este slot
        std::string op(journal.j_content.i_operation);
        op = op.c_str();
        
        if (op.empty()) {
            // Llenar este slot
            journal.j_count = i;
            memset(journal.j_content.i_operation, 0, 10);
            memset(journal.j_content.i_path, 0, 32);
            memset(journal.j_content.i_content, 0, 64);
            
            strncpy(journal.j_content.i_operation, operation.c_str(), 9);
            strncpy(journal.j_content.i_path, path.c_str(), 31);
            strncpy(journal.j_content.i_content, content.c_str(), 63);
            journal.j_content.i_date = (float)time(nullptr);
            
            file.clear();
            file.seekp(offset, std::ios::beg);
            file.write(reinterpret_cast<const char*>(&journal), sizeof(Journal));
            file.flush();
            
            return file.good();
        }
    }
    
    // Si todos están llenos, sobreescribir el primero
    Journal journal;
    journal.j_count = 0;
    memset(journal.j_content.i_operation, 0, 10);
    memset(journal.j_content.i_path, 0, 32);
    memset(journal.j_content.i_content, 0, 64);
    strncpy(journal.j_content.i_operation, operation.c_str(), 9);
    strncpy(journal.j_content.i_path, path.c_str(), 31);
    strncpy(journal.j_content.i_content, content.c_str(), 63);
    journal.j_content.i_date = (float)time(nullptr);
    
    file.clear();
    file.seekp(journalStart, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&journal), sizeof(Journal));
    file.flush();
    
    return file.good();
}

bool Ext2Writer::readJournal(std::vector<Journal>& journals) {
    journals.clear();
    
    if (sb.s_filesystem_type != 3) {
        return false;  // No hay journal en EXT2
    }
    
    int journalStart = partitionStart + sizeof(SuperBlock);
    
    for (int i = 0; i < sb.s_inodes_count; i++) {
        Journal journal;
        int offset = journalStart + i * sizeof(Journal);
        
        file.clear();
        file.seekg(offset, std::ios::beg);
        file.read(reinterpret_cast<char*>(&journal), sizeof(Journal));
        
        std::string op(journal.j_content.i_operation);
        op = op.c_str();
        
        if (!op.empty()) {
            journals.push_back(journal);
        }
    }
    
    return true;
}

bool Ext2Writer::updateSuperBlock(const SuperBlock& sb) {
    file.seekp(partitionStart, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&sb), sizeof(SuperBlock));
    return file.good();
}

// ============================================
// INODOS Y BLOQUES
// ============================================

Inode Ext2Writer::readInode(int inodeIndex) {
    Inode inode;
    int inodeOffset = partitionStart + sb.s_inode_start + (inodeIndex * sb.s_inode_size);
    
    file.seekg(inodeOffset, std::ios::beg);
    file.read(reinterpret_cast<char*>(&inode), sizeof(Inode));
    
    return inode;
}

bool Ext2Writer::writeInode(int inodeIndex, const Inode& inode) {
    int inodeOffset = partitionStart + sb.s_inode_start + (inodeIndex * sb.s_inode_size);
    file.seekp(inodeOffset, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&inode), sizeof(Inode));
    return file.good();
}

bool Ext2Writer::writeBlockFolder(int blockIndex, const BlockFolder& block) {
    int blockOffset = partitionStart + sb.s_block_start + (blockIndex * sb.s_block_size);
    file.seekp(blockOffset, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&block), sizeof(BlockFolder));
    return file.good();
}

bool Ext2Writer::writeBlockFile(int blockIndex, const BlockFile& block) {
    int blockOffset = partitionStart + sb.s_block_start + (blockIndex * sb.s_block_size);
    file.seekp(blockOffset, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&block), sizeof(BlockFile));
    return file.good();
}

// ============================================
// USERS.TXT
// ============================================

bool Ext2Writer::readUsersFile(std::vector<std::string>& lines) {
    return Ext2Reader::readUsersFile(lines);
}

bool Ext2Writer::writeUsersFile(const std::vector<std::string>& lines) {
    std::string content;
    for (const auto& line : lines) {
        if (!line.empty()) content += line + "\n";
    }
    
    int inodeIndex = 1;
    Inode inode = readInode(inodeIndex);
    
    // ✅ Verificar que el inodo sea correcto
    if (inode.i_type != 1) return false;
    
    // ✅ Calcular cuántos bloques se necesitan
    int numBlocks = (content.size() + 63) / 64;
    if (numBlocks == 0) numBlocks = 1;
    
    // ✅ Asignar bloques
    for (int i = 0; i < numBlocks && i < 12; i++) {
        int blockIndex = inode.i_block[i];
        
        if (blockIndex == -1) {
            blockIndex = findFreeBlock();
            if (blockIndex == -1) return false;
            inode.i_block[i] = blockIndex;
            markBlockOccupied(blockIndex);
            sb.s_free_blocks_count--;
        }
        
        BlockFile block;
        memset(&block, 0, sizeof(BlockFile));
        
        size_t start = i * 64;
        size_t length = std::min<size_t>(64, content.size() - start);
        memcpy(block.b_content, content.c_str() + start, length);
        
        writeBlockFile(blockIndex, block);
    }
    
    inode.i_size = content.size();
    writeInode(inodeIndex, inode);
    updateSuperBlock(sb);
    
    return true;
}

int Ext2Writer::getNextGID(const std::vector<std::string>& lines) {
    int maxGID = 0;
    for (const auto& line : lines) {
        if (line.empty()) continue;
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string part;
        while (std::getline(ss, part, ',')) {
            // Limpiar espacios
            part.erase(0, part.find_first_not_of(" \t"));
            part.erase(part.find_last_not_of(" \t") + 1);
            parts.push_back(part);
        }
        if (parts.size() >= 3 && parts[1] == "G") {
            try {
                int gid = std::stoi(parts[0]);
                if (gid > maxGID) maxGID = gid;
            } catch (...) {}
        }
    }
    return maxGID + 1;
}

int Ext2Writer::getNextUID(const std::vector<std::string>& lines) {
    int maxUID = 0;
    for (const auto& line : lines) {
        if (line.empty()) continue;
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string part;
        while (std::getline(ss, part, ',')) {
            part.erase(0, part.find_first_not_of(" \t"));
            part.erase(part.find_last_not_of(" \t") + 1);
            parts.push_back(part);
        }
        if (parts.size() >= 5 && parts[1] == "U") {
            try {
                int uid = std::stoi(parts[0]);
                if (uid > maxUID) maxUID = uid;
            } catch (...) {}
        }
    }
    return maxUID + 1;
}

bool Ext2Writer::findGroup(const std::vector<std::string>& lines, const std::string& name, int& gid) {
    for (const auto& line : lines) {
        if (line.empty()) continue;
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string part;
        while (std::getline(ss, part, ',')) {
            part.erase(0, part.find_first_not_of(" \t"));
            part.erase(part.find_last_not_of(" \t") + 1);
            parts.push_back(part);
        }
        if (parts.size() >= 3 && parts[1] == "G" && parts[2] == name) {
            try {
                gid = std::stoi(parts[0]);
                return true;
            } catch (...) {}
        }
    }
    return false;
}

bool Ext2Writer::findUser(const std::vector<std::string>& lines, const std::string& name, int& uid) {
    for (const auto& line : lines) {
        if (line.empty()) continue;
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string part;
        while (std::getline(ss, part, ',')) {
            part.erase(0, part.find_first_not_of(" \t"));
            part.erase(part.find_last_not_of(" \t") + 1);
            parts.push_back(part);
        }
        if (parts.size() >= 5 && parts[1] == "U" && parts[3] == name) {
            try {
                uid = std::stoi(parts[0]);
                return true;
            } catch (...) {}
        }
    }
    return false;
}

// ============================================
// ARCHIVOS Y CARPETAS
// ============================================

bool Ext2Writer::findFileInFolder(int folderInode, const std::string& name, int& foundInode) {
    Inode inode = readInode(folderInode);
    
    // Verificar que sea una carpeta
    if (inode.i_type != 0) {
        return false;
    }
    
    // Recorrer los bloques directos (hasta 12)
    for (int i = 0; i < 12; i++) {
        if (inode.i_block[i] == -1) break;
        
        BlockFolder block;
        if (!readBlockFolder(inode.i_block[i], block)) {
            continue;
        }
        
        // Recorrer las 4 entradas del bloque
        for (int j = 0; j < 4; j++) {
            std::string entryName(block.b_content[j].b_name);
            // Limpiar caracteres nulos
            entryName = entryName.c_str();
            
            if (!entryName.empty() && entryName == name) {
                foundInode = block.b_content[j].b_inode;
                return true;
            }
        }
    }
    
    return false;
}

bool Ext2Writer::addEntryToFolder(int folderInode, const std::string& name, int childInode) {
    Inode inode = readInode(folderInode);
    
    // Verificar que sea carpeta
    if (inode.i_type != 0) {
        return false;
    }
    
    // Buscar un espacio libre en los bloques de la carpeta
    for (int i = 0; i < 12; i++) {
        if (inode.i_block[i] == -1) continue;
        
        BlockFolder block;
        if (!readBlockFolder(inode.i_block[i], block)) {
            continue;
        }
        
        // Buscar un slot libre (b_inode == -1 o b_name vacío)
        for (int j = 0; j < 4; j++) {
            std::string entryName(block.b_content[j].b_name);
            entryName = entryName.c_str();
            
            if (entryName.empty() || block.b_content[j].b_inode == -1) {
                // Encontramos un slot libre
                strncpy(block.b_content[j].b_name, name.c_str(), 12);
                block.b_content[j].b_name[11] = '\0';
                block.b_content[j].b_inode = childInode;
                
                if (!writeBlockFolder(inode.i_block[i], block)) {
                    return false;
                }
                return true;
            }
        }
    }
    
    // Si no hay espacio, necesitamos crear un nuevo bloque
    int newBlock = findFreeBlock();
    if (newBlock == -1) {
        return false;
    }
    
    // Crear nuevo bloque de carpeta
    BlockFolder newBlockFolder;
    strncpy(newBlockFolder.b_content[0].b_name, name.c_str(), 12);
    newBlockFolder.b_content[0].b_name[11] = '\0';
    newBlockFolder.b_content[0].b_inode = childInode;
    
    if (!writeBlockFolder(newBlock, newBlockFolder)) {
        return false;
    }
    
    // Marcar el bloque como ocupado
    if (!markBlockOccupied(newBlock)) {
        return false;
    }
    
    // Agregar el bloque al inodo
    for (int i = 0; i < 12; i++) {
        if (inode.i_block[i] == -1) {
            inode.i_block[i] = newBlock;
            break;
        }
    }
    
    // Actualizar el tamaño del inodo
    inode.i_size += sizeof(BlockFolder);
    
    return writeInode(folderInode, inode);
}

bool Ext2Writer::createFolder(int parentInode, const std::string& name, 
                              int uid, int gid, int& newInode) {
    // 1. Buscar inodo libre
    newInode = findFreeInode();
    if (newInode == -1) {
        return false;
    }
    
    // 2. Buscar bloque libre para el bloque de carpeta
    int newBlock = findFreeBlock();
    if (newBlock == -1) {
        return false;
    }
    
    // 3. Crear el inodo de la carpeta
    Inode inode;
    inode.i_uid = uid;
    inode.i_gid = gid;
    inode.i_size = sizeof(BlockFolder);
    inode.i_atime = time(nullptr);
    inode.i_ctime = time(nullptr);
    inode.i_mtime = time(nullptr);
    inode.i_type = 0;  // Carpeta
    inode.i_perm[0] = '6';
    inode.i_perm[1] = '6';
    inode.i_perm[2] = '4';
    
    // Inicializar bloques
    for (int i = 0; i < 15; i++) {
        inode.i_block[i] = -1;
    }
    inode.i_block[0] = newBlock;
    
    // 4. Escribir el inodo
    if (!writeInode(newInode, inode)) {
        return false;
    }
    
    // 5. Crear el bloque de carpeta
    BlockFolder block;
    // Entrada 0: "."
    strncpy(block.b_content[0].b_name, ".", 12);
    block.b_content[0].b_inode = newInode;
    // Entrada 1: ".."
    strncpy(block.b_content[1].b_name, "..", 12);
    block.b_content[1].b_inode = parentInode;
    
    if (!writeBlockFolder(newBlock, block)) {
        return false;
    }
    
    // 6. Marcar inodo y bloque como ocupados
    if (!markInodeOccupied(newInode)) {
        return false;
    }
    if (!markBlockOccupied(newBlock)) {
        return false;
    }
    
    // 7. Agregar entrada al padre
    if (!addEntryToFolder(parentInode, name, newInode)) {
        return false;
    }
    
    // 8. Actualizar SuperBlock
    sb.s_free_inodes_count--;
    sb.s_free_blocks_count--;
    updateSuperBlock(sb);
    
    return true;
}

bool Ext2Writer::createFile(int parentInode, const std::string& name, 
                            int uid, int gid, 
                            const std::string& content, int& newInode) {
    // 1. Buscar inodo libre
    newInode = findFreeInode();
    if (newInode == -1) {
        return false;
    }
    
    // 2. Crear el inodo del archivo
    Inode inode;
    inode.i_uid = uid;
    inode.i_gid = gid;
    inode.i_size = content.size();
    inode.i_atime = time(nullptr);
    inode.i_ctime = time(nullptr);
    inode.i_mtime = time(nullptr);
    inode.i_type = 1;  // Archivo
    inode.i_perm[0] = '6';
    inode.i_perm[1] = '6';
    inode.i_perm[2] = '4';
    
    // Inicializar bloques
    for (int i = 0; i < 15; i++) {
        inode.i_block[i] = -1;
    }
    
    // 3. Asignar bloques para el contenido
    if (!content.empty()) {
        int numBlocks = (content.size() + 63) / 64;  // Redondear hacia arriba
        if (numBlocks > 12) numBlocks = 12;  // Por ahora solo bloques directos
        
        for (int i = 0; i < numBlocks; i++) {
            int newBlock = findFreeBlock();
            if (newBlock == -1) {
                return false;
            }
            
            inode.i_block[i] = newBlock;
            
            // Preparar el bloque de datos
            BlockFile block;
            memset(&block, 0, sizeof(BlockFile));
            
            size_t start = i * 64;
            size_t length = std::min<size_t>(64, content.size() - start);
            memcpy(block.b_content, content.c_str() + start, length);
            
            if (!writeBlockFile(newBlock, block)) {
                return false;
            }
            
            if (!markBlockOccupied(newBlock)) {
                return false;
            }
            
            sb.s_free_blocks_count--;
        }
    }
    
    // 4. Escribir el inodo
    if (!writeInode(newInode, inode)) {
        return false;
    }
    
    // 5. Marcar inodo como ocupado
    if (!markInodeOccupied(newInode)) {
        return false;
    }
    sb.s_free_inodes_count--;
    
    // 6. Agregar entrada al padre
    if (!addEntryToFolder(parentInode, name, newInode)) {
        return false;
    }
    
    // 7. Actualizar SuperBlock
    updateSuperBlock(sb);
    
    return true;
}

// ============================================
// UTILIDADES
// ============================================

int Ext2Writer::findFolderByPath(const std::string& path) {
    if (path.empty() || path == "/") {
        return 0;  // Raíz
    }
    
    // Dividir la ruta por "/"
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;
    
    while (std::getline(ss, part, '/')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    
    int currentInode = 0;  // Empezar en la raíz
    
    for (const auto& folderName : parts) {
        int foundInode;
        if (!findFileInFolder(currentInode, folderName, foundInode)) {
            return -1;  // No encontrado
        }
        
        // Verificar que sea carpeta
        Inode inode = readInode(foundInode);
        if (inode.i_type != 0) {
            return -1;  // No es carpeta
        }
        
        currentInode = foundInode;
    }
    
    return currentInode;
}

bool Ext2Writer::createFolderRecursive(const std::string& path, int uid, int gid, int& newInode) {
    // Verificar si ya existe
    int existing = findFolderByPath(path);
    if (existing != -1) {
        newInode = existing;
        return true;
    }
    
    // Dividir la ruta
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;
    
    while (std::getline(ss, part, '/')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    
    int currentInode = 0;  // Raíz
    
    for (const auto& folderName : parts) {
        int foundInode;
        if (findFileInFolder(currentInode, folderName, foundInode)) {
            currentInode = foundInode;
        } else {
            // Crear la carpeta
            int createdInode;
            if (!createFolder(currentInode, folderName, uid, gid, createdInode)) {
                return false;
            }
            currentInode = createdInode;
        }
    }
    
    newInode = currentInode;
    return true;
}