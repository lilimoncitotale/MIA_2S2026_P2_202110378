#include "Token.h"

//implementacion del constructor
Token::Token(Tokentype t, const std::string& v, int l, int c)
    :tipo(t), valor(v), linea(l), columna(c){}

//convertir el tipo de token a string
std::string Token::tipoToString() const{
    switch (tipo){
        case TokenType::COMANDO: return "COMANDO";
        case TokenType::PARAMETRO: return "PARAMETRO";
        case TokenType::IGUAL: return "IGUAL";
        case TokenType::NUMERO: return "NUMERO";
        case TokenType::CADENA: return "CADENA";
        case TokenType::RUTA: return "RUTA";
        case TokenType::LETRA: return "LETRA";
        case TokenType::COMILLAS: return "COMILLAS";
        case TokenType::DESCONOCIDO: return "DESCONOCIDO";
        case TokenType::FIN_DE_ARCHIVO: return "FIN_DE_ARCHIVO";
        default: return "TIPO_DESCONOCIDO";
    }
}

//imprimir el token completo
std::string Token::toString() const{
    return "Token("+ tipoToString() +", '"+ valor +"', linea "+std::to_string(linea) +", columna " + std::to_String(columna) +")";
    
}