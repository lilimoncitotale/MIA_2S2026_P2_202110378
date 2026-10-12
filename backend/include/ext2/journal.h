#ifndef JOURNAL_H
#define JOURNAL_H

#include <cstring>

#pragma pack(push, 1)

// Tamaño de la constante del journal
const int JOURNAL_SIZE = 50;

// Información que guarda cada entrada del journal
struct Information {
    char i_operation[10];   // "mkfile", "mkdir", "login", etc.
    char i_path[32];        // Ruta donde se hizo la operación
    char i_content[64];     // Contenido (si es archivo)
    float i_date;           // Fecha en formato float
    
    Information() {
        memset(i_operation, 0, 10);
        memset(i_path, 0, 32);
        memset(i_content, 0, 64);
        i_date = 0.0f;
    }
};

// Entrada del journal
struct Journal {
    int j_count;            // Contador (0-49)
    Information j_content;  // Información de la operación
    
    Journal() {
        j_count = 0;
        // j_content ya se inicializa con su constructor
    }
};

#pragma pack(pop)

#endif