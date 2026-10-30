#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <iostream>
//#include <map>
#include <string>
#include <string_view> //R
#include <utility> //R
#include <vector>
#include <cstdint>


//using namespace std;
// fara "using namespace std" in header (C++ Core Guidelines SF.7) //R
// orice fisier care include header-ul ar primi tot namespace-ul std //R

class HTTPRequest
{
private:
    //string _method;
    //string _uri;
    //string _version;
    //map<string, string> headers;
    //vector<uint8_t> body;
    std::string _method; //R
    std::string _uri; //R
    std::string _version; //R
    // vector de perechi in loc de map: pastram ordinea si numele original (CF4.3) //R
    std::vector<std::pair<std::string, std::string>> headers; //R
    std::vector<uint8_t> body; //R

    // tinta cererii, extrasa din uri sau din Host (CF4.2) //R
    std::string _host; //R
    uint16_t _port = 80; //R
    std::string _path; //R
    bool _absolute_form = false; //R

    // lungimea body-ului (CF3.3) //R
    size_t _content_length = 0; //R
    bool _chunked = false; //R

    int ParseRequestLine(std::string_view line); //R
    int ParseTarget(); //R
    int ParseBodyLength(); //R

public:
    //void ParseRequest(const string &raw_request);
    // returneaza 0 daca cererea e valida, altfel codul HTTP de eroare (400, 413, 501, 505) //R
    // string_view: nu se mai copiaza header-ele, doar ne uitam in buffer //R
    // ATENTIE: bufferul trebuie sa traiasca cat timp se apeleaza functia //R
    int ParseRequest(std::string_view raw_request); //R
    void PrintRequest() const;

    //getters
    //string GetMethod() const { return _method; }
    //string GetUri() const { return _uri; }
    //string GetVersion() const { return _version; }
    std::string GetMethod() const { return _method; } //R
    std::string GetUri() const { return _uri; } //R
    std::string GetVersion() const { return _version; } //R
    std::string GetHost() const { return _host; } //R
    uint16_t GetPort() const { return _port; } //R
    std::string GetPath() const { return _path; } //R
    bool IsAbsoluteForm() const { return _absolute_form; } //R
    size_t GetContentLength() const { return _content_length; } //R
    bool IsChunked() const { return _chunked; } //R
    const std::vector<std::pair<std::string, std::string>>& GetHeaders() const { return headers; } //R
    const std::vector<uint8_t>& GetBody() const { return body; } //R


    //cand accesam headers[key]
    //daca in headers exista intrarea key, returneaza value
    //dar daca in headers NU exista intrarea key, se va crea o intrare a carei valoare va fi ""
    //de aceea cautam prin map mai intai, ca sa nu modificam headers
    //
    //string GetHeaderValue(const string& key) const {
    //    auto it = headers.find(key);
    //    while(it != headers.end())
    //    {
    //        return it->second;
    //    }
    //    return "";
    //
    //}
    // cautare fara sa tina cont de majuscule (CF4.3), "Content-Length" == "content-length" //R
    // nu se creeaza intrari noi, doar se cauta //R
    std::string GetHeaderValue(std::string_view key) const; //R
    bool HasHeader(std::string_view key) const; //R
    size_t CountHeader(std::string_view key) const; //R


    //setters??? de ce as vrea setters
    //poate pentru "reguli de modificare"
    //schimba protocolul din HTTP/2 in HTTP/3
    // doar intre HTTP/1.0 si HTTP/1.1: HTTP/2 si HTTP/3 nu sunt suportate (DCS 2.3, CF8.3) //R
    // body-ul il citeste RequestReader de pe socket, dupa ParseRequest //R
    void SetBody(std::vector<uint8_t> &&new_body) { body = std::move(new_body); } //R
};


#endif
