#include "../../include/disk/mounted.h"
#include "../../include/commands/mount.h"
#include <iostream>
#include <fstream>
#include <cstring>

Mount::Mount(){}


//leer MBR desde el disco
bool Mount::readMBR(const std::string& path, MBR& mbr){
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()){
        lastError = "Error: No se pudo abrir el disco virtual en la ruta: " + path;
        return false;
    }

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
    file.close();

    return true;
}

bool Mount::writeMBR(const std::string& path, const MBR& mbr){
    std::ofstream file(path, std::ios::binary | std::ios::in);
    if (!file.is_open()){
        lastError = "Error: No se pudo abrir el disco virtual en la ruta: " + path;
        return false;
    }

    file.seekp(0, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&mbr), sizeof(MBR));
    file.close();

    return true;

}

//buscar particion por nombre

int Mount::findPartitionByName(const MBR& mbr, const std::string& name){
    for(int i = 0; i<4; i++){
        std::string partName(mbr.mbr_partitions[i].part_name);
        //limpiar caracteres nulos
        partName = partName.c_str();
        if (partName == name){
            return i; //retornamos el indice de la particion
        }

    }
    return -1; //no se encontró la particion
}

//verificar si se puede montar

bool Mount::canMount(char type){
    return (type == 'P' || type == 'L');
}

//metodo principal para montar la particion
bool Mount::montar(const std::string& path, const std::string& name) {
    std::cout << "🔍 Montando partición..." << std::endl;
    std::cout << "   Disco: " << path << std::endl;
    std::cout << "   Partición: " << name << std::endl;
    
    // 1. Verificar que el disco exista
    std::ifstream testFile(path, std::ios::binary);
    if (!testFile.is_open()) {
        lastError = "El disco no existe: " + path;
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    testFile.close();
    
    // 2. Leer el MBR
    MBR mbr;
    if (!readMBR(path, mbr)) {
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // ✅ DEBUG: Mostrar todas las particiones del MBR
    std::cout << "📋 Particiones en el MBR:" << std::endl;
    for (int i = 0; i < 4; i++) {
        std::string partName(mbr.mbr_partitions[i].part_name);
        partName = partName.c_str();  // Limpiar caracteres nulos
        
        std::cout << "  [" << i << "] name='" << partName 
                  << "' type='" << mbr.mbr_partitions[i].part_type
                  << "' size=" << mbr.mbr_partitions[i].part_size
                  << " start=" << mbr.mbr_partitions[i].part_start << std::endl;
    }
    
    // 3. Buscar la partición por nombre
    int index = findPartitionByName(mbr, name);
    if (index == -1) {
        lastError = "Partición no encontrada: " + name;
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    Partition& partition = mbr.mbr_partitions[index];
    std::cout << "✅ Partición encontrada en índice " << index << std::endl;
    
    // 4. Verificar que la partición tenga tamaño > 0
    if (partition.part_size <= 0) {
        lastError = "La partición está vacía (size=0)";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    
    // 5. Verificar que sea montable (P o L)
    if (!canMount(partition.part_type)) {
        lastError = "No se pueden montar particiones extendidas (E)";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    std::cout << "✅ Tipo de partición válido: " << partition.part_type << std::endl;
    
    // 6. Verificar que no esté ya montada
    if (g_mountedManager.isMounted(path, name)) {
        lastError = "La partición ya está montada";
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    std::cout << "✅ No está montada aún" << std::endl;
    
    // 7. Generar ID y guardar en el MBR
    std::string id = g_mountedManager.generateId(path);
    std::cout << "✅ ID generado: " << id << std::endl;
    
    // 8. Actualizar la partición en el MBR
    partition.part_status = '1';
    partition.part_correlative = g_mountedManager.getMounted().size() + 1;
    strncpy(partition.part_id, id.c_str(), 3);
    partition.part_id[3] = '\0';
    
    // 9. Guardar el MBR actualizado
    if (!writeMBR(path, mbr)) {
        std::cout << "❌ " << lastError << std::endl;
        return false;
    }
    std::cout << "✅ MBR actualizado" << std::endl;
    
    // 10. Guardar en memoria RAM
    // En Mount::montar()
    // ...
    if (!g_mountedManager.addMounted(path, name, partition, id)) {  // ← Pasar id
        partition.part_status = '0';
        partition.part_correlative = -1;
        memset(partition.part_id, 0, 4);
        writeMBR(path, mbr);
        return false;
    }
    
    std::cout << "✅ Partición montada exitosamente:" << std::endl;
    std::cout << "   ID: " << id << std::endl;
    
    return true;
}
