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

#include <vector>

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
class PatternEditorSidebar;
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

	explicit DrumPatternRow() noexcept;
	explicit DrumPatternRow( int nId, const QString& sType,
							 bool bAlternate ) noexcept;

	/** Associated #H2Core::Instrument::__id in the current #H2Core::Drumkit.
	 *
	 * If set to #EMPTY_INSTR_ID, the row does not correspond to any. This
	 * happens in case #H2Core::Note were created with a different
	 * #H2Core::Drumkit using an instrument type not present in the current one.
	 * The note can not be played back with the current kit but can be
	 * copy/pasted etc. like a regular one.
	 *
	 * Single source of truth for the current instrument ID is
	 * #H2Core::Hydrogen::m_nSelectedInstrumentNumber.
	 * #PatternEditorPanel::m_nSelectedRowDB in combination with
	 * #PatternEditorPanel::m_db serves only as a copy. (This is done as the
	 * current instrument is important to the core itself when rendering
	 * incoming MIDI notes.)
	 *
	 * Null element: #EMPTY_INSTR_ID */
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

	/** Odd number rows will be painted in an alternate color to make the
	 * visually distinct. */
	bool bAlternate;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const;
};

namespace H2Core
{
	class Note;
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
		NotePropertiesRuler* getKeyOctaveEditor() {	return m_pNoteKeyOctaveEditor;	}
		NotePropertiesRuler* getProbabilityEditor() {	return m_pNoteProbabilityEditor;	}
		PatternEditorSidebar* getSidebar() {	return m_pSidebar;	}
		PianoRollEditor* getPianoRollEditor() {		return m_pPianoRollEditor;	}
		PatternEditorRuler* getPatternEditorRuler() {		return m_pPatternEditorRuler;  }
		const QScrollArea* getDrumPatternEditorScrollArea() const { return m_pEditorScrollView; }
		const QScrollArea* getPianoRollEditorScrollArea() const { return m_pPianoRollScrollView; }
		const QScrollArea* getNoteVelocityScrollArea() const { return m_pNoteVelocityScrollView; }
		const QScrollArea* getNotePanScrollArea() const { return m_pNotePanScrollView; }
		const QScrollArea* getNoteLeadLagScrollArea() const { return m_pNoteLeadLagScrollView; }
		const QScrollArea* getNoteKeyOctaveScrollArea() const { return m_pNoteKeyOctaveScrollView; }
		const QScrollArea* getNoteProbabilityScrollArea() const { return m_pNoteProbabilityScrollView; }
		const QScrollBar* getVerticalScrollBar() const { return m_pPatternEditorVScrollBar; }
		const QScrollBar* getHorizontalScrollBar() const { return m_pPatternEditorHScrollBar; }
	NotePropertiesRuler::Mode getNotePropertiesMode() const;

		/** Allow to create other parts of pattern editor _after_ this class is
		 * fully initialized in order used it as the single source of truth. */
		void createEditors();

		void updateDrumkitLabel();

		virtual void resizeEvent(QResizeEvent *ev) override;

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
	virtual void stateChangedEvent( const H2Core::AudioEngine::State& ) override;
	virtual void relocationEvent() override;
		// ~ Implements EventListener interface

		std::shared_ptr<H2Core::Pattern> getPattern() const;
		int getPatternNumber() const;

		const std::vector<DrumPatternRow>& getDB() const;
		/** Prints the content of #m_db as a debug level log message for
		 * debugging purposes. */
		void printDB() const;
		/** Instead of using the exception-based range check of std::vector - a
		 * paradigm not used in the code base - we return null elements for
		 * instrument id and type - an empty row - in case @a nRow does not
		 * exist. */
		const DrumPatternRow getRowDB( int nRow ) const;
		int getSelectedRowDB() const;
		void setSelectedRowDB( int nNewRow );
		/** @return the vertical position of the row in the DB.
		 *
		 * This is especially helpful for order-based instrument operations. */
		int getRowIndexDB( const DrumPatternRow& row );
		/** Retrieves the row number @a pNote is located in. */
		int findRowDB( H2Core::Note* pNote, bool bSilent = false ) const;
		int getRowNumberDB() const;
		/** Returns the instrument corresponding to the currently selected row
		 * of the DB.
		 *
		 * In case this row is not associated with an instrument in the current
		 * drumkit - only instrument type set - or no row was selected at all, a
		 * nullptr is returned. */
		std::shared_ptr<H2Core::Instrument> getSelectedInstrument() const;

		void ensureCursorVisible();
		/** The row of the particular editor is maintained by the editor itself
		 * and can be accessed via #PatternEditor::getCursorPosition. */
		int getCursorColumn();
		void setCursorColumn( int nCursorColumn, bool bUpdateEditors = true );
		void moveCursorLeft( QKeyEvent* pEvent, int n = 1 );
		void moveCursorRight( QKeyEvent* pEvent, int n = 1 );

		void updateEditors( bool bPatternOnly = false );

	void patternSizeChangedAction( int nLength, double fDenominator,
								   int nSelectedPatternNumber );

		/** Returns either #DrumPatternEditor or #PianoRollEditor, depending on
		 * which of them is currently visible. */
		PatternEditor* getVisibleEditor() const;
		NotePropertiesRuler* getVisiblePropertiesRuler() const;
		/** Get notes to show in pattern editor. This may include "background"
		 * notes that are in currently-playing patterns rather than the current
		 * pattern. */
		std::vector<std::shared_ptr<H2Core::Pattern>> getPatternsToShow() const;

