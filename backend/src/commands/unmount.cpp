#include "../../include/commands/unmount.h"
#include "../../include/disk/mounted.h"
#include <iostream>

Unmount::Unmount() {}

bool Unmount::desmontar(const std::string& id) {
    MountedPartition* mounted = g_mountedManager.findById(id);
    if (!mounted) {
        lastError = "ID no encontrado: " + id;
        return false;
    }
    
    std::string diskPath = mounted->diskPath;
    std::string partitionName = mounted->partitionName;
    
    // Quitar del mountedManager
    g_mountedManager.removeById(id);
    
    // Actualizar el MBR: marcar como no montada
    // ... (opcional pero recomendado)
    
    std::cout << " Partición desmontada: " << id << std::endl;
    return true;
}