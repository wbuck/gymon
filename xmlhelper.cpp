#include "xmlhelper.h"
#include <pugixml/pugixml.hpp>
#include <sstream>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <memory>
#include <string.h>

namespace gymon
{
    std::optional<std::string> xmlhelper::getoffsets( 
        std::string_view path, int instance ) noexcept
    {
        pugi::xml_document doc;
	    if( auto result{ doc.load_file( path.data( ) ) }; result )
	    {
	    	pugi::xml_node node{ doc.child( "CurrentConfigs" ).child( "ResourceMgr" ) }; 
			if( !node ) 
			{
				if( auto logger{ spdlog::get( "gymon" ) }; logger )
            	    logger->error( "Failed to locate XML nodes CurrentConfigs or ResourceMgr" );

            	return std::nullopt;
			}
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

			// Grab the color mapping for the requested
			// gymea instance.
			std::string color{ "" };
			if( pugi::xml_node qanode{ 
				doc.child( "CurrentConfigs" ).child( "QaLssConfig" ) }; !qanode )
			{
				if( auto logger{ spdlog::get( "gymon" ) }; logger )
            	    logger->error( "Failed to locate XML node QaLssConfig" );
			} 
			else
			{
				color = qanode.child_value( "NozzleMap" );
				// Convert the channel map to a human readable string.
				if( strcasecmp( color.data( ), R"("CCCCC")" ) == 0 )
					color = "Cyan";
				else if( strcasecmp( color.data( ), R"("MMMMM")" ) == 0 )
					color = "Magenta";
				else if( strcasecmp( color.data( ), R"("YYYYY")" ) == 0 )
					color = "Yellow";
				else if( strcasecmp( color.data( ), R"("KKKKK")" ) == 0 )
					color = "Black";
			}						

	    	return fmt::format( 
	    		"Offset Gymea instance {0}: Color: {1}, PageX: {2}, MediaSensorDelay: {3}, Segments: [ {4} ]\r\n", 
                instance, color, px, delay, oss.str( ) );
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