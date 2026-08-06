#include "Lexer.h"
#include <cctype>
#include <iostream>

//constructor
Lexer::Lexer(const std::string& texto)
    :input (texto), pos(0), linea(1), columna(1) {}

//metodo principal de analisis

std::vector<Token> Lexer::analizar(){
    tokens.clear();
    errores.clear();
    pos = 0;
    linea = 1;
    columna = 1;

    while (pos < (int)input.length()){

        char c = input[pos];
        //1. ignorar espacios y tabulaciones
        if (c == ' ' || c == '\t'){
            pos++;
            columna++;
            continue;
        }

        //2. salto de linea
        if (c == '\n'){
            linea++;
            columna = 1;
            pos++;
            continue;

        }

        //3. comillas (para cadenas con espacios)
        if (c == '"'){
            tokenizarComillas();
            continue;
        }

        //4. guion (para parametros)
        if (c == '-'){
            tokenizarParametro();
            continue;
        }

        //5. numeros
        if (isdigit(c)){
            tokenizarNumero();
            continue;
        }

        //6. letras (comandos o letras simples)
        if (isalpha(c)){
            tokenizarPalabra();
            continue;
        }

        //7. igual

        if (c == '='){
            tokens.push_back(TokenType::IGUAL, "=", linea, columna);
            pos++;
            columna++;
            continue;
        }
        //.8 barra (inicio de ruta)
        if (c == '/'){
            tokenizarRuta();
            continue;
        }

        //9. Punto (para rutas que comienzan con .)
        if (c == '.'){
            tokenizarRutaDesdePunto();
            continue;
        }

        //10. caracter no reconocido
        agregarError("caracter no reconocido: '" +std::string(1, c) + "' en linea " +std::to_string(linea) + ", columna" + std::to_string(columna));
        pos++;
        columna++;
    }

    //agregar token de fin de archivo
    tokens.push_back(Token(TokenType::FIN_DE_ARCHIVO, "EOF", linea, columna));

    return tokens;
}

//obtener errores
std::vector<std::string> Lexer::getErrores() const{
    return errores;
}

//resetear el lexer
void Lexer::reset(const std::string& nuevoTexto){
    input = nuevoTexto;
    pos = 0;
    linea = 1;
    columna = 1;
    tokens.clear();
    errores.clear();
}

//METODOS PRIVADOS

void Lexer::agregarError(const std::string& mensaje){
    errores.push_back(mensaje);
    std::cerr << "[ERROR LEXICO]" <<mensaje << std::endl;
}

bool Lexer::esComando(const std::string& palabra){
    std:: vector <std::string> comandos = {
        "MKDISK", "RMDISK", "FDISK", "MOUNT", "MKFS", "MKUSR", "RMUSR", "MKFILE"

    };

    for (const auto& cmd : comandos){
        if (palabra == cmd){
            return true;
        }
    }
    return false;
}

bool Lexer::esParametroValido(const std::string& parametro){
    std::vector<std::string> parametrosValidos = {
        "-size", "-unit", "-fit", "-path", "-type", "-name",
        "-user", "-pass", "-grp", "-r", "-cont", "-id"
    };

    for (const auto& p : parametrosValidos){
        if (parametro ==p){
            return true;
        }
    }
    return false;
}