		////////////////////////////////////////////////////////////////////////
		////////// Top-level actions for the overall pattern editor. ///////////

		/** Remove all notes from the provided @a nRow of the
		  * #DrumPatternEditor.
		  *
		  * Note that removing all notes of rows in the #PianoRollEditor is not
		  * supported.
		  *
		  * @param nRow for which to remove all notes
		  * @param nPattern If set to `-1`, all notes of row @a nRow in the
		  *   all patterns of the current song will be removed. */
		void clearNotesInRow( int nRow, int nPattern = -1 );

		enum class FillNotes {
			All = 1,
			EverySecond = 2,
			EveryThird = 3,
			EveryFourth = 4,
			EverySixth = 6,
			EveryEighth = 8,
			EveryTwelfth = 12,
			EverySixteenth = 16
		};
		static QString FillNotesToQString( const FillNotes& fillNotes );

		/** Add every @a every note to row @a nRow.
		  *
		  * Note that filling notes is only supported for rows of the
		  * #DrumPatternEditor, not the #PianoRollEditor. */
		void fillNotesInRow( int nRow, FillNotes every );
		/** Serialized all notes in @a nRow for all patterns in the current song
		 * and stores the resulting string to the clipboard. */
		void copyNotesFromRowOfAllPatterns( int nRow );
		/** Same as copyNotesFromRowOfAllPatterns() but also removes all notes
		 * in row @a nRow. */
		void cutNotesFromRowOfAllPatterns( int nRow );
		/** Reads the serialized note list created by
		 * copyNotesFromRowOfAllPatterns() from clipboard and adds them to row
		 * @a nRow. */
		void pasteNotesToRowOfAllPatterns( int nRow );

		int getResolution() const;
		bool isUsingTriplets() const;

		/** Update #m_db based on #H2Core::Song::m_pDrumkit and #m_pPattern. */
		void updateDB();

		const std::map<std::shared_ptr<H2Core::Pattern>,
			std::vector<H2Core::Note*>>& getHoveredNotes() const;
		void setHoveredNotes( std::map<std::shared_ptr<H2Core::Pattern>,
							  std::vector<H2Core::Note*>> hoveredNotes );

	public slots:
		void showDrumEditor();
		void showPianoRollEditor();
		void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

	private slots:
		void gridResolutionChanged( int nSelected );
		void propertiesComboChanged( int nSelected );

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

		/** Currently selected pattern cached in frontend for convenience.*/
		std::shared_ptr<H2Core::Pattern>	m_pPattern;
		/** Number corresponding to #m_pPattern. */
		int m_nPatternNumber;

		/** Single source of truth for which #H2Core::Note to display (in which
		 * row) for all parts of the pattern editor.*/
		std::vector<DrumPatternRow> m_db;
		/** Currently activate row of #m_db.
		 *
		 * `-1` indicates no row is selected/available. */
		int m_nSelectedRowDB;

		/** Uses the index of a tab in #m_pTabBar as key and the index of the
		 * pattern associated as value.*/
		std::map<int, int> m_tabPatternMap;

		QPixmap				m_backgroundPixmap;
		ClickableLabel*		m_pDrumkitLabel;

		QTabBar* m_pTabBar;
		QWidget* m_pToolBar;
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
		QScrollArea*		m_pSidebarScrollView;
		PatternEditorSidebar  *m_pSidebar;

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
		QScrollArea*		m_pNoteKeyOctaveScrollView;
		NotePropertiesRuler *m_pNoteKeyOctaveEditor;

		// note probability editor
		QScrollArea *       m_pNoteProbabilityScrollView;
		NotePropertiesRuler *m_pNoteProbabilityEditor;

		QScrollBar *		m_pPatternEditorHScrollBar;
		QScrollBar *		m_pPatternEditorVScrollBar;
		QWidget*			m_pPatternEditorHScrollBarContainer;

		Button *			sizeDropdownBtn;
		Button *			resDropdownBtn;

		bool				m_bEnablePatternResize;

		// Cursor positioning
		int					m_nCursorColumn;
		int					m_nCursorIncrement;
		// ~ Cursor

		int m_nResolution;
		bool m_bIsUsingTriplets;

		bool m_bPatternSelectedViaTab;

		std::map<std::shared_ptr<H2Core::Pattern>,
			std::vector<H2Core::Note*>> m_hoveredNotes;

		virtual void dragEnterEvent(QDragEnterEvent *event) override;
		virtual void dropEvent(QDropEvent *event) override;

		virtual void showEvent(QShowEvent *ev) override;
};

inline std::shared_ptr<H2Core::Pattern> PatternEditorPanel::getPattern() const {
	return m_pPattern;
}
inline int PatternEditorPanel::getPatternNumber() const {
	return m_nPatternNumber;
}
inline const std::vector<DrumPatternRow>& PatternEditorPanel::getDB() const {
	return m_db;
}
inline int PatternEditorPanel::getSelectedRowDB() const {
	return m_nSelectedRowDB;
}
inline int PatternEditorPanel::getResolution() const {
	return m_nResolution;
}
inline bool PatternEditorPanel::isUsingTriplets() const {
	return m_bIsUsingTriplets;
}
inline const std::map<std::shared_ptr<H2Core::Pattern>,
	std::vector<H2Core::Note*>>& PatternEditorPanel::getHoveredNotes() const {
	return m_hoveredNotes;
}

#endif
