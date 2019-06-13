#include "resparse.h"
#include "strext.h"
#include <regex>
#include <fmt/format.h>

namespace gymon
{
	std::optional<std::vector<std::string>> resparse::parsems( std::string const& bashres ) noexcept
	{
		std::vector<std::string> lines{ strext::split( bashres, R"(\r\n|\n)" ) };
		std::vector<std::string> responses;
		responses.reserve( lines.size( ) );

		static std::regex const pattern{ R"(^(.*?)\sis\s(.*?)$)",
			std::regex_constants::ECMAScript | 
			std::regex_constants::icase		 |
			std::regex_constants::optimize };

		for( std::size_t i{ 0 }; i < lines.size( ); i++ )
		{
			if( std::smatch match; std::regex_search( lines[ i ], match, pattern ) )
			{
				responses.emplace_back( fmt::format( "Status {0}: {1}\n",
					match[ 1 ].str( ),
					match[ 2 ].str( ) ) );
			}
		}
		if( !responses.empty( ) )
			return responses;

		return std::nullopt;
	}

	std::optional<std::string> resparse::parsess( std::string const& bashres, int instance ) noexcept
	{
		static std::regex const pattern{ R"(^(\w+,\s+-?\d+,(?:\s*\w+\s*)+)$)",
			std::regex_constants::ECMAScript |
			std::regex_constants::icase 	 | 
			std::regex_constants::optimize };

		if( std::smatch match; std::regex_search( bashres, match, pattern ) )
			return fmt::format( "Status Gymea instance {0}: {1}\n", instance, match[ 1 ].str( ) );
		
		return std::nullopt;
	}

	std::optional<std::vector<std::string>> resparse::parsecmd( std::string const& bashres ) noexcept
	{
		std::vector<std::string> lines{ strext::split( bashres, R"(\r\n|\n)" ) };
		std::vector<std::string> responses;
		responses.reserve( lines.size( ) );

		// This pattern will work for both a single instance
		// command and a multi instance command.
		static std::regex const pattern{ R"((.*?):.*?(FAILED|OK).*?)",
			std::regex_constants::ECMAScript | 
			std::regex_constants::icase		 | 
			std::regex_constants::optimize };

		for( std::size_t i{ 0 }; i < lines.size( ); i++ )
		{
			if( std::smatch match; std::regex_search( lines[ i ], match, pattern ) )
			{		
				responses.emplace_back( fmt::format( "{0}: {1}\n",
					match[ 1 ].str( ),
					match[ 2 ].str( ) ) );
			}
		}
		if( !responses.empty( ) )
			return responses;

		return std::nullopt;		
	}
}