#ifndef MKUSR_H
#define MKUSR_H

#include <string>

class MKUSR {
private:
    std::string lastError;
    
public:
    MKUSR();
    bool crearUsuario(const std::string& username, 
                      const std::string& password,
                      const std::string& group);
    std::string getLastError() const { return lastError; }
};

#endif