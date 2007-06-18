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
 * $Id: JackDriver.cpp,v 1.12 2005/05/29 17:12:27 comix Exp $
 *
 */

#include "JackDriver.h"
#ifdef JACK_SUPPORT

#include <sys/types.h>
#include <unistd.h>
#include "lib/Hydrogen.h"
#include "lib/Preferences.h"

unsigned long jack_server_sampleRate = 0;
jack_nframes_t jack_server_bufferSize = 0;
JackDriver *jackDriverInstance = NULL;

int jackDriverSampleRate(jack_nframes_t nframes, void *arg) {
	char msg[100];
	sprintf( msg, "Jack SampleRate changed: the sample rate is now %d/sec", (int)nframes );
	jackDriverInstance->infoLog(msg);
	jack_server_sampleRate = nframes;
	return 0;
}




void jackDriverShutdown(void *arg) {
//	jackDriverInstance->deactivate();
	jackDriverInstance->client = NULL;
	( Hydrogen::getInstance() )->raiseError( Hydrogen::JACK_SERVER_SHUTDOWN );
}




JackDriver::JackDriver(JackProcessCallback processCallback)
 : GenericDriver( "JackDriver" )
{
	infoLog( "INIT" );
	jackDriverInstance = this;
	track_out_flag = true;
	this->processCallback = processCallback;
}





JackDriver::~JackDriver()
{
	infoLog( "DESTROY" );
	disconnect();
}






// return 0: ok
// return 1: cannot activate client
// return 2: cannot connect output port
// return 3: Jack server not running
// return 4: output port = NULL
int JackDriver::connect()
{
	infoLog("[connect]");

	if (jack_activate (client)) {
		( Hydrogen::getInstance() )->raiseError( Hydrogen::JACK_CANNOT_ACTIVATE_CLIENT );
		return 1;
	}

	if ( connect_out_flag ) {
	// connect the ports
		if ( jack_connect ( client, jack_port_name( output_port_1 ), output_port_name_1.c_str() ) ) {
			( Hydrogen::getInstance() )->raiseError( Hydrogen::JACK_CANNOT_CONNECT_OUTPUT_PORT );
			return 2;
		}
		if ( jack_connect ( client, jack_port_name( output_port_2 ), output_port_name_2.c_str() ) ) {
			( Hydrogen::getInstance() )->raiseError( Hydrogen::JACK_CANNOT_CONNECT_OUTPUT_PORT );
			return 2;
		}
	}

	return 0;
}





void JackDriver::disconnect()
{
	infoLog( "[disconnect]" );

	deactivate();
	jack_client_t *oldClient = client;
	client = NULL;
	if ( oldClient ) {
		infoLog( "calling jack_client_close" );
		int res = jack_client_close( oldClient );
		if ( res ) {
			errorLog("Error in jack_client_close");
			// FIXME: raise exception
		}
	}
	client = NULL;
}




void JackDriver::deactivate()
{
	infoLog( "[deactivate]" );
	if ( client ) {
		infoLog( "jack_deactivate" );
		int res = jack_deactivate( client );
		if ( res ) {
			errorLog("Error in jack_deactivate");
		}
	}
}




unsigned JackDriver::getBufferSize()
{
	return jack_server_bufferSize;
}



unsigned JackDriver::getSampleRate()
{
	return jack_server_sampleRate;
}




void JackDriver::updateTransportInfo()
{
	if (Preferences::getInstance()->m_bJackTransportMode ==  Preferences::USE_JACK_TRANSPORT) {
		m_JackTransportState = jack_transport_query( client, &m_JackTransportPos );

		// update m_transport with jack-transport data
		switch ( m_JackTransportState ) {
			case JackTransportStopped:
				m_transport.m_status = TransportInfo::STOPPED;
				//infoLog( "[updateTransportInfo] STOPPED - frames: " + toString(m_transportPos.frame) );
				break;

			case JackTransportRolling:
				m_transport.m_status = TransportInfo::ROLLING;
				//infoLog( "[updateTransportInfo] ROLLING - frames: " + toString(m_transportPos.frame) );
				break;

			case JackTransportStarting:
				m_transport.m_status = TransportInfo::STOPPED;
				//infoLog( "[updateTransportInfo] STARTING (stopped) - frames: " + toString(m_transportPos.frame) );
				break;

			default:
				errorLog( "[updateTransportInfo] Unknown jack transport state" );
		}


/*
		// FIXME
		// TickSize and BPM
		if ( m_JackTransportPos.valid & JackPositionBBT ) {
			if ( m_transport.m_nBPM != m_JackTransportPos.beats_per_minute ) {
				m_transport.m_nBPM = m_JackTransportPos.beats_per_minute;
				(Hydrogen::getInstance())->getSong()->m_fBPM = m_JackTransportPos.beats_per_minute;
				warningLog( "[updateTransportInfo] new BPM from jack-transport: " + toString(m_JackTransportPos.beats_per_minute) );
			}
		}
*/

		if ( m_transport.m_nFrames != m_JackTransportPos.frame ) {
			m_transport.m_nFrames = m_JackTransportPos.frame;
			warningLog( "[updateTransportInfo] internal frames != jack transport frames. resync!" );
		}
	}
}



float* JackDriver::getOut_L()
{
	jack_default_audio_sample_t *out = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port_1, jack_server_bufferSize);
	return out;
}

float* JackDriver::getOut_R()
{
	jack_default_audio_sample_t *out = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port_2, jack_server_bufferSize);
	return out;
}



float* JackDriver::getTrackOut_L(unsigned nTrack)
{
	jack_default_audio_sample_t *out = (jack_default_audio_sample_t *) jack_port_get_buffer (track_output_ports_L[nTrack], jack_server_bufferSize);
	return out;
}

