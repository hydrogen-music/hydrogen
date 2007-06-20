/* Hydrogen
 * Copyright(c) 2002-2007 Jonathan Dempsey, Alessandro Cominu
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
#include <hydrogen/DataPath.h>

#include <QFile>
#include <QApplication>

//#ifdef Q_OS_MACX
//#  include <Carbon.h>
//#endif

#include <iostream>
using namespace std;

#include "config.h"

namespace H2Core {

std::string DataPath::m_sDataPath = "";

std::string DataPath::getDataPath()
{
#ifdef Q_OS_MACX

	QString qStringPath = qApp->applicationDirPath() + QString ( "/../Resources/data" ) ;
	return std::string( qStringPath.toStdString() );
/*
	if ( m_sDataPath == "" ) {
		CFURLRef pluginRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
		CFStringRef macPath = CFURLCopyFileSystemPath(pluginRef, kCFURLPOSIXPathStyle);
		const char *path = CFStringGetCStringPtr(macPath, CFStringGetSystemEncoding());
		CFRelease(pluginRef);
		CFRelease(macPath);

		m_sDataPath = std::string( path ) + std::string( "/Contents/Resources/data" );
	}

	return m_sDataPath;
*/
#elif WIN32

	QString qStringPath = qApp->applicationDirPath() + QString ( "/data" ) ;
	return std::string( qStringPath.toStdString() );

#else
	if ( m_sDataPath == "" ) {
		QString qStringPath = qApp->applicationDirPath() + QString ( "/data" ) ;
		m_sDataPath = std::string( qStringPath.toStdString() );

		QFile file( m_sDataPath.c_str() );
		if ( !file.exists() ) {
			// try using the system wide data dir
			m_sDataPath = DATA_PATH;
		}
	}
	return m_sDataPath;
#endif
}

};

