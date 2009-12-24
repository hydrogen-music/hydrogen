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


#ifndef PATTERN_EDITOR_INSTRUMENT_LIST_H
#define PATTERN_EDITOR_INSTRUMENT_LIST_H

#include "config.h"

#include <hydrogen/globals.h>

#include <QtGui>

#include <hydrogen/Object.h>
#include "../widgets/PixmapWidget.h"

namespace H2Core
{
	class Pattern;
}

class PatternEditorPanel;
class ToggleButton;

class InstrumentLine : public PixmapWidget
{
	Q_OBJECT

	public:
		InstrumentLine(QWidget* pParent);

		void setName(const QString& sName);
		void setSelected(bool isSelected);
		void setNumber(int nIndex);
		void setMuted(bool isMuted);
		void setSoloed( bool soloed );

	private slots:
		void functionClearNotes();

		void functionFillAllNotes();
		void functionFillEveryTwoNotes();
		void functionFillEveryThreeNotes();
		void functionFillEveryFourNotes();
		void functionFillEverySixNotes();
		void functionFillEveryEightNotes();
		void functionFillNotes( int every );

		void functionRandomizeVelocity();
		void functionDeleteInstrument();
		void muteClicked();
		void soloClicked();


	private:
		QMenu *m_pFunctionPopup;
		QMenu *m_pFunctionPopupSub;
		QLabel *m_pNameLbl;
		bool m_bIsSelected;
		int m_nInstrumentNumber;	///< The related instrument number
		ToggleButton *m_pMuteBtn;
		ToggleButton *m_pSoloBtn;

		virtual void mousePressEvent(QMouseEvent *ev);
		H2Core::Pattern* getCurrentPattern();
};


class PatternEditorInstrumentList : public QWidget, public Object {
	Q_OBJECT

	public:
		PatternEditorInstrumentList( QWidget *parent, PatternEditorPanel *pPatternEditorPanel );
		~PatternEditorInstrumentList();

		virtual void mousePressEvent(QMouseEvent *event);
		virtual void mouseMoveEvent(QMouseEvent *event);


		virtual void dragEnterEvent(QDragEnterEvent *event);
		virtual void dropEvent(QDropEvent *event);


	public slots:
		void updateInstrumentLines();


	protected:
		PatternEditorPanel *m_pPatternEditorPanel;
		H2Core::Pattern *m_pPattern;
		uint m_nGridHeight;
		uint m_nEditorWidth;
		uint m_nEditorHeight;
		InstrumentLine* m_pInstrumentLine[MAX_INSTRUMENTS];
		QTimer *m_pUpdateTimer;

		QPoint __drag_start_position;

		InstrumentLine* createInstrumentLine();
		void moveInstrumentLine(int,int);

};


#endif
