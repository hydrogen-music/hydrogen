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

// check if jack support is disabled
#if defined(H2CORE_HAVE_JACK) || _DOXYGEN_
// JACK support es enabled.

#include <map>
#include <pthread.h>
#include <jack/jack.h>

#if defined(H2CORE_HAVE_JACKSESSION) || _DOXYGEN_
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
/**
 * JACK (Jack Audio Connection Kit) server driver. 
 *
 * This object will only be accessible if H2CORE_HAVE_JACK was defined
 * during the configuration and the user enables the support of the
 * JACK server.
 */
class JackAudioDriver : public AudioOutput
{
	H2_OBJECT
public:
	/** 
	 * Object holding the external client session with the JACK
	 * server. 
	 *
	 * It is set via init().
	 */
	jack_client_t *m_pClient;
	/**
	 * Constructor of the JACK server driver.
	 *
	 * Sets a number of variables:
	 * - #must_relocate = 0
	 * - #locate_countdown = 0
	 * - #bbt_frame_offset = 0
	 * - #track_port_count = 0
	 * - #__track_out_enabled = Preferences::m_bJackTrackOuts
	 *
	 * In addition, it also assigned the provided
	 * @a processCallback argument to the corresponding
	 * #processCallback variable and overwrites the memory
	 * allocated by the #track_output_ports_L and
	 * #track_output_ports_R variable with zeros.
	 *
	 * @param processCallback Prototype for the client supplied
			 function that is called by the engine anytime 
			 there is work to be done. It gets two input
			 arguments _nframes_ of type
			 jack_nframes_t, which specifies the number of
			 frames to process, and a void pointer called
			 _arg_, which points to a client supplied
			 structure.  Two preconditions do act on the
			 _nframes_ argument: _nframes_ ==
			 getBufferSize() and _nframes_ == pow(2,x) It
			 returns zero on success and non-zero on
			 error.
	 */
	JackAudioDriver( JackProcessCallback processCallback );
	/**
	 * Destructor of the JACK server driver.
	 *
	 * Calling disconnect().
	 */
	~JackAudioDriver();

	/**
	 * Connects to output ports via the JACK server.
	 *
	 * Starts by telling the JACK server that Hydrogen is ready to
	 * process audio using the jack_activate function (from the
	 * jack/jack.h header) and overwriting the memory allocated by
	 * #track_output_ports_L and #track_output_ports_R with
	 * zeros. If the #m_bConnectOutFlag variable is true or
	 * LashClient is used and Hydrogen is not within a new Lash
	 * project, the function attempts to connect the
	 * #output_port_1 port with #output_port_name_1 and the
	 * #output_port_2 port with #output_port_name_2. To establish
	 * the connection _jack_connect()_ (jack/jack.h) will be
	 * used. In case this was not successful, the function will
	 * look up all ports containing the JackPortIsInput
	 * (jack/types.h) flag using _jack_get_ports()_ (jack/jack.h)
	 * and attempts to connect to the first two it found.
	 *
	 * @return 
	 * - 0 : if either the connection of the output ports
	 *       did work, two ports having the JackPortIsInput flag
	 *       where found and the #output_port_1 and #output_port_2
	 *       ports where successfully connected to them, or the
	 *       user enabled Lash during compilation and an
	 *       established project was used.
	 * - 1 : The activation of the JACK client using
	 *       _jack_activate()_ (jack/jack.h) did fail.
	 * - 2 : The connections to #output_port_name_1 and
	 *       #output_port_name_2 could not be established and the
	 *       there were either no JACK ports holding the
	 *       JackPortIsInput flag found or no connection to them
	 *       could be established.
	 */
	int connect();
	/**
	 * Disconnects the JACK client of the Hydrogen app from the
	 * JACK server.
	 *
	 * Firstly, it calls deactivate(). Then, it closes the
	 * connection between the JACK server and the local client
	 * using jack_client_close (jack/jack.h), and sets the
	 * #m_pClient pointer to NULL.
	 */
	void disconnect();
	/**
	 * Deactivates JACK client of Hydrogen and disconnects all
	 * ports belonging to it.
	 *
	 * It calls the _jack_deactivate()_ (jack/jack.h) function on
	 * the current client #m_pClient and overwrites the memory
	 * allocated by #track_output_ports_L and
	 * #track_output_ports_R with zeros.
	 */
	void deactivate();
	/**
	 * Returns the global variable H2Core::jack_server_bufferSize,
	 * which is defined in the corresponding source file. The
	 * setting is performed by the inline function 
	 * _jackDriverBufferSize()_, which is also defined on
	 * source-side. 
	 */
	unsigned getBufferSize();
	/**
	 * Returns the global variable H2Core::jack_server_sampleRate,
	 * which is defined in the corresponding source file. The
	 * setting is performed by the inline function
	 * _jackDriverSampleRate()_, which is also defined on
	 * source-side.
	 */
	unsigned getSampleRate();
	int getNumTracks();

