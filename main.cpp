#include "server.h"
#include "connection.h"
#include "daemon.h"
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


static std::shared_ptr<spdlog::logger> _logger;
static const std::string _port{ "32001" };

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

static void servrun( std::optional<int32_t> sigfd )
{
	gymon::server server;
	std::future<void> future{ server.listen( _port, std::move( sigfd ) ) };	
	_logger->info( "Server listening on port {0}", _port );
	future.wait( );
	_logger->info( "Server shutting down" );
}

int main( )
{		
	std::optional<int32_t> sigfd{ gymon::daemon::daemonize( SIGTERM ) };
	// Open the log file.
	_logger = create_logger( );
	// Daemon-specific initialization goes here.
	_logger->debug( "Gymon started" );
	servrun( std::move( sigfd ) );
	_logger->debug( "Gymon terminated" );
	spdlog::drop( "gymon" );
	return EXIT_SUCCESS;
}