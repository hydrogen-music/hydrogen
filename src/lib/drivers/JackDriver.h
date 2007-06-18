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
 * $Id: JackDriver.h,v 1.7 2005/05/29 17:11:02 comix Exp $
 *
 */

#ifndef JACK_DRIVER_H
#define JACK_DRIVER_H

#include "config.h"
#include "GenericDriver.h"
#include "NullDriver.h"

// check if jack support is enabled
#ifdef JACK_SUPPORT

#include <pthread.h>
#include <jack/jack.h>
#include <jack/transport.h>

#include "../Globals.h"\

///
/// Jack (Jack Audio Connection Kit) server driver
///
class JackDriver : public GenericDriver
{
	public:
		jack_client_t *client;

		JackDriver(JackProcessCallback processCallback);
		~JackDriver();

		int connect();

		void disconnect();

		void deactivate();

		unsigned getBufferSize();

		unsigned getSampleRate();


		jack_transport_state_t getTransportState() {	return m_JackTransportState;	}
		jack_position_t getTransportPos() {	return m_JackTransportPos;	}

		void setTrackOuts(bool flag) { track_out_flag = flag;}
		bool getTrackOuts() { return track_out_flag;}

		void setConnectDefaults(bool flag) { connect_out_flag = flag;}
		bool getConnectDefaults() { return connect_out_flag;}

		float* getOut_L();
		float* getOut_R();
		float* getTrackOut_L(unsigned nTrack);
		float* getTrackOut_R(unsigned nTrack);

		int init(unsigned bufferSize);

		virtual void play();
		virtual void stop();
		virtual void locate( unsigned long nFrame );
		virtual void updateTransportInfo();
		virtual void setBpm(float fBPM);

	private:
		JackProcessCallback processCallback;
		jack_port_t *output_port_1;
		jack_port_t *output_port_2;
		string output_port_name_1;
		string output_port_name_2;
		jack_port_t *track_output_ports_L[MAX_INSTRUMENTS];
		jack_port_t *track_output_ports_R[MAX_INSTRUMENTS];

		jack_transport_state_t m_JackTransportState;
		jack_position_t m_JackTransportPos;

		bool track_out_flag;
		bool connect_out_flag;
};

#else

class JackDriver : public NullDriver
{
	public:
		JackDriver(audioProcessCallback processCallback) : NullDriver( processCallback ) {}

};

#endif // JACK_SUPPORT

#endif

