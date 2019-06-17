#pragma once
#include <string>
#include <string_view>
#include <cstdio>
#include <vector>
#include <iostream>
#include <regex>
#include <algorithm>

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

		static std::vector<std::string> split( std::string const& str, std::string_view pattern ) noexcept
		{
			std::vector<std::string> tokens;
			std::regex rgx{ pattern.data( ), 
				std::regex_constants::ECMAScript | 
				std::regex_constants::icase	};
			
			std::sregex_token_iterator it{ std::begin( str ), std::end( str ), rgx, -1 };
			std::sregex_token_iterator end;

			if( it == end )	
			{
				tokens.push_back( str );
				return tokens;				
			}
			
			while( it != end )
			{				
				tokens.push_back( it->str( ) );
				++it;
			}
			
			return tokens;
		}

		static void erase_all( std::string& str, std::string_view substr ) noexcept
		{
			std::size_t pos{ std::string::npos };
			while ( (pos = str.find( substr ) ) != std::string::npos )
				str.erase( pos, substr.length( ) );			
		}
	};
}
