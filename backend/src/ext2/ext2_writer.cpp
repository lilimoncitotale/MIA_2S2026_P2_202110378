#include "../../include/ext2/ext2_writer.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <algorithm>

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
    // Construir el contenido
    std::string content;
    for (const auto& line : lines) {
        if (!line.empty()) {
            content += line + "\n";
        }
    }
    
    std::cout << "📝 Escribiendo users.txt (" << content.size() << " bytes)" << std::endl;
    
    // Buscar el inodo de users.txt (debe ser el 1)
    int inodeIndex = 1;
    Inode inode = readInode(inodeIndex);
    
    if (inode.i_type != 1) {
        return false;
    }
    
    // Calcular cuántos bloques se necesitan
    int numBlocks = (content.size() + 63) / 64;
    if (numBlocks == 0) numBlocks = 1;
    
    std::cout << "   Bloques necesarios: " << numBlocks << std::endl;
    
    // Limpiar apuntadores antiguos (si había más de los que necesitamos)
    // Pero primero liberar los bloques que ya no se usan
    // (Por simplicidad, solo asignamos si no existen)
    
    // Asignar bloques para el contenido
    for (int i = 0; i < numBlocks && i < 12; i++) {  // Solo bloques directos
        int blockIndex = inode.i_block[i];
        
        // Si no tiene bloque asignado, buscar uno libre
        if (blockIndex == -1) {
            blockIndex = findFreeBlock();
            if (blockIndex == -1) {
                std::cerr << "❌ No hay bloques libres" << std::endl;
                return false;
            }
            inode.i_block[i] = blockIndex;
            markBlockOccupied(blockIndex);
            sb.s_free_blocks_count--;
        }
        
        // Preparar el bloque
        BlockFile block;
        memset(&block, 0, sizeof(BlockFile));
        
        size_t start = i * 64;
        size_t length = std::min<size_t>(64, content.size() - start);
        memcpy(block.b_content, content.c_str() + start, length);
        
        if (!writeBlockFile(blockIndex, block)) {
            return false;
        }
    }
    
    // Actualizar el tamaño del inodo
    inode.i_size = content.size();
    inode.i_mtime = time(nullptr);
    
    // Escribir el inodo actualizado
    if (!writeInode(inodeIndex, inode)) {
        return false;
    }
    
    // Actualizar SuperBlock
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