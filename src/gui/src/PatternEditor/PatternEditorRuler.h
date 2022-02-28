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

#ifndef PATTERN_EDITOR_RULER_H
#define PATTERN_EDITOR_RULER_H

#include "../EventListener.h"
#include <QtGui>
#include <QtWidgets>
#include "../Widgets/WidgetWithScalableFont.h"

#include <core/Object.h>
#include <core/Preferences/Preferences.h>

class PatternEditorPanel;

namespace H2Core
{
	class Pattern;
}

/** \ingroup docGUI*/
class PatternEditorRuler :  public QWidget, protected WidgetWithScalableFont<8, 10, 12>,  public H2Core::Object<PatternEditorRuler>, public EventListener
{
    H2_OBJECT(PatternEditorRuler)
	Q_OBJECT

	public:
		explicit PatternEditorRuler( QWidget* parent );
		~PatternEditorRuler();
	
		PatternEditorRuler(const PatternEditorRuler&) = delete;
		PatternEditorRuler& operator=( const PatternEditorRuler& rhs ) = delete;

		void paintEvent(QPaintEvent *ev) override;
		void updateStart(bool start);

		void showEvent( QShowEvent *ev ) override;
		void hideEvent( QHideEvent *ev ) override;

		void zoomIn();
		void zoomOut();
		float getGridWidth() const {
			return m_fGridWidth;
		};

		void createBackground();

	public slots:
		void updateEditor( bool bRedrawAll = false );
		void onPreferencesChanged( H2Core::Preferences::Changes changes );

	private:
		uint m_nRulerWidth;
		uint m_nRulerHeight;
		float m_fGridWidth;

		QPixmap *m_pBackground;
		QPixmap m_tickPosition;

		QTimer *m_pTimer;
		int m_nTicks;
		PatternEditorPanel *m_pPatternEditorPanel;
		H2Core::Pattern *m_pPattern;

		// Implements EventListener interface
		virtual void selectedPatternChangedEvent() override;
		//~ Implements EventListener interface
};


#endif
