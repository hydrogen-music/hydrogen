/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: Object.h,v 1.7 2005/06/04 22:01:32 comix Exp $
 *
 */

#ifndef H2_OBJECT_H
#define H2_OBJECT_H


#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <sstream>

#ifndef WIN32
	#include <unistd.h>
#endif

using namespace std;

class Object;

/**
 * Class for writing logs to the console
 */
class Logger {
	public:
		bool m_bUseFile;
		bool m_bIsRunning;
		std::vector<std::string> m_msgQueue;

		static Logger* getInstance();

		/** Destructor */
		~Logger();

		/** Simple log */
		void log(std::string logMsg);

		/** Information Log*/
		void info(Object *obj, const std::string& sLogMsg);

		/** Warning Log*/
		void warning(Object *obj, const std::string& sLogMsg);

		/** Error Log*/
		void error(Object *obj, const std::string& sErrorMsg);

	private:
		/** Constructor */
		Logger();

		static Logger *m_pInstance;
};


//----------------------------------------------------------------------------
/**
 * Base class.
 */
class Object
{
	public:
		/** Constructor */
		Object( std::string sClassName );
		Object( const Object& obj );

		/** Destructor */
		virtual ~Object();

		/** Return the class name */
		std::string getClassName() {	return m_sClassName;	}

		static unsigned getNObjects();
		static void printObjectMap();

		void infoLog(const std::string& sMsg);
		void warningLog(const std::string& sMsg);
		void errorLog(const std::string& sMsg);

		static void useVerboseLog( bool bUse );
		static bool isUsingVerboseLog();

	private:
		static unsigned m_nObjects;
		static std::map<std::string, int> m_ObjectMap;
		Logger *m_pLogger;
		static bool m_bUseLog;
		std::string m_sClassName;
};




template <class T>
inline std::string toString(const T& t)
{
	std::ostringstream osstream;
	osstream << t;
	return osstream.str();
}


inline int stringToInt( std::string str )
{
	std::istringstream isstream(str);
	int t;
	isstream >> t;
	return t;
}
inline float stringToFloat( std::string str )
{
	std::istringstream isstream(str);
	float t;
	isstream >> t;
	return t;
}


#endif
