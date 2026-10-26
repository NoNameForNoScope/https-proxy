# Document de Cerinte Software – HTTP Proxy

# Cuprins

***Capitolul 1 – Introducere***
1.1 Scopul proiectului
1.2 Lista definitiilor
1.3 Lista referintelor
1.4 Structura DCS

***Capitolul 2 – Descrierea generala a produsului software***
2.1 Descrierea produsului software
2.2 Detalierea platformei HW/SW
2.3 Constrangeri

***Capitolul 3 – Detalierea cerintelor specifice***
3.1 Cerinte functionale
3.2 Cerinte nefunctionale

---

# Capitolul 1 – Introducere

## *1.1 Scopul proiectului*

Proiectul consta intr-un server HTTP proxy scris in C++ pentru Linux. Browserul (sau orice alt client HTTP, de exemplu `curl`) este setat sa trimita cererile la proxy, iar proxy-ul le citeste, le trimite mai departe catre serverul real si intoarce raspunsul clientului. Intre client si proxy, respectiv intre proxy si server, exista doua conexiuni TCP separate.

In plus fata de simpla retransmitere, proxy-ul poate modifica traficul pe baza unor reguli scrise intr-un fisier: header-e adaugate, schimbate sau sterse din cereri si text inlocuit in paginile HTML primite. Nu ne propunem sa inlocuim un proxy complet de tip Squid. Ideea este sa implementam corect partea de retea si de protocol: citirea datelor de pe socket, parsarea mesajelor HTTP, stabilirea lungimii unui body si tratarea erorilor.

Clientii sunt serviti in paralel. Thread-ul principal face doar `accept()`, iar conexiunile acceptate sunt preluate de un numar fix de thread-uri dintr-un thread pool. Asa, o conexiune lenta nu blocheaza socketul pe care se asteapta clienti noi.

Proxy-ul poate fi configurat sa trimita tot traficul printr-un alt proxy HTTP (upstream). Cu aceasta optiune se pot lega doua instante ale aplicatiei una de alta, lucru util si la testare.

HTTPS nu este tratat. Cererile `CONNECT` primesc raspuns 501 si nu se face nicio decriptare TLS. Se folosesc doar interfete POSIX, iar sistemul tinta este Linux.

## *1.2 Lista definitiilor*


