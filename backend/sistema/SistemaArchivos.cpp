
#include "SistemaArchivos.h"
#include <iostream>
#include <sstream>
#include <iomanip>

// Constructor
SistemaArchivos::SistemaArchivos(const std::string& carnet) 
    : contadorMontajes(0), carnet(carnet) {}

// --- MÉTODOS AUXILIARES ---

bool SistemaArchivos::discoExiste(const std::string& ruta) {
    for (const auto& disco : discos) {
        if (disco.ruta == ruta) {
            return true;
        }
    }
    return false;
}

Disco* SistemaArchivos::buscarDisco(const std::string& ruta) {
    for (auto& disco : discos) {
        if (disco.ruta == ruta) {
            return &disco;
        }
    }
    return nullptr;
}

Particion* SistemaArchivos::buscarParticion(Disco* disco, const std::string& nombre) {
    if (!disco) return nullptr;
    for (auto& particion : disco->particiones) {
        if (particion.nombre == nombre) {
            return &particion;
        }
    }
    return nullptr;
}

std::string SistemaArchivos::generarID() {
    contadorMontajes++;
    // Formato: CARNET + Letra (A, B, C, ...)
    char letra = 'A' + (contadorMontajes - 1);
    return carnet + letra;
}

int SistemaArchivos::calcularEspacioLibre(Disco* disco) {
    if (!disco) return 0;
    int usado = 0;
    for (const auto& p : disco->particiones) {
        usado += p.tamanio;
    }
    return disco->tamanio - usado;
}

int SistemaArchivos::calcularTamanioEnBytes(int tamanio, char unidad) {
    switch(unidad) {
        case 'K': return tamanio * 1024;
        case 'M': return tamanio * 1024 * 1024;
        case 'B': return tamanio;
        default: return tamanio * 1024 * 1024; // M por defecto
    }
}

// --- MÉTODOS PARA DISCOS ---

bool SistemaArchivos::crearDisco(const std::string& ruta, int tamanio, char unidad, char ajuste) {
    // Verificar que no exista
    if (discoExiste(ruta)) {
        return false;
    }
    
    Disco nuevo;
    nuevo.ruta = ruta;
    nuevo.tamanio = tamanio;
    nuevo.unidad = unidad;
    nuevo.ajuste = ajuste;
    nuevo.espacio_libre = tamanio;
    
    discos.push_back(nuevo);
    
    std::cout << "[Sistema] Disco creado: " << ruta 
              << " (" << tamanio << unidad << ", ajuste: " << ajuste << ")" << std::endl;
    
    return true;
}

bool SistemaArchivos::eliminarDisco(const std::string& ruta) {
    for (auto it = discos.begin(); it != discos.end(); ++it) {
        if (it->ruta == ruta) {
            // Verificar que no tenga particiones montadas
            for (const auto& p : it->particiones) {
                if (p.montada) {
                    std::cout << "[Sistema] No se puede eliminar: particion " << p.nombre 
                              << " está montada" << std::endl;
                    return false;
                }
            }
            discos.erase(it);
            std::cout << "[Sistema] Disco eliminado: " << ruta << std::endl;
            return true;
        }
    }
    return false;
}

bool SistemaArchivos::discoExisteEnMemoria(const std::string& ruta) {
    return discoExiste(ruta);
}

std::vector<Disco> SistemaArchivos::getDiscos() const {
    return discos;
}

// --- MÉTODOS PARA PARTICIONES ---

