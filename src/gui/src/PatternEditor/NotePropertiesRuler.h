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

#ifndef NOTE_PROPERTIES_RULER_H
#define NOTE_PROPERTIES_RULER_H

#include "../EventListener.h"

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

#include <hydrogen/object.h>

namespace H2Core
{
	class Pattern;
	class NoteKey;
}

class PatternEditorPanel;

class NotePropertiesRuler : public QWidget, public H2Core::Object, public EventListener
{
    H2_OBJECT
	Q_OBJECT
	public:
		enum NotePropertiesMode {
			VELOCITY,
			PAN,
			LEADLAG,
			NOTEKEY,
			PROBABILITY
		};

		NotePropertiesRuler( QWidget *parent, PatternEditorPanel *pPatternEditorPanel, NotePropertiesMode mode );
		~NotePropertiesRuler();

		void zoomIn();
		void zoomOut();

		//public slots:
		void updateEditor();

	private:
		static const int m_nKeys = 24;
		static const int m_nBasePitch = 12;

		NotePropertiesMode m_Mode;

		PatternEditorPanel *m_pPatternEditorPanel;
		H2Core::Pattern *m_pPattern;
		float m_nGridWidth;
		uint m_nEditorWidth;
		uint m_nEditorHeight;

		QPixmap *m_pBackground;

		double m_fLastSetValue;
		bool m_bValueHasBeenSet;

		void createVelocityBackground(QPixmap *pixmap);
		void createPanBackground(QPixmap *pixmap);
		void createLeadLagBackground(QPixmap *pixmap);
		void createNoteKeyBackground(QPixmap *pixmap);
		void paintEvent(QPaintEvent *ev);
		void mousePressEvent(QMouseEvent *ev);
		void mouseMoveEvent(QMouseEvent *ev);
		void wheelEvent(QWheelEvent *ev);
		void mouseReleaseEvent(QMouseEvent *ev);
		void keyPressEvent( QKeyEvent *ev );
		void focusInEvent( QFocusEvent *ev );
		void focusOutEvent( QFocusEvent *ev );
		void addUndoAction();
		void prepareUndoAction( int x );

		// Implements EventListener interface
		virtual void selectedPatternChangedEvent();
		virtual void selectedInstrumentChangedEvent();
		//~ Implements EventListener interface
		int __nSelectedPatternNumber;
		int __nSelectedInstrument;
		bool m_bMouseIsPressed;

		float __velocity;
		float __oldVelocity;
		float __pan_L;
		float __pan_R;
		float __oldPan_L;
		float __oldPan_R;
		float __leadLag;
		float __oldLeadLag;
		float __probability;
		float __oldProbability;
		int __noteKeyVal;
		int __oldNoteKeyVal;
		int __octaveKeyVal;
		int __oldOctaveKeyVal;
		int __checkXPosition;

		int __columnCheckOnXmouseMouve;
		int __undoColumn;
		QString __mode;

};


#endif
