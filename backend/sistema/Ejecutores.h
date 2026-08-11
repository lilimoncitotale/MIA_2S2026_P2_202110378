
#ifndef EJECUTORES_H
#define EJECUTORES_H

#include <string>
#include "../parser/Parser.h"
#include "SistemaArchivos.h"

// Declaración del sistema global
extern SistemaArchivos sistema;

// Funciones ejecutoras
std::string ejecutarMKDISK(const ParametrosComando& params);
std::string ejecutarRMDISK(const ParametrosComando& params);
std::string ejecutarFDISK(const ParametrosComando& params);
std::string ejecutarMOUNT(const ParametrosComando& params);
std::string ejecutarMKFS(const ParametrosComando& params);
std::string ejecutarMKUSR(const ParametrosComando& params);
std::string ejecutarRMUSR(const ParametrosComando& params);
std::string ejecutarMKFILE(const ParametrosComando& params);

#endif // EJECUTORES_H
