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


#ifndef PATTERN_EDITOR_PANEL_H
#define PATTERN_EDITOR_PANEL_H

#include <QtGui>

#include <hydrogen/Object.h>

#include "PianoRollEditor.h"
#include "../EventListener.h"

class Button;
class ToggleButton;
class Fader;
class PatternEditorRuler;
class PatternEditorInstrumentList;
class NotePropertiesRuler;
class LCDCombo;
class DrumPatternEditor;
class PianoRollEditor;


namespace H2Core
{
	class Pattern;
}

///
/// Pattern Editor Panel
///
class PatternEditorPanel : public QWidget, public EventListener, public Object
{
	Q_OBJECT

	public:
		PatternEditorPanel(QWidget *parent);
		~PatternEditorPanel();

		DrumPatternEditor* getDrumPatternEditor() {	return m_pDrumPatternEditor;	}
		NotePropertiesRuler* getVelocityEditor() {	return m_pNoteVelocityEditor;	}
		NotePropertiesRuler* getPanEditor() {	return m_pNotePanEditor;	}
		NotePropertiesRuler* getLeadLagEditor() {	return m_pNoteLeadLagEditor;	}
		NotePropertiesRuler* getNoteKeyEditor() {	return m_pNoteNoteKeyEditor;	}
		PatternEditorInstrumentList* getInstrumentList() {	return m_pInstrumentList;	}
		PianoRollEditor* getPianoRollEditor() {		return m_pPianoRollEditor;	}

		void updateSLnameLabel();
		void displayorHidePrePostCB();
		void updatePianorollEditor();

		// Implements EventListener interface
		virtual void selectedPatternChangedEvent();
		virtual void selectedInstrumentChangedEvent();
		virtual void stateChangedEvent(int nState);
		//~ Implements EventListener interface

	private slots:
		void gridResolutionChanged( QString text );
		void propertiesComboChanged( QString text );
		void patternSizeChanged( QString text );

		void hearNotesBtnClick(Button *ref);
		void quantizeEventsBtnClick(Button *ref);

		void showDrumEditorBtnClick(Button *ref);

		void syncToExternalHorizontalScrollbar(int);
		void contentsMoving(int dummy);
		void on_patternEditorScroll(int);


		void zoomInBtnClicked(Button *ref);
		void zoomOutBtnClicked(Button *ref);

		void moveDownBtnClicked(Button *);
		void moveUpBtnClicked(Button *);

		void playselectedinstrument( QString text );
		void rightclickSelect( QString text );
		void recPreDeleteSelect( int index );
		void recPostDeleteSelect( int index );

	private:
		H2Core::Pattern *m_pPattern;
		QPixmap m_backgroundPixmap;
		QLabel *pSLlabel;

		// Editor top
		LCDCombo *__pattern_size_combo;
		LCDCombo *__resolution_combo;
		ToggleButton *__show_drum_btn;
		ToggleButton *__show_piano_btn;
		QComboBox *__recpredelete;
		QComboBox *__recpostdelete;


		// ~Editor top

		//
		LCDCombo * __pPropertiesCombo;

		// drum editor
		QScrollArea* m_pEditorScrollView;
		DrumPatternEditor *m_pDrumPatternEditor;

		// piano roll editor
		QScrollArea* m_pPianoRollInternScrollView;
		QScrollArea* m_pPianoRollScrollView;
		PianoRollEditor *m_pPianoRollEditor;

		// ruler
		QScrollArea* m_pRulerScrollView;
		PatternEditorRuler *m_pPatternEditorRuler;

		// instr list
		QScrollArea* m_pInstrListScrollView;
		PatternEditorInstrumentList  *m_pInstrumentList;

		// note velocity editor
		QScrollArea* m_pNoteVelocityScrollView;
		NotePropertiesRuler *m_pNoteVelocityEditor;

		// note pan editor
		QScrollArea* m_pNotePanScrollView;
		NotePropertiesRuler *m_pNotePanEditor;

		// note leadlag editor
		QScrollArea* m_pNoteLeadLagScrollView;
		NotePropertiesRuler *m_pNoteLeadLagEditor;

		// note notekey editor
		QScrollArea* m_pNoteNoteKeyScrollView;
		NotePropertiesRuler *m_pNoteNoteKeyEditor;
		


		QScrollBar *m_pPatternEditorHScrollBar;
		QScrollBar *m_pPatternEditorVScrollBar;

		// TOOLBAR
		QLabel *m_pPatternNameLbl;



		Button *m_pRandomVelocityBtn;
		//~ TOOLBAR


		Button *sizeDropdownBtn;
		Button *resDropdownBtn;


		QComboBox* __rightclickSelection;
 
		bool m_bEnablePatternResize;


		virtual void dragEnterEvent(QDragEnterEvent *event);
		virtual void dropEvent(QDropEvent *event);

		virtual void resizeEvent(QResizeEvent *ev);
		virtual void showEvent(QShowEvent *ev);
};



#endif