float* JackDriver::getTrackOut_R(unsigned nTrack)
{
	jack_default_audio_sample_t *out = (jack_default_audio_sample_t *) jack_port_get_buffer (track_output_ports_R[nTrack], jack_server_bufferSize);
	return out;
}


int JackDriver::init(unsigned nBufferSize)
{
	output_port_name_1 = Preferences::getInstance()->m_sJackPortName1;
	output_port_name_2 = Preferences::getInstance()->m_sJackPortName2;

	// check if another hydrogen instance is connected to jack
	if ( (client = jack_client_new( "hydrogen-tmp" ) ) == 0 ) {
		warningLog("Jack server not running?");
		return 3;
	}

	vector<string> clientList;
	const char **readports = jack_get_ports( client, NULL, NULL, JackPortIsOutput );
	int nPort = 0;
	while (readports && readports[nPort]) {
		string sPort = string(readports[nPort]);
		int nColonPos = sPort.find( ":" );
		string sClient = sPort.substr( 0, nColonPos );
		bool bClientExist = false;
		for ( int j = 0; j < clientList.size(); j++ ) {
			if ( sClient == clientList[ j ] ) {
				bClientExist = true;
				break;
			}
		}
		if ( !bClientExist ) {
			clientList.push_back( sClient );
		}
		nPort++;
	}
	jack_client_close( client );

	string sClientName = "";
	for ( int nInstance = 1; nInstance < 16; nInstance++ ) {
//		sprintf( clientName, "Hydrogen-%d", nInstance );
	//	sprintf( clientName, "Hydrogen-%d", getpid() );
		sClientName = "Hydrogen-" + toString( nInstance );
		bool bExist = false;
		for ( int i = 0; i < clientList.size(); i++ ) {
			if ( sClientName == clientList[ i ] ){
				bExist = true;
				break;
			}
		}
		if ( !bExist ) {
			break;
		}
	}


	// try to become a client of the JACK server
	if ( ( client = jack_client_new( sClientName.c_str() ) ) == 0 ) {
		errorLog( "[JackDriver::init] Jack server not running? (jack_client_new). Using " + sClientName + " as client name"  );
		return 3;
	}

	jack_server_sampleRate = jack_get_sample_rate (client);
	jack_server_bufferSize = jack_get_buffer_size (client);


	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/
	jack_set_process_callback (client, this->processCallback, 0);


	/* tell the JACK server to call `srate()' whenever
	   the sample rate of the system changes.
	*/
	jack_set_sample_rate_callback (client, jackDriverSampleRate, 0);


	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/
	jack_on_shutdown (client, jackDriverShutdown, 0);


	/* create two ports */
	output_port_1 = jack_port_register ( client, "out_L", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );
	output_port_2 = jack_port_register ( client, "out_R", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );

	if ( (output_port_1 == NULL) || ( output_port_2 == NULL ) ) {
		( Hydrogen::getInstance() )->raiseError( Hydrogen::JACK_ERROR_IN_PORT_REGISTER );
		return 4;
	}


	/* create MAX_INSTRUMENTS dedicated channel output ports */
	if (track_out_flag) {
		char tmpbuf[255];

		for (unsigned int n=0; n < MAX_INSTRUMENTS; ++n) {
			snprintf (tmpbuf, sizeof(tmpbuf), "track_out_%d_L", n+1);
			track_output_ports_L[n] = jack_port_register ( client, tmpbuf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );
			if (track_output_ports_L[n] == NULL) {
				( Hydrogen::getInstance() )->raiseError( Hydrogen::JACK_ERROR_IN_PORT_REGISTER );
			}

			snprintf (tmpbuf, sizeof(tmpbuf), "track_out_%d_R", n+1);
			track_output_ports_R[n] = jack_port_register ( client, tmpbuf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );
			if (track_output_ports_R[n] == NULL) {
				( Hydrogen::getInstance() )->raiseError( Hydrogen::JACK_ERROR_IN_PORT_REGISTER );
			}
		}
	}

	// clear buffers
//	jack_default_audio_sample_t *out_L = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port_1, jack_server_bufferSize);
//	jack_default_audio_sample_t *out_R = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port_2, jack_server_bufferSize);
//	memset( out_L, 0, nBufferSize * sizeof( float ) );
//	memset( out_R, 0, nBufferSize * sizeof( float ) );


	return 0;
}



void JackDriver::play()
{
	if (( Preferences::getInstance() )->m_bJackTransportMode ==  Preferences::USE_JACK_TRANSPORT) {
		if (client) {
			infoLog( "jack_transport_start()" );
			jack_transport_start( client );
		}
	}
	else {
		m_transport.m_status = TransportInfo::ROLLING;
	}
}



void JackDriver::stop()
{
	if (( Preferences::getInstance() )->m_bJackTransportMode ==  Preferences::USE_JACK_TRANSPORT) {
		if (client) {
			infoLog( "jack_transport_stop()" );
			jack_transport_stop( client );
		}
	}
	else {
		m_transport.m_status = TransportInfo::STOPPED;
	}
}



void JackDriver::locate( unsigned long nFrame )
{
	if (( Preferences::getInstance() )->m_bJackTransportMode ==  Preferences::USE_JACK_TRANSPORT) {
		if (client) {
			warningLog( "calling jack_transport_locate(" + toString(nFrame) + ")" );
			jack_transport_locate( client, nFrame );
		}
	}
	else {
		m_transport.m_nFrames = nFrame;
	}
}



void JackDriver::setBpm(float fBPM)
{
	warningLog( "[setBpm] " + toString(fBPM) );
	m_transport.m_nBPM = fBPM;
}


#endif // JACK_SUPPORT

