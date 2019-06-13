#pragma once
#include "sockresult.h"
#include <memory>
#include <sys/select.h>
#include <netdb.h>
#include <type_traits>
#include <string>
#include <pthread.h>
#include <future>
#include <string_view>

namespace gymon
{
    template<auto fn>
    using deleter_from_fn = std::integral_constant<decltype( fn ), fn>;

    template<typename T, auto fn>
    using custom_ptr = std::unique_ptr<T, deleter_from_fn<fn>>;

    class server
    {
    public:

        std::future<void> listen( std::string_view port ) noexcept;
    private:

        // Attempts to get a list of addresses that match
        // the supplied criteria.
        custom_ptr<struct addrinfo, freeaddrinfo> getaddrs( std::string_view port ) const noexcept;
        
        sockresult bindaddr( custom_ptr<struct addrinfo, freeaddrinfo> addresses ) noexcept;
    private:

        // File descriptor used for listening for 
        // incoming connections.
        int32_t _listener_fd;      
    };
}