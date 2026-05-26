/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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


#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <chrono>

#include <core/Object.h>

#include <QtGui>
#include <QtWidgets>

// ─────────────────────────────────────────────────────────────────────────────
// StatusLED
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Three-state LED widget for indicating connectivity status:
 * grey = not checked, green = reachable, red = unreachable.
 *
 * \ingroup docGUI docWidgets
 */
class StatusLED : public QWidget, public H2Core::Object<StatusLED> {
	H2_OBJECT( StatusLED )
	Q_OBJECT

   public:
	enum class State {
		Unchecked,	///< Grey — connectivity not yet checked
		Online,		///< Green — source is reachable
		Offline		///< Red — source is unreachable
	};

	StatusLED( QWidget* pParent, const QSize& size );
	~StatusLED() override;

	void setState( State state );
	State getState() const { return m_state; }

   protected:
	void paintEvent( QPaintEvent* ) override;

	State m_state;
};

#endif
