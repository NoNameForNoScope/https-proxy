#include "forward/SocketIO.hpp" //R
#include "config/Limits.hpp" //R

#include <algorithm> //R
#include <cerrno> //R
#include <sys/socket.h> //R
#include <sys/time.h> //R

bool SendAll(int fd, const char *data, size_t len) //R
{ //R
    size_t sent = 0; //R
    while(sent < len) //R
    { //R
        // MSG_NOSIGNAL: daca clientul a inchis primim EPIPE, nu SIGPIPE care ar opri procesul //R
        ssize_t n = send(fd, data + sent, len - sent, MSG_NOSIGNAL); //R
        if(n < 0 && errno == EINTR) //R
            continue; //R
        if(n <= 0) //R
            return false; //R
        sent += static_cast<size_t>(n); //R
    } //R
    return true; //R
} //R

bool SendAll(int fd, const std::string &data) //R
{ //R
    return SendAll(fd, data.data(), data.size()); //R
} //R

bool SetSocketTimeouts(int fd, int seconds) //R
{ //R
    struct timeval tv; //R
    tv.tv_sec = seconds; //R
    tv.tv_usec = 0; //R
    // dupa timeout recv()/send() intorc -1 cu errno EAGAIN sau EWOULDBLOCK (socket(7)) //R
    return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0 && //R
           setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == 0; //R
} //R

ReadStatus RequestReader::Fill() //R
{ //R
    char block[limits::kBlockSize]; //R
    while(true) //R
    { //R
        ssize_t n = recv(_fd, block, sizeof(block), 0); //R
        if(n > 0) //R
        { //R
            _buffer.append(block, static_cast<size_t>(n)); //R
            return ReadStatus::Ok; //R
        } //R
        // 0 = celalalt capat a inchis conexiunea (recv(2)) //R
        if(n == 0) //R
            return ReadStatus::Closed; //R
        if(errno == EINTR) //R
            continue; //R
        if(errno == EAGAIN || errno == EWOULDBLOCK) //R
            return ReadStatus::Timeout; //R
        return ReadStatus::Error; //R
    } //R
} //R

ReadStatus RequestReader::ReadHeaders(std::string &out_headers) //R
{ //R
    size_t scanned = 0; //R
    while(true) //R
    { //R
        // liniile goale dinaintea request line-ului se ignora (RFC 9112 2.2) //R
        while(_buffer.compare(0, 2, "\r\n") == 0) //R
            _buffer.erase(0, 2); //R

        // cautam doar in partea noua, minus 3 octeti in caz ca \r\n\r\n e taiat intre doua recv() //R
        size_t from = scanned > 3 ? scanned - 3 : 0; //R
        size_t end = _buffer.find("\r\n\r\n", from); //R
        if(end != std::string::npos) //R
        { //R
            size_t header_len = end + 4; //R
            if(header_len > limits::kMaxHeaderSize) //R
                return ReadStatus::TooLarge; //R
            out_headers = _buffer.substr(0, header_len); //R
            // ce e dupa \r\n\r\n ramane in buffer pentru body (CF3.2) //R
            _buffer.erase(0, header_len); //R
            return ReadStatus::Ok; //R
        } //R
        // CF3.4: peste 16 KB fara sfarsit de header-e //R
        if(_buffer.size() >= limits::kMaxHeaderSize) //R
            return ReadStatus::TooLarge; //R

        scanned = _buffer.size(); //R
        ReadStatus st = Fill(); //R
        if(st != ReadStatus::Ok) //R
            return st; //R
    } //R
} //R

ReadStatus RequestReader::StreamBody(size_t length, const BodySink &sink) //R
{ //R
    size_t done = 0; //R
    while(done < length) //R
    { //R
        if(_buffer.empty()) //R
        { //R
            ReadStatus st = Fill(); //R
            if(st != ReadStatus::Ok) //R
                return st; //R
        } //R
        // cel mult un bloc de 8 KB odata (CF6.4), restul e inceputul cererii urmatoare (CF9.3) //R
        size_t take = std::min({length - done, _buffer.size(), limits::kBlockSize}); //R
        if(!sink(_buffer.data(), take)) //R
            return ReadStatus::Error; //R
        _buffer.erase(0, take); //R
        done += take; //R
    } //R
    return ReadStatus::Ok; //R
} //R

