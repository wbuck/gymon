#pragma once
#include "connection.h"
#include "sockresult.h"
#include <memory>
#include <sys/select.h>
#include <netdb.h>
#include <type_traits>
#include <string>
#include <pthread.h>
#include <future>
#include <string_view>
#include <optional>
#include <spdlog/spdlog.h>
#include <optional>
#include <vector>

namespace gymon
{
    template<auto fn>
    using deleter_from_fn = std::integral_constant<decltype( fn ), fn>;

    template<typename T, auto fn>
    using custom_ptr = std::unique_ptr<T, deleter_from_fn<fn>>;

    class server
    {
    public:
        server( ) noexcept : _logger{ spdlog::get( "gymon" ) } { }

        std::future<void> listen( std::string port,
             std::optional<int> sigfd = std::nullopt ) noexcept;
    private:

        // Handles system signals by logging a message
        // and closing all open connections.
        void handlesig( int32_t sigfd ) const noexcept;

        // Attempts to get a list of addresses that match
        // the supplied criteria.
        custom_ptr<struct addrinfo, freeaddrinfo> getaddrs( std::string const& port ) const noexcept;
        
        sockresult bindaddr( custom_ptr<struct addrinfo, freeaddrinfo> addresses ) noexcept;
    private:

        // File descriptor used for listening for 
        // incoming connections.
        int32_t _listener_fd;
        // List of open connections.
        std::vector<connection<256>> _clients;
        std::shared_ptr<spdlog::logger> _logger;
    };
}