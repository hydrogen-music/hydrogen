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


#ifndef METRONOME_BUTTON_H
#define METRONOME_BUTTON_H

#include <core/Object.h>

#include "../EventListener.h"
#include "../Widgets/Button.h"

#include <QtGui>
#include <QPushButton>


/** \ingroup docGUI docWidgets*/
class MetronomeButton : public Button
					  , public H2Core::Object<MetronomeButton>
{
    H2_OBJECT(MetronomeButton)
	Q_OBJECT

	public:
		MetronomeButton( QWidget *pParent, const QSize& size );
		~MetronomeButton();

		void metronomeEvent( int nValue ) override;

		void updateStyleSheet();

	private:
		QTimer* m_pTimer;

		QColor m_colorBackgroundChecked;
		QColor m_colorBeat;
		QColor m_colorFirstBeat;
};

#endif
