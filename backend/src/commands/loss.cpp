#include "../../include/commands/loss.h"
#include "../../include/disk/mounted.h"
#include "../../include/ext2/superblock.h"
#include <iostream>
#include <fstream>
#include <vector>

Loss::Loss() {}

bool Loss::simular(const std::string& id) {
    MountedPartition* mounted = g_mountedManager.findById(id);
    if (!mounted) {
        lastError = "ID no encontrado: " + id;
        return false;
    }
    
    std::string diskPath = mounted->diskPath;
    int partitionStart = mounted->partition.part_start;
    
    std::fstream file(diskPath, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        lastError = "No se pudo abrir el disco";
        return false;
    }
    
    // Leer SuperBlock
    file.seekg(partitionStart, std::ios::beg);
    SuperBlock sb;
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBlock));
    
    // Calcular posiciones de los bloques a borrar
    int bmInodeStart = partitionStart + sb.s_bm_inode_start;
    int bmInodeSize = (sb.s_inodes_count + 7) / 8;
    
    int bmBlockStart = partitionStart + sb.s_bm_block_start;
    int bmBlockSize = (sb.s_blocks_count + 7) / 8;
    
    int inodeStart = partitionStart + sb.s_inode_start;
    int inodeSize = sb.s_inodes_count * sizeof(Inode);
    
    int blockStart = partitionStart + sb.s_block_start;
    int blockSize = sb.s_blocks_count * 64;
    
    // Llenar con ceros
    std::vector<char> zeros;
    
    // 1. Bitmap inodos
    zeros.assign(bmInodeSize, 0);
    file.seekp(bmInodeStart, std::ios::beg);
    file.write(zeros.data(), bmInodeSize);
    
    // 2. Bitmap bloques
    zeros.assign(bmBlockSize, 0);
    file.seekp(bmBlockStart, std::ios::beg);
    file.write(zeros.data(), bmBlockSize);
    
    // 3. Área de inodos
    zeros.assign(inodeSize, 0);
    file.seekp(inodeStart, std::ios::beg);
    file.write(zeros.data(), inodeSize);
    
    // 4. Área de bloques
    zeros.assign(blockSize, 0);
    file.seekp(blockStart, std::ios::beg);
    file.write(zeros.data(), blockSize);
    
    file.close();
    
    std::cout << " LOSS aplicado a partición " << id << std::endl;
    std::cout << "   Bitmaps, inodos y bloques borrados" << std::endl;
    std::cout << "   El journaling NO fue afectado" << std::endl;
    
    return true;
}