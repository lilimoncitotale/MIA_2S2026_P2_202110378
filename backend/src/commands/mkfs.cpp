#include "../../include/commands/mkfs.h"
#include "../../include/ext2/superblock.h"
#include "../../include/ext2/inode.h"
#include "../../include/ext2/block.h"
#include "../../include/disk/mounted.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>

MKFS::MKFS(){}

//calcular inodos y bloques

bool MKFS::calcularEstructuras(int partitionSize, int& inodesCount, int& blocksCount){
    //formula partitionSIze = sizeof(superBlock) + n + 3n + n*sizeof(inode) + 3n*64

    //donde n = numero de inodos

    int superBlockSize = sizeof(SuperBlock);
    int inodeSize = sizeof(Inode);
    int blockSize = 64; //sizeof(Block)

    //n = (parttitionsize - superBlockSize) / (1 + 3 + inodeSize + 3*blockSize)
    long long divisor = 1+3+inodeSize + 3*blockSize;
    long long numerador = partitionSize - superBlockSize;

    if(numerador <= 0){
        lastError = "Error: El tamaño de la particion es demasiado pequeño para crear un sistema de archivos";
        return false;

    }
    inodesCount = numerador / divisor;
    if (inodesCount < 2){
        lastError = "Error: El tamaño de la particion es demasiado pequeño para crear un sistema de archivos";
        return false;

    }

    blocksCount = 3*inodesCount;
    std::cout << "  Cálculo de estructuras:" << std::endl;
    std::cout << "   Tamaño partición: " << partitionSize << " bytes" << std::endl;
    std::cout << "   Inodos: " << inodesCount << std::endl;
    std::cout << "   Bloques: " << blocksCount << std::endl;
    
    return true;


}

void MKFS::calcularPosiciones(int partitionStart, int inodesCount, int blocksCount, int& superblockStart, int& bmInodeStart, int& bmBlockStart, int& inodeTableStart, int& blockTableStart, int& bmINodeSize, int& bmBlockSize){

    int currentPos = partitionStart;

    //superblock
    superblockStart = currentPos;
    currentPos += sizeof(SuperBlock);

    //bitmap de inodos (n bits = ceil(n/8) bytes)
    bmINodeSize = (inodesCount + 7) / 8; //ceil(n/8)
    bmInodeStart = currentPos;
    currentPos += bmINodeSize;

    //bitmap de bloques
    bmBlockSize = (blocksCount + 7) / 8; //ceil(n/8)
    bmBlockStart = currentPos;
    currentPos += bmBlockSize;

    //tabla de inodos
    inodeTableStart = currentPos;
    currentPos += inodesCount * sizeof(Inode);

    //tabla de bloques
    blockTableStart = currentPos;
    currentPos += blocksCount * 64;

}

//escribir superblock
bool MKFS::escribirSuperBlock(std::fstream& file, int partitionStart, int partitionSize, int inodesCount, int blocksCount, int bmInodeStart, int bmBlockStart, int inodeTableStart, int blockTableStart){
    SuperBlock sb;
    sb.s_inodes_count = inodesCount;
    sb.s_blocks_count = blocksCount;
    sb.s_free_inodes_count = inodesCount - 2; //inodo 0 y 1 ya estan ocupados
    sb.s_free_blocks_count = blocksCount - 2; //bloque 0 y 1 ya estan ocupados
    sb.s_mtime = time(nullptr);
    sb.s_umtime = time(nullptr);
    sb.s_mnt_count = 0;
    sb.s_magic = 0xEF53;
    sb.s_inode_size = sizeof(Inode);
    sb.s_block_size = 64;
    sb.s_first_ino = 2; //inodo 0 y 1 ya estan ocupados
    sb.s_first_blo = 2; //bloque 0 y 1 ya estan ocupados

    //guardar offsets relativos al inicio d la particion

    sb.s_bm_inode_start = bmInodeStart - partitionStart;
    sb.s_bm_block_start = bmBlockStart - partitionStart;
    sb.s_inode_start = inodeTableStart - partitionStart;
    sb.s_block_start = blockTableStart - partitionStart;

    //escribir SuperBlock
    file.seekp(partitionStart, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&sb), sizeof(SuperBlock));

    if (!file.good()){
        lastError = "Error: No se pudo escribir el superblock en la particion";
        return false;
    }

    std::cout << "  Superblock escrito exitosamente en la particion" << std::endl;
    return true;
}


