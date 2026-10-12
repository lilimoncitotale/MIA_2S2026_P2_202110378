#ifndef EXT2_WRITER_H
#define EXT2_WRITER_H

#include <string>
#include <vector>
#include <fstream>
#include "ext2_reader.h"
#include "journal.h"

class Ext2Writer : public Ext2Reader {
private:
    bool updateBitmap(int bitmapStart, int bitIndex, bool occupied);
    bool updateSuperBlock(const SuperBlock& sb);
    
public:
    Ext2Writer();
    ~Ext2Writer();
    
    bool openForWrite(const std::string& path, int partitionStart);
    
    // ============================================
    // BITMAPS
    // ============================================
    int findFreeInode();
    int findFreeBlock();
    bool markInodeOccupied(int inodeIndex);
    bool markBlockOccupied(int blockIndex);
    
    // ============================================
    // INODOS Y BLOQUES
    // ============================================
    Inode readInode(int inodeIndex);
    bool writeInode(int inodeIndex, const Inode& inode);
    bool writeBlockFolder(int blockIndex, const BlockFolder& block);
    bool writeBlockFile(int blockIndex, const BlockFile& block);
    
    // ============================================
    // USERS.TXT
    // ============================================
    bool readUsersFile(std::vector<std::string>& lines);
    bool writeUsersFile(const std::vector<std::string>& lines);
    int getNextGID(const std::vector<std::string>& lines);
    int getNextUID(const std::vector<std::string>& lines);
    bool findGroup(const std::vector<std::string>& lines, const std::string& name, int& gid);
    bool findUser(const std::vector<std::string>& lines, const std::string& name, int& uid);
    
    // ============================================
    // ARCHIVOS Y CARPETAS
    // ============================================
    bool findFileInFolder(int folderInode, const std::string& name, int& foundInode);
    bool addEntryToFolder(int folderInode, const std::string& name, int childInode);
    bool createFolder(int parentInode, const std::string& name, int uid, int gid, int& newInode);
    bool createFile(int parentInode, const std::string& name, int uid, int gid, 
                    const std::string& content, int& newInode);
    
    // ============================================
    // UTILIDADES
    // ============================================
    int findFolderByPath(const std::string& path);
    bool createFolderRecursive(const std::string& path, int uid, int gid, int& newInode);
    int findInodeByPath(const std::string& path);
    int findParentInode(const std::string& path);
    std::string getLastName(const std::string& path) const;
    bool hasPermission(int inodeIndex, char operation) const;
    bool freeInode(int inodeIndex);
    bool removeEntryFromFolder(int folderInode, const std::string& name);
    bool matchPattern(const std::string& value, const std::string& pattern) const;
    bool writeJournal(const std::string& operation, const std::string& path, const std::string& content);
    bool readJournal(std::vector<Journal>& journals);
    using Ext2Reader::readBlockFolder;
    using Ext2Reader::readBlockFile;
};

#endif