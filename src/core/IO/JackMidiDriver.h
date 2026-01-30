/*-
 * Copyright (c) 2011 Hans Petter Selasky <hselasky@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef JACK_MIDI_DRIVER_H
#define JACK_MIDI_DRIVER_H

#include <core/IO/MidiBaseDriver.h>

#if defined( H2CORE_HAVE_JACK ) || _DOXYGEN_

#include <pthread.h>

#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>

#include <vector>

#define JACK_MIDI_BUFFER_MAX 64 /* events */

namespace H2Core {

/** \ingroup docCore docMIDI */
class JackMidiDriver : public Object<JackMidiDriver>,
					   public virtual MidiBaseDriver {
	H2_OBJECT( JackMidiDriver )
   public:
	JackMidiDriver();
	virtual ~JackMidiDriver();

	void close() override;
	std::vector<QString> getExternalPortList( const PortType& portType
	) override;
	bool isInputActive() const override;
	bool isOutputActive() const override;
	void open() override;

	void getPortInfo( const QString& sPortName, int& nClient, int& nPort );
	void readJackMidi( jack_nframes_t nframes );
	void writeJackMidi( jack_nframes_t nframes );

	QString toQString( const QString& sPrefix = "", bool bShort = true )
		const override;

   private:
	void jackMidiOutEvent( uint8_t* buf, uint8_t len );

	void sendControlChangeMessage( const MidiMessage& msg ) override;
	void sendNoteOnMessage( const MidiMessage& msg ) override;
	void sendNoteOffMessage( const MidiMessage& msg ) override;
	void sendSystemRealTimeMessage( const MidiMessage& msg ) override;

	void lockMidiPart();
	void unlockMidiPart();

	jack_port_t* m_pMidiOutputPort;
	jack_port_t* m_pMidiInputPort;
	jack_client_t* m_pJackClient;
	pthread_mutex_t m_midiMutex;
	int m_nRunning;
	uint8_t m_jackMidiBuffer[JACK_MIDI_BUFFER_MAX * 4];
	uint32_t m_midiRxInPosition;
	uint32_t m_midiRxOutPosition;
};

};	// namespace H2Core

#endif /* H2CORE_HAVE_JACK */

#endif
