#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <vector>

namespace gymon
{
	class resparse
	{
	public:
		static std::optional<std::vector<std::string>> parsems( std::string const& bashres ) noexcept;
		static std::optional<std::string> parsess( std::string const& bashres, int instance ) noexcept;
		static std::optional<std::vector<std::string>> parsecmd( std::string const& bashres ) noexcept;
	};
}
