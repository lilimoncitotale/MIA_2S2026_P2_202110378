#ifndef LOGIN_H
#define LOGIN_H

#include <string>
#include <vector>

struct UserInfo{
    int uid;
    int gid;
    std::string username;
    std::string password;
    bool isValid;
};

struct GroupInfo{
    int gid;
    std::string name;
    bool isValid;
};

class Login{
    private:
        std::string lastError;

        //parsear users.txt
        bool parseUsersFile(const std::vector<std::string>& lines, std::vector<UserInfo>& users, std::vector<GroupInfo>& groups);
        //buscar usuario por nombre y password
        bool findUser(const std::vector<UserInfo>& users, const std::string& username, UserInfo& foundUser);

    public:
        Login();

        //metodo principal

        bool iniciarSesion(const std::string& id, const std::string&username, const std::string& password);

        std::string getLastError() const {return lastError;}
};

#endif // LOGIN_H