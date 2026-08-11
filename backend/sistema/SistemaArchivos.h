
#ifndef SISTEMA_ARCHIVOS_H
#define SISTEMA_ARCHIVOS_H

#include <string>
#include <vector>
#include <map>

// Estructura para representar una partición
struct Particion {
    std::string nombre;
    int inicio;
    int tamanio;
    char tipo;      // 'P' (Primaria), 'E' (Extendida), 'L' (Lógica)
    char ajuste;    // 'B' (Best Fit), 'F' (First Fit), 'W' (Worst Fit)
    bool montada;
    
    Particion() : inicio(0), tamanio(0), tipo('P'), ajuste('F'), montada(false) {}
};

// Estructura para representar un disco
struct Disco {
    std::string ruta;
    int tamanio;
    char unidad;    // 'K', 'M', 'B'
    char ajuste;    // 'B' (Best Fit), 'F' (First Fit), 'W' (Worst Fit)
    std::vector<Particion> particiones;
    int espacio_libre;
    
    Disco() : tamanio(0), unidad('M'), ajuste('F'), espacio_libre(0) {}
};

// Estructura para representar un montaje
struct Montaje {
    std::string id;             // Ej: "341A", "341B"
    std::string rutaDisco;
    std::string nombreParticion;
    bool activo;
    
    Montaje() : activo(true) {}
};

// Estructura para representar un usuario (para EXT2)
struct Usuario {
    std::string nombre;
    std::string contrasena;
    std::string grupo;
    
    Usuario() {}
    Usuario(const std::string& n, const std::string& c, const std::string& g)
        : nombre(n), contrasena(c), grupo(g) {}
};

// Estructura para representar un archivo (para EXT2)
struct Archivo {
    std::string ruta;
    int tamanio;
    std::string contenido;
    bool existe;
    
    Archivo() : tamanio(0), existe(false) {}
};

// Clase principal del sistema de archivos
class SistemaArchivos {
private:
    std::vector<Disco> discos;
    std::vector<Montaje> montajes;
    std::map<std::string, std::vector<Usuario>> usuariosPorParticion;  // key: id_montaje
    std::map<std::string, std::vector<Archivo>> archivosPorParticion;  // key: id_montaje
    
    int contadorMontajes;  // Para generar IDs únicos
    std::string carnet;    // Para generar IDs con el carnet
    
    // Métodos auxiliares
    bool discoExiste(const std::string& ruta);
    Disco* buscarDisco(const std::string& ruta);
    Particion* buscarParticion(Disco* disco, const std::string& nombre);
    std::string generarID();
    int calcularEspacioLibre(Disco* disco);
    
    
public:
    SistemaArchivos(const std::string& carnet = "202110378");
    
    // Métodos para discos
    bool crearDisco(const std::string& ruta, int tamanio, char unidad, char ajuste);
    bool eliminarDisco(const std::string& ruta);
    bool discoExisteEnMemoria(const std::string& ruta);
    std::vector<Disco> getDiscos() const;
    
    // Métodos para particiones
    bool crearParticion(const std::string& rutaDisco, const std::string& nombre, 
                        int tamanio, char tipo, char unidad, char ajuste);
    bool particionExiste(const std::string& rutaDisco, const std::string& nombre);
    
    // Métodos para montajes
    std::string montarParticion(const std::string& rutaDisco, const std::string& nombre);
    bool desmontarParticion(const std::string& id);
    bool estaMontada(const std::string& rutaDisco, const std::string& nombre);
    std::vector<Montaje> getMontajes() const;
    Montaje* buscarMontaje(const std::string& id);
    
    // Métodos para EXT2 (MKFS, MKUSR, RMUSR, MKFILE)
    bool formatearParticion(const std::string& id);
    bool crearUsuario(const std::string& id, const std::string& user, 
                     const std::string& pass, const std::string& grp);
    bool eliminarUsuario(const std::string& id, const std::string& user);
    bool crearArchivo(const std::string& id, const std::string& path, 
                     int tamanio, bool recursivo, const std::string& contenido);
    bool archivoExiste(const std::string& id, const std::string& path);
    int calcularTamanioEnBytes(int tamanio, char unidad);
    
    // Getters para mostrar estado
    std::string mostrarEstado() const;
};

#endif // SISTEMA_ARCHIVOS_H
