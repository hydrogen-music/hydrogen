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
class StatusMessageDisplay;

/** \ingroup docGUI*/
class PlayerControl :  public QLabel, protected WidgetWithScalableFont<5, 6, 7>, public EventListener,  public H2Core::Object<PlayerControl> {
    H2_OBJECT(PlayerControl)
	Q_OBJECT
public:
	explicit PlayerControl(QWidget *parent);
	~PlayerControl();

	void showStatusBarMessage( const QString& msg, const QString& sCaller = "" );

	virtual void timelineActivationEvent() override;
	virtual void tempoChangedEvent( int nValue ) override;
	virtual void jackTransportActivationEvent() override;
	/**
	 * Shared GUI update when activating Song or Pattern mode via
	 * button click or via OSC command.
	 *
	 * @param nValue If 0, Pattern mode will be activate. Else,
	 * Song mode will be activated instead.
	 */
	virtual void songModeActivationEvent() override;
	virtual void updateSongEvent( int nValue ) override;
	virtual void loopModeActivationEvent() override;
	virtual void driverChangedEvent() override;

		static constexpr int m_nMinimumHeight = 43;

public slots:
	void onPreferencesChanged( H2Core::Preferences::Changes changes );
	void activateSongMode( bool bActivate );
	virtual void jackTimebaseStateChangedEvent( int nState ) override;

private slots:
	void recBtnClicked();
	void playBtnClicked();
	void stopBtnClicked();
	void updatePlayerControl();
	void jackTransportBtnClicked();
	void jackTimebaseBtnClicked();
	void bpmChanged( double );
	void fastForwardBtnClicked();
	void rewindBtnClicked();
	void metronomeButtonClicked();
	void showMixerButtonClicked();
	void showInstrumentRackButtonClicked();

	//beatcounter
	void activateBeatCounter( bool bActivate );
	void bcSetPlayBtnClicked();
	void bcbUpButtonClicked();
	void bcbDownButtonClicked();
	void bctUpButtonClicked();
	void bctDownButtonClicked();
	// ~ beatcounter
		
	//rubberband
	void rubberbandButtonToggle();

	void deactivateMidiActivityLED();
private:
	/** Ensure that the full width of the status label is used without
	 * cutting of the beginning of the message.*/
	void updateStatusLabel();
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
	Button *m_pBCSetPlayBtn;
	Button *m_pBCTUpBtn;
	Button *m_pBCTDownBtn;
	Button *m_pBCBUpBtn;
	Button *m_pBCBDownBtn;
	// ~ beatcounter

	//rubberbandBPMChange
	Button *m_pRubberBPMChange;

	Button *m_pJackTransportBtn;
	Button *m_pJackTimebaseBtn;

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
	// ~ beatcounter

	MetronomeLED *m_pMetronomeLED;
	Button *m_pMetronomeBtn;

	Button *m_pShowMixerBtn;
	Button *m_pShowInstrumentRackBtn;

	StatusMessageDisplay *m_pStatusLabel;
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

	/** When updating the tempo of the BPM spin box it is crucial to
	 * indicated that this was done due to a batch event and not due
	 * to user input. Else a batch update would trigger its
	 * bpmChanged() slot, which in turn sets the core BPM again. When
	 * changing a lot of tempo very quick (switch between songs of
	 * different tempi) this spurious BPM setting will mess things up.*/
	bool m_bLCDBPMSpinboxIsArmed;
};


#endif
