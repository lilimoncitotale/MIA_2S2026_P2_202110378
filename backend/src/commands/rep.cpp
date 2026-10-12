#include "../../include/commands/rep.h"
#include "../../include/disk/mounted.h"
#include "../../include/ext2/ext2_reader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <ctime>

namespace fs = std::filesystem;

// ============================================
// CONSTRUCTOR
// ============================================
REP::REP() {}

// ============================================
// HELPER: Sanitizar texto para Graphviz
// (DEFINIDO AL INICIO para que esté disponible)
// ============================================
std::string REP::sanitizeForDot(const std::string& input, size_t maxLen) {
    std::string output;
    for (size_t i = 0; i < input.size() && i < maxLen; i++) {
        unsigned char c = input[i];
        if (c < 32 || c > 126) {
            if (c == '\n' || c == '\r' || c == '\t') output += ' ';
            continue;
        }
        if (c == '"') output += "\\\"";
        else if (c == '\\') output += "\\\\";
        else if (c == '<') output += "&lt;";
        else if (c == '>') output += "&gt;";
        else if (c == '&') output += "&amp;";
        else output += c;
    }
    return output;
}

// ============================================
// HELPERS
// ============================================
bool REP::splitPath(const std::string& fullPath, std::string& folder, std::string& fileName) {
    size_t lastSlash = fullPath.find_last_of('/');
    if (lastSlash == std::string::npos) {
        folder = "."; fileName = fullPath;
    } else {
        folder = fullPath.substr(0, lastSlash);
        fileName = fullPath.substr(lastSlash + 1);
    }
    return !fileName.empty();
}

bool REP::ensureFolder(const std::string& folder) {
    try {
        if (!fs::exists(folder)) fs::create_directories(folder);
        return true;
    } catch (...) { return false; }
}

bool REP::writeFile(const std::string& path, const std::string& content) {
    std::string folder, fileName;
    if (!splitPath(path, folder, fileName)) { lastError = "Ruta inválida"; return false; }
    if (!ensureFolder(folder)) { lastError = "No se pudo crear carpeta"; return false; }
    std::ofstream file(path);
    if (!file.is_open()) { lastError = "No se pudo crear archivo"; return false; }
    file << content;
    return true;
}

bool REP::runGraphviz(const std::string& dotContent, const std::string& outputPath) {
    std::string dotPath = "/tmp/rep_temp.dot";
    std::ofstream dotFile(dotPath);
    if (!dotFile.is_open()) { lastError = "No se pudo crear .dot temporal"; return false; }
    dotFile << dotContent;
    dotFile.close();
    
    std::string ext = "jpg";
    size_t dotPos = outputPath.find_last_of('.');
    if (dotPos != std::string::npos) ext = outputPath.substr(dotPos + 1);
    
    std::string folder, fileName;
    splitPath(outputPath, folder, fileName);
    ensureFolder(folder);
    
    std::string cmd = "dot -T" + ext + " " + dotPath + " -o \"" + outputPath + "\"";
    std::cout << "🛠️ Ejecutando: " << cmd << std::endl;
    
    int result = system(cmd.c_str());
    if (result != 0) { lastError = "Error Graphviz"; return false; }
    return true;
}

bool REP::getMountedInfo(const std::string& id, std::string& diskPath, 
                          int& partitionStart, Partition& partition) {
    MountedPartition* mounted = g_mountedManager.findById(id);
    if (!mounted) { lastError = "ID no encontrado: " + id; return false; }
    diskPath = mounted->diskPath;
    partition = mounted->partition;
    partitionStart = partition.part_start;
    return true;
}

