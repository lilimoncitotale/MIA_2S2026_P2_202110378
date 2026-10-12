#include "../../include/commands/fs_commands.h"
#include "../../include/disk/session.h"
#include "../../include/ext2/ext2_writer.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <vector>

// ============================================
// RENAME
// ============================================
bool Rename::renombrar(const std::string& path, const std::string& newName) {
    if (!g_activeSession.isActive) {
        lastError = "No hay sesión activa";
        return false;
    }
    
    Ext2Writer writer;
    if (!writer.openForWrite(g_activeSession.diskPath, 
                              g_activeSession.partition.part_start)) {
        lastError = "No se pudo abrir la partición";
        return false;
    }
    
    // 1. Verificar que existe
    int inodeIndex = writer.findInodeByPath(path);
    if (inodeIndex == -1 || inodeIndex == 0) {
        writer.close();
        lastError = "No existe: " + path;
        return false;
    }
    
    // 2. Verificar permisos sobre el archivo
    if (!writer.hasPermission(inodeIndex, 'w')) {
        writer.close();
        lastError = "Sin permisos de escritura";
        return false;
    }
    
    // 3. Verificar que el nuevo nombre no exista en la carpeta padre
    int parentInode = writer.findParentInode(path);
    int existe;
    if (writer.findFileInFolder(parentInode, newName, existe)) {
        writer.close();
        lastError = "Ya existe un archivo con ese nombre: " + newName;
        return false;
    }
    
    // 4. Actualizar la entrada en la carpeta padre
    std::string oldName = writer.getLastName(path);
    Inode parent = writer.readInode(parentInode);
    
    for (int i = 0; i < 12; i++) {
        if (parent.i_block[i] == -1) break;
        
        BlockFolder block;
        if (!writer.readBlockFolder(parent.i_block[i], block)) continue;
        
        for (int j = 0; j < 4; j++) {
            std::string entryName(block.b_content[j].b_name);
            size_t nullPos = entryName.find('\0');
            if (nullPos != std::string::npos) entryName = entryName.substr(0, nullPos);
            
            if (entryName == oldName) {
                memset(block.b_content[j].b_name, 0, 12);
                strncpy(block.b_content[j].b_name, newName.c_str(), 11);
                writer.writeBlockFolder(parent.i_block[i], block);
                
                // Journal
                writer.writeJournal("rename", path, newName);
                
                writer.close();
                std::cout << "✅ Renombrado: " << path << " → " << newName << std::endl;
                return true;
            }
        }
    }
    
    writer.close();
    lastError = "No se pudo renombrar";
    return false;
}

bool Remove::eliminar(const std::string& path) {
    if (!g_activeSession.isActive) {
        lastError = "No hay sesión activa";
        return false;
    }
    
    Ext2Writer writer;
    if (!writer.openForWrite(g_activeSession.diskPath, 
                              g_activeSession.partition.part_start)) {
        lastError = "No se pudo abrir la partición";
        return false;
    }
    
    // 1. Verificar que existe
    int inodeIndex = writer.findInodeByPath(path);
    if (inodeIndex == -1 || inodeIndex == 0) {  // -1 no existe, 0 = raíz
        writer.close();
        lastError = "No existe: " + path;
        return false;
    }
    
    // 2. Verificar permisos de escritura sobre la carpeta padre
    int parentInode = writer.findParentInode(path);
    if (parentInode == -1 || !writer.hasPermission(parentInode, 'w')) {
        writer.close();
        lastError = "Sin permisos de escritura en la carpeta padre";
        return false;
    }
    
    // 3. Si es carpeta, verificar que esté vacía (o eliminar recursivamente)
    Inode inode = writer.readInode(inodeIndex);
    if (inode.i_type == 0) {
        // Es carpeta: verificar que solo tenga . y ..
        bool vacia = true;
        for (int i = 0; i < 12; i++) {
            if (inode.i_block[i] == -1) break;
            
            BlockFolder block;
            if (!writer.readBlockFolder(inode.i_block[i], block)) continue;
            
            for (int j = 0; j < 4; j++) {
                std::string entryName(block.b_content[j].b_name);
                size_t nullPos = entryName.find('\0');
                if (nullPos != std::string::npos) entryName = entryName.substr(0, nullPos);
                
                if (!entryName.empty() && entryName != "." && entryName != "..") {
                    vacia = false;
                    break;
                }
            }
            if (!vacia) break;
        }
        
        if (!vacia) {
            writer.close();
            lastError = "La carpeta no está vacía";
            return false;
        }
    }
    
    // 4. Liberar el inodo
    writer.freeInode(inodeIndex);
    
    // 5. Remover la entrada de la carpeta padre
    std::string name = writer.getLastName(path);
    writer.removeEntryFromFolder(parentInode, name);
    
    // 6. Escribir journal
    writer.writeJournal("remove", path, "");
    
    writer.close();
    
    std::cout << "✅ Eliminado: " << path << std::endl;
    return true;
}