//inicializar bitmaps
bool MKFS::inicializarBitmaps(std::fstream& file, int bmInodeStart, int inodeSize, int bmBlockStart, int blockSize){
    //bitmap de inodos: todos en 0, luego marcar 0 y 1 como ocupados
    std::vector<char> bmInode(inodeSize, 0);
    if(inodeSize > 0){
        bmInode[0] = 0b00000011;
        //si el primer byte tiene mas bits, marcar los demas como ocupados
    }

    file.seekp(bmInodeStart, std::ios::beg);
    file.write(bmInode.data(), inodeSize);

    if(!file.good()){
        lastError = "Error: No se pudo escribir el bitmap de inodos en la particion";
        return false;
    }

    //bitmap de bloques: todos en 0, luego marcar 0 y 1 como ocupados
    std::vector<char> bmBlock(blockSize, 0);
    if(blockSize > 0){
        bmBlock[0] = 0b00000011;
    }

    file.seekp(bmBlockStart, std::ios::beg);
    file.write(bmBlock.data(), blockSize);

    if(!file.good()){
        lastError = "Error: No se pudo escribir el bitmap de bloques en la particion";
        return false;
    }

    std::cout << "  Bitmaps de inodos y bloques inicializados exitosamente" << std::endl;
    return true;
    
}

//crear nodos iniciales
bool MKFS::crearInodosIniciales(std::fstream& file, int inodeTableStart,int inodeSize, int blockTableStart){

    //crear vector de inodos(todos vacios)
    std::vector<Inode> inodes(2); //solo necesitamos los primeros 2

    //inodo 0: inodo de la raiz

    inodes[0].i_uid = 1; //root
    inodes[0].i_gid = 1; //root
    inodes[0].i_size = 0; //tamaño 0
    inodes[0].i_atime = time(nullptr);
    inodes[0].i_ctime = time(nullptr);
    inodes[0].i_mtime = time(nullptr);
    inodes[0].i_type = 0; //directorio
    inodes[0].i_perm[0] = '7'; //rwx
    inodes[0].i_perm[1] = '7';
    inodes[0].i_perm[2] = '7';
    inodes[0].i_block[0] = 0; //primer bloque de

    //inodo 1 archivo users.txt

    std::string usersContent = "1, G, root\n1, U, root, root, 123\n";

    inodes[1].i_uid = 1; //root
    inodes[1].i_gid = 1; //root
    inodes[1].i_size = usersContent.size(); //tamaño del contenido
    inodes[1].i_atime = time(nullptr);
    inodes[1].i_ctime = time(nullptr);
    inodes[1].i_mtime = time(nullptr);
    inodes[1].i_type = 1; //archivo
    inodes[1].i_perm[0] = '6'; //rw-
    inodes[1].i_perm[1] = '6';
    inodes[1].i_perm[2] = '4';
    inodes[1].i_block[0] = 1; //primer bloque de datos

    std::cout << "📝 Inodo 1 (users.txt):" << std::endl;
    std::cout << "   size=" << inodes[1].i_size << std::endl;
    std::cout << "   type=" << (int)inodes[1].i_type << std::endl;
    std::cout << "   block[0]=" << inodes[1].i_block[0] << std::endl;
    //escribir los inodos en la tabla
    file.seekp(inodeTableStart, std::ios::beg);
    file.write(reinterpret_cast<const char*>(inodes.data()), 2 * sizeof(Inode));

    if(!file.good()){
        lastError = "Error: No se pudo escribir los inodos iniciales en la particion";
        return false;
    }

    std::cout << "  Inodos iniciales creados exitosamente" << std::endl;
    return true;
}

//crear bloques iniciales
bool MKFS::crearBloquesIniciales(std::fstream& file, int blockTableStart){
    //bloque 0 carpeta raiz
    BlockFolder rootBlock;

    //entrada 0: "." (directorio actual)
    strncpy(rootBlock.b_content[0].b_name, ".", 11);
    rootBlock.b_content[0].b_name[11] = '\0'; //asegurarse de que sea null-terminated
    rootBlock.b_content[0].b_inode = 0; //inodo de la

    //entrada 1 ".." (directorio padre)
    strncpy(rootBlock.b_content[1].b_name, "..", 11);
    rootBlock.b_content[1].b_name[11] = '\0'; //asegurarse de que sea null-terminated
    rootBlock.b_content[1].b_inode = 0; //inodo de la raiz

    //entrada 2: "users.txt"
    strncpy(rootBlock.b_content[2].b_name, "users.txt", 11);
    rootBlock.b_content[2].b_name[11] = '\0'; //asegurarse de que sea null-terminated
    rootBlock.b_content[2].b_inode = 1; //inodo del archivo users.txt   

    //escribir el bloque raiz en la tabla de bloques
    file.seekp(blockTableStart, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&rootBlock), sizeof(BlockFolder));

    if(!file.good()){
        lastError = "Error: No se pudo escribir el bloque raiz en la particion";
        return false;
    }

    //bloque 1 archivo users.txt

    BlockFile usersBlock;
    std::string usersContent = "1, G, root\n1, U, root, root, 123\n";

    //copiar contenido al bloque
    size_t copySize = usersContent.size();
    if (copySize > 64) copySize = 64; //asegurarse de no exceder el tamaño del bloque
    memcpy(usersBlock.b_content, usersContent.c_str(), copySize);

    //rellenar el resto con ceros 
    if(copySize < 64){
        memset(usersBlock.b_content + copySize, 0, 64 - copySize);
    }

    //escribir bloque 1
    file.seekp(blockTableStart + sizeof(BlockFolder), std::ios::beg);
    file.write(reinterpret_cast<const char*>(&usersBlock), sizeof(BlockFile));

    if (!file.good()){
        lastError = "Error: No se pudo escribir el bloque de users.txt en la particion";
        return false;
    }

    std::cout << "  Bloques iniciales creados exitosamente" << std::endl;
    return true;

}


