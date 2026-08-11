#include "Parser.h"
#include <cctype>
#include <iostream>

//implementacion de parametrosComando

ParametrosComando::ParametrosComando() {
    limpiar();
}

void ParametrosComando::limpiar() {
    size = "";
    unit = "";
    fit = "";
    path = "";
    type = "";
    name = "";
    id = "";
    user = "";
    pass = "";
    grp = "";
    cont = "";
    type_format = "";
    r_flag = false;

    size_presente = false;
    unit_presente = false;
    fit_presente = false;
    path_presente = false;
    type_presente = false;
    name_presente = false;
    id_presente = false;
    user_presente = false;
    pass_presente = false;
    grp_presente = false;
    cont_presente = false;
    r_flag_presente = false;
    type_format_presente = false;
}

//constructor y metodos publicos
Parser::Parser(const std::vector<Token>& tokens)
    : tokens(tokens), pos(0), comando_actual(""){
        parametros.limpiar();
    }

bool Parser::analizar() {
    errores.clear();
    pos = 0;
    
    // Verificar que hay tokens
    if (tokens.empty()) {
        errores.push_back("No hay tokens para analizar");
        return false;
    }
    
    // El primer token debe ser un COMANDO
    if (tokens[0].tipo != TokenType::COMANDO) {
        errores.push_back("Se esperaba un comando, pero se encontró: " + tokens[0].valor);
        return false;
    }
    
    comando_actual = tokens[0].valor;
    pos = 1; // Saltar el comando
    
    // Analizar según el comando
    bool resultado = false;
    
    if (comando_actual == "MKDISK") {
        resultado = analizarMKDISK();
    } else if (comando_actual == "RMDISK") {
        resultado = analizarRMDISK();
    } else if (comando_actual == "FDISK") {
        resultado = analizarFDISK();
    } else if (comando_actual == "MOUNT") {
        resultado = analizarMOUNT();
    } else if (comando_actual == "MKFS") {
        resultado = analizarMKFS();
    } else if (comando_actual == "MKUSR") {
        resultado = analizarMKUSR();
    } else if (comando_actual == "RMUSR") {
        resultado = analizarRMUSR();
    } else if (comando_actual == "MKFILE") {
        resultado = analizarMKFILE();
    } else {
        errores.push_back("Comando no reconocido: " + comando_actual);
        return false;
    }
    
    return resultado;
}

std::vector<std::string> Parser::getErrores() const {
    return errores;
}

ParametrosComando Parser::getParametros() const {
    return parametros;
}

//verificaciones
bool Parser::esperarToken(TokenType tipo, const std::string& mensaje_error){
    if (pos < (int)tokens.size() && tokens[pos].tipo == tipo){
        pos++;
        return true;
    }else{
        errores.push_back(mensaje_error);
        return false;
    }
}

bool Parser::esFitValido(const std::string& fit){
    return (fit == "BF" || fit == "FF" || fit == "WF");
}

bool Parser::esNumeroValido(const std::string& valor){
    if (valor.empty()) return false;
    for (char c : valor){
        if (!isdigit(c)) return false;
    }
    return true;
}
bool Parser::esTamanioValido(const std::string& tamanio){
    if (!esNumeroValido(tamanio))return false;
    int num = std::stoi(tamanio);
    return num > 0;
}

bool Parser::esNombreValido(const std::string& valor){
    return !valor.empty() && valor.length() <= 16;
}

bool Parser::esLetraUnidadValida(const std::string& letra){
    return (letra == "K" || letra == "M");
}

bool Parser::esTypeValido(const std::string& type){
    return (type == "P" || type == "E" || type == "L");
}

bool Parser::esUsuarioValido(const std::string& usuario){
    return !usuario.empty() && usuario.length() <= 10;
}
bool Parser::esGrupoValido(const std::string& grupo){
    return !grupo.empty() && grupo.length() <= 10;
}
bool Parser::esContrasenaValida(const std::string& contrasena){
    return !contrasena.empty() && contrasena.length() <= 10;
}

//analisis de MKDISK

