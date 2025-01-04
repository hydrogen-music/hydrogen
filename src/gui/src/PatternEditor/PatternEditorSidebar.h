/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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


#ifndef PATTERN_EDITOR_INSTRUMENT_LIST_H
#define PATTERN_EDITOR_INSTRUMENT_LIST_H

#include <vector>
#include <memory>
#include <QtGui>
#include <QtWidgets>

#include <core/Globals.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include "PatternEditorPanel.h"
#include "../EventListener.h"
#include "../Selection.h"
#include "../Widgets/PixmapWidget.h"
#include "../Widgets/WidgetWithScalableFont.h"

namespace H2Core
{
	class Pattern;
}

class Button;

class SidebarLabel : public QLabel, public H2Core::Object<SidebarLabel>
{
	H2_OBJECT(SidebarLabel)
	Q_OBJECT

	public:
		SidebarLabel( QWidget* pParent, const QSize& size, const QString& sText,
					  int nIndent );
		~SidebarLabel();

		/** Text will be cleared on showPlusSign() */
		void setText( const QString& sNewText );
		/** Indicator to show add something new. Icon is cleared on setText() */
		void showPlusSign();
		bool isShowingPlusSign() const;
		void setColor( const QColor& backgroundColor, const QColor& textColor,
					   const QColor& cursorColor );
		void updateFont();
		void setShowCursor( bool bShowCursor );
		void setDimed( bool bDimed );

	signals:
		void labelClicked( QMouseEvent* pEvent );
		void labelDoubleClicked();

	private:

		static constexpr int nDimScaling = 125;

		virtual void enterEvent( QEvent *ev ) override;
		virtual void leaveEvent( QEvent *ev ) override;
		virtual void mousePressEvent( QMouseEvent* pEvent ) override;
		virtual void mouseDoubleClickEvent( QMouseEvent* pEvent ) override;
		virtual void paintEvent( QPaintEvent* ev) override;

		QWidget* m_pParent;
		QString m_sText;
		int m_nIndent;
		bool m_bShowPlusSign;
		QColor m_backgroundColor;
		QColor m_textColor;
		QColor m_cursorColor;
		/** Whether the mouse pointer entered the boundary of the widget.*/
		bool m_bEntered;
		bool m_bShowCursor;
		/** When using the #PianoRollEditor all rows not selected will be dimed
		 * to emphasize that one notes of one particular row are displayed. */
		bool m_bDimed;
};

inline bool SidebarLabel::isShowingPlusSign() const {
	return m_bShowPlusSign;
}

/** \ingroup docGUI*/
class SidebarRow : public PixmapWidget
				 , protected WidgetWithScalableFont<8, 10, 12>
{
    H2_OBJECT(SidebarRow)
	Q_OBJECT

	public:
		explicit SidebarRow( QWidget* pParent, const DrumPatternRow& row );

		void set( const DrumPatternRow& row );
		void setSelected( bool isSelected );
		void setDimed( bool bDimed );

		void updateFont();
		void updateStyleSheet();

		static constexpr int m_nButtonWidth = 18;
		static constexpr int m_nTypeLblWidth = 100;
		virtual void mousePressEvent(QMouseEvent *ev) override;

public slots:
		void update();

	private slots:
		void muteClicked();
		void soloClicked();
		void sampleWarningClicked();

	private:
		PatternEditorPanel* m_pPatternEditorPanel;
		QMenu *m_pFunctionPopup;
		QMenu *m_pFunctionPopupSub;
		QAction* m_pRenameInstrumentAction;
		QAction* m_pDeleteInstrumentAction;
		SidebarLabel* m_pInstrumentNameLbl;
		SidebarLabel* m_pTypeLbl;
		bool m_bIsSelected;
		DrumPatternRow m_row;
		Button *m_pMuteBtn;
		Button *m_pSoloBtn;
		Button *m_pSampleWarning;
		/** When using the #PianoRollEditor all rows not selected will be dimed
		 * to emphasize that one notes of one particular row are displayed. */
		bool m_bDimed;

		QColor m_backgroundColor;

	virtual void enterEvent( QEvent *ev ) override;
	virtual void leaveEvent( QEvent *ev ) override;
	virtual void paintEvent( QPaintEvent* ev ) override;

		void setMuted(bool isMuted);
		void setSoloed( bool soloed );
		void setSamplesMissing( bool bSamplesMissing );

		/** Whether the cursor entered the boundary of the widget.*/
		bool m_bEntered;
};


/** \ingroup docGUI*/
class PatternEditorSidebar : public QWidget,
							 public EventListener,
							 public H2Core::Object<PatternEditorSidebar> {
	H2_OBJECT(PatternEditorSidebar)
	Q_OBJECT

	public:
		PatternEditorSidebar( QWidget *parent );
		~PatternEditorSidebar();

		virtual void mousePressEvent(QMouseEvent *event) override;
		virtual void mouseMoveEvent(QMouseEvent *event) override;
		virtual void dragEnterEvent(QDragEnterEvent *event) override;
		virtual void dropEvent(QDropEvent *event) override;

		virtual void instrumentMuteSoloChangedEvent( int ) override;

		void updateEditor();
		void updateFont();
		void updateStyleSheet();

		void dimRows( bool bDim );

		static constexpr int m_nWidth = 301;
		static constexpr int m_nMargin = 10;
	public slots:
		void updateRows();


	protected:
		PatternEditorPanel* m_pPatternEditorPanel;
		uint m_nGridHeight;
		uint m_nEditorHeight;
		std::vector<std::shared_ptr<SidebarRow>> m_rows;
		DragScroller *m_pDragScroller;

		/** Vertical position the drag event started at. In case there is no
		 * valid drag, the value will be set to -1. */
		int m_nDragStartY;

private:
	void drawFocus( QPainter& painter );
};


#endif