| **Termen / Abreviere**                       | **Definitie**                                                                                                                                                                                                         |
| -------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| HTTP                                         | Protocolul prin care un client cere resurse de la un server web si primeste un raspuns                                                                                                                                |
| TCP                                          | Protocol de transport orientat pe conexiune. Ofera un flux de octeti, fara sa pastreze granitele mesajelor trimise                                                                                                    |
| IP                                           | Protocolul responsabil de adresarea si rutarea pachetelor intre masini                                                                                                                                                |
| Proxy / HTTP Proxy                           | Program care sta intre client si server: primeste cererea de la client, o trimite serverului si intoarce raspunsul                                                                                                    |
| Client                                       | Programul care deschide conexiunea si trimite cererea (in proiect, un browser sau`curl`)                                                                                                                              |
| Server destinatie                            | Serverul HTTP caruia ii este adresata cererea                                                                                                                                                                         |
| Upstream proxy                               | Al doilea proxy, catre care proxy-ul local trimite cererile in loc sa se conecteze direct la server                                                                                                                   |
| Socket                                       | Capat de comunicatie prin retea, folosit in program printr-un file descriptor                                                                                                                                         |
| File descriptor (fd)                         | Numar intreg dat de sistemul de operare pentru un fisier sau un socket deschis                                                                                                                                        |
| `socket()`, `bind()`, `listen()`, `accept()` | Apelurile folosite pe partea de server: creare socket, asociere cu adresa si portul local, trecere in starea de ascultare, acceptarea unei conexiuni (returneaza un fd nou pentru clientul respectiv)                 |
| `connect()`                                  | Deschide o conexiune TCP catre o adresa si un port                                                                                                                                                                    |
| `send()`, `recv()`                           | Trimit, respectiv primesc date pe un socket. Niciunul nu garanteaza ca transfera tot ce s-a cerut intr-un singur apel                                                                                                 |
| `getaddrinfo()`                              | Functie de biblioteca ce transforma un hostname in adresa IP                                                                                                                                                          |
| Thread / Thread pool                         | Un fir de executie dintr-un proces / un numar fix de thread-uri create la pornire, care preiau pe rand conexiuni dintr-o coada                                                                                        |
| Mutex                                        | Mecanism care lasa un singur thread sa foloseasca o resursa partajata la un moment dat                                                                                                                                |
| Condition variable                           | Mecanism prin care un thread adoarme pana cand alt thread il trezeste (folosit pentru coada de conexiuni)                                                                                                             |
| Request / Response                           | Mesajul HTTP trimis de client / mesajul trimis de server ca raspuns                                                                                                                                                   |
| Header                                       | Linie`Nume: valoare` dintr-un mesaj HTTP, cu informatii despre mesaj                                                                                                                                                  |
| Body                                         | Continutul unui mesaj HTTP, dupa header-e. Poate lipsi                                                                                                                                                                |
| `Content-Length`                             | Header care da dimensiunea body-ului in octeti                                                                                                                                                                        |
| Chunked                                      | Mod de transfer (`Transfer-Encoding: chunked`) in care body-ul vine in bucati, fiecare precedata de dimensiunea ei, iar ultima are dimensiunea 0                                                                      |
| Keep-Alive                                   | Folosirea aceleiasi conexiuni TCP pentru mai multe cereri HTTP, una dupa alta                                                                                                                                         |
| Header hop-by-hop                            | Header care are sens doar pe o conexiune (client-proxy sau proxy-server) si nu se retransmite, de exemplu`Connection` sau `Keep-Alive`                                                                                |
| Forma absoluta / forma origin                | In forma absoluta tinta cererii contine tot URL-ul (`GET http://example.com/a HTTP/1.1`), asa cum o trimite un browser unui proxy. In forma origin apare doar calea (`GET /a HTTP/1.1`), asa cum o asteapta un server |
| `CONNECT`                                    | Metoda HTTP folosita de browser pentru a cere un tunel catre un server, in special pentru HTTPS                                                                                                                       |
| DCS                                          | Document de Cerinte Software                                                                                                                                                                                          |
| POSIX                                        | Standard pentru interfetele sistemelor de tip Unix/Linux (socket-uri, thread-uri etc.)                                                                                                                                |

## *1.3 Lista referintelor*


| **Referinta**   | **Descriere**                                                                                                   | **URL / Locatie**                                     |
| --------------- | --------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------- |
| RFC 9110        | HTTP Semantics: metode, coduri de stare, header-e                                                               | https://www.rfc-editor.org/rfc/rfc9110                |
| RFC 9112        | HTTP/1.1: formatul mesajelor, lungimea body-ului, chunked, conexiuni persistente                                | https://www.rfc-editor.org/rfc/rfc9112                |
| RFC 9293        | Transmission Control Protocol (TCP)                                                                             | https://www.rfc-editor.org/rfc/rfc9293                |
| POSIX.1-2017    | Interfetele POSIX pentru socket-uri, thread-uri, semnale                                                        | https://pubs.opengroup.org/onlinepubs/9699919799/     |
| Linux man-pages | Documentatia pentru`socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`, `setsockopt`, `getaddrinfo` | https://man7.org/linux/man-pages/                     |
| pthreads(7)     | Thread-uri, mutex-uri si condition variables in Linux                                                           | https://man7.org/linux/man-pages/man7/pthreads.7.html |

## *1.4 Structura DCS*

Documentul este organizat astfel:

- Capitolul 1 descrie ce face proiectul si definitia termenilor de specialitate
- Capitolul 2 oferă o perspectivă de ansamblu asupra modului in care circula o cerere proxy, pe ce platforme ruleaza, ce valori implicite are si care sunt limitele
- Capitolul 3 detaliază punctual cerințele funcționale și pe cele ne-funcționale. Anexele ulterioare vor detalia snippeturi de cod.

---