bool Parser::analizarMKDISK(){
    //PARAMETROS
    // -size: obligatorio y mayor a 0
    // -unit: opcional, valores K o M, default M
    // -fit: opcional, valores BF, FF, WF, default FF
    // -path: obligatorio (ruta valida)

    bool size_encontrado = false;
    bool path_encontrado = false;

    while (pos < (int)tokens.size() && tokens[pos].tipo != TokenType::FIN_DE_ARCHIVO){
        Token token = tokens[pos];

        if (token.tipo == TokenType::PARAMETRO){
            if (token.valor == "-size"){
                if (parametros.size_presente){
                    errores.push_back("Error: -size repetido");
                    return false;
                }
                pos++;
                //verificamos que le siga un igual
                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -size"))return false;

                if(pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba un numero para -size");
                    return false;  
                }

                if (tokens[pos].tipo != TokenType::NUMERO){
                    errores.push_back("Error: Se esperaba un numero para -size, pero se encontro: " + tokens[pos].valor);
                    return false;
                }

                if (!esTamanioValido(tokens[pos].valor)){
                    errores.push_back("Error: El valor de -size debe ser un numero mayor a 0, pero se encontro: " + tokens[pos].valor);
                    return false;
                }

                //ya confirmado todo guardamos el valor
                parametros.size = tokens[pos].valor;
                parametros.size_presente = true;
                size_encontrado = true;
                pos++;

            }else if (token.valor == "-path"){
                if (parametros.path_presente){
                    errores.push_back("Error: -path repetido");
                    return false;
                }
                pos++;

                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -path"))return false;

                if (pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba una ruta para -path");
                    return false;
                }

                if (tokens[pos].tipo == TokenType::RUTA || tokens[pos].tipo == TokenType::CADENA){
                    parametros.path = tokens[pos].valor;
                    parametros.path_presente = true;
                    path_encontrado = true;
                    pos++;

                }else {
                    errores.push_back("Error: Se esperaba una ruta para -path, pero se encontro: " + tokens[pos].valor);
                    return false;
                }
            }else if (token.valor == "-unit"){
                if (parametros.unit_presente){
                    errores.push_back("Error: -unit repetido");
                    return false;
                }
                pos++;

                if (!esperarToken(TokenType::IGUAL, "se esperaba '=' despues de -unit"))return false;

                if(pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba un valor para -unit");
                    return false;
                }

                if (tokens[pos].tipo != TokenType::LETRA){
                    errores.push_back("Error: Se esperaba un valor para -unit (K o M), pero se encontro: " + tokens[pos].valor);
                    return false;
                }

                parametros.unit = tokens[pos].valor;
                parametros.unit_presente = true;
                pos++;
            }else if(token.valor == "-fit"){
                if (parametros.fit_presente){
                    errores.push_back("Error: -fit repetido");
                    return false;
                }
                pos++;

                if (!esperarToken(TokenType::IGUAL, "Error: Se esperaba '=' despues de -fit"))return false;

                if (pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba un valor para -fit");
                    return false;
                }

                if (tokens[pos].tipo != TokenType::LETRA){
                    errores.push_back("Error: Se esperaba un valor para -fit (BF, FF, WF), pero se encontro: " + tokens[pos].valor);
                    return false;
                }

                if (!esFitValido(tokens[pos].valor)){
                    errores.push_back("Error: El valor de -fit debe ser BF, FF o WF, pero se encontro: " + tokens[pos].valor);
                    return false;
                }

                parametros.fit = tokens[pos].valor;
                parametros.fit_presente = true;
                pos++;
            }else {
                
                errores.push_back("Error: Parametro no reconocido: " + token.valor);
                return false;
            }




              
        }else {
            errores.push_back("Error: Token inesperado: " + token.valor);
            return false;
        }
    }
    //verificar parametros obligatorios
    if (!size_encontrado){
        errores.push_back("Error: Falta el parametro obligatorio -size");
        return false;
    }
    if (!path_encontrado){
        errores.push_back("Error: Falta el parametro obligatorio -path");
        return false;
    }

    return true;

}

//Analisis de RMDISK

