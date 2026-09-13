#ifndef DISKMANAGER_H
#define DISKMANAGER_H

#include "disk.h"
#include <vector>


class DiskManager{
    private:
        std::vector<Disk> disks; //vector de discos virtuales
    public:
        void addDisk(const Disk& disk){
            disks.push_back(disk);
        }


};

extern DiskManager g_diskManager; //instancia global del DiskManager
#endif