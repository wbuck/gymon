#include "server.h"
#include "connection.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <vector>
#include <array>
#include <arpa/inet.h>
#include <sys/select.h>


namespace gymon
{    
    std::future<void> server::listen( std::string port, int sigfd ) noexcept
    {
        return std::async( std::launch::async, 
            [ this, port = std::move( port ), sigfd = sigfd ] 
        {
            // Master and temp file descriptor lists.
            fd_set masterfds, tempfds;
            FD_ZERO( &masterfds );
            FD_ZERO( &tempfds ); 

            FD_SET( sigfd, &masterfds );

            // Keeps track of the highest file descriptor.
	        int32_t fdmax{ sigfd };           

            // Will hold the clients IP address.
            struct sockaddr_storage remoteaddr;
            std::array<char, INET_ADDRSTRLEN> remoteip;

            // Helper method for getting either the IPV4
            // or IPV6 address of the remote.
            auto const get_in_addr = [ ]( struct sockaddr* sa ) -> void*
            {
                if( sa->sa_family == AF_INET )
                    return &( ( ( struct sockaddr_in* )sa )->sin_addr );

                return &( ( ( struct sockaddr_in6* )sa )->sin6_addr );
            };

            // Get a list of available addresses to
            // bind to.
            if( auto addresses{ getaddrs( port ) }; !addresses )
                return;
            else
            {
                if( bindaddr( std::move( addresses ) ) == sockresult::error )
                    return;
            }
            
            // Start listening on the given file descriptor.
            if( ::listen( _listener_fd, 10 ) < 0 )
            {
                if( _logger )
                {
                    _logger->critical( 
                        "Failed to listen for incoming connections: {0}", strerror( errno ) );
                }
                return;
            }

            // Add the listener to the master set.
	        FD_SET( _listener_fd, &masterfds );
            if( _listener_fd > fdmax )
	            fdmax = _listener_fd;

            // List of open connections.
            std::vector<connection<1024>> clients;
            
            while( true )
            {
                // Make a copy of the master set in order to
                // reset it after handling a request.
                tempfds = masterfds;
                if( select( fdmax + 1, &tempfds, nullptr, nullptr, nullptr ) < 0 ) 
		        {
                    if( errno == EINTR )
                    {
                        if( _logger )
                            _logger->warn( "Received SIGTERM signal: {0}", strerror( errno ) );
                        return;
                    }
		        	if( _logger )
                    {
                        _logger->critical( 
                            "Failed to wait asynchronously for network events: {0}", strerror( errno ) );
                    }
		        	return;
		        }
                if( FD_ISSET( sigfd, &tempfds ) )
                {
                    if( _logger )
                        _logger->warn( "SIGTERM received, exiting daemon" );
                    return;
                }

                // Check if the request is a new connection.
                else if( FD_ISSET( _listener_fd, &tempfds ) )
                {
                    socklen_t addrlen{ sizeof( remoteaddr ) };
                    // Accept the incoming connection.
			        if( int32_t socket{ accept( _listener_fd, 
				        reinterpret_cast<struct sockaddr*>( &remoteaddr ), &addrlen ) }; socket < 0 )
			        {
			        	if( errno == EAGAIN )
                        {
                            if( _logger )
                                _logger->warn( "Failed to accept new connection: {0}", strerror( errno ) );
                            continue;
                        }
			        	else
                        {
                            if( _logger )
                                _logger->critical( "Failed to accept new connection: {0}", strerror( errno ) );
                            return;
                        }
			        }
                    else
                    {
                        // Add new connection to master set.
                        FD_SET( socket, &masterfds );

                        // Keep track of the highest file descriptor.
                        if( socket > fdmax )
                            fdmax = socket;
                        
                        // Get the remote IP address.
                        std::string address{ inet_ntop( remoteaddr.ss_family,
                            get_in_addr( reinterpret_cast<struct sockaddr*>( &remoteaddr ) ),
                            remoteip.data( ), remoteip.size( ) ) };

                        // Add the new connection to our list of active
                        // connections.
                        clients.emplace_back( socket, address );                                                                                                                     
                        if( _logger )
                            _logger->debug( "Accepted new connection {0}:{1}", address, socket );
                    }                    
                }
                // Loop through the current connections to see
		        // if anyone has sent data.
		        for( auto it{ std::begin( clients ) }; it != std::end( clients ); )
		        {
		        	if( int32_t socket{ it->getsocket( ) }; FD_ISSET( socket, &tempfds ) )
		        	{
		        		if( !it->handlereq( ) )
		        		{
		        			// Close the socket and remove it from the
		        			// master set.
		        			close( socket );
		        			FD_CLR( socket, &masterfds );
		        			it = clients.erase( it );
		        			continue;
		        		}
		        	}
		        	++it;
		        }
            }
        } );
    }

    custom_ptr<struct addrinfo, freeaddrinfo> server::getaddrs( 
        std::string const& port ) const noexcept
    {
        // If successful this will contain a linked list
        // of possible addresses to use.
        struct addrinfo* addresses{ nullptr };

        // Set up the non blocking socket 
	    // address criteria.
	    struct addrinfo criteria { 0 };
	    criteria.ai_family	 = AF_UNSPEC;
	    criteria.ai_socktype = SOCK_STREAM;
	    criteria.ai_flags	 = AI_PASSIVE;

        while( true )
        {
            if( int32_t ec{ getaddrinfo( nullptr, port.c_str( ), 
                &criteria, &addresses ) }; ec < 0 )
		    {
			    if( ec == EAI_AGAIN ) 
			    {
                    if( _logger )
                        _logger->warn( "Failed to attain list of addresses: {0}", gai_strerror( ec ) );

			    	sleep( 5 );
			    	continue;
			    }

                if( _logger )
                    _logger->critical( "Failed to attain list of addresses: {0}", gai_strerror( ec ) );

                // We failed to attain a list of addresses
                // so return an empty pointer.
			    return custom_ptr<struct addrinfo, freeaddrinfo>{ };
		    }
		    return custom_ptr<struct addrinfo, freeaddrinfo>{ addresses };
        }
    }

    sockresult server::bindaddr( 
        custom_ptr<struct addrinfo, freeaddrinfo> addresses ) noexcept
    {
        struct addrinfo* address{ nullptr };

        for( address = addresses.get( ); 
             address != nullptr; address = address->ai_next )
        {
            if( _listener_fd = socket( address->ai_family, 
                address->ai_socktype, address->ai_protocol ); _listener_fd < 0 )
            {
                if( _logger )
                    _logger->critical( "Failed to create endpoint: {0}", strerror( errno ) );

                return sockresult::error;
            }

            // Set the socket option to reuse the address. This
            // precents the 'address in use' error.
            static constexpr int32_t reuse{ 1 };
            if( setsockopt( _listener_fd, SOL_SOCKET,
                SO_REUSEADDR, &reuse, sizeof( int32_t ) ) < 0 )
            {
                if( _logger )
                    _logger->error( "Failed to set socket options: {0}", strerror( errno ) );

			    close( _listener_fd );
			    continue;
            }

            if( bind( _listener_fd, address->ai_addr, address->ai_addrlen ) < 0 ) 
		    {
                if( _logger )
                    _logger->error( "Failed to bind socket: {0}", strerror( errno ) );

			    close( _listener_fd );
			    continue;
		    }            
		    break;
        }
        // If address is null it means we failed
	    // to bind the socket.
	    if( address == nullptr ) 
	    {
		    if( _logger )
                _logger->critical( "Failed to bind socket: {0}", strerror( errno ) );

		    return sockresult::error;
	    }
        return sockresult::success;
    }
}