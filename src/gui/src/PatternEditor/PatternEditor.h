/*
 * Hydrogen
 * Copyright(c) 2002-2020 by the Hydrogen Team
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

#ifndef PATERN_EDITOR_H
#define PATERN_EDITOR_H

#include "../EventListener.h"
#include "../Selection.h"

#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

namespace H2Core
{
	class Note;
	class Instrument;
}

class PatternEditorPanel;

//! Pattern Editor
//!
//! The PatternEditor class is an abstract base class for
//! functionality common to Pattern Editor components
//! (DrumPatternEditor, PianoRollEditor, NotePropertiesRuler).
//!
//! This covers common elements such as some selection handling,
//! timebase functions, and drawing grid lines.
//!
/** \ingroup docGUI*/
class PatternEditor : public QWidget,
					  public EventListener,
					  public H2Core::Object<PatternEditor>,
					  public SelectionWidget<H2Core::Note *>
{
	H2_OBJECT(PatternEditor)
	Q_OBJECT

public:
	enum class Editor {
		DrumPattern = 0,
		PianoRoll = 1,
		NotePropertiesRuler = 2,
		None = 3
	};
	
	enum class Mode {
		Velocity = 0,
		Pan = 1,
		LeadLag = 2,
		NoteKey = 3,
		Probability = 4,
		None = 5
	};
	static QString modeToQString( const Mode& mode );
	
	PatternEditor( QWidget *pParent );
	~PatternEditor();


	//! Set the editor grid resolution, dividing a whole note into `res` subdivisions. 
	void setResolution( uint res, bool bUseTriplets );
	uint getResolution() const { return m_nResolution; }
	bool isUsingTriplets() const { return m_bUseTriplets;	}

	float getGridWidth() const { return m_fGridWidth; }
	unsigned getGridHeight() const { return m_nGridHeight; }
	//! Zoom in / out on the time axis
	void zoomIn();
	void zoomOut();

	//! Clear the pattern editor selection
	void clearSelection() {
		m_selection.clearSelection();
	}

	//! Calculate colour to use for note representation based on note velocity. 
	static QColor computeNoteColor( float velocity );


	//! Merge together the selection groups of two PatternEditor objects to share a common selection.
	void mergeSelectionGroups( PatternEditor *pPatternEditor ) {
		m_selection.merge( &pPatternEditor->m_selection );
	}

	//! Ensure that the Selection contains only valid elements.
	virtual void validateSelection() override;

	//! Update the status of modifier keys in response to input events.
	virtual void updateModifiers( QInputEvent *ev );

	//! Update a widget in response to a change in selection
	virtual void updateWidget() override {
		updateEditor( true );
	}

	//! Deselecting notes
	virtual bool checkDeselectElements( const std::vector<SelectionIndex>& elements ) override;

	//! Change the mouse cursor during mouse gestures
	virtual void startMouseLasso( QMouseEvent *ev ) override {
		setCursor( Qt::CrossCursor );
	}

	virtual void startMouseMove( QMouseEvent *ev ) override {
		setCursor( Qt::DragMoveCursor );
	}

	virtual void endMouseGesture() override {
		unsetCursor();
	}

	//! Do two notes match exactly, from the pattern editor's point of view?
	//! They match if all user-editable properties are the same.
	bool notesMatchExactly( H2Core::Note *pNoteA, H2Core::Note *pNoteB ) const;

	//! Deselect some notes, and "overwrite" some others.
	void deselectAndOverwriteNotes( const std::vector< H2Core::Note *>& selected,
									const std::vector< H2Core::Note *>& overwritten );

	void undoDeselectAndOverwriteNotes( const std::vector< H2Core::Note *>& selected,
										const std::vector< H2Core::Note *>& overwritten );

	//! Raw Qt mouse events are passed to the Selection
	virtual void mousePressEvent( QMouseEvent *ev ) override;
	virtual void mouseMoveEvent( QMouseEvent *ev ) override;
	virtual void mouseReleaseEvent( QMouseEvent *ev ) override;
	
	virtual void mouseDragStartEvent( QMouseEvent *ev ) override;
	virtual void mouseDragUpdateEvent( QMouseEvent *ev ) override;
	virtual void mouseDragEndEvent( QMouseEvent *ev ) override;

	static constexpr int nMargin = 20;

	/** Caches the AudioEngine::m_nPatternTickPosition in the member
		variable #m_nTick and triggers an update(). */
	void updatePosition( float fTick );
	void editNoteLengthAction( int nColumn,
							   int nRealColumn,
							   int nRow,
							   int nLength,
							   int nSelectedPatternNumber,
							   int nSelectedInstrumentnumber,
							   const Editor& editor );
	
	static void editNotePropertiesAction( const Mode& mode,
										  const Editor& editor,
										  int nPatternNumber,
										  int nColumn,
										  int nRowDB,
										  float fVelocity,
										  float fPan,
										  float fLeadLag,
										  float fProbability,
										  int nNoteKey,
										  int nOctaveKey );
	static void triggerStatusMessage( H2Core::Note* pNote, const Mode& mode );

	/**
	 * Determines whether to pattern editor should show further
	 * patterns (determined by getPattersToShow()) or just the
	 * currently selected one.
	 */
	static bool isUsingAdditionalPatterns( const std::shared_ptr<H2Core::Pattern> pPattern );

protected:

	//! The Selection object.
	Selection< SelectionIndex > m_selection;

public slots:
	virtual void updateEditor( bool bPatternOnly = false ) = 0;
	virtual void selectAll() = 0;
	virtual void selectNone();
	virtual void deleteSelection() = 0;
	virtual void copy();
	virtual void paste() = 0;
	virtual void cut();
	virtual void alignToGrid();
	virtual void randomizeVelocity();
	virtual void selectInstrumentNotes( int nInstrument );
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );
	void scrolled( int nValue );

