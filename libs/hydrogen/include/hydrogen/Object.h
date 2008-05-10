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

#ifdef check
#undef check
#endif


#include <QtGui>
#include <map>
#include <vector>
#include <sstream>
#include <pthread.h>


// LOG MACROS

#define _INFOLOG(x) Logger::get_instance()->info_log( __PRETTY_FUNCTION__, "", x );
#define _WARNINGLOG(x) Logger::get_instance()->warning_log( __PRETTY_FUNCTION__, "", x );
#define _ERRORLOG(x) Logger::get_instance()->error_log( __PRETTY_FUNCTION__, "", x );

#define INFOLOG(x) Logger::get_instance()->info_log( __FUNCTION__, get_class_name(), x );
#define WARNINGLOG(x) Logger::get_instance()->warning_log( __FUNCTION__, get_class_name(), x );
#define ERRORLOG(x) Logger::get_instance()->error_log( __FUNCTION__, get_class_name(), x );



class Object;

/**
 * Class for writing logs to the console
 */
class Logger
{
public:
	bool __use_file;
	bool __running;
	std::vector<QString> __msg_queue;
	pthread_mutex_t __logger_mutex;

	static Logger* get_instance();

	/** Destructor */
	~Logger();

	void info_log( const char* funcname, const QString& class_name, const QString& msg );
	void warning_log( const char* funcname, const QString& class_name, const QString& msg );
	void error_log( const char* funcname, const QString& class_name, const QString& msg );

private:
	static Logger *__instance;

	/** Constructor */
	Logger();


};


/**
 * Base class.
 */
class Object
{
public:
	static bool __use_log;

	/** Constructor */
	Object( const QString& className );
	Object( const Object& obj );

	/** Destructor */
	virtual ~Object();

	const QString& get_class_name() const {
		return __class_name;
	}

	static int get_objects_number();
	static void print_object_map();
	static void use_verbose_log( bool verbose );
	static bool is_using_verbose_log();

private:
	static unsigned __objects;
	static std::map<QString, int> __object_map;
	Logger *__logger;
	QString __class_name;
};


// some useful functions..

template <class T>
inline QString to_string( const T& t )
{
	std::ostringstream osstream;
	osstream << t;

	return QString( osstream.str().c_str() );
}

inline int string_to_int( const QString& str )
{
	std::istringstream isstream( str.toStdString() );
	int t;
	isstream >> t;
	return t;
}
inline float string_to_float( const QString& str )
{
	std::istringstream isstream( str.toStdString() );
	float t;
	isstream >> t;
	return t;
}

#endif
