#include "../../include/commands/fdisk.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>

bool Fdisk::crearParticion(const std::string& path, int size, char fit, char unit, const char type, const std::string& name){
    if (unit == 'K') size *= 1024;
    else if (unit == 'M') size *= 1024 * 1024;
    else if (unit != 'B') return false;

     // 1. Abrimos el disco virtual
    std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        lastError = "No se pudo abrir el disco: " + path;
        std::cout << "❌ Error: " << lastError << std::endl;
        return false;
    }

    // 2. Leemos el MBR
    MBR mbr;
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
    
    // ✅ Verificar que el MBR sea válido
    if (mbr.mbr_size <= 0) {
        lastError = "El archivo no es un disco válido (MBR corrupto)";
        std::cout << "❌ Error: " << lastError << std::endl;
        file.close();
        return false;
    }

    // 3. Validamos que se pueda crear la partición
    if (!validarParticion(mbr, type)) {
        // lastError ya se guarda dentro de validarParticion
        file.close();
        return false;
    }

    // 4. Buscamos espacio libre
    int start = buscarEspacio(mbr, size, fit);
    if (start == -1) {
        lastError = "No hay espacio suficiente para la partición";
        std::cout << "❌ Error: " << lastError << std::endl;
        file.close();
        return false;
    }

    //validacion para logicas
    if (type == 'L'){
        bool espacioSuficiente = false;
        for (int i = 0; i < 4; i++){
            if(mbr.mbr_partitions[i].part_type == 'E'){
                //calcular el espacio usado por logicas existentes
                int usado = calcularEspacioLogicas(file, mbr.mbr_partitions[i]);
                int disponible = mbr.mbr_partitions[i].part_size - usado;

                std::cout << " Espacio en extendida: " << mbr.mbr_partitions[i].part_size << " bytes" << std::endl;
                std::cout << "   Usado por lógicas: " << usado << " bytes" << std::endl;
                std::cout << "   Disponible: " << disponible << " bytes" << std::endl;
                std::cout << "   Necesario: " << size << " bytes" << std::endl;

                if (size > disponible){
                    std::cout << "Error: No hay espacio suficiente en la particion extendida para crear la logica." << std::endl;
                    file.close();
                    return false;

                }
                espacioSuficiente = true;
                break;
            }
        }
        if (!espacioSuficiente){
            std::cout << "Error: No hay particion extendida para crear la logica." << std::endl;
            file.close();
            return false;
        }
    }

    \
    //5. buscar la primera particion libre en el MBR solo para primarias y extendidas
    int slot = -1;
    if (type == 'P' || type == 'E'){
        for (int i = 0; i<4; i++){
            if (mbr.mbr_partitions[i].part_size == 0){
                slot = i;
                break;
            }
        }
        if (slot == -1){
            return false; //no hay espacio para la particion
        }

    
    }

    //6.llenamos la particion en el MBR
    if (type == 'P' || type == 'E'){
        //primaria o extendidda va en el MBR
        Partition& part = mbr.mbr_partitions[slot];
        part.part_status = '0'; 
        part.part_type = type;
        part.part_fit = fit;
        part.part_start = start;
        part.part_size = size;
        strncpy(part.part_name, name.c_str(), sizeof(part.part_name));
        part.part_correlative = -1;
        memset(part.part_id,0,4);

        //si es extendida debemos crear la EBR inicial en el inicio de la particion extendida
        if (type == 'E'){
            EBR ebr;
            ebr.part_mount = '0';
            ebr.part_fit = 'F';
            ebr.part_start = start + sizeof(EBR); //el EBR inicial va al inicio de la particion extendida
            ebr.part_size = 0; //no hay logicas aun
            ebr.part_next = -1; //no hay logicas aun
            strncpy(ebr.part_name, "EBR_INICIAL", sizeof(ebr.part_name));

            //escribimos el ebr al inicio de la extendida
            file.seekp(start, std::ios::beg);
            file.write(reinterpret_cast<const char*>(&ebr), sizeof(EBR));
        }


    }else if(type == 'L'){
        //logica , se agrega al final de la lista de logicas de la extendida, para eso debemos buscar la extendida y recorrer sus logicas
        //buscamos la extendida
        for (int i = 0; i<4; i++){
            if(mbr.mbr_partitions[i].part_type == 'E'){
                //recorremos ebr hasta el ultimo
                int ebrStart = mbr.mbr_partitions[i].part_start;
                int ebrPos = mbr.mbr_partitions[i].part_start;
                EBR ebr;

                while(ebrPos != -1){
                    file.seekg(ebrPos, std::ios::beg);
                    file.read(reinterpret_cast<char*>(&ebr), sizeof(EBR));
                    if (ebr.part_next == -1){
                        //este es el ultimo ebr, agregamos la nueva logica aqui
                        EBR newEbr;
                        newEbr.part_mount = '0';
                        newEbr.part_fit = fit;
                        newEbr.part_start = ebrPos + sizeof(EBR) + ebr.part_size; //el inicio de la nueva logica es el final del ultimo ebr
                        newEbr.part_size = size;
                        newEbr.part_next = -1; //no hay siguiente logica aun
                        strncpy(newEbr.part_name, name.c_str(), sizeof(newEbr.part_name));

                        //escribimos el nuevo ebr al final del ultimo ebr
                        int nuevaPos = newEbr.part_start;
                        file.seekp(nuevaPos, std::ios::beg);
                        file.write(reinterpret_cast<const char*>(&newEbr), sizeof(EBR));

                        //actualizar el ebr anterior para que apunte al nuevo ebr
                        ebr.part_next = nuevaPos;
                        file.seekp(ebrPos, std::ios::beg);
                        file.write(reinterpret_cast<const char*>(&ebr), sizeof(EBR));

                        break;


                    }
                    ebrPos = ebr.part_next;

                }
                break;
            }
        }
    }
    //escribimos el MBR actualizado al inicio del disco
    file.seekp(0, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&mbr), sizeof(MBR));
    file.close();
    return true;
}

