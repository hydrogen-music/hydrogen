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

#ifndef SONG_EDITOR_PANEL_H
#define SONG_EDITOR_PANEL_H


#include "../EventListener.h"
#include <core/Object.h>
#include <core/Basics/Pattern.h>

#include <QtGui>
#include <QtWidgets>

#include "Widgets/Button.h"
#include "Widgets/EditorDefs.h"

class AutomationPathView;
class Button;
class Fader;
class LCDCombo;
class MidiLearnableToolButton;
class PlaybackTrackWaveDisplay;
class SongEditor;
class SongEditorPatternList;
class SongEditorPositionRuler;
class WidgetWithInput;

/** \ingroup docGUI*/
class SongEditorPanel : public QWidget,
						public EventListener,
						public H2Core::Object<SongEditorPanel>
{
	H2_OBJECT(SongEditorPanel)
	Q_OBJECT

	public:
		static constexpr int nMinimumHeight = 50;

		explicit SongEditorPanel( QWidget *parent );
		~SongEditorPanel();

		SongEditor* getSongEditor() const { return m_pSongEditor; }
		SongEditorPatternList* getSongEditorPatternList() const { return m_pPatternList; }
		SongEditorPositionRuler* getSongEditorPositionRuler() const { return m_pPositionRuler; }
		AutomationPathView* getAutomationPathView() const { return m_pAutomationPathView; }
	PlaybackTrackWaveDisplay* getPlaybackTrackWaveDisplay() const { return m_pPlaybackTrackWaveDisplay; }

		void ensureCursorIsVisible();
		void updateEditors( Editor::Update update );

		void showTimeline();
		void showPlaybackTrack();
		void updatePlaybackTrack();

		/**
		 * Turns the background color of #m_pPatternEditorLockedBtn red
		 * temporarily to signal the user her last action was not permitted.
		 */
		void highlightPatternEditorLocked();
		void restoreGroupVector( const QString& filename );

		/** @returns `true` in case any of the child editors or sidebar has
		 * focus.*/
		bool hasSongEditorFocus() const;

		// Implements EventListener interface
		/** Updates the associated buttons if the action mode was
		 * changed within the core.
		 *
		 * \param nValue 0 - select mode and 1 - draw mode.
		 */
		virtual void gridCellToggledEvent() override;
		virtual void jackTimebaseStateChangedEvent( int nState ) override;
		virtual void midiClockActivationEvent() override;
		virtual void nextPatternsChangedEvent() override;
		virtual void patternEditorLockedEvent() override;
		virtual void patternModifiedEvent() override;
		virtual void playbackTrackChangedEvent() override;
		virtual void playingPatternsChangedEvent() override;
		virtual void relocationEvent() override;
		virtual void selectedPatternChangedEvent() override;
		virtual void songModeActivationEvent() override;
		virtual void songSizeChangedEvent() override;
		virtual void stackedModeActivationEvent( int ) override;
		virtual void stateChangedEvent( const H2Core::AudioEngine::State& ) override;
		virtual void tempoChangedEvent( int ) override;
		virtual void timelineActivationEvent() override;
		virtual void timelineUpdateEvent( int ) override;
		virtual void updateSongEvent( int ) override;

	public slots:
		void onPreferencesChanged( const H2Core::Preferences::Changes& changes );
		void toggleAutomationAreaVisibility();


	private slots:
		void vScrollTo( int value );
		void hScrollTo( int value );

		void newPatBtnClicked();
		void clearSequence();
		
		void updatePlaybackFaderPeaks();
		void updatePlayHeadPosition();

		void timelineBtnClicked();
		void viewTimelineBtnClicked();
		void viewPlaybackTrackBtnClicked();
		void editPlaybackTrackBtnClicked();

		void zoomInBtnClicked();
		void zoomOutBtnClicked();
		
		void faderChanged( WidgetWithInput* pRef );

		void automationPathPointAdded(float x, float y);
		void automationPathPointRemoved(float x, float y);
		void automationPathPointMoved(float ox, float oy, float tx, float ty);

	private:
		virtual void resizeEvent( QResizeEvent *ev ) override;

		void resyncExternalScrollBar();

		void setTimelineActive( bool bActive );
		void setTimelineEnabled( bool bEnabled );

		void updateIcons();
		void updateJacktimebaseState();
		void updatePatternEditorLocked();
		void updatePatternMode();
		void updateStyleSheet();
		void updateTimeline();

		static const int			m_nPatternListWidth = 200;
									
		QScrollArea*				m_pEditorScrollView;
		QScrollArea*				m_pPatternListScrollView;
		QScrollArea*				m_pPositionRulerScrollView;
		QScrollArea*				m_pPlaybackTrackScrollView;
									
		QScrollBar *				m_pVScrollBar;
		QScrollBar *				m_pHScrollBar;
									
		QStackedWidget*				m_pWidgetStack;
		QScrollArea*				m_pAutomationPathScrollView;
									
		SongEditor*					m_pSongEditor;
		SongEditorPatternList *		m_pPatternList;
		SongEditorPositionRuler *	m_pPositionRuler;
		PlaybackTrackWaveDisplay*	m_pPlaybackTrackWaveDisplay;

		QToolBar*					m_pToolBar;
		QAction*					m_pClearAction;
		QAction*					m_pNewPatternAction;
		MidiLearnableToolButton*	m_pSinglePatternModeButton;
		MidiLearnableToolButton*	m_pStackedPatternModeButton;
		MidiLearnableToolButton*	m_pPatternEditorLockedButton;

		Fader*						m_pPlaybackTrackFader;

		Button *					m_pTimelineBtn;
		Button *					m_pViewTimelineBtn;
		Button *					m_pViewPlaybackBtn;
		Button *					m_pMutePlaybackBtn;
		Button *					m_pEditPlaybackBtn;


		QTimer*						m_pHighlightLockedTimer;
		QTimer*						m_pTimer;
		
		AutomationPathView *		m_pAutomationPathView;
		LCDCombo*					m_pAutomationCombo;

		bool m_bLastIsTimelineActivated;
};

#endif
