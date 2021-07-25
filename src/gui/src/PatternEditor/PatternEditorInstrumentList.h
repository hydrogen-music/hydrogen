/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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


#ifndef PATTERN_EDITOR_INSTRUMENT_LIST_H
#define PATTERN_EDITOR_INSTRUMENT_LIST_H


#include <core/Globals.h>

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include <core/Preferences.h>
#include "../Widgets/PixmapWidget.h"
#include "../Selection.h"
#include "../Widgets/WidgetWithScalableFont.h"

namespace H2Core
{
	class Pattern;
}

class PatternEditorPanel;
class ToggleButton;
class Button;

class InstrumentLine : public PixmapWidget, protected WidgetWithScalableFont<8, 10, 12>
{
    H2_OBJECT
	Q_OBJECT

	public:
		explicit InstrumentLine(QWidget* pParent);

		void setName(const QString& sName);
		void setSelected(bool isSelected);
		void setNumber(int nIndex);
		void setMuted(bool isMuted);
		void setSoloed( bool soloed );
		void setSamplesMissing( bool bSamplesMissing );

public slots:
		void onPreferencesChanged( bool bAppearanceOnly );

	private slots:
		void functionClearNotes();

		void functionFillAllNotes();
		void functionFillEveryTwoNotes();
		void functionFillEveryThreeNotes();
		void functionFillEveryFourNotes();
		void functionFillEverySixNotes();
		void functionFillEveryEightNotes();
		void functionFillEveryTwelveNotes();
		void functionFillEverySixteenNotes();
		void functionFillNotes( int every );
		void functionCopyAllInstrumentPatterns();
		void functionPasteAllInstrumentPatterns();
		void functionPasteInstrumentPatternExec(int patternID);
		void functionDeleteNotesAllPatterns();
		void functionCutNotesAllPatterns();

		void functionRandomizeVelocity();
		void functionDeleteInstrument();
		void functionRenameInstrument();
		void muteClicked();
		void soloClicked();
		void sampleWarningClicked();

		void selectInstrumentNotes();


	private:
		QMenu *m_pFunctionPopup;
		QMenu *m_pFunctionPopupSub;
		QLabel *m_pNameLbl;
		bool m_bIsSelected;
		int m_nInstrumentNumber;	///< The related instrument number
		ToggleButton *m_pMuteBtn;
		ToggleButton *m_pSoloBtn;
		Button *m_pSampleWarning;

		virtual void mousePressEvent(QMouseEvent *ev);
		H2Core::Pattern* getCurrentPattern();
		/** Used to detect changed in the font*/
		H2Core::Preferences::FontSize m_lastUsedFontSize;
};


class PatternEditorInstrumentList : public QWidget, public H2Core::Object {
	H2_OBJECT
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
		DragScroller *m_pDragScroller;

		QPoint __drag_start_position;

		InstrumentLine* createInstrumentLine();

};


#endif