int Fdisk::calcularEspacioLogicas(std::fstream& file, const Partition& extendida){
    int usado = 0;
    int ebrPos = extendida.part_start;
    EBR ebr;

    while (ebrPos != -1){
        file.seekg(ebrPos, std::ios::beg);
        file.read(reinterpret_cast<char*>(&ebr), sizeof(EBR));
        usado += ebr.part_size + sizeof(EBR); //sumamos el tamaño de la logica y el tamaño del EBR
        ebrPos = ebr.part_next;
    }
    return usado;
}

bool Fdisk::validarParticion(const MBR& mbr, char type){
    //primero contamos las particiones existentes
    int countUsed = 0;
    for (int i = 0; i<4; i++){
        if(mbr.mbr_partitions[i].part_size > 0){
            //si es 1 significa que esta usada
            countUsed++;
        }
    }

    //regla 1 maximo 4 particiones primarias + extendidas
    if(countUsed >=4 && type != 'L'){
        std::cout << "Error: No se pueden crear mas particiones, ya existen 4 particiones primarias o extendidas." << std::endl;
        return false;
    }
    //regla 2 solo puede existir una particion extendida
    if(type == 'E'){
        for (int i = 0; i<4; i++){
            if(mbr.mbr_partitions[i].part_type == 'E'){
                return false; //ya existe una particion extendida
            
            }
        }
    }

    //regla 3 si es logica debe existir una particion extendida
    if (type == 'L'){
        bool hayExtendida= false;
        for (int i = 0; i<4; i++){
            if(mbr.mbr_partitions[i].part_type == 'E'){
                hayExtendida = true;
                break;
            }
        }
        if (!hayExtendida){
            return false; //no hay particion extendida para crear la logica
        }
    }

    
    return true;

}

int Fdisk::buscarEspacio(const MBR& mbr, int size, char fit){
    //calculamos los espacios disponibles
    std::vector<EspacioLibre> espacios = calcularEspaciosLibres(mbr);

    //ordenamos segun el fit
    if (fit == 'F'){
        //first fit, no necesitamos ordenar
    }else if (fit == 'B'){ //best fit, ordenar por tamanio, menor a mayor
        std::sort(espacios.begin(), espacios.end(), [](const EspacioLibre& a, const EspacioLibre&b){
            return a.size < b.size;
        });

    }else if (fit == 'W'){//worst fit ordenar por tamanio mayor a menor
        std::sort(espacios.begin(), espacios.end(), [](const EspacioLibre& a, const EspacioLibre&b){
            return a.size > b.size;
        });

    }

    //buscamos el primero que sea suficiente para el tamaño solicitado
    for (const auto& espacio : espacios){
        if (espacio.size >= size){
            return espacio.start; //encontramos un espacio suficiente
        }
    }
    return -1; //no hay espacio suficiente 
}

std::vector<EspacioLibre> Fdisk::calcularEspaciosLibres(const MBR& mbr){
    std::vector<EspacioLibre> espacios;
    //el mbr ocupa desde el byte o hasta el size del mbr
    int currentPosition = sizeof(MBR);
    int diskSize = mbr.mbr_size;

    //recorremos las 4 parciiones del mbr (solo primarias y extendidas)
    for (int i = 0; i<4; i++){
        const Partition& partition = mbr.mbr_partitions[i];

        if (partition.part_size >0){ //particion ocupada
            //hay espacio disponible entre currentPosition y el inicio de la particion
            if (partition.part_start > currentPosition){
                EspacioLibre espacio;
                espacio.start = currentPosition;
                espacio.size = partition.part_start - currentPosition;
                espacios.push_back(espacio);
            }
            //actualizamos currentPosition al final de la particion
            currentPosition = partition.part_start + partition.part_size;
        }
    }
    //veridicamos si hay espacio disponible al final del disco
    if (diskSize > currentPosition){
        EspacioLibre espacio;
        espacio.start = currentPosition;
        espacio.size = diskSize - currentPosition;
        espacios.push_back(espacio);
    }
    return espacios;
}