ReadStatus RequestReader::ReadBody(size_t length, std::vector<uint8_t> &out) //R
{ //R
    out.clear(); //R
    out.reserve(length); //R
    return StreamBody(length, [&out](const char *data, size_t len) //R
    { //R
        out.insert(out.end(), data, data + len); //R
        return true; //R
    }); //R
} //R

ReadStatus RequestReader::ReadLine(std::string &line, size_t max_len) //R
{ //R
    while(true) //R
    { //R
        size_t eol = _buffer.find("\r\n"); //R
        if(eol != std::string::npos) //R
        { //R
            if(eol > max_len) //R
                return ReadStatus::BadFormat; //R
            line = _buffer.substr(0, eol); //R
            _buffer.erase(0, eol + 2); //R
            return ReadStatus::Ok; //R
        } //R
        if(_buffer.size() > max_len) //R
            return ReadStatus::BadFormat; //R
        ReadStatus st = Fill(); //R
        if(st != ReadStatus::Ok) //R
            return st; //R
    } //R
} //R

ReadStatus RequestReader::ReadChunkedBody(size_t max_size, std::vector<uint8_t> &out) //R
{ //R
    out.clear(); //R
    return StreamChunkedBody(max_size, [&out](const char *data, size_t len) //R
    { //R
        out.insert(out.end(), data, data + len); //R
        return true; //R
    }); //R
} //R

ReadStatus RequestReader::StreamChunkedBody(size_t max_size, const BodySink &sink) //R
{ //R
    // chunk = chunk-size [ chunk-ext ] CRLF chunk-data CRLF (RFC 9112 7.1) //R
    constexpr size_t kMaxLine = 1024; //R
    size_t total = 0; //R
    std::string line; //R
    while(true) //R
    { //R
        ReadStatus st = ReadLine(line, kMaxLine); //R
        if(st != ReadStatus::Ok) //R
            return st; //R

        // ignoram extensiile de dupa ';' //R
        size_t semi = line.find(';'); //R
        if(semi != std::string::npos) //R
            line.erase(semi); //R
        while(!line.empty() && (line.back() == ' ' || line.back() == '\t')) //R
            line.pop_back(); //R
        if(line.empty()) //R
            return ReadStatus::BadFormat; //R

        // dimensiunea e in hex; verificam limita la fiecare cifra, inainte sa alocam ceva (CNF4.2) //R
        size_t size = 0; //R
        for(char c : line) //R
        { //R
            int digit; //R
            if(c >= '0' && c <= '9') //R
                digit = c - '0'; //R
            else if(c >= 'a' && c <= 'f') //R
                digit = c - 'a' + 10; //R
            else if(c >= 'A' && c <= 'F') //R
                digit = c - 'A' + 10; //R
            else //R
                return ReadStatus::BadFormat; //R
            size = size * 16 + static_cast<size_t>(digit); //R
            if(size > max_size) //R
                return ReadStatus::TooLarge; //R
        } //R

        // ultimul chunk are dimensiunea 0, apoi vin trailer-ele pana la o linie goala //R
        if(size == 0) //R
        { //R
            size_t trailer_bytes = 0; //R
            while(true) //R
            { //R
                st = ReadLine(line, kMaxLine); //R
                if(st != ReadStatus::Ok) //R
                    return st; //R
                if(line.empty()) //R
                    return ReadStatus::Ok; //R
                trailer_bytes += line.size(); //R
                if(trailer_bytes > limits::kMaxHeaderSize) //R
                    return ReadStatus::TooLarge; //R
            } //R
        } //R

        // CF4.5: limita de 50 MB se aplica si sumei chunk-urilor //R
        if(total + size > max_size) //R
            return ReadStatus::TooLarge; //R
        total += size; //R

        // datele chunk-ului merg mai departe in blocuri de 8 KB, nu asteptam tot chunk-ul (CF6.4) //R
        st = StreamBody(size, sink); //R
        if(st != ReadStatus::Ok) //R
            return st; //R

        // dupa date trebuie sa vina CRLF //R
        while(_buffer.size() < 2) //R
        { //R
            st = Fill(); //R
            if(st != ReadStatus::Ok) //R
                return st; //R
        } //R
        if(_buffer.compare(0, 2, "\r\n") != 0) //R
            return ReadStatus::BadFormat; //R
        _buffer.erase(0, 2); //R
    } //R
} //R