	jack_transport_state_t getTransportState() {
		return m_JackTransportState;
	}
	jack_position_t getTransportPos() {
		return m_JackTransportPos;
	}
	
	/**
	 * Make sure the number of track outputs match the instruments
	 * in @a song, and name the ports.
	 */
	void makeTrackOutputs( Song * );
	/**
	 * Give the @a n 'th port the name of @a instr .
	 * If the n'th port doesn't exist, new ports up to n are
	 * created.
	 */
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

	/**
	 * Initializes the JACK audio driver.
	 *
	 * Firstly, it determines the destination ports
	 * #output_port_name_1 and #output_port_name_2 the output
	 * ports of Hydrogen will be connected to in connect() from
	 * Preferences::m_sJackPortName1 and
	 * Preferences::m_sJackPortName2. The name of the JACK client
	 * is either set to "Hydrogen" or, if H2CORE_HAVE_OSC was
	 * defined during compilation and OSC support is enabled, to
	 * Preferences::m_sNsmClientId via
	 * Preferences::getNsmClientId(). 
	 *
	 * Next, the function attempts to open an external client
	 * session with the JACK server using _jack_client_open()_
	 * (jack/jack.h) and saves it to the pointer #m_pClient. In
	 * case this didn't work properly, it will start two more
	 * attempts. Sometime JACK doesn't stop and start fast
	 * enough. If the compiler flag H2CORE_HAVE_JACKSESSION was
	 * set and the user enabled the usage of JACK session, the
	 * client will be opened using the corresponding option and
	 * the sessionID Token Preferences::jackSessionUUID, obtained
	 * via Preferences::getJackSessionUUID(), will be provided so
	 * the sessionmanager can identify the client again.
	 *
	 * If the client was opened properly, the function will get
	 * its sample rate using _jack_get_sample_rate()_ and buffer
	 * size using _jack_get_buffer_size()_ (both jack/jack.h) and
	 * stores them in H2Core::jack_server_sampleRate,
	 * Preferences::m_nSampleRate, H2Core::jack_server_bufferSize,
	 * and Preferences::m_nBufferSize. In addition, it also
	 * registers JackAudioDriver::processCallback,
	 * H2Core::jackDriverSampleRate, H2Core::jackDriverBufferSize,
	 * and H2Core::jackDriverShutdown using
	 * _jack_set_process_callback()_,
	 * _jack_set_sample_rate_callback()_,
	 * _jack_set_buffer_size_callback()_, and _jack_on_shutdown()_
	 * (all in jack/jack.h).
	 *
	 * Next, two output ports called "out_L" and "out_R" are
	 * registered for the client #m_pClient using
	 * _jack_port_register()_.
	 *
	 * If everything worked properly, LASH is used
	 * (Preferences::useLash()) by the user, and the LashClient is
	 * LashClient::isConnected() the name of the client will be
	 * stored in LashClient::jackClientName using
	 * LashClient::setJackClientName. If JACK session was enabled,
	 * the jack_session_callback() will be registered using
	 * _jack_set_session_callback()_ (jack/session.h).
	 *
	 * Finally, the function will check whether Hydrogen is in
	 * either JackMaster or JackSlave mode via
	 * Preferences::m_bJackMasterMode and calls initTimeMaster()
	 * if in JackMaster. It sets #m_nJackConditionalTakeOver to 1
	 * so Hydrogen will become the time master regardless if there
	 * is already one present or not.
	 *
	 * \param bufferSize Unused and only present to assure
	 * compatibility with the JACK API.
	 *
	 * \return
	 * -  0 : on success.
	 * - -1 : if the pointer #m_pClient obtained via
	 *        _jack_client_open()_ (jack/jack.h) is 0.
	 * -  4 : unable to register the "out_L" and/or "out_R" output
	 *        ports for the JACK client using
	 *        _jack_port_register()_ (jack/jack.h).
	 */
	int init( unsigned bufferSize );

