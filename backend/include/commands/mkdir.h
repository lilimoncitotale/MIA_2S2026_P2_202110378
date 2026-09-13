#ifndef MKDIR_H
#define MKDIR_H

#include <string>
#include <map>

class MKDIR {
private:
    std::string lastError;
    
    bool parsePath(const std::string& fullPath, std::string& parentPath, std::string& folderName);
    
public:
    MKDIR();
    
    bool crearCarpeta(const std::map<std::string, std::string>& params);
    std::string getLastError() const { return lastError; }
};

#endif