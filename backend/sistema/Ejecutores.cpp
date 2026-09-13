#include "Ejecutores.h"
#include "../include/commands/mkdisk.h"
#include "../include/commands/fdisk.h"
#include "../include/commands/mount.h"
#include "../include/commands/mkfs.h"
#include "../include/commands/login.h"
#include "../include/commands/logout.h"
#include "../include/disk/mounted.h"
#include "../include/disk/session.h"
#include <sstream>
#include <fstream>
#include <cstdio>
#include <map>
#include <iostream>
#include "../include/commands/mkdir.h"
#include "../include/commands/mkfile.h"
#include "../include/commands/mkusr.h"
#include "../include/commands/rmusr.h"
#include "../include/commands/mkgrp.h"
#include "../include/commands/rmgrp.h"
#include "../include/commands/chgrp.h"
#include "../include/commands/rep.h"
#include "../include/commands/cat.h"

// ============================================
// IMPLEMENTACIÓN DE RMDISK
// ============================================
std::string ejecutarRMDISK(const ParametrosComando& params) {
    std::stringstream output;
    if (!params.path_presente || params.path.empty()) {
        output << " Error: Falta -path" << std::endl;
        return output.str();
    }

    if (std::remove(params.path.c_str()) == 0) {
        output << "Disco eliminado exitosamente" << std::endl;
    } else {
        output << " Error: No se pudo eliminar el disco" << std::endl;
    }
    return output.str();
}

// ============================================
// IMPLEMENTACIÓN DE MKDISK
// ============================================
std::string ejecutarMKDISK(const ParametrosComando& params) {
    std::stringstream output;
    
    try {
        MkDisk mkdisk;
        
        int size = std::stoi(params.size);
        std::string unit = params.unit_presente ? params.unit : "M";
        std::string fit = params.fit_presente ? params.fit : "FF";
        std::string path = params.path;
        
        if (size <= 0) {
            output << " Error: El tamaño debe ser mayor a 0" << std::endl;
            return output.str();
        }
        
        if (path.empty()) {
            output << " Error: La ruta no puede estar vacía" << std::endl;
            return output.str();
        }
        
        mkdisk.crearDisco(size, fit, unit, path);
        output << "Disco creado exitosamente" << std::endl;
        
    } catch (const std::exception& e) {
        output << " Error: " << e.what() << std::endl;
    }
    
    return output.str();
}

// ============================================
// IMPLEMENTACIÓN DE FDISK
// ============================================
// En sistema/Ejecutores.cpp
std::string ejecutarFDISK(const ParametrosComando& params) {
    std::stringstream output;
    
    try {
        if (!params.path_presente) {
            output << "❌ Error: Falta -path" << std::endl;
            return output.str();
        }
        if (!params.name_presente) {
            output << "❌ Error: Falta -name" << std::endl;
            return output.str();
        }
        if (!params.size_presente) {
            output << "❌ Error: Falta -size" << std::endl;
            return output.str();
        }
        
        Fdisk fdisk;
        
        int size = std::stoi(params.size);  // ← Tamaño SIN convertir
        std::string unit = params.unit_presente ? params.unit : "M";
        std::string fit = params.fit_presente ? params.fit : "WF";
        std::string type = params.type_presente ? params.type : "P";
        std::string path = params.path;
        std::string name = params.name;
        
        // ✅ NO convertir aquí. Fdisk lo hace internamente.
        char unitChar = unit[0];
        char fitChar = fit[0];
        char typeChar = type[0];
        
        std::cout << "🔍 Enviando a Fdisk: size=" << size << ", unit=" << unitChar << std::endl;
        
        if (fdisk.crearParticion(path, size, fitChar, unitChar, typeChar, name)) {
            output << "✅ Partición creada exitosamente" << std::endl;
            output << "   Nombre: " << name << std::endl;
            output << "   Tipo: " << type << std::endl;
        } else {
            output << "❌ Error: " << fdisk.getLastError() << std::endl;
        }
        
    } catch (const std::exception& e) {
        output << "❌ Error: " << e.what() << std::endl;
    }
    
    return output.str();
}

