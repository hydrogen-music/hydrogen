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

///
/// Skin support
///
/** \ingroup docGUI*/
class Skin
{
public:
		/** Factor by which an average button (in a panel) is more wide than
		 * high. */
		static constexpr float fButtonWidthHeightRatio = 1.2;

		/** Factor by which the window background (empty space not covered by
		 * any other widget) of either pattern or song editor is made lighter in
		 * case the particular editor is chosen. */
		static constexpr int nEditorActiveScaling = 120;

		/** Different parts of the pattern editor share a common cursor. We will
		 * add a slight transparency to those cursors _not_ current focussed by
		 * the user. */
		static constexpr int nInactiveCursorAlpha = 170;

		/** Factor by which the background of a list element (sidebar) will be
		 * darker than the corresponding line. */
		static constexpr int nListBackgroundColorScaling = 120;
		/** Factor by which the background color of a list element will be
		 * darkened in order to produce the border color. */
		static constexpr int nListBackgroundDarkBorderScaling = 220;
		/** Factor by which the background color of a list element will be
		 * lightened in order to produce the border color. */
		static constexpr int nListBackgroundLightBorderScaling = 150;

		static constexpr int nPlayheadHeight = 8;
		static constexpr int nPlayheadWidth = 11;

		/** in pixel */
		static constexpr int nScrollBarWidth = 12;

		static constexpr int nToolBarCheckedScaling = 125;
		static constexpr int nToolBarHoveredScaling = 105;
		static constexpr int nToolBarPressedScaling = 110;

		enum class Stacked {
			None,
			Off,
			OffNext,
			On,
			OnNext
		};

		static void drawPlayhead( QPainter* p, int x, int y,
								  bool bHovered = false );
		static void drawStackedIndicator( QPainter* p, int x, int y,
										  const Skin::Stacked& stacked );

		/** Top-level stylesheet for Hydrogen.
		 *
		 * This one affects all widgets within Hydrogen (including popups, file
		 * browser etc) and will get the most recent color values from the
		 * #H2Core::Preferences.
		 *
		 * \return String version of the stylesheet.*/
		static QString getGlobalStyleSheet();

		static QString getImagePath();
		static int getPlayheadShaftOffset() {
			return std::floor( Skin::nPlayheadWidth / 2 ); }
		static QString getSvgImagePath();

		static QColor makeBackgroundColorInactive( const QColor& color );

		/** If a widget is marked inactive the value of its text color are
		 * reduced by this factor.*/
		static QColor makeTextColorInactive( const QColor& color );
		/** If a widget is marked inactive the value of its background color are
		 * reduced by this factor.*/
		static QColor makeWidgetColorInactive( const QColor& color );

		static bool moreBlackThanWhite( const QColor& color );

		/** Function used to update the global palette of the QApplication.
		 *
		 * It will get the most recent color values from the
		 * #H2Core::Preferences. */
		static void setPalette( QApplication *pQApp );

		static void setPlayheadPen( QPainter* p, bool bHovered = false );

		/** The default color for disabled icons is a mid grayish one. In
		 * combination with our default black icon color this leaves very little
		 * room for picking a background color of the tool bar to work with both
		 * of them. Yet alone one that is distinct from the other toolbars and
		 * integrates well in the overall UI. Therefore, we use this wrapper
		 * function to introduce our own disabled color for all icons in a
		 * toolbar.
		 *
		 * @{*/
		static void setToolBarIcon(
			QToolBar* pToolBar,
			QAction* pAction,
			const QString& sIconPath,
			const QColor& disabledColor
		);
		static void setToolBarIcon(
			QToolBar* pToolBar,
			QToolButton* pButton,
			const QString& sIconPath,
			const QColor& disabledColor
		);
		/** @} */

		/** Toolbars in Qt are quite hard to get right. This routine will
		 * help us to keep a consistent styling throughout all of them.*/
		static void setToolBarStyle(
			QToolBar* pToolBar,
			const QColor& background,
			bool bDrawBorders
		);
};

#endif
