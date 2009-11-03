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

#ifndef SONG_EDITOR_PANEL_H
#define SONG_EDITOR_PANEL_H


#include "../EventListener.h"
#include <hydrogen/Object.h>

#include <QtGui>

class Button;
class SongEditor;
class SongEditorPatternList;
class SongEditorPositionRuler;
class ToggleButton;


enum SongEditorActionMode
{
	SELECT_ACTION,
	DRAW_ACTION
};


class SongEditorPanel : public QWidget, public EventListener, public Object
{
	Q_OBJECT

	public:
		SongEditorPanel( QWidget *parent );
		~SongEditorPanel();

		void updateAll();
		void updatePositionRuler();
		void setModeActionBtn( bool mode );
		SongEditorActionMode getActionMode() {	return m_actionMode;	}

		// Implements EventListener interface
		virtual void selectedPatternChangedEvent();
		//~ Implements EventListener interface
		

	private slots:
		void on_patternListScroll();
		void on_EditorScroll();
		void syncToExternalScrollBar();

		void newPatBtnClicked( Button* );
		void upBtnClicked( Button* );
		void downBtnClicked( Button* );
		void clearSequence( Button* );
		void updatePlayHeadPosition();

		void pointerActionBtnPressed( Button* pBtn );
		void drawActionBtnPressed( Button* pBtn );
		void timeLineBtnPressed( Button* pBtn );
		void modeActionBtnPressed( );

		void zoomInBtnPressed( Button* pBtn );
		void zoomOutBtnPressed( Button* pBtn );


	private:
		SongEditorActionMode m_actionMode;

		uint m_nInitialWidth;
		uint m_nInitialHeight;

		static const int m_nPatternListWidth = 200;

		QScrollArea* m_pEditorScrollView;
		QScrollArea* m_pPatternListScrollView;
		QScrollArea* m_pPositionRulerScrollView;
		QScrollBar *m_pVScrollBar;
		QScrollBar *m_pHScrollBar;


		SongEditor* m_pSongEditor;
		SongEditorPatternList *m_pPatternList;
		SongEditorPositionRuler *m_pPositionRuler;

		Button *m_pUpBtn;
		Button *m_pDownBtn;
		Button *m_pClearPatternSeqBtn;
		ToggleButton *m_pPointerActionBtn;
		ToggleButton *m_pModeActionBtn;
		ToggleButton *m_pDrawActionBtn;
		ToggleButton *m_pTimeLineToggleBtn;

		QTimer* m_pTimer;


		virtual void resizeEvent( QResizeEvent *ev );
		void resyncExternalScrollBar();
};

#endif
