#include "core/HTTPResponse.hpp" //R

const char *ReasonPhrase(int status) //R
{ //R
    // textele sunt cele din RFC 9110 sectiunea 15, iar 431 din RFC 6585 sectiunea 5 //R
    switch(status) //R
    { //R
        case 200: return "OK"; //R
        case 400: return "Bad Request"; //R
        case 408: return "Request Timeout"; //R
        case 413: return "Content Too Large"; //R
        case 431: return "Request Header Fields Too Large"; //R
        case 501: return "Not Implemented"; //R
        case 502: return "Bad Gateway"; //R
        case 503: return "Service Unavailable"; //R
        case 504: return "Gateway Timeout"; //R
        case 505: return "HTTP Version Not Supported"; //R
        default:  return "Unknown"; //R
    } //R
} //R

std::string BuildTextResponse(int status, const std::string &body) //R
{ //R
    std::string code = std::to_string(status); //R
    return "HTTP/1.1 " + code + " " + ReasonPhrase(status) + "\r\n" //R
           "Content-Type: text/plain; charset=utf-8\r\n" //R
           "Content-Length: " + std::to_string(body.size()) + "\r\n" //R
           "Connection: close\r\n" //R
           "\r\n" + body; //R
} //R

std::string BuildErrorResponse(int status, const std::string &detail) //R
{ //R
    std::string body = std::to_string(status) + " " + ReasonPhrase(status); //R
    if(!detail.empty()) //R
        body += ": " + detail; //R
    return BuildTextResponse(status, body + "\n"); //R
} //R
