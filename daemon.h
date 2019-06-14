#pragma once
#include <signal.h>

namespace gymon
{
	using SignalHandler = void( * )( int signum, siginfo_t* info, void* ptr );

	class daemon
	{
	public:
		static int daemonize( SignalHandler pfnc ) noexcept;
	private:
		static int setsigact( SignalHandler pfnc ) noexcept;
	};
}
