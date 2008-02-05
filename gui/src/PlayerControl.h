/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#ifndef PLAYER_CONTROL_H
#define PLAYER_CONTROL_H

#include <QLabel>
#include <QWidget>
#include <QPixmap>
#include <QPaintEvent>

#include "EventListener.h"
#include "HydrogenApp.h"

#include "widgets/LCD.h"
#include "widgets/Button.h"
#include "widgets/CpuLoadWidget.h"
#include "widgets/MidiActivityWidget.h"
#include "widgets/PixmapWidget.h"

#include <hydrogen/Object.h>
#include <hydrogen/hydrogen.h>


class MetronomeWidget : public QWidget, public Object, public EventListener
{
	Q_OBJECT
	public:
		MetronomeWidget(QWidget *pParent);
		~MetronomeWidget();

		virtual void metronomeEvent( int nValue );
		virtual void paintEvent( QPaintEvent*);

	public slots:
		void updateWidget();

	private:
		enum m_state {
			METRO_FIRST,
			METRO_ON,
			METRO_OFF
		};

		int m_nValue;
		int m_state;

		QPixmap m_metro_off;
		QPixmap m_metro_on_firstbeat;
		QPixmap m_metro_on;

};


///
/// Player control panel
///
class PlayerControl : public QLabel, public Object
{
	Q_OBJECT
	public:
		PlayerControl(QWidget *parent);
		~PlayerControl();

		void showMessage( const QString& msg, int msec );

	private slots:
		void playBtnClicked(Button* ref);
		void stopBtnClicked(Button* ref);
		void updatePlayerControl();
		void songModeBtnClicked(Button* ref);
		void liveModeBtnClicked(Button* ref);
		void switchModeBtnClicked(Button* ref);
		void jackTransportBtnClicked(Button* ref);
		void bpmChanged();
		void bpmButtonClicked( Button *pRef );
		void bpmButtonPressed( Button* pBtn);
		void bpmClicked();
		void FFWDBtnClicked(Button *pRef);
		void RewindBtnClicked(Button *pRef);
		void songLoopBtnClicked(Button* ref);
		void metronomeButtonClicked(Button* ref);
		void onBpmTimerEvent();
		void onStatusTimerEvent();
		void showButtonClicked( Button* pRef );

	private:
		H2Core::Hydrogen *m_pEngine;
		QPixmap m_background;

		Button *m_pRwdBtn;
		ToggleButton *m_pPlayBtn;
		Button *m_pStopBtn;
		Button *m_pFfwdBtn;

		ToggleButton *m_pSongLoopBtn;

		ToggleButton *m_pSongModeBtn;
		ToggleButton *m_pLiveModeBtn;
		Button *m_pSwitchModeBtn;

		ToggleButton *m_pJackTransportBtn;

		Button *m_pBPMUpBtn;
		Button *m_pBPMDownBtn;

		CpuLoadWidget *m_pCpuLoadWidget;
		MidiActivityWidget *m_pMidiActivityWidget;

		LCDSpinBox *m_pLCDBPMSpinbox;

		LCDDisplay *m_pTimeDisplayH;
		LCDDisplay *m_pTimeDisplayM;
		LCDDisplay *m_pTimeDisplayS;
		LCDDisplay *m_pTimeDisplayMS;

		MetronomeWidget *m_pMetronomeWidget;
		ToggleButton *m_pMetronomeBtn;

		QTimer *m_pBPMTimer;

		int m_nBPMIncrement;

		ToggleButton *m_pShowMixerBtn;
		ToggleButton *m_pShowInstrumentRackBtn;

		LCDDisplay *m_pStatusLabel;
		QTimer *m_pStatusTimer;
};


#endif
