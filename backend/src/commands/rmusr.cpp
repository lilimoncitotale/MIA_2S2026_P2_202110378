#include "../../include/commands/rmusr.h"
#include "../../include/disk/session.h"
#include "../../include/ext2/ext2_writer.h"
#include <iostream>
#include <sstream>

RMUSR::RMUSR() {}

bool RMUSR::eliminarUsuario(const std::string& username) {
    std::cout << "🔍 Eliminando usuario..." << std::endl;
    std::cout << "   Usuario: " << username << std::endl;
    
    // 1. Verificar sesión activa
    if (!g_activeSession.isActive) {
        lastError = "Error: No hay sesión activa. Use LOGIN primero.";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 2. Verificar que sea root
    if (g_activeSession.uid != 1) {
        lastError = "Error: Solo el usuario root puede eliminar usuarios.";
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
    
    // 5. Buscar el usuario
    int uid;
    if (!writer.findUser(lines, username, uid)) {
        writer.close();
        lastError = "Error: Usuario no encontrado: " + username;
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 6. No permitir eliminar root
    if (uid == 1) {
        writer.close();
        lastError = "Error: No se puede eliminar el usuario root";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 7. Marcar como eliminado (UID = 0)
    bool encontrado = false;
    for (auto& line : lines) {
        if (line.empty()) continue;
        if (line.find(username) != std::string::npos) {
            // Reemplazar con "0, U, ..."
            std::vector<std::string> parts;
            std::stringstream ss(line);
            std::string part;
            while (std::getline(ss, part, ',')) {
                parts.push_back(part);
            }
            if (parts.size() >= 5) {
                std::string newLine = "0, U, " + parts[2] + ", " + username + ", " + parts[4];
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
    
    // 8. Escribir users.txt
    if (!writer.writeUsersFile(lines)) {
        writer.close();
        lastError = "Error: No se pudo escribir users.txt";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    writer.close();
    
    std::cout << "✅ Usuario eliminado exitosamente:" << std::endl;
    std::cout << "   Usuario: " << username << std::endl;
    
    return true;
}