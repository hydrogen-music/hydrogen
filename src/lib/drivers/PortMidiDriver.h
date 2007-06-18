/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: PortMidiDriver.h,v 1.7 2005/05/01 19:51:40 comix Exp $
 *
 */

#ifndef PORT_MIDI_DRIVER_H
#define PORT_MIDI_DRIVER_H

#include "config.h"

#ifdef PORTMIDI_SUPPORT

#include "MidiDriver.h"

#include <portmidi.h>

class PortMidiDriver : public MidiDriver
{
	public:
		PmStream *m_pMidiIn;
		bool m_bRunning;
		
		PortMidiDriver();
		~PortMidiDriver();
		
		virtual void open();
		virtual void close();
		virtual std::vector<std::string> getOutputPortList();

	private:

};

#endif

#endif

