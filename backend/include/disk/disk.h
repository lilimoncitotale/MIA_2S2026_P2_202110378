#ifndef DISK_H
#define DISK_H

#include "mbr.h"
#include <string>
#include <fstream>
#include <filesystem>
#include <vector>
#include <algorithm>

class Disk{
    private:
        MBR mbr;
        std::string path; //ruta donde se guardara el disco virtual
    public:
        Disk(int size, char fit, const std::string& unit, const std::string& path){
            mbr = MBR();
            mbr.mbr_size = size;
            mbr.fit = fit;
            this->path = path;
            //aqui se deberia guardar el disco en la ruta especificada
        }

        bool saveToFile(){
            std::filesystem::path diskPath(path);
            if (diskPath.has_parent_path()) {
                std::filesystem::create_directories(diskPath.parent_path());
            }

            std::ofstream file(path, std::ios::binary);
            if (!file.is_open())return false;

            std::vector<char> zeros(1024, 0);
            int remaining = mbr.mbr_size;
            while (remaining > 0) {
                const int chunk = std::min(remaining, static_cast<int>(zeros.size()));
                file.write(zeros.data(), chunk);
                remaining -= chunk;
            }
            if (!file.good()) return false;

            file.seekp(0, std::ios::beg);
            file.write(reinterpret_cast<const char*>(&mbr), sizeof(MBR));
            file.close();
            return true;
        }
        
};

#endif