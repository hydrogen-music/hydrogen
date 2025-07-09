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

#ifndef PORT_MIDI_DRIVER_H
#define PORT_MIDI_DRIVER_H

#include <core/IO/MidiInput.h>
#include <core/IO/MidiOutput.h>

#include <memory>

#if defined(H2CORE_HAVE_PORTMIDI) || _DOXYGEN_
#include <portmidi.h>

namespace H2Core
{

	class Note;

/** \ingroup docCore docMIDI */
class PortMidiDriver : public Object<PortMidiDriver>,
					   public virtual MidiInput,
					   public virtual MidiOutput
{
	H2_OBJECT(PortMidiDriver)
public:
	PmStream *m_pMidiIn;
	PmStream *m_pMidiOut;
	bool m_bRunning;

	PortMidiDriver();
	virtual ~PortMidiDriver();

	virtual void open() override;
	virtual void close() override;
	virtual std::vector<QString> getInputPortList() override;
	virtual std::vector<QString> getOutputPortList() override;

	virtual void handleQueueNote( const MidiMessage& msg ) override;
	virtual void handleQueueNoteOff( int channel, int key, int velocity ) override;
	virtual void handleQueueAllNoteOff() override;
	virtual void handleOutgoingControlChange( int param, int value, int channel ) override;

	static QString translatePmError( const PmError& err );
	/**
	 * Appends the content of @a msg to #MidiMessage::m_sysexData of
	 * @a pMidiMessage till 247 (EOX - end of exclusion) is
	 * encountered.
	 *
	 * @returns `true` - in case all SysEx data is gathered and @a
	 *   pMidiMessage can be considered complete or `false` if there
	 *   is still pending data.
	 */
	static bool appendSysExData( MidiMessage* pMidiMessage, const PmMessage& msg );

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;
private:
	int m_nVirtualInputDeviceId;
	int m_nVirtualOutputDeviceId;
};

};

#endif

#endif

