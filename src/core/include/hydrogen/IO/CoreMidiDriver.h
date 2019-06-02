/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Some cleanup . . . (20060222 Jonathan Dempsey)
 * Removed some unused code (20060514 Jonathan Dempsey)
 */

#ifndef CORE_MIDI_DRIVER_H
#define CORE_MIDI_DRIVER_H

#include <hydrogen/IO/MidiInput.h>
#include <hydrogen/IO/MidiOutput.h>

#if defined(H2CORE_HAVE_COREMIDI) || _DOXYGEN_

#include <CoreMidi/CoreMidi.h>

namespace H2Core
{

/** \ingroup docCore docAudioDriver docMIDI*/
class CoreMidiDriver : public virtual MidiInput, public virtual MidiOutput
{
public:
	/** \return #m_sClassName*/
	static const char* className() { return m_sClassName; }
	CoreMidiDriver();
	~CoreMidiDriver();

	bool m_bRunning;

	virtual void open();
	virtual void close();
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
	/** Contains the name of the class.
	 *
	 * This variable allows from more informative log messages
	 * with the name of the class the message is generated in
	 * being displayed as well. Queried using className().*/
	static const char* m_sClassName;
	void sendMidiPacket (MIDIPacketList *packetList);
};

}
; // namespace

#endif // H2CORE_HAVE_COREMIDI


#endif

