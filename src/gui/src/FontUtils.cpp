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

#include "FontUtils.h"

#include <QFont>
#include <QFontDatabase>

namespace
{
	// families() is non-static in Qt 5 and static in Qt 6; the instance
	// form would hard-error under QT_DISABLE_DEPRECATED_BEFORE >= 6.
	QStringList availableFontFamilies()
	{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
		return QFontDatabase::families();
#else
		return QFontDatabase().families();
#endif
	}
}

namespace FontUtils
{
void sanitizeFontTheme( const std::shared_ptr<H2Core::FontTheme>& pFont )
{
	pFont->sanitizeFamilies( availableFontFamilies(), QFont().family() );
}
}
