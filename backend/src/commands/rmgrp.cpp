#include "../../include/commands/rmgrp.h"
#include "../../include/disk/session.h"
#include "../../include/ext2/ext2_writer.h"
#include <iostream>
#include <sstream>
#include <algorithm>

RMGRP::RMGRP() {}

bool RMGRP::eliminarGrupo(const std::string& name) {
    std::cout << "🔍 Eliminando grupo..." << std::endl;
    std::cout << "   Nombre: " << name << std::endl;
    
    // 1. Verificar sesión activa
    if (!g_activeSession.isActive) {
        lastError = "Error: No hay sesión activa. Use LOGIN primero.";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 2. Verificar que sea root
    if (g_activeSession.uid != 1) {
        lastError = "Error: Solo el usuario root puede eliminar grupos.";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 3. Abrir users.txt
    Ext2Writer writer;
    if (!writer.openForWrite(g_activeSession.diskPath, 
                             g_activeSession.partition.part_start)) {
        lastError = "Error: No se pudo abrir la partición EXT2";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 4. Leer users.txt
    std::vector<std::string> lines;
    if (!writer.readUsersFile(lines)) {
        writer.close();
        lastError = "Error: No se pudo leer users.txt";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 5. Buscar el grupo
    int gid;
    if (!writer.findGroup(lines, name, gid)) {
        writer.close();
        lastError = "Error: Grupo no encontrado: " + name;
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 6. No permitir eliminar root
    if (gid == 1) {
        writer.close();
        lastError = "Error: No se puede eliminar el grupo root";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 7. Verificar que no tenga usuarios
    for (const auto& line : lines) {
        if (line.empty()) continue;
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string part;
        while (std::getline(ss, part, ',')) {
            parts.push_back(part);
        }
        if (parts.size() >= 3) {
            std::string type = parts[1];
            type.erase(remove(type.begin(), type.end(), ' '), type.end());
            if (type == "U") {
                try {
                    int userGID = std::stoi(parts[2]);
                    if (userGID == gid) {
                        writer.close();
                        lastError = "Error: El grupo tiene usuarios, no se puede eliminar";
                        std::cout << "❌ " << lastError << std::endl;
                        return false;
                    }
                } catch (...) {}
            }
        }
    }
    
    // 8. Marcar como eliminado (GID = 0)
    std::string newLine = "0, G, " + name;
    bool encontrado = false;
    for (auto& line : lines) {
        if (line.empty()) continue;
        if (line.find(name) != std::string::npos) {
            line = newLine;
            encontrado = true;
            break;
        }
    }
    
    if (!encontrado) {
        writer.close();
        lastError = "Error: Grupo no encontrado: " + name;
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 9. Escribir users.txt
    if (!writer.writeUsersFile(lines)) {
        writer.close();
        lastError = "Error: No se pudo escribir users.txt";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    writer.close();
    
    std::cout << "✅ Grupo eliminado exitosamente:" << std::endl;
    std::cout << "   Nombre: " << name << std::endl;
    
    return true;
}