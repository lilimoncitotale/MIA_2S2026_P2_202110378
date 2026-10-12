#include <iostream>
#include <string>
#include <sstream>
#include "httplib.h"
#include "lexer/Lexer.h"
#include "include/nlohmann/json.hpp"
#include "parser/Parser.h"
#include "sistema/Ejecutores.h"

using namespace httplib;
using json = nlohmann::json;

// Función para configurar CORS
void configurarCORS(Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

// Función para extraer el comando del JSON
std::string extraerComandoDelJSON(const std::string& body) {
    // Buscar "comando":" y extraer el valor
    size_t pos = body.find("\"comando\"");
    if (pos == std::string::npos) {
        return body;
    }
    
    pos = body.find(":", pos);
    if (pos == std::string::npos) return "";
    
    pos = body.find("\"", pos + 1);
    if (pos == std::string::npos) return "";
    
    size_t inicio = pos + 1;
    size_t fin = inicio;
    
    // Buscar la comilla de cierre que NO esté escapada
    while (fin < body.length()) {
        if (body[fin] == '"' && (fin == 0 || body[fin-1] != '\\')) {
            break;
        }
        fin++;
    }
    
    if (fin >= body.length()) return "";
    
    std::string comando = body.substr(inicio, fin - inicio);
    
    // 1. Reemplazar \" por " (desescapar comillas)
    size_t found = comando.find("\\\"");
    while (found != std::string::npos) {
        comando.replace(found, 2, "\"");
        found = comando.find("\\\"", found + 1);
    }
    
    // 2. Reemplazar \\ por \ (desescapar barras)
    found = comando.find("\\\\");
    while (found != std::string::npos) {
        comando.replace(found, 2, "\\");
        found = comando.find("\\\\", found + 1);
    }
    
    return comando;
}

int main() {
    // Inicializar el MountedManager con los últimos 2 dígitos del carnet
    // TODO: Cambiar "34" por los últimos 2 dígitos de tu carnet
    // g_mountedManager = MountedManager("34");
    
    Server svr;

    // Middleware para CORS (maneja OPTIONS)
    svr.set_pre_routing_handler([](const Request& req, Response& res) {
        if (req.method == "OPTIONS") {
            configurarCORS(res);
            res.status = 200;
            return Server::HandlerResponse::Handled;
        }
        return Server::HandlerResponse::Unhandled;
    });

    // ENDPOINT 1: Ping (GET)
    svr.Get("/ping", [](const Request& req, Response& res) {
        configurarCORS(res);
        res.set_content("{\"status\":\"ok\", \"message\":\"Servidor C++ funcionando!\"}", 
                       "application/json");
    });

    // ENDPOINT 2: Comando (POST) - Ejecuta el comando completo
    svr.Post("/comando", [](const Request& req, Response& res) {
        configurarCORS(res);
        
        std::string comandoTexto = extraerComandoDelJSON(req.body);
        std::cout << "Comando recibido: " << comandoTexto << std::endl;
        
        // 1. Lexer
        Lexer lexer(comandoTexto);
        std::vector<Token> tokens = lexer.analizar();
        std::vector<std::string> errores_lexicos = lexer.getErrores();
        
        if (!errores_lexicos.empty()) {
            json respuesta;
            respuesta["success"] = false;
            respuesta["errores"] = errores_lexicos;
            res.set_content(respuesta.dump(), "application/json");
            return;
        }
        
        // 2. Parser
        Parser parser(tokens);
        if (!parser.analizar()) {
            json respuesta;
            respuesta["success"] = false;
            respuesta["errores"] = parser.getErrores();
            res.set_content(respuesta.dump(), "application/json");
            return;
        }
        
        // 3. Obtener parámetros validados
        ParametrosComando params = parser.getParametros();
        std::string comando = tokens[0].valor;
        
        // 4. EJECUTAR EL COMANDO
        std::string resultado;
        
        if (comando == "MKDISK") {
            resultado = ejecutarMKDISK(params);
        } else if (comando == "RMDISK") {
            resultado = ejecutarRMDISK(params);
        } else if (comando == "FDISK") {
            resultado = ejecutarFDISK(params);
        } else if (comando == "MOUNT") {
            resultado = ejecutarMOUNT(params);
        } else if (comando == "MOUNTED") {
            resultado = ejecutarMOUNTED(params);
        } else if (comando == "MKFS") {
            resultado = ejecutarMKFS(params);
        } else if (comando == "LOGIN") {
            resultado = ejecutarLOGIN(params);
        } else if (comando == "LOGOUT") {
            resultado = ejecutarLOGOUT(params);
        } else if (comando == "MKUSR") {
            resultado = ejecutarMKUSR(params);
        } else if (comando == "MKGRP") {
            resultado = ejecutarMKGRP(params);
        } else if (comando == "RMGRP") {
            resultado = ejecutarRMGRP(params);
        } else if (comando == "CHGRP") {
            resultado = ejecutarCHGRP(params);
        } else if (comando == "MKDIR") {
            resultado = ejecutarMKDIR(params);
        } else if (comando == "RMUSR") {
            resultado = ejecutarRMUSR(params);
        } else if (comando == "MKFILE") {
            resultado = ejecutarMKFILE(params);
        } else if (comando == "CAT") {
            resultado = ejecutarCAT(params);
        } else if (comando == "JOURNALING") {
            resultado = ejecutarJOURNALING(params);
        } else if (comando == "REP") {
            resultado = ejecutarREP(params);
        } else if (comando == "LOSS") {
            resultado = ejecutarLOSS(params);
        }else {
            resultado = "❌ Comando no reconocido: " + comando;
        }
        
        // 5. Devolver respuesta
        json respuesta;
        respuesta["success"] = true;
        respuesta["resultado"] = resultado;
        
        res.set_content(respuesta.dump(), "application/json");
    });

    // ENDPOINT 3: Análisis Léxico (POST)
    svr.Post("/analizar", [](const Request& req, Response& res) {
        configurarCORS(res);
        
        std::string comando = extraerComandoDelJSON(req.body);
        std::cout << "Analizando comando: " << comando << std::endl;
        
        Lexer lexer(comando);
        std::vector<Token> tokens = lexer.analizar();
        std::vector<std::string> errores_lexicos = lexer.getErrores();

        std::vector<std::string> errores_sintacticos;
        ParametrosComando parametros;
        bool valido_sintacticamente = false;

        if (errores_lexicos.empty()) {
            Parser parser(tokens);
            valido_sintacticamente = parser.analizar();
            if (valido_sintacticamente) {
                parametros = parser.getParametros();
            } else {
                errores_sintacticos = parser.getErrores();
            }
        }

        // Usar nlohmann/json para construir la respuesta (más seguro)
        json respuesta;
        respuesta["tokens"] = json::array();
        for (const auto& token : tokens) {
            json t;
            t["tipo"] = token.tipoToString();
            t["valor"] = token.valor;
            t["linea"] = token.linea;
            t["columna"] = token.columna;
            respuesta["tokens"].push_back(t);
        }
        
        // Errores combinados
        std::vector<std::string> todos_errores = errores_lexicos;
        todos_errores.insert(todos_errores.end(), 
                            errores_sintacticos.begin(), 
                            errores_sintacticos.end());
        respuesta["errores"] = todos_errores;

        // Estadísticas
        respuesta["stats"]["totalTokens"] = tokens.size();
        respuesta["stats"]["totalErrores"] = todos_errores.size();
        respuesta["stats"]["validoSintacticamente"] = valido_sintacticamente;
        
        // Parámetros encontrados (solo si es válido)
        if (valido_sintacticamente) {
            json params;
            if (parametros.size_presente) params["size"] = parametros.size;
            if (parametros.path_presente) params["path"] = parametros.path;
            if (parametros.unit_presente) params["unit"] = parametros.unit;
            if (parametros.fit_presente) params["fit"] = parametros.fit;
            if (parametros.type_presente) params["type"] = parametros.type;
            if (parametros.name_presente) params["name"] = parametros.name;
            if (parametros.id_presente) params["id"] = parametros.id;
            if (parametros.user_presente) params["user"] = parametros.user;
            if (parametros.pass_presente) params["pass"] = parametros.pass;
            if (parametros.grp_presente) params["grp"] = parametros.grp;
            if (parametros.cont_presente) params["cont"] = parametros.cont;
            if (parametros.r_flag_presente) params["r"] = true;
            if (parametros.type_format_presente) params["type_format"] = parametros.type_format;
            respuesta["parametros"] = params;
        }
        
        res.set_content(respuesta.dump(), "application/json");
    });

    // Iniciar el servidor
    std::cout << "========================================" << std::endl;
    std::cout << "  SERVIDOR ANALIZADOR EXT2" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Puerto: 8080" << std::endl;
    std::cout << "  Endpoints:" << std::endl;
    std::cout << "    GET  /ping       - Probar conexión" << std::endl;
    std::cout << "    POST /comando    - Ejecutar comando" << std::endl;
    std::cout << "    POST /analizar   - Analizar léxicamente" << std::endl;
    std::cout << "  CORS: Habilitado" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Servidor listo! Esperando peticiones..." << std::endl;
    std::cout << "========================================" << std::endl;

    svr.listen("0.0.0.0", 8080);

    return 0;
}