#ifndef LOSS_H
#define LOSS_H

#include <string>

class Loss {
private:
    std::string lastError;
    
public:
    Loss();
    bool simular(const std::string& id);
    std::string getLastError() const { return lastError; }
};

#endif