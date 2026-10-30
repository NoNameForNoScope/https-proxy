#include "pool/ThreadPool.hpp" //R

#include <algorithm> //R
#include <cerrno> //R
#include <csignal> //R
#include <cstdio> //R
#include <cstring> //R
#include <ctime> //R
#include <sys/socket.h> //R
#include <unistd.h> //R
#include <utility> //R

ThreadPool::ThreadPool(int workers, size_t queue_capacity, Handler handler) //R
    : _worker_count(workers), _capacity(queue_capacity), _handler(std::move(handler)) //R
{ //R
} //R

ThreadPool::~ThreadPool() //R
{ //R
    Shutdown(); //R
    pthread_cond_destroy(&_not_empty); //R
    pthread_cond_destroy(&_conn_done); //R
    pthread_mutex_destroy(&_mutex); //R
} //R

bool ThreadPool::Start() //R
{ //R
    // blocam SIGINT/SIGTERM in workeri, ca semnalul sa ajunga la thread-ul principal (signal(7)) //R
    // thread-urile noi mostenesc masca de semnale a celui care le creeaza //R
    sigset_t block, old; //R
    sigemptyset(&block); //R
    sigaddset(&block, SIGINT); //R
    sigaddset(&block, SIGTERM); //R
    pthread_sigmask(SIG_BLOCK, &block, &old); //R

    bool ok = true; //R
    for(int i = 0; i < _worker_count; i++) //R
    { //R
        pthread_t tid; //R
        int err = pthread_create(&tid, nullptr, &ThreadPool::WorkerMain, this); //R
        if(err != 0) //R
        { //R
            // pthread_create nu seteaza errno, intoarce codul de eroare //R
            fprintf(stderr, "pthread_create worker %d: %s\n", i, strerror(err)); //R
            ok = false; //R
            break; //R
        } //R
        _threads.push_back(tid); //R
    } //R

    // thread-ul principal primeste din nou semnalele //R
    pthread_sigmask(SIG_SETMASK, &old, nullptr); //R
    return ok; //R
} //R

bool ThreadPool::TrySubmit(int fd) //R
{ //R
    pthread_mutex_lock(&_mutex); //R
    if(_stopping || _queue.size() >= _capacity) //R
    { //R
        pthread_mutex_unlock(&_mutex); //R
        return false; //R
    } //R
    _queue.push_back(fd); //R
    // trezim un singur worker, e un singur fd nou //R
    pthread_cond_signal(&_not_empty); //R
    pthread_mutex_unlock(&_mutex); //R
    return true; //R
} //R

void ThreadPool::Shutdown(int grace_sec) //R
{ //R
    pthread_mutex_lock(&_mutex); //R
    _stopping = true; //R
    // trezim toti workerii ca sa vada _stopping //R
    pthread_cond_broadcast(&_not_empty); //R

    // CF13.2: asteptam cel mult grace_sec secunde sa se termine conexiunile in curs //R
    struct timespec deadline; //R
    clock_gettime(CLOCK_REALTIME, &deadline); //R
    deadline.tv_sec += grace_sec; //R
    while(!_active.empty()) //R
    { //R
        // while + verificare: si aici pot exista spurious wakeups //R
        if(pthread_cond_timedwait(&_conn_done, &_mutex, &deadline) == ETIMEDOUT) //R
            break; //R
    } //R
    // dupa termen: shutdown() pe socket face ca recv()/send() din worker sa se intoarca imediat //R
    // fd-ul ramane deschis pana il inchide workerul, deci nu putem nimeri alt fd //R
    for(int fd : _active) //R
        shutdown(fd, SHUT_RDWR); //R
    pthread_mutex_unlock(&_mutex); //R

    for(pthread_t tid : _threads) //R
        pthread_join(tid, nullptr); //R
    _threads.clear(); //R

    // conexiunile care n-au apucat sa fie preluate se inchid (CF13.2) //R
    pthread_mutex_lock(&_mutex); //R
    for(int fd : _queue) //R
        close(fd); //R
    _queue.clear(); //R
    pthread_mutex_unlock(&_mutex); //R
} //R

void *ThreadPool::WorkerMain(void *arg) //R
{ //R
    static_cast<ThreadPool *>(arg)->WorkerLoop(); //R
    return nullptr; //R
} //R

void ThreadPool::WorkerLoop() //R
{ //R
    while(true) //R
    { //R
        pthread_mutex_lock(&_mutex); //R
        // while, nu if: pthread_cond_wait poate sa se intoarca si fara signal (spurious wakeup) //R
        while(_queue.empty() && !_stopping) //R
            pthread_cond_wait(&_not_empty, &_mutex); //R

        if(_stopping) //R
        { //R
            pthread_mutex_unlock(&_mutex); //R
            return; //R
        } //R

        int fd = _queue.front(); //R
        _queue.pop_front(); //R
        _active.push_back(fd); //R
        pthread_mutex_unlock(&_mutex); //R

        // conexiunea se serveste fara mutex, ca ceilalti workeri sa poata lua din coada //R
        _handler(fd); //R

        // scoatem fd-ul din lista si il inchidem sub mutex, ca Shutdown sa nu-l vada inchis pe jumatate //R
        pthread_mutex_lock(&_mutex); //R
        _active.erase(std::find(_active.begin(), _active.end(), fd)); //R
        close(fd); //R
        pthread_cond_signal(&_conn_done); //R
        pthread_mutex_unlock(&_mutex); //R
    } //R
} //R
