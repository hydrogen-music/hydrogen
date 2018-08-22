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

#ifndef H2C_OBJECT_H
#define H2C_OBJECT_H

#include "hydrogen/logger.h"
#include "hydrogen/config.h"
#include "hydrogen/globals.h"

#include <unistd.h>
#include <iostream>
#include <QtCore>

namespace H2Core {

/**
 * Base class.
 */
class Object {
	public:
		/** destructor */
		~Object();
		/** copy constructor */
		Object( const Object& obj );
		/** constructor */
		Object( const char* class_name );

		const char* class_name( ) const         { return __class_name; }        ///< return the class name
		/**
		 * enable/disable class instances counting
		 * \param flag the counting status to set
		 */
		static void set_count( bool flag );
		static bool count_active()              { return __count; }             ///< return true if class instances counting is enabled
		static unsigned objects_count()         { return __objects_count; }     ///< return the number of objects

		/**
		 * output the full objects map to a given ostream
		 * \param out the ostream to write to
		 */
		static void write_objects_map_to( std::ostream& out );
		static void write_objects_map_to_cerr() { Object::write_objects_map_to( std::cerr ); }  ///< ouput objects map to stderr

		/**
		 * must be called before any Object instanciation !
		 * \param logger the logger instance used to send messages to
		 * \param count should we count objects instances or not
		 */
		static int bootstrap( Logger* logger, bool count=false );
		static Logger* logger()                 { return __logger; }            ///< return the logger instance

	private:
		/**
		 * search for the class name within __objects_map, decrease class and global counts
		 * \param obj the object to be taken into account
		 */
		static void del_object( const Object* obj );
		/**
		 * search for the clas name within __objects_map, create it if doesn't exists, increase class and global counts
		 * \param obj the object to be taken into account
		 * \param copy is it called from a copy constructor
		 */
		static void add_object( const Object* obj, bool copy );

		/** an objects class map item type */
		typedef struct {
			unsigned constructed;
			unsigned destructed;
		} obj_cpt_t;
		/** the objects class map type */
		typedef std::map<const char*, obj_cpt_t> object_map_t;

		const char* __class_name;               ///< the object class name
		static bool __count;                    ///< should we count class instances
		static unsigned __objects_count;        ///< total objects count
		static object_map_t __objects_map;      ///< objects classes and instances count structure
		static pthread_mutex_t __mutex;         ///< yeah this has to be thread safe

	protected:
		static Logger* __logger;                ///< logger instance pointer
};

// Object inherited class declaration macro
#define H2_OBJECT                                                       \
	public: static const char* class_name() { return __class_name; }    \
	private: static const char* __class_name;                           \

// LOG MACROS
#define __LOG_METHOD(   lvl, msg )  if( __logger->should_log( (lvl) ) )                 { __logger->log( (lvl), class_name(), __FUNCTION__, msg ); }
#define __LOG_CLASS(    lvl, msg )  if( logger()->should_log( (lvl) ) )                 { logger()->log( (lvl), class_name(), __FUNCTION__, msg ); }
#define __LOG_OBJ(      lvl, msg )  if( __object->logger()->should_log( (lvl) ) )       { __object->logger()->log( (lvl), 0, __PRETTY_FUNCTION__, msg ); }
#define __LOG_STATIC(   lvl, msg )  if( H2Core::Logger::get_instance()->should_log( (lvl) ) )   { H2Core::Logger::get_instance()->log( (lvl), 0, __PRETTY_FUNCTION__, msg ); }
#define __LOG( logger,  lvl, msg )  if( (logger)->should_log( (lvl) ) )                 { (logger)->log( (lvl), 0, 0, msg ); }

// Object instance method logging macros
#define DEBUGLOG(x)     __LOG_METHOD( H2Core::Logger::Debug,   (x) );
#define INFOLOG(x)      __LOG_METHOD( H2Core::Logger::Info,    (x) );
#define WARNINGLOG(x)   __LOG_METHOD( H2Core::Logger::Warning, (x) );
#define ERRORLOG(x)     __LOG_METHOD( H2Core::Logger::Error,   (x) );

// Object class method logging macros
#define _DEBUGLOG(x)    __LOG_CLASS( H2Core::Logger::Debug,   (x) );
#define _INFOLOG(x)     __LOG_CLASS( H2Core::Logger::Info,    (x) );
#define _WARNINGLOG(x)  __LOG_CLASS( H2Core::Logger::Warning, (x) );
#define _ERRORLOG(x)    __LOG_CLASS( H2Core::Logger::Error,   (x) );

// logging macros using an Object *__object ( thread :  Object* __object = ( Object* )param; )
#define __DEBUGLOG(x)   __LOG_OBJ( H2Core::Logger::Debug,      (x) );
#define __INFOLOG(x)    __LOG_OBJ( H2Core::Logger::Info,       (x) );
#define __WARNINGLOG(x) __LOG_OBJ( H2Core::Logger::Warning,    (x) );
#define __ERRORLOG(x)   __LOG_OBJ( H2Core::Logger::Error,      (x) );

// logging macros using  ( thread :  Object* __object = ( Object* )param; )
#define ___DEBUGLOG(x)  __LOG_STATIC( H2Core::Logger::Debug,    (x) );
#define ___INFOLOG(x)   __LOG_STATIC( H2Core::Logger::Info,     (x) );
#define ___WARNINGLOG(x) __LOG_STATIC(H2Core::Logger::Warning,  (x) );
#define ___ERRORLOG(x)  __LOG_STATIC( H2Core::Logger::Error,    (x) );

};

#endif // H2C_OBJECT_H

/* vim: set softtabstop=4 noexpandtab: */
