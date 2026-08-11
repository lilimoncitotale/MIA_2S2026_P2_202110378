#include "Lexer.h"
#include <cctype>
#include <iostream>
#include <vector>  

// Constructor
Lexer::Lexer(const std::string& texto)
    : input(texto), pos(0), linea(1), columna(1) {}

// Método principal de análisis
std::vector<Token> Lexer::analizar() {
    tokens.clear();
    errores.clear();
    pos = 0;
    linea = 1;
    columna = 1;

    while (pos < (int)input.length()) {
        char c = input[pos];
        
        // 1. Ignorar espacios y tabulaciones
        if (c == ' ' || c == '\t') {
            pos++;
            columna++;
            continue;
        }

        // 2. Salto de línea
        if (c == '\n') {
            linea++;
            columna = 1;
            pos++;
            continue;
        }

        // 3. Comillas (para cadenas con espacios)
        if (c == '"') {
            tokenizarComillas();
            continue;
        }

        // 4. Guion (para parámetros)
        if (c == '-') {
            tokenizarParametro();
            continue;
        }

        // 5. Números
        if (isdigit(c)) {
            tokenizarNumero();
            continue;
        }

        // 6. Letras (comandos o letras simples)
        if (isalpha(c)) {
            tokenizarPalabra();
            continue;
        }

        // 7. Igual
        if (c == '=') {
            tokens.push_back(Token(TokenType::IGUAL, "=", linea, columna));
            pos++;
            columna++;
            continue;
        }

        // 8. Barra (inicio de ruta)
        if (c == '/') {
            tokenizarRuta();
            continue;
        }

        // 9. Punto (para rutas que comienzan con .)
        if (c == '.') {
            tokenizarRutaDesdePunto();
            continue;
        }

        // 10. Carácter no reconocido
        agregarError("caracter no reconocido: '" + std::string(1, c) + 
                    "' en linea " + std::to_string(linea) + 
                    ", columna " + std::to_string(columna));
        pos++;
        columna++;
    }

    // Agregar token de fin de archivo
    tokens.push_back(Token(TokenType::FIN_DE_ARCHIVO, "EOF", linea, columna));

    return tokens;
}

// Obtener errores
std::vector<std::string> Lexer::getErrores() const {
    return errores;
}

// Resetear el lexer
void Lexer::reset(const std::string& nuevoTexto) {
    input = nuevoTexto;
    pos = 0;
    linea = 1;
    columna = 1;
    tokens.clear();
    errores.clear();
}

// --- MÉTODOS PRIVADOS ---

void Lexer::agregarError(const std::string& mensaje) {
    errores.push_back(mensaje);
    std::cerr << "[ERROR LEXICO] " << mensaje << std::endl;
}

bool Lexer::esComando(const std::string& palabra) {
    std::vector<std::string> comandos = {
        "MKDISK", "RMDISK", "FDISK", "MOUNT", 
        "MKFS", "MKUSR", "RMUSR", "MKFILE"
    };

    for (const auto& cmd : comandos) {
        if (palabra == cmd) {
            return true;
        }
    }
    return false;
}

bool Lexer::esParametroValido(const std::string& parametro) {
    std::vector<std::string> parametrosValidos = {
        "-size", "-unit", "-fit", "-path", "-type", "-name",
        "-user", "-pass", "-grp", "-r", "-cont", "-id"
    };

    for (const auto& p : parametrosValidos) {
        if (parametro == p) {
            return true;
        }
    }
    return false;
}

void Lexer::tokenizarPalabra() {
    int inicioColumna = columna;
    std::string palabra;

    while (pos < (int)input.length() && (isalnum(input[pos]) || input[pos] == '_')) {
        palabra += input[pos];
        pos++;
        columna++;
    }

    // Convertir a mayúsculas para comparar
    std::string palabraMayus = palabra;
    for (auto& c : palabraMayus) c = toupper(c);

    // 1. Verificar si es un comando
    if (esComando(palabraMayus)) {
        tokens.push_back(Token(TokenType::COMANDO, palabraMayus, linea, inicioColumna));
        return;
    }

    // 2. Verificar si es BF, FF, WF (ajustes)
    if (palabra == "BF" || palabra == "FF" || palabra == "WF" ||
        palabra == "bf" || palabra == "ff" || palabra == "wf") {
        std::string valor = palabra;
        for (auto& c : valor) c = toupper(c);
        tokens.push_back(Token(TokenType::LETRA, valor, linea, inicioColumna));
        return;
    }

    // 3. Verificar si es una letra simple (K, M, B, P, E, L)
    if (palabra.length() == 1 && isalpha(palabra[0])) {
        char c = toupper(palabra[0]);
        // CORREGIDO: Usar comillas simples para char
        if (c == 'K' || c == 'M' || c == 'B' || 
            c == 'P' || c == 'E' || c == 'L') {
            tokens.push_back(Token(TokenType::LETRA, std::string(1, c), 
                                   linea, inicioColumna));
            return;
        }
    }

        // 4. Si no es nada de lo anterior, es un IDENTIFICADOR (no error)
    // Esto permite valores como "admin", "root", "part1", etc.
    tokens.push_back(Token(TokenType::IDENTIFICADOR, palabra, linea, inicioColumna));
}

