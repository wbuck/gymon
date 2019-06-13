#include "server.h"
#include "connection.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <vector>
#include <array>
#include <arpa/inet.h>


namespace gymon
{    
    std::future<void> server::listen( std::string_view port ) noexcept
    {
        return std::async( std::launch::async, [ & ] 
        {
            // Master and temp file descriptor lists.
            fd_set masterfds, tempfds;
            FD_ZERO( &masterfds );
            FD_ZERO( &tempfds );            

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
                perror( "Listening for incoming connections failed." );
                return;
            }

            // Add the listener to the master set.
	        FD_SET( _listener_fd, &masterfds );

            // Keep track of the highest file descriptor
	        int32_t fdmax{ _listener_fd };

            // List of open connections.
            std::vector<connection<1024>> clients;
            
            while( true )
            {
                // Make a copy of the master set in order to
                // reset it after handling a request.
                tempfds = masterfds;
                if( select( fdmax + 1, &tempfds, nullptr, nullptr, nullptr ) < 0 ) 
		        {
		        	perror( "Select failed" );
		        	return;
		        }

                // Check if the request is a new connection.
                if( FD_ISSET( _listener_fd, &tempfds ) )
                {
                    socklen_t addrlen{ sizeof( remoteaddr ) };
                    // Accept the incoming connection.
			        if( int32_t socket{ accept( _listener_fd, 
				        reinterpret_cast<struct sockaddr*>( &remoteaddr ), &addrlen ) }; socket < 0 )
			        {
			        	perror( "Error accepting new connection" );
			        	if( errno == EAGAIN ) continue;
			        	else return;
			        }
                    else
                    {
                        // Add new connection to master set.
                        FD_SET( socket, &masterfds );

                        // Keep track of the highest file descriptor.
                        if( socket > fdmax )
                            fdmax = socket;

                        // Add the new connection to our list of active
                        // connections.
                        clients.emplace_back( socket );
                        
                        // Get the remote IP address.
                        std::string address{ inet_ntop( remoteaddr.ss_family,
                            get_in_addr( reinterpret_cast<struct sockaddr*>( &remoteaddr ) ),
                            remoteip.data( ), remoteip.size( ) ) };
                        
                        std::cout << "New connection from " << address << " on socket " << socket << '\n';
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
		        			std::cout << "Removing connection " << socket << '\n';
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

    custom_ptr<struct addrinfo, freeaddrinfo> server::getaddrs( std::string_view port ) const noexcept
    {
        // If successful this will contain a linked list
        // of possible addresses to use.
        struct addrinfo* addresses;

        // Set up the non blocking socket 
	    // address criteria.
	    struct addrinfo criteria { 0 };
	    criteria.ai_family	 = AF_UNSPEC;
	    criteria.ai_socktype = SOCK_STREAM;
	    criteria.ai_flags	 = AI_PASSIVE;

        while( true )
        {
            if( int32_t ec{ getaddrinfo( nullptr, "32001", 
                &criteria, &addresses ) }; ec < 0 )
		    {
			    std::cerr << "Server error: " << gai_strerror( ec ) << '\n';
			    if( ec == EAI_AGAIN ) 
			    {
			    	sleep( 5 );
			    	continue;
			    }
                // We failed to attain a list of addresses
                // so return an empty pointer.
			    return custom_ptr<struct addrinfo, freeaddrinfo>{ };
		    }
		    return custom_ptr<struct addrinfo, freeaddrinfo>{ addresses };
        }
    }

    sockresult server::bindaddr( custom_ptr<struct addrinfo, freeaddrinfo> addresses ) noexcept
    {
        struct addrinfo* address{ nullptr };

        for( address = addresses.get( ); 
             address != nullptr; address = address->ai_next )
        {
            if( _listener_fd = socket( address->ai_family, 
                address->ai_socktype, address->ai_protocol ); _listener_fd < 0 )
            {
                perror( "Failed to create endpoint" );
                return sockresult::error;
            }

            // Set the socket option to reuse the address. This
            // precents the 'address in use' error.
            static constexpr int32_t reuse{ 1 };
            if( setsockopt( _listener_fd, SOL_SOCKET,
                SO_REUSEADDR, &reuse, sizeof( int32_t ) ) < 0 )
            {
                perror( "Failed to set socket options" );
			    close( _listener_fd );
			    continue;
            }

            if( bind( _listener_fd, address->ai_addr, address->ai_addrlen ) < 0 ) 
		    {
			    perror( "Failed to bind socket" );
			    close( _listener_fd );
			    continue;
		    }            
		    break;
        }
        // If address is null it means we failed
	    // to bind the socket.
	    if( address == nullptr ) 
	    {
		    std::cerr << "Failed to bind socket\n";
		    return sockresult::error;
	    }
        return sockresult::success;
    }
}