// ============================================
// REPORTE: SB
// ============================================
bool REP::reportSB(const std::string& id, const std::string& path) {
    std::string diskPath; int partitionStart; Partition partition;
    if (!getMountedInfo(id, diskPath, partitionStart, partition)) return false;
    
    std::ifstream file(diskPath, std::ios::binary);
    if (!file.is_open()) { lastError = "No se pudo abrir disco"; return false; }
    
    file.seekg(partitionStart, std::ios::beg);
    SuperBlock sb;
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBlock));
    file.close();
    
    char mtimeStr[64], umtimeStr[64];
    struct tm* tmInfo = localtime(&sb.s_mtime);
    strftime(mtimeStr, sizeof(mtimeStr), "%Y-%m-%d %H:%M", tmInfo);
    tmInfo = localtime(&sb.s_umtime);
    strftime(umtimeStr, sizeof(umtimeStr), "%Y-%m-%d %H:%M", tmInfo);
    
    std::stringstream dot;
    dot << "digraph SB {\n  node [shape=plaintext];\n  sb [label=<\n";
    dot << "    <table border='1' cellborder='1' cellspacing='0'>\n";
    dot << "      <tr><td colspan='2' bgcolor='lightblue'><b>SUPERBLOQUE</b></td></tr>\n";
    dot << "      <tr><td>s_filesystem_type</td><td>" << sb.s_filesystem_type << "</td></tr>\n";
    dot << "      <tr><td>s_inodes_count</td><td>" << sb.s_inodes_count << "</td></tr>\n";
    dot << "      <tr><td>s_blocks_count</td><td>" << sb.s_blocks_count << "</td></tr>\n";
    dot << "      <tr><td>s_free_blocks_count</td><td>" << sb.s_free_blocks_count << "</td></tr>\n";
    dot << "      <tr><td>s_free_inodes_count</td><td>" << sb.s_free_inodes_count << "</td></tr>\n";
    dot << "      <tr><td>s_mtime</td><td>" << mtimeStr << "</td></tr>\n";
    dot << "      <tr><td>s_umtime</td><td>" << umtimeStr << "</td></tr>\n";
    dot << "      <tr><td>s_mnt_count</td><td>" << sb.s_mnt_count << "</td></tr>\n";
    dot << "      <tr><td>s_magic</td><td>0x" << std::hex << sb.s_magic << std::dec << "</td></tr>\n";
    dot << "      <tr><td>s_inode_size</td><td>" << sb.s_inode_size << "</td></tr>\n";
    dot << "      <tr><td>s_block_size</td><td>" << sb.s_block_size << "</td></tr>\n";
    dot << "      <tr><td>s_first_ino</td><td>" << sb.s_first_ino << "</td></tr>\n";
    dot << "      <tr><td>s_first_blo</td><td>" << sb.s_first_blo << "</td></tr>\n";
    dot << "      <tr><td>s_bm_inode_start</td><td>" << sb.s_bm_inode_start << "</td></tr>\n";
    dot << "      <tr><td>s_bm_block_start</td><td>" << sb.s_bm_block_start << "</td></tr>\n";
    dot << "      <tr><td>s_inode_start</td><td>" << sb.s_inode_start << "</td></tr>\n";
    dot << "      <tr><td>s_block_start</td><td>" << sb.s_block_start << "</td></tr>\n";
    dot << "    </table>>];\n}\n";
    
    return runGraphviz(dot.str(), path);
}

// ============================================
// REPORTE: MBR
// ============================================
bool REP::reportMBR(const std::string& id, const std::string& path) {
    std::string diskPath; int partitionStart; Partition partition;
    if (!getMountedInfo(id, diskPath, partitionStart, partition)) return false;
    
    std::ifstream file(diskPath, std::ios::binary);
    if (!file.is_open()) { lastError = "No se pudo abrir disco"; return false; }
    
    MBR mbr;
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
    file.close();
    
    char dateStr[64];
    struct tm* tmInfo = localtime(&mbr.mbr_date);
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d %H:%M", tmInfo);
    
    std::stringstream dot;
    dot << "digraph MBR {\n  node [shape=plaintext];\n  mbr [label=<\n";
    dot << "    <table border='1' cellborder='1' cellspacing='0'>\n";
    dot << "      <tr><td colspan='2' bgcolor='lightblue'><b>MBR</b></td></tr>\n";
    dot << "      <tr><td>mbr_size</td><td>" << mbr.mbr_size << "</td></tr>\n";
    dot << "      <tr><td>mbr_creation_date</td><td>" << dateStr << "</td></tr>\n";
    dot << "      <tr><td>mbr_dsk_signature</td><td>" << mbr.mbr_disk_signature << "</td></tr>\n";
    dot << "      <tr><td>dsk_fit</td><td>" << mbr.fit << "</td></tr>\n";
    dot << "    </table>>];\n";
    
    for (int i = 0; i < 4; i++) {
        const Partition& p = mbr.mbr_partitions[i];
        std::string partName(p.part_name);
        partName = partName.c_str();
        
        dot << "  p" << i << " [label=<\n";
        dot << "    <table border='1' cellborder='1' cellspacing='0'>\n";
        dot << "      <tr><td colspan='2' bgcolor='lightyellow'><b>Partición " << i << "</b></td></tr>\n";
        dot << "      <tr><td>part_status</td><td>" << p.part_status << "</td></tr>\n";
        dot << "      <tr><td>part_type</td><td>" << p.part_type << "</td></tr>\n";
        dot << "      <tr><td>part_fit</td><td>" << p.part_fit << "</td></tr>\n";
        dot << "      <tr><td>part_start</td><td>" << p.part_start << "</td></tr>\n";
        dot << "      <tr><td>part_size</td><td>" << p.part_size << "</td></tr>\n";
        dot << "      <tr><td>part_name</td><td>" << partName << "</td></tr>\n";
        dot << "      <tr><td>part_correlative</td><td>" << p.part_correlative << "</td></tr>\n";
        dot << "      <tr><td>part_id</td><td>" << p.part_id << "</td></tr>\n";
        dot << "    </table>>];\n";
    }
    
    dot << "  mbr -> p0;\n  mbr -> p1;\n  mbr -> p2;\n  mbr -> p3;\n}\n";
    return runGraphviz(dot.str(), path);
}

