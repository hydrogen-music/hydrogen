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

#ifndef H2_MIDI_OUTPUT_H
#define H2_MIDI_OUTPUT_H

#include <core/Object.h>
#include <string>
#include <vector>
#include <memory>

namespace H2Core {

class MidiMessage;

/**
 * MIDI input base class
 */
/** \ingroup docCore docMIDI */
class MidiOutput : public virtual Object<MidiOutput>
{
	H2_OBJECT(MidiOutput)

	public:
		MidiOutput();
		virtual ~MidiOutput();
	
		virtual std::vector<QString> getInputPortList() = 0;

		void sendMessage( const MidiMessage& msg );

	private:
		virtual void sendControlChangeMessage( const MidiMessage& msg ) = 0;
		virtual void sendNoteOffMessage( const MidiMessage& msg ) = 0;
		virtual void sendNoteOnMessage( const MidiMessage& msg ) = 0;
};

};

#endif

