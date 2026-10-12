#include "api_handler.h"
#include "../include/disk/mounted.h"
#include "../include/disk/session.h"
#include "../include/ext2/ext2_reader.h"
#include "../include/ext2/ext2_writer.h"
#include "../include/commands/login.h"
#include "../include/commands/logout.h"
#include <filesystem>
#include <fstream>
#include <ctime>

namespace fs = std::filesystem;

// ============================================
// DISCOS
// ============================================

json ApiHandler::listarDiscos() {
    json result = json::array();
    
    std::vector<std::string> searchPaths = {
        "/tmp/MIA_P1_Calificacion",
        "/tmp/MIA_P2",
        "/tmp",
        std::string(getenv("HOME")) + "/Documents/Archivos/Proyecto 1/backend",
        "."
    };
    
    for (const auto& dir : searchPaths) {
        if (!fs::exists(dir)) continue;
        
        try {
            for (const auto& entry : fs::directory_iterator(dir)) {
                if (entry.path().extension() == ".mia") {
                    json disco;
                    disco["path"] = entry.path().string();
                    disco["name"] = entry.path().filename().string();
                    disco["size"] = (long long)fs::file_size(entry.path());
                    
                    std::ifstream file(entry.path().string(), std::ios::binary);
                    if (file.is_open()) {
                        MBR mbr;
                        file.seekg(0, std::ios::beg);
                        file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
                        
                        disco["mbr_size"] = mbr.mbr_size;
                        disco["fit"] = std::string(1, mbr.fit);
                        disco["signature"] = mbr.mbr_disk_signature;
                        
                        char dateStr[32];
                        struct tm* tmInfo = localtime(&mbr.mbr_date);
                        strftime(dateStr, sizeof(dateStr), "%Y-%m-%d %H:%M", tmInfo);
                        disco["creation_date"] = dateStr;
                        
                        // Particiones
                        json partes = json::array();
                        for (int i = 0; i < 4; i++) {
                            const Partition& p = mbr.mbr_partitions[i];
                            if (p.part_size == 0) continue;
                            
                            json parte;
                            std::string pName(p.part_name);
                            pName = pName.c_str();
                            parte["name"] = pName;
                            parte["type"] = std::string(1, p.part_type);
                            parte["fit"] = std::string(1, p.part_fit);
                            parte["start"] = p.part_start;
                            parte["size"] = p.part_size;
                            parte["status"] = (p.part_status == '1') ? "Montada" : "No montada";
                            
                            if (p.part_status == '1') {
                                std::string id(p.part_id, 4);
                                parte["id"] = id.c_str();
                            }
                            
                            partes.push_back(parte);
                        }
                        disco["partitions"] = partes;
                        
                        file.close();
                    }
                    
                    result.push_back(disco);
                }
            }
        } catch (...) {
            continue;
        }
    }
    
    return result;
}

json ApiHandler::infoDisco(const std::string& path) {
    json result;
    
    if (!fs::exists(path)) {
        result["error"] = "Disco no existe: " + path;
        return result;
    }
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        result["error"] = "No se pudo abrir el disco";
        return result;
    }
    
    MBR mbr;
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
    file.close();
    
    result["path"] = path;
    result["size"] = mbr.mbr_size;
    result["fit"] = std::string(1, mbr.fit);
    result["signature"] = mbr.mbr_disk_signature;
    
    char dateStr[32];
    struct tm* tmInfo = localtime(&mbr.mbr_date);
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d %H:%M", tmInfo);
    result["creation_date"] = dateStr;
    
    json partes = json::array();
    for (int i = 0; i < 4; i++) {
        const Partition& p = mbr.mbr_partitions[i];
        if (p.part_size == 0) continue;
        
        json parte;
        std::string pName(p.part_name);
        pName = pName.c_str();
        parte["name"] = pName;
        parte["type"] = std::string(1, p.part_type);
        parte["fit"] = std::string(1, p.part_fit);
        parte["start"] = p.part_start;
        parte["size"] = p.part_size;
        parte["status"] = (p.part_status == '1') ? "Montada" : "No montada";
        
        if (p.part_status == '1') {
            std::string id(p.part_id, 4);
            parte["id"] = id.c_str();
        }
        
        partes.push_back(parte);
    }
    result["partitions"] = partes;
    
    return result;
}

json ApiHandler::infoParticion(const std::string& id) {
    json result;
    
    MountedPartition* mounted = g_mountedManager.findById(id);
    if (!mounted) {
        result["error"] = "ID no encontrado: " + id;
        return result;
    }
    
    result["id"] = mounted->id;
    result["disk_path"] = mounted->diskPath;
    result["partition_name"] = mounted->partitionName;
    result["correlative"] = mounted->correlative;
    result["type"] = std::string(1, mounted->partition.part_type);
    result["start"] = mounted->partition.part_start;
    result["size"] = mounted->partition.part_size;
    
    return result;
}

// ============================================
// SISTEMA DE ARCHIVOS
// ============================================

