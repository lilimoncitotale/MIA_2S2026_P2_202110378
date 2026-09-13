#ifndef MKFS_H
#define MKFS_H

#include <string>
#include <fstream>

class MKFS {
    private:
        std::string lastError;

        //calcular numero de inodos y bloques
        bool calcularEstructuras(int partitionSize, int&inodesCount, int& blocksCount);

        //calcular posiciones de cada estructura
        void calcularPosiciones(int partitionStart, int inodesCount, int blocksCount, int& superblockStart, int& bmInodeStart, int& bmBlockStart, int& inodeTableStart, int& blockTableStart, int& bmINodeSize, int& bmBlockSize);

        //escribir el superblock
        bool escribirSuperBlock(std::fstream& file, int partitionStart, int partitionSize, int inodesCount, int blocksCount, int bmInodeStart, int bmBlockStart, int inodeTableStart, int blockTableStart);

        //inicializar bitmap de inodos
        bool inicializarBitmaps(std::fstream& file, int bmInodeStart, int inodeSize, int bmBlockStart, int blockSize);

        //crear inodos iniciales (raiz y directorio)
        bool crearInodosIniciales(std::fstream& file, int inodeTableStart,int inodeSize, int blockTableStart);

        //crear bloque inicial (raiz)

        bool crearBloquesIniciales(std::fstream& file, int blockTableStart);
    
    public:
        MKFS();

        //metodo principal para formatear la particion
        bool formatear(const std::string& id, const std::string& type);

        std::string getLastError() const {return lastError; }

};

#endif // MKFS_H