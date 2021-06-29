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

class SongEditorPanel : public QWidget, public EventListener, public H2Core::Object
{
	H2_OBJECT
	Q_OBJECT

	public:
		explicit SongEditorPanel( QWidget *parent );
		~SongEditorPanel();

		SongEditor* getSongEditor(){ return m_pSongEditor; }
		SongEditorPatternList* getSongEditorPatternList(){ return m_pPatternList; }
		SongEditorPositionRuler* getSongEditorPositionRuler(){ return m_pPositionRuler; }
		AutomationPathView* getAutomationPathView() const { return m_pAutomationPathView; }

		void updateAll();
		void updatePositionRuler();
		void toggleAutomationAreaVisibility();
		
		void showTimeline();
		void showPlaybackTrack();
		void updatePlaybackTrackIfNecessary();
		
		// Implements EventListener interface
		virtual void selectedPatternChangedEvent() override;
		void restoreGroupVector( QString filename );
		//~ Implements EventListener interface	
		///< an empty new pattern will be added to pattern list at idx
		void insertPattern( int idx, H2Core::Pattern* pPattern );
		///< pattern at idx within pattern list will be destroyed
		void deletePattern( int idx );

		/** Disables and deactivates the Timeline when an external
		 * JACK timebase master is detected and enables it when it's
		 * gone or Hydrogen itself becomes the timebase master.
		 */
		void updateTimelineUsage();
		virtual void timelineActivationEvent( int nValue ) override;
		/** Updates the associated buttons if the action mode was
		 * changed within the core.
		 *
		 * \param nValue 0 - select mode and 1 - draw mode.
		 */
		void actionModeChangeEvent( int nValue ) override;

	public slots:
		void setModeActionBtn( bool mode );
		void showHideTimeLine( bool bPressed ) {
			m_pTimeLineBtn->setChecked( bPressed );
			timeLineBtnPressed();
		}

	private slots:
		void vScrollTo( int value );
		void hScrollTo( int value );

		void newPatBtnClicked();
		void upBtnClicked();
		void downBtnClicked();
		void clearSequence();
		
		void updatePlaybackFaderPeaks();
		void updatePlayHeadPosition();

		void selectionModeBtnPressed();
		void drawModeBtnPressed();
		void timeLineBtnPressed();
		void viewTimeLineBtnPressed();
		void viewPlaybackTrackBtnPressed();
		void mutePlaybackTrackBtnPressed();
		void editPlaybackTrackBtnPressed();
		void modeActionBtnPressed( );

		void zoomInBtnPressed();
		void zoomOutBtnPressed();
		
		void faderChanged( WidgetWithInput* pRef );

		void automationPathChanged();
		void automationPathPointAdded(float x, float y);
		void automationPathPointRemoved(float x, float y);
		void automationPathPointMoved(float ox, float oy, float tx, float ty);

	private:
		uint					m_nInitialWidth;
		uint					m_nInitialHeight;

		static const int		m_nPatternListWidth = 200;

		QScrollArea*			m_pEditorScrollView;
		QScrollArea*			m_pPatternListScrollView;
		QScrollArea*			m_pPositionRulerScrollView;
		QScrollArea*			m_pPlaybackTrackScrollView;
		
		QScrollBar *			m_pVScrollBar;
		QScrollBar *			m_pHScrollBar;
		
		QStackedWidget*			m_pWidgetStack;
		QScrollArea*			m_pAutomationPathScrollView;


		SongEditor*				m_pSongEditor;
		SongEditorPatternList *	m_pPatternList;
		SongEditorPositionRuler *m_pPositionRuler;
		PlaybackTrackWaveDisplay*	 m_pPlaybackTrackWaveDisplay;


		Button *				m_pUpBtn;
		Button *				m_pDownBtn;
		Button *				m_pClearPatternSeqBtn;
		Button *			m_pSelectionModeBtn;
		Button *			m_pModeActionSingleBtn;
		Button *			m_pModeActionMultipleBtn;
		Button *			m_pDrawModeBtn;
		Button *			m_pTagbarBtn;
		
		Fader*					m_pPlaybackTrackFader;

		/** Store the tool tip of the Timeline since it gets
			overwritten during deactivation.*/
		QString					m_sTimelineToolTip;
		Button *			m_pTimeLineBtn;
		Button *			m_pPlaybackBtn;
		Button *			m_pViewTimeLineBtn;
		Button *			m_pViewPlaybackBtn;
		Button *			m_pMutePlaybackBtn;
		Button *				m_pEditPlaybackBtn;

		QTimer*					m_pTimer;
		
		AutomationPathView *	m_pAutomationPathView;
		LCDCombo*				m_pAutomationCombo;


		virtual void resizeEvent( QResizeEvent *ev ) override;
		void resyncExternalScrollBar();
};

#endif
