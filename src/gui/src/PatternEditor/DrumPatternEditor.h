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


#ifndef DRUM_PATTERN_EDITOR_H
#define DRUM_PATTERN_EDITOR_H

#include "../EventListener.h"
#include "../Selection.h"
#include "PatternEditor.h"
#include "NotePropertiesRuler.h"
#include "../Widgets/WidgetWithScalableFont.h"

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Helpers/Filesystem.h>

#include <QtGui>
#include <QtWidgets>

class PatternEditorInstrumentList;

///
/// Drum pattern editor
///
/** \ingroup docGUI*/
class DrumPatternEditor : public PatternEditor, protected WidgetWithScalableFont<7, 9, 11>
{
    H2_OBJECT(DrumPatternEditor)
	Q_OBJECT

	public:
		DrumPatternEditor(QWidget* parent, PatternEditorPanel *panel);
		~DrumPatternEditor();

		// Implements EventListener interface
		virtual void selectedPatternChangedEvent() override;
		virtual void selectedInstrumentChangedEvent() override;
	virtual void drumkitLoadedEvent() override;
	virtual void songModeActivationEvent() override;
		// ~ Implements EventListener interface
		void addOrDeleteNoteAction(		int nColumn,
										int row,
										int selectedPatternNumber,
										int oldLength,
										float oldVelocity,
										float fOldPan,
										float oldLeadLag,
										int oldNoteKeyVal,
										int oldOctaveKeyVal,
										float probability,
										bool listen,
										bool isMidi,
										bool isInstrumentMode,
										bool isNoteOff,
										bool isDelete );
		void moveNoteAction( int nColumn,
							 int nRow,
							 int nPattern,
							 int nNewColumn,
							 int nNewRow,
							 H2Core::Note *note);

		void addOrRemoveNote( int nColumn, int nRealColumn, int row,
							  bool bDoAdd = true, bool bDoDelete = true,
							  bool bIsNoteOff = false );
		void undoRedoAction(    int column,
								NotePropertiesRuler::Mode mode,
								int nSelectedPatternNumber,
								int nSelectedInstrument,
								float velocity,
								float pan,
								float leadLag,
								float probability,
								int noteKeyVal,
								int octaveKeyVal );
		void functionClearNotesUndoAction( std::list< H2Core::Note* > noteList, int nSelectedInstrument, int patternNumber );
		void functionFillNotesUndoAction( QStringList noteList, int nSelectedInstrument, int patternNumber );
		void functionFillNotesRedoAction( QStringList noteList, int nSelectedInstrument, int patternNumber );
		void functionRandomVelocityAction( QStringList noteVeloValue, int nSelectedInstrument, int selectedPatternNumber );
		void functionMoveInstrumentAction( int nSourceInstrument,  int nTargetInstrument );
		void functionDropInstrumentUndoAction( int nTargetInstrument, std::vector<int>* AddedComponents );
		/**
		 * \param sDrumkitPath
		 * \param sInstrumentName
		 * \param nTargetInstrument
		 * \param AddedComponents
		 * for the drumkit.
		 */
		void functionDropInstrumentRedoAction(QString sDrumkitPath, QString sInstrumentName, int nTargetInstrument, std::vector<int>* pAddedComponents );
		void functionDeleteInstrumentUndoAction(  std::list< H2Core::Note* > noteList, int nSelectedInstrument, QString instrumentName, QString drumkitName );
		void functionAddEmptyInstrumentUndo();
		void functionAddEmptyInstrumentRedo();
		void functionPasteNotesRedoAction(std::list<H2Core::Pattern*> & changeList, std::list<H2Core::Pattern*> & appliedList);
		void functionPasteNotesUndoAction(std::list<H2Core::Pattern*> & appliedList);

		// Synthetic UI events from selection manager
		virtual void mouseClickEvent( QMouseEvent *ev ) override;
		virtual void mouseDragStartEvent( QMouseEvent *ev ) override;
		virtual void mouseDragUpdateEvent( QMouseEvent *ev ) override;
		virtual void selectionMoveEndEvent( QInputEvent *ev ) override;

		// Selected notes are indexed by their address to ensure that a
		// note is definitely uniquely identified. This carries the risk
		// that state pointers to deleted notes may find their way into
		// the selection.
		virtual std::vector<SelectionIndex> elementsIntersecting( QRect r ) override;

		virtual QRect getKeyboardCursorRect() override;

	public slots:
		virtual void updateEditor( bool bPatternOnly = false ) override;
		virtual void selectAll() override;
		virtual void deleteSelection() override;
		virtual void paste() override;
		void onPreferencesChanged( H2Core::Preferences::Changes changes );

	private:
	void createBackground() override;
	/**
	 * Draw a note
	 *
	 * @param pNote Particular note to draw
	 * @param painter Painting device
	 * @param bIsForeground Whether the @a pNote is contained in the
	 *   pattern currently shown in the pattern editor (the one
	 *   selected in the song editor)
	 */
	void drawNote( H2Core::Note* pNote, QPainter& painter, bool bIsForeground = true );
		void drawPattern( QPainter& painter );
		void drawBackground( QPainter& pointer );
		void drawFocus( QPainter& painter );

		virtual void keyPressEvent (QKeyEvent *ev) override;
		virtual void keyReleaseEvent (QKeyEvent *ev) override;
		virtual void showEvent ( QShowEvent *ev ) override;
		virtual void hideEvent ( QHideEvent *ev ) override;
		virtual void paintEvent(QPaintEvent *ev) override;
	virtual void mousePressEvent( QMouseEvent *ev ) override;

		QString renameCompo( QString OriginalName );
};


#endif
