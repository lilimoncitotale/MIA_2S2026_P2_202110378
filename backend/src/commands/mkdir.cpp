#include "../../include/commands/mkdir.h"
#include "../../include/disk/session.h"
#include "../../include/ext2/ext2_writer.h"
#include <iostream>

MKDIR::MKDIR() {}

bool MKDIR::parsePath(const std::string& fullPath, std::string& parentPath, std::string& folderName) {
    // Eliminar "/" al final si existe
    std::string path = fullPath;
    if (!path.empty() && path.back() == '/') {
        path.pop_back();
    }
    
    size_t lastSlash = path.find_last_of('/');
    
    if (lastSlash == std::string::npos) {
        parentPath = "/";
        folderName = path;
    } else if (lastSlash == 0) {
        parentPath = "/";
        folderName = path.substr(1);
    } else {
        parentPath = path.substr(0, lastSlash);
        folderName = path.substr(lastSlash + 1);
    }
    
    if (folderName.empty()) {
        lastError = "Error: Nombre de carpeta vacío";
        return false;
    }
    
    return true;
}

bool MKDIR::crearCarpeta(const std::map<std::string, std::string>& params) {
    std::cout << "🔍 Creando carpeta..." << std::endl;
    
    // 1. Verificar sesión activa
    if (!g_activeSession.isActive) {
        lastError = "Error: No hay sesión activa. Use LOGIN primero.";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 2. Obtener parámetros
    std::string path = params.count("-path") ? params.at("-path") : "";
    bool p_flag = params.count("-p") > 0;
    
    std::cout << "   Ruta: " << path << std::endl;
    std::cout << "   Recursivo (-p): " << (p_flag ? "sí" : "no") << std::endl;
    
    // 3. Validar ruta
    if (path.empty() || path[0] != '/') {
        lastError = "Error: La ruta debe comenzar con '/'";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 4. Parsear la ruta
    std::string parentPath, folderName;
    if (!parsePath(path, parentPath, folderName)) {
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 5. Abrir EXT2
    Ext2Writer writer;
    if (!writer.openForWrite(g_activeSession.diskPath, 
                             g_activeSession.partition.part_start)) {
        lastError = "Error: No se pudo abrir la partición EXT2";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 6. Buscar o crear la carpeta padre
    int parentInode = writer.findFolderByPath(parentPath);
    
    if (parentInode == -1) {
        if (p_flag) {
            // Crear carpetas recursivamente
            std::cout << "📁 Creando carpetas padre..." << std::endl;
            if (!writer.createFolderRecursive(parentPath, g_activeSession.uid, 
                                               g_activeSession.gid, parentInode)) {
                writer.close();
                lastError = "Error: No se pudieron crear las carpetas padre";
                std::cout << "❌ " << lastError << std::endl;
                return false;
            }
        } else {
            writer.close();
            lastError = "Error: La carpeta padre no existe: " + parentPath;
            std::cout << "❌ " << lastError << std::endl;
            std::cout << "   Use -p para crear las carpetas automáticamente" << std::endl;
            return false;
        }
    }
    
    // 7. Verificar si ya existe
    int existingInode;
    if (writer.findFileInFolder(parentInode, folderName, existingInode)) {
        writer.close();
        lastError = "Error: Ya existe un archivo o carpeta con ese nombre";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 8. Crear la carpeta
    int newInode;
    if (!writer.createFolder(parentInode, folderName, 
                             g_activeSession.uid, 
                             g_activeSession.gid,
                             newInode)) {
        writer.close();
        lastError = "Error: No se pudo crear la carpeta";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }

    writer.writeJournal("mkdir", path, "");
    
    writer.close();
    
    std::cout << "✅ Carpeta creada exitosamente:" << std::endl;
    std::cout << "   Ruta: " << path << std::endl;
    std::cout << "   Inodo: " << newInode << std::endl;
    
    return true;
}