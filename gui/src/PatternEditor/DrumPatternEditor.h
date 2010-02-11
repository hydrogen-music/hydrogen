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

#include "config.h"
#include "../EventListener.h"

#include <hydrogen/Object.h>

#include <QtGui>

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
class DrumPatternEditor : public QWidget, public EventListener, public Object
{
	Q_OBJECT

	public:
		DrumPatternEditor(QWidget* parent, PatternEditorPanel *panel);
		~DrumPatternEditor();

		void setResolution(uint res, bool bUseTriplets);
		uint getResolution() {	return m_nResolution;	}
		bool isUsingTriplets() {	return m_bUseTriplets;	}

		void zoom_in();
		void zoom_out();

		// Implements EventListener interface
		virtual void patternModifiedEvent();
		virtual void patternChangedEvent();
		virtual void selectedPatternChangedEvent();
		virtual void selectedInstrumentChangedEvent();
		//~ Implements EventListener interface

		static QColor computeNoteColor( float );


	public slots:
		void updateEditor();

	private:
		float m_nGridWidth;
		uint m_nGridHeight;
		int m_nEditorHeight;
		uint m_nResolution;
		bool m_bUseTriplets;

		//QPixmap *m_pBackground;
		//QPixmap *m_pTemp;

		// usati per la lunghezza della nota
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

		int getColumn(QMouseEvent *ev);
};


#endif
