#include "../../include/disk/mounted.h"
#include <iostream>
#include <algorithm>

//inicializar la variable global
MountedManager g_mountedManager("78"); //inicializamos con los ultimos dos digitos del carnet


//constructor

MountedManager::MountedManager(const std::string& carnet)
    : carnetDigits(carnet),
    lastCorrelativeByDisk(0),
    currentLetter('A'),
    lastDiskPath("") {}


    //generarID

std::string MountedManager::generateId(const std::string& diskPath){
    //si es el primer disco o cambia de disco
    if(lastDiskPath.empty() || lastDiskPath != diskPath){
        //si es el primer disco empexamos con el primer correlativo y la primera letra
        if (lastDiskPath.empty()){
            lastCorrelativeByDisk = 1;

        }else{
            //cambiamos de disco, incrementamos de letra y reiniciamos el correlativo
            currentLetter++;
            lastCorrelativeByDisk = 1;
        }
        lastDiskPath = diskPath;
    }else{
        //mismo disco, incrementamos el correlativo
        lastCorrelativeByDisk++;
    }

    //formamos el ID:: [carnet][numero][Letra]
    std::string id = carnetDigits;
    id += std::to_string(lastCorrelativeByDisk);
    id += currentLetter;
    return id;

}

//agregar la particion montada
bool MountedManager::addMounted(const std::string& diskPath, 
                                const std::string& partitionName, 
                                const Partition& partition,
                                const std::string& id) {  // ← NUEVO parámetro
    // Verificar si ya está montada
    if (isMounted(diskPath, partitionName)) {
        std::cout << "❌ Error: La partición ya está montada" << std::endl;
        return false;
    }
    
    // ✅ YA NO GENERAR ID, USAR EL QUE LLEGA
    // std::string id = generateId(diskPath);  ← ELIMINAR ESTA LÍNEA
    
    // Crear estructura
    MountedPartition mounted;
    mounted.diskPath = diskPath;
    mounted.partitionName = partitionName;
    mounted.id = id;  // ← Usar el ID recibido
    mounted.correlative = lastCorrelativeByDisk;
    mounted.partition = partition;
    
    mountedPartitions.push_back(mounted);
    
    std::cout << "✅ Partición agregada a RAM con ID: " << id << std::endl;
    return true;
}


//buscar por ID
MountedPartition* MountedManager::findById(const std::string& id){
    for (auto& mounted : mountedPartitions){
        if (mounted.id == id){
            return &mounted;
        }
    }
    return nullptr;
}

//eliminar por ID
bool MountedManager::removeById(const std::string& id) {
    mountedPartitions.erase(
        std::remove_if(mountedPartitions.begin(), mountedPartitions.end(),
            [&id](const MountedPartition& mp) { return mp.id == id; }),
        mountedPartitions.end()
    );
    return true;
}

MountedPartition* MountedManager::findByDiskAndName(const std::string& diskPath,
                                                    const std::string& partitionName){
    for (auto& mounted : mountedPartitions){
        if(mounted.diskPath == diskPath && mounted.partitionName == partitionName){
            return &mounted;
        }
    }
    return nullptr;
}

//verificar si esta montada
bool MountedManager::isMounted(const std::string& diskPath, const std::string& partitionName){
    return findByDiskAndName(diskPath, partitionName) != nullptr;
}

//listar todas

void MountedManager::listMounted() const{
    std::cout << "=========================================" << std::endl;
    std::cout << "   PARTICIONES MONTADAS" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    if (mountedPartitions.empty()) {
        std::cout << "No hay particiones montadas." << std::endl;
        return;
    }
    
    for (const auto& mp : mountedPartitions) {
        std::cout << "ID: " << mp.id << std::endl;
        std::cout << "  Disco: " << mp.diskPath << std::endl;
        std::cout << "  Partición: " << mp.partitionName << std::endl;
        std::cout << "  Correlativo: " << mp.correlative << std::endl;
        std::cout << "  Tipo: " << mp.partition.part_type << std::endl;
        std::cout << "  Tamaño: " << mp.partition.part_size << " bytes" << std::endl;
        std::cout << "-----------------------------------------" << std::endl;
    }
}

//limpiar
void MountedManager::clear(){
    mountedPartitions.clear();
    lastCorrelativeByDisk = 0;
    currentLetter = 'A';
    lastDiskPath = "";
}