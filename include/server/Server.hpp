#ifndef SERVER_HPP //R
#define SERVER_HPP //R

#include "config/Config.hpp" //R
#include "log/Logger.hpp" //R
#include "pool/ThreadPool.hpp" //R

// socketul de ascultare si bucla de accept() (componenta server din DCS 2.1) //R
class Server //R
{ //R
private: //R
    const Config &_cfg; //R
    ThreadPool &_pool; //R
    Logger &_log; //R
    int _listen_fd = -1; //R

public: //R
    Server(const Config &cfg, ThreadPool &pool, Logger &log) : _cfg(cfg), _pool(pool), _log(log) {} //R
    ~Server() { Close(); } //R
    Server(const Server &) = delete; //R
    Server &operator=(const Server &) = delete; //R

    // CF1.3: socket(), SO_REUSEADDR, bind(), listen(); false + mesaj daca ceva esueaza //R
    bool Start(); //R

    // CF2.1: accept() in bucla pana la SIGINT/SIGTERM (CF13.1) //R
    void Run(); //R

    void Close(); //R

    // SIGINT/SIGTERM opresc bucla de accept, SIGPIPE e ignorat (DCS 2.2) //R
    static bool InstallSignalHandlers(); //R
}; //R

#endif //R
