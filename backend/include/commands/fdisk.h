#ifndef FDISK_H
#define FDISK_H

#include <fstream>
#include <string>
#include <vector>
#include "../disk/mbr.h"
#include "../disk/ebr.h"

struct EspacioLibre{
    int start;
    int size;
};

class Fdisk{
    private:
    std::string lastError;
    //metodos para crear particiones en un disco virtual
    bool validarParticion(const MBR& mbr, char type);
    int buscarEspacio(const MBR& mbr, int size, char fit);
    std::vector<EspacioLibre> calcularEspaciosLibres(const MBR& mbr);
    int calcularEspacioLogicas(std::fstream& file, const Partition& extendida);
    bool agregarEspacio(const std::string& path, const std::string& name, int size);
    bool eliminarParticion(const std::string& path, const std::string& name, 
                        const std::string& modo);

    public:
    //validamos que se pueda crear la particion
    
    bool crearParticion(const std::string& path, int size, char fit, char unit, const char type, const std::string& name);
    std::string getLastError() const { return lastError; }
};
#endif