#pragma once
#include "sighelper.h"
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <optional>

namespace gymon
{
	class daemon
	{
	public:
		template<typename ...Signals>
		static std::optional<int> daemonize( Signals&& ...signals ) noexcept
		{
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
			int32_t sigfd{ create_sigfd( std::forward<Signals&&>( signals )... ) };

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