// ============================================
// REPORTE: DISK
// ============================================
bool REP::reportDisk(const std::string& id, const std::string& path) {
    std::string diskPath; int partitionStart; Partition partition;
    if (!getMountedInfo(id, diskPath, partitionStart, partition)) return false;
    
    std::ifstream file(diskPath, std::ios::binary);
    if (!file.is_open()) { lastError = "No se pudo abrir disco"; return false; }
    
    MBR mbr;
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
    file.close();
    
    int totalSize = mbr.mbr_size;
    std::vector<Partition> partes;
    for (int i = 0; i < 4; i++) {
        if (mbr.mbr_partitions[i].part_size > 0) partes.push_back(mbr.mbr_partitions[i]);
    }
    std::sort(partes.begin(), partes.end(),
        [](const Partition& a, const Partition& b) { return a.part_start < b.part_start; });
    
    std::stringstream dot;
    dot << "digraph DISK {\n  node [shape=plaintext];\n  disk [label=<\n";
    dot << "    <table border='1' cellborder='1' cellspacing='0' cellpadding='4'>\n";
    dot << "      <tr><td colspan='2' bgcolor='lightblue'><b>DISCO</b></td></tr>\n";
    dot << "      <tr><td>Tamaño total</td><td>" << totalSize << " bytes</td></tr>\n";
    dot << "      <tr><td>MBR</td><td>" << sizeof(MBR) << " bytes</td></tr>\n";
    
    int currentPos = sizeof(MBR);
    int libreTotal = 0;
    for (const auto& p : partes) {
        if (p.part_start > currentPos) {
            int libre = p.part_start - currentPos;
            float pct = (float)libre / totalSize * 100;
            libreTotal += libre;
            dot << "      <tr><td>Espacio libre</td><td>" << libre << " bytes (" << pct << "%)</td></tr>\n";
        }
        std::string pName(p.part_name);
        pName = pName.c_str();
        float pct = (float)p.part_size / totalSize * 100;
        dot << "      <tr><td>" << pName << " (" << p.part_type << ")</td><td>" 
            << p.part_size << " bytes (" << pct << "%)</td></tr>\n";
        currentPos = p.part_start + p.part_size;
    }
    if (totalSize > currentPos) {
        int libre = totalSize - currentPos;
        float pct = (float)libre / totalSize * 100;
        libreTotal += libre;
        dot << "      <tr><td>Espacio libre</td><td>" << libre << " bytes (" << pct << "%)</td></tr>\n";
    }
    float pctLibre = (float)libreTotal / totalSize * 100;
    dot << "      <tr><td><b>Total libre</b></td><td><b>" << libreTotal 
        << " bytes (" << pctLibre << "%)</b></td></tr>\n";
    dot << "    </table>>];\n}\n";
    
    return runGraphviz(dot.str(), path);
}

// ============================================
// REPORTE: BM_INODE
// ============================================
bool REP::reportBMInode(const std::string& id, const std::string& path) {
    std::string diskPath; int partitionStart; Partition partition;
    if (!getMountedInfo(id, diskPath, partitionStart, partition)) return false;
    
    std::ifstream file(diskPath, std::ios::binary);
    if (!file.is_open()) { lastError = "No se pudo abrir disco"; return false; }
    
    file.seekg(partitionStart, std::ios::beg);
    SuperBlock sb;
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBlock));
    
    int bmSize = (sb.s_inodes_count + 7) / 8;
    std::vector<char> bm(bmSize);
    file.seekg(partitionStart + sb.s_bm_inode_start, std::ios::beg);
    file.read(bm.data(), bmSize);
    file.close();
    
    std::stringstream txt;
    txt << "BITMAP DE INODOS\nTotal inodos: " << sb.s_inodes_count << "\n================\n\n";
    int count = 0;
    for (int i = 0; i < sb.s_inodes_count; i++) {
        int bit = (bm[i/8] >> (i%8)) & 1;
        txt << bit; count++;
        if (count % 20 == 0) txt << "\n";
    }
    if (count % 20 != 0) txt << "\n";
    return writeFile(path, txt.str());
}