bool SistemaArchivos::crearParticion(const std::string& rutaDisco, const std::string& nombre, 
                                    int tamanio, char tipo, char unidad, char ajuste) {
    // Buscar el disco
    Disco* disco = buscarDisco(rutaDisco);
    if (!disco) {
        std::cout << "[Sistema] Error: Disco " << rutaDisco << " no existe" << std::endl;
        return false;
    }
    
    // Verificar que la partición no exista
    if (buscarParticion(disco, nombre)) {
        std::cout << "[Sistema] Error: Particion " << nombre << " ya existe" << std::endl;
        return false;
    }
    
    // Calcular tamaño en bytes CORRECTAMENTE
    int tamanioBytes = tamanio;
    if (unidad == 'K') tamanioBytes *= 1024;
    else if (unidad == 'M') tamanioBytes *= 1024 * 1024;
    else if (unidad == 'B') tamanioBytes = tamanio;
    // Si no se especifica unidad, por defecto K (según el enunciado)
    else tamanioBytes = tamanio * 1024;
    
    // Calcular espacio libre en bytes
    int espacioLibre = disco->tamanio;
    if (disco->unidad == 'K') espacioLibre *= 1024;
    else if (disco->unidad == 'M') espacioLibre *= 1024 * 1024;
    
    for (const auto& p : disco->particiones) {
        espacioLibre -= p.tamanio;
    }
    
    std::cout << "[DEBUG] Espacio libre: " << espacioLibre << " bytes, Necesario: " << tamanioBytes << " bytes" << std::endl;
    
    if (tamanioBytes > espacioLibre) {
        std::cout << "[Sistema] Error: Espacio insuficiente. Libre: " 
                  << espacioLibre << " bytes, Necesario: " << tamanioBytes << " bytes" << std::endl;
        return false;
    }
    
    // Crear la partición
    Particion nueva;
    nueva.nombre = nombre;
    nueva.tamanio = tamanioBytes;
    nueva.tipo = tipo;
    nueva.ajuste = ajuste;
    nueva.montada = false;
    
    // Calcular inicio (simplificado: al final del espacio usado)
    int inicio = 0;
    for (const auto& p : disco->particiones) {
        inicio += p.tamanio;
    }
    nueva.inicio = inicio;
    
    disco->particiones.push_back(nueva);
    disco->espacio_libre = calcularEspacioLibre(disco);
    
    std::cout << "[Sistema] Particion creada: " << nombre 
              << " (" << tamanioBytes << " bytes, tipo: " << tipo 
              << ", ajuste: " << ajuste << ") en " << rutaDisco << std::endl;
    
    return true;
}

bool SistemaArchivos::particionExiste(const std::string& rutaDisco, const std::string& nombre) {
    Disco* disco = buscarDisco(rutaDisco);
    if (!disco) return false;
    return buscarParticion(disco, nombre) != nullptr;
}

// --- MÉTODOS PARA MONTAJES ---

std::string SistemaArchivos::montarParticion(const std::string& rutaDisco, const std::string& nombre) {
    // Buscar disco
    Disco* disco = buscarDisco(rutaDisco);
    if (!disco) {
        std::cout << "[Sistema] Error: Disco " << rutaDisco << " no existe" << std::endl;
        return "";
    }
    
    // Buscar partición
    Particion* particion = buscarParticion(disco, nombre);
    if (!particion) {
        std::cout << "[Sistema] Error: Particion " << nombre << " no existe" << std::endl;
        return "";
    }
    
    // Verificar que no esté montada
    if (particion->montada) {
        std::cout << "[Sistema] Error: Particion " << nombre << " ya está montada" << std::endl;
        return "";
    }
    
    // Generar ID
    std::string id = generarID();
    
    // Crear montaje
    Montaje nuevo;
    nuevo.id = id;
    nuevo.rutaDisco = rutaDisco;
    nuevo.nombreParticion = nombre;
    nuevo.activo = true;
    montajes.push_back(nuevo);
    
    // Marcar partición como montada
    particion->montada = true;
    
    // Inicializar estructuras EXT2 para esta partición
    usuariosPorParticion[id] = std::vector<Usuario>();
    archivosPorParticion[id] = std::vector<Archivo>();
    
    std::cout << "[Sistema] Particion montada: " << nombre << " con ID: " << id << std::endl;
    
    return id;
}

bool SistemaArchivos::desmontarParticion(const std::string& id) {
    for (auto it = montajes.begin(); it != montajes.end(); ++it) {
        if (it->id == id && it->activo) {
            // Buscar la partición y marcarla como no montada
            Disco* disco = buscarDisco(it->rutaDisco);
            if (disco) {
                Particion* particion = buscarParticion(disco, it->nombreParticion);
                if (particion) {
                    particion->montada = false;
                }
            }
            it->activo = false;
            std::cout << "[Sistema] Particion desmontada: " << id << std::endl;
            return true;
        }
    }
    return false;
}

bool SistemaArchivos::estaMontada(const std::string& rutaDisco, const std::string& nombre) {
    for (const auto& m : montajes) {
        if (m.activo && m.rutaDisco == rutaDisco && m.nombreParticion == nombre) {
            return true;
        }
    }
    return false;
}

std::vector<Montaje> SistemaArchivos::getMontajes() const {
    return montajes;
}

Montaje* SistemaArchivos::buscarMontaje(const std::string& id) {
    for (auto& m : montajes) {
        if (m.id == id && m.activo) {
            return &m;
        }
    }
    return nullptr;
}

// --- MÉTODOS PARA EXT2 ---

