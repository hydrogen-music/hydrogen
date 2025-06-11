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

#include <QtGui>
#include <QtWidgets>
#include <chrono>

#include <core/Object.h>
#include "Widgets/Button.h"
#include "Widgets/WidgetWithScalableFont.h"

class LED;

/** Button in the #PlayerControl indicating the current state of the MIDI input
 * and output (using two LEDs). Clicking it will open the #MidiControlDialog.
 *
 * \ingroup docGUI*/
class MidiControlButton : public Button,
						  protected WidgetWithScalableFont<5, 6, 7>,
						  public H2Core::Object<MidiControlButton> {
		H2_OBJECT(MidiControlButton)
		Q_OBJECT

public:
		static constexpr std::chrono::milliseconds midiActivityTimeout{ 125 };
		static constexpr int nLEDHeight = 9;
		static constexpr int nLEDWidth = 11;

		explicit MidiControlButton( QWidget* pParent );
		~MidiControlButton();

		/** Activates the LED for a predefined time interval. */
		void flashMidiInputLED();
		/** Activates the LED for a predefined time interval. */
		void flashMidiOutputLED();

		/** Enables or disables the input and output LEDs based on the current
		 * preferences. */
		void updateActivation();

		// EventListerer
		void driverChangedEvent() override;
		void midiActivityEvent() override;

private slots:
		void deactivateMidiInputLED();
		void deactivateMidiOutputLED();

private:
		void paintEvent( QPaintEvent* pEvent ) override;

		LED* m_pMidiInputLED;
		LED* m_pMidiOutputLED;

		QLabel* m_pLabel;
		QTimer* m_pMidiInputTimer;
		QTimer* m_pMidiOutputTimer;
};


#endif