// ============================================
// REPORTE: BM_BLOCK
// ============================================
bool REP::reportBMBlock(const std::string& id, const std::string& path) {
    std::string diskPath; int partitionStart; Partition partition;
    if (!getMountedInfo(id, diskPath, partitionStart, partition)) return false;
    
    std::ifstream file(diskPath, std::ios::binary);
    if (!file.is_open()) { lastError = "No se pudo abrir disco"; return false; }
    
    file.seekg(partitionStart, std::ios::beg);
    SuperBlock sb;
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBlock));
    
    int bmSize = (sb.s_blocks_count + 7) / 8;
    std::vector<char> bm(bmSize);
    file.seekg(partitionStart + sb.s_bm_block_start, std::ios::beg);
    file.read(bm.data(), bmSize);
    file.close();
    
    std::stringstream txt;
    txt << "BITMAP DE BLOQUES\nTotal bloques: " << sb.s_blocks_count << "\n================\n\n";
    int count = 0;
    for (int i = 0; i < sb.s_blocks_count; i++) {
        int bit = (bm[i/8] >> (i%8)) & 1;
        txt << bit; count++;
        if (count % 20 == 0) txt << "\n";
    }
    if (count % 20 != 0) txt << "\n";
    return writeFile(path, txt.str());
}

// ============================================
// REPORTE: INODE
// ============================================
bool REP::reportInode(const std::string& id, const std::string& path) {
    std::string diskPath; int partitionStart; Partition partition;
    if (!getMountedInfo(id, diskPath, partitionStart, partition)) return false;
    
    std::ifstream file(diskPath, std::ios::binary);
    if (!file.is_open()) { lastError = "No se pudo abrir disco"; return false; }
    
    file.seekg(partitionStart, std::ios::beg);
    SuperBlock sb;
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBlock));
    
    int bmSize = (sb.s_inodes_count + 7) / 8;
    std::vector<char> bm(bmSize);
    file.seekg(partitionStart + sb.s_bm_inode_start, std::ios::beg);
    file.read(bm.data(), bmSize);
    
    std::stringstream dot;
    dot << "digraph INODES {\n  rankdir=LR;\n  node [shape=plaintext];\n";
    
    int nodesAdded = 0;
    for (int i = 0; i < sb.s_inodes_count; i++) {
        if (((bm[i/8] >> (i%8)) & 1) == 0) continue;
        
        Inode inode;
        file.seekg(partitionStart + sb.s_inode_start + i * sizeof(Inode), std::ios::beg);
        file.read(reinterpret_cast<char*>(&inode), sizeof(Inode));
        
        char atimeStr[32], ctimeStr[32], mtimeStr[32];
        struct tm* tmInfo;
        tmInfo = localtime(&inode.i_atime); strftime(atimeStr, sizeof(atimeStr), "%Y-%m-%d %H:%M", tmInfo);
        tmInfo = localtime(&inode.i_ctime); strftime(ctimeStr, sizeof(ctimeStr), "%Y-%m-%d %H:%M", tmInfo);
        tmInfo = localtime(&inode.i_mtime); strftime(mtimeStr, sizeof(mtimeStr), "%Y-%m-%d %H:%M", tmInfo);
        
        std::string typeStr = (inode.i_type == 0) ? "Carpeta" : "Archivo";
        std::string perms;
        perms += inode.i_perm[0]; perms += inode.i_perm[1]; perms += inode.i_perm[2];
        
        dot << "  inode" << i << " [label=<\n";
        dot << "    <table border='1' cellborder='1' cellspacing='0'>\n";
        dot << "      <tr><td colspan='2' bgcolor='lightgreen'><b>INODO " << i << "</b></td></tr>\n";
        dot << "      <tr><td>i_uid</td><td>" << inode.i_uid << "</td></tr>\n";
        dot << "      <tr><td>i_gid</td><td>" << inode.i_gid << "</td></tr>\n";
        dot << "      <tr><td>i_size</td><td>" << inode.i_size << "</td></tr>\n";
        dot << "      <tr><td>i_atime</td><td>" << atimeStr << "</td></tr>\n";
        dot << "      <tr><td>i_ctime</td><td>" << ctimeStr << "</td></tr>\n";
        dot << "      <tr><td>i_mtime</td><td>" << mtimeStr << "</td></tr>\n";
        dot << "      <tr><td>i_type</td><td>" << typeStr << "</td></tr>\n";
        dot << "      <tr><td>i_perm</td><td>" << perms << "</td></tr>\n";
        for (int b = 0; b < 15; b++) {
            if (inode.i_block[b] != -1) {
                dot << "      <tr><td>i_block[" << b << "]</td><td>" << inode.i_block[b] << "</td></tr>\n";
            }
        }
        dot << "    </table>>];\n";
        if (nodesAdded > 0) dot << "  inode" << (i-1) << " -> inode" << i << " [style=invis];\n";
        nodesAdded++;
    }
    dot << "}\n";
    file.close();
    return runGraphviz(dot.str(), path);
}

