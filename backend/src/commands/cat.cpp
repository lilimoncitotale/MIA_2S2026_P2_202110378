#include "../../include/commands/cat.h"
#include "../../include/disk/session.h"
#include "../../include/ext2/ext2_reader.h"
#include <iostream>
#include <algorithm>

CAT::CAT() {}

bool CAT::leerArchivos(const std::map<std::string, std::string>& params) {
    output.clear();
    std::cout << "🔍 Leyendo archivos..." << std::endl;
    
    // 1. Verificar sesión activa
    if (!g_activeSession.isActive) {
        lastError = "Error: No hay sesión activa. Use LOGIN primero.";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 2. Recolectar los archivos (-file1, -file2, ...)
    std::vector<std::pair<int, std::string>> files;  // (número, ruta)
    
    for (const auto& [key, value] : params) {
        // Verificar que sea -fileN
        if (key.find("-file") == 0 && key.length() > 5) {
            try {
                int num = std::stoi(key.substr(5));
                files.push_back({num, value});
            } catch (...) {
                continue;
            }
        }
    }
    
    if (files.empty()) {
        lastError = "Error: No se especificó ningún archivo (-file1, -file2, ...)";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // Ordenar por número (file1, file2, ...)
    std::sort(files.begin(), files.end());
    
    // 3. Abrir la partición EXT2
    Ext2Reader reader;
    if (!reader.open(g_activeSession.diskPath, 
                     g_activeSession.partition.part_start)) {
        lastError = "Error: No se pudo abrir la partición EXT2";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 4. Leer cada archivo
    bool exito = true;
    for (const auto& [num, filePath] : files) {
        std::cout << "   📄 Leyendo: " << filePath << std::endl;
        
        // Parsear la ruta
        std::string fileName = filePath;
        if (!fileName.empty() && fileName[0] == '/') {
            fileName = fileName.substr(1);
        }
        
        // Si tiene más "/", es una ruta anidada
        // TODO: Soportar rutas anidadas navegando carpetas
        size_t lastSlash = fileName.find_last_of('/');
        if (lastSlash != std::string::npos) {
            fileName = fileName.substr(lastSlash + 1);
        }
        
        std::string content;
        if (reader.readFile(fileName, content)) {
            output += content;
            if (!content.empty() && content.back() != '\n') output += '\n';
            // Mostrar contenido
            std::cout << content;
            // Asegurar salto de línea al final
            if (!content.empty() && content.back() != '\n') {
                std::cout << std::endl;
            }
        } else {
            std::cout << "❌ Error: No se pudo leer el archivo: " << filePath << std::endl;
            exito = false;
        }
    }
    
    reader.close();
    
    return exito;
}