bool SistemaArchivos::formatearParticion(const std::string& id) {
    Montaje* montaje = buscarMontaje(id);
    if (!montaje) {
        std::cout << "[Sistema] Error: ID " << id << " no está montado" << std::endl;
        return false;
    }
    
    // Limpiar usuarios y archivos de esta partición
    usuariosPorParticion[id].clear();
    archivosPorParticion[id].clear();
    
    std::cout << "[Sistema] Particion " << id << " formateada como EXT2" << std::endl;
    return true;
}

bool SistemaArchivos::crearUsuario(const std::string& id, const std::string& user, 
                                  const std::string& pass, const std::string& grp) {
    Montaje* montaje = buscarMontaje(id);
    if (!montaje) {
        std::cout << "[Sistema] Error: ID " << id << " no está montado" << std::endl;
        return false;
    }
    
    // Verificar que el usuario no exista
    auto& usuarios = usuariosPorParticion[id];
    for (const auto& u : usuarios) {
        if (u.nombre == user) {
            std::cout << "[Sistema] Error: Usuario " << user << " ya existe" << std::endl;
            return false;
        }
    }
    
    // Crear usuario
    Usuario nuevo(user, pass, grp);
    usuarios.push_back(nuevo);
    
    std::cout << "[Sistema] Usuario creado: " << user 
              << " (grupo: " << grp << ") en particion " << id << std::endl;
    
    return true;
}

bool SistemaArchivos::eliminarUsuario(const std::string& id, const std::string& user) {
    Montaje* montaje = buscarMontaje(id);
    if (!montaje) {
        std::cout << "[Sistema] Error: ID " << id << " no está montado" << std::endl;
        return false;
    }
    
    auto& usuarios = usuariosPorParticion[id];
    for (auto it = usuarios.begin(); it != usuarios.end(); ++it) {
        if (it->nombre == user) {
            usuarios.erase(it);
            std::cout << "[Sistema] Usuario eliminado: " << user << std::endl;
            return true;
        }
    }
    
    std::cout << "[Sistema] Error: Usuario " << user << " no existe" << std::endl;
    return false;
}

bool SistemaArchivos::crearArchivo(const std::string& id, const std::string& path, 
                                  int tamanio, bool recursivo, const std::string& contenido) {
    Montaje* montaje = buscarMontaje(id);
    if (!montaje) {
        std::cout << "[Sistema] Error: ID " << id << " no está montado" << std::endl;
        return false;
    }
    
    // Verificar que el archivo no exista
    auto& archivos = archivosPorParticion[id];
    for (const auto& a : archivos) {
        if (a.ruta == path) {
            std::cout << "[Sistema] Error: Archivo " << path << " ya existe" << std::endl;
            return false;
        }
    }
    
    // Crear archivo
    Archivo nuevo;
    nuevo.ruta = path;
    nuevo.tamanio = tamanio;
    nuevo.contenido = contenido;
    nuevo.existe = true;
    archivos.push_back(nuevo);
    
    std::cout << "[Sistema] Archivo creado: " << path 
              << " (" << tamanio << " bytes) en particion " << id << std::endl;
    
    return true;
}

bool SistemaArchivos::archivoExiste(const std::string& id, const std::string& path) {
    auto& archivos = archivosPorParticion[id];
    for (const auto& a : archivos) {
        if (a.ruta == path) {
            return true;
        }
    }
    return false;
}

// --- GETTER PARA MOSTRAR ESTADO ---

std::string SistemaArchivos::mostrarEstado() const {
    std::ostringstream oss;
    
    oss << "=== SISTEMA DE ARCHIVOS ===\n";
    oss << "Discos: " << discos.size() << "\n";
    
    for (const auto& disco : discos) {
        oss << "  " << disco.ruta << " (" << disco.tamanio << disco.unidad 
            << ", ajuste: " << disco.ajuste << ")\n";
        oss << "    Espacio libre: " << disco.espacio_libre << " bytes\n";
        for (const auto& p : disco.particiones) {
            oss << "     " << p.nombre << " (" << p.tamanio << " bytes, tipo: " 
                << p.tipo << ", " << (p.montada ? "montada" : "no montada") << ")\n";
        }
    }
    
    oss << "\nMontajes activos: " << montajes.size() << "\n";
    for (const auto& m : montajes) {
        if (m.activo) {
            oss << "   " << m.id << " → " << m.rutaDisco << " / " << m.nombreParticion << "\n";
        }
    }
    
    return oss.str();
}