// ============================================
// IMPLEMENTACIÓN DE MOUNT
// ============================================
std::string ejecutarMOUNT(const ParametrosComando& params) {
    std::stringstream output;
    
    try {
        if (!params.path_presente) {
            output << " Error: Falta -path" << std::endl;
            return output.str();
        }
        if (!params.name_presente) {
            output << " Error: Falta -name" << std::endl;
            return output.str();
        }
        
        Mount mount;
        std::string path = params.path;
        std::string name = params.name;
        
        if (mount.montar(path, name)) {
            MountedPartition* mounted = g_mountedManager.findByDiskAndName(path, name);
            if (mounted) {
                output << "Partición montada exitosamente" << std::endl;
                output << "   ID: " << mounted->id << std::endl;
            }
        } else {
            output << " Error: No se pudo montar la partición" << std::endl;
        }
        
    } catch (const std::exception& e) {
        output << " Error: " << e.what() << std::endl;
    }
    
    return output.str();
}

// ============================================
// IMPLEMENTACIÓN DE MOUNTED
// ============================================
std::string ejecutarMOUNTED(const ParametrosComando& params) {
    std::stringstream output;
    
    try {
        output << "=========================================" << std::endl;
        output << "   PARTICIONES MONTADAS" << std::endl;
        output << "=========================================" << std::endl;
        
        const auto& mounted = g_mountedManager.getMounted();
        
        if (mounted.empty()) {
            output << " No hay particiones montadas." << std::endl;
        } else {
            for (const auto& mp : mounted) {
                output << "ID: " << mp.id << std::endl;
                output << "  Disco: " << mp.diskPath << std::endl;
                output << "  Partición: " << mp.partitionName << std::endl;
                output << "-----------------------------------------" << std::endl;
            }
            output << "Total: " << mounted.size() << " particiones montadas" << std::endl;
        }
        output << "=========================================" << std::endl;
        
    } catch (const std::exception& e) {
        output << "Error: " << e.what() << std::endl;
    }
    
    return output.str();
}

// ============================================
// IMPLEMENTACIÓN DE MKFS
// ============================================
std::string ejecutarMKFS(const ParametrosComando& params) {
    std::stringstream output;
    
    try {
        if (!params.id_presente) {
            output << " Error: Falta -id" << std::endl;
            return output.str();
        }
        
        MKFS mkfs;
        std::string id = params.id;
        std::string type = params.type_format_presente ? params.type_format : "full";
        
        if (mkfs.formatear(id, type)) {
            output << "Partición formateada exitosamente como EXT2" << std::endl;
        } else {
            output << " Error: " << mkfs.getLastError() << std::endl;
        }
        
    } catch (const std::exception& e) {
        output << " Error: " << e.what() << std::endl;
    }
    
    return output.str();
}

// ============================================
// IMPLEMENTACIÓN DE LOGIN
// ============================================
std::string ejecutarLOGIN(const ParametrosComando& params) {
    std::stringstream output;
    
    try {
        if (!params.id_presente) {
            output << " Error: Falta -id" << std::endl;
            return output.str();
        }
        if (!params.user_presente) {
            output << " Error: Falta -user" << std::endl;
            return output.str();
        }
        if (!params.pass_presente) {
            output << " Error: Falta -pass" << std::endl;
            return output.str();
        }
        
        Login login;
        std::string id = params.id;
        std::string username = params.user;
        std::string password = params.pass;
        
        if (login.iniciarSesion(id, username, password)) {
            output << " Sesión iniciada exitosamente" << std::endl;
            output << "   Usuario: " << username << std::endl;
        } else {
            output << " Error: " << login.getLastError() << std::endl;
        }
        
    } catch (const std::exception& e) {
        output << " Error: " << e.what() << std::endl;
    }
    
    return output.str();
}

// ============================================
// IMPLEMENTACIÓN DE LOGOUT
// ============================================
std::string ejecutarLOGOUT(const ParametrosComando& params) {
    std::stringstream output;
    
    try {
        Logout logout;
        
        if (logout.cerrarSesion()) {
            output << " Sesión cerrada exitosamente" << std::endl;
        } else {
            output << "Error: " << logout.getLastError() << std::endl;
        }
        
    } catch (const std::exception& e) {
        output << " Error: " << e.what() << std::endl;
    }
    
    return output.str();
}

// ============================================
// IMPLEMENTACIÓN DE MKUSR (PENDIENTE)
// ============================================
std::string ejecutarMKUSR(const ParametrosComando& params) {
    std::stringstream output;
    MKUSR command;
    if (command.crearUsuario(params.user, params.pass, params.grp)) output << " Usuario creado exitosamente\n";
    else output << "Error: " << command.getLastError() << "\n";
    return output.str();
}

