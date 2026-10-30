#include "config/Config.hpp" //R

#include <cerrno> //R
#include <cstdlib> //R
#include <cstring> //R
#include <fstream> //R
#include <iostream> //R

namespace //R
{ //R
    std::string Trim(const std::string &s) //R
    { //R
        size_t start = s.find_first_not_of(" \t\r"); //R
        if(start == std::string::npos) //R
            return ""; //R
        size_t end = s.find_last_not_of(" \t\r"); //R
        return s.substr(start, end - start + 1); //R
    } //R

    // strtol cu verificare: tot sirul trebuie sa fie numar si sa fie in [min, max] //R
    bool ParseNumber(const std::string &text, long min, long max, long &out) //R
    { //R
        if(text.empty()) //R
            return false; //R
        errno = 0; //R
        char *end = nullptr; //R
        long value = std::strtol(text.c_str(), &end, 10); //R
        if(errno != 0 || *end != '\0' || value < min || value > max) //R
            return false; //R
        out = value; //R
        return true; //R
    } //R

    bool ParsePort(const std::string &text, uint16_t &port) //R
    { //R
        long value; //R
        if(!ParseNumber(text, 1, 65535, value)) //R
            return false; //R
        port = static_cast<uint16_t>(value); //R
        return true; //R
    } //R

    bool ParseWorkers(const std::string &text, int &workers) //R
    { //R
        long value; //R
        if(!ParseNumber(text, limits::kMinWorkers, limits::kMaxWorkers, value)) //R
            return false; //R
        workers = static_cast<int>(value); //R
        return true; //R
    } //R

    bool ParseYesNo(const std::string &text, bool &out) //R
    { //R
        if(text == "yes" || text == "1" || text == "true") //R
            out = true; //R
        else if(text == "no" || text == "0" || text == "false") //R
            out = false; //R
        else //R
            return false; //R
        return true; //R
    } //R

    // "host:port" pentru --upstream //R
    bool ParseHostPort(const std::string &text, std::string &host, uint16_t &port) //R
    { //R
        size_t colon = text.rfind(':'); //R
        if(colon == std::string::npos || colon == 0) //R
            return false; //R
        host = text.substr(0, colon); //R
        return ParsePort(text.substr(colon + 1), port); //R
    } //R

    // o linie "cheie=valoare" din proxy.conf //R
    bool ApplyKey(Config &cfg, const std::string &key, const std::string &value) //R
    { //R
        if(key == "listen_addr") //R
        { //R
            cfg.listen_addr = value; //R
            return !value.empty(); //R
        } //R
        if(key == "listen_port") //R
            return ParsePort(value, cfg.listen_port); //R
        if(key == "workers") //R
            return ParseWorkers(value, cfg.workers); //R
        if(key == "upstream_enabled") //R
            return ParseYesNo(value, cfg.upstream_enabled); //R
        if(key == "upstream_host") //R
        { //R
            cfg.upstream_host = value; //R
            return !value.empty(); //R
        } //R
        if(key == "upstream_port") //R
            return ParsePort(value, cfg.upstream_port); //R
        if(key == "log_file") //R
        { //R
            cfg.log_file = value; //R
            return !value.empty(); //R
        } //R
        // cheie necunoscuta: avertisment, nu eroare //R
        std::cerr << "warning: proxy.conf: cheie necunoscuta '" << key << "', ignorata\n"; //R
        return true; //R
    } //R

    // CF1.1: daca fisierul lipseste pornim cu valorile implicite si afisam un avertisment //R
    bool LoadFile(const std::string &path, Config &cfg) //R
    { //R
        std::ifstream file(path); //R
        if(!file) //R
        { //R
            std::cerr << "warning: nu pot deschide " << path << " (" << std::strerror(errno) //R
                      << "), folosesc valorile implicite\n"; //R
            return true; //R
        } //R

        std::string line; //R
        int line_no = 0; //R
        while(std::getline(file, line)) //R
        { //R
            line_no++; //R
            // tot ce e dupa '#' e comentariu //R
            size_t hash = line.find('#'); //R
            if(hash != std::string::npos) //R
                line.erase(hash); //R
            line = Trim(line); //R
            if(line.empty()) //R
                continue; //R

            size_t eq = line.find('='); //R
            if(eq == std::string::npos) //R
            { //R
                std::cerr << "error: " << path << ":" << line_no << ": lipseste '='\n"; //R
                return false; //R
            } //R
            std::string key = Trim(line.substr(0, eq)); //R
            std::string value = Trim(line.substr(eq + 1)); //R
            if(!ApplyKey(cfg, key, value)) //R
            { //R
                std::cerr << "error: " << path << ":" << line_no << ": valoare invalida pentru '" //R
                          << key << "': '" << value << "'\n"; //R
                return false; //R
            } //R
        } //R
        return true; //R
    } //R
} //R

