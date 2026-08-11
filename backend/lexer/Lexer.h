#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include "Token.h"

class Lexer {
private:
    std::string input; //texto de entrada
    int pos; //posicion actual en el texto
    int linea; //linea actual
    int columna; //columna actual
    std::vector<Token> tokens; //vector de tokens generados
    std::vector<std::string> errores; //lista de errores encontrados

    //metodos privados para tokenizar cada tipo

    void tokenizarPalabra();
    void tokenizarParametro();
    void tokenizarNumero();
    void tokenizarComillas();
    void tokenizarRuta();
    void tokenizarRutaDesdePunto();
    void agregarError(const std::string& mensaje);

    //metodo auxiliar para verificar si es un comando
    bool esComando(const std::string& palabra);

    //metodo para verificar si es un parametro valido
    bool esParametroValido(const std::string& palabra);

public:
    //constructor
    Lexer(const std::string& texto);

    //metodo principal:analiza el texto y devuelve los tokens
    std::vector<Token> analizar();

    //obtener los errores encontrados
    std::vector<std::string> getErrores() const;

    //limpiar el lexer para reutilizarlo
    void reset(const std::string& texto);

};
#endif // LEXER_H