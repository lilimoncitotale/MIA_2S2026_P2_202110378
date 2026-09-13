#ifndef MOUNTED_H
#define MOUNTED_H

#include <string>
#include <vector>
#include "mbr.h"

//estructura para guardar una particion montada en RAM

struct MountedPartition{
    std::string diskPath; //ruta del disco virtual
    std::string partitionName; //nombre de la particion
    std::string id; //id de la particion montada
    int correlative; //correlativo de la particion montada
    Partition partition; //informacion de la particion

    MountedPartition(){
        correlative = 0;
    }
};

//clase para manejar las particiones montadas en RAM
class MountedManager{
    private:
    std::vector<MountedPartition> mountedPartitions; //vector de particiones montadas

    //ultimos dos digitos del carnet fijos
    std::string carnetDigits = "78";

    //contadores para generar los IDS
    int lastCorrelativeByDisk; //ultimo numero usado en el disco actual
    char currentLetter;
    std::string lastDiskPath; //ruta del ultimo disco usado

public:
    MountedManager(const std::string& carnet);

    //generar nuevo ID
    std::string generateId(const std::string& diskPath);

    //agregar particion montada
    bool addMounted(const std::string& diskPath, 
                    const std::string& partitionName, 
                    const Partition& partition,
                    const std::string& id);

    //buscar por ID
    MountedPartition* findById(const std::string& id);

    //buscar por disco y nombre
    MountedPartition* findByDiskAndName(const std::string& diskPath, const std::string& partitionName);

    //listar todas las montadas
    void listMounted() const;

    //verificar si la particion ya esta montada 
    bool isMounted(const std::string& diskPath, const std::string& partitionName);

    //getters
    const std::vector<MountedPartition>& getMounted() const {return mountedPartitions;}

    //limpiar (para logout)
    void clear();
};

//variable global del MountedManager
extern MountedManager g_mountedManager;

#endif