# Capitolul 2 – Descrierea generala a produsului software

## *2.1 Descrierea produsului software*

Proxy-ul este un program C++ pentru Linux. Browserul este configurat sa foloseasca adresa si portul proxy-ului (implicit `127.0.0.1:8080`) astfel incat toate cererile HTTP sa fie interceptate de el.

Produsul este scalabil orizontal si vertical. Scalabilitatea pe orizontala presupune optiunea de a gestiona mai multe requesturi simultan de la mai multe browsere, prin mai multe threaduri de executie. Scalabilitatea pe verticala presupune posibilitatea de a implementa un proxy chain, astfel incat fiecare server proxy sa redirectioneze traficul (upstream_ip && upstream_port) catre un alt proxy configurat sa asculte pe ip-ul si portul de upstream al proxyului anterior.

O cerere este procesata astfel:

1. Thread-ul principal accepta conexiunea si pune fd-ul clientului in coada de lucru.
2. Un worker liber scoate fd-ul din coada si citeste cererea de pe socket, pana are header-ele complete si, daca exista, tot body-ul.
3. Parseaza request line-ul si header-ele si afla serverul caruia ii este adresata cererea.
4. Aplica regulile de modificare pentru cereri, daca exista.
5. Deschide o conexiune TCP catre server (sau catre upstream proxy, daca este activ) si trimite cererea.
6. Citeste raspunsul serverului, aplica eventualele reguli pentru raspunsuri si il trimite clientului.
7. Scrie in log o linie despre cerere. In cazul in care conexiunea cu un client poate ramane deschisa, workerul asteapta urmatoarea cerere, altfel inchide socketurile si ia o conexiune noua din coada.

Pentru partile comune intre thread-uri (coada de conexiuni si fisierul de log) se folosesc mutex-uri si condition variables din pthreads.

Codul este impartit in componente separate:


| Componenta | Rol                                                                            |
| ---------- | ------------------------------------------------------------------------------ |
| `config`   | Citeste`proxy.conf`si argumentele din linia de comanda                         |
| `server`   | Creeaza socketul de ascultare si ruleaza bucla de`accept()`                    |
| `pool`     | Coada de conexiuni si thread-urile worker                                      |
| `http`     | Parsarea si construirea mesajelor HTTP, fara dependenta de socket-uri          |
| `forward`  | Citirea/trimiterea datelor pe socket-uri, conectarea la server sau la upstream |
| `log`      | Scrierea in fisierul de log                                                    |

Configurarea se face dintr-un fisier `proxy.conf` cu linii de forma `cheie=valoare`, de exemplu:

```
listen_addr=127.0.0.1
listen_port=8080
workers=8
upstream_enabled=no
upstream_host=127.0.0.1
upstream_port=8081
log_file=proxy.log
```

## *2.2 Detalierea platformei HW/SW*

**Hardware.** Proxy-ul nu face calcule grele, deci nu are cerinte speciale: un nucleu si 512 MB RAM sunt de ajuns pentru configuratia implicita cu 8 workeri. Pentru teste, loopback-ul este suficient, iar pentru browser este nevoie de acces la internet.

**Software:**

* Linux (dezvoltat pentru Ubuntu 22.04 sau o distributie echivalenta);
* g++ 11 sau mai nou, cu standardul C++17, si `make`;
* glibc si pthreads (compilare cu `-pthread`);
* pentru testare: `curl`, `nc` (netcat), `python3 -m http.server` ca server de test si un browser (Firefox) cu proxy HTTP setat manual;
* pentru verificarea codului: `valgrind` si ThreadSanitizer (`-fsanitize=thread`).

**Functii de retea folosite:** `socket()`, `setsockopt()` (pentru `SO_REUSEADDR`, `SO_RCVTIMEO` si `SO_SNDTIMEO`), `bind()`, `listen()`, `accept()`, `connect()`, `recv()`, `send()`, `close()` si `getaddrinfo()` pentru rezolvarea hostname-urilor. Pentru oprire se folosesc semnalele SIGINT si SIGTERM, iar SIGPIPE este ignorat.

