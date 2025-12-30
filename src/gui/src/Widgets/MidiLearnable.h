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

#include <core/Midi/MidiEvent.h>

#include "../EventListener.h"

class MidiAction;

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

    void setMidiAction( std::shared_ptr<MidiAction> pMidiAction );

    std::shared_ptr<MidiAction> getMidiAction() const {
		return m_pMidiAction;
    }

		const QString& getBaseToolTip() const;
		void setBaseToolTip( const QString& sNewTip );

	/**
	 * Update #m_registeredMidiEvents since the underlying
	 * #H2Core::MidiEventMap changed.
	 */
	void midiMapChangedEvent() override;

protected:
		/** Create the resulting tool tip by combining #m_sBaseToolTip with some
		 * formatting, stock strings and current MIDI assignments. */
		QString composeToolTip() const;
		/** To be implemented by the child widget to set the result of
		 * #composeToolTip().
		 */
		virtual void updateToolTip() = 0;

		QString m_sBaseToolTip;

    std::shared_ptr<MidiAction> m_pMidiAction;

	/**
	 * Stores all MIDI events mapped to #m_pAction.
	 *
	 * It consists of pairs of MIDI events and the associated MIDI
	 * parameter.
	 */ 
	std::vector<std::pair<H2Core::MidiEvent::Type,int>> m_registeredMidiEvents;
};

inline const QString& MidiLearnable::getBaseToolTip() const {
	return m_sBaseToolTip;
}

#endif // MIDILEARNABLE_H
