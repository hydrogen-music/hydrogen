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

#include <vector>
#include <memory>
#include <QtGui>
#include <QtWidgets>

#include <core/config.h>
#include <core/Globals.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include "PatternEditorPanel.h"
#include "../EventListener.h"
#include "../Selection.h"
#include "../Widgets/WidgetWithScalableFont.h"

namespace H2Core
{
	class Pattern;
}

class Button;

class SidebarLabel : public QLineEdit, public H2Core::Object<SidebarLabel> {
	H2_OBJECT( SidebarLabel )
	Q_OBJECT

   public:
	static constexpr int nDimScaling = 125;

	enum class Type { Instrument, Type };

	SidebarLabel(
		QWidget* pParent,
		Type type,
		const QSize& size,
		const QString& sText,
		int nLeftMargin
	);
	~SidebarLabel();

	/** Text will be cleared on showPlusSign() */
	void setText( const QString& sNewText );
	/** Indicator to show add something new. Icon is cleared on setText() */
	void setShowPlusSign( bool bShowPlusSign );
	bool isShowingPlusSign() const;
	void setColor(
		const QColor& backgroundColor,
		const QColor& textColor,
		const QColor& cursorColor
	);
	void updateFont();
	void setShowCursor( bool bShowCursor );
	void setDimed( bool bDimed );

   signals:
	void editAccepted();
	void editRejected();
	void labelClicked( QMouseEvent* pEvent );
	void labelDoubleClicked( QMouseEvent* pEvent );

   private:
#ifdef H2CORE_HAVE_QT6
	virtual void enterEvent( QEnterEvent* ev ) override;
#else
	virtual void enterEvent( QEvent* ev ) override;
#endif
	void keyPressEvent( QKeyEvent* pEvent ) override;
	void leaveEvent( QEvent* ev ) override;
	void mouseDoubleClickEvent( QMouseEvent* pEvent ) override;
	void mousePressEvent( QMouseEvent* pEvent ) override;
    void mouseMoveEvent( QMouseEvent* ev ) override;
	void paintEvent( QPaintEvent* ev ) override;

	void updateStyle();

	QWidget* m_pParent;
	Type m_type;
	QString m_sText;
	bool m_bShowPlusSign;
	QColor m_backgroundColor;
	QColor m_textColor;
	QColor m_textBaseColor;
	QColor m_cursorColor;
	/** Whether the mouse pointer entered the boundary of the widget.*/
	bool m_bEntered;
	bool m_bShowCursor;
	/** When using the #PianoRollEditor all rows not selected will be dimed
	 * to emphasize that one notes of one particular row are displayed. */
	bool m_bDimed;
};

inline bool SidebarLabel::isShowingPlusSign() const
{
	return m_bShowPlusSign;
}

/** \ingroup docGUI*/
class SidebarRow : public QWidget,
				   public H2Core::Object<SidebarRow>,
				   protected WidgetWithScalableFont<8, 10, 12> {
	H2_OBJECT( SidebarRow )
	Q_OBJECT

   public:
	static constexpr int m_nButtonWidth = 18;
	static constexpr int m_nTypeLblWidth = 100;

	explicit SidebarRow( QWidget* pParent, const DrumPatternRow& row );

	void set( const DrumPatternRow& row );
	void setDimed( bool bDimed );
	void setDragHovered( bool bDragHovered );
	void setSelected( bool isSelected );

	void updateColors();
	void updateFont();
	void updateStyleSheet();
	void updateTypeLabelVisibility( bool bVisible );

	void mousePressEvent( QMouseEvent* ev ) override;

   public slots:
	void update();

   private slots:
	void muteClicked();
	void soloClicked();
	void sampleWarningClicked();

   private:
#ifdef H2CORE_HAVE_QT6
	void enterEvent( QEnterEvent* ev ) override;
#else
	void enterEvent( QEvent* ev ) override;
#endif
	void leaveEvent( QEvent* ev ) override;

	void setMuted( bool isMuted );
	void setSoloed( bool soloed );
	void setSamplesMissing( bool bSamplesMissing );

	PatternEditorPanel* m_pPatternEditorPanel;
	QMenu* m_pFunctionPopup;
	QMenu* m_pFunctionPopupSub;
	QAction* m_pRenameInstrumentAction;
	QAction* m_pDuplicateInstrumentAction;
	QAction* m_pDeleteInstrumentAction;
	QAction* m_pTypeLabelVisibilityAction;
	SidebarLabel* m_pInstrumentNameLbl;
	SidebarLabel* m_pTypeLbl;
	DrumPatternRow m_row;
	Button* m_pMuteBtn;
	Button* m_pSoloBtn;
	Button* m_pSampleWarning;

	/** When using the #PianoRollEditor all rows not selected will be dimed
	 * to emphasize that one notes of one particular row are displayed. */
	bool m_bDimed;

	/** A drag move is taking place in the parent widget and it is moving
	 * right above this widget. */
	bool m_bDragHovered;

	/** Whether the cursor entered the boundary of the widget.*/
	bool m_bEntered;

	bool m_bIsSelected;
};

/** \ingroup docGUI*/
class PatternEditorSidebar : public QWidget,
							 public EventListener,
							 public H2Core::Object<PatternEditorSidebar> {
	H2_OBJECT( PatternEditorSidebar )
	Q_OBJECT

   public:
	static constexpr int m_nWidth = 301;
	static constexpr int m_nMargin = 10;

	PatternEditorSidebar( QWidget* parent );
	~PatternEditorSidebar();

	void updateColors();
	void updateEditor();
	void updateFont();
	void updateStyleSheet();
	void updateTypeLabelVisibility( bool bVisible );

	void dimRows( bool bDim );

	// EventListener
	void instrumentMuteSoloChangedEvent( int ) override;

	// Qt events
	void dragEnterEvent( QDragEnterEvent* event ) override;
	void dragMoveEvent( QDragMoveEvent* ev ) override;
	void dropEvent( QDropEvent* event ) override;
	void mouseMoveEvent( QMouseEvent* event ) override;
	void mousePressEvent( QMouseEvent* event ) override;

   public slots:
	/** Update every SidebarRow, create or destroy lines if necessary. */
	void updateRows();

   private:
	int yToRow( int nY ) const;

	PatternEditorPanel* m_pPatternEditorPanel;
	uint m_nEditorHeight;
	std::vector<SidebarRow*> m_rows;
	DragScroller* m_pDragScroller;

	/** Vertical position the drag event started at. In case there is no
	 * valid drag, the value will be set to -1. */
	int m_nDragStartY;
	/** A value of -1 will cause the rendering to be omitted. */
	int m_nLastDragRow;
};

#endif