## *2.3 Constrangeri*

### Valori implicite si limite


| Parametru                                                  | Valoare implicita | Observatii                                            |
| ---------------------------------------------------------- | ----------------- | ----------------------------------------------------- |
| Adresa de ascultare                                        | `127.0.0.1`       | se schimba doar explicit                              |
| Port                                                       | `8080`            |                                                       |
| Worker threads                                             | `8`               | configurabil intre 1 si 64                            |
| Dimensiunea cozii de conexiuni                             | `64`              |                                                       |
| Dimensiunea maxima a header-elor (request line inclus)     | `16 KB`           | peste limita se raspunde cu 431                       |
| Dimensiunea maxima a body-ului unei cereri                 | `50 MB`           | dupa`Content-Length`; peste limita se raspunde cu 413 |
| Dimensiunea maxima a unui raspuns pe care se aplica reguli | `5 MB`            | peste limita raspunsul se trimite nemodificat         |
| Timeout`connect()`                                         | `10 s`            |                                                       |
| Timeout`recv()` / `send()`                                 | `30 s`            |                                                       |
| Timeout conexiune inactiva (keep-alive)                    | `15 s`            |                                                       |
| Dimensiunea blocului de transfer                           | `8 KB`            |                                                       |

### Protocol

* Se accepta HTTP/1.0 si HTTP/1.1. Pentru alte versiuni se raspunde cu 505.
* Metodele tratate sunt GET, HEAD, POST, PUT, DELETE si OPTIONS. Alte metode, precum `CONNECT`, primesc 501.
* Header-ele se termina cu `\r\n\r\n`. Dimensiunea body-ului se afla din `Content-Length` sau din `Transfer-Encoding: chunked`. Daca apar ambele, cererea este respinsa cu 400.
* Raspunsurile la `HEAD` si cele cu status 1xx, 204 si 304 nu au body, indiferent de header-ele primite.
* ## Proxy chaining
* Upstream-ul se configureaza cu `upstream_host` si `upstream_port` si se activa sau dezactiva din `upstream_enabled` ori din linia de comanda.
* Cu upstream activ, proxy-ul local nu se conecteaza la serverul final si nu rezolva hostname-ul acestuia. Cererea ramane in forma absoluta.
* Fara upstream, request line-ul este rescris in forma origin inainte de trimitere.

### Ce nu face proxy-ul

* Nu trateaza HTTPS/TLS si nu face tunele `CONNECT`.
* Nu suporta HTTP/2, WebSocket sau alte protocoale obtinute prin `Upgrade`.
* Nu face cache si nu are autentificare.
* Nu foloseste IPv6, doar IPv4.

---

# Capitolul 3 – Detalierea cerintelor specifice

## *3.1 Cerinte functionale*

### CF1. Pornire si configurare

* **CF1.1** La pornire proxy-ul citeste fisierul `proxy.conf`. Calea poate fi data ca prim argument, altfel se cauta in directorul curent. Daca fisierul lipseste, porneste cu valorile implicite din 2.3 si afiseaza un avertisment.
* **CF1.2** Optiunile din linia de comanda (`--port`, `--workers`, `--upstream host:port`) au prioritate fata de fisier.
* **CF1.3** Se creeaza socketul server cu `socket()`, se seteaza `SO_REUSEADDR`, apoi `bind()` si `listen()`. Daca bind-ul esueaza (de exemplu portul este ocupat), proxy-ul afiseaza cauza si se inchide cu cod de iesire diferit de 0.
* **CF1.4** La pornire se afiseaza pe stdout adresa si portul de ascultare, numarul de workeri si starea upstream-ului (activ cu `host:port` sau dezactivat).

### CF2. Acceptarea conexiunilor si thread pool

* **CF2.1** Thread-ul principal apeleaza `accept()` intr-o bucla si pune fd-ul primit in coada de conexiuni. Nu citeste si nu scrie nimic pe socketul clientului.
* **CF2.2** Coada are cel mult 64 de elemente. Accesul se face sub mutex, iar workerii asteapta pe o condition variable cand coada e goala.
* **CF2.3** Daca coada este plina, conexiunea nou acceptata primeste imediat raspuns 503 si este inchisa.
* **CF2.4** Workerii se creeaza o singura data, la pornire. Un worker serveste o conexiune pana la inchiderea ei, apoi revine la coada.

