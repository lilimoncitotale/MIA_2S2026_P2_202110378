#ifndef FS_COMMANDS_H
#define FS_COMMANDS_H

#include <string>
#include <vector>

// ============================================
// RENAME
// ============================================
class Rename {
public:
    bool renombrar(const std::string& path, const std::string& newName);
    std::string getLastError() const { return lastError; }
private:
    std::string lastError;
};

// ============================================
// REMOVE
// ============================================
class Remove {
public:
    bool eliminar(const std::string& path);
    std::string getLastError() const { return lastError; }
private:
    std::string lastError;
};

// ============================================
// COPY
// ============================================
class Copy {
public:
    bool copiar(const std::string& origen, const std::string& destino);
    std::string getLastError() const { return lastError; }
private:
    std::string lastError;
};

// ============================================
// MOVE
// ============================================
class Move {
public:
    bool mover(const std::string& origen, const std::string& destino);
    std::string getLastError() const { return lastError; }
private:
    std::string lastError;
};

// ============================================
// FIND
// ============================================
class Find {
public:
    bool buscar(const std::string& path, const std::string& name);
    std::string getLastError() const { return lastError; }
private:
    std::string lastError;
    std::vector<std::string> resultados;
};

// ============================================
// CHOWN
// ============================================
class Chown {
public:
    bool cambiarPropietario(const std::string& path, const std::string& usuario, bool recursivo);
    std::string getLastError() const { return lastError; }
private:
    std::string lastError;
};

#endif