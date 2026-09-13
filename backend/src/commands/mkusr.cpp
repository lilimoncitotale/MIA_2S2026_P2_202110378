#include "../../include/commands/mkusr.h"
#include "../../include/disk/session.h"
#include "../../include/ext2/ext2_writer.h"
#include <iostream>
#include <sstream>

MKUSR::MKUSR() {}

bool MKUSR::crearUsuario(const std::string& username, 
                         const std::string& password,
                         const std::string& group) {
    std::cout << "🔍 Creando usuario..." << std::endl;
    std::cout << "   Usuario: " << username << std::endl;
    std::cout << "   Grupo: " << group << std::endl;
    
    // 1. Verificar sesión activa
    if (!g_activeSession.isActive) {
        lastError = "Error: No hay sesión activa. Use LOGIN primero.";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 2. Verificar que sea root
    if (g_activeSession.uid != 1) {
        lastError = "Error: Solo el usuario root puede crear usuarios.";
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
    
    // 5. Verificar que el usuario no exista
    int existingUID;
    if (writer.findUser(lines, username, existingUID)) {
        writer.close();
        lastError = "Error: El usuario ya existe: " + username;
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 6. Verificar que el grupo exista
    int gid;
    if (!writer.findGroup(lines, group, gid)) {
        writer.close();
        lastError = "Error: Grupo no encontrado: " + group;
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 7. Obtener nuevo UID
    int newUID = writer.getNextUID(lines);
    
    // 8. Agregar el usuario
    std::string newLine = std::to_string(newUID) + ", U, " + group + ", " + username + ", " + password;
    lines.push_back(newLine);
    
    // 9. Escribir users.txt
    if (!writer.writeUsersFile(lines)) {
        writer.close();
        lastError = "Error: No se pudo escribir users.txt";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    writer.close();
    
    std::cout << "✅ Usuario creado exitosamente:" << std::endl;
    std::cout << "   Usuario: " << username << std::endl;
    std::cout << "   Grupo: " << group << std::endl;
    std::cout << "   UID: " << newUID << std::endl;
    std::cout << "   GID: " << gid << std::endl;
    
    return true;
}