### CF3. Citirea cererii

* **CF3.1** Datele se citesc cu `recv()` intr-un buffer care creste pana cand apare secventa `\r\n\r\n`.
* **CF3.2** Octetii primiti dupa `\r\n\r\n` nu se pierd: fac parte din body sau, daca cererea nu are body, din cererea urmatoare de pe aceeasi conexiune.
* **CF3.3** Lungimea body-ului se stabileste astfel: cu `Content-Length` se citesc exact atatia octeti; cu `Transfer-Encoding: chunked` se citesc chunk-urile pana la cel de dimensiune 0; fara niciunul dintre ele cererea nu are body.
* **CF3.4** Daca header-ele depasesc 16 KB, se raspunde cu 431 si conexiunea se inchide.
* **CF3.5** Daca clientul nu trimite nimic 30 s in timp ce cererea este incompleta, se raspunde cu 408 si conexiunea se inchide.

### CF4. Parsarea cererii

* **CF4.1** Din request line se extrag metoda, targetul si versiunea. Metodele neacceptate (inclusiv `CONNECT`) primesc 501, iar versiunile diferite de HTTP/1.0 si HTTP/1.1 primesc 505.
* **CF4.2** Targetul se asteapta in forma absoluta (`http://host[:port]/cale`). Daca este in forma origin, host-ul se ia din header-ul `Host`. Portul implicit este 80.
* **CF4.3** Header-ele se retin ca perechi nume-valoare, in ordinea in care au venit. Cautarea dupa nume nu tine cont de majuscule.
* **CF4.4** Se raspunde cu 400 cand request line-ul nu are trei parti, cand un header nu contine `:`, cand `Content-Length` nu este un numar valid sau cand apar si `Content-Length`, si `Transfer-Encoding`.
* **CF4.5** Daca `Content-Length` depaseste 50 MB, se raspunde cu 413.

### CF5. Conectarea la server

* **CF5.1** Hostname-ul serverului se rezolva cu `getaddrinfo()` (IPv4).
* **CF5.2** Se creeaza un socket TCP nou si se apeleaza `connect()`, cu timeout de 10 s.
* **CF5.3** Daca hostname-ul nu poate fi rezolvat sau conexiunea este refuzata, clientul primeste 502. Daca se depaseste timeout-ul, primeste 504. In ambele cazuri socketul spre server se inchide.
* **CF5.4** Conexiunea client-proxy si conexiunea proxy-server sunt independente: se citeste de pe una si se scrie pe cealalta, fara ca ele sa se amestece.

### CF6. Trimiterea cererii

* **CF6.1** Fara upstream, request line-ul se rescrie in forma origin (`GET /cale HTTP/1.1`). Header-ul `Host` se pastreaza, iar daca lipseste se completeaza din tinta.
* **CF6.2** Header-ele hop-by-hop din cerere se sterg. Spre server se adauga `Connection: close`.
* **CF6.3** Cererea se trimite cu `send()` intr-o bucla, pana la ultimul octet.
* **CF6.4** Body-ul cererii se retransmite pe masura ce este citit, in blocuri de 8 KB, fara a fi tinut integral in memorie.

### CF7. Citirea si trimiterea raspunsului

* **CF7.1** Proxy-ul citeste de pe conexiunea cu serverul status line-ul si header-ele raspunsului (limita de 16 KB) si retine codul de stare.
* **CF7.2** Lungimea body-ului raspunsului se stabileste dupa regulile din 2.3: fara body pentru `HEAD`, 1xx, 204 si 304; altfel `Content-Length`, chunked sau citire pana la inchiderea conexiunii de catre server.
* **CF7.3** Cand nu se aplica nicio regula de modificare, raspunsul este retransmis neschimbat, pe masura ce soseste, fara a fi stocat integral.
* **CF7.4** Daca clientul inchide conexiunea in timpul transferului, proxy-ul inceteaza sa citeasca de la server si inchide ambele socket-uri.

