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


#ifndef PATTERN_EDITOR_INSTRUMENT_LIST_H
#define PATTERN_EDITOR_INSTRUMENT_LIST_H


#include <core/Globals.h>

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include "../Widgets/PixmapWidget.h"
#include "../EventListener.h"
#include "../Selection.h"
#include "../Widgets/WidgetWithScalableFont.h"
#include "../Widgets/WidgetWithHighlightedList.h"

namespace H2Core
{
	class Pattern;
}

class PatternEditorPanel;
class Button;

/** \ingroup docGUI*/
class InstrumentLine : public PixmapWidget
					 , protected WidgetWithScalableFont<8, 10, 12>
					 , protected WidgetWithHighlightedList
{
    H2_OBJECT(InstrumentLine)
	Q_OBJECT

	public:
		explicit InstrumentLine(QWidget* pParent);

	int getNumber() const;
	
		void setName(const QString& sName);
		void setSelected(bool isSelected);
		void setNumber(int nIndex);
		void setMuted(bool isMuted);
		void setSoloed( bool soloed );
		void setSamplesMissing( bool bSamplesMissing );

	static constexpr int m_nButtonWidth = 18;

public slots:
		void onPreferencesChanged( H2Core::Preferences::Changes changes );

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
		void functionDeleteNotesAllPatterns();
		void functionCutNotesAllPatterns();

		void functionRandomizeVelocity();
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
		Button *m_pMuteBtn;
		Button *m_pSoloBtn;
		Button *m_pSampleWarning;

		virtual void mousePressEvent(QMouseEvent *ev) override;
	virtual void mouseDoubleClickEvent( QMouseEvent* ev ) override;
	virtual void enterEvent( QEvent *ev );
	virtual void leaveEvent( QEvent *ev );
	virtual void paintEvent( QPaintEvent* ev ) override;
		H2Core::Pattern* getCurrentPattern();

	void updateStyleSheet();
	void setRowSelection( RowSelection rowSelection );

	/** Whether the cursor entered the boundary of the widget.*/
	bool m_bEntered;
};

inline int InstrumentLine::getNumber() const {
	return m_nInstrumentNumber;
}


/** \ingroup docGUI*/
class PatternEditorInstrumentList :  public QWidget,
									 public EventListener,
									 public H2Core::Object<PatternEditorInstrumentList> {
	H2_OBJECT(PatternEditorInstrumentList)
	Q_OBJECT

	public:
		PatternEditorInstrumentList( QWidget *parent, PatternEditorPanel *pPatternEditorPanel );
		~PatternEditorInstrumentList();

		virtual void mousePressEvent(QMouseEvent *event) override;
		virtual void mouseMoveEvent(QMouseEvent *event) override;


		virtual void dragEnterEvent(QDragEnterEvent *event) override;
		virtual void dropEvent(QDropEvent *event) override;

	virtual void selectedInstrumentChangedEvent() override;
	virtual void updateSongEvent( int nEvent ) override;
	virtual void drumkitLoadedEvent() override;
	virtual void instrumentParametersChangedEvent( int ) override;
	
	void repaintInstrumentLines();
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

private:
	void drawFocus( QPainter& painter );
};


#endif
