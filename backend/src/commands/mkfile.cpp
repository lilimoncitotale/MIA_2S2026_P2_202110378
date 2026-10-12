#include "../../include/commands/mkfile.h"
#include "../../include/disk/session.h"
#include "../../include/ext2/ext2_writer.h"
#include <iostream>
#include <fstream>
#include <sstream>

MKFILE::MKFILE() {}

bool MKFILE::parsePath(const std::string& fullPath, std::string& parentPath, std::string& fileName) {
    // Buscar la última "/"
    size_t lastSlash = fullPath.find_last_of('/');
    
    if (lastSlash == std::string::npos) {
        // No hay "/", el archivo está en la raíz
        parentPath = "/";
        fileName = fullPath;
    } else if (lastSlash == 0) {
        // Es "/archivo"
        parentPath = "/";
        fileName = fullPath.substr(1);
    } else {
        parentPath = fullPath.substr(0, lastSlash);
        fileName = fullPath.substr(lastSlash + 1);
    }
    
    if (fileName.empty()) {
        lastError = "Error: Nombre de archivo vacío";
        return false;
    }
    
    return true;
}

std::string MKFILE::generateContent(int size) {
    std::string content;
    for (int i = 0; i < size; i++) {
        content += std::to_string(i % 10);
    }
    return content;
}

bool MKFILE::readFileFromPC(const std::string& pcPath, std::string& content) {
    std::ifstream file(pcPath, std::ios::binary);
    if (!file.is_open()) {
        lastError = "Error: No se pudo abrir el archivo: " + pcPath;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    content = buffer.str();
    file.close();
    
    return true;
}

bool MKFILE::crearArchivo(const std::map<std::string, std::string>& params) {
    std::cout << "🔍 Creando archivo..." << std::endl;
    
    // 1. Verificar sesión activa
    if (!g_activeSession.isActive) {
        lastError = "Error: No hay sesión activa. Use LOGIN primero.";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 2. Obtener parámetros
    std::string path = params.count("-path") ? params.at("-path") : "";
    bool r_flag = params.count("-r") > 0;
    bool hasSize = params.count("-size") > 0;
    bool hasCont = params.count("-cont") > 0;
    
    int size = hasSize ? std::stoi(params.at("-size")) : 0;
    std::string contPath = hasCont ? params.at("-cont") : "";
    
    std::cout << "   Ruta: " << path << std::endl;
    std::cout << "   Tamaño: " << size << std::endl;
    std::cout << "   Contenido: " << (hasCont ? contPath : "ninguno") << std::endl;
    std::cout << "   Recursivo: " << (r_flag ? "sí" : "no") << std::endl;
    
    // 3. Validar ruta
    if (path.empty() || path[0] != '/') {
        lastError = "Error: La ruta debe comenzar con '/'";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 4. Parsear la ruta
    std::string parentPath, fileName;
    if (!parsePath(path, parentPath, fileName)) {
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
        if (r_flag) {
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
            std::cout << "   Use -r para crear las carpetas automáticamente" << std::endl;
            return false;
        }
    }
    
    // 7. Verificar si el archivo ya existe
    int existingInode;
    if (writer.findFileInFolder(parentInode, fileName, existingInode)) {
        writer.close();
        lastError = "Error: El archivo ya existe: " + fileName;
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 8. Obtener contenido
    std::string content;
    if (hasCont) {
        // -cont tiene prioridad
        if (!readFileFromPC(contPath, content)) {
            writer.close();
            std::cout << "❌ " << lastError << std::endl;
            return false;
        }
    } else if (hasSize) {
        content = generateContent(size);
    }
    // Si no hay -cont ni -size, el archivo queda vacío (0 bytes)
    
    // 9. Crear el archivo
    int newInode;
    if (!writer.createFile(parentInode, fileName, 
                           g_activeSession.uid, 
                           g_activeSession.gid,
                           content, newInode)) {
        writer.close();
        lastError = "Error: No se pudo crear el archivo";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }

    writer.writeJournal("mkfile", path, content.substr(0, 64));
    
    writer.close();
    
    std::cout << "✅ Archivo creado exitosamente:" << std::endl;
    std::cout << "   Ruta: " << path << std::endl;
    std::cout << "   Tamaño: " << content.size() << " bytes" << std::endl;
    std::cout << "   Inodo: " << newInode << std::endl;
    
    return true;
}