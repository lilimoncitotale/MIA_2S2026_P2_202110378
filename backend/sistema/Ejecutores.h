#ifndef EJECUTORES_H
#define EJECUTORES_H

#include <string>
#include "../parser/Parser.h"

// SOLO DECLARACIONES (sin implementación)
std::string ejecutarMKDISK(const ParametrosComando& params);
std::string ejecutarRMDISK(const ParametrosComando& params);
std::string ejecutarFDISK(const ParametrosComando& params);
std::string ejecutarMOUNT(const ParametrosComando& params);
std::string ejecutarMOUNTED(const ParametrosComando& params);
std::string ejecutarMKFS(const ParametrosComando& params);
std::string ejecutarLOGIN(const ParametrosComando& params);
std::string ejecutarLOGOUT(const ParametrosComando& params);
std::string ejecutarMKUSR(const ParametrosComando& params);
std::string ejecutarRMUSR(const ParametrosComando& params);
std::string ejecutarMKFILE(const ParametrosComando& params);
std::string ejecutarMKGRP(const ParametrosComando& params);
std::string ejecutarRMGRP(const ParametrosComando& params);
std::string ejecutarCHGRP(const ParametrosComando& params);
std::string ejecutarMKDIR(const ParametrosComando& params);
std::string ejecutarREP(const ParametrosComando& params); 
std::string ejecutarCAT(const ParametrosComando& params);

#endif