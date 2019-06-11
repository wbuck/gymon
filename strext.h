#pragma once
#include <string>
#include <string_view>
#include <cstdio>
#include <vector>
#include <iostream>

namespace gymon
{
	class strext
	{
	public:
		static bool starts_with( std::string_view str, std::string_view prefix ) noexcept
		{
			if( prefix.size( ) > str.size( ) ) return false;
			return std::equal( std::begin( prefix ), std::end( prefix ), std::begin( str ) );
		}

		static bool ends_with( std::string_view str, std::string_view suffix ) noexcept
		{
			if( suffix.size( ) > str.size( ) ) return false;
			return std::equal( std::rbegin( suffix ), std::rend( suffix ), std::rbegin( str ) );
		}

		static std::vector<std::string> split( std::string_view str, std::string_view delim )
		{
			std::vector<std::string> tokens;
			size_t prev = 0, pos = 0;
			do
			{
				if( pos = str.find( delim, prev ); pos == std::string::npos )
					pos = str.length( );

				if( std::string token{ str.substr( prev, pos - prev ) }; !token.empty( ) )
					tokens.push_back( std::move( token ) );

				prev = pos + delim.length( );
			} while( pos < str.length( ) && prev < str.length( ) );
			return tokens;
		}
	};
}
