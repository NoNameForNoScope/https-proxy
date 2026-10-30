// testele pentru componenta http, fara retea (CNF6.2) //R
// codul care citeste tests/file.txt e cel din vechiul main.cpp //R
#include "core/HTTPRequest.hpp"
#include "core/HTTPResponse.hpp" //R

#include <fcntl.h>
#include <unistd.h>
#include <cstdio> //R
#include <cstdlib> //R

static int g_failed = 0; //R

// verifica ce cod intoarce ParseRequest pentru o cerere data //R
static void Expect(const char *name, const std::string &raw, int expected) //R
{ //R
    HTTPRequest req; //R
    int got = req.ParseRequest(raw); //R
    bool ok = got == expected; //R
    if(!ok) //R
        g_failed++; //R
    printf("[%s] %-45s asteptat %3d, primit %3d\n", ok ? " OK " : "FAIL", name, expected, got); //R
} //R

static void Check(const char *name, bool condition) //R
{ //R
    if(!condition) //R
        g_failed++; //R
    printf("[%s] %s\n", condition ? " OK " : "FAIL", name); //R
} //R

static void ParseSampleFile() //R
{
    HTTPRequest req;

    int fd = open("tests/file.txt", O_RDONLY);
    if(fd < 0)
    {
        perror("Opening file.txt");
        exit(-1);
    }


    //Met1
    //vector<uint8_t> raw_request;
    std::vector<uint8_t> raw_request; //R
    uint8_t buffer[4096];
    ssize_t bytes_read;

    while((bytes_read = read(fd, buffer, 4096)) > 0)
        //raw_request.end() este inceputul pentru ca este gol momentan
        raw_request.insert(raw_request.end(), buffer, buffer + bytes_read);



    /*
     //Met 2
    vector<uint8_t> raw_request(4096);
    ssize_t bytes_read = 0;

    //raw_request.data() este pointer catre heap unde vor fi salvate datele
    do
    {
        bytes_read = read(fd, raw_request.data(), 4096);
    }while(bytes_read > 0);
    */

    close(fd);

    if(bytes_read < 0)
    {
        perror("Reading from file.txt");
        exit(-1);
    }

    //cout << "Request size: " << raw_request.size() << "\r\n";
    std::cout << "Request size: " << raw_request.size() << "\r\n"; //R

    // Conversie in string, ineficient, se copiaza tot inca odata
    // cout << "Request(vector<uint8_t>):\r\n" << string(raw_request.begin(), raw_request.end());
    // cout << "\r\n\r\r\n\r\n\r\n\r\n";

    //cout << "Request(reinterpret cast const char*):\r\n";
    //cout.write(reinterpret_cast<const char*>(raw_request.data()), raw_request.size());
    //cout << "\r\n\r\n";


    // se face copie la headere, ineficient
    // todo: string_view?
    // creeaza un string peste zona de memorie, un fel de view in SQL?
    // understand and implement it
    //req.ParseRequest(string(raw_request.begin(), raw_request.end()));
    // string_view peste vector, fara copie (TODO-ul din vechiul main) //R
    std::string_view view(reinterpret_cast<const char*>(raw_request.data()), raw_request.size()); //R
    int err = req.ParseRequest(view); //R
    req.PrintRequest();

    // file.txt e copiat din devtools si are "HTTP/2", proxy-ul accepta doar 1.0 / 1.1 => 505 //R
    Check("file.txt: HTTP/2 => 505", err == 505); //R

    // aceeasi cerere cu HTTP/1.1, cum o trimite browserul catre un proxy HTTP //R
    std::string fixed(view); //R
    size_t ver = fixed.find("HTTP/2"); //R
    fixed.replace(ver, 6, "HTTP/1.1"); //R
    err = req.ParseRequest(fixed); //R
    req.PrintRequest(); //R
    Check("file.txt cu HTTP/1.1: cod 0", err == 0); //R
    Check("file.txt: 23 header-e", req.GetHeaders().size() == 23); //R
    Check("file.txt: host = www.google.com", req.GetHost() == "www.google.com"); //R
    Check("file.txt: header gol pastrat", req.HasHeader("Sec-Ch-Ua-Form-Factors") && req.GetHeaderValue("Sec-Ch-Ua-Form-Factors").empty()); //R
} //R