// ============================================
// REPORTE: BLOCK
// ============================================
bool REP::reportBlock(const std::string& id, const std::string& path) {
    std::string diskPath; int partitionStart; Partition partition;
    if (!getMountedInfo(id, diskPath, partitionStart, partition)) return false;
    
    std::ifstream file(diskPath, std::ios::binary);
    if (!file.is_open()) { lastError = "No se pudo abrir disco"; return false; }
    
    file.seekg(partitionStart, std::ios::beg);
    SuperBlock sb;
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBlock));
    
    int bmBlockSize = (sb.s_blocks_count + 7) / 8;
    std::vector<char> bmBlock(bmBlockSize);
    file.seekg(partitionStart + sb.s_bm_block_start, std::ios::beg);
    file.read(bmBlock.data(), bmBlockSize);
    
    std::stringstream dot;
    dot << "digraph BLOCKS {\n  rankdir=LR;\n  node [shape=plaintext];\n\n";
    
    for (int i = 0; i < sb.s_blocks_count; i++) {
        if (((bmBlock[i/8] >> (i%8)) & 1) == 0) continue;
        
        BlockFolder folderBlock;
        file.seekg(partitionStart + sb.s_block_start + i * sb.s_block_size, std::ios::beg);
        file.read(reinterpret_cast<char*>(&folderBlock), sizeof(BlockFolder));
        
        bool esCarpeta = false;
        int validEntries = 0;
        for (int j = 0; j < 4; j++) {
            std::string name(folderBlock.b_content[j].b_name, 12);
            size_t nullPos = name.find('\0');
            if (nullPos != std::string::npos) name = name.substr(0, nullPos);
            bool valid = !name.empty() && name.length() <= 12;
            for (char c : name) if (c < 32 || c > 126) { valid = false; break; }
            if (valid) validEntries++;
        }
        if (validEntries >= 2) {
            bool inodosValidos = true;
            for (int j = 0; j < 4; j++) {
                int inodeRef = folderBlock.b_content[j].b_inode;
                if (inodeRef < -1 || inodeRef >= sb.s_inodes_count) { inodosValidos = false; break; }
            }
            if (inodosValidos) esCarpeta = true;
        }
        
        if (esCarpeta) {
            dot << "  block" << i << " [label=<\n";
            dot << "    <table border='1' cellborder='1' cellspacing='0' cellpadding='2'>\n";
            dot << "      <tr><td colspan='2' bgcolor='lightyellow'><b>BLOQUE " << i << " (Carpeta)</b></td></tr>\n";
            dot << "      <tr><td><b>b_name</b></td><td><b>b_inode</b></td></tr>\n";
            for (int j = 0; j < 4; j++) {
                std::string name(folderBlock.b_content[j].b_name, 12);
                size_t nullPos = name.find('\0');
                if (nullPos != std::string::npos) name = name.substr(0, nullPos);
                name = sanitizeForDot(name, 12);
                if (!name.empty()) {
                    dot << "      <tr><td>" << name << "</td><td>" << folderBlock.b_content[j].b_inode << "</td></tr>\n";
                }
            }
            dot << "    </table>>];\n\n";
        } else {
            BlockFile fileBlock;
            file.seekg(partitionStart + sb.s_block_start + i * sb.s_block_size, std::ios::beg);
            file.read(reinterpret_cast<char*>(&fileBlock), sizeof(BlockFile));
            std::string content = sanitizeForDot(std::string(fileBlock.b_content, 64), 64);
            dot << "  block" << i << " [label=<\n";
            dot << "    <table border='1' cellborder='1' cellspacing='0' cellpadding='2'>\n";
            dot << "      <tr><td bgcolor='lightcyan'><b>BLOQUE " << i << " (Archivo)</b></td></tr>\n";
            dot << "      <tr><td>" << content << "</td></tr>\n";
            dot << "    </table>>];\n\n";
        }
        if (i > 0) dot << "  block" << (i-1) << " -> block" << i << " [style=invis];\n";
    }
    dot << "}\n";
    file.close();
    return runGraphviz(dot.str(), path);
}

