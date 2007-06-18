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
 * $Id: PatternEditorPanel.h,v 1.15 2005/05/09 18:11:55 comix Exp $
 *
 */


#ifndef PATTERN_EDITOR_PANEL_H
#define PATTERN_EDITOR_PANEL_H

#include <map>
using namespace std;

#include "lib/Object.h"

#include <qwidget.h>
#include <qpixmap.h>
#include <qcombobox.h>
#include <qmessagebox.h>
#include <qtooltip.h>
#include <qscrollview.h>
#include <qscrollbar.h>
#include <qframe.h>
#include <qspinbox.h>
#include <qlabel.h>

#include "PatternEditor.h"
#include "gui/EventListener.h"
#include "gui/widgets/LCD.h"

class Button;
class ToggleButton;
class Fader;

class PatternEditor;
class PatternEditorRuler;
class PatternEditorInstrumentList;
class NotePropertiesRuler;



///
/// Pattern Editor Panel
///
class PatternEditorPanel : public QWidget, public EventListener, public Object
{
	Q_OBJECT

	public:
		PatternEditorPanel(QWidget *parent);
		~PatternEditorPanel();
		void updateStart(bool start);

		void resizeEvent ( QResizeEvent *ev );
		void showEvent ( QShowEvent *ev );

		PatternEditor* getPatternEditor() {	return m_pPatternEditor;	}
		NotePropertiesRuler* getVelocityEditor() {	return m_pNoteVelocityEditor;	}
		NotePropertiesRuler* getPitchEditor() {	return m_pNotePitchEditor;	}
		PatternEditorInstrumentList* getInstrumentList() {	return m_pInstrumentList;	}

		void setSelectedInstrument( int nInstr );

		Pattern* getPattern() {	return m_pPattern;	}

		// Implements EventListener interface
		virtual void patternModifiedEvent();
		virtual void selectedPatternChangedEvent();
		virtual void selectedInstrumentChangedEvent();
		virtual void stateChangedEvent(int nState);
		//~ Implements EventListener interface

	public slots:
		void sizeDropdownBtnClicked(Button *ref);
		void resDropdownBtnClicked(Button *ref);

		void gridResolutionChanged( int nSelected );
		void patternSizeChanged( int nSelected );

		void hearNotesBtnClick(Button *ref);
		void recordEventsBtnClick(Button *ref);
		void quantizeEventsBtnClick(Button *ref);

		void showVelocityBtnClick(Button *ref);
		void showPitchBtnClick(Button *ref);

		void syncToExternalHorizontalScrollbar(int);
		void contentsMoving(int dummy);

		void zoomInBtnClicked(Button *ref);
		void zoomOutBtnClicked(Button *ref);

		void nextPatternBtnClicked(Button*);
		void prevPatternBtnClicked(Button*);

		void moveDownBtnClicked(Button *);
		void moveUpBtnClicked(Button *);

	private:
		enum NotePropertiesMode {
			VELOCITY,
			PITCH
		};
		NotePropertiesMode m_notePropertiesMode;

		Pattern *m_pPattern;
		QPixmap m_backgroundPixmap;

		// editor
		QScrollView* m_pEditorScrollView;
		PatternEditor *m_pPatternEditor;

		// ruler
		QScrollView* m_pRulerScrollView;
		PatternEditorRuler *m_pPatternEditorRuler;

		// instr list
		QScrollView* m_pInstrListScrollView;
		PatternEditorInstrumentList  *m_pInstrumentList;

		// note velocity editor
		QScrollView* m_pNoteVelocityScrollView;
		NotePropertiesRuler *m_pNoteVelocityEditor;

		// note Pitch editor
		QScrollView* m_pNotePitchScrollView;
		NotePropertiesRuler *m_pNotePitchEditor;


		QScrollBar *m_pPatternEditorHScrollBar;
		QScrollBar *m_pPatternEditorVScrollBar;

		// TOOLBAR
		LCDDisplay *m_pPatternNumberLCD;
		LCDDisplay *m_pNameLCD;
		Button *m_pNextPatternBtn;
		Button *m_pPrevPatternBtn;

		LCDDisplay *m_pResolutionLCD;
		QPopupMenu *m_pResolutionPopup;

		LCDDisplay *m_pPatternSizeLCD;
		QPopupMenu *m_pPatternSizePopup;

		Button *m_pMoveUpBtn;
		Button *m_pMoveDownBtn;

		Button *m_pRandomVelocityBtn;
		//~ TOOLBAR


		Button *sizeDropdownBtn;
		Button *resDropdownBtn;

		ToggleButton *hearNotesBtn;
		ToggleButton *recordEventsBtn;
		ToggleButton *quantizeEventsBtn;

		ToggleButton *m_pShowVelocityBtn;
		ToggleButton *m_pShowPitchBtn;

		Button *m_pZoomInBtn;
		Button *m_pZoomOutBtn;

		bool m_bEnablePatternResize;


		void setupPattern();
};



#endif


