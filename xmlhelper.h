#pragma once
#include <optional>
#include <string>
#include <string_view>

namespace gymon
{
    class xmlhelper
    {
    public:
        static std::optional<std::string> getoffsets( 
            std::string_view path, int instance ) noexcept;
    };
}

