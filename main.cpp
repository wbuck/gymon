#include "server.h"
#include "connection.h"
#include "daemon.h"
#include "sighelper.h"
#include <iostream>
#include <future>
#include <vector>
#include <chrono>
#include <cstdio>
#include <syslog.h>
#include <optional>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/syslog_sink.h>
#include <spdlog/async.h>
#include <CLI/CLI.hpp>

static std::shared_ptr<spdlog::logger> _logger;

static std::shared_ptr<spdlog::logger> create_logger( bool run_as_console )
{
	spdlog::init_thread_pool( 8192, 1 );
	std::vector<spdlog::sink_ptr> sinks;

	if( run_as_console )
	{
		sinks.emplace_back( std::make_shared<spdlog::sinks::stdout_color_sink_mt>( ) );
		sinks.front( )->set_level( spdlog::level::debug );
	}
    
	//sinks.emplace_back( std::make_shared<spdlog::sinks::syslog_sink_mt>( 
	//	"gymon", LOG_PID, LOG_DAEMON ) );
	 sinks.emplace_back( std::make_shared<spdlog::sinks::rotating_file_sink_mt>( 
		 "/var/log/gymon/gymon.log", 1024 * 1024 * 10 , 3 ) );
	sinks.back( )->set_level( spdlog::level::debug );

    auto logger{ std::make_shared<spdlog::async_logger>( 
		"gymon", std::begin( sinks ), std::end( sinks ),
		 spdlog::thread_pool( ), spdlog::async_overflow_policy::block ) };
		
	logger->set_level( spdlog::level::debug );
    spdlog::register_logger( logger );
	spdlog::flush_every( std::chrono::seconds( 5 ) );
	return logger;
}

static void servrun( std::string const& port, std::optional<int32_t> sigfd )
{
	gymon::server server;
	std::future<void> future{ server.listen( port, std::move( sigfd ) ) };	
	if( _logger )
		_logger->info( "Server listening on port {0}", port );
	future.wait( );

	if( _logger )
		_logger->info( "Server shutting down" );
}

int main( int argc, char** argv )
{		
	CLI::App app{ "Gymon Options" };
	bool run_as_console{ false };
	app.add_flag( "-c", run_as_console, "Run as console application" );

	std::string port{ "32001" };
	app.add_option( "-p, --port", port, "Listening port" );
	CLI11_PARSE( app, argc, argv );

	std::optional<int32_t> sigfd{ std::nullopt };
	if( !run_as_console )
		gymon::daemon::daemonize( SIGTERM ).swap( sigfd );
	
	// Create a signal file descriptor which becomes
	// active when a system signal is received.
	else if( int32_t fd{ gymon::create_sigfd( SIGTERM, SIGINT ) }; fd > -1 )
		sigfd = fd;
	
	_logger = create_logger( run_as_console );
	
	if( _logger )
		_logger->debug( "Gymon started" );

	servrun( port, std::move( sigfd ) );

	if( _logger )
	{
		_logger->debug( "Gymon terminated" );
		spdlog::drop( "gymon" );
	}

	return EXIT_SUCCESS;
}