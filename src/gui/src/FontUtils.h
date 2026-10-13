/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef H2C_FONT_UTILS_H
#define H2C_FONT_UTILS_H

#include <memory>

#include <core/Preferences/Theme.h>

/** \ingroup docGUI*/
namespace FontUtils
{
/**
 * Sanitizes the font families of pFont in place against the local font
 * database, falling back to the system default family.
 *
 * The single QFontDatabase access point of the font sanitization: the
 * core must stay free of QFont/QFontDatabase since it also runs
 * headless in plugin hosts, without the QGuiApplication these APIs
 * require.
 */
void sanitizeFontTheme( const std::shared_ptr<H2Core::FontTheme>& pFont );
}

#endif
