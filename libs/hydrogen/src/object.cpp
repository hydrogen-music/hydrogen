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

#include <hydrogen/Object.h>

#include <QDir>
#include <iostream>
#include <pthread.h>

#ifdef WIN32
	#include <windows.h>
#else
	#include <unistd.h>
#endif

using namespace std;

unsigned int Object::m_nObjects = 0;
bool Object::m_bUseLog = false;
std::map<std::string, int> Object::m_ObjectMap;




/**
 * Constructor
 */
Object::Object( const std::string& sClassName )
 : m_sClassName( sClassName )
{
	++m_nObjects;
	m_pLogger = Logger::getInstance();

#ifdef WIN32
	Object::useVerboseLog( true );
#endif

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
#ifdef WIN32
	useVerboseLog( true );
#endif
	m_sClassName = obj.getClassName();

	++m_nObjects;
//	m_sClassName = obj.getClassName;
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
int Object::getNObjects()
{
	return m_nObjects;
}


/*
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
*/


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
	if ( param == NULL ) {
		// ??????
		return NULL;
	}

#ifdef WIN32
	::AllocConsole();
//	::SetConsoleTitle( "Hydrogen debug log" );
	freopen( "CONOUT$", "wt", stdout);
#endif

	Logger *pLogger = (Logger*)param;

	FILE *pLogFile = NULL;
	if ( pLogger->m_bUseFile ) {
#ifdef Q_OS_MACX
	  	string sLogFilename = QDir::homePath().append("/Library/Hydrogen/hydrogen.log").toStdString();
#else
		string sLogFilename = QDir::homePath().append("/.hydrogen/hydrogen.log").toStdString();
#endif

		pLogFile = fopen( sLogFilename.c_str(), "w" );
		if ( pLogFile == NULL ) {
			std::cerr << "Error: can't open log file for writing..." << std::endl;
		}
		else {
			fprintf( pLogFile, "Start logger" );
		}
	}

	while ( pLogger->m_bIsRunning ) {
#ifdef WIN32
		Sleep( 1000 );
#else
		usleep( 1000000 );
#endif
		pthread_mutex_lock( &pLogger->m_logger_mutex );


		vector<std::string>::iterator it;
		string tmpString;
		while ( (it  = pLogger->m_msgQueue.begin() ) != pLogger->m_msgQueue.end() ) {
			tmpString = *it;
			pLogger->m_msgQueue.erase( it );
			printf( tmpString.c_str() );

			if ( pLogFile ) {
				fprintf( pLogFile, tmpString.c_str() );
				fflush( pLogFile );
			}
		}
		pthread_mutex_unlock( &pLogger->m_logger_mutex );
	}

	if ( pLogFile ) {
		fprintf( pLogFile, "Stop logger" );
		fclose( pLogFile );
	}
#ifdef WIN32
	::FreeConsole();
#endif


#ifdef WIN32
		Sleep( 1000 );
#else
		usleep( 100000 );
#endif

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
 : m_bUseFile( false )
 , m_bIsRunning( true )
{
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	pthread_mutex_init( &m_logger_mutex, NULL );


	pthread_create(&loggerThread, &attr, loggerThread_func, this);
}




/**
 * Destructor
 */
Logger::~Logger()
{
	m_bIsRunning = false;
	pthread_join(loggerThread, NULL);

}





void Logger::infoLog( const std::string& logMsg )
{
	if ( !Object::isUsingVerboseLog() ) return;

	pthread_mutex_lock( &m_logger_mutex );

#ifdef WIN32
	m_msgQueue.push_back( logMsg + "\n" );
#else
	m_msgQueue.push_back( string( "\033[32m" ) + logMsg + string("\033[0m").append("\n") );
#endif

	pthread_mutex_unlock( &m_logger_mutex );

}




void Logger::warningLog( const std::string& logMsg )
{
	pthread_mutex_lock( &m_logger_mutex );

	#ifdef WIN32
	m_msgQueue.push_back( logMsg + "\n" );
#else
	m_msgQueue.push_back( string( "\033[36m" ) + logMsg + string("\033[0m").append("\n") );
#endif

	pthread_mutex_unlock( &m_logger_mutex );
}



void Logger::errorLog( const std::string& logMsg )
{
	pthread_mutex_lock( &m_logger_mutex );

#ifdef WIN32
	m_msgQueue.push_back( logMsg + "\n" );
#else
	m_msgQueue.push_back( string( "\033[31m" ) + logMsg + string("\033[0m").append("\n") );
#endif

	pthread_mutex_unlock( &m_logger_mutex );
}




/*
#ifndef WIN32


static const WORD bgMask( BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY );


void Logger__setREDColor()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	HANDLE hCon = GetStdHandle( STD_OUTPUT_HANDLE );
	GetConsoleScreenBufferInfo(hCon, &csbi);

//	WORD wRGBI = GetRGBI(fgColor, FOREGROUND_INTENSITY, FOREGROUND_RED, FOREGROUND_GREEN, FOREGROUND_BLUE);
	WORD color = 10	| 255;
	csbi.wAttributes &= bgMask;
	csbi.wAttributes |= color;

	SetConsoleTextAttribute( hCon, csbi.wAttributes );

//	wRGBI = GetRGBI(bgColor, BACKGROUND_INTENSITY, BACKGROUND_RED, BACKGROUND_GREEN, BACKGROUND_BLUE);

//	csbi.wAttributes &= fgMask;
//	csbi.wAttributes |= wRGBI;

	SetConsoleTextAttribute( hCon, csbi.wAttributes );
}


#endif
*/

