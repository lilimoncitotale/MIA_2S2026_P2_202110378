#include "../../include/ext2/ext2_reader.h"
#include <iostream>
#include <cstring>
#include <sstream>      
#include <vector>       
Ext2Reader::Ext2Reader() : isOpen(false) {}

Ext2Reader::~Ext2Reader() {
    close();
}

bool Ext2Reader::open(const std::string& path, int partitionStart){
    this->diskPath = path;
    this->partitionStart = partitionStart;

    file.open(diskPath, std::ios::binary | std::ios::in);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el disco en la ruta: " << diskPath << std::endl;
        return false;
    }

    if (!readSuperBlock()) {
        std::cerr << "Error: No se pudo leer el superblock." << std::endl;
        file.close();
        return false;

    }

    isOpen = true;
    return true;
}

void Ext2Reader::close(){
    if(file.is_open()){
        file.close();

    }
    isOpen = false;
}


bool Ext2Reader::readSuperBlock(){
    //leer el superblock desde el inicio de la particion
    file.seekg(partitionStart, std::ios::beg);
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBlock));

    if(!file.good()){
        return false;
    }

    //verificar el magic number
    if(sb.s_magic != 0xEF53){
        std::cerr << "Error: El superblock no tiene el magic number esperado." << std::endl;
        return false;
    }

    return true;
}

Inode Ext2Reader::readInode(int inodeIndex){
    Inode inode;
    int inodeOffset = partitionStart + sb.s_inode_start + (inodeIndex * sb.s_inode_size);

    file.seekg(inodeOffset, std::ios::beg);
    file.read(reinterpret_cast<char*>(&inode), sizeof(Inode));

    return inode;
}

bool Ext2Reader::readBlockFolder(int blockIndex, BlockFolder& block){
    int blockOffset = partitionStart + sb.s_block_start + (blockIndex * sb.s_block_size);

    file.seekg(blockOffset, std::ios::beg);
    file.read(reinterpret_cast<char*>(&block), sizeof(BlockFolder));

    return file.good();
}

bool Ext2Reader::readBlockFile(int blockIndex, BlockFile& block){
    int blockOffset = partitionStart + sb.s_block_start + (blockIndex * sb.s_block_size);

    file.seekg(blockOffset, std::ios::beg);
    file.read(reinterpret_cast<char*>(&block), sizeof(BlockFile));

    return file.good();

}

bool Ext2Reader::findFileInFolder(int inodeIndex, const std::string& fileName, int& foundInode){
    Inode inode = readInode(inodeIndex);
    
    std::cout << "🔍 Buscando '" << fileName << "' en inodo " << inodeIndex << std::endl;
    std::cout << "   i_type=" << (int)inode.i_type << std::endl;
    std::cout << "   i_block[0]=" << inode.i_block[0] << std::endl;
    std::cout << "   i_block[1]=" << inode.i_block[1] << std::endl;
    
    // Verificar que sea una carpeta
    if(inode.i_type != 0){
        std::cout << "   ❌ No es carpeta (i_type=" << (int)inode.i_type << ")" << std::endl;
        return false;
    }
    
    // Recorrer los bloques directos (hasta 12)
    for (int i = 0; i<12; i++){
        if (inode.i_block[i] == -1){
            continue;
        }
        
        std::cout << "   📂 Leyendo bloque " << inode.i_block[i] << std::endl;
        
        BlockFolder block;
        if(!readBlockFolder(inode.i_block[i], block)){
            std::cout << "   ❌ No se pudo leer el bloque" << std::endl;
            continue;
        }
        
        // Recorrer las entradas del bloque
        for(int j = 0; j < 4; j++){
            std::string name(block.b_content[j].b_name);
            name = name.c_str();
            
            std::cout << "      [" << j << "] name='" << name 
                      << "' inode=" << block.b_content[j].b_inode << std::endl;
            
            if(!name.empty() && name == fileName){
                foundInode = block.b_content[j].b_inode;
                std::cout << "   ✅ Encontrado en entrada " << j << std::endl;
                return true;
            }
        }
    }
    
    std::cout << "   ❌ No encontrado" << std::endl;
    return false;
}

bool Ext2Reader::readFile(const std::string& path, std::string& content) {
    // ✅ Navegar por la ruta
    int inodeIndex = 0;  // Raíz
    std::stringstream ss(path);
    std::string part;
    std::vector<std::string> parts;
    
    while (std::getline(ss, part, '/')) {
        if (!part.empty()) parts.push_back(part);
    }
    
    for (size_t i = 0; i < parts.size(); i++) {
        int nextInode = -1;
        if (!findFileInFolder(inodeIndex, parts[i], nextInode)) {
            std::cerr << "Error: No se encontró: " << parts[i] << std::endl;
            return false;
        }
        inodeIndex = nextInode;
    }
    
    Inode inode = readInode(inodeIndex);
    
    if (inode.i_type != 1) {
        std::cerr << "Error: No es archivo" << std::endl;
        return false;
    }
    
    content.clear();
    int bytesRead = 0;
    
    for (int i = 0; i < 12 && bytesRead < inode.i_size; i++) {
        if (inode.i_block[i] == -1) break;
        
        BlockFile block;
        readBlockFile(inode.i_block[i], block);
        
        int remaining = inode.i_size - bytesRead;
        int toCopy = (remaining < 64) ? remaining : 64;
        
        content.append(block.b_content, toCopy);
        bytesRead += toCopy;
    }
    
    return true;
}

bool Ext2Reader::readUsersFile(std::vector<std::string>& lines){
    std::string content;
    if(!readFile("users.txt", content)){
        return false;

    }

    //dividir por lineas
    std::string line;

    for(char c : content){
        if(c == '\n'){
            if(!line.empty()){
                lines.push_back(line);
                line.clear();
            }
        }else{
            line += c;
        }
    }

    //ultima linea si no termina en \n
    if(!line.empty()){
        lines.push_back(line);
    }

    return true;
}
