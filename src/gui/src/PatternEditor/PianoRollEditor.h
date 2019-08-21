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

#ifndef PIANO_ROLL_EDITOR_H
#define PIANO_ROLL_EDITOR_H

#include <hydrogen/object.h>
#include "../EventListener.h"

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

namespace H2Core
{
	class Pattern;
	class Note;
}

class PatternEditorPanel;

class PianoRollEditor: public QWidget, public EventListener, public H2Core::Object
{
    H2_OBJECT
    Q_OBJECT
	public:
		PianoRollEditor( QWidget *pParent, PatternEditorPanel *panel );
		~PianoRollEditor();


		// Implements EventListener interface
		virtual void selectedPatternChangedEvent();
		virtual void selectedInstrumentChangedEvent();
		virtual void patternModifiedEvent();
		//~ Implements EventListener interface
		void setResolution(uint res, bool bUseTriplets, bool bUseQuintuplets, bool bUseSeptuplets, bool bUse9tuplets);

		void zoom_in();
		void zoom_out();
		void addOrDeleteNoteAction(  int nColumn,
					     int pressedLine,
					     int selectedPatternNumber,
					     int selectedinstrument,
					     int oldLength,
					     float oldVelocity,
					     float oldPan_L,
					     float oldPan_R,
					     float oldLeadLag,
					     int oldNoteKeyVal,
                                             int oldOctaveKeyVal,
                                             bool noteOff);

		void editNotePropertiesAction(   int nColumn,
						int nRealColumn,
						int selectedPatternNumber,
						int selectedInstrumentnumber,
						float velocity,
						float pan_L,
						float pan_R,
						float leadLag,
						int pressedLine );
                void editNoteLengthAction( int nColumn,  int nRealColumn, int length, int selectedPatternNumber, int nSelectedInstrumentnumber, int pressedLine );

	public slots:
		void updateEditor();

	private:

		unsigned m_nRowHeight;
		unsigned m_nOctaves;

		uint m_nResolution;
		bool m_bRightBtnPressed;
		bool m_bUseTriplets;
	  	bool m_bUseQuintuplets;
	  	bool m_bUseSeptuplets;
	  	bool m_bUse9tuplets;

		H2Core::Pattern *m_pPattern;

		float m_nGridWidth;
		uint m_nEditorWidth;
		uint m_nEditorHeight;
		QPixmap *m_pBackground;
		QPixmap *m_pTemp;
		int m_pOldPoint;

		PatternEditorPanel *m_pPatternEditorPanel;
		H2Core::Note *m_pDraggedNote;

		void createBackground();
		void drawPattern();
		void draw_grid(QPainter& p );
		void drawNote( H2Core::Note *pNote, QPainter *pPainter );

		virtual void paintEvent(QPaintEvent *ev);
		virtual void mousePressEvent(QMouseEvent *ev);
		virtual void mouseMoveEvent(QMouseEvent *ev);
		virtual void mouseReleaseEvent(QMouseEvent *ev);
//		virtual void keyPressEvent ( QKeyEvent * ev );
		int getColumn(QMouseEvent *ev);
		int __selectedInstrumentnumber;
		int __selectedPatternNumber;
		int __nRealColumn;
		int __nColumn;
		int __pressedLine;
		int __oldLength;
		
		float __velocity;
		float __oldVelocity;
		float __pan_L;
		float __oldPan_L;
		float __pan_R;
		float __oldPan_R;
		float __leadLag;
		float __oldLeadLag;		
};

#endif

