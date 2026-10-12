#include "../../include/commands/mkgrp.h"
#include "../../include/disk/session.h"
#include "../../include/ext2/ext2_writer.h"
#include <iostream>
#include <sstream>

MKGRP::MKGRP() {}

bool MKGRP::crearGrupo(const std::string& name) {
    std::cout << "🔍 Creando grupo..." << std::endl;
    std::cout << "   Nombre: " << name << std::endl;
    
    // 1. Verificar sesión activa
    if (!g_activeSession.isActive) {
        lastError = "Error: No hay sesión activa. Use LOGIN primero.";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 2. Verificar que sea root (UID = 1)
    if (g_activeSession.uid != 1) {
        lastError = "Error: Solo el usuario root puede crear grupos.";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 3. Abrir el archivo users.txt
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
    
    // 5. Verificar que el grupo no exista
    int existingGID;
    if (writer.findGroup(lines, name, existingGID)) {
        writer.close();
        lastError = "Error: El grupo ya existe: " + name;
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 6. Obtener nuevo GID
    int newGID = writer.getNextGID(lines);
    
    // 7. Agregar el grupo
    std::string newLine = std::to_string(newGID) + ", G, " + name;
    lines.push_back(newLine);
    
    // 8. Escribir users.txt
    if (!writer.writeUsersFile(lines)) {
        writer.close();
        lastError = "Error: No se pudo escribir users.txt";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    writer.writeJournal("mkgrp", "/users.txt", name);
    
    writer.close();
    
    // 9. Mostrar éxito
    std::cout << "✅ Grupo creado exitosamente:" << std::endl;
    std::cout << "   Nombre: " << name << std::endl;
    std::cout << "   GID: " << newGID << std::endl;
    
    return true;
}