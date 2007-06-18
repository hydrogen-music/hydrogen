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
 * $Id: AlsaMidiDriver.h,v 1.10 2005/05/01 19:51:40 comix Exp $
 *
 */

#include "config.h"

#ifdef ALSA_SUPPORT


#ifndef ALSA_MIDI_DRIVER_H
#define ALSA_MIDI_DRIVER_H

#include <alsa/asoundlib.h>
#include <string>
#include <vector>

#include "../Globals.h"

#include "MidiDriver.h"

using std::string;
using std::vector;

using namespace std;

///
/// Alsa Midi Driver
/// Based on Matthias Nagorni alsa sequencer example
///
class AlsaMidiDriver : public MidiDriver
{
	public:
		AlsaMidiDriver();
		~AlsaMidiDriver();

		virtual void open();
		virtual void close();
		virtual std::vector<std::string> getOutputPortList();

		void midi_action(snd_seq_t *seq_handle);
		void getPortInfo(const std::string& sPortName, int& nClient, int& nPort);


	private:
		void sysexEvent( unsigned char *midiBuffer, int nBytes );
};

#endif // ALSA_SUPPORT

#endif
