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
	/** If a widget is marked inactive the value of its background
		color are reduced by this factor.*/
	static QColor makeWidgetColorInactive( QColor color );
		/** If a widget is marked inactive the value of its text color
		are reduced by this factor.*/
	static QColor makeTextColorInactive( QColor color );

	static constexpr int nPlayheadWidth = 11;
	static constexpr int nPlayheadHeight = 8;
	static int getPlayheadShaftOffset() {
		return std::floor( Skin::nPlayheadWidth / 2 ); }
	static void setPlayheadPen( QPainter* p, bool bHovered = false );
	static void drawPlayhead( QPainter* p, int x, int y, bool bHovered = false );

	enum class Stacked {
		None,
		Off,
		OffNext,
		On,
		OnNext
	};

	static void drawStackedIndicator( QPainter* p, int x, int y, Skin::Stacked stacked );
};


#endif
