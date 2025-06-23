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

#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include "../EventListener.h"
#include "../Widgets/WidgetWithScalableFont.h"

class BeatCounter;
class Button;
class ClickableLabel;
class LCDSpinBox;
class LCDDisplay;
class LED;
class MetronomeButton;
class MidiControlButton;
class PanelGroupBox;

/** \ingroup docGUI*/
class PlayerControl : public QWidget,
					  protected WidgetWithScalableFont<5, 6, 7>,
					  public EventListener,
					  public H2Core::Object<PlayerControl> {
    H2_OBJECT(PlayerControl)
	Q_OBJECT
public:

		static constexpr int nBorder = 1;
		static constexpr int nFontSize = 20;
		static constexpr int nHeight = 36;
		static constexpr int nMargin = 2;
		static constexpr int nWidgetHeight = PlayerControl::nHeight -
			PlayerControl::nMargin * 2;


		explicit PlayerControl(QWidget *parent);
		~PlayerControl();

		void updatePlayerControl();

		void mousePressEvent( QMouseEvent* pEvent ) override;

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

	//rubberband
	void rubberbandButtonToggle();

		void updateTime();
private:
		void updateStyleSheet();

		LCDDisplay* m_pTimeDisplay;

		PanelGroupBox* m_pEditorGroup;
		Button* m_pSongModeBtn;
		Button* m_pPatternModeBtn;

		QWidget* m_pTransportGroup;
		Button* m_pRwdBtn;
		Button* m_pRecBtn;
		Button* m_pPlayBtn;
		Button* m_pStopBtn;
		Button* m_pFfwdBtn;
		Button* m_pSongLoopBtn;

		QWidget* m_pBeatCounterGroup;
		BeatCounter* m_pBeatCounter;

		QWidget* m_pTempoGroup;
		MetronomeButton* m_pMetronomeBtn;
		LCDSpinBox* m_pBpmSpinBox;

		QWidget* m_pRubberBandGroup;
		Button* m_pRubberBandBtn;

		QWidget* m_pJackGroup;
		Button* m_pJackTransportBtn;
		Button* m_pJackTimebaseBtn;

		MidiControlButton* m_pMidiControlButton;

		QWidget* m_pVisibilityGroup;
		Button* m_pShowMixerBtn;
		Button* m_pShowInstrumentRackBtn;
		Button* m_pShowPreferencesBtn;
		Button* m_pShowDirectorBtn;
		Button* m_pShowPlaylistEditorBtn;
		Button* m_pShowAutomationBtn;
		Button* m_pShowPlaybackTrackBtn;

		QMenu* m_pPopupMenu;

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
