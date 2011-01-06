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

#ifndef H2_OBJECT_H
#define H2_OBJECT_H

#include "hydrogen/logger.h"

#include <map>
#include <iostream>
#include <QtCore>

/**
 * Base class.
 */
class Object {
    public:
        ~Object();
        Object( const Object& obj );
        Object( const char* class_name );

        const char* class_name( ) { return __class_name;  }

	    static void set_count( bool status );
	    static bool count_active() { return __count; };
        static unsigned objects_count() { return __objects_count; };

        static void write_objects_map_to( std::ostream &out );
        static void write_objects_map_to_cerr() { Object::write_objects_map_to( std::cerr ); }

        /**
         * must be called before any Object instanciation !
         */
        static int bootstrap( Logger *logger, bool count=false );
        static Logger* logger() { return __logger; }

    private:
        const char* __class_name;   /* without this, destructor will need a map ref=>class name */
        static void del_object( const Object* );
        static void add_object( const Object*, bool );

        static bool __count;
        static unsigned __objects_count;
        typedef struct { unsigned constructed; unsigned destructed; } obj_cpt_t;
        typedef std::map<const char*, obj_cpt_t> object_map_t;
        static object_map_t __objects_map;
	    static pthread_mutex_t __mutex;

    protected:
        static Logger* __logger;
};

/* Object inherited class declaration macro */
#define H2_OBJECT                                                       \
    public: static const char* class_name() { return __class_name; }    \
    private: static const char* __class_name;                           \

// LOG MACROS
#define __LOG_METHOD(   lvl, msg )  if( __logger->should_log( (lvl) ) )                 { __logger->log( (lvl), class_name(), __FUNCTION__, msg ); }
#define __LOG_CLASS(    lvl, msg )  if( logger()->should_log( (lvl) ) )                 { logger()->log( (lvl), class_name(), __FUNCTION__, msg ); }
#define __LOG_OBJ(      lvl, msg )  if( __object->logger()->should_log( (lvl) ) )       { __object->logger()->log( (lvl), 0, __PRETTY_FUNCTION__, msg ); }
#define __LOG_STATIC(   lvl, msg )  if( Logger::get_instance()->should_log( (lvl) ) )   { Logger::get_instance()->log( (lvl), 0, __PRETTY_FUNCTION__, msg ); }
#define __LOG( logger,  lvl, msg )  if( (logger)->should_log( (lvl) ) )                 { (logger)->log( (lvl), 0, 0, msg ); }

/* Object instance method logging macros */
#define DEBUGLOG(x)     __LOG_METHOD( Logger::Debug,   (x) );
#define INFOLOG(x)      __LOG_METHOD( Logger::Info,    (x) );
#define WARNINGLOG(x)   __LOG_METHOD( Logger::Warning, (x) );
#define ERRORLOG(x)     __LOG_METHOD( Logger::Error,   (x) );

/* Object class method logging macros */
#define _DEBUGLOG(x)    __LOG_CLASS( Logger::Debug,   (x) );
#define _INFOLOG(x)     __LOG_CLASS( Logger::Info,    (x) );
#define _WARNINGLOG(x)  __LOG_CLASS( Logger::Warning, (x) );
#define _ERRORLOG(x)    __LOG_CLASS( Logger::Error,   (x) );

/* logging macros using an Object *__object ( thread :  Object* __object = ( Object* )param; ) */
#define __DEBUGLOG(x)   __LOG_OBJ( Logger::Debug,      (x) );
#define __INFOLOG(x)    __LOG_OBJ( Logger::Info,       (x) );
#define __WARNINGLOG(x) __LOG_OBJ( Logger::Warning,    (x) );
#define __ERRORLOG(x)   __LOG_OBJ( Logger::Error,      (x) );

/* logging macros using  ( thread :  Object* __object = ( Object* )param; ) */
#define ___DEBUGLOG(x)  __LOG_STATIC( Logger::Debug,    (x) );
#define ___INFOLOG(x)   __LOG_STATIC( Logger::Info,     (x) );
#define ___WARNINGLOG(x) __LOG_STATIC(Logger::Warning,  (x) );
#define ___ERRORLOG(x)  __LOG_STATIC( Logger::Error,    (x) );

#endif // H2_OBJECT_H
