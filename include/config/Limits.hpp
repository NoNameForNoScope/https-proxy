#ifndef LIMITS_HPP //R
#define LIMITS_HPP //R

#include <cstddef> //R

// valorile implicite si limitele din DCS 2.3 //R
// sunt fixe, nu se schimba din proxy.conf //R
namespace limits //R
{ //R
    constexpr size_t kQueueSize = 64;                      // coada de conexiuni //R
    constexpr size_t kMaxHeaderSize = 16 * 1024;           // header-e + request line => 431 //R
    constexpr size_t kMaxBodySize = 50 * 1024 * 1024;      // body cerere => 413 //R
    constexpr size_t kMaxRuleResponseSize = 5 * 1024 * 1024; // raspuns pe care se aplica reguli //R
    constexpr int kConnectTimeoutSec = 10;                 // connect() //R
    constexpr int kIoTimeoutSec = 30;                      // recv() / send() => 408 //R
    constexpr int kKeepAliveTimeoutSec = 15;               // conexiune inactiva //R
    constexpr size_t kBlockSize = 8 * 1024;                // blocul de transfer //R
    constexpr int kMinWorkers = 1; //R
    constexpr int kMaxWorkers = 64; //R
} //R

#endif //R
