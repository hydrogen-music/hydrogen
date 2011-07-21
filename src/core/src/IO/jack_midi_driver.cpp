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

#include "JackMidiDriver.h"

#ifdef H2CORE_HAVE_JACK

#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/globals.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>

#ifdef H2CORE_HAVE_LASH
#include <hydrogen/LashClient.h>
#endif

using namespace std;

namespace H2Core
{

const char* JackMidiDriver::__class_name = "JackMidiDriver";

void
JackMidiDriver::lock(void)
{
	pthread_mutex_lock(&mtx);
}

void
JackMidiDriver::unlock(void)
{
	pthread_mutex_unlock(&mtx);
}

void
JackMidiDriver::JackMidiWrite(jack_nframes_t nframes)
{
	int error;
	int events;
	int i;
	void *buf;
	jack_midi_event_t event;
	uint8_t buffer[8];

	if (input_port == NULL)
		return;

	buf = jack_port_get_buffer(input_port, nframes);
	if (buf == NULL)
		return;

#ifdef JACK_MIDI_NEEDS_NFRAMES
	events = jack_midi_get_event_count(buf, nframes);
#else
	events = jack_midi_get_event_count(buf);
#endif

	for (i = 0; i < events; i++) {
		MidiMessage msg;

#ifdef JACK_MIDI_NEEDS_NFRAMES
		error = jack_midi_event_get(&event, buf, i, nframes);
#else
		error = jack_midi_event_get(&event, buf, i);
#endif
		if (error)
			continue;
		if (running < 1)
			continue;

		error = event.size;
		if (error > (int)sizeof(buffer))
			error = (int)sizeof(buffer);

		memset(buffer, 0, sizeof(buffer));
		memcpy(buffer, event.buffer, error);

		switch (buffer[0] >> 4) {
		case 0x8:	 /* note off */
			msg.m_type = MidiMessage::NOTE_OFF;
			msg.m_nData1 = buffer[1];
			msg.m_nData2 = buffer[2];
			msg.m_nChannel = buffer[0] & 0xF;
			handleMidiMessage(msg);
			break;
		case 0x9:	 /* note on */
			msg.m_type = MidiMessage::NOTE_ON;
			msg.m_nData1 = buffer[1];
			msg.m_nData2 = buffer[2];
			msg.m_nChannel = buffer[0] & 0xF;
			handleMidiMessage(msg);
			break;
		case 0xB:	 /* control change */
			msg.m_type = MidiMessage::CONTROL_CHANGE;
			msg.m_nData1 = buffer[1];
			msg.m_nData2 = buffer[2];
			msg.m_nChannel = buffer[0] & 0xF;
			handleMidiMessage(msg);
			break;
		case 0xC:	 /* program change */
			msg.m_type = MidiMessage::PROGRAM_CHANGE;
			msg.m_nData1 = buffer[1];
			msg.m_nData2 = buffer[2];
			msg.m_nChannel = buffer[0] & 0xF;
			handleMidiMessage(msg);
			break;
		case 0xF:
			switch (buffer[0]) {
			case 0xF0:	/* system exclusive */
				break;
			case 0xF1:
				msg.m_type = MidiMessage::QUARTER_FRAME;
				msg.m_nData1 = buffer[1];
				msg.m_nData2 = buffer[2];
				msg.m_nChannel = 0;
				handleMidiMessage(msg);
				break;
			case 0xF2:
				msg.m_type = MidiMessage::SONG_POS;
				msg.m_nData1 = buffer[1];
				msg.m_nData2 = buffer[2];
				msg.m_nChannel = 0;
				handleMidiMessage(msg);
				break;
			case 0xFA:
				msg.m_type = MidiMessage::START;
				msg.m_nData1 = buffer[1];
				msg.m_nData2 = buffer[2];
				msg.m_nChannel = 0;
				handleMidiMessage(msg);
				break;
			case 0xFB:
				msg.m_type = MidiMessage::CONTINUE;
				msg.m_nData1 = buffer[1];
				msg.m_nData2 = buffer[2];
				msg.m_nChannel = 0;
				handleMidiMessage(msg);
				break;
			case 0xFC:
				msg.m_type = MidiMessage::STOP;
				msg.m_nData1 = buffer[1];
				msg.m_nData2 = buffer[2];
				msg.m_nChannel = 0;
				handleMidiMessage(msg);
				break;
			default:
				break;
			}
		default:
			break;
		}
	}
}

void
JackMidiDriver::JackMidiRead(jack_nframes_t nframes)
{
	uint8_t *buffer;
	void *buf;
	jack_nframes_t t;
	uint8_t data[1];
	uint8_t len;

	if (output_port == NULL)
		return;

	buf = jack_port_get_buffer(output_port, nframes);
	if (buf == NULL)
		return;

#ifdef JACK_MIDI_NEEDS_NFRAMES
	jack_midi_clear_buffer(buf, nframes);
#else
	jack_midi_clear_buffer(buf);
#endif

	t = 0;
	lock();
	while ((t < nframes) &&
	       (rx_out_pos != rx_in_pos)) {

		len = buffer[4 * rx_in_pos];
		if (len == 0) {
			rx_in_pos++;
			if (rx_in_pos >= JACK_MIDI_BUFFER_MAX)
				rx_in_pos = 0;
			continue;
		}

#ifdef JACK_MIDI_NEEDS_NFRAMES
		buffer = jack_midi_event_reserve(buf, t, len, nframes);
#else
		buffer = jack_midi_event_reserve(buf, t, len);
#endif
		if (buffer == NULL)
			break;

		memcpy(buffer, buffer + (4 * rx_in_pos) + 1, len);
		t++;
		rx_in_pos++;
		if (rx_in_pos >= JACK_MIDI_BUFFER_MAX)
			rx_in_pos = 0;
	}
	unlock();
}

void
JackMidiDriver::JackMidiOutEvent(uint8_t buf[4], uint8_t len)
{
	uint32_t next_pos;

	lock();

	next_pos = rx_out_pos + 1;
	if (next_pos >= JACK_MIDI_BUFFER_MAX)
		next_pos = 0;

	if (next_pos == rx_in_pos) {
		/* buffer is full */
		unlock();
		return;
	}

	if (len > 3)
		len = 3;

	buffer[(4 * next_pos)] = len;
	buffer[(4 * next_pos) + 1] = buf[0];
	buffer[(4 * next_pos) + 2] = buf[1];
	buffer[(4 * next_pos) + 3] = buf[2];

	rx_out_pos = next_pos;

	unlock();
}

static int
JackMidiProcessCallback(jack_nframes_t nframes, void *arg)
{
	JackMidiDriver *jmd = (JackMidiDriver *)arg;

	if (nframes <= 0)
		return (0);

	jmd->JackMidiRead(nframes);
	jmd->JackMidiWrite(nframes);

	return (0);
}

static void
JackMidiShutdown(void *arg)
{
	UNUSED(arg);
	Hydrogen::get_instance()->raiseError( Hydrogen::JACK_SERVER_SHUTDOWN );
}

JackMidiDriver::JackMidiDriver()
    : MidiInput( __class_name ), MidiOutput( __class_name ), Object( __class_name )
{
	pthread_mutex_init(&mtx, NULL);

	running = 0;
	rx_in_pos = 0;
	rx_out_pos = 0;
	output_port = 0;
	input_port = 0;

	jack_client = jack_client_open("hydrogen-midi",
	    JackNoStartServer, NULL);

	if (jack_client == NULL)
		return;

	jack_set_process_callback(jack_client,
	    JackMidiProcessCallback, this);

	jack_on_shutdown(jack_client,
	    JackMidiShutdown, 0);

	output_port = jack_port_register(
	    jack_client, "TX", JACK_DEFAULT_MIDI_TYPE,
	    JackPortIsOutput, 0);

	input_port = jack_port_register(
	    jack_client, "RX", JACK_DEFAULT_MIDI_TYPE,
	    JackPortIsInput, 0);

	jack_activate(jack_client);
}

JackMidiDriver::~JackMidiDriver()
{
	if (jack_client != NULL)
		jack_deactivate(jack_client);

	pthread_mutex_destroy(&mtx);
}

void
JackMidiDriver::open()
{
	running ++;
}

void
JackMidiDriver::close()
{
	running --;
}

std::vector<QString>
JackMidiDriver::getOutputPortList()
{
	vector<QString> outputList;

	outputList.push_back("Default");

	return outputList;
}

void
JackMidiDriver::getPortInfo(const QString& sPortName, int& nClient, int& nPort)
{
	if (sPortName == "None") {
		nClient = -1;
		nPort = -1;
		return;
	}

	nClient = 0;
	nPort = 0;
}

void JackMidiDriver::handleQueueNote(Note* pNote)
{

	uint8_t buffer[4];
	int channel;
	int key;
	int vel;

	channel = pNote->get_instrument()->get_midi_out_channel();
	if (channel < 0 || channel > 15)
		return;

	key = (pNote->get_octave() +3 ) * 12 + pNote->get_key() + pNote->get_instrument()->get_midi_out_note() - 60;
	if (key < 0 || key > 127)
		return;

	vel = pNote->get_midi_velocity();
	if (vel < 0 || vel > 127)
		return;

	buffer[0] = 0x90 | channel;	/* note on */
	buffer[1] = key;
	buffer[2] = vel;
	buffer[3] = 0;

	JackMidiOutEvent(buffer, 3);
}

void
JackMidiDriver::handleQueueNoteOff(int channel, int key, int vel)
{
	uint8_t buffer[4];

	if (channel < 0 || channel > 15)
		return;
	if (key < 0 || key > 127)
		return;
	if (vel < 0 || vel > 127)
		return;

	buffer[0] = 0x90 | channel;	/* note on */
	buffer[1] = key;
	buffer[2] = 0;
	buffer[3] = 0;

	JackMidiOutEvent(buffer, 3);
}

void JackMidiDriver::handleQueueAllNoteOff()
{
	InstrumentList *instList = Hydrogen::get_instance()->getSong()->get_instrument_list();
	Instrument *curInst;
	unsigned int numInstruments = instList->size();
	unsigned int i;
	int channel;
	int key;

	for (i = 0; i < numInstruments; i++) {
		curInst = instList->get(i);
	
		channel = curInst->get_midi_out_channel();
		if (channel < 0 || channel > 15)
			continue;
		key = curInst->get_midi_out_note();
		if (key < 0 || key > 127)
			continue;

		handleQueueNoteOff(channel, key, 0);	
	}
}

};

#endif			/* H2CORE_HAVE_JACK */

