#ifndef CAT_H
#define CAT_H

#include <string>
#include <vector>
#include <map>

class CAT {
private:
    std::string lastError;
    std::string output;
    
public:
    CAT();
    
    // Recibe un mapa con -file1, -file2, ..., -fileN
    bool leerArchivos(const std::map<std::string, std::string>& params);
    
    std::string getLastError() const { return lastError; }
    std::string getOutput() const { return output; }
};

#endif