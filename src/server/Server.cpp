#include "server/Server.hpp" //R
#include "core/HTTPResponse.hpp" //R
#include "forward/SocketIO.hpp" //R

#include <arpa/inet.h> //R
#include <cerrno> //R
#include <csignal> //R
#include <cstdio> //R
#include <cstring> //R
#include <netinet/in.h> //R
#include <sys/socket.h> //R
#include <unistd.h> //R

namespace //R
{ //R
    // intr-un handler de semnal e sigur doar sa scriem un volatile sig_atomic_t //R
    volatile sig_atomic_t g_stop = 0; //R

    void OnStopSignal(int) //R
    { //R
        g_stop = 1; //R
    } //R

    // CNF7.2: "bind 127.0.0.1:8080: Address already in use" //R
    void PrintSocketError(const char *op, const Config &cfg) //R
    { //R
        fprintf(stderr, "%s %s:%u: %s\n", op, cfg.listen_addr.c_str(), //R
                static_cast<unsigned>(cfg.listen_port), strerror(errno)); //R
    } //R
} //R

bool Server::InstallSignalHandlers() //R
{ //R
    struct sigaction sa; //R
    memset(&sa, 0, sizeof(sa)); //R
    sa.sa_handler = OnStopSignal; //R
    sigemptyset(&sa.sa_mask); //R
    // fara SA_RESTART: accept() se intoarce cu EINTR cand vine semnalul (signal(7)) //R
    sa.sa_flags = 0; //R
    if(sigaction(SIGINT, &sa, nullptr) != 0 || sigaction(SIGTERM, &sa, nullptr) != 0) //R
    { //R
        perror("sigaction"); //R
        return false; //R
    } //R

    // scrierea pe un socket inchis nu trebuie sa opreasca procesul //R
    signal(SIGPIPE, SIG_IGN); //R
    return true; //R
} //R

bool Server::Start() //R
{ //R
    // doar IPv4 (DCS 2.3) //R
    struct sockaddr_in addr; //R
    memset(&addr, 0, sizeof(addr)); //R
    addr.sin_family = AF_INET; //R
    addr.sin_port = htons(_cfg.listen_port); //R
    if(inet_pton(AF_INET, _cfg.listen_addr.c_str(), &addr.sin_addr) != 1) //R
    { //R
        fprintf(stderr, "listen_addr invalid: %s (asteptam o adresa IPv4)\n", _cfg.listen_addr.c_str()); //R
        return false; //R
    } //R

    _listen_fd = socket(AF_INET, SOCK_STREAM, 0); //R
    if(_listen_fd < 0) //R
    { //R
        PrintSocketError("socket", _cfg); //R
        return false; //R
    } //R

    // SO_REUSEADDR: putem reporni imediat pe acelasi port (CF1.3) //R
    int yes = 1; //R
    if(setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) != 0) //R
    { //R
        PrintSocketError("setsockopt SO_REUSEADDR", _cfg); //R
        Close(); //R
        return false; //R
    } //R

    if(bind(_listen_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) != 0) //R
    { //R
        PrintSocketError("bind", _cfg); //R
        Close(); //R
        return false; //R
    } //R

    if(listen(_listen_fd, SOMAXCONN) != 0) //R
    { //R
        PrintSocketError("listen", _cfg); //R
        Close(); //R
        return false; //R
    } //R

    // accept() se intoarce cu EAGAIN la fiecare secunda, ca sa verificam g_stop //R
    // altfel un semnal venit chiar inainte de accept() ar fi observat abia la urmatorul client //R
    SetSocketTimeouts(_listen_fd, 1); //R
    return true; //R
} //R

void Server::Run() //R
{ //R
    while(!g_stop) //R
    { //R
        struct sockaddr_in client_addr; //R
        socklen_t len = sizeof(client_addr); //R
        int client_fd = accept(_listen_fd, reinterpret_cast<struct sockaddr *>(&client_addr), &len); //R
        if(client_fd < 0) //R
        { //R
            // semnal sau timeout pe socketul de ascultare: verificam g_stop si continuam //R
            if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) //R
                continue; //R
            // CF11.1: eroarea se logheaza, proxy-ul nu se opreste //R
            _log.LogError(std::string("accept: ") + strerror(errno)); //R
            continue; //R
        } //R

        // CF2.1: thread-ul principal doar pune fd-ul in coada //R
        // CF2.3: coada plina => 503 si inchidem imediat //R
        if(!_pool.TrySubmit(client_fd)) //R
        { //R
            SetSocketTimeouts(client_fd, 1); //R
            SendAll(client_fd, BuildErrorResponse(503, "coada de conexiuni este plina")); //R
            close(client_fd); //R
            _log.LogError("queue full, conexiune respinsa cu 503"); //R
        } //R
    } //R
    printf("Semnal de oprire primit, inchid proxy-ul...\n"); //R
} //R

void Server::Close() //R
{ //R
    if(_listen_fd >= 0) //R
        close(_listen_fd); //R
    _listen_fd = -1; //R
} //R
