#pragma once
#include "command.h"
#include "strext.h"
#include "sockresult.h"
#include "resparse.h"
#include <array>
#include <errno.h>
#include <algorithm>
#include <type_traits>
#include <cstdlib>
#include <string_view>
#include <memory>
#include <string>
#include <iterator>
#include <string.h>
#include <regex>
#include <optional>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fmt/format.h>
#include <fmt/time.h>


namespace gymon
{
	template<std::size_t N>
	class connection
	{
		using socketfd = int32_t;
		static_assert( N <= 4096, "Size cannot exceed 4096" );

	public:
		explicit connection( socketfd socket ) noexcept
			: _socket{ socket }	{ }

		connection( connection&& ) = default;

		connection& operator=( connection&& ) = default;

		connection( connection const& ) = delete;

		connection& operator=( connection const& ) = delete;

		socketfd getsocket( ) const noexcept
		{ return _socket; }

		bool handlereq( ) noexcept;		

	private:

		sockresult receive( ) noexcept;

		sockresult send( std::string_view msg ) const noexcept;

		std::optional<command> getcmd( std::string const& req ) const noexcept;

		std::optional<std::string> execshell( command const& cmd ) const noexcept;

		template<typename Iter, typename ...Args>
		int32_t indexof( Iter begin, Iter end, Args&& ...args ) const noexcept;

	private:
		socketfd _socket;
		std::array<char, N> _buffer;
		ssize_t _bytesread{ 0 };
	};

	template<std::size_t N>
	inline bool connection<N>::handlereq( ) noexcept
	{
		switch( receive( ) )
		{
			case sockresult::closed:
			{
				std::cout << "Connection closed by remote host\n";
				close( _socket );
				return false;
			}
			case sockresult::error:
			{
				perror( "Error encountered while receving" );
				close( _socket );
				return false;
			}
			case sockresult::retry:
			{
				perror( "Error encountered while receving. Retrying receive." );
				return true;
			}
			case sockresult::success:
			{
				// Check to see if the line terminator was
				// received. If not, continue listening.
				if( int32_t index{ indexof( std::begin( _buffer ),
					std::end( _buffer ), '#', 'e', 'n', 'd' ) }; index > -1 )
				{
					std::string req{ std::begin( _buffer ), std::begin( _buffer ) + index };
					// We received a complete message so parse
					// the request.
					if( auto cmd{ getcmd( req ) }; cmd.has_value( ) )
					{
						// Attempt the execute the shell command.
						if( auto outp{ execshell( cmd.value( ) ) }; outp.has_value( ) )
						{
							if( send( outp.value( ) ) == sockresult::error )
							{
								perror( "Failed to write to socket." );
								close( _socket );
								return false;
							}
						}
						else
						{							
							if( send( fmt::format( 
								"ERROR: Unable to execute '{0}' command", req ) ) == sockresult::error )
							{
								perror( "Failed to write to socket." );
								close( _socket );
								return false;
							}
						}
					}
					// We were unable to parse the request so alert
					// th client of the error.
					else
					{						
						if( send( fmt::format( 
							"ERROR: The request '{0}' is invalid", req ) ) == sockresult::error )
						{
							perror( "Failed to write to socket." );
							close( _socket );
							return false;
						}
					}
					std::fill_n( _buffer.data( ), _bytesread, 0 );
					_bytesread = 0;
				}
				else if( _bytesread >= static_cast<ssize_t>( _buffer.size( ) - 1 ) )
				{
					std::cerr << "Command too large\n";
					close( _socket );
					return false;
				}
				return true;
			}				
			default:
			{
				std::cerr << "Invalid socket result.";
				close( _socket );
				return false;
			}
		}
	}

	template<std::size_t N>
	inline sockresult connection<N>::receive( ) noexcept
	{
		// Read the incoming TCP stream from the client.
		_bytesread += ::recv( _socket, _buffer.data( ) + _bytesread, _buffer.size( ) - 1, 0 );

		// Socket was closed by the remote client.
		if( _bytesread == 0 )		
			return sockresult::closed;
		
		// An error was encountered while receving data
		// from the client.
		else if( _bytesread < 0 )
		{
			// No data was available when attempting to read
			// so just return to a waiting state.
			if( errno == ETIMEDOUT || errno == EAGAIN )
				return sockresult::retry;

			// Some other error occurred so close the
			// socket.
			return sockresult::error;
		}
		return sockresult::success;
	}	

