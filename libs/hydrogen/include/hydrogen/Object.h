/*
 * Hydrogen
 * Copyright(c) 2002-2007 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef H2_OBJECT_H
#define H2_OBJECT_H


#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <pthread.h>


// LOG MACROS

#define _INFOLOG(x) Logger::getInstance()->infoLog(       "(I) [" + std::string( __PRETTY_FUNCTION__ ) + std::string( "] \t" ) + x );
#define _WARNINGLOG(x) Logger::getInstance()->warningLog( "(W) [" + std::string( __PRETTY_FUNCTION__ ) + std::string( "] \t" ) + x );
#define _ERRORLOG(x) Logger::getInstance()->errorLog(     "(E) [" + std::string( __PRETTY_FUNCTION__ ) + std::string( "] \t" ) + x );




#define INFOLOG(x) Logger::getInstance()->infoLog(       "(I) " + getClassName() + " \t [" + std::string( __FUNCTION__ ) + std::string( "] \t" ) + x );
#define WARNINGLOG(x) Logger::getInstance()->warningLog( "(W) " + getClassName() + " \t [" + std::string( __FUNCTION__ ) + std::string( "] \t" ) + x );
#define ERRORLOG(x) Logger::getInstance()->errorLog(     "(E) " + getClassName() + " \t [" + std::string( __FUNCTION__ ) + std::string( "] \t" ) + x );



class Object;

/**
 * Class for writing logs to the console
 */
class Logger {
	public:
		bool m_bUseFile;
		bool m_bIsRunning;
		std::vector<std::string> m_msgQueue;
		pthread_mutex_t m_logger_mutex;

		static Logger* getInstance();

		/** Destructor */
		~Logger();

		void infoLog( const std::string& logMsg );
		void warningLog( const std::string& logMsg );
		void errorLog( const std::string& logMsg );

	private:
		static Logger *m_pInstance;

		/** Constructor */
		Logger();


};


/**
 * Base class.
 */
class Object
{
	public:
		static bool m_bUseLog;

		/** Constructor */
		Object( const std::string& sClassName );
		Object( const Object& obj );

		/** Destructor */
		virtual ~Object();

		const std::string& getClassName() const {	return 	m_sClassName;	}

		static int getNObjects();
		static void printObjectMap();
		static void useVerboseLog( bool bUse );
		static bool isUsingVerboseLog();

	private:
		static unsigned m_nObjects;
		static std::map<std::string, int> m_ObjectMap;
		Logger *m_pLogger;
		std::string m_sClassName;
};


// some useful functions..

template <class T>
inline std::string toString(const T& t)
{
	std::ostringstream osstream;
	osstream << t;
	return osstream.str();
}

inline int stringToInt( const std::string& str )
{
	std::istringstream isstream(str);
	int t;
	isstream >> t;
	return t;
}
inline float stringToFloat( const std::string& str )
{
	std::istringstream isstream(str);
	float t;
	isstream >> t;
	return t;
}

#endif
