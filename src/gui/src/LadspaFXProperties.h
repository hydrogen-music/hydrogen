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

#ifndef LADSPA_FX_PROPERTIES_H
#define LADSPA_FX_PROPERTIES_H

#include <vector>

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>

class Fader;
class LCDDisplay;
class InstrumentNameWidget;
class WidgetWithInput;

/** \ingroup docGUI*/
class LadspaFXProperties :  public QWidget,  public H2Core::Object<LadspaFXProperties> {
    H2_OBJECT(LadspaFXProperties)
	Q_OBJECT

	public:
		LadspaFXProperties(QWidget* parent, uint nLadspaFX);
		~LadspaFXProperties();

		void updateControls();

		virtual void showEvent ( QShowEvent *ev ) override;
		virtual void closeEvent( QCloseEvent *ev ) override;

	public slots:
		void faderChanged( WidgetWithInput* ref );
		void selectFXBtnClicked();
		void removeFXBtnClicked();
		void activateBtnClicked();
		void updateOutputControls();

	private:
		uint m_nLadspaFX;

		QLabel *m_pNameLbl;

		std::vector<Fader*> m_pInputControlFaders;
		std::vector<InstrumentNameWidget*> m_pInputControlNames;
		std::vector<LCDDisplay*> m_pInputControlLabel;

		std::vector<Fader*> m_pOutputControlFaders;
		std::vector<InstrumentNameWidget*> m_pOutputControlNames;

		QScrollArea* m_pScrollArea;
		QFrame* m_pFrame;

		QPushButton *m_pSelectFXBtn;
		QPushButton *m_pActivateBtn;
		QPushButton *m_pRemoveFXBtn;

		QTimer* m_pTimer;
};

#endif

