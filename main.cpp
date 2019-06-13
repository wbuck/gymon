#include "server.h"
#include "connection.h"
#include <iostream>
#include <cstdio>
#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <mutex>
#include <thread>
#include <vector>
#include <ctype.h>
#include <sys/time.h>
#include <fcntl.h>
#include <future>
#include <memory>


//static constexpr char const* _name{ "gymon" };
//static bool _recvsig{ false };
//static std::mutex _mutex;

//static void signal_handler( int32_t signum, siginfo_t* info, void* ptr )
//{
//	if( signum == SIGTERM )
//	{
//		std::scoped_lock<std::mutex> lock{ _mutex };
//		syslog( LOG_NOTICE, "%s SIGTERM %d received", _name, signum );
//		_recvsig = true;
//	}	
//}

#define PORT "32001"   // port we're listening on

// get sockaddr, IPv4 or IPv6:
void* get_in_addr( struct sockaddr* sa )
{
	if( sa->sa_family == AF_INET ) {
		return &( ( ( struct sockaddr_in* )sa )->sin_addr );
	}

	return &( ( ( struct sockaddr_in6* )sa )->sin6_addr );
}

static void tcp_server_async( )
{
	fd_set master;    // master file descriptor list
	fd_set read_fds;  // temp file descriptor list for select()
	int32_t fdmax;        // maximum file descriptor number

	int listener;     // listening socket descriptor
	struct sockaddr_storage remoteaddr; // client address

	char remoteIP[ INET6_ADDRSTRLEN ];      

	struct addrinfo* addresses;

	FD_ZERO( &master );    // clear the master and temp sets
	FD_ZERO( &read_fds );

	// Set up the non blocking socket 
	// address criteria.
	struct addrinfo criteria { 0 };
	criteria.ai_family	 = AF_UNSPEC;
	criteria.ai_socktype = SOCK_STREAM;
	criteria.ai_flags	 = AI_PASSIVE;

	while( true )
	{
		// Attempt to get a list of addresses that match
		// our criteria.
		if( int32_t ec{ getaddrinfo( nullptr, PORT, &criteria, &addresses ) }; ec < 0 )
		{
			std::cerr << "Server error: " << gai_strerror( ec ) << '\n';
			if( ec == EAI_AGAIN ) 
			{
				sleep( 5 );
				continue;
			}
			exit( 1 );
		}
		break;
	}	

	struct addrinfo* address{ nullptr };

	for( address = addresses; address != nullptr; address = address->ai_next ) 
	{
		if( listener = socket( address->ai_family, 
			address->ai_socktype, address->ai_protocol ); listener < 0 )
			continue;

		// Set the socket option to reuse the address. This
		// prevents the 'address in use' error.
		static constexpr int32_t reuse{ 1 };
		if( setsockopt( listener, SOL_SOCKET, 
			SO_REUSEADDR, &reuse, sizeof( int32_t ) ) < 0 )
		{
			perror( "Failed to set socket options" );
			close( listener );
			continue;
		}

		if( bind( listener, address->ai_addr, address->ai_addrlen ) < 0 ) 
		{
			perror( "Failed to bind socket" );
			close( listener );
			continue;
		}
		break;
	}

	// If address is null it means we failed
	// to bind the socket.
	if( address == nullptr ) 
	{
		std::cerr << "Failed to bind socket\n";
		exit( 2 );
	}

	freeaddrinfo( addresses ); // all done with this

	// listen
	if( listen( listener, 10 ) < 0 ) 
	{
		perror( "Listening for incoming connections failed." );
		exit( 3 );
	}

	// add the listener to the master set
	FD_SET( listener, &master );

	// keep track of the biggest file descriptor
	fdmax = listener; // so far, it's this one

	using con = gymon::connection<1024>;
	std::vector<con> connections;
	// main loop
	while( true )
	{
		read_fds = master; // copy it
		if( select( fdmax + 1, &read_fds, NULL, NULL, NULL ) < 0 ) 
		{
			perror( "Select failed" );
			exit( 4 );
		}
		// New connection.
		if( FD_ISSET( listener, &read_fds ) )
		{
			socklen_t addrlen{ sizeof( remoteaddr ) };
			// Accept the incoming connection.
			if( int32_t socket{ accept( listener, 
				reinterpret_cast<struct sockaddr*>( &remoteaddr ), &addrlen ) }; socket < 0 )
			{
				perror( "Error accepting new connection" );
				if( errno == EAGAIN ) continue;
				else exit( EXIT_FAILURE );
			}
			else
			{
				// Add to master set.
				FD_SET( socket, &master );
				// keep track of the max.
				if( socket > fdmax )
					fdmax = socket;

				connections.emplace_back( socket );
				auto addr{ inet_ntop( remoteaddr.ss_family,
					get_in_addr( reinterpret_cast<struct sockaddr*>( &remoteaddr ) ),
					remoteIP, INET6_ADDRSTRLEN ) };
				std::cout << "New connection from " << addr << " on socket " << socket << '\n';				
			}
		}
		// Loop through the current connections to see
		// if anyone has sent data.
		for( auto it{ std::begin( connections ) }; it != std::end( connections ); )
		{
			con& client{ *it };
			if( int32_t socket{ client.getsocket( ) }; FD_ISSET( socket, &read_fds ) )
			{
				if( !client.handlereq( ) )
				{
					std::cout << "Removing connection " << socket << '\n';
					// Close the socket and remove it from the
					// master set.
					close( socket );
					FD_CLR( socket, &master );
					it = connections.erase( it );
					continue;
				}
			}
			++it;
		}
	}
}

int main( )
{
	gymon::server server;
	std::future<void> future{ server.listen( "32001" ) };
	std::cout << "Server started\n";
	future.wait( );
	return 0;
	//gymon::daemon::daemonize( signal_handler );
	//// Open the log file.
	//openlog( _name, LOG_PID, LOG_DAEMON );
	//
	//// Daemon-specific initialization goes here.
	//while( true )
	//{
	//	syslog( LOG_NOTICE, "%s started.", _name );
	//	sleep( 60 );
	//	break;
	//}
	//syslog( LOG_NOTICE, "%s terminated.", _name );
	//closelog( );
	//return EXIT_SUCCESS;
	//return 0;
}