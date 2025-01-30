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

#include <core/Object.h>
#include <string>
#include <vector>
#include "MidiCommon.h"

namespace H2Core
{

/**
 * MIDI input base class
 */
/** \ingroup docCore docMIDI */
class MidiInput : public virtual Object<MidiInput>
{
	H2_OBJECT(MidiInput);
public:
	MidiInput();
	virtual ~MidiInput();

	virtual void open() = 0;
	virtual void close() = 0;
	virtual std::vector<QString> getOutputPortList() = 0;

	void setActive( bool isActive ) {
		m_bActive = isActive;
	}
	void handleMidiMessage( const MidiMessage& msg );
	void handleSysexMessage( const MidiMessage& msg );
	void handleControlChangeMessage( const MidiMessage& msg );
	void handleProgramChangeMessage( const MidiMessage& msg );
	void handlePolyphonicKeyPressureMessage( const MidiMessage& msg );

protected:
	bool m_bActive;

	void handleNoteOnMessage( const MidiMessage& msg );
	void handleNoteOffMessage( const MidiMessage& msg, bool CymbalChoke );
};

};

#endif