//metodo principal: formatear

bool MKFS::formatear(const std::string& id, const std::string&type){
    std::cout << "  Formateando partición EXT2..." << std::endl;
    std::cout << "   ID: " << id << std::endl;
    std::cout << "   Tipo: " << type << std::endl;

    //buscar la particion en memoria RAM

    MountedPartition* mounted = g_mountedManager.findById(id);
    if(!mounted){
        lastError = "Error: No se encontró la partición con ID: " + id;
        return false;
    }

    //obtener datos de la particion

    std::string diskPath = mounted->diskPath;
    Partition partition = mounted->partition;
    int partitionStart = partition.part_start;
    int partitionSize = partition.part_size;

    std::cout << "   Disco: " << diskPath << std::endl;
    std::cout << "   Partición: " << mounted->partitionName << std::endl;
    std::cout << "   Inicio: " << partitionStart << " bytes" << std::endl;
    std::cout << "   Tamaño: " << partitionSize << " bytes" << std::endl;

    //abrir el disco
    std::fstream file(diskPath, std::ios::in | std::ios::out | std::ios::binary);
    if(!file.is_open()){
        lastError = "Error: No se pudo abrir el disco: " + diskPath;
        return false;
    }

    //calcular inodos y bloques

    int inodesCount, blocksCount;
    if(!calcularEstructuras(partitionSize, inodesCount, blocksCount)){
        file.close();
        return false;
    }

    //calcular posiciones
    int superBlockStart, bmInodeStart, bmBlockStart;
    int inodeTableStart, blockTableStart;
    int bmInodeSize, bmBlockSize;

    calcularPosiciones(partitionStart, inodesCount, blocksCount, superBlockStart, bmInodeStart, bmBlockStart, inodeTableStart, blockTableStart, bmInodeSize, bmBlockSize); 
    std::cout << "  Posiciones de estructuras:" << std::endl;
    std::cout << "   SuperBlock: " << superBlockStart << std::endl;
    std::cout << "   Bitmap Inodos: " << bmInodeStart << " (" << bmInodeSize << " bytes)" << std::endl;
    std::cout << "   Bitmap Bloques: " << bmBlockStart << " (" << bmBlockSize << " bytes)" << std::endl;
    std::cout << "   Tabla Inodos: " << inodeTableStart << std::endl;
    std::cout << "   Tabla Bloques: " << blockTableStart << std::endl;

    //escribir superblock
    if(!escribirSuperBlock(file, partitionStart, partitionSize, inodesCount, blocksCount, bmInodeStart, bmBlockStart, inodeTableStart, blockTableStart)){
        file.close();
        return false;
    }

    if(!inicializarBitmaps(file, bmInodeStart, bmInodeSize, bmBlockStart, bmBlockSize)){
        file.close();
        return false;
    }

    //crear inodos iniciales
    if(!crearInodosIniciales(file, inodeTableStart, sizeof(Inode), blockTableStart)){
        file.close();
        return false;
    }

    //crear bloques iniciales
    if(!crearBloquesIniciales(file, blockTableStart)){
        file.close();
        return false;
    }

    file.close();

    // 10. Mostrar éxito
    std::cout << "=========================================" << std::endl;
    std::cout << "  Partición formateada exitosamente como EXT2" << std::endl;
    std::cout << "   ID: " << id << std::endl;
    std::cout << "   Inodos: " << inodesCount << std::endl;
    std::cout << "   Bloques: " << blocksCount << std::endl;
    std::cout << "   Tamaño de partición: " << partitionSize << " bytes" << std::endl;
    std::cout << "   Archivo users.txt creado con usuario root" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    return true;
}
