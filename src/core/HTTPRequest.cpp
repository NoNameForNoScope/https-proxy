#include "core/HTTPRequest.hpp"
#include <sstream>
#include <cctype>
#include <algorithm>

void HTTPRequest::ParseRequest(const string &raw_request)
{
    istringstream stream(raw_request);
    string line;

    //citim request line
    if(getline(stream, line))
    {
        //getline scoate \n, trebuie sa tratam noi \r
        if(!line.empty() && line.back() == '\r')
            line.pop_back();
        
        istringstream first_line_stream(line);
        first_line_stream >> _method >> _uri >> _version;
    }

    //citim headerele
    while(getline(stream, line))
    {
        //rand gol sau doar \r => final de header
        if(line.empty() || line == "\r")
            break;
        
        ///stergem \r
        if(line.back() == '\r')
            line.pop_back();
        
        //extragem cheie valoare din linie dupa ':'
        size_t delim_pos = line.find(':');
        if(delim_pos != string::npos)
        {
            string key = line.substr(0, delim_pos);
            string value = line.substr(delim_pos+1);

            //dupa ':' apare ' ' sau '\t'
            size_t start_val = value.find_first_not_of(" \t");
            if(start_val != string::npos)
                value = value.substr(start_val);
            else
                value = "";     //valoarea contine doar spatii


            // real proxies lowercase the key
            
            // lowercase lambda function from stackoverflow 
            transform(key.begin(), key.end(), key.begin(),
                [](unsigned char c){ return std::tolower(c); });

            headers[key] = value;
        }
    }
}

void HTTPRequest::PrintRequest() const
{
    cout << _method << " " << _uri << " " << _version << "\r\n";
    for(const auto& pair : headers)
        cout << pair.first << ": " << pair.second << "\r\n";
    cout << "\r\n";
}