### CF8. Interfata grafica si Interceptarea traficului

* **CF8.1** Proxy-ul ofera o interfata grafica dedicata, prin care utilizatorul poate monitoriza si prelucra traficul HTTP in tranzit, functionalitate asemanatoare cu Burp Suite.
* **CF8.2** Interfata ofera un control de tip buton pentru comutarea interceptarii ```Toggle Intercept ON/OFF```. Cand Intercept este ON, orice cerere venita de la client este retinuta in proxy, iar interfata o expune in modul de editare. Aici utilizatorul poate interveni asupra metodei, uri-ului, header-elor sau body-ului. Requestul pleaca spre server doar dupa confirmarea explicita a utilizatorului (buton de ```Forward```). In caz contrar, requestul poate fi distrus/anulat (buton de ```Drop```).
* **CF8.3** In modul de editare al requestului, putem edita liber campurile requestului. Cand dam forward, bufferului in care a fost salvat requestul interceptat va fi suprascris cu datele editate, care va fi apoi impachetat in formatul primit(application/json, text/html, etc.). De asemenea, putem avea si reguli de modificare automata, adica o lista de reguli generale: schimbarea protocolului http, schimbarea tipului de content, schimbarea unui cuvant la toate aparitiile sale, etc.
* **CF8.4** Interfata detine o vizualizare denumita **View HTTP History**, unde toate tranzactiile traficului sunt logate. Selectand o anumita inregistrare in istoric, interfata va afisa direct request-ul si, alaturat (intr-o vedere side-by-side sau tab-uri paralele), response-ul corespunzator generat de catre server.
* **CF8.5** Daca Intercept este OFF, request-urile nu mai raman blocate in modul de editare, fiind trimise fluid si nealterat catre server. Cu toate acestea, ele (si raspunsurile aferente) vor continua sa fie centralizate si vizibile in modulul HTTP History.


### CF9. Conexiuni persistente

* **CF9.1** Pe conexiunea cu clientul, HTTP/1.1 este persistent implicit, iar HTTP/1.0 doar daca clientul trimite `Connection: keep-alive`. Dupa un raspuns complet proxy-ul asteapta urmatoarea cerere cel mult 15 s.
* **CF9.2** Conexiunea cu clientul se inchide cand clientul trimite `Connection: close`, cand expira timeout-ul de inactivitate, cand apare o eroare sau cand raspunsul nu are lungime cunoscuta (CF7.2).
* **CF9.3** Daca in buffer au ramas date dupa o cerere completa (cerere trimisa inainte ca raspunsul sa fie gata), ele sunt tratate ca inceput al cererii urmatoare.
* **CF9.4** Fiecare cerere este procesata separat, chiar daca vine pe aceeasi conexiune.

### CF10. Upstream proxy

* **CF10.1** Upstream-ul este activ cand `upstream_enabled=yes` sau cand se da `--upstream host:port`. Poate fi oprit din aceleasi locuri.
* **CF10.2** Cand este activ, `connect()` se face catre upstream, iar cererea se trimite in forma absoluta (`GET http://host/cale HTTP/1.1`), cu header-ele procesate ca la CF6.2.
* **CF10.3** Daca upstream-ul nu poate fi contactat, clientul primeste 502, iar in log se scrie adresa upstream-ului.
* **CF10.4** Cu upstream dezactivat, proxy-ul merge direct la server.

### CF11. Tratarea erorilor

* **CF11.1** Valoarea returnata de fiecare apel de socket este verificata. O eroare pe o conexiune duce la inchiderea acelei conexiuni si la eliberarea fd-urilor, nu la oprirea proxy-ului.
* **CF11.2** Daca serverul inchide conexiunea inainte de sfarsitul raspunsului, proxy-ul inchide si conexiunea cu clientul, ca acesta sa nu astepte la nesfarsit.
* **CF11.3** Cand proxy-ul trebuie sa raspunda el insusi clientului, foloseste urmatoarele coduri:


