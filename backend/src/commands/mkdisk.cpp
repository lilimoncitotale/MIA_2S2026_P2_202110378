#include <iostream>
#include "../../include/commands/mkdisk.h"
#include "../../include/disk/diskManager.h"
#include "../../include/disk/disk.h"

#include <string>
#include <iostream>

void MkDisk::crearDisco (int size, std::string fit, std::string unit, std::string path) {
    // Implementación de la función para crear un disco virtual
    //por ahora solo el mbr
    //solo filtramos el Fit por que mbr espera un char no un string
    char fitChar = 'F'; // Valor por defecto
    if (fit == "BF"){
        //si es BF en mbr solo espera B
        fitChar = 'B';
    }else if (fit == "FF"){
        //si es FF en mbr solo espera F
        fitChar = 'F';
    }else if (fit == "WF"){
        //si es WF en mbr solo espera W
        fitChar = 'W';
    }else{
        std::cout << "Error: Fit no valido. Debe ser BF, FF o WF." << std::endl;
        return;
    }
    //ahora modificamos el tamaño del disco segun la unidad
    if (unit == "K"){
        size = size * 1024; //convertimos a bytes
    }else if (unit == "M"){
        size = size * 1024 * 1024; //convertimos a bytes
    }

    Disk disk(size, fitChar, unit, path);
    // Guardamos el disco en el archivo
    if (disk.saveToFile()){
        std::cout << "Disco virtual creado exitosamente en: " << path << std::endl;
        std::cout << "Tamaño: " << size << " bytes, Fit: " << fitChar << std::endl;
        // Agregamos el disco al DiskManager
        g_diskManager.addDisk(disk);
    }else{
        std::cout << "Error: No se pudo crear el disco virtual en: " << path << std::endl;
    }

    
            
            
}
