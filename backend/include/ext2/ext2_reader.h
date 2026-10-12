#ifndef EXT2_READER_H
#define EXT2_READER_H

#include <string>
#include <vector>
#include <fstream>
#include "superblock.h"
#include "inode.h"
#include "block.h"

class Ext2Reader{
    protected:

        std::string diskPath;
        int partitionStart;
        SuperBlock sb;
        std::fstream file;
        bool isOpen;

        bool readSuperBlock();
        Inode readInode(int inodeIndex);
        bool readBlockFolder (int blockIndex, BlockFolder& block);
        bool readBlockFile (int blockIndex, BlockFile& block);

    public:

        Ext2Reader();
        ~Ext2Reader();
        bool open(const std::string& path, int partitionStart);
        void close();

        //leer el contenido de un archivo por su ruta
        bool readFile(const std::string& path, std::string& content);

        //buscar un archivo por nombre en una carpeta
        bool findFileInFolder(int inodeIndex, const std::string& fileName, int& foundInode);

        //leer el archivo users.txt y parsearlo
        bool readUsersFile(std::vector<std::string>& lines);
        Inode readInodePublic(int index) { return readInode(index); }
        const SuperBlock& getSB() const { return sb; }
        bool readBlockFolderPublic(int index, BlockFolder& block) { return readBlockFolder(index, block); }
        bool readBlockFilePublic(int index, BlockFile& block) { return readBlockFile(index, block); }

};

#endif // EXT2_READER_H