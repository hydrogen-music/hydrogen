/*
 * Hydrogen
 * Copyright(c) 2002-2006 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#ifndef PORT_MIDI_DRIVER_H
#define PORT_MIDI_DRIVER_H

#ifdef PORTMIDI_SUPPORT

#include <hydrogen/IO/MidiInput.h>
#include <portmidi.h>

namespace H2Core {

class PortMidiDriver : public MidiInput
{
	public:
		PmStream *m_pMidiIn;
		bool m_bRunning;

		PortMidiDriver();
		virtual ~PortMidiDriver();

		virtual void open();
		virtual void close();
		virtual std::vector<std::string> getOutputPortList();

	private:

};

};

#endif

#endif

