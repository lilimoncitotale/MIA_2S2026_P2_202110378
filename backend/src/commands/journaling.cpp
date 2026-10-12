#include "../../include/commands/journaling.h"
#include "../../include/disk/mounted.h"
#include "../../include/ext2/ext2_writer.h"
#include <iostream>
#include <iomanip>
#include <ctime>

Journaling::Journaling() {}

bool Journaling::mostrar(const std::string& id) {
    MountedPartition* mounted = g_mountedManager.findById(id);
    if (!mounted) {
        lastError = "ID no encontrado: " + id;
        return false;
    }
    
    Ext2Writer writer;
    if (!writer.openForWrite(mounted->diskPath, mounted->partition.part_start)) {
        lastError = "No se pudo abrir la partición";
        return false;
    }
    
    std::vector<Journal> journals;
    if (!writer.readJournal(journals)) {
        writer.close();
        lastError = "La partición no es EXT3 (no tiene journaling)";
        std::cout << "Error: " << lastError << std::endl;
        return false;
    }
    
    writer.close();
    
    std::cout << "=========================================" << std::endl;
    std::cout << "   JOURNALING - Partición " << id << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << std::left 
              << std::setw(15) << "Operación"
              << std::setw(25) << "Path"
              << std::setw(30) << "Contenido"
              << "Fecha" << std::endl;
    std::cout << "-----------------------------------------" << std::endl;
    
    for (const auto& j : journals) {
        time_t t = (time_t)j.j_content.i_date;
        char dateStr[32];
        strftime(dateStr, sizeof(dateStr), "%Y-%m-%d %H:%M", localtime(&t));
        
        std::cout << std::left 
                  << std::setw(15) << j.j_content.i_operation
                  << std::setw(25) << j.j_content.i_path
                  << std::setw(30) << j.j_content.i_content
                  << dateStr << std::endl;
    }
    
    std::cout << "=========================================" << std::endl;
    std::cout << "Total: " << journals.size() << " operaciones" << std::endl;
    
    return true;
}