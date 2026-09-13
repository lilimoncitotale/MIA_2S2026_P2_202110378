#ifndef SUPERBLOCK_H
#define SUPERBLOCK_H

#include <ctime>
#include <cstring>
#include "inode.h"
#include "block.h"
#include "block.h"

#pragma pack(push, 1)

struct SuperBlock{
    
    int s_filesystem_type; // 0 -> 3fs, 1 -> ext2fs
    int s_inodes_count; // Cantidad de inodos
    int s_blocks_count; // Cantidad de bloques
    int s_free_blocks_count; // Cantidad de bloques libres
    int s_free_inodes_count; // Cantidad de inodos libres
    time_t s_mtime; // Ultima fecha de montaje
    time_t s_umtime; // Ultima fecha de desmontaje
    int s_mnt_count; // Veces montado
    int s_magic; // 0xEF53
    int s_inode_size; // Tamaño del inodo
    int s_block_size; // Tamaño del bloque
    int s_first_ino; // Primer inodo libre
    int s_first_blo; // Primer bloque libre
    int s_bm_inode_start; // Inicio del bitmap de inodos
    int s_bm_block_start; // Inicio del bitmap de bloques
    int s_inode_start; // Inicio de los inodos
    int s_block_start; // Inicio de los bloques

    SuperBlock(){
        s_filesystem_type = 2;
        s_inodes_count = 0;
        s_blocks_count = 0;
        s_free_blocks_count = 0;
        s_free_inodes_count = 0;
        s_mtime = time(nullptr);
        s_umtime = time(nullptr);
        s_mnt_count = 0;
        s_magic = 0xEF53;
        s_inode_size = sizeof(Inode);
        s_block_size = sizeof(BlockFile);
        s_first_ino = 1;
        s_first_blo = 1;
        s_bm_inode_start = 0;
        s_bm_block_start = 0;
        s_inode_start = 0;
        s_block_start = 0;
    }
};

#pragma pack(pop)

#endif // SUPERBLOCK_H