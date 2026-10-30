#!/bin/bash
# teste de retea pentru ce e gata acum (CNF8), se ruleaza pe Linux din radacina proiectului #R
# forwarding-ul nu e facut inca, asa ca cererile valide primesc 501 cu cererea parsata #R
PROXY=http://127.0.0.1:8080 #R

make || exit 1 #R
./proxy_server --port 8080 --workers 4 & #R
PID=$! #R
sleep 1 #R

echo "== GET simplu, forma absoluta (asteptam 501 + cererea parsata)" #R
curl -s -i -x $PROXY http://localhost:9000/index.html #R

echo "== POST cu body (Body: 3 bytes)" #R
curl -s -x $PROXY -d "a=b" http://localhost:9000/form #R

echo "== POST chunked (Body: 11 bytes (chunked))" #R
curl -s -x $PROXY -H "Transfer-Encoding: chunked" --data-binary "hello world" http://localhost:9000/up #R

echo "== CONNECT / HTTPS (asteptam 501)" #R
curl -s -o /dev/null -w "%{http_connect}\n" -x $PROXY https://example.com/ #R

echo "== header-e peste 16 KB (asteptam 431)" #R
curl -s -o /dev/null -w "%{http_code}\n" -x $PROXY -H "X-Big: $(head -c 17000 /dev/zero | tr '\0' a)" http://localhost:9000/ #R

echo "== cerere malformata (asteptam 400)" #R
printf 'GET /\r\n\r\n' | nc -q 2 127.0.0.1 8080 | head -1 #R

echo "== cerere fragmentata (asteptam 501 = parsata corect)" #R
(printf 'GET http://a.com/ HT'; sleep 1; printf 'TP/1.1\r\nHo'; sleep 1; printf 'st: a.com\r\n\r\n') | nc -q 2 127.0.0.1 8080 | head -1 #R

echo "== 20 clienti simultani (asteptam 20 de 501)" #R
seq 20 | xargs -P 20 -I{} curl -s -o /dev/null -w "%{http_code}\n" -x $PROXY http://localhost:9000/{} | sort | uniq -c #R

echo "== log" #R
tail -n 5 proxy.log #R

#$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ #R
#$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ #R

#TODO verificari cu un server local, dupa CF5-CF7 (scenariile din CNF8) #R
#pornim serverul de test: python3 -m http.server 9000 & #R
#GET: curl -x $PROXY http://localhost:9000/ => 200 si acelasi continut ca fara proxy #R
#POST cu body: curl -x $PROXY -d "a=b" ... => continutul ajunge intact la server #R
#  http.server nu accepta POST (raspunde 501), trebuie un server de test care afiseaza body-ul #R
#raspuns mare: fisier de 10 MB descarcat prin proxy => acelasi md5sum cu originalul #R
#server inaccesibil: curl -x $PROXY http://nu-exista.invalid/ => 502 #R
#cand e gata, verificarile cu 501 de mai sus se schimba in codurile reale #R
# #R
#$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ #R
#$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ #R

kill -INT $PID #R
wait $PID #R
echo "proxy oprit cu codul $?" #R
