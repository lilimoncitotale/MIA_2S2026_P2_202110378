#ifndef JOURNALING_H
#define JOURNALING_H

#include <string>

class Journaling {
private:
    std::string lastError;
    
public:
    Journaling();
    bool mostrar(const std::string& id);
    std::string getLastError() const { return lastError; }
};

#endif