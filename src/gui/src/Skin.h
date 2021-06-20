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
		return QColor( 116, 124, 149 );
	}
	/** A text color that contrasts with Highlight. */
	static QColor getHighlightedTextColor() {
		return QColor( 255, 255, 255 );
	}

	static QString getGlobalStyleSheet() {
		return QString( "\
QToolTip { \
    padding: 1px; \
    border: 1px solid rgb(199, 202, 204); \
    background-color: rgb(227, 243, 252); \
    color: rgb(64, 64, 66); \
} \
QPushButton { \
    color: #0a0a0a; \
    border: 1px solid #0a0a0a; \
    border-radius: 2px; \
    padding: 5px; \
    background-color: qlineargradient(x1: 0.1, y1: 0.1, x2: 1, y2: 1, \
                                      stop: 0 #dae0f2, stop: 1 #9298aa); \
} \
QPushButton:hover { \
background-color: qlineargradient(x1: 0.1, y1: 0.1, x2: 1, y2: 1, \
                                      stop: 0 #e1e7fa, stop: 1 #9ba1b4); \
} \
QPushButton:checked { \
background-color: qlineargradient(x1: 0.1, y1: 0.1, x2: 1, y2: 1, \
                                      stop: 0 #a2cdff, stop: 1 #69a2e5); \
} \
QComboBox { \
    color: #0a0a0a; \
    background-color: #a4aabe; \
} \
QComboBox QAbstractItemView { \
    background-color: #babfcf; \
} \
QLineEdit { \
    color: #ffffff; \
    background-color: #3a3e48; \
} \
QDoubleSpinBox, QSpinBox { \
    color: #ffffff; \
    background-color: #374f6c; \
    selection-color: #0a0a0a; \
    selection-background-color: #babfcf; \
} \
QDoubleSpinBox:selected, QSpinBox:selected { \
    color: blue; \
    background-color: #babfcf; \
}"
						);
	}

};


#endif
