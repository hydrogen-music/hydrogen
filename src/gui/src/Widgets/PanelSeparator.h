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


#ifndef PANEL_SEPARATOR_H
#define PANEL_SEPARATOR_H

#include <QtGui>
#include <QtWidgets>

/** Provides a consistent means to separate widgets of different topics within a
 * panel.
 *
 * \ingroup docGUI docWidgets*/
class PanelSeparator : public QWidget
{
	Q_OBJECT

public:
		static constexpr float fEffectiveHeight = 0.8;
		static constexpr int nMarginHorizontal = 4;
		static constexpr int nWidth = 1;

		PanelSeparator( QWidget* pParent );
		~PanelSeparator();

		void setColor( const QColor& Color );

	private:
		void paintEvent( QPaintEvent* pEvent ) override;

		QColor m_color;
};

#endif
