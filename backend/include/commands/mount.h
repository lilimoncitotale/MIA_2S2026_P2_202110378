#ifndef MOUNT_H
#define MOUNT_H

#include <string>
#include "../disk/mbr.h"

class Mount{
    private:
        std::string lastError;

        //leer MBR desde el disco
        bool readMBR(const std::string& path, MBR& mbr);

        //escribir MBR al disco
        bool writeMBR(const std::string& path, const MBR& mbr);

        //buscar la particion por nombre en el MBR
        int findPartitionByName(const MBR& mbr, const std::string& name);

        //verificar si la particion se puede montar (solo P o L)
        bool canMount(char type);

    public:
        Mount();

        //metodo para montar la particion
        bool montar(const std::string& path, const std::string& name);

        std::string getLastError() const { return lastError; }

};

#endif