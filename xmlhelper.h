#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <pugixml/pugixml.hpp>
#include <array>

namespace gymon
{
    class xmlhelper
    {
    public:
        static std::optional<std::string> getoffsets( int instance ) noexcept;
        
        static std::optional<std::string> getcolor( int instance ) noexcept;

    private:
        static std::optional<std::string> getcolor(
            pugi::xml_document const& doc ) noexcept;
    
    private:   
        // Stores the human readable nozzle color
        // map for each Gymea instance.
        static std::array<std::string, 4> _cache;     

        // Array of paths to each Gymea instances configuration.
		static constexpr std::array<char const*, 4> _paths {
			"/opt/memjet/gymea/data/currentConfigs.xml",
			"/opt/memjet/gymea/data/currentConfigs-1.xml",
			"/opt/memjet/gymea/data/currentConfigs-2.xml",
			"/opt/memjet/gymea/data/currentConfigs-3.xml"
		};
    };    
}