	virtual void play();
	virtual void stop();
	virtual void locate( unsigned long nFrame );
	/**
	 * When jack_transport_start() is called, it takes effect from
	 * the next processing cycle. The location info from the
	 * timebase_master, if there is one, will not be available
	 * until the _next_ next cycle. The code must therefore wait
	 * one cycle before syncing up with timebase_master.
	 */
	virtual void updateTransportInfo();
	virtual void setBpm( float fBPM );
	/**
	 * ??? Calculates the offset in the frame number.
	 *
	 * Sets the #bbt_frame_offset variable to the _frame_ member
	 * of #m_JackTransportPos, which is updated in
	 * updateTransportInfo(), minus the frame number of a local
	 * version of the variable including humanization etc.
	 */
	void calculateFrameOffset();
	/**
	 * ???
	 *
	 * Set the variable #locate_countdown to @a cycles_to_wait and
	 * #locate_frame to @a frame.
	 */
	void locateInNCycles( unsigned long frame, int cycles_to_wait = 2 );

//jack timebase callback
	/**
	 * Sets Hydrogen as time master or releases it from that
	 * responsibility. 
	 *
	 * Called at the very end of init() if
	 * Preferences::m_bJackMasterMode is equal to
	 * Preferences::USE_JACK_TIME_MASTER. In this case
	 * _jack_set_timebase_callback()_ (jack/transport.h) is called
	 * to set Hydrogen as time master or else
	 * _jack_release_timebase()_ (jack/transport.h) does release
	 * Hydrogen from this responsibility.
	 *
	 * If for some reason registering Hydrogen as time master does
	 * not work, the function sets Preferences::m_bJackMasterMode
	 * to Preferences::NO_JACK_TIME_MASTER.
	 */ 
	void initTimeMaster();
	void com_release();
//~ jack timebase callback

protected:
//jack timebase callback
	static void jack_timebase_callback( jack_transport_state_t state,
					    jack_nframes_t nframes,
					    jack_position_t *pos,
					    int new_pos,
					    void *arg );
//~ jack timebase callback

#if defined(H2CORE_HAVE_JACKSESSION) || _DOXYGEN_
	/**
	 * Function to call by the JACK server when a session event is
	 * to be delivered.
	 *
	 * It is registered to the JACK client in init() using
	 * _jack_set_session_callback()_ (jack/session.h) if
	 * H2CORE_HAVE_JACKSESSION was defined during compilation.
	 */
	static void jack_session_callback( jack_session_event_t *event,
					   void *arg );
	
	void jack_session_callback_impl( jack_session_event_t *event );
#endif

private:
	void relocateBBT();


	H2Core::Hydrogen *		m_pEngine;

	long long			bbt_frame_offset;
	int				must_relocate;		// A countdown to wait for valid information from another Time Master.
	int				locate_countdown;	// (Unrelated) countdown, for postponing a call to 'locate'.
	unsigned long			locate_frame;		// The frame to locate to (used in 'locateInNCycles'.)
	/**
	 * Function the JACK server will call whenever there is work
	 * to do. 
	 *
	 * It is handed to JackAudioDriver as an input argument and
	 * gets registered as using _jack_set_process_callback()_
	 * (jack/jack.h) in init().
	 *
	 * The code must be suitable for real-time execution.  That
	 * means that it cannot call functions that might block for a
	 * long time. This includes malloc, free, printf,
	 * pthread_mutex_lock, sleep, wait, poll, select,
	 * pthread_join, pthread_cond_wait, etc, etc.
	 */
	JackProcessCallback		processCallback;
	/**
	 * First source port for which a connection to
	 * #output_port_name_1 will be established in connect() via
	 * the JACK server.
	 */
	jack_port_t *			output_port_1;
	/**
	 * Second source port for which a connection to
	 * #output_port_name_2 will be established in connect() via
	 * the JACK server.
	 */
	jack_port_t *			output_port_2;
	/**
	 * Destination of the first source port #output_port_1, for
	 * which a connection will be established in connect(). It is
	 * set to Preferences::m_sJackPortName1 during the call of
	 * init(). 
	 */
	QString				output_port_name_1;
	/**
	 * Destination of the second source port #output_port_2, for
	 * which a connection will be established in connect(). It is
	 * set to Preferences::m_sJackPortName2 during the call of
	 * init(). 
	 */
	QString				output_port_name_2;
	int				track_map[MAX_INSTRUMENTS][MAX_COMPONENTS];
	int				track_port_count;
	jack_port_t *			track_output_ports_L[MAX_INSTRUMENTS];
	jack_port_t *		 	track_output_ports_R[MAX_INSTRUMENTS];

	jack_transport_state_t		m_JackTransportState;
	jack_position_t			m_JackTransportPos;

	/**
	 * Probably tells whether or not the output ports of the
	 * current session ARE already properly connected in the JACK
	 * server. Or maybe if they SHOULD be connected.
	 */
	bool				m_bConnectOutFlag;
	/**
	 * Specifies whether to use a conditional take over in the
	 * switching of the JACK time master. If set to non-zero the
	 * take over will fail if there is already a time master
	 * present. It will be initialized with 1 in init().
	 */
	int				m_nJackConditionalTakeOver;

};

}; // H2Core namespace


#else // H2CORE_HAVE_JACK
// JACK is disabled

namespace H2Core {
class JackAudioDriver : public NullDriver {
	H2_OBJECT
public:
	/**
	 * Fallback version of the JackAudioDriver in case
	 * H2CORE_HAVE_JACK was not defined during the configuration
	 * and the usage of the JACK audio server is not intended by
	 * the user.
	 */
	JackAudioDriver( audioProcessCallback processCallback ) : NullDriver( processCallback ) {}

};

}; // H2Core namespace


#endif // H2CORE_HAVE_JACK

#endif

