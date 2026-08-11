#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include "../lexer/Token.h"


//estructura para almacenar los parametros de un comando

struct ParametrosComando{
    //parametros comunes
    std::string size;
    std::string unit;
    std::string fit;
    std::string path;
    std::string type;
    std::string name;
    std::string id;
    std::string user;
    std::string pass;
    std::string grp;
    std::string cont;
    std::string type_format;
    bool r_flag;

    //banderas para saber que parametros estan presentes
    bool size_presente;
    bool unit_presente;
    bool fit_presente;
    bool path_presente;
    bool type_presente;
    bool name_presente;
    bool id_presente;
    bool user_presente;
    bool pass_presente;
    bool grp_presente;
    bool cont_presente;
    bool r_flag_presente;
    bool type_format_presente;

    //constructor
    ParametrosComando();

    //metodo para limpiar
    void limpiar();
};

//Clase Parser
class Parser{
private:
    std::vector<Token> tokens; //tokens a analizar
    int pos; //posicion actual en el vector de tokens
    std::vector<std::string> errores; //lista de errores encontrados
    std::string comando_actual; //comando actual que se esta analizando
    ParametrosComando parametros; //parametros del comando actual

    //metodos privados para analizar cada comando
    bool analizarMKDISK();
    bool analizarRMDISK();
    bool analizarFDISK();
    bool analizarMOUNT();
    bool analizarMKFS();
    bool analizarMKUSR();
    bool analizarRMUSR();
    bool analizarMKFILE();

    //metodos auxiliares
    bool esperarToken(TokenType tipo, const std::string& mensaje_error);
    bool esTamanioValido(const std::string& tamanio);
    bool esFitValido(const std::string& fit);
    bool esNumeroValido(const std::string& valor);
    bool esNombreValido(const std::string& valor);
    bool esLetraUnidadValida(const std::string& letra);
    bool esTypeValido(const std::string& type);
    bool esUsuarioValido(const std::string& usuario);
    bool esGrupoValido(const std::string& grupo);
    bool esContrasenaValida(const std::string& contrasena);

public:
    Parser(const std::vector<Token>& tokens);
    bool analizar();
    std::vector<std::string> getErrores() const;
    ParametrosComando getParametros() const;
};

#endif //Parser_
