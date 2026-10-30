#ifndef SOCKETIO_HPP //R
#define SOCKETIO_HPP //R

#include <cstdint> //R
#include <functional> //R
#include <string> //R
#include <vector> //R

// rezultatul unei citiri de pe socket //R
enum class ReadStatus //R
{ //R
    Ok, //R
    Closed,     // recv() a intors 0, celalalt capat a inchis //R
    Timeout,    // SO_RCVTIMEO a expirat (EAGAIN / EWOULDBLOCK) //R
    TooLarge,   // s-a depasit limita (431 / 413) //R
    BadFormat,  // chunked invalid => 400 //R
    Error       // alta eroare de socket sau sink-ul a refuzat datele //R
}; //R

// primeste body-ul bucata cu bucata (max 8 KB odata); intoarce false ca sa opreasca citirea //R
// cand avem forwarding (CF6.4), sink-ul va trimite fiecare bucata direct la server //R
using BodySink = std::function<bool(const char *data, size_t len)>; //R

// send() nu garanteaza ca trimite tot, asa ca repetam pana la ultimul octet (CF6.3) //R
bool SendAll(int fd, const char *data, size_t len); //R
bool SendAll(int fd, const std::string &data); //R

// seteaza SO_RCVTIMEO si SO_SNDTIMEO pe un socket //R
bool SetSocketTimeouts(int fd, int seconds); //R

// CF3: citeste o cerere de pe socketul clientului //R
// TCP e un flux de octeti, o cerere poate veni in mai multe recv() sau doua cereri intr-unul //R
class RequestReader //R
{ //R
private: //R
    int _fd; //R
    // octetii primiti dar inca nefolositi (CF3.2), raman pentru body sau cererea urmatoare //R
    std::string _buffer; //R

    ReadStatus Fill(); //R
    ReadStatus ReadLine(std::string &line, size_t max_len); //R

public: //R
    explicit RequestReader(int fd) : _fd(fd) {} //R

    // CF3.1: citeste pana la \r\n\r\n, pune request line + header-ele in out_headers //R
    // TooLarge daca header-ele trec de 16 KB (CF3.4) //R
    ReadStatus ReadHeaders(std::string &out_headers); //R

    // CF3.3 + CF6.4: exact length octeti dupa Content-Length, dati la sink in blocuri de 8 KB //R
    // memoria pe conexiune ramane mica, oricat de mare e body-ul (CNF1.3) //R
    ReadStatus StreamBody(size_t length, const BodySink &sink); //R

    // CF3.3 + CF6.4: chunk-uri pana la cel de dimensiune 0, datele decodate merg la sink //R
    ReadStatus StreamChunkedBody(size_t max_size, const BodySink &sink); //R

    // Intercept ON (CF8.2, CF6.4): cererea e retinuta, deci body-ul se tine intreg in out //R
    ReadStatus ReadBody(size_t length, std::vector<uint8_t> &out); //R
    ReadStatus ReadChunkedBody(size_t max_size, std::vector<uint8_t> &out); //R

    // true daca s-a primit ceva din cererea curenta (pentru 408 vs inchidere simpla) //R
    bool HasPendingData() const { return !_buffer.empty(); } //R
}; //R

#endif //R
