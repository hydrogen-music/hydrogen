/* Hydrogen
 * Copyright(c) 2002-2005 Jonathan Dempsey, Alessandro Cominu
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
#include "DataPath.h"

#include <qfile.h>
#include <iostream>
using namespace std;

std::string DataPath::m_sDataPath = "";


std::string DataPath::getDataPath()
{
#ifdef Q_OS_MACX

	QString qStringPath = qApp->applicationDirPath() + QString ( "/../Resources/data" ) ;
	return std::string( qStringPath.ascii() );

#endif
#ifdef WIN32
	
	QString qStringPath = qApp->applicationDirPath() + QString ( "/data" ) ;
	return std::string( qStringPath.ascii() );

#else
	if ( m_sDataPath == "" ) {
		QString qStringPath = qApp->applicationDirPath() + QString ( "/data" ) ;
		m_sDataPath = std::string( qStringPath.ascii() );
	
		QFile file( m_sDataPath.c_str() );
		if ( !file.exists() ) {
			// try using the system wide data dir
			m_sDataPath = DATA_PATH;
		}
	}
	return m_sDataPath;
#endif
}

