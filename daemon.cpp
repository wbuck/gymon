#include "daemon.h"
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <iostream>
#include <sys/signalfd.h>


namespace gymon
{
	int daemon::daemonize( SignalHandler pfnc ) noexcept
	{
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

		// TODO: Open logs here.

		// Create a new SID for the child process.
		if( sid = setsid( ); sid < 0 )
		{
			// Log failure.
			exit( EXIT_FAILURE );
		}

		// Catch, ignore and handle signals.
		//signal( SIGCHLD, SIG_IGN );
		//signal( SIGHUP, SIG_IGN );
		//if( setsigact( pfnc ) < 0 )
		//{
		//	std::cerr << "Failed to set signal action\n";
		//	exit( EXIT_FAILURE );
		//}
		sigset_t mask;
		sigemptyset( &mask );
		sigaddset( &mask, SIGTERM );

		int32_t sigfd{ signalfd( -1, &mask, 0 ) };
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

		return sigfd;
	}

	int daemon::setsigact( SignalHandler pfnc ) noexcept
	{
		static struct sigaction action { 0 };
		action.sa_sigaction = pfnc;
		action.sa_flags = SA_SIGINFO;
		return sigaction( SIGTERM, &action, nullptr );
	}
}