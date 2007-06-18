/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: SongEditorPanel.h,v 1.19 2005/05/11 20:31:10 comix Exp $
 *
 */

#ifndef SONG_EDITOR_PANEL_H
#define SONG_EDITOR_PANEL_H

#include "SongEditor.h"

#include "gui/EventListener.h"
#include "lib/Object.h"

#include <qscrollview.h>
#include <qwidget.h>
#include <qtimer.h>

class Button;

enum SongEditorActionMode {
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
		SongEditorActionMode getActionMode() {	return m_actionMode;	}

	public slots:
		void contentsMove( int x, int y );
		void newPatBtnClicked( Button* );
		void upBtnClicked( Button* );
		void downBtnClicked( Button* );
		void clearSequence( Button* );
		void updatePlayHeadPosition();

		void pointerActionBtnPressed( Button* pBtn );
		void drawActionBtnPressed( Button* pBtn );

	private:
		SongEditorActionMode m_actionMode;

		uint m_nInitialWidth;
		uint m_nInitialHeight;

		static const int m_nPatternListWidth = 200;

		QScrollView* m_pEditorScrollView;
		QScrollView* m_pPatternListScrollView;
		QScrollView* m_pPositionRulerScrollView;

		SongEditor *m_pSongEditor;
		SongEditorPatternList *m_pPatternList;
		SongEditorPositionRuler *m_pPositionRuler;

		Button *m_pUpBtn;
		Button *m_pDownBtn;
		Button *m_pClearPatternSeqBtn;
		ToggleButton *m_pPointerActionBtn;
		ToggleButton *m_pDrawActionBtn;

		QTimer* m_pTimer;

		// Implements EventListener interface
		virtual void patternChangedEvent();
		//~ Implements EventListener interface

		void resizeEvent ( QResizeEvent *ev );
};

#endif
