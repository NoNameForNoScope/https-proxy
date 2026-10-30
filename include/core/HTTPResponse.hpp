#ifndef HTTPRESPONSE_HPP //R
#define HTTPRESPONSE_HPP //R

#include <string> //R

// textul standard al codului de stare: 400 => "Bad Request" //R
const char *ReasonPhrase(int status); //R

// raspuns construit de proxy: body text scurt, Content-Length si Connection: close (CF11.3) //R
std::string BuildTextResponse(int status, const std::string &body); //R

// raspunsul de eroare, cu textul erorii in body //R
std::string BuildErrorResponse(int status, const std::string &detail = ""); //R

#endif //R
