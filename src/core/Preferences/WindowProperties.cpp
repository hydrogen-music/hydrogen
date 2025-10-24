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

#include "WindowProperties.h"

#include <core/Helpers/Xml.h>

namespace H2Core {

WindowProperties::WindowProperties()
	: x( 0 )
	, y( 0 )
	, width( 0 )
	, height( 0 )
	, visible( true ) {
}

WindowProperties::WindowProperties( int _x, int _y, int _width, int _height,
									bool _visible, const QByteArray& geometry )
	: x( _x )
	, y( _y )
	, width( _width )
	, height( _height )
	, visible( _visible )
	, m_geometry( geometry ) {
}

WindowProperties::WindowProperties(const WindowProperties & other)
		: x( other.x )
		, y( other.y )
		, width( other.width )
		, height( other.height )
		, visible( other.visible )
		, m_geometry( other.m_geometry )
{}

WindowProperties::~WindowProperties() {
}

WindowProperties WindowProperties::loadFrom( const XMLNode& node,
											const WindowProperties& defaults,
											const bool bSilent ) {
	WindowProperties prop { defaults };

	prop.visible = node.read_bool( "visible", prop.visible, false, false, bSilent );
	prop.x = node.read_int( "x", prop.x, false, false, bSilent );
	prop.y = node.read_int( "y", prop.y, false, false, bSilent );
	prop.width = node.read_int( "width", prop.width, false, false, bSilent );
	prop.height = node.read_int( "height", prop.height, false, false, bSilent );
	prop.m_geometry = QByteArray::fromBase64(
		node.read_string( "geometry", prop.m_geometry.toBase64(), false, true,
						 bSilent ).toUtf8() );

	return std::move( prop );
}

void WindowProperties::saveTo( XMLNode& node ) const {
	node.write_bool( "visible", visible );
	node.write_int( "x", x );
	node.write_int( "y", y );
	node.write_int( "width", width );
	node.write_int( "height", height );
	node.write_string( "geometry", QString( m_geometry.toBase64() ) );
}

QString WindowProperties::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[WindowProperties]\n" ).arg( sPrefix )
			.append( QString( "%1%2x: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( x ) )
			.append( QString( "%1%2y: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( y ) )
			.append( QString( "%1%2width: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( width ) )
			.append( QString( "%1%2height: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( height ) )
			.append( QString( "%1%2visible: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( visible ) )
			.append( QString( "%1%2m_geometry: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( QString( m_geometry.toHex( ':' ) ) ) );
	}
	else {
		sOutput = QString( "[WindowProperties] " )
			.append( QString( "x: %1" ).arg( x ) )
			.append( QString( ", y: %1" ).arg( y ) )
			.append( QString( ", width: %1" ).arg( width ) )
			.append( QString( ", height: %1" ).arg( height ) )
			.append( QString( ", visible: %1" ).arg( visible ) )
			.append( QString( ", m_geometry: %1" )
					 .arg( QString( m_geometry.toHex( ':' ) ) ) );
	}

	return sOutput;
}
};
