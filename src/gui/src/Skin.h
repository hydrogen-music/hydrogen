/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#ifndef H2_SKIN_H
#define H2_SKIN_H

#include <QString>
#include <QtGui>
#include <core/Helpers/Filesystem.h>

///
/// Skin support
///
/** \ingroup docGUI*/
class Skin
{
public:
	static QString getImagePath()
	{
		return H2Core::Filesystem::img_dir().append( "/gray" );
	}
	static QString getSvgImagePath()
	{
		return H2Core::Filesystem::img_dir().append( "/scalable" );
	}
	/** Top-level stylesheet for Hydrogen.

		This one affects all widgets within Hydrogen (including
		popups, file browser etc) and will get the most recent color
		values from the #H2Core::Preferences.
		
		\return String version of the stylesheet.*/
	static QString getGlobalStyleSheet();
	/** Function used to update the global palette of the
	QApplication.
	
	It will get the most recent color values from the #H2Core::Preferences.*/
	static void setPalette( QApplication *pQApp );

	/** Get the style sheet used for warning icons. 
	 *
	 * In addition, the icon of the warning button has to be set to
	 * Skin::getSvgImagePath() + "/icons/warning.svg".
	 *
	 * \param nSize Size in pixel (value will be used as both width
	 * and height).
	 *
	 * \return Argument used of the setStyleSheet() method of the
	 * warning button.
	 */
	static QString getWarningButtonStyleSheet( int nSize );

	/**
	 * Draws the background of a row in both the pattern list of the
	 * SongEditor and the instrument list in the PatternEditor using
	 * @a p.
	 *
	 * \param p Painter used in the calling QPaintEvent routine.
	 * \param rect Boundary that encloses element (one row).
	 * \param background Color used.
	 * \param bHovered Whether the element is currently hovered by mouse.
	 */
	static void drawListBackground( QPainter* p, QRect rect, QColor background,
									bool bHovered );

};


#endif
