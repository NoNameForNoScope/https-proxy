#include "core/HTTPRequest.hpp"
//#include <sstream>
#include "config/Limits.hpp" //R
#include <cctype>
#include <algorithm>

namespace //R
{ //R
    // comparatie fara majuscule, numele de header sunt case-insensitive (RFC 9110 5.1) //R
    bool EqualsIgnoreCase(std::string_view a, std::string_view b) //R
    { //R
        if(a.size() != b.size()) //R
            return false; //R
        for(size_t i = 0; i < a.size(); i++) //R
            if(std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) //R
                return false; //R
        return true; //R
    } //R

    // scoate spatiile si tab-urile de la capete (OWS, RFC 9112 5.1) //R
    std::string_view Trim(std::string_view s) //R
    { //R
        size_t start = s.find_first_not_of(" \t"); //R
        if(start == std::string_view::npos) //R
            return {};     //valoarea contine doar spatii //R
        size_t end = s.find_last_not_of(" \t"); //R
        return s.substr(start, end - start + 1); //R
    } //R

    bool IsDigits(std::string_view s) //R
    { //R
        if(s.empty()) //R
            return false; //R
        for(char c : s) //R
            if(c < '0' || c > '9') //R
                return false; //R
        return true; //R
    } //R
} //R

//void HTTPRequest::ParseRequest(const string &raw_request)
//{
//    istringstream stream(raw_request);
//    string line;
int HTTPRequest::ParseRequest(std::string_view raw_request) //R
{ //R
    // resetam tot, ca acelasi obiect sa poata fi refolosit pe keep-alive //R
    *this = HTTPRequest(); //R

    size_t pos = 0; //R
    // citeste o linie din raw_request, fara \n si fara \r //R
    auto next_line = [&](std::string_view &line) -> bool //R
    { //R
        if(pos >= raw_request.size()) //R
            return false; //R
        size_t eol = raw_request.find('\n', pos); //R
        if(eol == std::string_view::npos) //R
            eol = raw_request.size(); //R
        line = raw_request.substr(pos, eol - pos); //R
        pos = eol + 1; //R
        //getline scoate \n, trebuie sa tratam noi \r //R
        if(!line.empty() && line.back() == '\r') //R
            line.remove_suffix(1); //R
        return true; //R
    }; //R

    std::string_view line; //R

    //citim request line
    //if(getline(stream, line))
    //{
    //    //getline scoate \n, trebuie sa tratam noi \r
    //    if(!line.empty() && line.back() == '\r')
    //        line.pop_back();
    //
    //    istringstream first_line_stream(line);
    //    first_line_stream >> _method >> _uri >> _version;
    //}
    if(!next_line(line)) //R
        return 400; //R
    int err = ParseRequestLine(line); //R
    if(err != 0) //R
        return err; //R

    //citim headerele
    //while(getline(stream, line))
    //{
    //    //rand gol sau doar \r => final de header
    //    if(line.empty() || line == "\r")
    //        break;
    //
    //    ///stergem \r
    //    if(line.back() == '\r')
    //        line.pop_back();
    while(next_line(line)) //R
    { //R
        //rand gol => final de header //R
        if(line.empty()) //R
            break; //R

        // obs-fold (linie care incepe cu spatiu) e respinsa cu 400 (RFC 9112 5.2) //R
        if(line.front() == ' ' || line.front() == '\t') //R
            return 400; //R

        //extragem cheie valoare din linie dupa ':'
        size_t delim_pos = line.find(':');
        //if(delim_pos != string::npos)
        //{
        //    string key = line.substr(0, delim_pos);
        //    string value = line.substr(delim_pos+1);
        //
        //    //dupa ':' apare ' ' sau '\t'
        //    size_t start_val = value.find_first_not_of(" \t");
        //    if(start_val != string::npos)
        //        value = value.substr(start_val);
        //    else
        //        value = "";     //valoarea contine doar spatii
        //
        //
        //    // real proxies lowercase the key
        //
        //    // lowercase lambda function from stackoverflow
        //    transform(key.begin(), key.end(), key.begin(),
        //        [](unsigned char c){ return std::tolower(c); });
        //
        //    headers[key] = value;
        //}
        // header fara ':' => 400 (CF4.4) //R
        if(delim_pos == std::string_view::npos || delim_pos == 0) //R
            return 400; //R

        std::string_view key = line.substr(0, delim_pos); //R
        // nu e voie spatiu intre nume si ':' (RFC 9112 5.1) //R
        if(key.back() == ' ' || key.back() == '\t') //R
            return 400; //R

        //dupa ':' apare ' ' sau '\t', le taiem si de la final //R
        std::string_view value = Trim(line.substr(delim_pos + 1)); //R

        // pastram numele exact cum a venit, comparatia se face case-insensitive la cautare //R
        headers.emplace_back(std::string(key), std::string(value)); //R
    //}
    } //R

    err = ParseTarget(); //R
    if(err != 0) //R
        return err; //R

    return ParseBodyLength(); //R
} //R

