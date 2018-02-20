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
#ifdef H2CORE_HAVE_JACK

#include <map>
#include <pthread.h>
#include <jack/jack.h>

#ifdef H2CORE_HAVE_JACKSESSION
#include <jack/session.h>
#endif

#include <jack/transport.h>

#include <hydrogen/globals.h>
#include <hydrogen/hydrogen.h>



namespace H2Core
{

class Song;
class Instrument;
class InstrumentComponent;

///
/// Jack (Jack Audio Connection Kit) server driver.
///
class JackAudioDriver : public AudioOutput
{
	H2_OBJECT
public:
	jack_client_t *m_pClient;

	JackAudioDriver( JackProcessCallback processCallback );
	~JackAudioDriver();

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


	void makeTrackOutputs( Song * );
	void setTrackOutput( int, Instrument *, InstrumentComponent *, Song * );

	void setConnectDefaults( bool flag ) {
		m_bConnectOutFlag = flag;
	}
	bool getConnectDefaults() {
		return m_bConnectOutFlag;
	}

	float* getOut_L();
	float* getOut_R();
	float* getTrackOut_L( unsigned nTrack );
	float* getTrackOut_R( unsigned nTrack );
	float* getTrackOut_L( Instrument *, InstrumentComponent * );
    float* getTrackOut_R( Instrument *, InstrumentComponent * );

	int init( unsigned bufferSize );

	virtual void play();
	virtual void stop();
	virtual void locate( unsigned long nFrame );
	virtual void updateTransportInfo();
	virtual void setBpm( float fBPM );
	void calculateFrameOffset();
	void locateInNCycles( unsigned long frame, int cycles_to_wait = 2 );

//jack timebase callback
	void initTimeMaster();
	void com_release();
//~ jack timebase callback

protected:
//jack timebase callback
	static void jack_timebase_callback(jack_transport_state_t state,
										jack_nframes_t nframes,
										jack_position_t *pos,
										int new_pos,
										void *arg);
//~ jack timebase callback

#ifdef H2CORE_HAVE_JACKSESSION
		static void jack_session_callback(jack_session_event_t *event, void *arg);

		void jack_session_callback_impl(jack_session_event_t *event);
#endif

private:
	void relocateBBT();


	H2Core::Hydrogen *		m_pEngine;

	long long				bbt_frame_offset;
	int						must_relocate;		// A countdown to wait for valid information from another Time Master.
	int						locate_countdown;	// (Unrelated) countdown, for postponing a call to 'locate'.
	unsigned long			locate_frame;		// The frame to locate to (used in 'locateInNCycles'.)

	JackProcessCallback		processCallback;
	jack_port_t *			output_port_1;
	jack_port_t *			output_port_2;
	QString					output_port_name_1;
	QString					output_port_name_2;
	int						track_map[MAX_INSTRUMENTS][MAX_COMPONENTS];
	int						track_port_count;
	jack_port_t *			track_output_ports_L[MAX_INSTRUMENTS];
	jack_port_t *			track_output_ports_R[MAX_INSTRUMENTS];

	jack_transport_state_t	m_JackTransportState;
	jack_position_t			m_JackTransportPos;

	bool					m_bConnectOutFlag;

//jack timebase callback
	bool					m_bCond;
//~ jack timebase callback

};

#else

namespace H2Core {

class JackAudioDriver : public NullDriver
{
	H2_OBJECT
public:
	JackAudioDriver( audioProcessCallback processCallback ) : NullDriver( processCallback ) {}

};


#endif // H2CORE_HAVE_JACK

};

#endif

