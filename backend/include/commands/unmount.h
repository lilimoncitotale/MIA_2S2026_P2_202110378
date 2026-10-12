#ifndef UNMOUNT_H
#define UNMOUNT_H

#include <string>

class Unmount {
private:
    std::string lastError;
    
public:
    Unmount();
    bool desmontar(const std::string& id);
    std::string getLastError() const { return lastError; }
};

#endif