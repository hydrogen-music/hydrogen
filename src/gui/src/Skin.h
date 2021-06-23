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
#ifndef H2_SKIN_H
#define H2_SKIN_H

#include <QString>
#include <core/Helpers/Filesystem.h>

///
/// Skin support
///
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
	/**A general background color.*/
	static QColor getWindowColor() {
		return QColor( 58, 62, 72 );
	}
	/**A general foreground color.*/
	static QColor getWindowTextColor() {
		return QColor( 255, 255, 255 );
	}
	/**Used as the background color for text entry widgets; usually white or another light color.*/
	static QColor getBaseColor() {
		return QColor( 88, 94, 112 );
	}
	/**Used as the alternate background color in views with alternating row colors*/
	static QColor getAlternateBaseColor() {
		return QColor( 138, 144, 162 );
	}
	/**The foreground color used with Base. This is usually the same as the Foreground, in which case it must provide good contrast with Background and Base.*/
	static QColor getTextColor() {
		return QColor( 255, 255, 255 );
	}
	/**The general button background color. This background can be different from Background as some styles require a different background color for buttons.*/
	static QColor getButtonColor() {
		return QColor( 88, 94, 112 );
	}
	/**A foreground color used with the Button color.*/
	static QColor getButtonTextColor() {
		return QColor( 255, 255, 255 );
	}
	/**Lighter than Button color.*/
	static QColor getLightColor() {
		return QColor( 138, 144, 162 );
	}
	/**Between Button and Light.*/
	static QColor getMidLightColor() {
		return QColor( 128, 134, 152 );
	}
	/**Darker than Button.*/
	static QColor getDarkColor() {
		return QColor( 58, 62, 72 );
	}
	/**Between Button and Dark.*/
	static QColor getMidColor() {
		return QColor( 81, 86, 99 );
	}
	/**A very dark color. By default, the shadow color is Qt::black.*/
	static QColor getShadowTextColor() {
		return QColor( 255, 255, 255 );
	}
	/** A color to indicate a selected item or the current item.*/
	static QColor getHighlightColor() {
		return QColor( 206, 150, 30 );
	}
	static QColor getCustomButtonColor() {
		return QColor( 164, 170, 190 );
	}
	static QColor getCustomButtonTextColor() {
		return QColor( 10, 10, 10 );
	}
	/** A text color that contrasts with Highlight. */
	static QColor getHighlightedTextColor() {
		return QColor( 255, 255, 255 );
	}
	static QColor getBlueAccentColor() {
		return QColor( 67, 96, 131 );
	}

	static QString getGlobalStyleSheet();
};


#endif
