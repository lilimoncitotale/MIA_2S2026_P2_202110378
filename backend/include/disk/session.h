#ifndef SESSION_H
#define SESSION_H

#include <string>
#include "mbr.h"

//estructura para la sesion activa

struct ActiveSession{
    std::string id; //id de la particion
    std::string diskpath; //ruta del disco
    std::string diskPath; //alias usado por los comandos EXT2
    std::string partitionName; //nombre de la particion
    Partition partition; //particion activa
    std::string username; //usuario logueado
    int uid;    //UID del usuario
    int gid;    //GID del grupo
    bool isActive; //indica si hay una sesion activa

    ActiveSession(){
        isActive = false;
        uid = -1;
        gid = -1;
    }

    void clear(){
        isActive = false;
        id = "";
        diskpath = "";
        diskPath = "";
        partitionName = "";
        username = "";
        uid = -1;
        gid = -1;
    }
};

//variable global para la sesion activa
extern ActiveSession g_activeSession;

#endif // SESSION_H