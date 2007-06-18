/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: LadspaFXProperties.h,v 1.8 2005/05/01 19:50:57 comix Exp $
 *
 */


#ifndef LADSPA_FX_PROPERTIES_H
#define LADSPA_FX_PROPERTIES_H

#include "config.h"


#include <vector>
using std::vector;

#include "qwidget.h"
#include "qcombobox.h"
#include "qscrollview.h"
#include "qhbox.h"
#include "qpushbutton.h"
#include "qlabel.h"

#include "widgets/Fader.h"
#include "widgets/LCD.h"
#include "gui/Mixer/MixerLine.h"

#include "lib/Object.h"

class LadspaFXProperties : public QWidget, public Object {
	Q_OBJECT

	public:
		LadspaFXProperties(QWidget* parent, uint nLadspaFX);
		~LadspaFXProperties();

		void updateControls();

		void showEvent ( QShowEvent *ev );
		void hideEvent ( QShowEvent *ev );
		void closeEvent( QCloseEvent *ev );

	public slots:
		void faderChanged(Fader * ref);
		void selectFXBtnClicked();
		void activateBtnClicked();
		void updateOutputControls();

	private:
		uint m_nLadspaFX;

		QLabel *m_pNameLbl;

		vector<Fader*> m_pInputControlFaders;
		vector<InstrumentNameWidget*> m_pInputControlNames;
		vector<LCDDisplay*> m_pInputControlLabel;

		vector<Fader*> m_pOutputControlFaders;
		vector<InstrumentNameWidget*> m_pOutputControlNames;

		QScrollView* m_pScrollView;
		QFrame *m_pFrame;

		QPushButton *m_pSelectFXBtn;
		QPushButton *m_pActivateBtn;

		QTimer *m_pTimer;
};

#endif