| Situatie                                  | Cod |
| ----------------------------------------- | --- |
| Cerere malformata                         | 400 |
| Cerere incompleta dupa 30 s               | 408 |
| Body prea mare (`Content-Length` > 50 MB) | 413 |
| Header-e prea mari                        | 431 |
| Metoda neacceptata sau`CONNECT`           | 501 |
| Versiune HTTP neacceptata                 | 505 |
| Server sau upstream inaccesibil           | 502 |
| Timeout la server                         | 504 |
| Coada de conexiuni plina                  | 503 |

Raspunsurile de eroare au un body scurt cu textul erorii si `Connection: close`.

### CF12. Logging

* **CF12.1** Pentru fiecare cerere terminata se scrie in `proxy.log` o linie cu data, metoda, host-ul, calea, codul de stare si durata procesarii:

```
2026-10-08 14:32:11 GET example.com /index.html 200 43ms
```

* **CF12.2** Erorile se scriu in acelasi fisier, cu prefixul `ERR`, ca sa se poata separa de cererile reusite:

```
2026-10-08 14:32:15 ERR connect example.com:80 timeout
```

* **CF12.3** Fisierul se deschide in mod append. Scrierea o face un singur thread la un moment dat, sub mutex, iar fiecare linie se scrie dintr-o data, ca liniile de la workeri diferiti sa nu se amestece.

### CF13. Oprirea aplicatiei

* **CF13.1** La SIGINT sau SIGTERM proxy-ul nu mai accepta conexiuni noi.
* **CF13.2** Workerii isi termina cererea in curs (cel mult 5 s), apoi se opresc. Conexiunile ramase in coada se inchid.
* **CF13.3** Thread-ul principal face `pthread_join` pe toti workerii, inchide socketul de ascultare si fisierul de log si iese cu codul 0.

## *3.2 Cerinte nefunctionale*

### CNF1. Performanta

* **CNF1.1** O cerere GET fara reguli, catre un server pe aceeasi masina, trebuie sa dureze cu mai putin de 10 ms mai mult prin proxy decat direct (masurat cu `curl -w "%{time_total}"`).
* **CNF1.2** Proxy-ul trebuie sa serveasca cel putin 8 clienti simultani, cu configuratia implicita, fara ca unul sa-i astepte pe ceilalti.
* **CNF1.3** In afara regulilor de modificare, memoria folosita pentru o conexiune ramane sub 100 KB (buffere pentru header-e si blocuri de 8 KB), indiferent de dimensiunea transferului.

### CNF2. Concurenta si scalabilitate

* **CNF2.1** Numarul de workeri se schimba din configuratie, fara recompilare.
* **CNF2.2** Coada de conexiuni si fisierul de log sunt protejate cu mutex-uri. Rularea cu ThreadSanitizer, sub incarcare, nu trebuie sa raporteze data race-uri.
* **CNF2.3** Un client lent nu blocheaza ceilalti clienti atat timp cat mai exista workeri liberi. Daca toti workerii sunt ocupati cu clienti lenti, conexiunile noi asteapta in coada, iar timeout-urile din 2.3 limiteaza cat dureaza asta.

### CNF3. Fiabilitate

* **CNF3.1** O eroare pe o conexiune (reset, timeout, inchidere neasteptata) nu opreste procesul si nu afecteaza celelalte conexiuni.
* **CNF3.2** Toate fd-urile se inchid dupa folosire. Dupa 1000 de cereri consecutive, numarul de intrari din `/proc/<pid>/fd` trebuie sa fie acelasi ca inainte de ele.
* **CNF3.3** Rularea sub `valgrind` nu trebuie sa arate pierderi de memorie in scenariile din CNF8.

### CNF4. Securitate

