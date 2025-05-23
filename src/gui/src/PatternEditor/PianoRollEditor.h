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

#ifndef PIANO_ROLL_EDITOR_H
#define PIANO_ROLL_EDITOR_H

#include <core/config.h>
#include <core/Object.h>

#include "PatternEditor.h"
#include "../Selection.h"

#include <QtGui>
#include <QtWidgets>

#include <memory>
#include <vector>

namespace H2Core {
	class Note;
}

class PitchLabel : public QLabel, public H2Core::Object<PitchLabel>
{
	H2_OBJECT(PitchLabel)
	Q_OBJECT

	public:
		PitchLabel( QWidget* pParent, const QString& sText, int nHeight );
		~PitchLabel();

		void setBackgroundColor( const QColor& backgroundColor );
		void updateStyleSheet();
		void updateFont();
		void setSelected( bool bSelected );

	private:
#ifdef H2CORE_HAVE_QT6
		virtual void enterEvent( QEnterEvent *ev ) override;
#else
		virtual void enterEvent( QEvent *ev ) override;
#endif
		virtual void leaveEvent( QEvent *ev ) override;
		virtual void mousePressEvent( QMouseEvent* pEvent ) override;
		virtual void paintEvent( QPaintEvent* ev) override;

		QWidget* m_pParent;
		QString m_sText;
		QColor m_backgroundColor;
		QColor m_textColor;
		QColor m_cursorColor;
		/** Whether the mouse pointer entered the boundary of the widget.*/
		bool m_bEntered;
		bool m_bSelected;
};

/** \ingroup docGUI*/
class PitchSidebar : public QWidget,
					 public H2Core::Object<PitchSidebar> {
	H2_OBJECT(PitchSidebar)
	Q_OBJECT

	public:
		PitchSidebar( QWidget *parent, int nHeight, int nGridHeight );
		~PitchSidebar();

		void updateRows();

		void setRowColor( int nRowIndex, const QColor& backgroundColor );
		void updateStyleSheet();
		void updateFont();
		void selectedRow( int nRowIndex );

		void rowPressed( QMouseEvent *ev, PitchLabel* pLabel );

	private:
		QMenu *m_pFunctionPopup;
		QMenu *m_pFunctionPopupSub;

		std::vector<PitchLabel*> m_rows;
		int m_nHeight;
		int m_nGridHeight;
		int m_nRowClicked;
};

/** \ingroup docGUI*/
class PianoRollEditor: public PatternEditor,
					   public H2Core::Object<PianoRollEditor>
{
    H2_OBJECT(PianoRollEditor)
    Q_OBJECT
	public:
		PianoRollEditor( QWidget *pParent );
		~PianoRollEditor();

		// Selection manager interface
		//! Selections are indexed by Note pointers.

		virtual std::vector<SelectionIndex> elementsIntersecting( const QRect& r ) override;

		QPoint noteToPoint( std::shared_ptr<H2Core::Note> pNote ) const;

		void updateFont();
		void updateStyleSheet();

	public slots:
		virtual void selectAll() override;

	private:
		void createBackground() override;

		virtual void paintEvent(QPaintEvent *ev) override;
		virtual void keyPressEvent ( QKeyEvent * ev ) override;

		PitchSidebar* m_pPitchSidebar;
};

#endif

