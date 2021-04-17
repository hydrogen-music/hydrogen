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

#include <core/Object.h>

#include "PianoRollEditor.h"
#include "../EventListener.h"
#include "../Widgets/LCDCombo.h"
#include "../Widgets/LCD.h"

class Button;
class ToggleButton;
class Fader;
class PatternEditorRuler;
class PatternEditorInstrumentList;
class NotePropertiesRuler;
class LCDCombo;
class DrumPatternEditor;
class PianoRollEditor;


enum patternEditorRightClickMode { VELOCITY_SELECTED, PAN_SELECTED, LEAD_LAG_SELECTED };

namespace H2Core
{
	class Pattern;
}

///
/// Pattern Editor Panel
///
class PatternEditorPanel : public QWidget, public EventListener, public H2Core::Object
{
	H2_OBJECT
	Q_OBJECT

	public:
		explicit PatternEditorPanel(QWidget *parent);
		~PatternEditorPanel();

		DrumPatternEditor* getDrumPatternEditor() {	return m_pDrumPatternEditor;	}
		NotePropertiesRuler* getVelocityEditor() {	return m_pNoteVelocityEditor;	}
		NotePropertiesRuler* getPanEditor() {	return m_pNotePanEditor;	}
		NotePropertiesRuler* getLeadLagEditor() {	return m_pNoteLeadLagEditor;	}
		NotePropertiesRuler* getNoteKeyEditor() {	return m_pNoteNoteKeyEditor;	}
		NotePropertiesRuler* getProbabilityEditor() {	return m_pNoteProbabilityEditor;	}
		PatternEditorInstrumentList* getInstrumentList() {	return m_pInstrumentList;	}
		PianoRollEditor* getPianoRollEditor() {		return m_pPianoRollEditor;	}
		int getPropertiesComboValue(){ return __pPropertiesCombo->selected(); }

		void updateSLnameLabel();
		void updatePianorollEditor();

		// Implements EventListener interface
		virtual void selectedPatternChangedEvent() override;
		virtual void selectedInstrumentChangedEvent() override;
		//~ Implements EventListener interface

		void ensureCursorVisible();
		int getCursorIndexPosition();
		int getCursorPosition(); // TODO use this in many lines rather than the explicit expression? make inline
		float getCursorFloatPosition(); // TODO use this in many lines rather than the explicit expression? make inline
		void setCursorIndexPosition( int nGridIndex );
		void setCursorPosition(int nColumn ); //TODO deprecate and use next
		// used to update the cursor when changing resolution
		void setCursorPosition(float fColumn ); //TODO rename setCursorFloatTickPosition. 
		int moveCursorLeft( int n = 1 );
		int moveCursorRight( int n = 1 );

		void selectInstrumentNotes( int nInstrument );

		void updateEditors( bool bPatternOnly = false );
		//! Granularity of grid positioning ( = distance between grid marks), in tick units
		float granularity() const { // float for tuplets
			return (float) MAX_NOTES * m_nTupletDenominator / ( m_nTupletNumerator * m_nResolution );
		}
		int getTupletNumerator(){ return m_nTupletNumerator; }
		int getTupletDenominator(){ return m_nTupletDenominator; }
		int getResolution(){ return m_nResolution; }

	private slots:
		void gridResolutionChanged( int nSelected );
		void propertiesComboChanged( int nSelected );
		void patternLengthChanged();
		void updatePatternSizeLCD();
		void patternSizeLCDClicked();
		void denominatorWarningClicked();
		void tupletLCDClicked();
		void setResolutionToAllEditors( int nResolution );
		void setTupletRatioToAllEditors( int nTupletNum, int nTupletDen );


		void hearNotesBtnClick(Button *ref);
		void quantizeEventsBtnClick(Button *ref);

		void showDrumEditorBtnClick(Button *ref);

		void syncToExternalHorizontalScrollbar(int);
		void contentsMoving(int dummy);
		void on_patternEditorVScroll(int);
		void on_patternEditorHScroll(int);


		void zoomInBtnClicked(Button *ref);
		void zoomOutBtnClicked(Button *ref);

		void moveDownBtnClicked(Button *);
		void moveUpBtnClicked(Button *);

	private:
		H2Core::Pattern *	m_pPattern;
		QPixmap				m_backgroundPixmap;
		QLabel *			pSLlabel;

		// Editor top
		LCDDisplay *		__pattern_size_LCD;
		Button *			m_pDenominatorWarning;
		LCDCombo *			__resolution_combo;
		LCDDisplay *		m_pTupletLCD;
		ToggleButton *		__show_drum_btn;
		ToggleButton *		__show_piano_btn;
		// ~Editor top

		//note properties combo
		LCDCombo *			__pPropertiesCombo;

		// drum editor
		QScrollArea*		m_pEditorScrollView;
		DrumPatternEditor *	m_pDrumPatternEditor;

		// piano roll editor
		QScrollArea*		m_pPianoRollScrollView;
		PianoRollEditor *	m_pPianoRollEditor;

		// ruler
		QScrollArea*		m_pRulerScrollView;
		PatternEditorRuler *m_pPatternEditorRuler;

		// instr list
		QScrollArea*		m_pInstrListScrollView;
		PatternEditorInstrumentList  *m_pInstrumentList;

		// note velocity editor
		QScrollArea*		m_pNoteVelocityScrollView;
		NotePropertiesRuler *m_pNoteVelocityEditor;

		// note pan editor
		QScrollArea*		m_pNotePanScrollView;
		NotePropertiesRuler *m_pNotePanEditor;

		// note leadlag editor
		QScrollArea*		m_pNoteLeadLagScrollView;
		NotePropertiesRuler *m_pNoteLeadLagEditor;

		// note notekey editor
		QScrollArea*		m_pNoteNoteKeyScrollView;
		NotePropertiesRuler *m_pNoteNoteKeyEditor;

		// note probability editor
		QScrollArea *       m_pNoteProbabilityScrollView;
		NotePropertiesRuler *m_pNoteProbabilityEditor;

		QScrollBar *		m_pPatternEditorHScrollBar;
		QScrollBar *		m_pPatternEditorVScrollBar;

		// TOOLBAR
		QLabel *			m_pPatternNameLbl;
		Button *			m_pRandomVelocityBtn;
		//~ TOOLBAR

		Button *			sizeDropdownBtn;
		Button *			resDropdownBtn;

		bool				m_bEnablePatternResize;
		
		
		//TODO should these 3 members be here or only in preferences?
		uint m_nResolution;
		int	m_nTupletNumerator;
		int	m_nTupletDenominator;

		/* Cursor positioning
		* it refers to the current grid granularity (which depends on resolution and tuplet ratio) */
		int					m_nCursorIndexPosition;
		//~ Cursor

		virtual void dragEnterEvent(QDragEnterEvent *event) override;
		virtual void dropEvent(QDropEvent *event) override;

		virtual void resizeEvent(QResizeEvent *ev) override;
		virtual void showEvent(QShowEvent *ev) override;
};




#endif
