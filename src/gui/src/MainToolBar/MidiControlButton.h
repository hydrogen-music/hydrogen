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

#ifndef MIDI_CONTROL_BUTTON_H
#define MIDI_CONTROL_BUTTON_H

#include <QSvgRenderer>
#include <QtGui>
#include <QtWidgets>
#include <chrono>

#include <core/Object.h>

#include "../EventListener.h"
#include "../Widgets/WidgetWithScalableFont.h"

//class LED;

/** Button in the #MainToolBar indicating the current state of the MIDI input
 * and output (using two LEDs). Clicking it will open the #MidiControlDialog.
 *
 * \ingroup docGUI*/
class MidiControlButton : public QToolButton,
						  public EventListener,
						  protected WidgetWithScalableFont<5, 6, 7>,
						  public H2Core::Object<MidiControlButton> {
		H2_OBJECT(MidiControlButton)
		Q_OBJECT

public:
		static constexpr std::chrono::milliseconds midiActivityTimeout{ 125 };
		static constexpr int nIconWidth = 17;
		static constexpr int nLogoWidth = 55;

		explicit MidiControlButton( QWidget* pParent );
		~MidiControlButton();

		/** Activates icon for MIDI input for a predefined time interval. */
		void flashMidiInputIcon();
		/** Activates icon for MIDI input for a predefined time interval. */
		void flashMidiOutputIcon();

		/** Enables or disables the input and output symbols based on the
		 * current preferences. */
		void updateActivation();
		void updateIcons();

		// EventListerer
		void driverChangedEvent() override;
		void midiInputEvent() override;

private:
		void paintEvent( QPaintEvent* pEvent ) override;

		bool m_bMidiInputActive;
		bool m_bMidiOutputActive;

		QSvgRenderer* m_pIconInputSvg;
		QSvgRenderer* m_pMidiLogoSvg;
		QSvgRenderer* m_pIconOutputSvg;
		QTimer* m_pMidiInputTimer;
		QTimer* m_pMidiOutputTimer;
};


#endif
