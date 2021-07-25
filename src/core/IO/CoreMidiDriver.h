/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * CoreMidi driver for Hydrogen
 * Copyright(c) 2005-2006 by Jonathan Dempsey
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
 * Some cleanup . . . (20060222 Jonathan Dempsey)
 * Removed some unused code (20060514 Jonathan Dempsey)
 */

#ifndef CORE_MIDI_DRIVER_H
#define CORE_MIDI_DRIVER_H

#include <core/IO/MidiInput.h>
#include <core/IO/MidiOutput.h>

#if defined(H2CORE_HAVE_COREMIDI) || _DOXYGEN_

#include <CoreMidi/CoreMidi.h>

namespace H2Core
{

class CoreMidiDriver : public virtual MidiInput, public virtual MidiOutput
{
	H2_OBJECT
public:
	CoreMidiDriver();
	~CoreMidiDriver();

	bool m_bRunning;

	virtual void open();
	virtual void close();
	virtual std::vector<QString> getInputPortList();
	virtual std::vector<QString> getOutputPortList();

	virtual void handleQueueNote(Note* pNote);
	virtual void handleQueueNoteOff( int channel, int key, int velocity );
	virtual void handleQueueAllNoteOff();
	virtual void handleOutgoingControlChange( int param, int value, int channel );

	MIDIClientRef  h2MIDIClient;
	ItemCount cmSources;
	MIDIEndpointRef cmH2Src;

	MIDIPortRef h2InputRef;
	MIDIPortRef h2OutputRef;
	MIDIEndpointRef cmH2Dst;

	MIDIEndpointRef h2VirtualOut;

private:
	void sendMidiPacket (MIDIPacketList *packetList);
};

}
; // namespace

#endif // H2CORE_HAVE_COREMIDI


#endif

