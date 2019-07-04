#include "xmlhelper.h"
#include <pugixml/pugixml.hpp>
#include <sstream>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <memory>
#include <string.h>

namespace gymon
{
	std::array<std::string, 4> xmlhelper::_cache;
	
    std::optional<std::string> xmlhelper::getoffsets( int instance ) noexcept
    {
		if( instance > static_cast<int32_t>( _paths.size( ) ) - 1 || instance < 0 )
		{
			if( auto logger{ spdlog::get( "gymon" ) }; logger )
            	logger->error( "Invalid instance {0} passed to 'getoffsets'", instance );

			return std::nullopt;
		}
        pugi::xml_document doc;
	    if( auto result{ doc.load_file( _paths[ instance ] ) }; result )
	    {
	    	pugi::xml_node node{ doc.child( "CurrentConfigs" ).child( "ResourceMgr" ) }; 
			if( !node ) 
			{
				if( auto logger{ spdlog::get( "gymon" ) }; logger )
            	    logger->error( "Failed to locate XML nodes 'CurrentConfigs' or 'ResourceMgr'" );

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
			std::optional<std::string> color{ getcolor( doc ) };					
			if( !color.has_value( ) ) color = "";
			else
				_cache[ instance ] = color.value( );
			
	    	return fmt::format( 
	    		"Offset {0} Gymea instance {1}: PageX: {2}, MediaSensorDelay: {3}, Segments: [ {4} ]\r\n", 
                	color.value( ), instance, px, delay, oss.str( ) );
	    }
        else
        {
            if( auto logger{ spdlog::get( "gymon" ) }; logger )
            {
                logger->error( "Failed to open {0} due to: {1}", 
                    _paths[ instance ], result.description( ) );
            }
	        return std::nullopt;            
        }                
    }

	std::optional<std::string> xmlhelper::getcolor( int instance ) noexcept
	{
		if( instance > static_cast<int32_t>( _paths.size( ) ) - 1 || instance < 0 )
		{
			if( auto logger{ spdlog::get( "gymon" ) }; logger )
            	logger->error( "Invalid instance {0} passed to 'getcolor'", instance );
				
			return std::nullopt;
		}

		// If we already have the value stored, just return
		// it instead of opening and reading the XML config.
		if( !_cache[ instance ].empty( ) ) 
			return _cache[ instance ];

		pugi::xml_document doc;
	    if( auto result{ doc.load_file( _paths[ instance ] ) }; result )
		{
	    	std::optional<std::string> color{ getcolor( doc ) };
			// Update the cache with the color value.
			if( color.has_value( ) )
				_cache[ instance ] = color.value( );
			return color;
		}
		else
		{
			if( auto logger{ spdlog::get( "gymon" ) }; logger )
            {
                logger->error( "Failed to open {0} due to: {1}", 
                    _paths[ instance ], result.description( ) );
            }
	        return std::nullopt; 
		}
	}

	std::optional<std::string> xmlhelper::getcolor(
		pugi::xml_document const& doc ) noexcept
	{
		// Grab the color mapping for the requested
		// gymea instance.
		if( pugi::xml_node node{ 
			doc.child( "CurrentConfigs" ).child( "QaLssConfig" ) }; !node )
		{
			if( auto logger{ spdlog::get( "gymon" ) }; logger )
        	    logger->error( "Failed to locate XML nodes 'CurrentConfigs' or 'QaLssConfig'" );

			return std::nullopt;
		} 
		else
		{
			if( std::string color{ node.child_value( "NozzleMap" ) }; color.empty( ) )
			{
				if( auto logger{ spdlog::get( "gymon" ) }; logger )
        	    	logger->error( "Failed to get the 'NozzleMap' XML node value" );

				return std::nullopt;
			}
			else
			{
				// Convert the channel map to a human readable string.
				if( strcasecmp( color.data( ), R"("CCCCC")" ) == 0 )
					color = "Cyan";
				else if( strcasecmp( color.data( ), R"("MMMMM")" ) == 0 )
					color = "Magenta";
				else if( strcasecmp( color.data( ), R"("YYYYY")" ) == 0 )
					color = "Yellow";
				else if( strcasecmp( color.data( ), R"("KKKKK")" ) == 0 )
					color = "Black";
				else
				{
					if( auto logger{ spdlog::get( "gymon" ) }; logger )
        	    		logger->warn( "Unrecognized nozzle color mapping {0}", color );
				}				
				return color;				
			}						
		}
	}
}