void Lexer::tokenizarParametro() {
    int inicioColumna = columna;
    std::string parametro = "-";
    pos++;
    columna++;

    while (pos < (int)input.length() && (isalnum(input[pos]) || input[pos] == '_')) {
        parametro += input[pos];
        pos++;
        columna++;
    }

    if (esParametroValido(parametro)) {
        tokens.push_back(Token(TokenType::PARAMETRO, parametro, linea, inicioColumna));
    } else {
        agregarError("Parametro no reconocido: '" + parametro + 
                    "' en linea " + std::to_string(linea) + 
                    ", columna " + std::to_string(inicioColumna));
        tokens.push_back(Token(TokenType::DESCONOCIDO, parametro, linea, inicioColumna));
    }
}

void Lexer::tokenizarNumero() {
    int inicioColumna = columna;
    std::string token;

    // Leer dígitos
    while (pos < (int)input.length() && isdigit(input[pos])) {
        token += input[pos];
        pos++;
        columna++;
    }

    // Si después de los dígitos hay letras o guiones bajos, es un IDENTIFICADOR
    // (como "202110378A" o "2024_archivo")
    if (pos < (int)input.length() && (isalpha(input[pos]) || input[pos] == '_')) {
        while (pos < (int)input.length() && (isalnum(input[pos]) || input[pos] == '_')) {
            token += input[pos];
            pos++;
            columna++;
        }
        tokens.push_back(Token(TokenType::IDENTIFICADOR, token, linea, inicioColumna));
    } else {
        // Solo números → NUMERO
        tokens.push_back(Token(TokenType::NUMERO, token, linea, inicioColumna));
    }
}

void Lexer::tokenizarComillas() {
    // Token de comillas de apertura
    tokens.push_back(Token(TokenType::COMILLAS, "\"", linea, columna));
    pos++;
    columna++;

    int inicioColumna = columna;
    std::string contenido;

    // Leer todo hasta la comilla de cierre
    while (pos < (int)input.length() && input[pos] != '"') {
        contenido += input[pos];
        pos++;
        columna++;
    }

    // Verificar que se encontró la comilla de cierre
    if (pos < (int)input.length() && input[pos] == '"') {
        // Token de la cadena
        tokens.push_back(Token(TokenType::CADENA, contenido, linea, inicioColumna));
        // Token de comillas de cierre
        tokens.push_back(Token(TokenType::COMILLAS, "\"", linea, columna));
        pos++;
        columna++;
    } else {
        agregarError("comillas sin cerrar en linea " + std::to_string(linea));
    }
}

void Lexer::tokenizarRuta() {
    int inicioColumna = columna;
    std::string ruta;

    while (pos < (int)input.length()) {
        char c = input[pos];
        // Una ruta puede contener letras, números, /, ., -, _
        if (isalnum(c) || c == '/' || c == '.' || c == '-' || c == '_' ) {
            ruta += c;
            pos++;
            columna++;
        } else {
            break;
        }
    }

    if (!ruta.empty()) {
        tokens.push_back(Token(TokenType::RUTA, ruta, linea, inicioColumna));
    }
}

void Lexer::tokenizarRutaDesdePunto() {
    int inicioColumna = columna;
    std::string ruta = ".";
    pos++;
    columna++;

    while (pos < (int)input.length()) {
        char c = input[pos];
        if (isalnum(c) || c == '/' || c == '.' || c == '-' || c == '_') {
            ruta += c;
            pos++;
            columna++;
        } else {
            break;
        }
    }

    if (!ruta.empty()) {
        tokens.push_back(Token(TokenType::RUTA, ruta, linea, inicioColumna));
    }
}