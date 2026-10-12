#include "../../include/disk/session.h"
#include "../../include/disk/mounted.h"
#include "../../include/commands/login.h"
#include "../../include/ext2/ext2_reader.h"
#include "../../include/ext2/ext2_writer.h"

#include <iostream>
#include <sstream>

Login::Login(){}

bool Login::parseUsersFile(const std::vector<std::string>& lines, std::vector<UserInfo>& users, std::vector<GroupInfo>& groups){

    users.clear();
    groups.clear();

    for(const std::string& line : lines){
        if (line.empty()) continue;

        //dividir la linea por comas
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string part;

        while(std::getline(ss, part, ',')){
            //eliminar espacios
            size_t start = part.find_first_not_of(" \t");
            size_t end = part.find_last_not_of(" \t");

            if(start != std::string::npos && end != std::string::npos){
                parts.push_back(part.substr(start, end - start + 1));
            }else if(start != std::string::npos){
                parts.push_back(part.substr(start));
            }


        }

        if (parts.size() < 3 ) continue; //linea invalida

        //verificar si es usuario o grupo
        if(parts[1] == "G"){
            //grupo: GID, G , NOMBRE
            GroupInfo group;
            group.gid = std::stoi(parts[0]);
            group.name = parts[2];
            group.isValid = true;
            groups.push_back(group);
        }else if(parts[1] == "U"){
            //usuario: UID, U, GRUPO, NOMBRE, PASSWORD
            if(parts.size() < 5) continue; //linea invalida
            UserInfo user;
            user.uid = std::stoi(parts[0]);
            user.gid = -1;
            for (const GroupInfo& group : groups) {
                if (group.name == parts[2] && group.isValid) {
                    user.gid = group.gid;
                    break;
                }
            }
            if (user.gid == -1) continue;
            user.username = parts[3];
            user.password = parts[4];
            user.isValid = true;
            users.push_back(user);
        }
    }

    return true;

}

bool Login::findUser(const std::vector<UserInfo>& users, const std::string& username, UserInfo& foundUser){
    for(const UserInfo& user: users){
        if(user.username == username && user.isValid){
            foundUser = user;
            return true;

        }
    }

    return false;

}

bool Login::iniciarSesion(const std::string& id, const std::string&username, const std::string& password){
   std::cout << "  Iniciando sesión..." << std::endl;
    std::cout << "   ID: " << id << std::endl;
    std::cout << "   Usuario: " << username << std::endl; 

    //verificar que no haya sesion activa
    if(g_activeSession.isActive){
        lastError = "Ya hay una sesión activa. Cierre la sesión actual antes de iniciar una nueva.";
        return false;

    }

    //buscar la particion en RAM
    MountedPartition* mounted = g_mountedManager.findById(id);
    if(!mounted){
        lastError = "No se encontró la partición con ID: " + id;
        return false;

    }

    std::string diskPath = mounted->diskPath;
    int partitionStart = mounted->partition.part_start;
    std::string partitionName = mounted->partitionName;

    std::cout << "   Disco: " << diskPath << std::endl;
    std::cout << "   Partición: " << partitionName << std::endl;

    //leer el archivo users.txt desde la particion
    Ext2Reader reader;
    if(!reader.open(diskPath, partitionStart)){
        lastError = "No se pudo abrir la partición para leer el archivo users.txt.";
        return false;

    }

    std::vector<std::string> lines;
    if(!reader.readUsersFile(lines)){
        reader.close();
        lastError = "No se pudo leer el archivo users.txt desde la partición.";
        return false;
    }

    reader.close();


    //parsear el archivo users.txt
    std::vector<UserInfo> users;
    std::vector<GroupInfo> groups;

    if(!parseUsersFile(lines, users, groups)){
        lastError = "No se pudo parsear el archivo users.txt.";
        return false;

    }

    //buscar el usuario
    UserInfo foundUser;
    if(!findUser(users, username, foundUser)){
        lastError = "Usuario no encontrado: " + username;
        return false;
    }

    //verificar la contraseña
    if(foundUser.password != password){
        lastError = "Contraseña incorrecta para el usuario: " + username;
        return false;
    }

    //iniciar sesion
    g_activeSession.isActive = true;
    g_activeSession.id = id;
    g_activeSession.diskpath = diskPath;
    g_activeSession.diskPath = diskPath;
    g_activeSession.partitionName = partitionName;
    g_activeSession.partition = mounted->partition;
    g_activeSession.username = username;
    g_activeSession.uid = foundUser.uid;
    g_activeSession.gid = foundUser.gid;

    //mostrar exito

    std::cout << "=========================================" << std::endl;
    std::cout << "  Sesión iniciada exitosamente" << std::endl;
    std::cout << "   Usuario: " << username << std::endl;
    std::cout << "   UID: " << foundUser.uid << std::endl;
    std::cout << "   GID: " << foundUser.gid << std::endl;
    std::cout << "   Partición: " << partitionName << " (" << id << ")" << std::endl;
    std::cout << "=========================================" << std::endl;

    Ext2Writer writer;
    if (writer.openForWrite(diskPath, partitionStart)) {
        writer.writeJournal("login", "/", username);
        writer.close();
    }
    
    return true;
}