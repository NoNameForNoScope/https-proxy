#include "log/Logger.hpp" //R

#include <cerrno> //R
#include <cstdio> //R
#include <ctime> //R
#include <fcntl.h> //R
#include <unistd.h> //R

namespace //R
{ //R
    // "2026-10-08 14:32:11", localtime_r e varianta thread-safe a lui localtime //R
    std::string Timestamp() //R
    { //R
        time_t now = time(nullptr); //R
        struct tm tm_now; //R
        localtime_r(&now, &tm_now); //R
        char buf[32]; //R
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_now); //R
        return buf; //R
    } //R
} //R

Logger::~Logger() //R
{ //R
    Close(); //R
} //R

bool Logger::Open(const std::string &path) //R
{ //R
    // O_APPEND: fiecare write() se adauga la final, chiar daca fisierul exista //R
    _fd = open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644); //R
    if(_fd < 0) //R
    { //R
        perror(("open " + path).c_str()); //R
        return false; //R
    } //R
    return true; //R
} //R

void Logger::Close() //R
{ //R
    pthread_mutex_lock(&_mutex); //R
    if(_fd >= 0) //R
        close(_fd); //R
    _fd = -1; //R
    pthread_mutex_unlock(&_mutex); //R
} //R

void Logger::WriteLine(const std::string &line) //R
{ //R
    std::string full = Timestamp() + " " + line + "\n"; //R

    // CF12.3: un singur thread scrie la un moment dat, linia intreaga dintr-o data //R
    pthread_mutex_lock(&_mutex); //R
    if(_fd >= 0) //R
    { //R
        size_t written = 0; //R
        while(written < full.size()) //R
        { //R
            ssize_t n = write(_fd, full.data() + written, full.size() - written); //R
            if(n < 0 && errno == EINTR) //R
                continue; //R
            if(n <= 0) //R
                break; //R
            written += static_cast<size_t>(n); //R
        } //R
    } //R
    pthread_mutex_unlock(&_mutex); //R
} //R

void Logger::LogRequest(const std::string &method, const std::string &host, //R
                        const std::string &path, int status, long duration_ms) //R
{ //R
    // cererile invalide pot sa nu aiba host/cale, punem "-" ca linia sa aiba mereu 6 campuri //R
    WriteLine((method.empty() ? "-" : method) + " " + (host.empty() ? "-" : host) + " " + //R
              (path.empty() ? "-" : path) + " " + std::to_string(status) + " " + //R
              std::to_string(duration_ms) + "ms"); //R
} //R

void Logger::LogError(const std::string &message) //R
{ //R
    WriteLine("ERR " + message); //R
} //R
