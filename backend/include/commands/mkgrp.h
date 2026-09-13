#ifndef MKGRP_H
#define MKGRP_H

#include <string>

class MKGRP {
private:
    std::string lastError;
    
public:
    MKGRP();
    bool crearGrupo(const std::string& name);
    std::string getLastError() const { return lastError; }
};

#endif