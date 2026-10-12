#ifndef API_HANDLER_H
#define API_HANDLER_H

#include <string>
#include "include/nlohmann/json.hpp"

using json = nlohmann::json;

class ApiHandler {
public:
    static json listarDiscos();
    static json infoDisco(const std::string& path);
    static json infoParticion(const std::string& id);
    static json listarCarpeta(const std::string& id, const std::string& path);
    static json leerArchivo(const std::string& id, const std::string& path);
    static json loginGrafico(const std::string& id, const std::string& user, const std::string& pass);
    static json logoutGrafico();
    static json estadoSesion();
    static json listarJournal(const std::string& id);
};

#endif