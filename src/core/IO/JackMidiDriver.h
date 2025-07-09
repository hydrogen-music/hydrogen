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

#include <core/IO/MidiInput.h>
#include <core/IO/MidiOutput.h>

#if defined(H2CORE_HAVE_JACK) || _DOXYGEN_

#include <pthread.h>

#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>

#include <string>
#include <vector>
#include <memory>

#define	JACK_MIDI_BUFFER_MAX 64	/* events */

namespace H2Core
{

/** \ingroup docCore docMIDI */
class JackMidiDriver : public Object<JackMidiDriver>,
					   public virtual MidiInput,
					   public virtual MidiOutput
{
	H2_OBJECT(JackMidiDriver)
public:
	JackMidiDriver();
	virtual ~JackMidiDriver();

	virtual void open() override;
	virtual void close() override;
	virtual std::vector<QString> getInputPortList() override;
	virtual std::vector<QString> getOutputPortList() override;

	void getPortInfo( const QString& sPortName, int& nClient, int& nPort );
	void JackMidiWrite(jack_nframes_t nframes);
	void JackMidiRead(jack_nframes_t nframes);
	
	virtual void handleQueueAllNoteOff() override;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;
private:
	void JackMidiOutEvent(uint8_t *buf, uint8_t len);

	void sendControlChangeMessage( const MidiMessage& msg ) override;
	void sendNoteOnMessage( const MidiMessage& msg ) override;
	void sendNoteOffMessage( const MidiMessage& msg ) override;

	void lock();
	void unlock();

	jack_port_t *output_port;
	jack_port_t *input_port;
	jack_client_t *jack_client;
	pthread_mutex_t mtx;
	int running;
		uint8_t jack_buffer[JACK_MIDI_BUFFER_MAX * 4];
	uint32_t rx_in_pos;
	uint32_t rx_out_pos;
};

};

#endif			/* H2CORE_HAVE_JACK */

#endif