// ============================================
// IMPLEMENTACIÓN DE RMUSR (PENDIENTE)
// ============================================
std::string ejecutarRMUSR(const ParametrosComando& params) {
    std::stringstream output;
    RMUSR command;
    if (command.eliminarUsuario(params.user)) output << " Usuario eliminado exitosamente\n";
    else output << "Error: " << command.getLastError() << "\n";
    return output.str();
}

// ============================================
// IMPLEMENTACIÓN DE MKFILE (PENDIENTE)
// ============================================
std::string ejecutarMKFILE(const ParametrosComando& params) {
    std::stringstream output;
    MKFILE command;
    std::map<std::string, std::string> values{{"-path", params.path}};
    if (params.size_presente) values["-size"] = params.size;
    if (params.cont_presente) values["-cont"] = params.cont;
    if (params.r_flag_presente) values["-r"] = "true";
    if (command.crearArchivo(values)) output << " Archivo creado exitosamente\n";
    else output << "Error: " << command.getLastError() << "\n";
    return output.str();
}

std::string ejecutarMKDIR(const ParametrosComando& params) {
    std::stringstream output;
    
    try {
        if (!params.path_presente) {
            output << " Error: Falta -path" << std::endl;
            return output.str();
        }
        
        MKDIR mkdir;
        std::map<std::string, std::string> cmdParams;
        cmdParams["-path"] = params.path;
        if (params.r_flag_presente) {
            cmdParams["-p"] = "true";
        }
        
        if (mkdir.crearCarpeta(cmdParams)) {
            output << "✅ Carpeta creada exitosamente" << std::endl;
        } else {
            output << "❌ Error: " << mkdir.getLastError() << std::endl;
        }
        
    } catch (const std::exception& e) {
        output << "❌ Error: " << e.what() << std::endl;
    }
    
    return output.str();
}

std::string ejecutarMKGRP(const ParametrosComando& params) {
    MKGRP command;
    return command.crearGrupo(params.name) ? " Grupo creado exitosamente\n" : "Error: " + command.getLastError() + "\n";
}

std::string ejecutarRMGRP(const ParametrosComando& params) {
    RMGRP command;
    return command.eliminarGrupo(params.name) ? " Grupo eliminado exitosamente\n" : "Error: " + command.getLastError() + "\n";
}

std::string ejecutarCHGRP(const ParametrosComando& params) {
    CHGRP command;
    return command.cambiarGrupo(params.user, params.grp) ? " Grupo cambiado exitosamente\n" : "Error: " + command.getLastError() + "\n";
}
// ============================================
// IMPLEMENTACIÓN DE REP
// ============================================
std::string ejecutarREP(const ParametrosComando& params) {
    std::stringstream output;
    
    try {
        if (!params.id_presente) {
            output << "❌ Error: Falta -id" << std::endl;
            return output.str();
        }
        if (!params.path_presente) {
            output << "❌ Error: Falta -path" << std::endl;
            return output.str();
        }
        if (!params.name_presente) {
            output << "❌ Error: Falta -name" << std::endl;
            return output.str();
        }
        
        REP rep;
        std::map<std::string, std::string> cmdParams;
        cmdParams["-id"] = params.id;
        cmdParams["-path"] = params.path;
        cmdParams["-name"] = params.name;
        
        if (params.cont_presente) {
            cmdParams["-path_file_ls"] = params.cont;
        }
        
        if (rep.generar(cmdParams)) {
            output << "✅ Reporte generado exitosamente" << std::endl;
            output << "   Nombre: " << params.name << std::endl;
            output << "   Ruta: " << params.path << std::endl;
        } else {
            output << "❌ Error: " << rep.getLastError() << std::endl;
        }
        
    } catch (const std::exception& e) {
        output << "❌ Error: " << e.what() << std::endl;
    }
    
    return output.str();
}
std::string ejecutarCAT(const ParametrosComando& params) {
    std::stringstream output;
    
    try {
        if (!params.files_presente) {
            output << "❌ Error: Falta al menos un -fileN" << std::endl;
            return output.str();
        }
        
        CAT cat;
        std::map<std::string, std::string> cmdParams;
        for (const auto& [num, path] : params.files) {
            cmdParams["-file" + std::to_string(num)] = path;
        }
        
        if (cat.leerArchivos(cmdParams)) {
            output << "✅ Archivos leídos exitosamente" << std::endl;
        } else {
            output << "❌ Error: " << cat.getLastError() << std::endl;
        }
        
    } catch (const std::exception& e) {
        output << "❌ Error: " << e.what() << std::endl;
    }
    
    return output.str();
}