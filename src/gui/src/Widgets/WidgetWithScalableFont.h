/*
 * Hydrogen
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef WIDGET_WITH_SCALABLE_FONT_H
#define WIDGET_WITH_SCALABLE_FONT_H

#include <core/Object.h>
#include <core/Preferences/Preferences.h>

/** Widget is affected by the "Font size" settings in the
 *	PreferencesDialog.
 *
 * To improve accessibility, three different font sizes,
 * H2Core::FontTheme::FontSize::Small,
 * H2Core::FontTheme::FontSize::Medium, and
 * H2Core::FontTheme::FontSize::Large, are available.
 */
/** \ingroup docGUI docWidgets*/
template < int nSmall, int nNormal, int nLarge >
class WidgetWithScalableFont {
protected:
  constexpr int getPointSize( H2Core::FontTheme::FontSize fontSize ) const {
    switch ( fontSize ) {
    case H2Core::FontTheme::FontSize::Small:
      return nSmall;
    case H2Core::FontTheme::FontSize::Medium:
      return nNormal;
    case H2Core::FontTheme::FontSize::Large:
      return nLarge;
    default:
      ___ERRORLOG( QString( "Unknown font size: %1" ).arg( static_cast<int>( fontSize ) ) );
      return 10;
    }
  }
};

 #endif