// ============================================
// COPY
// ============================================
bool Copy::copiar(const std::string& origen, const std::string& destino) {
    if (!g_activeSession.isActive) {
        lastError = "No hay sesión activa";
        return false;
    }
    
    Ext2Writer writer;
    if (!writer.openForWrite(g_activeSession.diskPath, 
                              g_activeSession.partition.part_start)) {
        lastError = "No se pudo abrir la partición";
        return false;
    }
    
    // 1. Verificar origen existe y es archivo (por simplicidad)
    int srcInode = writer.findInodeByPath(origen);
    if (srcInode == -1) {
        writer.close();
        lastError = "No existe origen: " + origen;
        return false;
    }
    
    Inode src = writer.readInode(srcInode);
    if (src.i_type != 1) {
        writer.close();
        lastError = "Solo se copian archivos (no carpetas)";
        return false;
    }
    
    // 2. Verificar permiso de lectura sobre el archivo origen
    if (!writer.hasPermission(srcInode, 'r')) {
        writer.close();
        lastError = "Sin permisos de lectura sobre el origen";
        return false;
    }
    
    // 3. Verificar que el destino padre exista y tenga permiso de escritura
    int destParent = writer.findParentInode(destino);
    if (destParent == -1) {
        writer.close();
        lastError = "No existe la carpeta destino";
        return false;
    }
    
    if (!writer.hasPermission(destParent, 'w')) {
        writer.close();
        lastError = "Sin permisos de escritura en destino";
        return false;
    }
    
    // 4. Leer contenido del archivo origen
    std::string content;
    if (src.i_block[0] != -1) {
        for (int i = 0; i < 12; i++) {
            if (src.i_block[i] == -1) break;
            BlockFile block;
            writer.readBlockFile(src.i_block[i], block);
            content.append(block.b_content, 64);
        }
    }
    
    // Truncar al tamaño real
    if (content.size() > (size_t)src.i_size) {
        content = content.substr(0, src.i_size);
    }
    
    // 5. Crear el archivo en el destino
    std::string destName = writer.getLastName(destino);
    int newInode;
    if (!writer.createFile(destParent, destName, 
                           g_activeSession.uid, 
                           g_activeSession.gid,
                           content, newInode)) {
        writer.close();
        lastError = "No se pudo crear el archivo destino";
        return false;
    }
    
    // 6. Journal
    writer.writeJournal("copy", origen, destino);
    
    writer.close();
    std::cout << "✅ Copiado: " << origen << " → " << destino << std::endl;
    return true;
}

// ============================================
// MOVE
// ============================================
bool Move::mover(const std::string& origen, const std::string& destino) {
    if (!g_activeSession.isActive) {
        lastError = "No hay sesión activa";
        return false;
    }
    
    // Mover = copiar + eliminar
    Copy c;
    if (!c.copiar(origen, destino)) {
        lastError = c.getLastError();
        return false;
    }
    
    Remove r;
    if (!r.eliminar(origen)) {
        lastError = r.getLastError();
        return false;
    }
    
    // Journal
    Ext2Writer writer;
    if (writer.openForWrite(g_activeSession.diskPath, 
                             g_activeSession.partition.part_start)) {
        writer.writeJournal("move", origen, destino);
        writer.close();
    }
    
    std::cout << "✅ Movido: " << origen << " → " << destino << std::endl;
    return true;
}

// ============================================
// FIND
// ============================================
// Función auxiliar recursiva (declarar estática en el .cpp)
static void findRecursive(Ext2Writer& writer, int inodeIndex, 
                           const std::string& currentPath,
                           const std::string& pattern,
                           std::vector<std::string>& results) {
    Inode inode = writer.readInode(inodeIndex);
    if (inode.i_type != 0) return;  // No es carpeta
    
    // Recorrer bloques de la carpeta
    for (int i = 0; i < 12; i++) {
        if (inode.i_block[i] == -1) break;
        
        BlockFolder block;
        if (!writer.readBlockFolder(inode.i_block[i], block)) continue;
        
        for (int j = 0; j < 4; j++) {
            std::string name(block.b_content[j].b_name);
            size_t nullPos = name.find('\0');
            if (nullPos != std::string::npos) name = name.substr(0, nullPos);
            
            if (name.empty() || name == "." || name == "..") continue;
            
            int childInode = block.b_content[j].b_inode;
            if (childInode < 0) continue;
            
            std::string fullPath = currentPath + "/" + name;
            if (currentPath == "/") fullPath = "/" + name;
            
            // Verificar patrón
            if (writer.matchPattern(name, pattern)) {
                results.push_back(fullPath);
            }
            
            // Recursión si es carpeta
            Inode child = writer.readInode(childInode);
            if (child.i_type == 0) {
                findRecursive(writer, childInode, fullPath, pattern, results);
            }
        }
    }
}

