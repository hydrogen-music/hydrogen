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


#ifndef MIDILEARNABLE_H
#define MIDILEARNABLE_H

#include <memory>
#include <vector>
#include <QString>
#include <core/MidiAction.h>
#include <core/IO/MidiCommon.h>

#include "../EventListener.h"

/**
 * Every widget which supports MidiLearn should derive from this
 * Class.
 *
 * MIDI-learnable widgets serve as a more convenient interface to the
 * #MidiTable provided in the #PreferencesDialog. Instead of
 * assigning a MIDI message type and parameter e.g. STRIP_SOLO_TOGGLE
 * and specify the strip number to 2, one can also SHIFT - left click
 * the solo button of the second strip and send the MIDI event.
 *
 * \ingroup docGUI docWidgets docMIDI
 */
class MidiLearnable : public EventListener
{
public:
    MidiLearnable();
	~MidiLearnable();

    void setAction( std::shared_ptr<Action> pAction );

    std::shared_ptr<Action> getAction() const {
		return m_pAction;
    }

	/**
	 * Update #m_registeredMidiEvents since the underlying
	 * #H2Core::MidiMap changed.
	 */
	void midiMapChangedEvent() override;

	/**
	 * Indicates child class to recalculate its tool tip in case
	 * #m_registeredMidiEvents changed.
	 */
	virtual void updateTooltip(){};

protected:
    std::shared_ptr<Action> m_pAction;

	/**
	 * Stores all MIDI events mapped to #m_pAction.
	 *
	 * It consists of pairs of MIDI events and the associated MIDI
	 * parameter.
	 */ 
	std::vector<std::pair<H2Core::MidiMessage::Event,int>> m_registeredMidiEvents;
};

#endif // MIDILEARNABLE_H
