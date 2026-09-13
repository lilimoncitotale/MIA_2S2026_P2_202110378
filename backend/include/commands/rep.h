#ifndef REP_H
#define REP_H

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include "../disk/mbr.h"
#include "../ext2/superblock.h"
#include "../ext2/inode.h"
#include "../ext2/block.h"

class REP {
private:
    std::string lastError;
    
    // ============================================
    // HELPERS
    // ============================================
    
    bool writeFile(const std::string& path, const std::string& content);
    bool runGraphviz(const std::string& dotContent, const std::string& outputPath);
    bool splitPath(const std::string& fullPath, std::string& folder, std::string& fileName);
    bool ensureFolder(const std::string& folder);
    bool getMountedInfo(const std::string& id, std::string& diskPath, int& partitionStart, Partition& partition);
    
    // ✅ NUEVO: Helper para sanitizar texto
    std::string sanitizeForDot(const std::string& input, size_t maxLen = 64);
    
    // ============================================
    // REPORTES
    // ============================================
    
    bool reportMBR(const std::string& id, const std::string& path);
    bool reportDisk(const std::string& id, const std::string& path);
    bool reportSB(const std::string& id, const std::string& path);
    bool reportBMInode(const std::string& id, const std::string& path);
    bool reportBMBlock(const std::string& id, const std::string& path);
    bool reportInode(const std::string& id, const std::string& path);
    bool reportBlock(const std::string& id, const std::string& path);
    bool reportTree(const std::string& id, const std::string& path);
    bool reportFile(const std::string& id, const std::string& path, const std::string& filePath);
    bool reportLs(const std::string& id, const std::string& path, const std::string& folderPath);
    
public:
    REP();
    bool generar(const std::map<std::string, std::string>& params);
    std::string getLastError() const { return lastError; }
};

#endif