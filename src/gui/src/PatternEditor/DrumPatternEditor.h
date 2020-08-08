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


#ifndef DRUM_PATTERN_EDITOR_H
#define DRUM_PATTERN_EDITOR_H

#include "../EventListener.h"

#include <hydrogen/object.h>

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

namespace H2Core
{
	class Note;
	class Pattern;
}

class PatternEditorInstrumentList;
class PatternEditorPanel;
///
/// Drum pattern editor
///
class DrumPatternEditor : public QWidget, public EventListener, public H2Core::Object
{
    H2_OBJECT
	Q_OBJECT

	public:
		DrumPatternEditor(QWidget* parent, PatternEditorPanel *panel);
		~DrumPatternEditor();

		void setResolution(uint res, bool bUseTriplets);
		uint getResolution() {	return m_nResolution;	}
		bool isUsingTriplets() { return m_bUseTriplets;	}

		void zoom_in();
		void zoom_out();

		static QColor computeNoteColor( float );

		// Implements EventListener interface
		virtual void patternModifiedEvent();
		virtual void patternChangedEvent();
		virtual void selectedPatternChangedEvent();
		virtual void selectedInstrumentChangedEvent();
		//~ Implements EventListener interface
		void addOrDeleteNoteAction(		int nColumn,
										int row,
										int selectedPatternNumber,
										int oldLength,
										float oldVelocity,
										float oldPan_L,
										float oldPan_R,
										float oldLeadLag,
										int oldNoteKeyVal,
										int oldOctaveKeyVal,
										bool listen,
										bool isMidi,
										bool isInstrumentMode,
										bool isNoteOff,
										bool isDelete );
		void addOrRemoveNote( int nColumn, int nRealColumn, int row );
		void editNoteLengthAction( int nColumn, int nRealColumn, int row, int length, int selectedPatternNumber );
		void undoRedoAction(    int column,
								QString mode,
								int nSelectedPatternNumber,
								int nSelectedInstrument,
								float velocity,
								float pan_L,
								float pan_R,
								float leadLag,
								float probability,
								int noteKeyVal,
								int octaveKeyVal );
		void functionClearNotesRedoAction( int nSelectedInstrument, int selectedPatternNumber );
		void functionClearNotesUndoAction( std::list< H2Core::Note* > noteList, int nSelectedInstrument, int patternNumber );
		void functionFillNotesUndoAction( QStringList noteList, int nSelectedInstrument, int patternNumber );
		void functionFillNotesRedoAction( QStringList noteList, int nSelectedInstrument, int patternNumber );
		void functionRandomVelocityAction( QStringList noteVeloValue, int nSelectedInstrument, int selectedPatternNumber );
		void functionMoveInstrumentAction( int nSourceInstrument,  int nTargetInstrument );
		void functionDropInstrumentUndoAction( int nTargetInstrument, std::vector<int>* AddedComponents );
		void functionDropInstrumentRedoAction(QString sDrumkitName, QString sInstrumentName, int nTargetInstrument, std::vector<int>* AddedComponents );
		void functionDeleteInstrumentUndoAction(  std::list< H2Core::Note* > noteList, int nSelectedInstrument, QString instrumentName, QString drumkitName );
		void functionAddEmptyInstrumentUndo();
		void functionAddEmptyInstrumentRedo();
		void functionPasteNotesRedoAction(std::list<H2Core::Pattern*> & changeList, std::list<H2Core::Pattern*> & appliedList);
		void functionPasteNotesUndoAction(std::list<H2Core::Pattern*> & appliedList);

	public slots:
		void updateEditor();

	private:
		float m_nGridWidth;
		uint m_nGridHeight;
		int m_nEditorHeight;
		uint m_nResolution;
		bool m_bUseTriplets;

		bool m_bRightBtnPressed;
		H2Core::Note *m_pDraggedNote;
		//~


		H2Core::Pattern *m_pPattern;

		PatternEditorPanel *m_pPatternEditorPanel;

		void __draw_note( H2Core::Note* note, QPainter& painter );
		void __draw_pattern( QPainter& painter );
		void __draw_grid( QPainter& painter );
		void __create_background( QPainter& pointer );

		virtual void mousePressEvent(QMouseEvent *ev);
		virtual void mouseReleaseEvent(QMouseEvent *ev);
		virtual void mouseMoveEvent(QMouseEvent *ev);
		virtual void keyPressEvent (QKeyEvent *ev);
		virtual void showEvent ( QShowEvent *ev );
		virtual void hideEvent ( QHideEvent *ev );
		virtual void paintEvent(QPaintEvent *ev);
		virtual void focusInEvent( QFocusEvent *ev );

		int getColumn(QMouseEvent *ev);

		int findFreeCompoID( int startingPoint = 0 );
		int findExistingCompo( QString SourceName );
		QString renameCompo( QString OriginalName );

		int __nRealColumn;
		int __nColumn;
		int __row;
		int __oldLength;
		int __selectedPatternNumber;
};


#endif
