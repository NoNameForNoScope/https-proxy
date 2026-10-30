//#include "core/HTTPRequest.hpp"
//
//#include <fcntl.h>
//#include <unistd.h>
#include "config/Config.hpp" //R
#include "log/Logger.hpp" //R
#include "pool/ThreadPool.hpp" //R
#include "server/ClientHandler.hpp" //R
#include "server/Server.hpp" //R

#include <iostream> //R

// testul cu tests/file.txt s-a mutat in tests/parse_test.cpp (make test) //R
// TODO-urile despre body / IParser / editare (CF8) sunt la finalul lui main, neschimbate //R

//int main(int argc, char *argv[], char *envp[])
int main(int argc, char *argv[]) //R
{ //R
    // CF1.1 + CF1.2 //R
    Config cfg; //R
    ConfigResult res = LoadConfig(argc, argv, cfg); //R
    if(res == ConfigResult::Help) //R
        return 0; //R
    if(res == ConfigResult::Error) //R
        return 1; //R

    // CF12.3: log in mod append //R
    Logger log; //R
    if(!log.Open(cfg.log_file)) //R
        return 1; //R

    if(!Server::InstallSignalHandlers()) //R
        return 1; //R

    // CF2: coada de 64 si cfg.workers thread-uri; handler-ul HTTP e dat din afara (CNF6.3) //R
    ThreadPool pool(cfg.workers, limits::kQueueSize, //R
                    [&cfg, &log](int fd) { HandleClient(fd, cfg, log); }); //R

    // CF1.3: daca bind-ul esueaza iesim cu cod != 0 //R
    Server server(cfg, pool, log); //R
    if(!server.Start()) //R
        return 1; //R

    if(!pool.Start()) //R
    { //R
        pool.Shutdown(); //R
        return 1; //R
    } //R

    // CF1.4 //R
    PrintConfig(cfg); //R

    server.Run(); //R

    // CF13: asteptam slaves inchidem socketul si log-ul //R
    server.Close(); //R
    pool.Shutdown(); //R
    log.Close(); //R
    std::cout << "Proxy oprit." << std::endl; //R



    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    //TODO
    //contentLength = req.GetHeaderValue("Content-Length");
    //if(contentLentgh != "")
    //  if(contentLength > 0)
    //      continua cu citirea bodyului
    //      bazat pe Content-Type, parsezi intr un anumit stil
    //      faci o interfata (POO) IParser
    //      folosesti parserul necesar, implementezi mai multe
    //
    //
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //
    //
    //  deja am facut functie de print header, trebuie facuta si pentru body
    //  trebuie doar sa fie printata intr un textbox editabil
    //  probabil un fd catre o aplicatie de editare (vscode???) sau ceva facut de noi
    //  apoi salvam intr o instanta noua req
    //  daca la editare este introdus si in body, schimbam formatul req
    //  probabil citim de la 0, nu incercam sa inlocuim requestul primit
    //  apoi dam drop la requestul initial

    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    return 0; //R
} //R
