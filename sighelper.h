#pragma once
#include <sys/signalfd.h>
#include <signal.h>
#include <type_traits>
#include <vector>

namespace gymon
{
    template<typename ...Signals>
	static int create_sigfd( Signals&& ...signals ) noexcept
    {
        static_assert( std::conjunction_v<std::is_same<Signals, int>...>, 
				"Invalid signal type. Signals must be a 32 bit integer" );

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
				return sigfd;
			}

			// Block the signals that we handle using signalfd
			// so that they don't cause signal handlers or default
			// signal actions to execute.
			if( sigprocmask( SIG_BLOCK, &mask, nullptr ) < 0 )
				perror( "sigprocmask" );			
		}	
        return sigfd;
    }
}