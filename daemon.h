#pragma once
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <sys/signalfd.h>
#include <signal.h>
#include <type_traits>
#include <vector>
#include <optional>

namespace gymon
{
	class daemon
	{
	public:
		template<typename ...Signals>
		static std::optional<int> daemonize( Signals&& ...signals ) noexcept
		{
			static_assert( std::conjunction_v<std::is_same<Signals, int>...>, 
				"Invalid signal type. Signals must be a 32 bit integer" );			

			std::cout << "Forking off the parent process\n";
			pid_t pid, sid;

			// Fork off the parent process.
			if( pid = fork( ); pid < 0 )
			{
				std::cerr << "Failed to fork process. Pid " << pid << '\n';
				exit( EXIT_FAILURE );
			}
			else if( pid > 0 )
			{
				// If we got a good PID, then we can
				// exit the parent process.
				std::cout << "Fork success, exiting parents process\n";
				exit( EXIT_SUCCESS );
			}

			// Create a new SID for the child process.
			if( sid = setsid( ); sid < 0 )
			{
				// Log failure.
				exit( EXIT_FAILURE );
			}

			// Create a signal file descriptor which becomes
			// active when a system signal is received.
			int32_t sigfd{ -1 };
			if( std::vector<int> sigs{ 
					std::forward<Signals&&>( signals )... }; !sigs.empty( ) )
			{
				sigset_t mask;
				sigemptyset( &mask );

				for( int32_t const& sig : sigs )
					sigaddset( &mask, sig );

				sigfd = signalfd( -1, &mask, 0 );
				if( sigfd < 0 )
				{
					perror( "signalfd" );
					exit( EXIT_FAILURE );
				}

				// Block the signals that we handle using signalfd
				// so that they don't cause signal handlers or default
				// signal actions to execute.
				if( sigprocmask( SIG_BLOCK, &mask, nullptr ) < 0 )
				{
					perror( "sigprocmask" );
					exit( EXIT_FAILURE );
				}
			}			

			// Fork off for a second time.
			if( pid = fork( ); pid < 0 )
				exit( EXIT_FAILURE );
			else if( pid > 0 )
				exit( EXIT_SUCCESS );

			// Set new file permissions.
			umask( 0 );
			
			// Change the working directory to the root directory
			// or another appropriate directory.
			if( ( chdir( "/" ) ) < 0 )
			{
				// Log failure.
				exit( EXIT_FAILURE );
			}

			// Close standard file descriptors.
			close( STDIN_FILENO	);
 			close( STDOUT_FILENO );
 			close( STDERR_FILENO );

			return sigfd > 0 ? sigfd : std::optional<int>{ };
		}
	};
}
