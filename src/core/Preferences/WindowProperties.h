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

#ifndef WINDOW_PROPERTIES_H
#define WINDOW_PROPERTIES_H

#include <core/Object.h>

#include <QByteArray>

namespace H2Core
{

class XMLNode;

/** \ingroup H2CORE docCore docConfiguration*/
class WindowProperties : public H2Core::Object<WindowProperties>
{
	H2_OBJECT(WindowProperties)
public:
	int x;
	int y;
	int width;
	int height;
	bool visible;
	QByteArray m_geometry;

	WindowProperties();
	WindowProperties( int _x, int _y, int _width, int _height, bool _visible,
					  const QByteArray& geometry = QByteArray() );
	WindowProperties( const WindowProperties &other );
	~WindowProperties();

	void set(int _x, int _y, int _width, int _height, bool _visible, const QByteArray& geometry = QByteArray() ) {
		x = _x; y = _y;
		width = _width; height = _height;
		visible = _visible;
		m_geometry = geometry;
	}

	static WindowProperties loadFrom( const XMLNode& node,
									 const WindowProperties& defaults,
									 bool bSilent = false );
	void saveTo( XMLNode& node ) const;

	QString toQString( const QString& sPrefix = "",
					   bool bShort = true ) const override;
};

};

#endif

