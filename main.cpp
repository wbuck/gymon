#include "server.h"
#include "connection.h"
#include "daemon.h"
#include <iostream>
#include <future>
#include <vector>
#include <chrono>
#include <memory>
#include <signal.h>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/syslog_sink.h>
#include <spdlog/async.h>


static std::shared_ptr<spdlog::logger> _logger;
static const std::string _port{ "32001" };
static constexpr char const* _name{ "gymon" };
static bool _recvsig{ false };
static std::mutex _mutex;

static void signal_handler( int32_t signum, siginfo_t* info, void* ptr )
{
	if( signum == SIGTERM )
	{
		std::scoped_lock<std::mutex> lock{ _mutex };
		if( _logger )
			_logger->debug( "Received SIGTERM {0}, shutting down", signum ); 
		_recvsig = true; 
	}	
}

static std::shared_ptr<spdlog::logger> create_logger( )
{
	spdlog::init_thread_pool( 8192, 1 );
    //auto stdout_sink{ std::make_shared<spdlog::sinks::stdout_color_sink_mt>( ) };
	//stdout_sink->set_level( spdlog::level::debug );

    auto rotating_sink{ 
		std::make_shared<spdlog::sinks::rotating_file_sink_mt>( "/home/wbuckley/gymonlog.txt", 1024 * 1024 * 10 , 3 ) };
	rotating_sink->set_level( spdlog::level::debug );
	
	//auto system_sink{ std::make_shared<spdlog::sinks::syslog_sink_mt>( "gymon", LOG_PID, LOG_DAEMON ) };
	//system_sink->set_level( spdlog::level::debug );

    std::vector<spdlog::sink_ptr> sinks { /*stdout_sink,*/ rotating_sink, /*system_sink*/ };
    auto logger = std::make_shared<spdlog::async_logger>( 
		"gymon", std::begin( sinks ), std::end( sinks ),
		 spdlog::thread_pool( ), spdlog::async_overflow_policy::block );
		
	logger->set_level( spdlog::level::debug );
    spdlog::register_logger( logger );
	spdlog::flush_every( std::chrono::seconds( 5 ) );
	return logger;
}

static void servrun( int32_t sigfd )
{
	gymon::server server;
	std::future<void> future{ server.listen( _port, sigfd ) };	
	_logger->info( "Server listening on port {0}", _port );
	future.wait( );
	_logger->info( "Server shutting down" );
}

static void servstop( )
{
	
}

int main( )
{		
	int32_t sigfd{ gymon::daemon::daemonize( signal_handler ) };
	// Open the log file.
	_logger = create_logger( );
	// Daemon-specific initialization goes here.
	_logger->debug( "Gymon started" );
	servrun( sigfd );
	_logger->debug( "Gymon terminated" );
	spdlog::drop( "gymon" );
	return EXIT_SUCCESS;
}