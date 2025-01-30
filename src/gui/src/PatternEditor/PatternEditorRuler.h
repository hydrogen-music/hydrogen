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
	/**
	 * Queries the audio engine to update the current position of the
	 * playhead.
	 *
	 * \param bForce The transport position is cached and updates in
	 * the transport position are only propagated to the other member
	 * of the PatternEditor once it changes. However, this will leave the
	 * pattern editor in a dirty state during startup since the ruler
	 * has to wait for all other associated objects being
	 * constructed. Using the @a bForce option an update is performed
	 * regardlessly.
	 */
	void updatePosition( bool bForce = false );

		void showEvent( QShowEvent *ev ) override;
		void hideEvent( QHideEvent *ev ) override;
	void mouseMoveEvent( QMouseEvent *ev ) override;
	void mousePressEvent( QMouseEvent *ev ) override;
	void leaveEvent( QEvent *ev ) override;

		void zoomIn();
		void zoomOut();
		float getGridWidth() const;
		int getWidthActive() const;

		void createBackground();
		void invalidateBackground();
		bool m_bBackgroundInvalid;

	public slots:
		void updateEditor( bool bRedrawAll = false );

	private:
		PatternEditorPanel* m_pPatternEditorPanel;
		uint m_nRulerWidth;
		uint m_nRulerHeight;
		float m_fGridWidth;

		QPixmap *m_pBackgroundPixmap;

		QTimer *m_pTimer;
		int m_nTick;

	int m_nHoveredColumn;
	/**
	 * Length of the song in pixels. As soon as the x coordinate of an
	 * event is smaller than this value, it lies within the active
	 * range of the song.
	 */
	int m_nWidthActive;
	/** Updates #m_nWidthActive.*/
	bool updateActiveRange();

		// Implements EventListener interface
	virtual void stateChangedEvent( const H2Core::AudioEngine::State& ) override;
	virtual void relocationEvent() override;
		// ~ Implements EventListener interface
};

inline float PatternEditorRuler::getGridWidth() const {
	return m_fGridWidth;
}
inline int PatternEditorRuler::getWidthActive() const {
	return m_nWidthActive;
}

#endif
