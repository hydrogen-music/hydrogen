/*  Changed DATA_PATH to a DataPath::getDataPath() call to accommodate Mac OS X 
 *  application bundles (2004/12/15 Jonathan Dempsey) */
   
#ifndef H2_SKIN_H
#define H2_SKIN_H
 
#include <string>
#include "lib/DataPath.h"

///
/// Skin support
/// 
class Skin
{
	public:
		static std::string getImagePath() {
			//std::string sSkin = "default";
			std::string sSkin = "gray";
			return std::string( DataPath::getDataPath() ) + std::string( "/img/" ) + sSkin ;
		}

};


#endif
