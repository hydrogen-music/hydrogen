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

#include "hydrogen/Object.h"

#include <iostream>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

unsigned int Object::__objects = 0;
bool Object::__count_objects = false;
std::map<QString, int> Object::__object_map;

/**
 * Constructor
 */
Object::Object( const QString& class_name )
		: __class_name( class_name )
{
	++__objects;
	__logger = Logger::get_instance();

	if ( __count_objects ) {
		int nInstances = __object_map[ __class_name ];
		++nInstances;
		__object_map[ __class_name ] = nInstances;
	}
}

/**
 * Copy constructor
 */
Object::Object( const Object& obj )
{

	__class_name = obj.get_class_name();

	++__objects;
//	__class_name = obj.getClassName;
	__logger = Logger::get_instance();

	if ( __count_objects ) {
		int nInstances = __object_map[ __class_name ];
		++nInstances;
		__object_map[ __class_name ] = nInstances;
	}
}

/**
 * Destructor
 */
Object::~Object()
{
	--__objects;

	if ( __count_objects ) {
		int nInstances = __object_map[ __class_name ];
		--nInstances;
		__object_map[ __class_name ] = nInstances;
	}
}


/**
 * Return the number of Objects not deleted
 */
int Object::get_objects_number()
{
	return __objects;
}

void Object::set_logging_level(const char* level)
{
    unsigned log_level = Logger::parse_log_level( level );
	Logger::set_log_level( log_level );
	__count_objects = (log_level&Logger::Debug)>0;
	Logger::get_instance()->__use_file = __count_objects;
}

void Object::print_object_map()
{
	if (!__count_objects) {
		std::cout << "[Object::print_object_map -- "
			"object map disabled.]" << std::endl;
		return;
	}

	std::cout << "[Object::print_object_map]" << std::endl;

    std::map<QString, int>::iterator iter = __object_map.begin();
	int nTotal = 0;
	do {
		int nInstances = ( *iter ).second;
		QString sObject = ( *iter ).first;
		if ( nInstances != 0 ) {
			std::cout << nInstances << "\t" << sObject.toLocal8Bit().constData() << std::endl;
		}
		nTotal += nInstances;
		iter++;
	} while ( iter != __object_map.end() );

	std::cout << "Total : " << nTotal << " objects." << std::endl;
}