protected:

	//! Granularity of grid positioning (in ticks)
	int granularity() const {
		int nBase;
		if (m_bUseTriplets) {
			nBase = 3;
		}
		else {
			nBase = 4;
		}
		return 4 * MAX_NOTES / ( nBase * m_nResolution );
	}

	uint m_nEditorHeight;
	uint m_nEditorWidth;

	// width of the editor covered by the current pattern.
	int m_nActiveWidth;

	float m_fGridWidth;
	unsigned m_nGridHeight;

	uint m_nResolution;
	bool m_bUseTriplets;
	bool m_bFineGrained;
	bool m_bCopyNotMove;

	bool m_bSelectNewNotes;
	H2Core::Note *m_pDraggedNote;
	
	PatternEditorPanel* m_pPatternEditorPanel;
	QMenu *m_pPopupMenu;

	QList< QAction * > m_selectionActions;

	void showPopupMenu( const QPoint & pos );

	int getColumn( int x, bool bUseFineGrained = false ) const;
		/** Function in the same vein as getColumn() but calculates both column
		 * and row information from the provided event position. */
		void mouseEventToColumnRow( QMouseEvent* pEvent, int* pColumn,
									int* pRow, int* pRealColumn = nullptr,
									bool bUseFineGrained = false ) const;
	QPoint movingGridOffset() const;

	//! Draw lines for note grid.
	void drawGridLines( QPainter &p, const Qt::PenStyle& style = Qt::SolidLine ) const;

	//! Colour to use for outlining selected notes
	QColor selectedNoteColor() const;

	/**
	 * Draw a note
	 *
	 * @param p Painting device
	 * @param pos Center of the note to draw
	 * @param pNote Particular note to draw
	 * @param bIsForeground Whether the @a pNote is contained in the
	 *   pattern currently shown in the pattern editor (the one
	 *   selected in the song editor)
	 */
	void drawNoteSymbol( QPainter &p, const QPoint& pos, H2Core::Note *pNote,
						 bool bIsForeground = true ) const;

	//! Get notes to show in pattern editor.
	//! This may include "background" notes that are in currently-playing patterns
	//! rather than the current pattern.
	std::vector<std::shared_ptr<H2Core::Pattern>> getPatternsToShow( void );

	/** Updates #m_pBackgroundPixmap to show the latest content. */
	virtual void createBackground();
	void invalidateBackground();
	QPixmap *m_pBackgroundPixmap;
	bool m_bBackgroundInvalid;

	/**
	 * Adjusts #m_nActiveWidth and #m_nEditorWidth to the current
	 * state of the editor.
	 */
	void updateWidth();

	/** Indicates whether the mouse pointer entered the widget.*/
	bool m_bEntered;
	virtual void enterEvent( QEvent *ev ) override;
	virtual void leaveEvent( QEvent *ev ) override;
	virtual void focusInEvent( QFocusEvent *ev ) override;
	virtual void focusOutEvent( QFocusEvent *ev ) override;

	int m_nTick;

	/** Stores the properties of @a pNote in member variables.*/
	void storeNoteProperties( const H2Core::Note* pNote );
	
	/** Cached properties used when adjusting a note property via
	 * right-press mouse movement.
	 */
	/** Selected row in the DB / DrumPatternEditor */
	int m_nSelectedRow = 0;
	int m_nRealColumn = 0;
	int m_nColumn = 0;
	int m_nDragStartRow = 0;
	int m_nPressedLine = 0;
	int m_nOldPoint;
	
	int m_nOldLength = 0;
	float m_fVelocity = 0;
	float m_fOldVelocity = 0;
	float m_fPan = 0;
	float m_fOldPan = 0;
	float m_fLeadLag = 0;
	float m_fOldLeadLag = 0;
	float m_fProbability = 0;
	float m_fOldProbability = 0;
	Editor m_editor;
	Mode m_mode;
};

#endif // PATERN_EDITOR_H
