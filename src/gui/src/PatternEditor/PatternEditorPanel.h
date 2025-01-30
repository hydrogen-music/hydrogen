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


#ifndef PATTERN_EDITOR_PANEL_H
#define PATTERN_EDITOR_PANEL_H

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/AudioEngine/AudioEngine.h>

#include "PianoRollEditor.h"
#include "../EventListener.h"
#include "../Widgets/LCDCombo.h"
#include "../Widgets/WidgetWithScalableFont.h"

class Button;
class Fader;
class PatternEditorRuler;
class PatternEditorInstrumentList;
class NotePropertiesRuler;
class DrumPatternEditor;
class PianoRollEditor;
class ClickableLabel;
class LCDSpinBox;

enum patternEditorRightClickMode { VELOCITY_SELECTED, PAN_SELECTED, LEAD_LAG_SELECTED };

namespace H2Core
{
	class Pattern;
}

///
/// Pattern Editor Panel
///
/** \ingroup docGUI*/
class PatternEditorPanel :  public QWidget, protected WidgetWithScalableFont<8, 10, 12>, public EventListener,  public H2Core::Object<PatternEditorPanel>
{
	H2_OBJECT(PatternEditorPanel)
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
		PatternEditorRuler* getPatternEditorRuler() {		return m_pPatternEditorRuler;  }
		const QScrollArea* getDrumPatternEditorScrollArea() const { return m_pEditorScrollView; }
		const QScrollArea* getPianoRollEditorScrollArea() const { return m_pPianoRollScrollView; }
		const QScrollArea* getNoteVelocityScrollArea() const { return m_pNoteVelocityScrollView; }
		const QScrollArea* getNotePanScrollArea() const { return m_pNotePanScrollView; }
		const QScrollArea* getNoteLeadLagScrollArea() const { return m_pNoteLeadLagScrollView; }
		const QScrollArea* getNoteNoteKeyScrollArea() const { return m_pNoteNoteKeyScrollView; }
		const QScrollArea* getNoteProbabilityScrollArea() const { return m_pNoteProbabilityScrollView; }
		const QScrollBar* getVerticalScrollBar() const { return m_pPatternEditorVScrollBar; }
		const QScrollBar* getHorizontalScrollBar() const { return m_pPatternEditorHScrollBar; }
	NotePropertiesRuler::Mode getNotePropertiesMode() const;
	

		void updateSLnameLabel();

		// Implements EventListener interface
		virtual void selectedPatternChangedEvent() override;
		virtual void selectedInstrumentChangedEvent() override;
	virtual void patternModifiedEvent() override;
	virtual void playingPatternsChangedEvent() override;
	virtual void drumkitLoadedEvent() override;
	virtual void updateSongEvent( int nValue ) override;
	virtual void songModeActivationEvent() override;
	virtual void stackedModeActivationEvent( int ) override;
	virtual void songSizeChangedEvent() override;
	virtual void patternEditorLockedEvent() override;
	virtual void relocationEvent() override;
		// ~ Implements EventListener interface

		void ensureCursorVisible();
		int getCursorPosition();
		void setCursorPosition(int nCursorPosition);
		int moveCursorLeft( int n = 1 );
		int moveCursorRight( int n = 1 );

		void selectInstrumentNotes( int nInstrument );

		void updatePatternInfo();

		void updateEditors( bool bPatternOnly = false );

	void patternSizeChangedAction( int nLength, double fDenominator,
								   int nSelectedPatternNumber );

	public slots:
		void showDrumEditor();
		void showPianoRollEditor();
		void onPreferencesChanged( H2Core::Preferences::Changes changes );

	private slots:
		void gridResolutionChanged( int nSelected );
		void propertiesComboChanged( int nSelected );
	/** Batch version for setting the values of the pattern size spin boxes.*/
		void updatePatternSizeLCD();

		void hearNotesBtnClick();
		void quantizeEventsBtnClick();

		void showDrumEditorBtnClick();

		void syncToExternalHorizontalScrollbar(int);
		void contentsMoving(int dummy);
		void on_patternEditorVScroll(int);
		void on_patternEditorHScroll(int);


		void zoomInBtnClicked();
		void zoomOutBtnClicked();

	void patternSizeChanged( double );
	void switchPatternSizeFocus();

	private:
	void updateStyleSheet();
	
		H2Core::Pattern *	m_pPattern;
	int m_nSelectedPatternNumber;
		QPixmap				m_backgroundPixmap;
		QLabel *			m_pSLlabel;

	QWidget* m_pEditorTop1;
	QWidget* m_pEditorTop2;
	QWidget* m_pSizeResol;
	QWidget* m_pRec;

	LCDSpinBox* m_pLCDSpinBoxNumerator;
	LCDSpinBox* m_pLCDSpinBoxDenominator;
	/** Indicates whether the LCD spin boxes for the pattern size have
		been altered by Hydrogen or by the user.*/
	bool m_bArmPatternSizeSpinBoxes;

		// Editor top
		LCDCombo *			m_pResolutionCombo;
		Button *		__show_drum_btn;
		Button *		__show_piano_btn;
	Button *		m_pHearNotesBtn;
	Button *		m_pQuantizeEventsBtn;
	
		ClickableLabel*		m_pPatternSizeLbl;
		ClickableLabel*		m_pResolutionLbl;
		ClickableLabel*		m_pHearNotesLbl;
		ClickableLabel*		m_pQuantizeEventsLbl;
		ClickableLabel*		m_pShowPianoLbl;
		// ~Editor top

		//note properties combo
		LCDCombo *			m_pPropertiesCombo;

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
		// ~ TOOLBAR

		Button *			sizeDropdownBtn;
		Button *			resDropdownBtn;

		bool				m_bEnablePatternResize;

		// Cursor positioning
		int					m_nCursorPosition;
		int					m_nCursorIncrement;
		// ~ Cursor

		virtual void dragEnterEvent(QDragEnterEvent *event) override;
		virtual void dropEvent(QDropEvent *event) override;

		virtual void resizeEvent(QResizeEvent *ev) override;
		virtual void showEvent(QShowEvent *ev) override;
};




#endif
