#ifndef RMUSR_H
#define RMUSR_H

#include <string>

class RMUSR {
private:
    std::string lastError;
    
public:
    RMUSR();
    bool eliminarUsuario(const std::string& username);
    std::string getLastError() const { return lastError; }
};

#endif