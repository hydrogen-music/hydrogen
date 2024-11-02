/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
class PixmapWidget;

enum patternEditorRightClickMode { VELOCITY_SELECTED, PAN_SELECTED, LEAD_LAG_SELECTED };

/** Properties of a single row in #DrumPatternEditor.
 *
 * It is used to correlate an instrument ID and instrument type with a row in
 * the editor. Since either of them can be absent, this struct helps to all
 * widgets in the pattern editor part to decided which #H2Core::Note to render
 * based on the currently selected row of #DrumPatternEditor.
 *
 * In addition, it also carries state parameters for the actual row in
 * #DrumPatternEditor. */
struct DrumPatternRow {
	/** Associated #H2Core::Instrument::__id in the current #H2Core::Drumkit.
	 *
	 * If set to `-1`, the row does not correspond to any. This happens in case
	 * #H2Core::Note were created with a different #H2Core::Drumkit using an
	 * instrument type not present in the current one. The note can not be
	 * played back with the current kit but can be copy/pasted etc. like a
	 * regular one.
	 *
	 * Single source of truth for the current instrument ID is
	 * #H2Core::Hydrogen::m_nSelectedInstrumentNumber.
	 * #PatternEditorPanel::m_nSelectedRowDB in combination with
	 * #PatternEditorPanel::m_db serves only as a copy. (This is done as the
	 * current instrument is important to the core itself when rendering
	 * incoming MIDI notes.)
	 *
	 * Null element: -1 */
	int nInstrumentID;
	/** Associated #H2Core::DrumkitMap::Type.
	 *
	 * If set to an empty string, the row does not correspond to any. This
	 * happens in case #H2Core::Note were created for an #H2Core::Instrument not
	 * associated with a type yet, e.g. an import of a custom legacy (pre 2.0)
	 * kit.
	 *
	 * Single source of truth for the current instrument type is
	 * #PatternEditorPanel::m_nSelectedRowDB in combination with
	 * #PatternEditorPanel::m_db.
	 *
	 * Null element: "" (empty string) */
	QString sType;
};

namespace H2Core
{
	class Pattern;
}

/** Central widget bundling all individual parts of the pattern editor, serving
 * as the single source of truth for their most important properties, and
 * provided a number of buttons in a panel. */
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

		/** Allow to create other parts of pattern editor _after_ this class is
		 * fully initialized in order used it as the single source of truth. */
		void createEditors();

		void updateDrumkitLabel();

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

		H2Core::Pattern* getPattern() const;

		const std::map<int, DrumPatternRow>& getDB() const;
		int getSelectedRowDB() const;
		void setSelectedRowDB( int nNewRow );

		void ensureCursorVisible();
		int getCursorPosition();
		void setCursorPosition(int nCursorPosition);
		int moveCursorLeft( int n = 1 );
		int moveCursorRight( int n = 1 );

		void selectInstrumentNotes( int nInstrument );

		void updateEditors( bool bPatternOnly = false );

	void patternSizeChangedAction( int nLength, double fDenominator,
								   int nSelectedPatternNumber );

	public slots:
		void showDrumEditor();
		void showPianoRollEditor();
		void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

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

		void patchBayBtnClicked();

	private:

		void updatePatternInfo();
		void updateStyleSheet();
		void updatePatternName();
		/** Update #m_db based on #H2Core::Song::m_pDrumkit and #m_pPattern. */
		void updateDB();
		/** Prints the content of #m_db as a debug level log message for
		 * debugging purposes. */
		void printDB();
	
		H2Core::Pattern*	m_pPattern;

		/** Single source of truth for which #H2Core::Note to display (in which
		 * row) for all parts of the pattern editor.*/
		std::map<int, DrumPatternRow> m_db;
		/** Currently activate row of #m_db.
		 *
		 * `-1` indicates no row is selected/available. */
		int m_nSelectedRowDB;

		QPixmap				m_backgroundPixmap;
		ClickableLabel*		m_pDrumkitLabel;

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
		Button* m_pPatchBayBtn;
	
		ClickableLabel*		m_pPatternSizeLbl;
		ClickableLabel*		m_pResolutionLbl;
		ClickableLabel*		m_pHearNotesLbl;
		ClickableLabel*		m_pQuantizeEventsLbl;
		ClickableLabel*		m_pShowPianoLbl;
		// ~Editor top

		//note properties combo
		LCDCombo *			m_pPropertiesCombo;
		PixmapWidget*		m_pPropertiesPanel;

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
		QWidget*			m_pPatternEditorHScrollBarContainer;

		// TOOLBAR
		ClickableLabel*			m_pPatternNameLbl;
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

inline H2Core::Pattern* PatternEditorPanel::getPattern() const {
	return m_pPattern;
}
inline const std::map<int, DrumPatternRow>& PatternEditorPanel::getDB() const {
	return m_db;
}
inline int PatternEditorPanel::getSelectedRowDB() const {
	return m_nSelectedRowDB;
}

#endif
