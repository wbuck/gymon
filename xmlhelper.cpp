#include "xmlhelper.h"
#include <pugixml/pugixml.hpp>
#include <sstream>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <memory>

namespace gymon
{
    std::optional<std::string> xmlhelper::getoffsets( 
        std::string_view path, int instance ) noexcept
    {
        pugi::xml_document doc;
	    if( auto result{ doc.load_file( path.data( ) ) }; result )
	    {
	    	pugi::xml_node node{ doc.child( "CurrentConfigs" ).child( "ResourceMgr" ) };    
	    	std::ostringstream oss;
            // Grab the segment offset value for each of the
            // 11 segmentsin the printhead.
	    	for( auto i = 0; i < 11; i++ )
	    	{
	    		std::string value{ node.child_value( fmt::format( "SegY{0}", i ).c_str( ) ) };
	    		if( value.empty( ) ) value = '0';
	    		if( i < 10 ) oss << value << ", ";			
	    		else oss << value;
	    	}
	    	std::string px{ node.child_value( "PageX" ) };
	    	if( px.empty( ) ) px = '0';
	    	std::string delay{ node.child_value( "MediaSensorDelay" ) };
	    	if( delay.empty( ) ) delay = '0';

	    	return fmt::format( 
	    		"Offset Gymea instance {0}: PageX: {1}, MediaSensorDelay: {2}, Segments: [ {3} ]\r\n", 
                instance, px, delay, oss.str( ) );
	    }
        else
        {
            if( auto logger{ spdlog::get( "gymon" ) }; logger )
            {
                logger->error( "Failed to open {0} due to: {1}", 
                    path, result.description( ) );
            }
	        return std::nullopt;            
        }                
    }
}