int HTTPRequest::ParseRequestLine(std::string_view line) //R
{ //R
    // request-line = method SP request-target SP HTTP-version (RFC 9112 3) //R
    size_t sp1 = line.find(' '); //R
    if(sp1 == std::string_view::npos) //R
        return 400; //R
    size_t sp2 = line.find(' ', sp1 + 1); //R
    if(sp2 == std::string_view::npos) //R
        return 400; //R
    // trebuie fix trei parti, nevide (CF4.4) //R
    if(line.find(' ', sp2 + 1) != std::string_view::npos) //R
        return 400; //R

    _method = std::string(line.substr(0, sp1)); //R
    _uri = std::string(line.substr(sp1 + 1, sp2 - sp1 - 1)); //R
    _version = std::string(line.substr(sp2 + 1)); //R
    if(_method.empty() || _uri.empty() || _version.empty()) //R
        return 400; //R

    // metoda e case-sensitive, "get" nu e "GET" //R
    static const char *kMethods[] = {"GET", "HEAD", "POST", "PUT", "DELETE", "OPTIONS"}; //R
    bool known = false; //R
    for(const char *m : kMethods) //R
        if(_method == m) //R
            known = true; //R
    // CONNECT si restul metodelor => 501 (CF4.1) //R
    if(!known) //R
        return 501; //R

    // doar HTTP/1.0 si HTTP/1.1, altfel 505 (CF4.1) //R
    if(_version != "HTTP/1.0" && _version != "HTTP/1.1") //R
        return _version.rfind("HTTP/", 0) == 0 ? 505 : 400; //R

    return 0; //R
} //R

int HTTPRequest::ParseTarget() //R
{ //R
    std::string_view uri = _uri; //R
    std::string_view authority; //R

    // forma absoluta: http://host[:port]/cale (RFC 9112 3.2.2), asa trimite browserul la proxy //R
    if(uri.size() >= 7 && EqualsIgnoreCase(uri.substr(0, 7), "http://")) //R
    { //R
        _absolute_form = true; //R
        uri.remove_prefix(7); //R
        size_t path_start = uri.find_first_of("/?"); //R
        authority = uri.substr(0, path_start); //R
        if(path_start == std::string_view::npos) //R
            _path = "/"; //R
        else if(uri[path_start] == '?') //R
            _path = "/" + std::string(uri.substr(path_start)); //R
        else //R
            _path = std::string(uri.substr(path_start)); //R
    } //R
    // forma origin: /cale, host-ul se ia din header-ul Host (CF4.2) //R
    else if(uri.front() == '/' || (uri == "*" && _method == "OPTIONS")) //R
    { //R
        _path = _uri; //R
        // un singur Host, altfel 400 (RFC 9112 3.2) //R
        if(CountHeader("Host") != 1) //R
            return 400; //R
        authority = Trim(GetHeaderValue("Host")); //R
    } //R
    // https://, ftp:// etc. nu sunt tratate //R
    else //R
        return 400; //R

    // la HTTP/1.1 header-ul Host e obligatoriu (RFC 9112 3.2) //R
    if(_version == "HTTP/1.1" && !HasHeader("Host")) //R
        return 400; //R
    // mai multe Host => 400, si in forma absoluta (RFC 9112 3.2) //R
    if(CountHeader("Host") > 1) //R
        return 400; //R

    // separam host si port, portul implicit e 80 (RFC 9110 4.2.1) //R
    size_t colon = authority.rfind(':'); //R
    std::string_view host = authority.substr(0, colon); //R
    if(colon != std::string_view::npos) //R
    { //R
        std::string_view port = authority.substr(colon + 1); //R
        if(!port.empty()) //R
        { //R
            if(!IsDigits(port) || port.size() > 5) //R
                return 400; //R
            unsigned long p = std::stoul(std::string(port)); //R
            if(p == 0 || p > 65535) //R
                return 400; //R
            _port = static_cast<uint16_t>(p); //R
        } //R
    } //R
    // user:parola@host nu acceptam //R
    if(host.empty() || host.find('@') != std::string_view::npos) //R
        return 400; //R

    // hostname-ul nu tine cont de majuscule, il tinem cu litere mici //R
    _host = std::string(host); //R
    // real proxies lowercase the key //R

    // lowercase lambda function from stackoverflow //R
    transform(_host.begin(), _host.end(), _host.begin(), //R
        [](unsigned char c){ return std::tolower(c); }); //R

    return 0; //R
} //R

