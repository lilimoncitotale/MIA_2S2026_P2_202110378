#ifndef MBR_H
#define MBR_H



#include <cstring>
#include <ctime>
#include <cstdlib>
#include "partition.h"

#pragma pack(push, 1) //para que no haya relleno de bytes en la estructura


class MBR {
    public:
        int mbr_size;
        time_t mbr_date;
        int mbr_disk_signature;  //identifica al disco de forma unica
        char fit; //tipo de ajuste de particion
        Partition mbr_partitions[4]; //arreglo de particiones

    public:
        //constructor inicia los valores a predefinidos
        MBR(){
            mbr_size = 0;
            mbr_date = time(nullptr); //obtiene la fecha y hora actual
            mbr_disk_signature = rand(); //genera un numero aleatorio entre 0 y 100000
            fit = 'F';
            //las particiones ya se inicializan con el constructor de la clase Partition
        };

};

#pragma pack(pop) //fin de la estructura sin relleno de bytes

#endif