/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: PatternEditor.h,v 1.10 2005/05/09 18:11:54 comix Exp $
 *
 */


#ifndef PATTERN_EDITOR_H
#define PATTERN_EDITOR_H

#include <qwidget.h>
#include <qtimer.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qapplication.h>
#include <qpopupmenu.h>

#include "lib/Object.h"

class PatternEditorInstrumentList;
class PatternEditorVelocityRuler;
class PatternEditorPanel;
class Note;
class Pattern;

///
/// Pattern editor
///
class PatternEditor : public QWidget, public Object {
	Q_OBJECT

	public:
		PatternEditor(QWidget* parent, PatternEditorPanel *pPatternEditorPanel);
		~PatternEditor();

		void paintEvent(QPaintEvent *ev);
		void setResolution(uint res, bool bUseTriplets);
		uint getResolution() {	return m_nResolution;	}
		bool isUsingTriplets() {	return m_bUseTriplets;	}

		void showEvent ( QShowEvent *ev );
		void hideEvent ( QHideEvent *ev );

		void zoomIn();
		void zoomOut();

	public slots:
		void updateEditor(bool forceRepaint = false);

	private:
		uint m_nGridWidth;
		uint m_nGridHeight;

		QPixmap m_background;

		bool m_bChanged;
		bool m_bNotesChanged;

		uint m_nResolution;
		bool m_bUseTriplets;

		// usati per la lunghezza della nota
		bool m_bRightBtnPressed;
		Note *m_pDraggedNote;
		//~

		Pattern *m_pPattern;

		PatternEditorPanel *m_pPatternEditorPanel;

		void drawNote(Note* note, uint nSequence, QPixmap *pixmap);
		void drawPattern(QPixmap *pixmap);
		void createBackground(QPixmap *pixmap);

		void mousePressEvent(QMouseEvent *ev);
		void mouseReleaseEvent(QMouseEvent *ev);
		void mouseMoveEvent(QMouseEvent *ev);
		void keyPressEvent (QKeyEvent *ev);

		int getColumn(QMouseEvent *ev);

		// selecting/moving/copying notes etc
		enum RegionManipulationMode {
			COPY_MODE,
			MOVE_MODE,
			DELETE_MODE
		};

		bool m_bLasso;
		bool m_bLassoRepaint;
		bool m_bDrag;
		bool m_bDragRepaint;
		bool m_bSelect;
		bool m_bDragCopy;
		QPoint m_mousePos;
		QRect m_srcRegion;
		QRect m_oldRegion;
		QRect m_dstRegion;
		bool inSelect( int nColumn, int nRow, QRect area);
		uint getXPos(int nColumn);
		uint getYPos(int nRow);
		int getSnapWidth();
		void markupNotes();
		void copyNotes();
		void manipulateRegion(RegionManipulationMode mode);
		vector <Note*> m_noteList;
		vector <QPoint> m_notePoint;
};





class PatternEditorRuler : public QWidget, public Object
{
	Q_OBJECT

	public:
		PatternEditorRuler(QWidget* parent, PatternEditorPanel *pPatternEditorPanel);
		~PatternEditorRuler();

		void paintEvent(QPaintEvent *ev);
		void updateStart(bool start);

		void showEvent ( QShowEvent *ev );
		void hideEvent ( QHideEvent *ev );

		void zoomIn();
		void zoomOut();

	public slots:
		void updateEditor( bool bRedrawAll = false );

	private:
		uint m_nRulerWidth;
		uint m_nRulerHeight;
		uint m_nGridWidth;
		QPixmap m_background;
		QPixmap m_tickPosition;
		QPixmap m_temp;
		QTimer *m_pTimer;
		int m_nTicks;
		bool m_bChanged;
		PatternEditorPanel *m_pPatternEditorPanel;
		Pattern *m_pPattern;

};



class PatternEditorInstrumentList : public QWidget, public Object {
	Q_OBJECT

	public:
		PatternEditorInstrumentList( QWidget *parent, PatternEditorPanel *pPatternEditorPanel );
		~PatternEditorInstrumentList();


	public slots:
		void updateEditor();

		void functionClearNotes();
		void functionFillNotes();
		void functionMute();
		void functionLock();
		void functionSolo();
		void functionRandomizeVelocity();


	private:
		PatternEditorPanel *m_pPatternEditorPanel;
		Pattern *m_pPattern;
		uint m_nGridHeight;
		uint m_nEditorWidth;
		uint m_nEditorHeight;
		bool m_bChanged;
		QPixmap m_background;
		QPixmap m_temp;

		QPixmap m_genericIcon;
		QPopupMenu *m_pFunctionPopup;

		void paintEvent(QPaintEvent *ev);
		void createBackground(QPixmap *pixmap);
		void mousePressEvent(QMouseEvent *ev);
		Pattern* getCurrentPattern();
};



class NotePropertiesRuler : public QWidget, public Object
{
	Q_OBJECT
	public:
		enum NotePropertiesMode {
			VELOCITY,
			PITCH
		};

		NotePropertiesRuler( QWidget *parent, PatternEditorPanel *pPatternEditorPanel, NotePropertiesMode mode );
		~NotePropertiesRuler();

		void zoomIn();
		void zoomOut();

	public slots:
		void updateEditor();

	private:
		static const int m_nKeys = 24;
		static const int m_nBasePitch = 12;

		NotePropertiesMode m_mode;

		PatternEditorPanel *m_pPatternEditorPanel;
		Pattern *m_pPattern;
		uint m_nGridWidth;
		uint m_nEditorWidth;
		uint m_nEditorHeight;

		bool m_bChanged;
		QPixmap m_background;

		void createVelocityBackground(QPixmap *pixmap);
		void createPitchBackground(QPixmap *pixmap);
		void paintEvent(QPaintEvent *ev);
		void mousePressEvent(QMouseEvent *ev);
		void mouseMoveEvent(QMouseEvent *ev);
};


#endif
