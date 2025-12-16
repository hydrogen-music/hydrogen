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

#ifndef SONG_EDITOR_PATTERN_LIST_H
#define SONG_EDITOR_PATTERN_LIST_H

#include <memory>

#include <unistd.h>

#include <QtGui>
#include <QtWidgets>
#include <QList>

#include <core/Object.h>
#include "PatternFillDialog.h"
#include "Selection.h"
#include "../Widgets/WidgetWithScalableFont.h"

namespace H2Core {
	class License;
	class Pattern;
}

class InlineEdit;

///
/// Song editor pattern list
///
/** \ingroup docGUI*/
class SongEditorPatternList :  public QWidget
							, protected WidgetWithScalableFont<8, 10, 12>
							, public H2Core::Object<SongEditorPatternList>
{
    H2_OBJECT(SongEditorPatternList)
	Q_OBJECT

	public:
		static constexpr int nWidth = 200;
		static constexpr int nMargin = 25;
	
		explicit SongEditorPatternList( QWidget *parent );
		~SongEditorPatternList();
	
		SongEditorPatternList(const SongEditorPatternList&) = delete;
		SongEditorPatternList& operator=( const SongEditorPatternList& rhs ) = delete;

		void movePatternLine( int, int );
		void acceptPatternPropertiesDialogSettings( const int nNewVersion,
													const QString& newPatternName,
													const QString& sNewAuthor,
													const QString& newPatternInfo,
													const H2Core::License& newLicense,
													const QString& newPatternCategory,
													int patternNr );
		void revertPatternPropertiesDialogSettings( const int nOldVersion,
													const QString& oldPatternName,
													const QString& sOldAuthor,
													const QString& oldPatternInfo,
													const H2Core::License& oldLicense,
													const QString& oldPatternCategory,
													int patternNr);
		int getGridHeight() { return m_nGridHeight; }

		void updateEditor();

	public slots:
		void patternPopup_edit();
		void patternPopup_save();
		void patternPopup_export();
		void patternPopup_replace();
		void patternPopup_properties();
		void patternPopup_delete();
		void patternPopup_duplicate();
		void patternPopup_fill();
		void patternPopup_virtualPattern();
		void inlineEditingAccepted();
		void inlineEditingRejected();

	private:
		void inlineEditPatternName( int row );

		void dragEnterEvent( QDragEnterEvent* event ) override;
		void dropEvent( QDropEvent* event ) override;
		void leaveEvent( QEvent* ev ) override;
		void mouseDoubleClickEvent( QMouseEvent* ev ) override;
		void mouseMoveEvent( QMouseEvent* event ) override;
		void mousePressEvent( QMouseEvent* ev ) override;
		void paintEvent( QPaintEvent* ev ) override;

		void createBackground();

		int 				m_nGridHeight;
		int 				m_nWidth;
		static constexpr int 	m_nInitialHeight = 10;
		int m_nRowClicked;

		QPixmap *			m_pBackgroundPixmap;
		bool m_bBackgroundInvalid;
							
		QPixmap				m_playingPattern_on_Pixmap;
		QPixmap				m_playingPattern_off_Pixmap;
		QPixmap				m_playingPattern_empty_Pixmap;
							
		QMenu *				m_pPatternPopup;
		InlineEdit*			m_pInlineEdit;
		std::shared_ptr<H2Core::Pattern>	m_pPatternBeingEdited;

		DragScroller *		m_pDragScroller;

		QPointF m_dragStartPoint;
        quint64 m_dragStartTimeStamp;

		/**
		 * Specifies the row the mouse cursor is currently hovered
		 * over. -1 for no cursor.
		 */
		int m_nRowHovered;
};

#endif
