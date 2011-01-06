/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include "hydrogen/logger.h"

#include <cstdio>
#include <QtCore/QDir>
#include <QtCore/QString>

#ifdef WIN32
#include <windows.h>
#define LOGGER_SLEEP Sleep( 100 )
#else
#include <unistd.h>
#define LOGGER_SLEEP usleep( 1000000 )
#endif

unsigned Logger::__bit_msk = 0;
Logger* Logger::__instance=0;
const char* Logger::__levels[] = { "None", "Error", "Warning", "Info", "Debug" };

pthread_t loggerThread;

void* loggerThread_func( void* param ) {
	if ( param == 0 ) return 0;
	Logger *logger = ( Logger* )param;
#ifdef WIN32
	::AllocConsole();
//	::SetConsoleTitle( "Hydrogen debug log" );
	freopen( "CONOUT$", "wt", stdout );
#endif
	FILE *log_file = 0;
	if ( logger->__use_file ) {
        // TODO
#ifdef Q_OS_MACX
		QString sLogFilename = QDir::homePath().append( "/Library/Hydrogen/hydrogen.log" );
#else
		QString sLogFilename = QDir::homePath().append( "/.hydrogen/hydrogen.log" );
#endif
		log_file = fopen( sLogFilename.toLocal8Bit(), "w" );
		if ( log_file ) {
			fprintf( log_file, "Start logger" );
		} else {
			fprintf( stderr, "Error: can't open log file for writing...\n" );
		}
	}
	Logger::queue_t* queue = &logger->__msg_queue;
    Logger::queue_t::iterator it, last;
    QString tmpString;
	while ( logger->__running ) {
        LOGGER_SLEEP;
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
		    queue->erase( queue->begin(), last );
            // lock before removing last
		    pthread_mutex_lock( &logger->__mutex );
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
    LOGGER_SLEEP;
	pthread_exit( 0 );
	return 0;
}

Logger* Logger::bootstrap( unsigned msk ) {
    Logger::set_bit_mask( msk );
    return Logger::create_instance();
    return Logger::get_instance();
}

Logger* Logger::create_instance() {
    if ( __instance == 0 ) __instance = new Logger;
}

Logger::Logger() : __use_file( false ), __running( true ) {
	__instance = this;
	pthread_attr_t attr;
	pthread_attr_init( &attr );
	pthread_mutex_init( &__mutex, 0 );
	pthread_create( &loggerThread, &attr, loggerThread_func, this );
}

Logger::~Logger() {
	__running = false;
	pthread_join( loggerThread, 0 );
}

void Logger::log( unsigned level, const QString& class_name, const char* func_name, const QString& msg ) {
	if( level == None ) return;
	const char* prefix[] = { "", "(E) ", "(W) ", "(I) ", "(D) " };
#ifdef WIN32
	const char* color[] = { "", "", "", "", "" };
#else
	const char* color[] = { "", "\033[31m", "\033[36m", "\033[32m", "\033[35m" };
#endif // WIN32

	int i;
	switch(level) {
	case None:
		assert(false);
		i = 0;
		break;
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
	default:
		i = 0;
		break;
	}

	QString tmp = QString("%1%2%3::%4 %5\033[0m\n")
		.arg(color[i])
		.arg(prefix[i])
		.arg(class_name)
		.arg(func_name)
		.arg(msg);

	pthread_mutex_lock( &__mutex);
	__msg_queue.push_back( tmp );
	pthread_mutex_unlock( &__mutex );
}

unsigned Logger::parse_log_level(const char* level) {
	unsigned log_level;
	if( 0 == strncasecmp( level, __levels[0], sizeof(__levels[0]) ) ) {
		log_level = Logger::None;
	} else if ( 0 == strncasecmp( level, __levels[1], sizeof(__levels[1]) ) ) {
		log_level = Logger::Error;
	} else if ( 0 == strncasecmp( level, __levels[2], sizeof(__levels[2]) ) ) {
		log_level = Logger::Error | Logger::Warning;
	} else if ( 0 == strncasecmp( level, __levels[3], sizeof(__levels[3]) ) ) {
		log_level = Logger::Error | Logger::Warning | Logger::Info;
	} else if ( 0 == strncasecmp( level, __levels[4], sizeof(__levels[4]) ) ) {
		log_level = Logger::Error | Logger::Warning | Logger::Info | Logger::Debug;
	} else {
#ifdef HAVE_SSCANF
		int val = sscanf(level,"%x",&log_level);
		if( val != 1 ) {
			log_level = Logger::Error;
	    }
#else
        int log_level = hextoi( level, -1 );
        if( log_level==-1 ) {
			log_level = Logger::Error;
        }
#endif
	}
    return log_level;
}

#ifndef HAVE_SSCANF
int Logger::hextoi(const char* str, long len) {
    long pos = 0;
    char c = 0;
    int v = 0;
    int res = 0;
    bool leading_zero = false;

    while(1) {
        if((len!=-1) && (pos>=len) ) {
            break;
        }
        c = str[pos];
        if(c==0) {
            break;
        } else if(c=='x' || c=='X') {
            if ( (pos==1) && leading_zero ) {
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
            if ( (c=='0') && (pos==0) ) {
                leading_zero = true;
            }
            v = c-'0';
        } else {
            return -1;
        }
        if(v>15) { return -1; }
        //assert( v == (v & 0xF) );
        res = (res << 4) | v;
        assert( (res & 0xF) == (v & 0xF) );
        pos++;
    }
    return res;
}
#endif // HAVE_SSCANF

