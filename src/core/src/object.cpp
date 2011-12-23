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

#include "hydrogen/object.h"

#include <cassert>
#include <sstream>
#include <iomanip>
#include <cstdlib>


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

Logger* Object::__logger = 0;
bool Object::__count = false;
unsigned Object::__objects_count = 0;
pthread_mutex_t Object::__mutex;
Object::object_map_t Object::__objects_map;

int Object::bootstrap( Logger* logger, bool count ) {
    if( __logger==0 && logger!=0 ) {
        __logger = logger;
        __count = count;
        pthread_mutex_init( &__mutex, 0 );
        return 0;
    }
    return 1;
}

Object::~Object( ) {
#ifdef H2CORE_HAVE_DEBUG
    if( __count ) del_object( this );
#endif
}

Object::Object( const Object& obj ) : __class_name( obj.__class_name ) {
#ifdef H2CORE_HAVE_DEBUG
    if( __count ) add_object( this, true );
#endif
}

Object::Object( const char* class_name ) :__class_name( class_name ) {
#ifdef H2CORE_HAVE_DEBUG
    if( __count ) add_object( this, false );
#endif
}

void Object::set_count( bool flag ) {
#ifdef H2CORE_HAVE_DEBUG
    __count = flag;
#else
    if( __logger!=0 && __logger->should_log( Logger::Error ) ) {
        __logger->log(  Logger::Error, "set_count", "Object", "not compiled with H2CORE_HAVE_DEBUG flag set" );
    }
#endif
}

inline void Object::add_object( const Object* obj, bool copy ) {
#ifdef H2CORE_HAVE_DEBUG
    const char* class_name = ( ( Object* )obj )->class_name();
    if( __logger && __logger->should_log( Logger::Constructors ) ) __logger->log( Logger::Debug, 0, class_name, ( copy ? "Copy Constructor" : "Constructor" ) );
    pthread_mutex_lock( &__mutex );
    //if( __objects_map.size()==0) atexit( Object::write_objects_map_to_cerr );
    __objects_count++;
    __objects_map[ class_name ].constructed++;
    pthread_mutex_unlock( &__mutex );
#endif
}

inline void Object::del_object( const Object* obj ) {
#ifdef H2CORE_HAVE_DEBUG
    const char* class_name = ( ( Object* )obj )->class_name();
    if( __logger && __logger->should_log( Logger::Constructors ) ) __logger->log( Logger::Debug, 0, class_name, "Destructor" );
    object_map_t::iterator it_count = __objects_map.find( class_name );
    if ( it_count==__objects_map.end() ) {
        if( __logger!=0 && __logger->should_log( Logger::Error ) ) {
            std::stringstream msg;
            msg << "the class " <<  class_name << " is not registered ! [" << obj << "]";
            __logger->log( Logger::Error,"del_object", "Object", QString::fromStdString( msg.str() ) );
        }
        return;
    }
    assert( ( *it_count ).first == class_name );
    pthread_mutex_lock( &__mutex );
    assert( __objects_map[class_name].constructed > ( __objects_map[class_name].destructed ) );
    __objects_count--;
    assert( __objects_count>=0 );
    __objects_map[ ( *it_count ).first ].destructed++;
    pthread_mutex_unlock( &__mutex );
#endif
}
void Object::write_objects_map_to( std::ostream& out ) {
#ifdef H2CORE_HAVE_DEBUG
    if( !__count ) {
#ifdef WIN32
        out << "level must be Debug or higher"<< std::endl;
#else
        out << "\033[35mlog level must be \033[31mDebug\033[35m or higher\033[0m"<< std::endl;
#endif
        return;
    }
    std::ostringstream o;
    pthread_mutex_lock( &__mutex );
    object_map_t::iterator it = __objects_map.begin();
    while ( it != __objects_map.end() ) {
        o << "\t[ " << std::setw( 30 ) << ( *it ).first << " ]\t" << std::setw( 6 ) << ( *it ).second.constructed << "\t" << std::setw( 6 ) << ( *it ).second.destructed
          << "\t" << std::setw( 6 ) << ( *it ).second.constructed - ( *it ).second.destructed << std::endl;
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
    out << "Object::write_objects_map_to :: not compiled with H2CORE_HAVE_DEBUG flag set" << std::endl;
#else
    out << "\033[35mObject::write_objects_map_to :: \033[31mnot compiled with H2CORE_HAVE_DEBUG flag set\033[0m" << std::endl;
#endif
#endif
}

};

/* vim: set softtabstop=4 expandtab: */
