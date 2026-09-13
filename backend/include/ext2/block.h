#ifndef BLOCK_H
#define BLOCK_H

#include <cstring>

#pragma pack(push, 1)

//contenido de una carpeta (nombre + apuntero al inodo)
struct Content{
    char b_name[12];
    int b_inode;

    Content(){
        memset(b_name, 0, 12);
        b_inode = -1;
    }
};

//bloque de carpeta
struct BlockFolder{
    Content b_content[4];

    BlockFolder(){
        for (Content& content : b_content) {
            content = Content();
        }
    }
};

//bloque de archivo 64 bytes

struct BlockFile {
    char b_content[64];

    BlockFile(){
        memset(b_content, 0, 64);
    }
};

//bloque de apuntadores (16 apuntadores de 4 bytes cada uno)
struct BlockPointer{
    int b_pointer[16];

    BlockPointer(){
        for (int i = 0; i < 16; i++) {
            b_pointer[i] = -1; // Inicializar todos los apuntadores a -1 (no asignados)
        }
    }
};

#pragma pack(pop)
#endif // BLOCK_H
