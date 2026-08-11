#ifndef TOKEN_H
#define TOKEN_H

#include <string>

//enumeracion para los tipos de tokens
enum class TokenType {
    COMANDO,   //MKDISK,RMDISK,FDISK,MOUNT,MKFS,MKUSR.RMUSR, MKFILE
    PARAMETRO, //-size, -path, -unit, -fit, -type, -name, -user, -pass, -grp, -cont, -id
    IGUAL, // = 
    NUMERO, // 123, 456, 789
    CADENA, // "TEXTO CON ESPACIOS"
    RUTA, // /home/usuario/disco1.mia
    LETRA, // K,M,B,P,E,L,BF,FF,WF
    COMILLAS, // "
    IDENTIFICADOR, // nombre de particion, nombre de usuario, nombre de grupo
    DESCONOCIDO,
    FIN_DE_ARCHIVO

};

//estructura para almacenar el token
struct Token {
    TokenType tipo;
    std::string valor;
    int linea;
    int columna;

    //constructor
    Token(TokenType t = TokenType::DESCONOCIDO,
        const std::string& v = "",
        int l = 1,
        int c = 1);

    //metodo para obtener el nombre del tipo para debug
    std::string tipoToString() const;

    //metodo para imprimir el token para debug
    std::string toString() const;
};

#endif // TOKEN_H