/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "core/Object.h"

#include <cassert>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <typeinfo>

#ifdef WIN32
#    include "core/Timehelper.h"
#else
#    include <unistd.h>
#    include <sys/time.h>
#endif

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

/**
* @class Object
*
* @brief Base class of all components of hydrogen.
*
* Every component of hydrogen is inherited from the
* Object class. Each object has a qualified name
* and gets registered in a memory map at creation.
* This memory map helps to debug memory leaks and
* can be printed at any time.
*
*/

namespace H2Core {

Logger* Base::__logger = nullptr;
bool Base::__count = false;
std::atomic<int> Base::__objects_count(0);
pthread_mutex_t Base::__mutex;
object_internal_map_t Base::__objects_map;
QString Base::sPrintIndention = "  ";
timeval Base::__last_clock = { 0, 0 };

int Base::bootstrap( Logger* logger, bool count ) {
	if( __logger==nullptr && logger!=nullptr ) {
		__logger = logger;
		__count = count;
		pthread_mutex_init( &__mutex, nullptr );
		return 0;
	}
	return 1;
}

void Base::set_count( bool flag ) {
#ifdef H2CORE_HAVE_DEBUG
	__count = flag;
#else
	if( __logger!=0 && __logger->should_log( Logger::Error ) ) {
		__logger->log(  Logger::Error, "set_count", "Object", "not compiled with H2CORE_HAVE_DEBUG flag set" );
	}
#endif
}

void Base::write_objects_map_to( std::ostream& out, object_map_t* map ) {
#ifdef H2CORE_HAVE_DEBUG
	if( !__count ) {
#ifdef WIN32
		out << "level must be Debug or higher"<< std::endl;
#else
		out << "\033[35mlog level must be \033[31mDebug\033[35m or higher\033[0m"<< std::endl;
#endif
		return;
	}
	object_map_t snapshot;
	if ( map == nullptr ) {
		snapshot = getObjectMap();
		map = &snapshot;
	}
	
	std::ostringstream o;
	pthread_mutex_lock( &__mutex );
	object_map_t::iterator it = map->begin();
	while ( it != map->end() ) {
		if ( it->second.constructed || it->second.destructed ) {
			o << "\t[ " << std::setw( 30 ) << ( *it ).first << " ]\t" << std::setw( 6 ) << ( *it ).second.constructed << "\t" << std::setw( 6 ) << ( *it ).second.destructed
			  << "\t" << std::setw( 6 ) << ( *it ).second.constructed - ( *it ).second.destructed << std::endl;
		}
		it++;
	}
	pthread_mutex_unlock( &__mutex );
#ifndef WIN32
	out << std::endl << "\033[35m";
#endif
	out << "Objects map :" << std::setw( 30 ) << "class\t" << "constr   destr   alive" << std::endl << o.str() << "Total : " << std::setw( 6 ) << __objects_count << " objects.";
#ifndef WIN32
	out << "\033[0m";
#endif
	out << std::endl << std::endl;
#else
#ifdef WIN32
	out << "Base::write_objects_map_to :: not compiled with H2CORE_HAVE_DEBUG flag set" << std::endl;
#else
	out << "\033[35mBase::write_objects_map_to :: \033[31mnot compiled with H2CORE_HAVE_DEBUG flag set\033[0m" << std::endl;
#endif
#endif
}

int Base::getAliveObjectCount() {
#ifdef H2CORE_HAVE_DEBUG
	int nCount = 0;
	for (const auto& ii : __objects_map ) {
		if (!strcmp(ii.first, "Object")) {
			return ii.second->constructed - ii.second->destructed;
		}
	}
	return nCount;
#else
	ERRORLOG( "This function is only supported in debug builds of Hydrogen." );
	return 0;
#endif
}

object_map_t Base::getObjectMap() {
	object_map_t mapCopy;
	obj_cpt_t copy;
	
	for ( auto const& ii : __objects_map ) {
		copy.constructed = ii.second->constructed;
		copy.destructed = ii.second->destructed;
		mapCopy.insert( std::pair<const char*, obj_cpt_t>( ii.first, copy ) );
	}
	
	return mapCopy;
}

void Base::printObjectMapDiff( const object_map_t& mapSnapshot ) {
	object_map_t mapDiff;
	obj_cpt_t diff;

	// Since key value pairs are only inserted but not erased while
	// Hydrogen is running it is save to assume for mapSnapshot to be
	// a subset of the current object map.
	for ( const auto& ii : __objects_map ) {
		auto it = mapSnapshot.find( ii.first );
		if ( it != mapSnapshot.end() ) {
			diff.constructed = ii.second->constructed - it->second.constructed;
			diff.destructed = ii.second->destructed - it->second.destructed;
			mapDiff.insert( std::pair<const char*, obj_cpt_t>( ii.first, diff ) );
		}
	}

	write_objects_map_to( std::cout, &mapDiff );
}

QString Base::toQString( const QString& sPrefix, bool bShort ) const {
	return QString( "[%1] instances alive: %2" )
		.arg( class_name() ).arg( count_active() );
}

void Base::Print( bool bShort ) const {
	DEBUGLOG( toQString( "", bShort ) );
}

void Base::logBacktrace() const {
#ifdef HAVE_EXECINFO_H
	const int nMaxFrames = 128;
	void *frames[ nMaxFrames ];
	int nFrames = backtrace( frames, nMaxFrames );
	char **symbols = backtrace_symbols( frames, nFrames );
	for ( int i = 0; i < nFrames; i++ ) {
		DEBUGLOG( QString("%1").arg( symbols[i] ) );
	}
#else
	DEBUGLOG( "Compiled without backtrace support" );
#endif
}

QString Base::base_clock_in( const QString& sMsg )
{
	struct timeval now;
	gettimeofday( &now, nullptr );
	__last_clock = now;

	QString sResult( "Start clocking" );
	if ( ! sMsg.isEmpty() ) {
		sResult = QString( "%1: %2" ).arg( sMsg ).arg( sResult );
	}

	return std::move( sResult );
}

QString Base::base_clock( const QString& sMsg )
{
	struct timeval now;
	gettimeofday( &now, nullptr );

	QString sResult;
	if ( __last_clock.tv_sec == 0 && __last_clock.tv_usec == 0 ) {
		// Clock is invoked for the first time.
		sResult = "Start clocking";
	} else {
		sResult =  QString( "elapsed [%1]ms" )
			.arg( ( now.tv_sec - __last_clock.tv_sec ) * 1000.0 +
				  ( now.tv_usec - __last_clock.tv_usec ) / 1000.0 );
	}

	__last_clock = now;

	if ( ! sMsg.isEmpty() ) {
		sResult = QString( "%1: %2" ).arg( sMsg ).arg( sResult );
	}

	return std::move( sResult );
}

std::ostream& operator<<( std::ostream& os, const Base& object ) {
	return os << object.toQString( "", true ).toLocal8Bit().data() << std::endl;
}

std::ostream& operator<<( std::ostream& os, const Base* object ) {
	return os << object->toQString( "", true ).toLocal8Bit().data() << std::endl;
}

void Base::registerClass(const char *name, const atomic_obj_cpt_t *counters)
{
	if ( ! counters ) {
		qWarning() << "Base::registerClass: " << name << " null counters!";
	}
	if ( counters->constructed ) {
		// already registered by another thread
		return;
	}
	if ( ! __objects_map[name] ) {
	__objects_map[name] = counters;
	} else {
		qWarning() << "Base::registerClass: " << name << " already registered";
	}
}

}; // namespace H2Core

/* vim: set softtabstop=4 noexpandtab: */
