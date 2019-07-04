#include "resparse.h"
#include "strext.h"
#include "xmlhelper.h"
#include <regex>
#include <fmt/format.h>
#include <string.h>
#include <optional>

namespace gymon
{
	std::optional<std::vector<std::string>> resparse::parsems( std::string const& bashres ) noexcept
	{
		std::vector<std::string> lines{ strext::split( bashres, R"(\r\n|\n)" ) };
		std::vector<std::string> responses;
		responses.reserve( lines.size( ) );

		static std::regex const pattern{ R"(^(Gymea\sinstance)\s(\d)\sis\s(.*?)$)",
			std::regex_constants::ECMAScript | 
			std::regex_constants::icase		 |
			std::regex_constants::optimize };

		for( std::size_t i{ 0 }; i < lines.size( ); i++ )
		{
			if( std::smatch match; std::regex_search( lines[ i ], match, pattern ) )
			{
				// Pull the instance out of the command line
				// reply.
				std::string strinstance{ match[ 2 ].str( ) };
				char* notconv;
				int64_t instance{ strtol( strinstance.c_str( ), &notconv, 10 ) };
				if( *notconv ) instance = -1;
				else 
				{
					if( instance < 0 || instance > 3 )
						instance = -1;
				}

				std::optional<std::string> color{ 
					xmlhelper::getcolor( static_cast<int32_t>( instance ) ) };

				if( !color.has_value( ) ) color = "";

				responses.emplace_back( fmt::format( "Status {0} {1} {2}: {3}\r\n",
					color.value( ),
					match[ 1 ].str( ),
					strinstance,
					match[ 3 ].str( ) ) );
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
		{
			std::optional<std::string> color{ xmlhelper::getcolor( instance ) };
			if( !color.has_value( ) ) color = "";

			return fmt::format( "Status {0} Gymea instance {1}: {2}\r\n", 
				color.value( ), instance, match[ 1 ].str( ) );
		}			
		
		return std::nullopt;
	}

	std::optional<std::vector<std::string>> resparse::parsecmd( std::string const& bashres ) noexcept
	{
		std::vector<std::string> lines{ strext::split( bashres, R"(\r\n|\n)" ) };
		std::vector<std::string> responses;
		responses.reserve( lines.size( ) );

		// This pattern will work for both a single instance
		// command and a multi instance command.
		static std::regex const pattern{ R"((\w+)\s(\w+)\s(\w+)\s(\d):.*?(FAILED|OK).*?)",
			std::regex_constants::ECMAScript | 
			std::regex_constants::icase		 | 
			std::regex_constants::optimize };

		for( std::size_t i{ 0 }; i < lines.size( ); i++ )
		{
			if( std::smatch match; std::regex_search( lines[ i ], match, pattern ) )
			{		
				// Pull the instance out of the command line
				// reply.
				std::string strinstance{ match[ 4 ].str( ) };
				char* notconv;
				int64_t instance{ strtol( strinstance.c_str( ), &notconv, 10 ) };
				if( *notconv ) instance = -1;
				else 
				{
					if( instance < 0 || instance > 3 )
						instance = -1;
				}

				std::optional<std::string> color{ 
					xmlhelper::getcolor( static_cast<int32_t>( instance ) ) };

				if( !color.has_value( ) ) color = "";

				responses.emplace_back( fmt::format( "{0} {1} {2} {3} {4}: {5}\r\n",
					match[ 1 ].str( ),
					color.value( ),
					match[ 2 ].str( ),
					match[ 3 ].str( ),
					strinstance,
					match[ 5 ].str( ) ) );
			}
		}
		if( !responses.empty( ) )
			return responses;

		return std::nullopt;		
	}
}