json ApiHandler::listarCarpeta(const std::string& id, const std::string& path) {
    json result;
    
    MountedPartition* mounted = g_mountedManager.findById(id);
    if (!mounted) {
        result["error"] = "ID no encontrado";
        return result;
    }
    
    Ext2Reader reader;
    if (!reader.open(mounted->diskPath, mounted->partition.part_start)) {
        result["error"] = "No se pudo abrir la partición (¿está formateada?)";
        return result;
    }
    
    int folderInode = 0;
    Inode folderInodeData = reader.readInode(folderInode);
    
    json files = json::array();
    
    for (int i = 0; i < 12; i++) {
        if (folderInodeData.i_block[i] == -1) break;
        
        BlockFolder block;
        int blockOffset = mounted->partition.part_start + 
                          reader.getSB().s_block_start + 
                          folderInodeData.i_block[i] * reader.getSB().s_block_size;
        
        std::ifstream file2(mounted->diskPath, std::ios::binary);
        file2.seekg(blockOffset, std::ios::beg);
        file2.read(reinterpret_cast<char*>(&block), sizeof(BlockFolder));
        file2.close();
        
        for (int j = 0; j < 4; j++) {
            std::string name(block.b_content[j].b_name, 12);
            size_t nullPos = name.find('\0');
            if (nullPos != std::string::npos) name = name.substr(0, nullPos);
            
            if (name.empty() || name == "." || name == "..") continue;
            
            int childInode = block.b_content[j].b_inode;
            if (childInode < 0) continue;
            
            Inode child = reader.readInode(childInode);
            
            json item;
            item["name"] = name;
            item["type"] = (child.i_type == 0) ? "folder" : "file";
            item["size"] = child.i_size;
            item["uid"] = child.i_uid;
            item["gid"] = child.i_gid;
            item["perm"] = std::string(child.i_perm, 3);
            
            files.push_back(item);
        }
    }
    
    reader.close();
    
    result["path"] = path;
    result["files"] = files;
    return result;
}

json ApiHandler::leerArchivo(const std::string& id, const std::string& path) {
    json result;
    
    MountedPartition* mounted = g_mountedManager.findById(id);
    if (!mounted) {
        result["error"] = "ID no encontrado";
        return result;
    }
    
    Ext2Reader reader;
    if (!reader.open(mounted->diskPath, mounted->partition.part_start)) {
        result["error"] = "No se pudo abrir la partición";
        return result;
    }
    
    std::string fileName = path;
    if (!fileName.empty() && fileName[0] == '/') {
        fileName = fileName.substr(1);
    }
    size_t lastSlash = fileName.find_last_of('/');
    if (lastSlash != std::string::npos) {
        fileName = fileName.substr(lastSlash + 1);
    }
    
    std::string content;
    if (!reader.readFile(fileName, content)) {
        reader.close();
        result["error"] = "No se pudo leer: " + path;
        return result;
    }
    
    reader.close();
    
    result["path"] = path;
    result["content"] = content;
    return result;
}

// ============================================
// SESIÓN
// ============================================

json ApiHandler::loginGrafico(const std::string& id, 
                                const std::string& user, 
                                const std::string& pass) {
    json result;
    
    Login login;
    if (login.iniciarSesion(id, user, pass)) {
        result["success"] = true;
        result["message"] = "Sesión iniciada";
        result["user"] = user;
        result["uid"] = g_activeSession.uid;
        result["gid"] = g_activeSession.gid;
        result["id"] = id;
    } else {
        result["success"] = false;
        result["error"] = login.getLastError();
    }
    
    return result;
}

json ApiHandler::logoutGrafico() {
    json result;
    
    Logout logout;
    if (logout.cerrarSesion()) {
        result["success"] = true;
        result["message"] = "Sesión cerrada";
    } else {
        result["success"] = false;
        result["error"] = logout.getLastError();
    }
    
    return result;
}

json ApiHandler::estadoSesion() {
    json result;
    result["active"] = g_activeSession.isActive;
    if (g_activeSession.isActive) {
        result["user"] = g_activeSession.username;
        result["uid"] = g_activeSession.uid;
        result["gid"] = g_activeSession.gid;
        result["id"] = g_activeSession.id;
        result["disk_path"] = g_activeSession.diskPath;
        result["partition_name"] = g_activeSession.partitionName;
    }
    return result;
}

// ============================================
// JOURNAL
// ============================================

json ApiHandler::listarJournal(const std::string& id) {
    json result;
    
    MountedPartition* mounted = g_mountedManager.findById(id);
    if (!mounted) {
        result["error"] = "ID no encontrado";
        return result;
    }
    
    Ext2Writer writer;
    if (!writer.openForWrite(mounted->diskPath, mounted->partition.part_start)) {
        result["error"] = "No se pudo abrir la partición";
        return result;
    }
    
    std::vector<Journal> journals;
    if (!writer.readJournal(journals)) {
        writer.close();
        result["error"] = "La partición no es EXT3";
        return result;
    }
    writer.close();
    
    json entries = json::array();
    for (const auto& j : journals) {
        json e;
        e["operation"] = std::string(j.j_content.i_operation);
        e["path"] = std::string(j.j_content.i_path);
        e["content"] = std::string(j.j_content.i_content);
        
        time_t t = (time_t)j.j_content.i_date;
        char dateStr[32];
        strftime(dateStr, sizeof(dateStr), "%Y-%m-%d %H:%M", localtime(&t));
        e["date"] = dateStr;
        
        entries.push_back(e);
    }
    
    result["id"] = id;
    result["entries"] = entries;
    return result;
}