#include "../../include/disk/session.h"
#include "../../include/commands/logout.h"
#include <iostream>

Logout::Logout(){}

bool Logout::cerrarSesion(){
    std::cout << "  Cerrando sesión..." << std::endl;

    //verificar que haya sesion activa
    if(!g_activeSession.isActive){
        lastError = "No hay sesión activa para cerrar.";
        return false;
    }

    //mostrar informacion de la sesion que se va a cerrar
    std::cout << "   Usuario: " << g_activeSession.username << std::endl;
    std::cout << "   ID de partición: " << g_activeSession.id << std::endl;
    std::cout << "   Disco: " << g_activeSession.diskpath << std::endl;

    //cerrar la sesion
    g_activeSession.clear();

    //mostrar exito
    std::cout << "  Sesión cerrada exitosamente." << std::endl;

    return true;
                
    
}