int HTTPRequest::ParseBodyLength() //R
{ //R
    bool has_te = HasHeader("Transfer-Encoding"); //R
    bool has_cl = HasHeader("Content-Length"); //R

    // ambele => 400, posibil request smuggling (CF4.4, RFC 9112 6.3) //R
    if(has_te && has_cl) //R
        return 400; //R

    if(has_te) //R
    { //R
        // acceptam doar "chunked" ca ultima codare, restul => 501 (RFC 9112 6.1) //R
        std::string te = GetHeaderValue("Transfer-Encoding"); //R
        transform(te.begin(), te.end(), te.begin(), //R
            [](unsigned char c){ return std::tolower(c); }); //R
        if(CountHeader("Transfer-Encoding") != 1 || Trim(te) != "chunked") //R
            return 501; //R
        _chunked = true; //R
        return 0; //R
    } //R

    if(has_cl) //R
    { //R
        // mai multe Content-Length diferite => 400 (RFC 9112 6.3) //R
        std::string first; //R
        for(const auto &pair : headers) //R
        { //R
            if(!EqualsIgnoreCase(pair.first, "Content-Length")) //R
                continue; //R
            // doar cifre, fara semn, fara spatii (CF4.4) //R
            if(!IsDigits(pair.second)) //R
                return 400; //R
            if(!first.empty() && first != pair.second) //R
                return 400; //R
            first = pair.second; //R
        } //R

        // verificam limita inainte de conversie, ca sa nu avem overflow (CNF4.2) //R
        std::string_view digits = first; //R
        digits.remove_prefix(std::min(digits.find_first_not_of('0'), digits.size())); //R
        if(digits.size() > 9) //R
            return 413; //R
        _content_length = digits.empty() ? 0 : std::stoul(std::string(digits)); //R
        // peste 50 MB => 413 (CF4.5) //R
        if(_content_length > limits::kMaxBodySize) //R
            return 413; //R
    } //R

    //fara niciunul dintre ele cererea nu are body (CF3.3) //R
    return 0; //R
} //R

std::string HTTPRequest::GetHeaderValue(std::string_view key) const //R
{ //R
    for(const auto &pair : headers) //R
        if(EqualsIgnoreCase(pair.first, key)) //R
            return pair.second; //R
    return ""; //R
} //R

bool HTTPRequest::HasHeader(std::string_view key) const //R
{ //R
    return CountHeader(key) > 0; //R
} //R

size_t HTTPRequest::CountHeader(std::string_view key) const //R
{ //R
    size_t count = 0; //R
    for(const auto &pair : headers) //R
        if(EqualsIgnoreCase(pair.first, key)) //R
            count++; //R
    return count; //R
}

void HTTPRequest::PrintRequest() const
{
    //cout << _method << " " << _uri << " " << _version << "\r\n";
    std::cout << _method << " " << _uri << " " << _version << "\r\n"; //R
    for(const auto& pair : headers)
        //cout << pair.first << ": " << pair.second << "\r\n";
        std::cout << pair.first << ": " << pair.second << "\r\n"; //R
    //cout << "\r\n";
    std::cout << "\r\n"; //R
    // body-ul il afisam doar ca dimensiune, poate fi binar //R
    if(!body.empty()) //R
        std::cout << "[body: " << body.size() << " bytes]\r\n"; //R
} //R