bool Find::buscar(const std::string& path, const std::string& name) {
    if (!g_activeSession.isActive) {
        lastError = "No hay sesión activa";
        return false;
    }
    
    Ext2Writer writer;
    if (!writer.openForWrite(g_activeSession.diskPath, 
                              g_activeSession.partition.part_start)) {
        lastError = "No se pudo abrir la partición";
        return false;
    }
    
    int startInode = writer.findInodeByPath(path);
    if (startInode == -1) {
        writer.close();
        lastError = "No existe: " + path;
        return false;
    }
    
    std::vector<std::string> results;
    findRecursive(writer, startInode, path, name, results);
    
    writer.writeJournal("find", path, name);
    writer.close();
    
    std::cout << "🔍 Resultados de búsqueda (" << results.size() << "):" << std::endl;
    for (const auto& r : results) {
        std::cout << "   " << r << std::endl;
    }
    
    return true;
}

// ============================================
// CHOWN
// ============================================
static bool chownRecursive(Ext2Writer& writer, int inodeIndex, int newUid) {
    Inode inode = writer.readInode(inodeIndex);
    inode.i_uid = newUid;
    writer.writeInode(inodeIndex, inode);
    
    if (inode.i_type != 0) return true;  // No es carpeta
    
    for (int i = 0; i < 12; i++) {
        if (inode.i_block[i] == -1) break;
        
        BlockFolder block;
        if (!writer.readBlockFolder(inode.i_block[i], block)) continue;
        
        for (int j = 0; j < 4; j++) {
            std::string name(block.b_content[j].b_name);
            size_t nullPos = name.find('\0');
            if (nullPos != std::string::npos) name = name.substr(0, nullPos);
            
            if (name.empty() || name == "." || name == "..") continue;
            
            int childInode = block.b_content[j].b_inode;
            if (childInode >= 0) {
                chownRecursive(writer, childInode, newUid);
            }
        }
    }
    
    return true;
}

bool Chown::cambiarPropietario(const std::string& path, 
                                const std::string& usuario, 
                                bool recursivo) {
    if (!g_activeSession.isActive) {
        lastError = "No hay sesión activa";
        return false;
    }
    
    Ext2Writer writer;
    if (!writer.openForWrite(g_activeSession.diskPath, 
                              g_activeSession.partition.part_start)) {
        lastError = "No se pudo abrir la partición";
        return false;
    }
    
    // 1. Buscar el usuario para obtener su UID
    std::vector<std::string> lines;
    writer.readUsersFile(lines);
    
    int newUid = -1;
    for (const auto& line : lines) {
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string p;
        while (std::getline(ss, p, ',')) {
            p.erase(0, p.find_first_not_of(" \t"));
            p.erase(p.find_last_not_of(" \t") + 1);
            parts.push_back(p);
        }
        if (parts.size() >= 5 && parts[1] == "U" && parts[3] == usuario) {
            newUid = std::stoi(parts[0]);
            break;
        }
    }
    
    if (newUid == -1) {
        writer.close();
        lastError = "Usuario no encontrado: " + usuario;
        return false;
    }
    
    // 2. Verificar que existe el path
    int inodeIndex = writer.findInodeByPath(path);
    if (inodeIndex == -1) {
        writer.close();
        lastError = "No existe: " + path;
        return false;
    }
    
    // 3. Verificar permisos: root o dueño actual
    Inode inode = writer.readInode(inodeIndex);
    if (g_activeSession.uid != 1 && g_activeSession.uid != inode.i_uid) {
        writer.close();
        lastError = "Sin permisos para cambiar propietario";
        return false;
    }
    
    // 4. Cambiar propietario
    if (recursivo) {
        chownRecursive(writer, inodeIndex, newUid);
    } else {
        inode.i_uid = newUid;
        writer.writeInode(inodeIndex, inode);
    }
    
    // 5. Journal
    writer.writeJournal("chown", path, usuario);
    
    writer.close();
    std::cout << "✅ Propietario cambiado: " << path << " → " << usuario << std::endl;
    return true;
}