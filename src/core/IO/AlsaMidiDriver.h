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

#ifndef ALSA_MIDI_DRIVER_H
#define ALSA_MIDI_DRIVER_H

#include <core/IO/MidiBaseDriver.h>

#if defined(H2CORE_HAVE_ALSA) || _DOXYGEN_

#include <alsa/asoundlib.h>
#include <string>
#include <vector>
#include <memory>

namespace H2Core
{

///
/// Alsa Midi Driver
/// Based on Matthias Nagorni alsa sequencer example
///
/** \ingroup docCore docMIDI */
class AlsaMidiDriver : public Object<AlsaMidiDriver>,
					   public virtual MidiBaseDriver
{
	H2_OBJECT(AlsaMidiDriver)
public:
	AlsaMidiDriver();
	virtual ~AlsaMidiDriver();

	void close() override;
	std::vector<QString> getExternalPortList( const PortType& portType ) override;
	void open() override;

	void midi_action( snd_seq_t *seq_handle );
	void getPortInfo( const QString& sPortName, int& nClient, int& nPort );

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
	void sendControlChangeMessage( const MidiMessage& msg ) override;
	void sendNoteOnMessage( const MidiMessage& msg ) override;
	void sendNoteOffMessage( const MidiMessage& msg ) override;

		std::vector<QString> getPortList( int nCapability );
};

};

#endif // H2CORE_HAVE_ALSA

#endif
