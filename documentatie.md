## Surse si unde sunt folosite in cod

1. RFC 9112 (HTTP/1.1) – https://www.rfc-editor.org/rfc/rfc9112
   `src/core/HTTPRequest.cpp`: `ParseRequest`, `ParseRequestLine`, `ParseTarget`, `ParseBodyLength`; `src/forward/SocketIO.cpp`: `ReadHeaders`, `StreamBody`, `StreamChunkedBody`.

2. RFC 9110 (HTTP Semantics) – https://www.rfc-editor.org/rfc/rfc9110
   `src/core/HTTPRequest.cpp`: parsarea si cautarea header-elor; `src/core/HTTPResponse.cpp`: `ReasonPhrase`, `BuildTextResponse`, `BuildErrorResponse`.

3. RFC 6585 (codul 431) – https://www.rfc-editor.org/rfc/rfc6585
   `src/core/HTTPResponse.cpp`: codul 431; `src/forward/SocketIO.cpp`: limita header-elor in `ReadHeaders`.

4. socket(7) – https://man7.org/linux/man-pages/man7/socket.7.html
   `src/server/Server.cpp`: `Start`; `src/forward/SocketIO.cpp`: `SetSocketTimeouts`.

5. send(2) – https://man7.org/linux/man-pages/man2/send.2.html
   `src/forward/SocketIO.cpp`: `SendAll`.

6. recv(2) – https://man7.org/linux/man-pages/man2/recv.2.html
   `src/forward/SocketIO.cpp`: `RequestReader::Fill`.

7. accept(2) – https://man7.org/linux/man-pages/man2/accept.2.html
   `src/server/Server.cpp`: `Server::Run`.

8. signal(7) – https://man7.org/linux/man-pages/man7/signal.7.html
   `src/server/Server.cpp`: `OnStopSignal`, `InstallSignalHandlers` si verificarea opririi in `Run`.

9. pthread_cond_wait(3p) – https://man7.org/linux/man-pages/man3/pthread_cond_wait.3p.html
   `src/pool/ThreadPool.cpp`: `TrySubmit`, `Shutdown`, `WorkerLoop`.

10. std::tolower – https://en.cppreference.com/w/cpp/string/byte/tolower
   `src/core/HTTPRequest.cpp`: `EqualsIgnoreCase`, folosita la cautarea header-elor.

11. std::basic_string_view – https://en.cppreference.com/w/cpp/string/basic_string_view
   `include/core/HTTPRequest.hpp` si `src/core/HTTPRequest.cpp`: parametrul `std::string_view` din functiile de parsare; `tests/parse_test.cpp`: `ParseSampleFile`.

12. C++ Core Guidelines SF.7 – https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-using-directive
   `include/core/HTTPRequest.hpp`: eliminarea lui `using namespace std`.

## To do

- Conectarea la server: rezolvare DNS cu `getaddrinfo()`, `connect()` cu timeout si raspunsuri 502/504 la eroare (CF5).
- Trimiterea cererii catre server: forma origin, eliminarea header-elor hop-by-hop si retransmiterea body-ului in blocuri (CF6).
- Citirea raspunsului si trimiterea lui catre client, cu Content-Length, chunked sau pana la inchiderea conexiunii (CF7).
- Verificari cu un server local: GET, POST cu body, raspuns mare si server inaccesibil.
