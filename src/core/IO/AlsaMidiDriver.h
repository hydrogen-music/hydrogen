/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef ALSA_MIDI_DRIVER_H
#define ALSA_MIDI_DRIVER_H

#include <core/IO/MidiInput.h>
#include <core/IO/MidiOutput.h>

#if defined(H2CORE_HAVE_ALSA) || _DOXYGEN_

#include <alsa/asoundlib.h>
#include <string>
#include <vector>

namespace H2Core
{

///
/// Alsa Midi Driver
/// Based on Matthias Nagorni alsa sequencer example
///
class AlsaMidiDriver : public virtual MidiInput, public virtual MidiOutput
{
	H2_OBJECT
public:
	AlsaMidiDriver();
	virtual ~AlsaMidiDriver();

	virtual void open();
	virtual void close();
	virtual std::vector<QString> getInputPortList();
	virtual std::vector<QString> getOutputPortList();

	void midi_action( snd_seq_t *seq_handle );
	void getPortInfo( const QString& sPortName, int& nClient, int& nPort );
	virtual void handleQueueNote(Note* pNote);
	
	virtual void handleQueueNoteOff( int channel, int key, int velocity );
	virtual void handleQueueAllNoteOff();
	virtual void handleOutgoingControlChange( int param, int value, int channel );

private:
};

};

#endif // H2CORE_HAVE_ALSA

#endif
