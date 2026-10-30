#ifndef CONFIG_HPP //R
#define CONFIG_HPP //R

#include "config/Limits.hpp" //R
#include <cstdint> //R
#include <string> //R

// setarile care se pot schimba fara recompilare (CNF5.3) //R
// valorile implicite sunt cele din DCS 2.3 //R
struct Config //R
{ //R
    std::string listen_addr = "127.0.0.1"; //R
    uint16_t listen_port = 8080; //R
    int workers = 8; //R
    bool upstream_enabled = false; //R
    std::string upstream_host = "127.0.0.1"; //R
    uint16_t upstream_port = 8081; //R
    std::string log_file = "proxy.log"; //R
}; //R

enum class ConfigResult //R
{ //R
    Ok,     // pornim proxy-ul //R
    Help,   // s-a cerut --help, iesim cu 0 //R
    Error   // configuratie gresita, iesim cu cod != 0 //R
}; //R

// CF1.1 + CF1.2: citeste proxy.conf, apoi aplica argumentele din linia de comanda //R
ConfigResult LoadConfig(int argc, char *argv[], Config &cfg); //R

// CNF7.3: lista optiunilor si valorile implicite //R
void PrintUsage(const char *prog); //R

// CF1.4: adresa, portul, workerii si starea upstream-ului //R
void PrintConfig(const Config &cfg); //R

#endif //R