void Lexer::tokenizarPalabra(){
    int inicioColumna = columna;
    std::string palabra;

    while (pos < (int)input.length() && (isalnum(input[pos]) || input[pos] == '_')){
        palabra += input[pos];
        pos++;
        columna++;
    }

    //convertir a mayusculas para comparar
    std::string palabraMayus = palabra;
    for (auto& c : palabraMayus) c = toupper(c);

    //1. verificar si es un comando
    if (esComando(palabraMayus)){
        tokens.push_back(Token(TokenType::COMANDO, palabraMayus, linea, inicioColumna));
        return;
    }

    //2. verificar si es BF, FF, WF 
    if (palabra == "BF" || palabra == "FF" || palabra == "WF" || palabra == "bf" || palabra == "ff" || palabra == "wf"){
        std::string valor = palabra;
        for (auto& c : valor) c = toupper(c);
        tokens.push_back(Token(TokenType::LETRA, valor, linea, inicioColumna));
        return;
    }

    //3. verificar si es una letra simple
    if (palabra.length() == 1 && isalpha(palabra[0])){
        char c = toupper(palabra[0]);
        if (c == "K" || c == "M" || c == "B" || c =="P" || c == "E" || c =="L"){
            tokens.push_back(Token(TokenType::LETRA, std::string(1, c), linea, inicioColumna));
            return;
        }
    }

    //4. si no es nada reconocido es un error
    agregarError("palabra no reconocida: '" +palabra + "'en linea " +std::to_string(linea) +
        ", columna " +std::to_string(inicioColumna));  
        tokens.push_back(Token(TokenType::DESCONOCIDO, palabra, linea, inicioColumna));   
}

void Lexer:: tokenizarParametro(){
    int inicioColumna = columna;
    std::string parametro = "-";
    pos++;
    columna++;

    while (pos < (int)input.length() && (isalnum(input[pos]) || input[pos] == '_' )){
        parametro += input[pos];
        pos++;
        columna++;
    }

    if (esParametroValido(parametro)){
        tokens.push_back(Token(TokenType::PARAMETRO, parametro, linea, inicioColumna));

    
    }else {
        agregarError("Parametro no reconocido: '" + parametro + "' en linea " + std::to_string(linea) +
            ", columna " + std::to_string(inicioColumna));
        tokens.push_back(Token(TokenType::DESCONOCIDO, parametro, linea, inicioColumna));
    }
}

void Lexer::tokenizarNumero(){
    int inicioColumna = columna;
    std::string numero;

    while (pos < (int)input.length() && isdigit(input[pos])){
        numero += input[pos];
        pos++;
        columna++;
    }
    
    tokens.push_back(Token(TokenType::NUMERO, numero, linea, inicioColumna));

}

void Lexer::tokenizarComillas(){
    //token de comillas de apertura
    tokens.push_back(Token(TokenType::COMILLAS, "\"", linea, columna));
    pos++;
    columna++;

    int inicioColumna = columna;
    std::string contenido;

    //leer todo hasta la comilla de cierre
    while (pos < (int)input.length() && input[pos] != '"'){
        contenido += input[pos];
        pos++;
        columna++;
    }

    //verificar que se encontro la comilla de cierre
    if (pos < (int)input.length() && input[pos] == '"'){
        //token de la caneda
        tokens.push_back(Token(TokenType::CADENA, contenido, linea, inicioColumna));
        //token de comillas de cierre
        tokens.push_back(Token(TokenType::COMILLAS, "\"", linea, columna));
        pos++;
        columna++;

    }else {

        agregarError("comillas sin cerrar en linea" + std::to_string(linea));
    }

}

void Lexer::tokenizarRuta(){
    int inicioColumna = columna;
    std::string ruta;

    while(pos < (int)input.length()){
        char c  = input[pos];

        //una ruta puede contener letras, numeros, guiones, puntos, barras y espacios
        if (isalnum(c) || c == '-' || c == '.'|| c == '/' || c == '_'){
            ruta += c;
            pos++;
            columna++;
        }else {
            break;
        }
    }
    if (!ruta.empty()){
        tokens.push_back(Token(TokenType::RUTA, ruta, linea, inicioColumna));
    }
}

void Lexer::tokenizarRutaDesdePunto(){
    int inicioColumna = columna;
    std::string ruta = ".";
    pos++;
    columna++;

    while (pos < (int)input.length()){
        char c = input[pos];
        if (isalnum(c) || c == '/' || c == '.' || c == '-'|| c == '_'){
            ruta += c;
            pos++;
            columna++;
        }else {
            break;
        }
    }

    if (!ruta.empty()){
        tokens.push_back(Token(TokenType::RUTA, ruta, linea, inicioColumna));
    }
}