#ifndef RMGRP_H
#define RMGRP_H

#include <string>

class RMGRP {
private:
    std::string lastError;
    
public:
    RMGRP();
    bool eliminarGrupo(const std::string& name);
    std::string getLastError() const { return lastError; }
};

#endif