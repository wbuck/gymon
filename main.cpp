#include "server.h"
#include "connection.h"
#include <iostream>
#include <future>
#include <vector>
#include <chrono>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async.h>


static std::shared_ptr<spdlog::logger> _logger;
static const std::string _port{ "32001" };
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

static void create_logger( )
{
	spdlog::init_thread_pool( 8192, 1 );
    auto stdout_sink{ std::make_shared<spdlog::sinks::stdout_color_sink_mt>( ) };
	stdout_sink->set_level( spdlog::level::debug );

    auto rotating_sink{ 
		std::make_shared<spdlog::sinks::rotating_file_sink_mt>( "gymonlog.txt", 1024 * 1024 * 10 , 3 ) };
	rotating_sink->set_level( spdlog::level::debug );

    std::vector<spdlog::sink_ptr> sinks { stdout_sink, rotating_sink };
    auto logger = std::make_shared<spdlog::async_logger>( 
		"gymon", std::begin( sinks ), std::end( sinks ),
		 spdlog::thread_pool( ), spdlog::async_overflow_policy::block );
		
	logger->set_level( spdlog::level::debug );
    spdlog::register_logger( logger );
	spdlog::flush_every( std::chrono::seconds( 5 ) );
}

int main( )
{
	create_logger( );
	_logger = spdlog::get( "gymon" );

	gymon::server server;
	std::future<void> future{ server.listen( _port ) };	
	_logger->info( "Server listening on port {0}", _port );

	future.wait( );
	_logger->info( "Server shutting down" );
	spdlog::drop( "gymon" );
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