#ifndef INODE_H
#define INODE_H

#include <ctime>
#include <cstring>

#pragma pack(push, 1)

struct Inode{
    int i_uid; // Identificador del usuario propietario
    int i_gid; // Identificador del grupo propietario
    int i_size; // Tamaño del archivo en bytes
    time_t i_atime; // Último acceso
    time_t i_ctime; // Última creación
    time_t i_mtime; // Última modificación
    int i_block[15]; // Punteros a bloques
    char i_type; // Tipo de archivo: '0' para archivo, '1'
    char i_perm[3]; // Permisos de archivo: lectura, escritura, ejecución

    Inode(){
        i_uid = 1;
        i_gid = 1;
        i_size = 0;
        i_atime = time(nullptr);
        i_ctime = time(nullptr);
        i_mtime = time(nullptr);
        for (int i = 0; i < 15; i++) {
            i_block[i] = -1; // Inicializar punteros a bloques como -1 (no asignados)
        }
        i_type = '0'; // Inicializar tipo de archivo como '0' (archivo
        i_perm[0] = '7'; 
        i_perm[1] = '7';
        i_perm[2] = '7';
    }
};

#pragma pack(pop)
#endif // INODE_H