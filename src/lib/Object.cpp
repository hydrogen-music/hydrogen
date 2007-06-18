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
 * $Id: Object.cpp,v 1.10 2005/07/13 19:33:11 comix Exp $
 *
 */

#include "Object.h"
#include "config.h"

#include <pthread.h>

#ifdef WIN32
	#include <windows.h>
#endif

using namespace std;

unsigned int Object::m_nObjects = 0;
bool Object::m_bUseLog = false;
std::map<std::string, int> Object::m_ObjectMap;




/**
 * Constructor
 */
Object::Object( std::string sClassName )
{
	++m_nObjects;
	m_sClassName = sClassName;
	m_pLogger = Logger::getInstance();

	if (m_bUseLog) {
		int nInstances = m_ObjectMap[ m_sClassName ];
		++nInstances;
		m_ObjectMap[ m_sClassName ] = nInstances;
	}
}


/**
 * Copy constructor
 */
Object::Object( const Object& obj )
{
	++m_nObjects;
	m_sClassName = obj.m_sClassName;
	m_pLogger = Logger::getInstance();

	if (m_bUseLog) {
		int nInstances = m_ObjectMap[ m_sClassName ];
		++nInstances;
		m_ObjectMap[ m_sClassName ] = nInstances;
	}
}


/**
 * Destructor
 */
Object::~Object()
{
	--m_nObjects;

	if (m_bUseLog) {
		int nInstances = m_ObjectMap[ m_sClassName ];
		--nInstances;
		m_ObjectMap[ m_sClassName ] = nInstances;
	}
}





/**
 * Return the number of Objects not deleted
 */
unsigned Object::getNObjects()
{
	return m_nObjects;
}



void Object::infoLog(const std::string& sMsg)
{
	if (m_bUseLog) {
		m_pLogger->info(this, sMsg);
	}
}


void Object::warningLog(const std::string& sMsg)
{
	m_pLogger->warning(this, sMsg);
}




void Object::errorLog(const std::string& sMsg)
{
	m_pLogger->error(this, sMsg);
}


void Object::useVerboseLog( bool bUse )
{
	m_bUseLog = bUse;
	Logger::getInstance()->m_bUseFile = bUse;
}

bool Object::isUsingVerboseLog()
{
	return m_bUseLog;
}



void Object::printObjectMap()
{
	std::cout << "[Object::printObjectMap]" << std::endl;

	map<std::string, int>::iterator iter = m_ObjectMap.begin();
	int nTotal = 0;
	do {
		int nInstances = (*iter).second;
		std::string sObject = (*iter).first;
		if ( nInstances != 0 ) {
			std::cout << nInstances << "\t" << sObject << std::endl;
		}
		nTotal += nInstances;
		iter++;
	} while ( iter != m_ObjectMap.end() );

	std::cout << "Total : " << nTotal << " objects." << std::endl;
}



////////////////////////


Logger* Logger::m_pInstance = NULL;

pthread_t loggerThread;

void* loggerThread_func(void* param)
{
//	std::cout << "LOGGER THREAD INIT" << std::endl;

	if ( param == NULL ) {
		return NULL;
	}

	Logger *pLogger = (Logger*)param;

	FILE *pLogFile = NULL;
	if ( pLogger->m_bUseFile ) {
		pLogFile = fopen( "hydrogen.log", "w" );
	}

	while ( pLogger->m_bIsRunning ) {
#ifdef WIN32
		Sleep( 1000 );
#else
		usleep( 100000 );
#endif

		vector<std::string>::iterator it;
		while ( (it  = pLogger->m_msgQueue.begin() ) != pLogger->m_msgQueue.end() ) {
			std::cout << *it;

			if ( pLogFile ) {
				fprintf( pLogFile, (*it).c_str() );
			}
			pLogger->m_msgQueue.erase( it );
		}
	}

	if ( pLogger->m_bUseFile ) {
		fclose( pLogFile );
	}

	pthread_exit(NULL);
	return NULL;
}


Logger* Logger::getInstance()
{
	if ( !m_pInstance ) {
		m_pInstance = new Logger();
	}
	return m_pInstance;
}


/**
 * Constructor
 */
Logger::Logger()
 : m_bIsRunning( true )
 , m_bUseFile( false )
{
//	std::cout << "LOGGER INIT" << std::endl;
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	pthread_create(&loggerThread, &attr, loggerThread_func, this);
}




/**
 * Destructor
 */
Logger::~Logger()
{
	m_bIsRunning = false;
	pthread_join(loggerThread, NULL);

//	std::cout << "LOGGER DESTROY" << std::endl;
}





/**
 * Simple log
 */
void Logger::log(std::string logMsg)
{
	m_msgQueue.push_back( logMsg.append("\n") );
}





/**
 * Information Log
 */
void Logger::info(Object *obj, const std::string& sLogMsg)
{
#ifndef WIN32
	std::string sMsg = std::string("\033[32m[INFO]      ") + obj->getClassName() +  std::string("   \t") + sLogMsg  + std::string("\033[0m\n");
#else
	std::string sMsg = std::string("[INFO]").append( obj->getClassName() ).append( "   \t" ).append( sLogMsg ).append( "\n" );
#endif
	m_msgQueue.push_back( sMsg );
}




/**
 * Warning Log
 */
void Logger::warning(Object *obj, const std::string& sLogMsg)
{
#ifndef WIN32
	std::string sMsg = std::string( "\033[36m[WARNING]   ") + obj->getClassName() + std::string("   \t") + sLogMsg  + std::string("\033[0m\n");
#else
	std::string sMsg = std::string("[WARNING]").append( obj->getClassName() ).append( "   \t" ).append( sLogMsg ).append( "\n" );
#endif
	m_msgQueue.push_back( sMsg );
}



/**
 * Error Log
 */
void Logger::error(Object *obj, const std::string& sErrorMsg)
{
#ifndef WIN32
	std::string sMsg = std::string("\033[31m[ERROR]     ") + obj->getClassName() + std::string("   \t") + sErrorMsg + std::string("\033[0m\n");
#else
	std::string sMsg = std::string("[ERROR]").append( obj->getClassName() ).append( "   \t" ).append( sErrorMsg ).append( "\n" );
#endif
	m_msgQueue.push_back( sMsg );
}

