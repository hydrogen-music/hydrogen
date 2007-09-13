#ifndef H2EXCEPTION_H
#define H2EXCEPTION_H

#include <string>
#include <stdexcept>

namespace H2Core
{

class H2Exception : public std::runtime_error
{
public:

	H2Exception( const std::string& msg ) : std::runtime_error( msg )
	{
	}

};

};

#endif
