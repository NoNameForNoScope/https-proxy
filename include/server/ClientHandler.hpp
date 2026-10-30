#ifndef CLIENTHANDLER_HPP //R
#define CLIENTHANDLER_HPP //R

#include "config/Config.hpp" //R
#include "log/Logger.hpp" //R

// ce face un worker cu o conexiune: citeste cererea, o parseaza, raspunde, logheaza //R
// NU inchide client_fd, il inchide ThreadPool dupa ce functia se termina //R
void HandleClient(int client_fd, const Config &cfg, Logger &log); //R

#endif //R
