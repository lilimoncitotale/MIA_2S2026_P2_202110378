
#include "Ejecutores.h"
#include <iostream>
#include <sstream>

// Instancia global del sistema
SistemaArchivos sistema("202110378");

std::string ejecutarMKDISK(const ParametrosComando& params) {
    // Verificar que el disco no exista
    if (sistema.discoExisteEnMemoria(params.path)) {
        return "Error: El disco " + params.path + " ya existe";
    }
    
    // Obtener parámetros
    int tamanio = std::stoi(params.size);
    char unidad = params.unit.empty() ? 'M' : params.unit[0];
    char ajuste = params.fit.empty() ? 'F' : params.fit[0];
    
    // Crear el disco
    if (sistema.crearDisco(params.path, tamanio, unidad, ajuste)) {
        std::ostringstream msg;
        msg << " Disco creado exitosamente:\n";
        msg << "   Ruta: " << params.path << "\n";
        msg << "   Tamaño: " << tamanio << unidad << "\n";
        msg << "   Ajuste: " << ajuste << "\n";
        return msg.str();
    } else {
        return " Error al crear el disco";
    }
}

std::string ejecutarRMDISK(const ParametrosComando& params) {
    if (!sistema.discoExisteEnMemoria(params.path)) {
        return "Error: El disco " + params.path + " no existe";
    }
    
    if (sistema.eliminarDisco(params.path)) {
        return "Disco eliminado exitosamente: " + params.path;
    } else {
        return "Error: No se puede eliminar el disco (tiene particiones montadas)";
    }
}

std::string ejecutarFDISK(const ParametrosComando& params) {
    // Verificar que el disco exista
    if (!sistema.discoExisteEnMemoria(params.path)) {
        return "Error: El disco " + params.path + " no existe";
    }
    
    // Verificar que la partición no exista
    if (sistema.particionExiste(params.path, params.name)) {
        return "Error: La partición " + params.name + " ya existe en " + params.path;
    }
    
    // Obtener parámetros
    int tamanio = std::stoi(params.size);
    char tipo = params.type.empty() ? 'P' : params.type[0];
    char unidad = params.unit.empty() ? 'K' : params.unit[0];
    char ajuste = params.fit.empty() ? 'F' : params.fit[0];
    
    // Crear la partición
    if (sistema.crearParticion(params.path, params.name, tamanio, tipo, unidad, ajuste)) {
        std::ostringstream msg;
        msg << "Partición creada exitosamente:\n";
        msg << "   Nombre: " << params.name << "\n";
        msg << "   Tamaño: " << tamanio << unidad << " (" 
            << sistema.calcularTamanioEnBytes(tamanio, unidad) << " bytes)\n";
        msg << "   Tipo: " << tipo << "\n";
        msg << "  Ajuste: " << ajuste << "\n";
        msg << "  En disco: " << params.path;
        return msg.str();
    } else {
        return " Error al crear la partición (posiblemente espacio insuficiente)";
    }
}

std::string ejecutarMOUNT(const ParametrosComando& params) {
    // Verificar que el disco exista
    if (!sistema.discoExisteEnMemoria(params.path)) {
        return "Error: El disco " + params.path + " no existe";
    }
    
    // Verificar que la partición exista
    if (!sistema.particionExiste(params.path, params.name)) {
        return "Error: La partición " + params.name + " no existe en " + params.path;
    }
    
    // Verificar que no esté montada
    if (sistema.estaMontada(params.path, params.name)) {
        return "Error: La partición " + params.name + " ya está montada";
    }
    
    // Montar la partición
    std::string id = sistema.montarParticion(params.path, params.name);
    if (!id.empty()) {
        std::ostringstream msg;
        msg << " Partición montada exitosamente:\n";
        msg << "   ID: " << id << "\n";
        msg << "   Partición: " << params.name << "\n";
        msg << "   Disco: " << params.path;
        return msg.str();
    } else {
        return " Error al montar la partición";
    }
}

std::string ejecutarMKFS(const ParametrosComando& params) {
    // Verificar que el ID exista y esté montado
    Montaje* montaje = sistema.buscarMontaje(params.id);
    if (!montaje) {
        return "Error: El ID " + params.id + " no está montado";
    }
    
    // Formatear la partición
    if (sistema.formatearParticion(params.id)) {
        std::ostringstream msg;
        msg << "Partición formateada como EXT2:\n";
        msg << "   ID: " << params.id << "\n";
        msg << "   Partición: " << montaje->nombreParticion << "\n";
        msg << "   Disco: " << montaje->rutaDisco;
        return msg.str();
    } else {
        return " Error al formatear la partición";
    }
}

std::string ejecutarMKUSR(const ParametrosComando& params) {
    // Verificar que el ID exista
    Montaje* montaje = sistema.buscarMontaje(params.id);
    if (!montaje) {
        return "Error: El ID " + params.id + " no está montado";
    }
    
    // Crear el usuario
    if (sistema.crearUsuario(params.id, params.user, params.pass, params.grp)) {
        std::ostringstream msg;
        msg << "Usuario creado exitosamente:\n";
        msg << "   Usuario: " << params.user << "\n";
        msg << "   Contraseña: " << params.pass << "\n";
        msg << "   Grupo: " << params.grp << "\n";
        msg << "   En partición: " << params.id;
        return msg.str();
    } else {
        return " Error: El usuario " + params.user + " ya existe";
    }
}

std::string ejecutarRMUSR(const ParametrosComando& params) {
    // Verificar que el ID exista
    Montaje* montaje = sistema.buscarMontaje(params.id);
    if (!montaje) {
        return "Error: El ID " + params.id + " no está montado";
    }
    
    // Eliminar el usuario
    if (sistema.eliminarUsuario(params.id, params.user)) {
        return " Usuario eliminado exitosamente: " + params.user;
    } else {
        return " Error: El usuario " + params.user + " no existe";
    }
}

std::string ejecutarMKFILE(const ParametrosComando& params) {
    // Verificar que el ID exista
    Montaje* montaje = sistema.buscarMontaje(params.id);
    if (!montaje) {
        return "Error: El ID " + params.id + " no está montado";
    }
    
    // Verificar que el archivo no exista
    if (sistema.archivoExiste(params.id, params.path)) {
        return "Error: El archivo " + params.path + " ya existe";
    }
    
    // Obtener parámetros
    int tamanio = params.size_presente ? std::stoi(params.size) : 0;
    std::string contenido = params.cont_presente ? params.cont : "";
    
    // Crear el archivo
    if (sistema.crearArchivo(params.id, params.path, tamanio, params.r_flag, contenido)) {
        std::ostringstream msg;
        msg << " Archivo creado exitosamente:\n";
        msg << "   Ruta: " << params.path << "\n";
        msg << "  Tamaño: " << tamanio << " bytes\n";
        msg << "   En partición: " << params.id;
        if (params.r_flag) {
            msg << "\n   Se crearon las carpetas recursivamente";
        }
        return msg.str();
    } else {
        return " Error al crear el archivo";
    }
}
