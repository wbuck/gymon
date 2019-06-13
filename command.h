#pragma once
#include "strext.h"
#include "cmdtype.h"
#include <optional>
#include <type_traits>
#include <string>
#include <string_view>
#include <cstdlib>
#include <optional>
#include <string.h>
#include <fmt/format.h>
#include <iostream>

namespace gymon
{
	template<typename T>
	struct is_integer 
		: public std::false_type { };

	template<>
	struct is_integer<int> 
		: public std::true_type { };

	template<typename T>
	constexpr bool is_integer_v = is_integer<T>::value;

	template<typename T>
	struct is_cstring 
		: public std::false_type { };

	template<>
	struct is_cstring<char> 
		: public std::true_type { };

	template<typename T>
	constexpr bool is_cstring_v = is_cstring<T>::value;

	template<typename T>
	struct is_string 
		: public std::false_type { };

	template<>
	struct is_string<std::string> 
		: public std::true_type { };	

	template<typename T>
	constexpr bool is_string_v = is_string<T>::value;

	template<typename T>
	struct is_string_view
		: public std::false_type { };

	template<>
	struct is_string_view<std::string_view>
		: public std::true_type { };

	template<typename T>
	constexpr bool is_string_view_v = is_string_view<T>::value;

	template<typename T>
	struct is_string_type
	{
		static constexpr bool value{ 
			is_cstring_v<T> || is_string_v<T> || is_string_view_v<T> };
	};

	template<typename T>
	constexpr bool is_string_type_v = is_string_type<T>::value;

	template<typename T>
	struct is_cmdtype 
		: public std::false_type { };

	template<>
	struct is_cmdtype<cmdtype>
		: public std::true_type { };

	template<typename T>
	constexpr bool is_cmdtype_v = is_cmdtype<T>::value;

	struct command
	{
		// Sets the Gymea instance value for this command.
		template<typename T>
		bool setinstance( T const& value ) noexcept
		{
			bool result{ false };
			if constexpr( is_integer_v<T> )
			{
				_instance = value;
				result = true;
			}
			else if constexpr( is_cstring_v<std::decay_t<T>> )
			{
				char* notconv;
				_instance = strtol( value, &notconv, 10 );
				// If notconv is empty it means the
				// conversion was successful.
				result = !*notconv;
			}
			else if constexpr( is_string_v<T> )
			{
				char* notconv;
				_instance = strtol( value.c_str( ), &notconv, 10 );
				// If notconv is empty it means the
				// conversion was successful.
				result = !*notconv;
			}
			else if constexpr( is_string_view_v<T> )
			{
				char* notconv;
				_instance = strtol( value.data( ), &notconv, 10 );
				// If notconv is empty it means the
				// conversion was successful.
				result = !*notconv;
			}
			return result;
		}

		template<typename T>
		bool settype( T const& value ) noexcept
		{
			bool result{ false };
			if constexpr( is_cmdtype_v<T> )
			{
				_type = value;
				result = true;
			}
			else if constexpr( is_string_type_v<T> )
			{
				if( auto ctype{ string_totype( value ) }; ctype.has_value( ) )
				{
					_type = ctype.value( );					
					result = true;
				}									
			}
			return result;
		}

		std::string buildcmd( ) const noexcept
		{
			// Redirect any stderr output to stdout.
			static constexpr char const* redirect{ " 2>&1" };
			if( _instance.has_value( ) )
			{				
				return fmt::format( "service gymea {0} {1} {2}",
					type_tostring( ), _instance.value( ), redirect );
			}
			else
			{
				return fmt::format( "service gymea {0} {1}",
					type_tostring( ), redirect );
			}
		}

		// Returns the command type.
		cmdtype gettype( ) const noexcept
		{ return _type; }

		// Returns the optional Gymea instance ID.		
		std::optional<int> getinstance( ) const noexcept
		{ return _instance; }
	
		// Returns a string representation of the
		// cmdtype enum.
		constexpr char const* type_tostring( ) const noexcept
		{
			switch( _type )
			{
				case cmdtype::start:
					return "start";
				case cmdtype::stop:
					return "stop";
				case cmdtype::restart:
					return "restart";
				default:
					return "status";
			}
		}

		private:

		// Returns the cmdtype enum member corresponding
		// to the input string_view.
		std::optional<cmdtype> string_totype( std::string_view value ) const noexcept
		{
			if( strcasecmp( "start", value.data( ) ) == 0 )
				return cmdtype::start;
			if( strcasecmp( "stop", value.data( ) ) == 0 )
				return cmdtype::stop;
			if( strcasecmp( "restart", value.data( ) ) == 0 )
				return cmdtype::restart;
			if( strcasecmp( "status", value.data( ) ) == 0 )
				return cmdtype::status;

			return std::nullopt;
		}

	private:
		// Specifies what command to execute.
		cmdtype _type;

		// Specifies what Gymea instance to issue
		// the command for. If this is a multi-instance
		// command then this optional will be null.
		std::optional<int> _instance;
	};
}
