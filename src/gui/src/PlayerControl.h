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
#include <chrono>

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
class ClickableLabel;

/** \ingroup docGUI*/
class PlayerControl :  public QLabel, protected WidgetWithScalableFont<5, 6, 7>, public EventListener,  public H2Core::Object<PlayerControl> {
    H2_OBJECT(PlayerControl)
	Q_OBJECT
public:
	explicit PlayerControl(QWidget *parent);
	~PlayerControl();

	void showMessage( const QString& msg, int msec );
	void showScrollMessage( const QString& msg, int msec, bool test );
	void resetStatusLabel();

	virtual void timelineActivationEvent( int ) override;
	virtual void tempoChangedEvent( int nValue ) override;
	virtual void jackTransportActivationEvent( int nValue ) override;
	virtual void jackTimebaseStateChangedEvent( int nValue ) override;
	/**
	 * Shared GUI update when activating Song or Pattern mode via
	 * button click or via OSC command.
	 *
	 * @param nValue If 0, Pattern mode will be activate. Else,
	 * Song mode will be activated instead.
	 */
	void songModeActivationEvent( int nValue ) override;

public slots:
	void onPreferencesChanged( H2Core::Preferences::Changes changes );

private slots:
	void recBtnClicked();
	void playBtnClicked();
	void stopBtnClicked();
	void updatePlayerControl();
	void songModeBtnClicked();
	void patternModeBtnClicked();
	void jackTransportBtnClicked();
	//jack time master
	void jackMasterBtnClicked();	//~ jack time master
	void bpmChanged( double );
	void fastForwardBtnClicked();
	void rewindBtnClicked();
	void songLoopBtnClicked();
	void metronomeButtonClicked();
	void onStatusTimerEvent();
	void onScrollTimerEvent();
	void showMixerButtonClicked();
	void showInstrumentRackButtonClicked();

	//beatcounter
	void bcOnOffBtnClicked();
	void bcSetPlayBtnClicked();
	void bcbUpButtonClicked();
	void bcbDownButtonClicked();
	void bctUpButtonClicked();
	void bctDownButtonClicked();
	//~ beatcounter
		
	//rubberband
	void rubberbandButtonToggle();

	void deactivateMidiActivityLED();
private:
	/** Ensure that the full width of the status label is used without
	 * cutting of the beginning of the message.*/
	void updateStatusLabel();
	/**
	 * Shared GUI update when activating loop mode via button
	 * click or via OSC command.
	 *
	 * @param nValue If 0, loop mode will be deactivate.
	 */
	void loopModeActivationEvent( int nValue ) override;
	void midiActivityEvent() override;
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
	Button *m_pBCOnOffBtn;
	Button *m_pBCSpaceBtn;
	Button *m_pBCSetPlayBtn;
	Button *m_pBCTUpBtn;
	Button *m_pBCTDownBtn;
	Button *m_pBCBUpBtn;
	Button *m_pBCBDownBtn;
	//~ beatcounter

	//rubberbandBPMChange
	Button *m_pRubberBPMChange;

	Button *m_pJackTransportBtn;
	//jack time master
	Button *m_pJackMasterBtn;
	QString m_sJackMasterModeToolTip;
	//~ jack time master

	CpuLoadWidget *m_pCpuLoadWidget;
	LED *m_pMidiActivityLED;
	ClickableLabel* m_pMidiInLbl;
	ClickableLabel* m_pCpuLbl;

	LCDSpinBox *m_pLCDBPMSpinbox;
	ClickableLabel* m_pBPMLbl;
	LCDDisplay *m_pTimeDisplay;
	ClickableLabel* m_pTimeHoursLbl;
	ClickableLabel* m_pTimeMinutesLbl;
	ClickableLabel* m_pTimeSecondsLbl;
	ClickableLabel* m_pTimeMilliSecondsLbl;

	//beatcounter
	PixmapWidget *m_pControlsBCPanel;

	QLabel *m_pBCDisplayZ;
	QLabel *m_pBCDisplayB;
	QLabel *m_pBCDisplayT;
	//~ beatcounter

	MetronomeLED *m_pMetronomeLED;
	Button *m_pMetronomeBtn;

	Button *m_pShowMixerBtn;
	Button *m_pShowInstrumentRackBtn;

	LCDDisplay *m_pStatusLabel;
	QTimer *m_pStatusTimer;
	QTimer *m_pScrollTimer;
	QString m_pScrollMessage;
	/** Used to turn off the LED #m_pMidiActivityLED indicating an
		incoming MIDI event after #m_midiActivityTimeout
		milliseconds.*/ 
	QTimer *m_pMidiActivityTimer;
	std::chrono::milliseconds m_midiActivityTimeout;

	bool m_bLastBCOnOffBtnState;
	
	/** Store the tool tip of the beat counter since it gets
		overwritten during deactivation.*/
	void updateBPMSpinbox();
	void updateBeatCounter();
	void updateBPMSpinboxToolTip();
	void updateBeatCounterToolTip();
	QString m_sBCOnOffBtnToolTip;
	QString m_sBCOnOffBtnTimelineToolTip;
	QString m_sBCOnOffBtnJackTimebaseToolTip;
	QString m_sLCDBPMSpinboxToolTip;
	QString m_sLCDBPMSpinboxTimelineToolTip;
	QString m_sLCDBPMSpinboxJackTimebaseToolTip;
};


#endif
