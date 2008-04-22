/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#ifndef H2_JACK_OUTPUT_H
#define H2_JACK_OUTPUT_H

#include <hydrogen/IO/AudioOutput.h>
#include <hydrogen/IO/NullDriver.h>

// check if jack support is enabled
#ifdef JACK_SUPPORT

#include <pthread.h>
#include <jack/jack.h>
#include <jack/transport.h>

#include <hydrogen/globals.h>


namespace H2Core
{

class Song;
class Instrument;

///
/// Jack (Jack Audio Connection Kit) server driver.
///
class JackOutput : public AudioOutput
{
public:
	jack_client_t *client;

	JackOutput( JackProcessCallback processCallback );
	~JackOutput();

	int connect();
	void disconnect();
	void deactivate();
	unsigned getBufferSize();
	unsigned getSampleRate();
	int getNumTracks();

	jack_transport_state_t getTransportState() {
		return m_JackTransportState;
	}
	jack_position_t getTransportPos() {
		return m_JackTransportPos;
	}

	void setPortName( int nPort, bool bLeftChannel, const QString& sName );
	void makeTrackOutputs( Song * );
	void setTrackOutput( int, Instrument * );

	void setConnectDefaults( bool flag ) {
		connect_out_flag = flag;
	}
	bool getConnectDefaults() {
		return connect_out_flag;
	}

	float* getOut_L();
	float* getOut_R();
	float* getTrackOut_L( unsigned nTrack );
	float* getTrackOut_R( unsigned nTrack );

	int init( unsigned bufferSize );

	virtual void play();
	virtual void stop();
	virtual void locate( unsigned long nFrame );
	virtual void updateTransportInfo();
	virtual void setBpm( float fBPM );
	void calculateFrameOffset();

private:
	void relocateBBT();
	long long bbt_frame_offset;
	int must_relocate;

	JackProcessCallback processCallback;
	jack_port_t *output_port_1;
	jack_port_t *output_port_2;
	QString output_port_name_1;
	QString output_port_name_2;
	int track_port_count;
	jack_port_t *track_output_ports_L[MAX_INSTRUMENTS];
	jack_port_t *track_output_ports_R[MAX_INSTRUMENTS];

	jack_transport_state_t m_JackTransportState;
	jack_position_t m_JackTransportPos;

	bool connect_out_flag;
};

#else

namespace H2Core {

class JackOutput : public NullDriver
{
public:
	JackOutput( audioProcessCallback processCallback ) : NullDriver( processCallback ) {}

};


#endif // JACK_SUPPORT

};

#endif