// ============================================
// REPORTE: TREE
// ============================================
bool REP::reportTree(const std::string& id, const std::string& path) {
    std::string diskPath; int partitionStart; Partition partition;
    if (!getMountedInfo(id, diskPath, partitionStart, partition)) return false;
    
    std::ifstream file(diskPath, std::ios::binary);
    if (!file.is_open()) { lastError = "No se pudo abrir disco"; return false; }
    
    file.seekg(partitionStart, std::ios::beg);
    SuperBlock sb;
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBlock));
    
    int bmInodeSize = (sb.s_inodes_count + 7) / 8;
    std::vector<char> bmInode(bmInodeSize);
    file.seekg(partitionStart + sb.s_bm_inode_start, std::ios::beg);
    file.read(bmInode.data(), bmInodeSize);
    
    int bmBlockSize = (sb.s_blocks_count + 7) / 8;
    std::vector<char> bmBlock(bmBlockSize);
    file.seekg(partitionStart + sb.s_bm_block_start, std::ios::beg);
    file.read(bmBlock.data(), bmBlockSize);
    
    std::stringstream dot;
    dot << "digraph TREE {\n  rankdir=LR;\n  node [shape=plaintext];\n  graph [nodesep=0.5, ranksep=0.5];\n\n";
    
    for (int i = 0; i < sb.s_inodes_count; i++) {
        if (((bmInode[i/8] >> (i%8)) & 1) == 0) continue;
        
        Inode inode;
        file.seekg(partitionStart + sb.s_inode_start + i * sizeof(Inode), std::ios::beg);
        file.read(reinterpret_cast<char*>(&inode), sizeof(Inode));
        
        char atimeStr[32], ctimeStr[32], mtimeStr[32];
        struct tm* tmInfo;
        tmInfo = localtime(&inode.i_atime); strftime(atimeStr, sizeof(atimeStr), "%Y-%m-%d %H:%M", tmInfo);
        tmInfo = localtime(&inode.i_ctime); strftime(ctimeStr, sizeof(ctimeStr), "%Y-%m-%d %H:%M", tmInfo);
        tmInfo = localtime(&inode.i_mtime); strftime(mtimeStr, sizeof(mtimeStr), "%Y-%m-%d %H:%M", tmInfo);
        
        std::string typeStr = (inode.i_type == 0) ? "Carpeta" : "Archivo";
        std::string perms;
        perms += inode.i_perm[0]; perms += inode.i_perm[1]; perms += inode.i_perm[2];
        
        dot << "  inode" << i << " [label=<\n";
        dot << "    <table border='1' cellborder='1' cellspacing='0' cellpadding='2'>\n";
        dot << "      <tr><td colspan='2' bgcolor='lightgreen'><b>INODO " << i << "</b></td></tr>\n";
        dot << "      <tr><td>i_uid</td><td>" << inode.i_uid << "</td></tr>\n";
        dot << "      <tr><td>i_gid</td><td>" << inode.i_gid << "</td></tr>\n";
        dot << "      <tr><td>i_size</td><td>" << inode.i_size << "</td></tr>\n";
        dot << "      <tr><td>i_atime</td><td>" << atimeStr << "</td></tr>\n";
        dot << "      <tr><td>i_ctime</td><td>" << ctimeStr << "</td></tr>\n";
        dot << "      <tr><td>i_mtime</td><td>" << mtimeStr << "</td></tr>\n";
        dot << "      <tr><td>i_type</td><td>" << typeStr << "</td></tr>\n";
        dot << "      <tr><td>i_perm</td><td>" << perms << "</td></tr>\n";
        for (int b = 0; b < 15; b++) {
            if (inode.i_block[b] != -1) {
                dot << "      <tr><td>i_block[" << b << "]</td><td>" << inode.i_block[b] << "</td></tr>\n";
            }
        }
        dot << "    </table>>];\n\n";
        
        for (int b = 0; b < 15; b++) {
            if (inode.i_block[b] == -1) continue;
            dot << "  inode" << i << " -> block" << inode.i_block[b] << ";\n";
        }
    }
    
    for (int i = 0; i < sb.s_blocks_count; i++) {
        if (((bmBlock[i/8] >> (i%8)) & 1) == 0) continue;
        
        BlockFolder folderBlock;
        file.seekg(partitionStart + sb.s_block_start + i * sb.s_block_size, std::ios::beg);
        file.read(reinterpret_cast<char*>(&folderBlock), sizeof(BlockFolder));
        
        bool esCarpeta = false;
        int validEntries = 0;
        for (int j = 0; j < 4; j++) {
            std::string name(folderBlock.b_content[j].b_name, 12);
            size_t nullPos = name.find('\0');
            if (nullPos != std::string::npos) name = name.substr(0, nullPos);
            bool valid = !name.empty() && name.length() <= 12;
            for (char c : name) if (c < 32 || c > 126) { valid = false; break; }
            if (valid) validEntries++;
        }
        if (validEntries >= 2) {
            bool inodosValidos = true;
            for (int j = 0; j < 4; j++) {
                int inodeRef = folderBlock.b_content[j].b_inode;
                if (inodeRef < -1 || inodeRef >= sb.s_inodes_count) { inodosValidos = false; break; }
            }
            if (inodosValidos) esCarpeta = true;
        }
        
        if (esCarpeta) {
            dot << "  block" << i << " [label=<\n";
            dot << "    <table border='1' cellborder='1' cellspacing='0' cellpadding='2'>\n";
            dot << "      <tr><td colspan='2' bgcolor='lightyellow'><b>BLOQUE " << i << " (Carpeta)</b></td></tr>\n";
            dot << "      <tr><td><b>b_name</b></td><td><b>b_inode</b></td></tr>\n";
            for (int j = 0; j < 4; j++) {
                std::string name(folderBlock.b_content[j].b_name, 12);
                size_t nullPos = name.find('\0');
                if (nullPos != std::string::npos) name = name.substr(0, nullPos);
                name = sanitizeForDot(name, 12);
                if (!name.empty()) {
                    dot << "      <tr><td>" << name << "</td><td>" << folderBlock.b_content[j].b_inode << "</td></tr>\n";
                }
            }
            dot << "    </table>>];\n\n";
            
            for (int j = 0; j < 4; j++) {
                std::string name(folderBlock.b_content[j].b_name, 12);
                size_t nullPos = name.find('\0');
                if (nullPos != std::string::npos) name = name.substr(0, nullPos);
                if (!name.empty() && name != "." && name != "..") {
                    int childInode = folderBlock.b_content[j].b_inode;
                    if (childInode >= 0 && childInode < sb.s_inodes_count) {
                        dot << "  block" << i << " -> inode" << childInode 
                            << " [label=\"" << sanitizeForDot(name, 12) << "\"];\n";
                    }
                }
            }
        } else {
            BlockFile fileBlock;
            file.seekg(partitionStart + sb.s_block_start + i * sb.s_block_size, std::ios::beg);
            file.read(reinterpret_cast<char*>(&fileBlock), sizeof(BlockFile));
            std::string content = sanitizeForDot(std::string(fileBlock.b_content, 64), 64);
            dot << "  block" << i << " [label=<\n";
            dot << "    <table border='1' cellborder='1' cellspacing='0' cellpadding='2'>\n";
            dot << "      <tr><td bgcolor='lightcyan'><b>BLOQUE " << i << " (Archivo)</b></td></tr>\n";
            dot << "      <tr><td>" << content << "</td></tr>\n";
            dot << "    </table>>];\n\n";
        }
    }
    dot << "}\n";
    file.close();
    return runGraphviz(dot.str(), path);
}