* **CNF4.1** Proxy-ul asculta implicit doar pe `127.0.0.1`. Daca este setata o alta adresa (de exemplu `0.0.0.0`), la pornire se afiseaza un avertisment ca proxy-ul poate fi folosit din afara masinii.
* **CNF4.2** Dimensiunile din cerere (header-e, `Content-Length`, dimensiunea chunk-urilor) sunt verificate inainte de a se aloca sau copia ceva, iar limitele din 2.3 se aplica intotdeauna.
* **CNF4.3** Codul nu foloseste functii fara verificarea dimensiunii (`strcpy`, `sprintf`, `gets`). Se folosesc `std::string`, `std::vector` si `snprintf`.
* **CNF4.4** Body-urile si header-ele `Cookie` si `Authorization` nu se scriu in log.

### CNF5. Portabilitate

* **CNF5.1** Codul se compileaza cu g++ 11 sau mai nou (C++17) pe o distributie Linux cu glibc si pthreads, cu o singura comanda `make`.
* **CNF5.2** Se folosesc doar interfete POSIX pentru retea si sincronizare.
* **CNF5.3** Adresa, portul, numarul de workeri si upstream-ul se schimba fara a modifica sursele.

### CNF6. Mentenabilitate

* **CNF6.1** Codul respecta impartirea pe componente din 2.1 (`config`, `server`, `pool`, `http`, `forward`, `log`).
* **CNF6.2** Componenta `http` lucreaza pe buffere de memorie, nu pe socket-uri, deci poate fi testata fara retea.
* **CNF6.3** Logica de thread pool nu contine cod specific HTTP.
* **CNF6.4** Formatul liniilor de log se poate schimba doar in componenta `log`.

### CNF7. Utilizabilitate

* **CNF7.1** Configurarea se face doar din `proxy.conf`, `rules.conf` si argumentele din linia de comanda.
* **CNF7.2** Mesajele de eroare spun ce operatie a esuat si pe ce adresa sau fd (de exemplu `bind 127.0.0.1:8080: Address already in use`).
* **CNF7.3** `--help` afiseaza lista optiunilor si valorile implicite.

### CNF8. Testabilitate

Proxy-ul trebuie sa poata fi testat fara browser. Scenariile minime sunt:


| Scenariu                      | Cum se verifica                                                                                                                                                                                                                                        |
| ----------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| GET simplu                    | `curl -x http://127.0.0.1:8080 http://localhost:9000/` catre `python3 -m http.server 9000`                                                                                                                                                             |
| POST cu body                  | `curl -x ... -d "a=b" ...`, continutul ajunge intact la server                                                                                                                                                                                         |
| Raspuns mare (10 MB)          | fisierul descarcat prin proxy are acelasi`md5sum` cu originalul                                                                                                                                                                                        |
| Raspuns chunked               | server de test care trimite chunked; raspunsul ajunge complet la client                                                                                                                                                                                |
| Conexiune persistenta         | `curl -v` cu doua URL-uri in aceeasi comanda; a doua cerere reutilizeaza conexiunea                                                                                                                                                                    |
| Cerere fragmentata            | cerere scrisa cu`nc`, trimisa cate cateva caractere cu pauze; proxy-ul o asambleaza corect                                                                                                                                                             |
| Clienti simultani             | 20 de procese`curl` in paralel (`xargs -P 20`), toate primesc raspuns                                                                                                                                                                                  |
| Coada plina                   | mai multe conexiuni decat incap in coada; cele in plus primesc 503                                                                                                                                                                                     |
| Client care inchide la mijloc | `curl --max-time 1` pe un fisier mare; proxy-ul ramane functional, iar fd-urile se elibereaza                                                                                                                                                          |
| Server care inchide la mijloc | server de test cu`nc` care inchide dupa jumatate din raspuns; clientul nu ramane blocat                                                                                                                                                                |
| Server inexistent             | cerere catre un host care nu exista; clientul primeste 502                                                                                                                                                                                             |
| Interceptare si Editare       | Cu Intercept ON in interfata, cererea ramane in asteptare.<br />Modificarea cererii<br /> manual si apasarea Forward duce la receptionarea cererii alterate pe serverul de test.<br />Perechea cerere-raspuns este documentata corect in HTTP History |
| Proxy chaining                | doua instante (8080 spre 8081 spre server); cererea apare in log-urile ambelor instante                                                                                                                                                                |