ConfigResult LoadConfig(int argc, char *argv[], Config &cfg) //R
{ //R
    // primul argument care nu incepe cu "--" e calea catre proxy.conf (CF1.1) //R
    std::string conf_path = "proxy.conf"; //R
    int first_opt = 1; //R
    if(argc > 1 && std::strncmp(argv[1], "--", 2) != 0) //R
    { //R
        conf_path = argv[1]; //R
        first_opt = 2; //R
    } //R

    // --help nu are nevoie de fisier //R
    for(int i = 1; i < argc; i++) //R
        if(std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) //R
        { //R
            PrintUsage(argv[0]); //R
            return ConfigResult::Help; //R
        } //R

    if(!LoadFile(conf_path, cfg)) //R
        return ConfigResult::Error; //R

    // CF1.2: linia de comanda are prioritate fata de fisier //R
    for(int i = first_opt; i < argc; i++) //R
    { //R
        std::string opt = argv[i]; //R
        bool has_value = i + 1 < argc; //R

        if(opt == "--port" && has_value) //R
        { //R
            if(!ParsePort(argv[++i], cfg.listen_port)) //R
            { //R
                std::cerr << "error: --port: port invalid '" << argv[i] << "'\n"; //R
                return ConfigResult::Error; //R
            } //R
        } //R
        else if(opt == "--workers" && has_value) //R
        { //R
            if(!ParseWorkers(argv[++i], cfg.workers)) //R
            { //R
                std::cerr << "error: --workers: trebuie intre " << limits::kMinWorkers << " si " //R
                          << limits::kMaxWorkers << "\n"; //R
                return ConfigResult::Error; //R
            } //R
        } //R
        else if(opt == "--upstream" && has_value) //R
        { //R
            if(!ParseHostPort(argv[++i], cfg.upstream_host, cfg.upstream_port)) //R
            { //R
                std::cerr << "error: --upstream: format asteptat host:port\n"; //R
                return ConfigResult::Error; //R
            } //R
            cfg.upstream_enabled = true; //R
        } //R
        // CF10.1: upstream-ul se poate opri si din linia de comanda //R
        else if(opt == "--no-upstream") //R
            cfg.upstream_enabled = false; //R
        else //R
        { //R
            std::cerr << "error: optiune necunoscuta sau fara valoare: " << opt << "\n"; //R
            PrintUsage(argv[0]); //R
            return ConfigResult::Error; //R
        } //R
    } //R
    return ConfigResult::Ok; //R
} //R

void PrintUsage(const char *prog) //R
{ //R
    Config def; //R
    std::cout << "Usage: " << prog << " [proxy.conf] [optiuni]\n" //R
              << "  --port N              portul de ascultare (implicit " << def.listen_port << ")\n" //R
              << "  --workers N           numarul de thread-uri worker, " << limits::kMinWorkers << "-" //R
              << limits::kMaxWorkers << " (implicit " << def.workers << ")\n" //R
              << "  --upstream host:port  trimite tot traficul prin alt proxy HTTP\n" //R
              << "  --no-upstream         dezactiveaza upstream-ul din proxy.conf\n" //R
              << "  --help                afiseaza acest mesaj\n" //R
              << "Fara fisier se folosesc: listen_addr=" << def.listen_addr //R
              << ", log_file=" << def.log_file << ", upstream dezactivat\n"; //R
} //R

void PrintConfig(const Config &cfg) //R
{ //R
    std::cout << "Proxy pornit pe " << cfg.listen_addr << ":" << cfg.listen_port //R
              << ", workers=" << cfg.workers << ", upstream="; //R
    if(cfg.upstream_enabled) //R
        std::cout << "activ (" << cfg.upstream_host << ":" << cfg.upstream_port << ")"; //R
    else //R
        std::cout << "dezactivat"; //R
    std::cout << ", log=" << cfg.log_file << std::endl; //R

    // CNF4.1: avertisment daca nu ascultam doar local //R
    if(cfg.listen_addr != "127.0.0.1") //R
        std::cerr << "warning: proxy-ul asculta pe " << cfg.listen_addr //R
                  << " si poate fi folosit din afara masinii\n"; //R
} //R
