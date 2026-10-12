#include "../../include/commands/chgrp.h"
#include "../../include/disk/session.h"
#include "../../include/ext2/ext2_writer.h"
#include <iostream>
#include <sstream>

CHGRP::CHGRP() {}

bool CHGRP::cambiarGrupo(const std::string& username, const std::string& group) {
    std::cout << "🔍 Cambiando grupo de usuario..." << std::endl;
    std::cout << "   Usuario: " << username << std::endl;
    std::cout << "   Nuevo grupo: " << group << std::endl;
    
    // 1. Verificar sesión activa
    if (!g_activeSession.isActive) {
        lastError = "Error: No hay sesión activa. Use LOGIN primero.";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 2. Verificar que sea root
    if (g_activeSession.uid != 1) {
        lastError = "Error: Solo el usuario root puede cambiar grupos.";
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
    
    // 5. Verificar que el usuario exista
    int uid;
    if (!writer.findUser(lines, username, uid)) {
        writer.close();
        lastError = "Error: Usuario no encontrado: " + username;
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 6. No permitir cambiar grupo de root
    if (uid == 1) {
        writer.close();
        lastError = "Error: No se puede cambiar el grupo del usuario root";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 7. Verificar que el grupo exista
    int gid;
    if (!writer.findGroup(lines, group, gid)) {
        writer.close();
        lastError = "Error: Grupo no encontrado: " + group;
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 8. Actualizar el usuario
    bool encontrado = false;
    for (auto& line : lines) {
        if (line.empty()) continue;
        if (line.find(username) != std::string::npos) {
            std::vector<std::string> parts;
            std::stringstream ss(line);
            std::string part;
            while (std::getline(ss, part, ',')) {
                parts.push_back(part);
            }
            if (parts.size() >= 5) {
                std::string newLine = parts[0] + ", U, " + group + ", " + username + ", " + parts[4];
                line = newLine;
                encontrado = true;
                break;
            }
        }
    }
    
    if (!encontrado) {
        writer.close();
        lastError = "Error: Usuario no encontrado: " + username;
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
    writer.writeJournal("chgrp", "/users.txt", username + "->" + group);
    
    writer.close();
    
    std::cout << "✅ Grupo cambiado exitosamente:" << std::endl;
    std::cout << "   Usuario: " << username << std::endl;
    std::cout << "   Nuevo grupo: " << group << std::endl;
    std::cout << "   GID: " << gid << std::endl;
    
    return true;
}