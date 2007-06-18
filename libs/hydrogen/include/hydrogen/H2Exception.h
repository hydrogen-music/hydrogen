#ifndef H2EXCEPTION_H
#define H2EXCEPTION_H

#include <string>

namespace H2Core {

class H2Exception
{
	public:
		H2Exception( const std::string& msg ) : message( msg ) {}
		std::string message;
};

};

#endif