	template<std::size_t N>
	inline std::optional<std::string> connection<N>::execshell( command const& cmd ) const noexcept
	{
		auto const deleter{ [ ]( FILE* file ) { if( file ) pclose( file ); } };
		std::unique_ptr<FILE, decltype( deleter )> pfile{
			popen( cmd.buildcmd( ).c_str( ), "r" ), deleter };

		if( !pfile.get( ) )
		{
			perror( "Failed to open shell process." );
			return std::nullopt;
		}

		std::ostringstream oss;
		char buffer[ 256 ]{ 0 };

		if( FILE* file{ fopen( R"(/mnt/c/Users/wbuckley/OneDrive/Projects/gymon-make/GymeaOutput.txt)", "r" ) }; file )
		{
			while( fgets( buffer, sizeof( buffer ), file ) != nullptr )
			{
				oss << buffer;
				bzero( buffer, sizeof( buffer ) );
			}
			fclose( file );
		}
		
		
		//while( fgets( buffer, sizeof( buffer ), pfile.get( ) ) != nullptr )
		//{
		//	oss << buffer;
		//	bzero( buffer, sizeof( buffer ) );
		//}
		if( pfile && ferror( pfile.get( ) ) )
		{
			perror( "Failed to execute shell command" );
			return std::nullopt;
		}

		// Remove the trailing CRLF from the stdout
		// output.
		auto constexpr trim_from = [ ]( std::string& str, auto delim ) 
		{
			if( std::size_t pos{ str.find_first_of( delim ) }; pos != std::string::npos )
				str.erase( pos, str.length( ) );
		};

		// Wrap the parsing logic so that it can
		// be reused.
		using pfunc = std::optional<std::vector<std::string>>( * )( std::string_view sv );
		auto const parse = [ & ]( pfunc func, std::string const& str ) 
			-> std::optional<std::string>
		{
			std::optional<std::vector<std::string>> responses{ func( str ) };
			if( responses.has_value( ) )
			{
				// Reset the stream.
				std::ostringstream{ }.swap( oss );
				for( std::string const& resp : responses.value( ) )
					oss << resp;

				return oss.str( );
			}
			return std::nullopt;				
		};
		
		if( std::string out{ oss.str( ) }; !out.empty( ) )
		{
			if( cmd.gettype( ) == cmdtype::restart ||
				cmd.gettype( ) == cmdtype::start   ||
				cmd.gettype( ) == cmdtype::stop )
			{
				if( auto resp{ parse( resparse::parsecmd, out ) }; resp.has_value( ) )
					return resp.value( );

				// If we reach here it means we were unable
				// to parse the shell reply.
				trim_from( out, '\n' );
				return out;
			}
			else if( cmd.gettype( ) == cmdtype::status &&
					 cmd.getinstance( ).has_value( ) )
			{
				trim_from( out, '\n' );
				auto resp{ resparse::parsess( out, cmd.getinstance( ).value( ) ) };				
				return resp.has_value( ) ? resp : out;
			}
			else
			{
				if( auto resp{ parse( resparse::parsems, out ) }; resp.has_value( ) )
					return resp.value( );
				// If we reach here it means we were unable
				// to parse the shell reply.
				trim_from( out, '\n' );
				return out;
			}
		}		
		return std::nullopt;
	} 

	template<std::size_t N>
	template<typename Iter, typename ...Args>
	inline int32_t connection<N>::indexof( Iter begin, Iter end, Args&& ...args ) const noexcept
	{		
		static_assert( std::conjunction_v<std::is_same<char, Args>...>,
			"Pattern parameters must be of type char" );

		static_assert( std::is_same_v<char, typename std::iterator_traits<Iter>::value_type>,
			"The iterators underlying type must be of type char" );

		std::vector<char> const needle{ std::forward<Args&&>( args )... };

		// Find the first occurance of the delimiter 
		// in the buffer.
		if( auto it{ std::search( begin, end,
			std::begin( needle ), std::end( needle ) ) }; it != end )
		{
			std::ptrdiff_t index{ std::distance( begin, it ) };
			return static_cast<int32_t>( index );
		}
		return -1;
	}

	template<std::size_t N>
	inline sockresult connection<N>::send( std::string_view msg ) const noexcept
	{
		// The delimiter used to bookend the outgoing
		// message.
		static constexpr char const* delimiter{ "#tcl" };
				
		// Format the outgoing message with the correct
		// delimiters and a CRLF.
		std::time_t timer{ std::time( nullptr ) };
		std::string const fmsg{ fmt::format( "{}{:%Y%m%d%H%M%S}\n{}\n{}\r\n", 
			delimiter, *std::localtime( &timer ), msg, delimiter ) };

		ssize_t bytessent{ 0 };
		while( bytessent < static_cast<ssize_t>( fmsg.size( ) ) )
		{
			if( bytessent = ::send( _socket, fmsg.c_str( ) +
					bytessent, fmsg.size( ) - bytessent, 0 ); bytessent <= 0 )
			{
				if( bytessent == 0 )
				{
					std::cerr << "Attemping send again\n";
					continue;
				}
				// A temporary error was encountered to try
				// sending the data again.
				else if( errno == EINTR || errno == EAGAIN )
				{
					perror( "Error encountered while writing. Retrying send." );
					continue;
				}
				else
				{
					perror( "Error encountered while writing, closing socket" );
					return sockresult::error;
				}
			}
		}
		return sockresult::success;
	}

	template<std::size_t N>
	inline std::optional<command> connection<N>::getcmd( std::string const& req ) const noexcept
	{
		// Parse the request and look for what
		// command was issued.
		static std::regex const pattern{ R"(^\s{0,5}gymea\s{0,5}(\w+)\s{0,5}([0-3])?\s{0,5}$)",
			std::regex_constants::ECMAScript | std::regex_constants::icase };

		std::smatch match;
		if( std::regex_search( req, match, pattern ) )
		{
			command cmd;
			// If set type is successful it means we received
			// a good command.
			if( cmd.settype( match[ 1 ].str( ) ) )
			{
				// An instace was also sent.
				if( match[ 2 ].matched )
					( void )cmd.setinstance( match[ 2 ].str( ) );

				return cmd;
			}
		}			
		return std::nullopt;
	}
}