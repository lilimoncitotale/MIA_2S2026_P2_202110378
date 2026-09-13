#ifndef LOGOUT_H
#define LOGOUT_H

#include <string>

class Logout{
    private:
        std::string lastError;

    public:
        Logout();

        //metodo principal
        bool cerrarSesion();

        std::string getLastError() const {return lastError;}
};

#endif // LOGOUT_H