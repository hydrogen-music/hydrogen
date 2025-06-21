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


#ifndef PANEL_GROUP_BOX_H
#define PANEL_GROUP_BOX_H

#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <QtGui>
#include <QtWidgets>
#include <QSvgRenderer>

#include "../EventListener.h"

/** Provides a consistent groupping of e.g. buttons that are all part of the
 * same state (e.g. mutually exclusive song mode and pattern mode buttons). LED
 * identicating a user selection.
 *
 * \ingroup docGUI docWidgets*/
class PanelGroupBox : public QWidget, public H2Core::Object<PanelGroupBox>
{
    H2_OBJECT(PanelGroupBox)
	Q_OBJECT

public:
		static constexpr int nBorder = 1;
		static constexpr int nMarginVertical = 1;
		static constexpr int nMarginHorizontal = 2;
		static constexpr int nSpacing = 1;

		PanelGroupBox( QWidget* pParent );
		~PanelGroupBox();

		void addWidget( QWidget* pWidget );

		void setBorderColor( const QColor& borderColor );

		void updateStyleSheet();

	private:
		QHBoxLayout* m_pLayout;

		QColor m_borderColor;
};

inline void PanelGroupBox::setBorderColor( const QColor& borderColor ) {
	m_borderColor = borderColor;
}
#endif
