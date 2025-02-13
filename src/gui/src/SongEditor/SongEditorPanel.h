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

class SongEditor;
class SongEditorPatternList;
class SongEditorPositionRuler;
class Button;
class Fader;
class WidgetWithInput;
class AutomationPathView;
class LCDCombo;
class PlaybackTrackWaveDisplay;

/** \ingroup docGUI*/
class SongEditorPanel : public QWidget,
						public EventListener,
						public H2Core::Object<SongEditorPanel>
{
	H2_OBJECT(SongEditorPanel)
	Q_OBJECT

	public:
		explicit SongEditorPanel( QWidget *parent );
		~SongEditorPanel();

		SongEditor* getSongEditor() const { return m_pSongEditor; }
		SongEditorPatternList* getSongEditorPatternList() const { return m_pPatternList; }
		SongEditorPositionRuler* getSongEditorPositionRuler() const { return m_pPositionRuler; }
		AutomationPathView* getAutomationPathView() const { return m_pAutomationPathView; }
	PlaybackTrackWaveDisplay* getPlaybackTrackWaveDisplay() const { return m_pPlaybackTrackWaveDisplay; }

		void updateAll();
		void updatePositionRuler();
		
		void showTimeline();
		void showPlaybackTrack();
		void updatePlaybackTrackIfNecessary();

		void setTimelineActive( bool bActive );
		void setTimelineEnabled( bool bEnabled );

		/**
		 * Turns the background color of #m_pPatternEditorLockedBtn red
		 * temporarily to signal the user her last action was not permitted.
		 */
		void highlightPatternEditorLocked();
		void restoreGroupVector( const QString& filename );

		static constexpr int m_nMinimumHeight = 50;
		
		// Implements EventListener interface
		/** Updates the associated buttons if the action mode was
		 * changed within the core.
		 *
		 * \param nValue 0 - select mode and 1 - draw mode.
		 */
		virtual void actionModeChangeEvent( int nValue ) override;
		virtual void gridCellToggledEvent() override;
		virtual void jackTimebaseStateChangedEvent( int nState ) override;
		virtual void patternEditorLockedEvent() override;
		virtual void patternModifiedEvent() override;
		virtual void playbackTrackChangedEvent() override;
		virtual void playingPatternsChangedEvent() override;
		virtual void selectedPatternChangedEvent() override;
		virtual void songModeActivationEvent() override;
		virtual void stackedModeActivationEvent( int ) override;
		virtual void stateChangedEvent( const H2Core::AudioEngine::State& ) override;
		virtual void timelineActivationEvent() override;
		virtual void updateSongEvent( int ) override;

	public slots:
	/** Used by the shotlist during automated generation of images
		for the manual. */
	void activateStackedMode( bool bActivate );
	void activateSelectMode( bool bActivate );
		void toggleAutomationAreaVisibility();


	private slots:
		void vScrollTo( int value );
		void hScrollTo( int value );

		void newPatBtnClicked();
		void upBtnClicked();
		void downBtnClicked();
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


		Button *					m_pUpBtn;
		Button *					m_pDownBtn;
		Button *					m_pClearPatternSeqBtn;
		Button *					m_pSelectionModeBtn;
		Button *					m_pDrawModeBtn;
		
		Fader*						m_pPlaybackTrackFader;

		Button *					m_pTimelineBtn;
		Button *					m_pViewTimelineBtn;
		Button *					m_pViewPlaybackBtn;
		Button *					m_pMutePlaybackBtn;
		Button *					m_pEditPlaybackBtn;

		Button *			m_pPlaySelectedSingleBtn;
		Button *			m_pPlaySelectedMultipleBtn;
		Button *			m_pPatternEditorLockedBtn;
		Button *			m_pPatternEditorUnlockedBtn;


		QTimer*						m_pHighlightLockedTimer;
		QTimer*						m_pTimer;
		
		AutomationPathView *		m_pAutomationPathView;
		LCDCombo*					m_pAutomationCombo;

		virtual void				resizeEvent( QResizeEvent *ev ) override;
		void						resyncExternalScrollBar();

		bool m_bLastIsTimelineActivated;
};

#endif
