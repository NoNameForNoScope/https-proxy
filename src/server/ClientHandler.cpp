#include "server/ClientHandler.hpp" //R
#include "core/HTTPRequest.hpp" //R
#include "core/HTTPResponse.hpp" //R
#include "forward/SocketIO.hpp" //R

#include <chrono> //R

namespace //R
{ //R
    long ElapsedMs(std::chrono::steady_clock::time_point start) //R
    { //R
        auto diff = std::chrono::steady_clock::now() - start; //R
        return static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(diff).count()); //R
    } //R

    // trimite raspunsul de eroare si scrie linia in log //R
    void Reply(int fd, Logger &log, const HTTPRequest &req, int status, const std::string &detail, //R
               std::chrono::steady_clock::time_point start) //R
    { //R
        SendAll(fd, BuildErrorResponse(status, detail)); //R
        log.LogRequest(req.GetMethod(), req.GetHost(), req.GetPath(), status, ElapsedMs(start)); //R
    } //R
} //R

// fd-ul il inchide ThreadPool dupa ce functia se termina (vezi ThreadPool.hpp) //R
void HandleClient(int client_fd, const Config &cfg, Logger &log) //R
{ //R
    // TODO CF5/CF10: cfg.upstream_* se folosesc cand facem conectarea la server / upstream //R
    (void)cfg; //R

    auto start = std::chrono::steady_clock::now(); //R
    HTTPRequest req; //R

    // CF3.5: daca clientul nu mai trimite nimic 30 s, recv() se intoarce cu timeout //R
    if(!SetSocketTimeouts(client_fd, limits::kIoTimeoutSec)) //R
        log.LogError("setsockopt timeout pe fd " + std::to_string(client_fd)); //R

    RequestReader reader(client_fd); //R
    std::string raw_headers; //R
    ReadStatus st = reader.ReadHeaders(raw_headers); //R
    switch(st) //R
    { //R
        case ReadStatus::Ok: //R
            break; //R
        case ReadStatus::TooLarge: //R
            // CF3.4 //R
            Reply(client_fd, log, req, 431, "header-ele depasesc 16 KB", start); //R
            return; //R
        case ReadStatus::Timeout: //R
            // CF3.5: 408 doar daca cererea a inceput; conexiune goala => inchidem simplu //R
            if(reader.HasPendingData()) //R
                Reply(client_fd, log, req, 408, "cerere incompleta dupa 30 s", start); //R
            return; //R
        default: //R
            // clientul a inchis sau eroare de socket: nu avem cui raspunde //R
            return; //R
    } //R

    // CF4: parsarea lucreaza pe buffer, fara socket (CNF6.2) //R
    int err = req.ParseRequest(raw_headers); //R
    if(err != 0) //R
    { //R
        Reply(client_fd, log, req, err, "", start); //R
        return; //R
    } //R

    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ //R
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ //R

    //TODO CF5 conectarea la server //R
    //host = cfg.upstream_enabled ? cfg.upstream_host : req.GetHost() //R
    //port = cfg.upstream_enabled ? cfg.upstream_port : req.GetPort() //R
    //getaddrinfo(host, port) doar IPv4 (AF_INET, SOCK_STREAM) //R
    //  daca nu se rezolva => Reply(client_fd, log, req, 502, ...) //R
    //server_fd = socket(AF_INET, SOCK_STREAM, 0) //R
    //connect() cu timeout de limits::kConnectTimeoutSec (10 s) //R
    //  conexiune refuzata => 502, timeout => 504 //R
    //  in ambele cazuri close(server_fd) si LogError cu host:port //R
    // //R
    // //R
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ //R
    // //R
    //TODO CF6 trimiterea cererii catre server //R
    //fara upstream: request line in forma origin => req.GetMethod() + " " + req.GetPath() + " " + req.GetVersion() //R
    //cu upstream: request line ramane in forma absoluta (CF10.2) //R
    //daca lipseste Host il completam din req.GetHost() (+ port daca nu e 80) //R
    //stergem header-ele hop-by-hop: Connection, Keep-Alive si ce e listat in Connection (RFC 9110 7.6.1) //R
    //adaugam Connection: close //R
    //SendAll(server_fd, request line + headere + "\r\n") //R
    //body-ul: sink-ul de mai jos face SendAll(server_fd, data, len) in loc sa numere octetii //R
    //  blocuri de 8 KB, nu tinem body-ul in memorie (CF6.4, Intercept OFF) //R
    // //R
    // //R
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ //R
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ //R

    // CF3.3 + CF6.4: body-ul (Content-Length sau chunked) trece prin sink in blocuri de 8 KB //R
    // Intercept (CF8) nu exista inca, deci suntem mereu in cazul "Intercept OFF": nu tinem body-ul in memorie //R
    size_t body_bytes = 0; //R
    BodySink count_only = [&body_bytes](const char *, size_t len) //R
    { //R
        body_bytes += len; //R
        return true; //R
    }; //R
    if(req.IsChunked()) //R
        st = reader.StreamChunkedBody(limits::kMaxBodySize, count_only); //R
    else if(req.GetContentLength() > 0) //R
        st = reader.StreamBody(req.GetContentLength(), count_only); //R
    else //R
        st = ReadStatus::Ok; //R

    if(st != ReadStatus::Ok) //R
    { //R
        if(st == ReadStatus::Timeout) //R
            Reply(client_fd, log, req, 408, "body incomplet dupa 30 s", start); //R
        else if(st == ReadStatus::TooLarge) //R
            Reply(client_fd, log, req, 413, "body-ul depaseste 50 MB", start); //R
        else if(st == ReadStatus::BadFormat) //R
            Reply(client_fd, log, req, 400, "chunked invalid", start); //R
        return; //R
    } //R

    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ //R
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ //R

    //TODO CF7 raspunsul de la server catre client //R
    //citim status line + headerele de la server_fd (max 16 KB), ca in RequestReader::ReadHeaders //R
    //  retinem codul de stare pentru log //R
    //fara body: cererea a fost HEAD sau status 1xx, 204, 304 //R
    //altfel: Content-Length => StreamBody //R
    //        chunked        => StreamChunkedBody //R
    //        niciunul       => citim pana inchide serverul (recv() == 0) //R
    //fiecare bucata de 8 KB merge direct la client cu SendAll, pe masura ce vine (CF7.3) //R
    //clientul inchide la mijloc => oprim citirea de la server (CF7.4) //R
    //serverul inchide la mijloc => inchidem si clientul, sa nu astepte (CF11.2) //R
    //close(server_fd), LogRequest cu codul primit de la server //R
    //cand e gata CF5-CF7, raspunsul 501 de mai jos dispare //R
    // //R
    // //R
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ //R
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ //R

    // pana atunci raspundem cu 501 si cu ce am inteles din cerere, ca sa se poata testa cu curl //R
    std::string info = "Forwarding-ul (CF5-CF7) nu este implementat inca.\n" //R
                       "Cerere parsata: " + req.GetMethod() + " " + req.GetHost() + ":" + //R
                       std::to_string(req.GetPort()) + " " + req.GetPath() + " " + req.GetVersion() + //R
                       "\nForma: " + (req.IsAbsoluteForm() ? "absoluta" : "origin") + //R
                       "\nHeader-e: " + std::to_string(req.GetHeaders().size()) + //R
                       "\nBody: " + std::to_string(body_bytes) + " bytes" + //R
                       (req.IsChunked() ? " (chunked)" : "") + "\n"; //R
    SendAll(client_fd, BuildTextResponse(501, info)); //R
    log.LogRequest(req.GetMethod(), req.GetHost(), req.GetPath(), 501, ElapsedMs(start)); //R

    // TODO CF9: keep-alive, deocamdata o cerere pe conexiune (raspundem cu Connection: close) //R
} //R