// ============================================
// REPORTE: FILE
// ============================================
bool REP::reportFile(const std::string& id, const std::string& path, 
                     const std::string& filePath) {
    if (filePath.empty()) { lastError = "Falta -path_file_ls"; return false; }
    
    std::string diskPath; int partitionStart; Partition partition;
    if (!getMountedInfo(id, diskPath, partitionStart, partition)) return false;
    
    Ext2Reader reader;
    if (!reader.open(diskPath, partitionStart)) { lastError = "No se pudo abrir EXT2"; return false; }
    
    std::string content;
    std::string fileName = filePath;
    if (!fileName.empty() && fileName[0] == '/') fileName = fileName.substr(1);
    
    size_t lastSlash = fileName.find_last_of('/');
    if (lastSlash != std::string::npos) {
        fileName = fileName.substr(lastSlash + 1);
    }
    
    if (!reader.readFile(fileName, content)) {
        reader.close();
        lastError = "No se pudo leer archivo: " + filePath;
        return false;
    }
    reader.close();
    
    std::stringstream dot;
    dot << "digraph FILE {\n  node [shape=plaintext];\n  file [label=<\n";
    dot << "    <table border='1' cellborder='1' cellspacing='0' cellpadding='4'>\n";
    dot << "      <tr><td bgcolor='lightblue'><b>" << sanitizeForDot(filePath, 100) << "</b></td></tr>\n";
    
    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) {
        dot << "      <tr><td align='left'>" << sanitizeForDot(line, 200) << "</td></tr>\n";
    }
    dot << "    </table>>];\n}\n";
    return runGraphviz(dot.str(), path);
}

