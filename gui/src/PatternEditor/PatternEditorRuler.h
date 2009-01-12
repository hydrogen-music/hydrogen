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

#ifndef PATTERN_EDITOR_RULER_H
#define PATTERN_EDITOR_RULER_H

#include "../EventListener.h"

#include <QtGui>
#include <hydrogen/Object.h>

class PatternEditorPanel;

namespace H2Core
{
	class Pattern;
}

class PatternEditorRuler : public QWidget, public Object, public EventListener
{
	Q_OBJECT

	public:
		PatternEditorRuler( QWidget* parent );
		~PatternEditorRuler();

		void paintEvent(QPaintEvent *ev);
		void updateStart(bool start);

		void showEvent( QShowEvent *ev );
		void hideEvent( QHideEvent *ev );

		void zoomIn();
		void zoomOut();
		float getGridWidth() const {
		return m_nGridWidth;
		};

	public slots:
		void updateEditor( bool bRedrawAll = false );

	private:
		uint m_nRulerWidth;
		uint m_nRulerHeight;
		float m_nGridWidth;

		QPixmap *m_pBackground;
		QPixmap m_tickPosition;

		QTimer *m_pTimer;
		int m_nTicks;
		PatternEditorPanel *m_pPatternEditorPanel;
		H2Core::Pattern *m_pPattern;

		// Implements EventListener interface
		virtual void selectedPatternChangedEvent();
		//~ Implements EventListener interface
};


#endif
