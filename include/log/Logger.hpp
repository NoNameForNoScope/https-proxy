#ifndef LOGGER_HPP //R
#define LOGGER_HPP //R

#include <pthread.h> //R
#include <string> //R

// CF12: un singur fisier de log, folosit de toti workerii //R
// formatul liniilor se schimba doar aici (CNF6.4) //R
class Logger //R
{ //R
private: //R
    int _fd = -1; //R
    pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER; //R

    void WriteLine(const std::string &line); //R

public: //R
    Logger() = default; //R
    ~Logger(); //R
    Logger(const Logger &) = delete; //R
    Logger &operator=(const Logger &) = delete; //R

    // deschide in mod append (CF12.3) //R
    bool Open(const std::string &path); //R
    void Close(); //R

    // CF12.1: "2026-10-08 14:32:11 GET example.com /index.html 200 43ms" //R
    // fara body, Cookie sau Authorization (CNF4.4) //R
    void LogRequest(const std::string &method, const std::string &host, //R
                    const std::string &path, int status, long duration_ms); //R

    // CF12.2: "2026-10-08 14:32:15 ERR connect example.com:80 timeout" //R
    void LogError(const std::string &message); //R
}; //R

#endif //R