// ============================================
// REPORTE: LS
// ============================================
bool REP::reportLs(const std::string& id, const std::string& path, 
                   const std::string& folderPath) {
    std::string diskPath; int partitionStart; Partition partition;
    if (!getMountedInfo(id, diskPath, partitionStart, partition)) return false;
    
    Ext2Reader reader;
    if (!reader.open(diskPath, partitionStart)) { lastError = "No se pudo abrir EXT2"; return false; }
    
    int folderInode = 0;
    Inode folderInodeData = reader.readInodePublic(folderInode);
    
    if (folderInodeData.i_type != 0) {
        reader.close();
        lastError = "La ruta no es carpeta";
        return false;
    }
    
    struct Entry {
        std::string name, perms, date;
        int uid, gid, size;
    };
    std::vector<Entry> entries;
    
    for (int i = 0; i < 12; i++) {
        if (folderInodeData.i_block[i] == -1) break;
        
        BlockFolder block;
        int blockOffset = partitionStart + reader.getSB().s_block_start 
                         + folderInodeData.i_block[i] * reader.getSB().s_block_size;
        std::ifstream file2(diskPath, std::ios::binary);
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
            
            Inode childData = reader.readInodePublic(childInode);
            
            Entry e;
            e.name = name;
            e.uid = childData.i_uid;
            e.gid = childData.i_gid;
            e.size = childData.i_size;
            e.perms = "";
            e.perms += childData.i_perm[0]; e.perms += childData.i_perm[1]; e.perms += childData.i_perm[2];
            char dateStr[32];
            struct tm* tmInfo = localtime(&childData.i_mtime);
            strftime(dateStr, sizeof(dateStr), "%Y-%m-%d %H:%M", tmInfo);
            e.date = dateStr;
            entries.push_back(e);
        }
    }
    reader.close();
    
    std::stringstream dot;
    dot << "digraph LS {\n  node [shape=plaintext];\n  ls [label=<\n";
    dot << "    <table border='1' cellborder='1' cellspacing='0' cellpadding='4'>\n";
    dot << "      <tr><td colspan='6' bgcolor='lightblue'><b>LISTADO: " 
        << sanitizeForDot(folderPath, 50) << "</b></td></tr>\n";
    dot << "      <tr bgcolor='lightgray'>";
    dot << "<td><b>Permisos</b></td><td><b>Owner</b></td><td><b>Grupo</b></td>";
    dot << "<td><b>Size</b></td><td><b>Fecha</b></td><td><b>Name</b></td></tr>\n";
    
    for (const auto& e : entries) {
        dot << "      <tr><td>" << e.perms << "</td><td>" << e.uid << "</td><td>" << e.gid 
            << "</td><td>" << e.size << "</td><td>" << e.date 
            << "</td><td align='left'>" << sanitizeForDot(e.name, 50) << "</td></tr>\n";
    }
    dot << "    </table>>];\n}\n";
    return runGraphviz(dot.str(), path);
}

// ============================================
// GENERAR
// ============================================
bool REP::generar(const std::map<std::string, std::string>& params) {
    if (params.count("-name") == 0) { lastError = "Falta -name"; return false; }
    if (params.count("-path") == 0) { lastError = "Falta -path"; return false; }
    if (params.count("-id") == 0) { lastError = "Falta -id"; return false; }
    
    std::string name = params.at("-name");
    std::string path = params.at("-path");
    std::string id = params.at("-id");
    
    std::string nameLower = name;
    for (char& c : nameLower) c = tolower(c);
    
    std::cout << "📊 Generando reporte: " << nameLower << std::endl;
    
    if (nameLower == "mbr") return reportMBR(id, path);
    if (nameLower == "disk") return reportDisk(id, path);
    if (nameLower == "sb") return reportSB(id, path);
    if (nameLower == "bm_inode") return reportBMInode(id, path);
    if (nameLower == "bm_block") return reportBMBlock(id, path);
    if (nameLower == "inode") return reportInode(id, path);
    if (nameLower == "block") return reportBlock(id, path);
    if (nameLower == "tree") return reportTree(id, path);
    if (nameLower == "file") {
        std::string f = params.count("-path_file_ls") ? params.at("-path_file_ls") : "";
        return reportFile(id, path, f);
    }
    if (nameLower == "ls") {
        std::string f = params.count("-path_file_ls") ? params.at("-path_file_ls") : "/";
        return reportLs(id, path, f);
    }
    
    lastError = "Reporte no reconocido: " + name;
    return false;
}