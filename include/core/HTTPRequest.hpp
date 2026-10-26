#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <cstdint>


using namespace std;

class HTTPRequest
{
private:
    string _method;
    string _uri;
    string _version;
    map<string, string> headers; 
    vector<uint8_t> body;

public:
    void ParseRequest(const string &raw_request);
    void PrintRequest() const;

    //getters
    string GetMethod() const { return _method; }
    string GetUri() const { return _uri; }
    string GetVersion() const { return _version; }

    
    //cand accesam headers[key]
    //daca in headers exista intrarea key, returneaza value
    //dar daca in headers NU exista intrarea key, se va crea o intrare a carei valoare va fi ""
    //de aceea cautam prin map mai intai, ca sa nu modificam headers

    string GetHeaderValue(const string& key) const {
        auto it = headers.find(key);
        while(it != headers.end())
        {
            return it->second;
        }
        return "";

    }
    

    //setters??? de ce as vrea setters
    //poate pentru "reguli de modificare"
    //schimba protocolul din HTTP/2 in HTTP/3
};


#endif