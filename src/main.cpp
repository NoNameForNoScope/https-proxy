#include "core/HTTPRequest.hpp"

#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[], char *envp[])
{
    HTTPRequest req;

    int fd = open("tests/file.txt", O_RDONLY);
    if(fd < 0)
    {
        perror("Opening file.txt");
        exit(-1);
    }


    //Met1
    vector<uint8_t> raw_request;
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

    cout << "Request size: " << raw_request.size() << "\r\n";

    // Conversie in string, ineficient, se copiaza tot inca odata
    // cout << "Request(vector<uint8_t>):\r\n" << string(raw_request.begin(), raw_request.end());
    // cout << "\r\n\r\r\n\r\n\r\n\r\n";

    cout << "Request(reinterpret cast const char*):\r\n";
    cout.write(reinterpret_cast<const char*>(raw_request.data()), raw_request.size());
    cout << "\r\n\r\n";


    // se face copie la headere, ineficient
    // todo: string_view?
    // creeaza un string peste zona de memorie, un fel de view in SQL?
    // understand and implement it
    req.ParseRequest(string(raw_request.begin(), raw_request.end()));
    req.PrintRequest();



    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    
    //TODO
    //contentLength = req.GetHeaderValue("Content-Length");
    //if(contentLentgh != "")
    //  if(contentLength > 0)
    //      continua cu citirea bodyului
    //      bazat pe Content-Type, parsezi intr un anumit stil
    //      faci o interfata (POO) IParser
    //      folosesti parserul necesar, implementezi mai multe
    //
    //
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //
    //
    //  deja am facut functie de print header, trebuie facuta si pentru body
    //  trebuie doar sa fie printata intr un textbox editabil
    //  probabil un fd catre o aplicatie de editare (vscode???) sau ceva facut de noi
    //  apoi salvam intr o instanta noua req
    //  daca la editare este introdus si in body, schimbam formatul req
    //  probabil citim de la 0, nu incercam sa inlocuim requestul primit
    //  apoi dam drop la requestul initial
    
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    
    return 0;
}