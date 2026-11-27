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

// Pure-QtCore port of Qt's QTextHtmlParser/QTextHtmlImporter plus
// QTextDocument::toPlainText. The plugin UI links QtCore only, so QtGui's
// rich-text path is unavailable; format-only branches are dropped. Behavior is
// pinned case-by-case by the reference battery in PluginUiModelTest.

#ifndef H2C_PLUGIN_UI_HTML_H
#define H2C_PLUGIN_UI_HTML_H

#include <QString>

namespace H2Core {

	/** Rich text (HTML, as drumkit.xml's <info> element stores it) to
	 * plain text: tags dropped, entities decoded, block boundaries as
	 * newlines, whitespace collapsed per white-space mode. */
	QString pluginUiHtmlToPlainText( const QString& sHtml );

} // namespace H2Core

#endif // H2C_PLUGIN_UI_HTML_H
