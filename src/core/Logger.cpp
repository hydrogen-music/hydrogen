/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include "core/Logger.h"
#include "core/Helpers/Filesystem.h"

#include <cstdio>
#include <chrono>
#include <thread>
#include <QtCore/QDir>
#include <QtCore/QString>

#ifdef WIN32
#include <windows.h>
#endif

namespace H2Core {

unsigned Logger::__bit_msk = 0;
Logger* Logger::__instance=nullptr;
const char* Logger::__levels[] = { "None", "Error", "Warning", "Info", "Debug", "Constructors", "Locks" };
thread_local QString *Logger::pCrashContext = nullptr;

pthread_t loggerThread;

void* loggerThread_func( void* param ) {
	if ( param == nullptr ) return nullptr;
	Logger* logger = ( Logger* )param;
#ifdef WIN32
#  ifdef H2CORE_HAVE_DEBUG
	::AllocConsole();
//	::SetConsoleTitle( "Hydrogen debug log" );
	freopen( "CONOUT$", "wt", stdout );
#  endif
#endif
	FILE* log_file = nullptr;
	if ( logger->__use_file ) {

		QString sLogFilename = Filesystem::log_file_path();
				
		log_file = fopen( sLogFilename.toLocal8Bit(), "w" );
		if ( log_file ) {
			fprintf( log_file, "Start logger" );
		} else {
			fprintf( stderr, "Error: can't open log file for writing...\n" );
		}
	}
	Logger::queue_t* queue = &logger->__msg_queue;
	Logger::queue_t::iterator it, last;

	while ( logger->__running ) {
		pthread_mutex_lock( &logger->__mutex );
		pthread_cond_wait( &logger->__messages_available, &logger->__mutex );
		pthread_mutex_unlock( &logger->__mutex );
		if( !queue->empty() ) {
			for( it = last = queue->begin() ; it != queue->end() ; ++it ) {
				last = it;
				fprintf( stdout, "%s", it->toLocal8Bit().data() );
				if( log_file ) {
					fprintf( log_file, "%s", it->toLocal8Bit().data() );
					fflush( log_file );
				}
			}
			// remove all in front of last
			pthread_mutex_lock( &logger->__mutex );
			queue->erase( queue->begin(), last );
			queue->pop_front();
			pthread_mutex_unlock( &logger->__mutex );
		}
	}
	if ( log_file ) {
		fprintf( log_file, "Stop logger" );
		fclose( log_file );
	}
#ifdef WIN32
	::FreeConsole();
#endif

	fflush( stdout );
	pthread_exit( nullptr );
	return nullptr;
}

Logger* Logger::bootstrap( unsigned msk ) {
	Logger::set_bit_mask( msk );
	return Logger::create_instance();
}

Logger* Logger::create_instance() {
	if ( __instance == nullptr ) __instance = new Logger;
	return __instance;
}

Logger::Logger() : __use_file( true ), __running( true ) {
	__instance = this;
	pthread_attr_t attr;
	pthread_attr_init( &attr );
	pthread_mutex_init( &__mutex, nullptr );
	pthread_cond_init( &__messages_available, nullptr );
	pthread_create( &loggerThread, &attr, loggerThread_func, this );
}

Logger::~Logger() {
	__running = false;
	pthread_cond_broadcast ( &__messages_available );
	pthread_join( loggerThread, nullptr );
}

void Logger::log( unsigned level, const QString& class_name, const char* func_name, const QString& msg ) {

	if( level == None ){
		return;
	}

	const char* prefix[] = { "", "(E) ", "(W) ", "(I) ", "(D) ", "(C)", "(L) " };
#ifdef WIN32
	const char* color[] = { "", "", "", "", "", "", "" };
#else
	const char* color[] = { "", "\033[31m", "\033[36m", "\033[32m", "\033[35m", "\033[35;1m", "\033[35;1m" };
#endif // WIN32

	int i;
	switch( level ) {
	case Error:
		i = 1;
		break;
	case Warning:
		i = 2;
		break;
	case Info:
		i = 3;
		break;
	case Debug:
		i = 4;
		break;
	case Constructors:
		i = 5;
		break;
	case Locks:
		i = 6;
		break;
	default:
		i = 0;
		break;
	}

	QString tmp = QString( "%1%2%3::%4 %5\033[0m\n" )
				  .arg( color[i] )
				  .arg( prefix[i] )
				  .arg( class_name )
				  .arg( func_name )
				  .arg( msg );

	pthread_mutex_lock( &__mutex );
	__msg_queue.push_back( tmp );
	pthread_mutex_unlock( &__mutex );
	pthread_cond_broadcast( &__messages_available );
}

void Logger::flush() const {

	int nTimeout = 100;
	for ( int ii = 0; ii < nTimeout; ++ii ) {
		if ( __msg_queue.empty() ) {
			break;
		}

		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
	}
	return;
}

unsigned Logger::parse_log_level( const char* level ) {
	unsigned log_level = Logger::None;
	if( 0 == strncasecmp( level, __levels[0], strlen( __levels[0] ) ) ) {
		log_level = Logger::None;
	} else if ( 0 == strncasecmp( level, __levels[1], strlen( __levels[1] ) ) ) {
		log_level = Logger::Error;
	} else if ( 0 == strncasecmp( level, __levels[2], strlen( __levels[2] ) ) ) {
		log_level = Logger::Error | Logger::Warning;
	} else if ( 0 == strncasecmp( level, __levels[3], strlen( __levels[3] ) ) ) {
		log_level = Logger::Error | Logger::Warning | Logger::Info;
	} else if ( 0 == strncasecmp( level, __levels[4], strlen( __levels[4] ) ) ) {
		log_level = Logger::Error | Logger::Warning | Logger::Info | Logger::Debug;
	} else if ( 0 == strncasecmp( level, __levels[5], strlen( __levels[5] ) ) ) {
		log_level = Logger::Error | Logger::Warning | Logger::Info | Logger::Debug | Logger::Constructors;
	} else if ( 0 == strncasecmp( level, __levels[6], strlen( __levels[6] ) ) ) {
		log_level = Logger::Error | Logger::Warning | Logger::Info | Logger::Debug | Logger::Locks;
	} else {
#ifdef HAVE_SSCANF
		int val = sscanf( level,"%x",&log_level );
		if( val != 1 ) {
			log_level = Logger::Error;
		}
#else
		log_level = hextoi( level, -1 );
		if( log_level==-1 ) {
			log_level = Logger::Error;
		}
#endif
	}
	return log_level;
}

#ifndef HAVE_SSCANF
int Logger::hextoi( const char* str, long len ) {
	long pos = 0;
	char c = 0;
	int v = 0;
	int res = 0;
	bool leading_zero = false;

	while( 1 ) {
		if( ( len!=-1 ) && ( pos>=len ) ) {
			break;
		}
		c = str[pos];
		if( c==0 ) {
			break;
		} else if( c=='x' || c=='X' ) {
			if ( ( pos==1 ) && leading_zero ) {
				assert( res == 0 );
				pos++;
				continue;
			} else {
				return -1;
			}
		} else if( c>='a' ) {
			v = c-'a'+10;
		} else if( c>='A' ) {
			v = c-'A'+10;
		} else if( c>='0' ) {
			if ( ( c=='0' ) && ( pos==0 ) ) {
				leading_zero = true;
			}
			v = c-'0';
		} else {
			return -1;
		}
		if( v>15 ) {
			return -1;
		}
		//assert( v == (v & 0xF) );
		res = ( res << 4 ) | v;
		assert( ( res & 0xF ) == ( v & 0xF ) );
		pos++;
	}
	return res;
}
#endif // HAVE_SSCANF


Logger::CrashContext::CrashContext( QString *pContext ) {
	pSavedContext = Logger::pCrashContext;
	Logger::pCrashContext = pContext;
	pThisContext = nullptr;
}

Logger::CrashContext::CrashContext( QString sContext ) {
	pSavedContext = Logger::pCrashContext;
	// Copy context string
	pThisContext = new QString( sContext );
	Logger::pCrashContext = pThisContext;
}

Logger::CrashContext::~CrashContext() {
	Logger::pCrashContext = pSavedContext;
	if ( pThisContext ) {
		delete pThisContext;
	}
}

};

/* vim: set softtabstop=4 noexpandtab: */
