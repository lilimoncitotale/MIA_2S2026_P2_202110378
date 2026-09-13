#ifndef CHGRP_H
#define CHGRP_H

#include <string>

class CHGRP {
private:
    std::string lastError;
    
public:
    CHGRP();
    bool cambiarGrupo(const std::string& username, const std::string& group);
    std::string getLastError() const { return lastError; }
};

#endif