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

#ifndef PLAYER_CONTROL_H
#define PLAYER_CONTROL_H

#include <QtGui>
#include <QtWidgets>

#include "EventListener.h"
#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include "Widgets/WidgetWithScalableFont.h"

namespace H2Core
{
	class Hydrogen;
}

class LCDSpinBox;
class LCDDisplay;
class Button;
class CpuLoadWidget;
class PixmapWidget;
class LED;
class MetronomeLED;
class MidiControlButton;
class ClickableLabel;
class StatusMessageDisplay;

/** \ingroup docGUI*/
class PlayerControl : public QLabel,
					  protected WidgetWithScalableFont<5, 6, 7>,
					  public EventListener,
					  public H2Core::Object<PlayerControl> {
    H2_OBJECT(PlayerControl)
	Q_OBJECT
public:
	explicit PlayerControl(QWidget *parent);
	~PlayerControl();

	void updatePlayerControl();

	void showStatusBarMessage( const QString& msg, const QString& sCaller = "" );

		virtual void beatCounterEvent() override;
		virtual void driverChangedEvent() override;
		virtual void jackTimebaseStateChangedEvent( int nState ) override;
		virtual void jackTransportActivationEvent() override;
		virtual void loopModeActivationEvent() override;
		virtual void metronomeEvent( int ) override;
		virtual void songModeActivationEvent() override;
		virtual void stateChangedEvent( const H2Core::AudioEngine::State& ) override;
		virtual void tempoChangedEvent( int nValue ) override;
		virtual void timelineActivationEvent() override;
		virtual void updateSongEvent( int nValue ) override;

		static constexpr int m_nMinimumHeight = 43;

public slots:
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );
	void activateSongMode( bool bActivate );

private slots:
	void recBtnClicked();
	void playBtnClicked();
	void stopBtnClicked();
	void jackTransportBtnClicked();
	void jackTimebaseBtnClicked();
	void bpmChanged( double );
	void fastForwardBtnClicked();
	void rewindBtnClicked();
	void metronomeButtonClicked();

	//beatcounter
	void activateBeatCounter();
	void beatCounterSetPlayBtnClicked();
	void beatCounterTotalBeatsUpBtnClicked();
	void beatCounterTotalBeatsDownBtnClicked();
	void beatCounterBeatLengthUpBtnClicked();
	void beatCounterBeatLengthDownBtnClicked();
	// ~ beatcounter
		
	//rubberband
	void rubberbandButtonToggle();

		void updateTime();
private:
	/** Ensure that the full width of the status label is used without
	 * cutting of the beginning of the message.*/
	void updateStatusLabel();

	H2Core::Hydrogen *m_pHydrogen;
	QPixmap m_background;

	Button *m_pRwdBtn;
	Button *m_pRecBtn;
	Button *m_pPlayBtn;
	Button *m_pStopBtn;
	Button *m_pFfwdBtn;

	Button *m_pSongLoopBtn;
	Button *m_pSongModeBtn;
	LED			 *m_pSongModeLED;
	Button *m_pPatternModeBtn;
	LED			 *m_pPatternModeLED;

	//beatcounter
	PixmapWidget *m_pControlsBCPanel;

	QLabel *m_pBCDisplayZ;
	QLabel *m_pBeatCounterTotalBeatsDisplay;
	QLabel *m_pBeatCounterBeatLengthDisplay;

	Button *m_pBCOnOffBtn;
	Button *m_pBeatCounterSetPlayBtn;
	Button *m_pBeatCounterBeatLengthUpBtn;
	Button *m_pBeatCounterBeatLengthDownBtn;
	Button *m_pBeatCounterTotalBeatsUpBtn;
	Button *m_pBeatCounterTotalBeatsDownBtn;
	// ~ beatcounter

	//rubberbandBPMChange
	Button *m_pRubberBPMChange;

	Button *m_pJackTransportBtn;
	Button *m_pJackTimebaseBtn;

	CpuLoadWidget *m_pCpuLoadWidget;
	ClickableLabel* m_pCpuLbl;
		MidiControlButton* m_pMidiControlButton;

	LCDSpinBox *m_pBpmSpinBox;
	ClickableLabel* m_pBPMLbl;
	LCDDisplay *m_pTimeDisplay;
	ClickableLabel* m_pTimeHoursLbl;
	ClickableLabel* m_pTimeMinutesLbl;
	ClickableLabel* m_pTimeSecondsLbl;
	ClickableLabel* m_pTimeMilliSecondsLbl;

	MetronomeLED *m_pMetronomeLED;
	Button *m_pMetronomeBtn;

	Button *m_pShowMixerBtn;
	Button *m_pShowInstrumentRackBtn;

	StatusMessageDisplay *m_pStatusLabel;

		void updateBeatCounter();
		void updateBpmSpinBox();
		void updateJackTransport();
		void updateJackTimebase();
		void updateLoopMode();
		void updateSongMode();
		void updateTransportControl();

	QString m_sBCOnOffBtnToolTip;
	QString m_sBCOnOffBtnTimelineToolTip;
	QString m_sBCOnOffBtnJackTimebaseToolTip;
	QString m_sLCDBPMSpinboxToolTip;
	QString m_sLCDBPMSpinboxTimelineToolTip;
	QString m_sLCDBPMSpinboxJackTimebaseToolTip;
};


#endif
