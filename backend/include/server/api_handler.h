#ifndef API_HANDLER_H
#define API_HANDLER_H

#include <string>
#include "../include/nlohmann/json.hpp"

using json = nlohmann::json;

class ApiHandler {
public:
    // Discos
    static json listarDiscos();
    static json infoDisco(const std::string& path);
    
    // Particiones
    static json infoParticion(const std::string& id);
    
    // Sistema de archivos
    static json listarCarpeta(const std::string& id, const std::string& path);
    static json leerArchivo(const std::string& id, const std::string& path);
    
    // Sesión
    static json loginGrafico(const std::string& id, 
                              const std::string& user, 
                              const std::string& pass);
    static json logoutGrafico();
    static json estadoSesion();
    
    // Journal
    static json listarJournal(const std::string& id);
};

#endif