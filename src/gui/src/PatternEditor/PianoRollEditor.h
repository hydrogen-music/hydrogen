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

#ifndef PIANO_ROLL_EDITOR_H
#define PIANO_ROLL_EDITOR_H

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Basics/Note.h>
#include "../Selection.h"
#include "PatternEditor.h"
#include "../Widgets/WidgetWithScalableFont.h"

#include <QtGui>
#include <QtWidgets>

/** \ingroup docGUI*/
class PianoRollEditor: public PatternEditor, protected WidgetWithScalableFont<7, 9, 11>, public H2Core::Object<PianoRollEditor>
{
    H2_OBJECT(PianoRollEditor)
    Q_OBJECT
	public:
		PianoRollEditor( QWidget *pParent, QScrollArea *pScrollView );
		~PianoRollEditor();

		// Selection manager interface
		//! Selections are indexed by Note pointers.

		virtual std::vector<SelectionIndex> elementsIntersecting( const QRect& r ) override;

		QPoint noteToPoint( H2Core::Note* pNote ) const;

	public slots:
		virtual void updateEditor( bool bPatternOnly = false ) override;
		virtual void selectAll() override;
		void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

	private:
		void createBackground() override;
		void drawPattern();
		void drawFocus( QPainter& painter );

		virtual void paintEvent(QPaintEvent *ev) override;
		virtual void keyPressEvent ( QKeyEvent * ev ) override;
		
		QPixmap *m_pBackground;
		QPixmap *m_pTemp;

		QScrollArea *m_pScrollView;
};

#endif

