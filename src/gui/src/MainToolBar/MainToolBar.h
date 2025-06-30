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

#ifndef MAIN_TOOL_BAR_H
#define MAIN_TOOL_BAR_H

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include "../EventListener.h"
#include "../Widgets/WidgetWithScalableFont.h"

class BeatCounter;
class BpmSpinBox;
class Button;
class ClickableLabel;
class LCDSpinBox;
class LCDDisplay;
class LED;
class MidiControlButton;
class MidiLearnableToolButton;
class PanelGroupBox;

/** \ingroup docGUI*/
class MainToolBar : public QToolBar,
					protected WidgetWithScalableFont<5, 6, 7>,
					public EventListener,
					public H2Core::Object<MainToolBar> {
    H2_OBJECT(MainToolBar)
	Q_OBJECT
public:

		static constexpr int nBorder = 1;
		static constexpr int nFontSize = 20;
		static constexpr int nHeight = 40;
		static constexpr int nMargin = 3;
		static constexpr int nSpacing = 3;
		static constexpr int nWidgetHeight = MainToolBar::nHeight -
			MainToolBar::nMargin * 2;


		explicit MainToolBar(QWidget *parent);
		~MainToolBar();

		/** Unlike all other widgets we have provide visibility buttons for,
		 * #PreferencesDialog is a transient widget we can not check visibility
		 * on. */
		void setPreferencesVisibilityState( bool bChecked );
		void updateActions();

		void mousePressEvent( QMouseEvent* pEvent ) override;

		void beatCounterEvent() override;
		void driverChangedEvent() override;
		void jackTimebaseStateChangedEvent( int nState ) override;
		void jackTransportActivationEvent() override;
		void loopModeActivationEvent() override;
		void metronomeEvent( int ) override;
		void songModeActivationEvent() override;
		void stateChangedEvent( const H2Core::AudioEngine::State& ) override;
		void tempoChangedEvent( int nValue ) override;
		void timelineActivationEvent() override;
		void updateSongEvent( int nValue ) override;

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

	//rubberband
	void rubberbandButtonToggle();

		void updateTime();
private:
		void updateIcons();
		void updateStyleSheet();

		LCDDisplay* m_pTimeDisplay;

		QAction* m_pRwdAction;
		QAction* m_pRecAction;
		QAction* m_pPlayAction;
		QAction* m_pStopAction;
		QAction* m_pFfwdAction;
		QAction* m_pSongLoopAction;

		PanelGroupBox* m_pEditorGroup;
		QAction* m_pSongModeAction;
		QAction* m_pPatternModeAction;

		MidiLearnableToolButton* m_pMetronomeButton;
		BpmSpinBox* m_pBpmSpinBox;

		BeatCounter* m_pBeatCounter;
		QAction* m_pBeatCounterAction;
		QAction* m_pBeatCounterSeparator;

		QAction* m_pRubberBandAction;

		QAction* m_pJackTransportAction;
		MidiLearnableToolButton* m_pJackTimebaseButton;

		MidiControlButton* m_pMidiControlButton;

		QAction* m_pShowPlaylistEditorAction;
		QAction* m_pShowDirectorAction;
		QAction* m_pShowMixerAction;
		QAction* m_pShowInstrumentRackAction;
		QAction* m_pShowPreferencesAction;
		QAction* m_pShowAutomationAction;
		QAction* m_pShowPlaybackTrackAction;

		QMenu* m_pPopupMenu;

		QTimer* m_pTimer;

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
