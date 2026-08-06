#include <iostream>
#include <string>
#include "httplib.h"
#include <sstream>
#include "lexer/Lexer.h"

using namespace httplib;

//funcion que procesa los comandos
std::string procesarComando(const std::string& comando){
    //solo devuelve mensaje de prueba
    return "Comando recibido: " + comando;
}

//funcion para agregar headers CORS a la respuesta
void configurarCORS(Response& res){
    res.set_header("Acess-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

//funcion para extraer el comando del JSON
std::string extraerComandoDelJSON(const std::string& body){
    //buscar "comando":" y extraer el valor 
    size_t pos = body.find("\"comando\"");
    if (pos == std::string::npos){
        return body; //si no tiene formato JSON devolver el body como esta
    }

    pos = body.find(":", pos);
    
}

int main (){
//crear el servidor
    Server svr;

    //middleware para agregar headers CORS a todas las respuestas
    svr.set_pre_routing_handler([](const Request& req, Response& res){

        //si es una solicitud OPTIONS (preflight), respondem,os inmediatamente
        if (req.method == "OPTIONS"){
            configurarCORS(res);
            res.status = 200;
            return Server::HandlerResponse::Handled;
        }

        return Server::HandlerResponse::Unhandled;
    });

    //endpoints

    //1. endpoint de prueba GET para verificar que el servidor funciona
    svr.Get("/ping", [](const Request& req, Response& res){
        configurarCORS(res);
        res.set_content("{\"status\":\"ok\", \"message\":\"Servidor funcionando\"}", "application/json");

    });

    //2. endpoint para recibir comandos POST
    svr.Post("/comando", [](const Request& req, Response& res){
        configurarCORS(res);
        //obtener el comando del cuerpo de la peticion JSON
        std::string comando = req.body;

        //procesar el comando (solo imprime en consola)
        std::cout << "comando recibido: " <<comando <<std::endl;
    

    //simular procesamiento
    std::string resultado = procesarComando(comando);

    //crear respuesta json
    std::string response_json = "{\"succes\":true, \"resultado\": \"" +resultado + "\"}";
    res.set_content(response_json, "application/json");

    });

    //3. Endpoint para analisis lexico
    svr.Post("/analizar", [](const Request& req, Response& res){
        configurarCORS(res);
        std::string comando = req.body;

        //por ahora solo devuelve el comando recibido
        std::string response_json = "{\"tokens\": [\"token1\", \"token2\"], \"errores\": []}";
        res.set_content(response_json, "application/json");
    });

    //se configura el servidor para escuchar el puerto 8080
    std::cout << "==== SERVIDOR C++ INICIADO ====" << std::endl;
    std::cout << "Escuchando en http://localhost:8080" << std::endl;
    std::cout << "Endpoints disponibles:" << std::endl;
    std::cout << "GET /ping - Verifica que el servidor funciona" << std::endl;
    std::cout << "POST /comando - Recibe un comando y devuelve el resultado" << std::endl;
    std::cout << "POST /analizar" << std::endl;
    std::cout << "===============================" << std::endl;

    svr.listen ("0.0.0.0", 8080);
    return 0;


}