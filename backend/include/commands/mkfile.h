#ifndef MKFILE_H
#define MKFILE_H

#include <string>
#include <map>

class MKFILE {
private:
    std::string lastError;
    
    // Parsear la ruta: separa carpeta padre y nombre del archivo
    bool parsePath(const std::string& fullPath, std::string& parentPath, std::string& fileName);
    
    // Generar contenido según el tamaño (0-9 repetido)
    std::string generateContent(int size);
    
    // Leer contenido de un archivo de la PC
    bool readFileFromPC(const std::string& pcPath, std::string& content);
    
public:
    MKFILE();
    
    bool crearArchivo(const std::map<std::string, std::string>& params);
    std::string getLastError() const { return lastError; }
};

#endif