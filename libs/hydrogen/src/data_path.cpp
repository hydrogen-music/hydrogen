/* Hydrogen
 * Copyright(c) 2002-2008 Jonathan Dempsey, Alessandro Cominu
 *
 * http://www.hydrogen-music.org
 *
 * DataPath.h
 * Header to define the path to the data files for Hydrogen in such a
 * way that self-contained Mac OS X application bundles can be built.
 * Copyright (c) 2005 Jonathan Dempsey
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <hydrogen/data_path.h>

#include <QFile>
#include <QApplication>

//#ifdef Q_OS_MACX
//#  include <Carbon.h>
//#endif

#include <iostream>
using namespace std;

#include "config.h"

namespace H2Core
{

QString DataPath::__data_path;

QString DataPath::get_data_path()
{
#ifdef Q_OS_MACX
    //Bundle: Prepare hydrogen to use path names which are used in app bundles: http://en.wikipedia.org/wiki/Application_Bundle
    #ifdef BUNDLE_SUPPORT
	__data_path = qApp->applicationDirPath() + QString ( "/../Resources/data" ) ;
    #elif NO_GUI_SUPPORT
	__data_path = QString ( "./data" ) ;
    #else
	__data_path = qApp->applicationDirPath() + QString ( "/data" ) ;
    #endif

	return __data_path;
#elif WIN32

	__data_path = qApp->applicationDirPath() + QString ( "/data" ) ;
	return __data_path;

#else
	if ( __data_path.isEmpty() ) {
		#ifdef NO_GUI_SUPPORT
		    __data_path = QString ( "./data" ) ;
		#else
		    __data_path = qApp->applicationDirPath() + QString ( "/data" ) ;
		#endif

		/*
		 * We're checking against __data_path + "/drumkits" because it is possible that there is a data directory
		 * which does not belong to hydrogen. See Bug #140 for details.
		 */

		QFile file( __data_path + "/drumkits" );
		if ( !file.exists() ) {
			// try using the system wide data dir
			__data_path = DATA_PATH;
		}
	}
	return __data_path;
#endif
}

};

