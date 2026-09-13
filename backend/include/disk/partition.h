#ifndef PARTITION_H
#define PARTITION_H


#include <cstring>
#include <ctime>

#pragma pack(push, 1) //para que no haya relleno de bytes en la estructura

class Partition{

    
    public:
        char part_status; //estado de la particion indica si esta montada o no
        char part_type; //tipo de particion tendra valores P o E de principal o extendida
        char part_fit; //tipo de ajuste de particion // tendra los valores B, F o W de best fit, first fit o worst fit
        int part_start; //inicio de la particion en que byte
        int part_size; //tamaño de la particion en bytes
        char part_name[16]; //nombre de la particion
        int part_correlative; //numero correlativo de la particion para identificarla de forma unica empezara con -1
        char part_id[4]; //id de la particion montada para identificarla de forma unica, tendra el formato de 4 caracteres
        //constructor inicia los valores a predefinidos
        Partition(){
            part_status = '0';
            part_type = 'P';
            part_fit = 'F';
            part_start = 0;
            part_size = 0;
            memset(part_name, 0, sizeof(part_name));
            part_correlative = -1;
            memset(part_id, 0, sizeof(part_id));
        };
};

#pragma pack(pop) //fin de la estructura sin relleno de bytes

#endif
