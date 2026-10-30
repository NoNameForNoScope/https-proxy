#ifndef THREADPOOL_HPP //R
#define THREADPOOL_HPP //R

#include <deque> //R
#include <functional> //R
#include <pthread.h> //R
#include <vector> //R

// CF2: coada de conexiuni + numar fix de workeri //R
// nu stie nimic de HTTP (CNF6.3), primeste doar fd-uri si apeleaza handler-ul //R
class ThreadPool //R
{ //R
public: //R
    // handler-ul primeste fd-ul clientului; pool-ul il inchide dupa ce handler-ul se termina //R
    // asa stim mereu ce fd-uri sunt inca deschise, pentru oprirea in 5 s (CF13.2) //R
    using Handler = std::function<void(int)>; //R

    ThreadPool(int workers, size_t queue_capacity, Handler handler); //R
    ~ThreadPool(); //R
    ThreadPool(const ThreadPool &) = delete; //R
    ThreadPool &operator=(const ThreadPool &) = delete; //R

    // CF2.4: workerii se creeaza o singura data, la pornire //R
    bool Start(); //R

    // CF2.3: false daca coada e plina (sau pool-ul se opreste), apelantul raspunde cu 503 //R
    bool TrySubmit(int fd); //R

    // CF13.2: workerii au cel mult grace_sec secunde sa termine cererea in curs, //R
    // apoi conexiunile lor se inchid fortat; fd-urile ramase in coada se inchid //R
    // CF13.3: pthread_join pe toti workerii //R
    void Shutdown(int grace_sec = 5); //R

private: //R
    static void *WorkerMain(void *arg); //R
    void WorkerLoop(); //R

    int _worker_count; //R
    size_t _capacity; //R
    Handler _handler; //R

    std::deque<int> _queue; //R
    std::vector<int> _active;   // fd-urile servite acum de workeri //R
    std::vector<pthread_t> _threads; //R
    bool _stopping = false; //R

    // CF2.2: coada se acceseaza doar sub mutex, workerii dorm pe condition variable //R
    pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER; //R
    pthread_cond_t _not_empty = PTHREAD_COND_INITIALIZER; //R
    // semnalat cand un worker termina o conexiune (folosit la oprire) //R
    pthread_cond_t _conn_done = PTHREAD_COND_INITIALIZER; //R
}; //R

#endif //R
