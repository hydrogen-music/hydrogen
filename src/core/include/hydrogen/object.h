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
* @class Object
*
* @brief Base class of most classes in hydrogen.
*
* Most components of hydrogen do inherit from the Object class. Each
* object has a qualified name (#m_sClassName), which is set prior to
* the creation of the first instance of the corresponding class, and
* gets registered in a memory map at creation (#__objects_map).  This
* map helps to debug memory leaks and can be printed at any time using
* write_objects_map_to(). In addition, the inheritance from Object
* allows for more informative log messages in the member functions
* using INFOLOG() or _INFOLOG()
*
* Before any Object or a derived class can be created, the Logger has
* to be initialized and the static bootstrap() member be called first.
*
*/
class Object {
	public:
		/** Destructor
		 *
		 * If Hydrogen was compiled with the `WANT_DEBUG` flag causing
		 * #H2CORE_HAVE_DEBUG to be set, del_object() with the current
		 * object as argument will be called whenever #__count is
		 * `true`.*/
		~Object();
		/** Copy constructor
		 *
		 * If Hydrogen was compiled with the `WANT_DEBUG` flag causing
		 * #H2CORE_HAVE_DEBUG to be set, add_object() with the current
		 * object as first and `true` as second argument will be
		 * called whenever #__count is `true`.*/
		Object( const Object& obj );
		/** Constructor
		 *
		 * If Hydrogen was compiled with the `WANT_DEBUG` flag causing
		 * #H2CORE_HAVE_DEBUG to be set, add_object() with the current
		 * object as first and `false` as second argument will be
		 * called whenever #__count is `true`.*/
		Object( const char* m_sClassName );
		/** \return #m_sClassName*/
		const char* className() { return m_sClassName; }
		/**
		 * Whether the number and the specific types of the created
		 * objects should be tracked.
		 *
		 * If Hydrogen was compiled with the `WANT_DEBUG` flag causing
		 * #H2CORE_HAVE_DEBUG to be set, @a flag enables or disables
		 * the counting of the instances created for a specific
		 * class. If, however, debugging was not enabled, an error
		 * messages will be printed using the Logger.
		 *
		 * \param flag Enables (`true`) or disable the counting.
		 */
		static void set_count( bool flag );
		
		/**\return #__count*/
		static bool count_active()              { return __count; }
		/**\return #__objects_count*/
		static unsigned objects_count()         { return __objects_count; }

		/**
		 * Prints #__objects_map to the ostream @a out.
		 *
		 * This action will only be performed if Hydrogen was
		 * compiled with the `WANT_DEBUG` flag causing
		 * #H2CORE_HAVE_DEBUG to be set and #__count is set to
		 * `true`. In addition #__mutex will be locked during the
		 * operation.
		 *
		 * \param out The ostream to write to
		 */
		static void write_objects_map_to( std::ostream& out );
		/** 
		 * Prints #__objects_map to stderr.
		 *
		 * Wrapper around write_objects_map_to() passing `std::cerr`
		 * as argument.*/
		static void write_objects_map_to_cerr() { Object::write_objects_map_to( std::cerr ); }

		/**
		 * Must be called before any Object instantiating!
		 *
		 * It assigns @a logger to #__logger, @a count to #__count,
		 * and initializes #__mutex by calling `pthread_mutex_init()`.
		 *
		 * \param logger The logger instance used to send messages to.
		 * \param count Should we count objects instances or not.
		 *
		 * \return `0` if everything worked fine and `1` if either
		 * #__logger or @a logger is a `nullptr`.
		 */
		static int bootstrap( Logger* logger, bool count=false );
		/**\return #__logger*/
		static Logger* logger()                 { return __logger; }

	private:
		/**
		 * Decrements #__objects_count and increments the
		 * obj_cpt_t::destructed member of #__objects_map.
		 *
		 * After locking #__mutex a key in #__objects_map
		 * corresponding to the class name of @a obj will be
		 * searched. If found, #__objects_count will be decremented
		 * and the value of the obj_cpt_t::destructed member of #__objects_map
		 * matching the class name of @a obj will be incremented.
		 *
		 * These actions will only be performed if Hydrogen was
		 * compiled with the `WANT_DEBUG` flag causing
		 * #H2CORE_HAVE_DEBUG to be set.
		 *
		 * \param obj The Object to be taken into account.
		 */
		static void del_object( const Object* obj );
		/**
		 * Increments #__objects_count and the obj_cpt_t::constructed
		 * member of #__objects_map.
		 *
		 * After locking #__mutex both #__objects_count and the value
		 * of the obj_cpt_t::constructed member of the key in
		 * #__objects_map matching the class name of @a obj will be
		 * incremented. If such a key does not exist yet, it will be
		 * created.
		 *
		 * These actions will only be performed if Hydrogen was
		 * compiled with the `WANT_DEBUG` flag causing
		 * #H2CORE_HAVE_DEBUG to be set.
		 *
		 * \param obj The Object to be taken into account.
		 * \param copy Specifies whether the function is called from a
		 * copy constructor. It only affects the debugging messages
		 * printed by the Logger.
		 */
		static void add_object( const Object* obj, bool copy );

		/** Struct used in #__objects_map to keep track of the number
		 * of constructed and destroyed instances of each individual
		 * class.*/
		typedef struct {
			/** Number of constructed instances of a particular class.*/
			unsigned constructed;
			/** Number of destroyed instances of a particular class.*/
			unsigned destructed;
		} obj_cpt_t;
	
		/** Map class used by #__objects_map.*/
		typedef std::map<const char*, obj_cpt_t> object_map_t;

		/** Contains the name of the class.
		 *
		 * This variable allows from more informative log messages
		 * with the name of the class the message is generated in
		 * being displayed as well. Queried using className().*/
		const char* m_sClassName;
		/**
		 * Specifies whether the creation and destruction of each
		 * class derived from Object should be counted using
		 * add_object() and del_object().
		 *
		 * It is globally initialized to `false` and locally in
		 * bootstrap(). In addition it is set by set_count() and
		 * queried using count_active().
		 */
		static bool __count;
		/**
		 * Number of objects, which are either of class Object itself
		 * or derived from it.
		 *
		 * It is globally initialized to `0`, incremented in
		 * add_object(), decremented in del_object(), and queried
		 * using objects_count().
		 */
		static unsigned __objects_count;
		/**
		 * A map keeping track of the number of constructed and
		 * destroyed instances per individual class.
		 *
		 * The keys of the map correspond to the individual names of
		 * the classes encountered and the value to a struct counting
		 * both the number of created instances obj_cpt_t::constructed
		 * and destroyed instances obj_cpt_t::destructed. When
		 * hydrogen is started in debugging mode (with command line
		 * argument `-VDebug`) the content of the map will be
		 * displayed using write_objects_map_to() after telling the
		 * application to shut down.
		 *
		 * It is globally initialized to an empty obj_cpt_t and
		 * modified by add_object() and del_object().
		 */
		static object_map_t __objects_map;
		/**
		 * Mutex ensuring writing to #__objects_map and
		 * #__objects_count will be thread safe.
		 *
		 * It is globally initialized to a `pthread_mutex_t` object
		 * and initialized in bootstrap().
		 */
		static pthread_mutex_t __mutex;

	protected:
		/**
		 * Pointer to the Logger singleton.
		 *
		 * Using this object all classes derived from Object are able
		 * to print more informative messages using the various log
		 * macros.
		 *
		 * It is initialized globally to a `nullptr`, set in
		 * bootstrap(), and queried using logger().
		 */
		static Logger* __logger;
};


//////////////////////////////////////////////////////////////////////
// LOG MACROS

/**
 * Print a log message using the private H2Core::Object::__logger
 * member of the particular class.
 *
 * To create log messages, please use the more user-friendly wrappers,
 * like INFOLOG(), instead.
 *
 * If H2Core::Logger::should_log() returns `true`, this macro will
 * call H2Core::Logger::log() of the private H2Core::Object::__logger
 * member of the class the macro is placed in. It will provide the log
 * level @a lvl, the return value of H2Core::Object::className(), the
 * implicitly declared identifier `__FUNCTION__` (which contains the
 * name of the calling function), and @a msg as input arguments. Since
 * the H2Core::Object::m_sClassName is a static variable and set
 * before the actual class is constructed, also static member
 * functions without its corresponding class being created can be
 * logged.
 *
 * Please keep in mind that only objects inherited from H2Core::Object
 * can use this logging macro!
 *
 * \param lvl Log level. Has to be one of H2Core::Logger::Debug,
 * H2Core::Logger::Info, H2Core::Logger::Warning, and
 * H2Core::Logger::Error.
 * \param msg User message.
 */
#define __LOG_METHOD(   lvl, msg )  if( __logger->should_log( (lvl) ) )                 { __logger->log( (lvl), className(), __FUNCTION__, msg ); }

/**
 * Print a log message using the public H2Core::Object::logger()
 * member of the particular class.
 *
 * To create log messages, please use the more user-friendly wrappers,
 * like _INFOLOG(), instead.
 *
 * If H2Core::Logger::should_log() returns `true`, this macro will
 * call H2Core::Logger::log() of the public H2Core::Object::logger()
 * member of the class the macro is placed in. It will provide the log
 * level @a lvl, the return value of H2Core::Object::className(), the
 * implicitly declared identifier `__FUNCTION__` (which contains the
 * name of the calling function), and @a msg as input arguments. Since
 * the H2Core::Object::m_sClassName is a static variable and set
 * before the actual class is constructed, also static member
 * functions without its corresponding class being created can be
 * logged.
 *
 * Please keep in mind that only objects inherited from H2Core::Object
 * can use this logging macro!
 *
 * \param lvl Log level. Has to be one of H2Core::Logger::Debug,
 * H2Core::Logger::Info, H2Core::Logger::Warning, and
 * H2Core::Logger::Error.
 * \param msg User message.
 */
#define __LOG_CLASS(    lvl, msg )  if( logger()->should_log( (lvl) ) )                 { logger()->log( (lvl), className(), __FUNCTION__, msg ); }

/**
 * Print a log message using the public H2Core::Object::logger()
 * member of the object stored in `__object`.
 *
 * This particular version of the logging it used in the callback
 * functions of the audio drivers. To those functions an object
 * derived from the H2Core::Object class will be provided as an
 * argument, which will internally assigned to a variable named
 * `__object`. This enables also those functions living outside of the
 * classes of hydrogen to use the same logging style.
 *
 * To create log messages, please use the more user-friendly wrappers,
 * like __INFOLOG(), instead.
 *
 * If H2Core::Logger::should_log() returns `true`, this macro will
 * call H2Core::Logger::log() of the public H2Core::Object::logger()
 * member of the `__object` variable. It will provide the log level @a
 * lvl, `0`, the implicitly declared identifier `__PRETTY_FUNCTION__`
 * (which contains the name of the calling function), and @a msg as
 * input arguments. Since those functions are called outside of any
 * hydrogen classes, no class name can be provided.
 *
 * \param lvl Log level. Has to be one of H2Core::Logger::Debug,
 * H2Core::Logger::Info, H2Core::Logger::Warning, and
 * H2Core::Logger::Error.
 * \param msg User message.
 */
#define __LOG_OBJ(      lvl, msg )  if( __object->logger()->should_log( (lvl) ) )       { __object->logger()->log( (lvl), 0, __PRETTY_FUNCTION__, msg ); }

/**
 * Print a log message using the global instance of the Logger
 * singleton.
 *
 * This particular version of the logging is a fallback version that
 * should always work. It is mainly used in global functions, like
 * the `audioEngine_*` ones in `hydrogen.cpp`.
 *
 * To create log messages, please use the more user-friendly wrappers,
 * like ___INFOLOG(), instead.
 *
 * If H2Core::Logger::should_log() returns `true`, this macro will
 * call H2Core::Logger::log() of the global instance of the
 * H2Core::Logger singleton retrieved using
 * H2Core::Logger::get_instance(). It will provide the log level @a
 * lvl, `0`, the implicitly declared identifier `__PRETTY_FUNCTION__`
 * (which contains the name of the calling function), and @a msg as
 * input arguments. Since those functions are called outside of any
 * hydrogen classes, no class name can be provided.
 *
 * \param lvl Log level. Has to be one of H2Core::Logger::Debug,
 * H2Core::Logger::Info, H2Core::Logger::Warning, and
 * H2Core::Logger::Error.
 * \param msg User message.
 */
#define __LOG_STATIC(   lvl, msg )  if( H2Core::Logger::get_instance()->should_log( (lvl) ) )   { H2Core::Logger::get_instance()->log( (lvl), 0, __PRETTY_FUNCTION__, msg ); }

// Object instance method logging macros

/** Wrapper around __LOG_METHOD() using H2Core::Logger::Debug as log
 *	level. 
 * \param x User message.*/ 
#define DEBUGLOG(x)     __LOG_METHOD( H2Core::Logger::Debug,   (x) );
/** Wrapper around __LOG_METHOD() using H2Core::Logger::Info as log
 *	level. 
 * \param x User message.*/
#define INFOLOG(x)      __LOG_METHOD( H2Core::Logger::Info,    (x) );
/** Wrapper around __LOG_METHOD() using H2Core::Logger::Warning as log
 *	level. 
 * \param x User message.*/
#define WARNINGLOG(x)   __LOG_METHOD( H2Core::Logger::Warning, (x) );
/** Wrapper around __LOG_METHOD() using H2Core::Logger::Error as log
 *	level. 
 * \param x User message.*/
#define ERRORLOG(x)     __LOG_METHOD( H2Core::Logger::Error,   (x) );

/** Wrapper around __LOG_CLASS() using H2Core::Logger::Debug as log
 *	level. 
 * \param x User message.*/
#define _DEBUGLOG(x)    __LOG_CLASS( H2Core::Logger::Debug,   (x) );
/** Wrapper around __LOG_CLASS() using H2Core::Logger::Info as log
 *	level. 
 * \param x User message.*/
#define _INFOLOG(x)     __LOG_CLASS( H2Core::Logger::Info,    (x) );
/** Wrapper around __LOG_CLASS() using H2Core::Logger::Warning as log
 *	level. 
 * \param x User message.*/
#define _WARNINGLOG(x)  __LOG_CLASS( H2Core::Logger::Warning, (x) );
/** Wrapper around __LOG_CLASS() using H2Core::Logger::Error as log
 *	level. 
 * \param x User message.*/
#define _ERRORLOG(x)    __LOG_CLASS( H2Core::Logger::Error,   (x) );

/** Wrapper around __LOG_OBJ() using H2Core::Logger::Debug as log
 *	level. 
 * \param x User message.*/
#define __DEBUGLOG(x)   __LOG_OBJ( H2Core::Logger::Debug,      (x) );
/** Wrapper around __LOG_OBJ() using H2Core::Logger::Info as log
 *	level. 
 * \param x User message.*/
#define __INFOLOG(x)    __LOG_OBJ( H2Core::Logger::Info,       (x) );
/** Wrapper around __LOG_OBJ() using H2Core::Logger::Warning as log
 *	level. 
 * \param x User message.*/
#define __WARNINGLOG(x) __LOG_OBJ( H2Core::Logger::Warning,    (x) );
/** Wrapper around __LOG_OBJ() using H2Core::Logger::Error as log
 *	level. 
 * \param x User message.*/
#define __ERRORLOG(x)   __LOG_OBJ( H2Core::Logger::Error,      (x) );

/** Wrapper around __LOG_STATIC() using H2Core::Logger::Debug as log
 *	level. 
 * \param x User message.*/
#define ___DEBUGLOG(x)  __LOG_STATIC( H2Core::Logger::Debug,    (x) );
/** Wrapper around __LOG_STATIC() using H2Core::Logger::Info as log
 *	level. 
 * \param x User message.*/
#define ___INFOLOG(x)   __LOG_STATIC( H2Core::Logger::Info,     (x) );
/** Wrapper around __LOG_STATIC() using H2Core::Logger::Warning as log
 *	level. 
 * \param x User message.*/
#define ___WARNINGLOG(x) __LOG_STATIC(H2Core::Logger::Warning,  (x) );
/** Wrapper around __LOG_STATIC() using H2Core::Logger::Error as log
 *	level. 
 * \param x User message.*/
#define ___ERRORLOG(x)  __LOG_STATIC( H2Core::Logger::Error,    (x) );

};

#endif // H2C_OBJECT_H

/* vim: set softtabstop=4 noexpandtab: */