int main() //R
{ //R
    ParseSampleFile(); //R

    std::cout << "\n--- forma absoluta / origin (CF4.2) ---\n"; //R
    { //R
        HTTPRequest req; //R
        int err = req.ParseRequest("GET http://Example.com:8000/a/b?x=1 HTTP/1.1\r\nHost: example.com:8000\r\n\r\n"); //R
        Check("absoluta: cod 0", err == 0); //R
        Check("absoluta: host = example.com", req.GetHost() == "example.com"); //R
        Check("absoluta: port = 8000", req.GetPort() == 8000); //R
        Check("absoluta: path = /a/b?x=1", req.GetPath() == "/a/b?x=1"); //R
        Check("absoluta: IsAbsoluteForm", req.IsAbsoluteForm()); //R
    } //R
    { //R
        HTTPRequest req; //R
        int err = req.ParseRequest("GET http://example.com HTTP/1.1\r\nHost: example.com\r\n\r\n"); //R
        Check("absoluta fara cale: path = /, port = 80", err == 0 && req.GetPath() == "/" && req.GetPort() == 80); //R
    } //R
    { //R
        HTTPRequest req; //R
        int err = req.ParseRequest("GET /index.html HTTP/1.0\r\nHost: localhost:9000\r\n\r\n"); //R
        Check("origin: host/port din Host", err == 0 && req.GetHost() == "localhost" && req.GetPort() == 9000); //R
    } //R

    std::cout << "\n--- header-e (CF4.3) ---\n"; //R
    { //R
        HTTPRequest req; //R
        req.ParseRequest("POST http://a.com/ HTTP/1.1\r\nHost: a.com\r\nX-One: 1\r\nContent-Length:   5  \r\nX-Two: 2\r\n\r\n"); //R
        Check("cautare case-insensitive", req.GetHeaderValue("content-length") == "5"); //R
        Check("valoare fara spatii la capete", req.GetHeaderValue("CONTENT-LENGTH") == "5"); //R
        Check("ordinea pastrata", req.GetHeaders().size() == 4 && req.GetHeaders()[1].first == "X-One" //R
              && req.GetHeaders()[3].first == "X-Two"); //R
        Check("Content-Length = 5", req.GetContentLength() == 5); //R
        Check("header lipsa => \"\"", req.GetHeaderValue("Cookie").empty()); //R
    } //R

    std::cout << "\n--- erori (CF4.1, CF4.4, CF4.5) ---\n"; //R
    Expect("request line cu 2 parti", "GET /\r\nHost: a\r\n\r\n", 400); //R
    Expect("request line cu 4 parti", "GET / x HTTP/1.1\r\nHost: a\r\n\r\n", 400); //R
    Expect("header fara ':'", "GET / HTTP/1.1\r\nHost: a\r\nBadHeader\r\n\r\n", 400); //R
    Expect("spatiu inainte de ':'", "GET / HTTP/1.1\r\nHost : a\r\n\r\n", 400); //R
    Expect("HTTP/1.1 fara Host", "GET / HTTP/1.1\r\n\r\n", 400); //R
    Expect("doua Host in forma absoluta", "GET http://a.com/ HTTP/1.1\r\nHost: a.com\r\nHost: b.com\r\n\r\n", 400); //R
    Expect("HTTP/1.0 origin fara Host", "GET / HTTP/1.0\r\n\r\n", 400); //R
    Expect("Content-Length nenumeric", "POST http://a/ HTTP/1.1\r\nHost: a\r\nContent-Length: 12a\r\n\r\n", 400); //R
    Expect("Content-Length negativ", "POST http://a/ HTTP/1.1\r\nHost: a\r\nContent-Length: -1\r\n\r\n", 400); //R
    Expect("doua Content-Length diferite", "POST http://a/ HTTP/1.1\r\nHost: a\r\nContent-Length: 1\r\nContent-Length: 2\r\n\r\n", 400); //R
    Expect("Content-Length + Transfer-Encoding", "POST http://a/ HTTP/1.1\r\nHost: a\r\nContent-Length: 3\r\nTransfer-Encoding: chunked\r\n\r\n", 400); //R
    Expect("Content-Length > 50 MB", "POST http://a/ HTTP/1.1\r\nHost: a\r\nContent-Length: 52428801\r\n\r\n", 413); //R
    Expect("Content-Length urias (overflow)", "POST http://a/ HTTP/1.1\r\nHost: a\r\nContent-Length: 99999999999999999999999\r\n\r\n", 413); //R
    Expect("Content-Length = 50 MB exact", "POST http://a/ HTTP/1.1\r\nHost: a\r\nContent-Length: 52428800\r\n\r\n", 0); //R
    Expect("Transfer-Encoding: gzip", "POST http://a/ HTTP/1.1\r\nHost: a\r\nTransfer-Encoding: gzip\r\n\r\n", 501); //R
    Expect("Transfer-Encoding: chunked", "POST http://a/ HTTP/1.1\r\nHost: a\r\nTransfer-Encoding: chunked\r\n\r\n", 0); //R
    Expect("CONNECT (HTTPS)", "CONNECT example.com:443 HTTP/1.1\r\nHost: example.com:443\r\n\r\n", 501); //R
    Expect("metoda necunoscuta", "BREW / HTTP/1.1\r\nHost: a\r\n\r\n", 501); //R
    Expect("metoda cu litere mici", "get / HTTP/1.1\r\nHost: a\r\n\r\n", 501); //R
    Expect("HTTP/2.0", "GET / HTTP/2.0\r\nHost: a\r\n\r\n", 505); //R
    Expect("versiune fara HTTP/", "GET / FOO\r\nHost: a\r\n\r\n", 400); //R
    Expect("https:// in forma absoluta", "GET https://a.com/ HTTP/1.1\r\nHost: a.com\r\n\r\n", 400); //R
    Expect("port invalid", "GET http://a.com:99999/ HTTP/1.1\r\nHost: a.com\r\n\r\n", 400); //R
    Expect("obs-fold", "GET / HTTP/1.1\r\nHost: a\r\nX: 1\r\n  continuare\r\n\r\n", 400); //R
    Expect("OPTIONS *", "OPTIONS * HTTP/1.1\r\nHost: a\r\n\r\n", 0); //R

    std::cout << "\n--- raspunsuri de eroare (CF11.3) ---\n"; //R
    { //R
        std::string r = BuildErrorResponse(431); //R
        Check("431 status line", r.rfind("HTTP/1.1 431 Request Header Fields Too Large\r\n", 0) == 0); //R
        Check("431 are Connection: close", r.find("\r\nConnection: close\r\n") != std::string::npos); //R
        std::string body = r.substr(r.find("\r\n\r\n") + 4); //R
        Check("Content-Length corect", r.find("Content-Length: " + std::to_string(body.size()) + "\r\n") != std::string::npos); //R
    } //R

    std::cout << "\n" << (g_failed == 0 ? "Toate testele au trecut" : "Teste picate: " + std::to_string(g_failed)) << "\n"; //R
    return g_failed == 0 ? 0 : 1; //R
} //R
