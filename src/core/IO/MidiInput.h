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

#ifndef H2_MIDI_INPUT_H
#define H2_MIDI_INPUT_H

#include <core/Midi/MidiAction.h>
#include <core/Midi/MidiMessage.h>
#include <core/Object.h>

#include <string>
#include <vector>

namespace H2Core
{

class Instrument;

/**
 * MIDI input base class
 */
/** \ingroup docCore docMIDI */
class MidiInput : public virtual Object<MidiInput>
{
	H2_OBJECT(MidiInput);
public:
		struct HandledInput {
			QTime timestamp;
			MidiMessage::Type type;
			int nData1;
			int nData2;
			int nChannel;

			std::vector<MidiAction::Type> actionTypes;
			QStringList mappedInstruments;
		};

	MidiInput();
	virtual ~MidiInput();

		/** Checks whether input part of the MIDI driver was properly set up and
		 * could receive incoming MIDI events. This does not mean yet that there
		 * is an established connection to another MIDI device. Such a
		 * connection could (depending on the driver and OS) be established
		 * outside of Hydrogen without letting us know. */
		virtual bool isInputActive() const = 0;

		virtual HandledInput handleMessage( const MidiMessage& msg );
		void handleSysexMessage( const MidiMessage& msg,
								 HandledInput& handledInput );
		void handleControlChangeMessage( const MidiMessage& msg,
										 HandledInput& handledInput );
		void handleProgramChangeMessage( const MidiMessage& msg,
										 HandledInput& handledInput );
		void handlePolyphonicKeyPressureMessage( const MidiMessage& msg,
												 HandledInput& handledInput );

protected:
	void handleNoteOnMessage( const MidiMessage& msg, HandledInput& handledInput );
	void handleNoteOffMessage( const MidiMessage& msg, bool CymbalChoke,
							   HandledInput& handledInput );
};

};

#endif