bool Parser::analizarRMDISK(){
    bool path_encontrado = false;

    while (pos < (int)tokens.size() && tokens[pos].tipo != TokenType::FIN_DE_ARCHIVO){
        Token token = tokens[pos];
        if (token.tipo == TokenType::PARAMETRO){
            if (token.valor == "-path"){
                if (parametros.path_presente){
                    errores.push_back("Error: -path repetido");
                    return false;
                }
                pos++;

                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -path"))return false;
                
                if (pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba una ruta para -path");
                    return false;
                }

                if (tokens[pos].tipo == TokenType::RUTA || tokens[pos].tipo == TokenType::CADENA){
                    parametros.path = tokens[pos].valor;
                    parametros.path_presente = true;
                    path_encontrado = true;
                    pos++;

                }else {
                    errores.push_back("Error: Se esperaba una ruta para -path, pero se encontro: " + tokens[pos].valor);
                    return false;
                }
            }else{
                errores.push_back("Error: Parametro no reconocido: " + token.valor);
                return false;
            }
        } else {
            errores.push_back("Error: Token inesperado: " + token.valor);
            return false;
        }
    }
    if (!path_encontrado){
        errores.push_back("Error: Falta el parametro obligatorio -path");
        return false;
    }

    return true;
}

//analisis de FDISK
bool Parser::analizarFDISK(){
    //PARAMETROS
    // -size: obligatorio y mayor a 0
    // -unit: opcional, valores K o M, default M
    // -fit: opcional, valores BF, FF, WF, default FF
    // -path: obligatorio (ruta valida)
    // -name: obligatorio (nombre de la particion)
    // -type: opcional, valores P o E, default P

    bool size_encontrado = false;
    bool path_encontrado = false;
    bool name_encontrado = false;

    while (pos < (int)tokens.size() && tokens[pos].tipo != TokenType::FIN_DE_ARCHIVO){
        Token token = tokens[pos];

        if (token.tipo == TokenType::PARAMETRO){
            if (token.valor == "-size"){
                if (parametros.size_presente){
                    errores.push_back("Error: -size repetido");
                    return false;
                }
                pos++;
                //verificamos que le siga un igual
                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -size"))return false;

                if(pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba un numero para -size");
                    return false;  
                }

                if (tokens[pos].tipo != TokenType::NUMERO){
                    errores.push_back("Error: Se esperaba un numero para -size, pero se encontro: " + tokens[pos].valor);
                    return false;
                }

                if (!esTamanioValido(tokens[pos].valor)){
                    errores.push_back("Error: El valor de -size debe ser un numero mayor a 0, pero se encontro: " + tokens[pos].valor);
                    return false;
                }

                //ya confirmado todo guardamos el valor
                parametros.size = tokens[pos].valor;
                parametros.size_presente = true;
                size_encontrado = true;
                pos++;

            }else if (token.valor == "-path"){
                if (parametros.path_presente){
                    errores.push_back("Error: -path repetido");
                    return false;
                }
                pos++;

                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -path"))return false;

                if (pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba una ruta para -path");
                    return false;
                }

                if (tokens[pos].tipo == TokenType::RUTA || tokens[pos].tipo == TokenType::CADENA){
                    parametros.path = tokens[pos].valor;
                    parametros.path_presente = true;
                    path_encontrado = true;
                    pos++;

                }else {
                    errores.push_back("Error: Se esperaba una ruta para -path, pero se encontro: " + tokens[pos].valor);
                    return false;
                }
            }else if (token.valor == "-name"){
                if (parametros.name_presente){
                    errores.push_back("Error: -name repetido");
                    return false;
                }
                pos++;

                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -name"))return false;

                if (pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba un valor para -name");
                    return false;
                }

                std::string nombre = tokens[pos].valor;
                if (tokens[pos].tipo == TokenType::IDENTIFICADOR || tokens[pos].tipo == TokenType::LETRA || tokens[pos].tipo == TokenType::RUTA || tokens[pos].tipo == TokenType::CADENA){
                    if (!esNombreValido(nombre)){
                        errores.push_back("Error: El valor de -name debe ser un nombre valido (no vacio y maximo 16 caracteres), pero se encontro: " + nombre);
                        return false;
                    }
                    parametros.name = nombre;
                    parametros.name_presente = true;
                    name_encontrado = true;
                    pos++;

                }else{
                    errores.push_back("Error: Se esperaba un valor para -name, pero se encontro: " + tokens[pos].valor);
                    return false;
                }

                
            }else if(token.valor == "-unit") {
                if (parametros.unit_presente){
                    errores.push_back("Error: -unit repetido");
                    return false;
                }
                pos++;

                if(!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -unit"))return false;

                if (pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba un valor para -unit");
                    return false;
                }

                if (tokens[pos].tipo != TokenType::LETRA){
                    errores.push_back("Error: Se esperaba un valor para -unit (K o M), pero se encontro: " + tokens[pos].valor);
                    return false;
                }

                if (!esLetraUnidadValida(tokens[pos].valor)){
                    errores.push_back("Error: El valor de -unit debe ser K o M, pero se encontro: " + tokens[pos].valor);
                    return false;
                }

                parametros.unit = tokens[pos].valor;
                parametros.unit_presente = true;
                pos++;
            }else if (token.valor == "-type"){
                if (parametros.type_presente){
                    errores.push_back("Error: -type repetido");
                    return false;
                }

                pos++;

                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -type"))return false;

                if(pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba un valor para -type");
                    return false;
                }

                if (tokens[pos].tipo != TokenType::LETRA){
                    errores.push_back("Error: Se esperaba un valor para -type (P, E o L), pero se encontro: " + tokens[pos].valor);
                    return false;
                }

                if (!esTypeValido(tokens[pos].valor)){
                    errores.push_back("Error: El valor de -type debe ser P, E o L, pero se encontro: " + tokens[pos].valor);
                    return false;
                }
                parametros.type = tokens[pos].valor;
                parametros.type_presente = true;
                pos++;
            }else if (token.valor == "fit"){
                if (parametros.fit_presente){
                    errores.push_back("Error: -fit repetido");
                    return false;
                }
                pos++;

                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -fit"))return false;

                if (pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba un valor para -fit");
                    return false;
                }

                if (tokens[pos].tipo != TokenType::LETRA){
                    errores.push_back("Error: Se esperaba un valor para -fit (BF, FF o WF), pero se encontro: " + tokens[pos].valor);
                    return false;
                }

                if (!esFitValido(tokens[pos].valor)){
                    errores.push_back("Error: El valor de -fit debe ser BF, FF o WF, pero se encontro: " + tokens[pos].valor);
                    return false;
                }

                parametros.fit = tokens[pos].valor;
                parametros.fit_presente = true;
                pos++;
            }else {
                errores.push_back("Error: Parametro no reconocido: " + token.valor);
                return false;
            }
        }else{
            errores.push_back("Error: Token inesperado: " + token.valor);
            return false;
        }
    }

    if(!size_encontrado){
        errores.push_back("Error: Falta el parametro obligatorio -size");
        return false;
    }
    if (!path_encontrado){
        errores.push_back("Error: Falta el parametro obligatorio -path");
        return false;
    }
    if (!name_encontrado){
        errores.push_back("Error: Falta el parametro obligatorio -name");
        return false;
    }

    return true;
}   

//analisis de MOunt
bool Parser::analizarMOUNT(){
    //parametros
    // -path obligatorio (ruta valida)
    //- name obligatorio (nombre de la particion)
    bool path_encontrado = false;
    bool name_encontrado = false;

    while (pos < (int)tokens.size() && tokens[pos].tipo != TokenType::FIN_DE_ARCHIVO){
        Token token = tokens[pos];
        if (token.tipo == TokenType::PARAMETRO){
            if (token.valor == "-path"){
                if (parametros.path_presente){
                    errores.push_back("Error: -path repetido");
                    return false;
                }
                pos++;

                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -path"))return false;

                if (pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba una ruta para -path");
                    return false;
                }

                if (tokens[pos].tipo == TokenType::RUTA || tokens[pos].tipo == TokenType::CADENA){
                    parametros.path = tokens[pos].valor;
                    parametros.path_presente = true;
                    path_encontrado = true;
                    pos++;

                }else {
                    errores.push_back("Error: Se esperaba una ruta para -path, pero se encontro: " + tokens[pos].valor);
                    return false;
                }
            }else if(token.valor == "-name"){
                if (parametros.name_presente){
                    errores.push_back("Error: -name repetido");
                    return false;
                }
                pos++;

                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -name"))return false;

                if (pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba un valor para -name");
                    return false;
                }

                std::string nombre = tokens[pos].valor;
                if (tokens[pos].tipo == TokenType::IDENTIFICADOR || tokens[pos].tipo == TokenType::LETRA || tokens[pos].tipo == TokenType::RUTA || tokens[pos].tipo == TokenType::CADENA){
                    if (!esNombreValido(nombre)){
                        errores.push_back("Error: El valor de -name debe ser un nombre valido (no vacio y maximo 16 caracteres), pero se encontro: " + nombre);
                        return false;
                    }
                    parametros.name = nombre;
                    parametros.name_presente = true;
                    name_encontrado = true;
                    pos++;

                }else{
                    errores.push_back("Error: Se esperaba un valor para -name, pero se encontro: " + tokens[pos].valor);
                    return false;
                }
            }else {
                errores.push_back("Error: Parametro no reconocido: " + token.valor);
                return false;

            }
        }else{
            errores.push_back("Error: Token inesperado: " + token.valor);
            return false;
        }

    }

    if(!path_encontrado){
        errores.push_back("Error: Falta el parametro obligatorio -path");
        return false;
    }
    if (!name_encontrado){
        errores.push_back("Error: Falta el parametro obligatorio -name");
        return false;
    }

    return true;
}

//analisis de MKFS
bool Parser::analizarMKFS(){
    //parametros
    //id obligatorio (id de la particion)
    //type opcional (valores: fast, full, default fast)
    bool id_encontrado = false;

    while (pos < (int)tokens.size() && tokens[pos].tipo != TokenType::FIN_DE_ARCHIVO){
        Token token = tokens[pos];
        if (token.tipo == TokenType::PARAMETRO){
            if (token.valor == "-id"){
                if (parametros.id_presente){
                    errores.push_back("Error: -id repetido");
                    return false;
                }
                pos++;

                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -id"))return false;

                if (pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba un valor para -id");
                    return false;
                }

                if (tokens[pos].tipo != TokenType::IDENTIFICADOR || tokens[pos].tipo != TokenType::LETRA || tokens[pos].tipo != TokenType::CADENA){
                    parametros.id = tokens[pos].valor;
                    parametros.id_presente = true;
                    id_encontrado = true;
                    pos++; 
                }else{
                    errores.push_back("Error: Se esperaba un valor para -id, pero se encontro: " + tokens[pos].valor);
                    return false;
                }
                
            }else if(token.valor == "-type"){
                if (parametros.type_format_presente){
                    errores.push_back("Error: -type repetido");
                    return false;
                }
                pos++;

                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -type"))return false;

                if (pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba un valor para -type");
                    return false;
                }

                if (tokens[pos].tipo == TokenType::LETRA){
                    std::string valor = tokens[pos].valor;
                    if (valor == "FULL" || valor == "full"){
                        parametros.type_format = "full";
                        parametros.type_format_presente = true;
                        pos++;
                    }else{
                        errores.push_back("Error: El valor de -type debe ser 'full' o 'fast', pero se encontro: " + valor);
                        return false;
                    }
                }else{
                    errores.push_back("Error: Se esperaba un valor para -type, pero se encontro: " + tokens[pos].valor);
                    return false;
                }
            }else {
                errores.push_back("Error: Parametro no reconocido: " + token.valor);
                return false;

            }
        }else{
            errores.push_back("Error: Token inesperado: " + token.valor);
            return false;
        }

    }
    if (!id_encontrado){
        errores.push_back("Error: Falta el parametro obligatorio -id");
        return false;
    }
    return true;
}

//analisis de MKUSR
bool Parser::analizarMKUSR(){
    bool user_encontrado = false;
    bool pass_encontrado = false;
    bool grp_encontrado = false;

    while (pos < (int)tokens.size() && tokens[pos].tipo != TokenType::FIN_DE_ARCHIVO){
        Token token = tokens[pos];

        if (token.tipo == TokenType::PARAMETRO){
            if (token.valor == "-id") {
                if (parametros.id_presente) {
                    errores.push_back("Error: -id repetido");
                    return false;
                }
                pos++;
                
                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' después de -id")) return false;
                
                if (pos >= (int)tokens.size()) {
                    errores.push_back("Error: Se esperaba un valor para -id");
                    return false;
                }
                
                if (tokens[pos].tipo == TokenType::IDENTIFICADOR || 
                    tokens[pos].tipo == TokenType::LETRA || 
                    tokens[pos].tipo == TokenType::CADENA) {
                    parametros.id = tokens[pos].valor;
                    parametros.id_presente = true;
                    pos++;
                } else {
                    errores.push_back("Error: Se esperaba un valor para -id, pero se encontro: " + tokens[pos].valor);
                    return false;
                }
            }
            else if (token.valor == "-user"){
                if (parametros.user_presente){
                    errores.push_back("Error: -user repetido");
                    return false;
                }
                pos++;

                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -user"))return false;

                if (pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba un valor para -user");
                    return false;
                }

                if(tokens[pos].tipo == TokenType::IDENTIFICADOR || tokens[pos].tipo == TokenType::LETRA || tokens[pos].tipo == TokenType::CADENA){
                    if (!esUsuarioValido(tokens[pos].valor)){
                        errores.push_back("Error: El valor de -user debe ser un nombre valido (no vacio y maximo 16 caracteres), pero se encontro: " + tokens[pos].valor);
                        return false;
                    }
                    parametros.user = tokens[pos].valor;
                    parametros.user_presente = true;
                    user_encontrado = true;
                    pos++;
                }else{
                    errores.push_back("Error: Se esperaba un valor para -user, pero se encontro: " + tokens[pos].valor);
                    return false;
                }
            }else if(token.valor == "-pass"){
                if (parametros.pass_presente){
                    errores.push_back("Error: -pass repetido");
                    return false;
                }
                pos++;

                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -pass"))return false;

                if (pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba un valor para -pass");
                    return false;
                }

                if (tokens[pos].tipo == TokenType::LETRA || tokens[pos].tipo == TokenType::NUMERO || tokens[pos].tipo == TokenType::CADENA){
                    if (!esContrasenaValida(tokens[pos].valor)){
                        errores.push_back("Error: El valor de -pass debe ser una contrasena valida (no vacia y maximo 16 caracteres), pero se encontro: " + tokens[pos].valor);
                        return false;
                    }
                    parametros.pass = tokens[pos].valor;
                    parametros.pass_presente = true;
                    pass_encontrado = true;
                    pos++;

                }else{
                    errores.push_back("Error: Se esperaba un valor para -pass, pero se encontro: " + tokens[pos].valor);
                    return false;
                }
                
            }else if(token.valor == "-grp"){
                if (parametros.grp_presente){
                    errores.push_back("Error: -grp repetido");
                    return false;
                }
                pos++;

                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -grp"))return false;

                if (pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba un valor para -grp");
                    return false;
                }

                if (tokens[pos].tipo == TokenType::IDENTIFICADOR || tokens[pos].tipo == TokenType::LETRA || tokens[pos].tipo == TokenType::CADENA){
                    if (!esGrupoValido(tokens[pos].valor)){
                        errores.push_back("Error: El valor de -grp debe ser un nombre valido (no vacio y maximo 16 caracteres), pero se encontro: " + tokens[pos].valor);
                        return false;
                    }
                    parametros.grp = tokens[pos].valor;
                    parametros.grp_presente = true;
                    grp_encontrado = true;
                    pos++;
                }else{
                    errores.push_back("Error: Se esperaba un valor para -grp, pero se encontro: " + tokens[pos].valor);
                    return false;
                }
            }else {
                errores.push_back("Error: Parametro no reconocido: " + token.valor);
                return false;

            }
        }else{
            errores.push_back("Error: Token inesperado: " + token.valor);
            return false;
        }
    }

    if(!user_encontrado){
        errores.push_back("Error: Falta el parametro obligatorio -user");
        return false;
    }
    if(!pass_encontrado){
        errores.push_back("Error: Falta el parametro obligatorio -pass");
        return false;  
    }
    if(!grp_encontrado){
        errores.push_back("Error: Falta el parametro obligatorio -grp");
        return false;  
    }

    return true;
}

//analisis de RMUSR
bool Parser::analizarRMUSR(){
    bool user_encontrado = false;

    while (pos < (int)tokens.size() && tokens[pos].tipo != TokenType::FIN_DE_ARCHIVO){
        Token token = tokens[pos];

        if (token.tipo == TokenType::PARAMETRO){
            if (token.valor == "-user"){
                if (parametros.user_presente){
                    errores.push_back("Error: -user repetido");
                    return false;
                }
                pos++;

                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -user"))return false;

                if (pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba un valor para -user");
                    return false;
                }

                if (tokens[pos].tipo == TokenType::LETRA || tokens[pos].tipo == TokenType::CADENA){
                    
                    parametros.user = tokens[pos].valor;
                    parametros.user_presente = true;
                    user_encontrado = true;
                    pos++;

                }else{
                    errores.push_back("Error: Se esperaba un valor para -user, pero se encontro: " + tokens[pos].valor);
                    return false;
                }
            }else {
                errores.push_back("Error: Parametro no reconocido: " + token.valor);
                return false;

            }
        }else{
            errores.push_back("Error: Token inesperado: " + token.valor);
            return false;
        }

    }
    if (!user_encontrado){
        errores.push_back("Error: Falta el parametro obligatorio -user");
        return false;
    }
    return true;
}

//analisis de MKFILE
bool Parser::analizarMKFILE(){
    bool path_encontrado = false;
    //opcionales
    //r
    //size
    //cont

    while (pos < (int)tokens.size() && tokens[pos].tipo != TokenType::FIN_DE_ARCHIVO){
        Token token = tokens[pos];

        if (token.tipo == TokenType::PARAMETRO){
            if (token.valor == "-id") {
                if (parametros.id_presente) {
                    errores.push_back("Error: -id repetido");
                    return false;
                }
                pos++;
                
                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' después de -id")) return false;
                
                if (pos >= (int)tokens.size()) {
                    errores.push_back("Error: Se esperaba un valor para -id");
                    return false;
                }
                
                if (tokens[pos].tipo == TokenType::IDENTIFICADOR || 
                    tokens[pos].tipo == TokenType::LETRA || 
                    tokens[pos].tipo == TokenType::CADENA) {
                    parametros.id = tokens[pos].valor;
                    parametros.id_presente = true;
                    pos++;
                } else {
                    errores.push_back("Error: Se esperaba un valor para -id, pero se encontro: " + tokens[pos].valor);
                    return false;
                }
            }
            else if (token.valor == "-path"){
                if (parametros.path_presente){
                    errores.push_back("Error: -path repetido");
                    return false;
                }
                pos++;

                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -path"))return false;

                if (pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba una ruta para -path");
                    return false;
                }

                if (tokens[pos].tipo == TokenType::RUTA || tokens[pos].tipo == TokenType::CADENA){
                    parametros.path = tokens[pos].valor;
                    parametros.path_presente = true;
                    path_encontrado = true;
                    pos++;

                }else {
                    errores.push_back("Error: Se esperaba una ruta para -path, pero se encontro: " + tokens[pos].valor);
                    return false;
                }
            }else if(token.valor == "-size"){
                if (parametros.size_presente){
                    errores.push_back("Error: -size repetido");
                    return false;
                }
                pos++;

                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -size"))return false;

                if(pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba un numero para -size");
                    return false;  
                }

                if (tokens[pos].tipo != TokenType::NUMERO){
                    errores.push_back("Error: Se esperaba un numero para -size, pero se encontro: " + tokens[pos].valor);
                    return false;
                }

                if (!esTamanioValido(tokens[pos].valor)){
                    errores.push_back("Error: El valor de -size debe ser un numero mayor a 0, pero se encontro: " + tokens[pos].valor);
                    return false;
                }

                //ya confirmado todo guardamos el valor
                parametros.size = tokens[pos].valor;
                parametros.size_presente = true;
                pos++;
            }else if(token.valor == "-r"){
                if (parametros.r_flag_presente){
                    errores.push_back("Error: -r repetido");
                    return false;
                }

                parametros.r_flag = true;
                parametros.r_flag_presente = true;
                pos++;
            }else if(token.valor == "-cont"){
                if (parametros.cont_presente){
                    errores.push_back("Error: -cont repetido");
                    return false;
                }
                pos++;

                if (!esperarToken(TokenType::IGUAL, "Se esperaba '=' despues de -cont"))return false;

                if (pos >= (int)tokens.size()){
                    errores.push_back("Error: Se esperaba un valor para -cont");
                    return false;
                }

                if (tokens[pos].tipo == TokenType::RUTA || tokens[pos].tipo == TokenType::CADENA){
                    parametros.cont = tokens[pos].valor;
                    parametros.cont_presente = true;
                    pos++;

                }else{
                    errores.push_back("Error: Se esperaba un valor para -cont, pero se encontro: " + tokens[pos].valor);
                    return false;
                }
            }
            else {
                    errores.push_back("Error: Parametro no reconocido: " + token.valor);
                    return false;

            }
        }else{
            errores.push_back("Error: Token inesperado: " + token.valor);
            return false;
        }
    }
    if (!path_encontrado){
        errores.push_back("Error: Falta el parametro obligatorio -path");
        return false;
    }
    return true;
}