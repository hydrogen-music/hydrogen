/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef PLAYER_CONTROL_H
#define PLAYER_CONTROL_H

#include <QtGui>
#include <QtWidgets>

#include "EventListener.h"
#include <core/Object.h>
#include <core/Preferences.h>
#include "Widgets/WidgetWithScalableFont.h"

namespace H2Core
{
	class Hydrogen;
}

class LCDSpinBox;
class LCDDisplay;
class Button;
class ToggleButton;
class CpuLoadWidget;
class MidiActivityWidget;
class PixmapWidget;

///
///
///
class MetronomeWidget : public QWidget,public EventListener, public H2Core::Object
{
    H2_OBJECT(MetronomeWidget)
	Q_OBJECT
	public:
		explicit MetronomeWidget(QWidget *pParent);
		~MetronomeWidget();

		virtual void metronomeEvent( int nValue ) override;
		virtual void paintEvent( QPaintEvent*) override;


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
class PlayerControl : public QLabel, protected WidgetWithScalableFont<5, 6, 7>, public EventListener, public H2Core::Object
{
    H2_OBJECT(PlayerControl)
	Q_OBJECT
	public:
		explicit PlayerControl(QWidget *parent);
		~PlayerControl();

		void showMessage( const QString& msg, int msec );
		void showScrollMessage( const QString& msg, int msec, bool test );
		void resetStatusLabel();

		virtual void tempoChangedEvent( int nValue ) override;
		virtual void jackTransportActivationEvent( int nValue ) override;
		virtual void jackTimebaseActivationEvent( int nValue ) override;
		/**
		 * Shared GUI update when activating Song or Pattern mode via
		 * button click or via OSC command.
		 *
		 * @param nValue If 0, Pattern mode will be activate. Else,
		 * Song mode will be activated instead.
		 */
		void songModeActivationEvent( int nValue ) override;

public slots:
		void onPreferencesChanged( bool bAppearanceOnly );

	private slots:
		void recBtnClicked(Button* ref);
		void playBtnClicked(Button* ref);
		void stopBtnClicked(Button* ref);
		void updatePlayerControl();
		void songModeBtnClicked(Button* ref);
		void liveModeBtnClicked(Button* ref);
		void jackTransportBtnClicked(Button* ref);
		//jack time master
		void jackMasterBtnClicked(Button* ref);
		//~ jack time master
		void bpmChanged();
		void bpmButtonClicked( Button *pRef );
		void bpmClicked();
		void FFWDBtnClicked(Button *pRef);
		void RewindBtnClicked(Button *pRef);
		void songLoopBtnClicked(Button* ref);
		void metronomeButtonClicked(Button* ref);
		void onStatusTimerEvent();
		void onScrollTimerEvent();
		void showButtonClicked( Button* pRef );

		//beatcounter
		void bconoffBtnClicked( Button* ref);
		void bcSetPlayBtnClicked(Button* ref);
		void bcbButtonClicked(Button* bBtn);
		void bctButtonClicked(Button* tBtn);
		//~ beatcounter
		
		//rubberband
		void rubberbandButtonToggle(Button* ref);

	private:
		/**
		 * Shared GUI update when activating loop mode via button
		 * click or via OSC command.
		 *
		 * @param nValue If 0, loop mode will be deactivate.
		 */
		void loopModeActivationEvent( int nValue ) override;
		H2Core::Hydrogen *m_pHydrogen;
		QPixmap m_background;

		Button *m_pRwdBtn;
		ToggleButton *m_pRecBtn;
		ToggleButton *m_pPlayBtn;
		Button *m_pStopBtn;
		Button *m_pFfwdBtn;

		ToggleButton *m_pSongLoopBtn;

		ToggleButton *m_pSongModeBtn;
		ToggleButton *m_pLiveModeBtn;

		//beatcounter
		/** Store the tool tip of the beat counter since it gets
			overwritten during deactivation.*/
		QString m_sBConoffBtnToolTip;
		ToggleButton *m_pBConoffBtn;
		ToggleButton *m_pBCSpaceBtn;
		ToggleButton *m_pBCSetPlayBtn;
		Button *m_pBCTUpBtn;
		Button *m_pBCTDownBtn;
		Button *m_pBCBUpBtn;
		Button *m_pBCBDownBtn;
		//~ beatcounter

		//rubberbandBPMChange
		ToggleButton *m_pRubberBPMChange;

		ToggleButton *m_pJackTransportBtn;
		//jack time master
		ToggleButton *m_pJackMasterBtn;
		QString m_sJackMasterModeToolTip;
		//~ jack time master
		Button *m_pBPMUpBtn;
		Button *m_pBPMDownBtn;

		CpuLoadWidget *m_pCpuLoadWidget;
		MidiActivityWidget *m_pMidiActivityWidget;

		LCDSpinBox *m_pLCDBPMSpinbox;

		LCDDisplay *m_pTimeDisplayH;
		LCDDisplay *m_pTimeDisplayM;
		LCDDisplay *m_pTimeDisplayS;
		LCDDisplay *m_pTimeDisplayMS;

		//beatcounter
		PixmapWidget *m_pControlsBCPanel;

		LCDDisplay *m_pBCDisplayZ;
		LCDDisplay *m_pBCDisplayB;
		LCDDisplay *m_pBCDisplayT;
		//~ beatcounter

		MetronomeWidget *m_pMetronomeWidget;
		ToggleButton *m_pMetronomeBtn;

		ToggleButton *m_pShowMixerBtn;
		ToggleButton *m_pShowInstrumentRackBtn;

		LCDDisplay *m_pStatusLabel;
		QTimer *m_pStatusTimer;
		QTimer *m_pScrollTimer;
		QString m_pScrollMessage;
		/** Used to detect changed in the font*/
		H2Core::Preferences::FontSize m_lastUsedFontSize;
};


#endif
