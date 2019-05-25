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
 * __Timebase Master__:
 *
 * The timebase master is responsible to update additional information
 * in the transport information of the JACK server apart from the
 * transport position in frames (see TransportInfo::m_nFrames if you
 * aren't familiar with frames), like the current beat, bar, tick,
 * tick size, speed etc. Every client can be registered as timebase
 * master by supplying a callback (for Hydrogen this would be
 * jack_timebase_callback()) but there can be at most one timebase
 * master at a time. Having none at all is perfectly fine too. Apart
 * from this additional responsibility, the registered client has no
 * other rights compared to others.
 *
 * After the status of the JACK transport has changed from
 * _JackTransportStarting_ to _JackTransportRolling_, the timebase
 * master needs an additional cycle to update its information.
 *
 * __Transport Control__:
 *
 * Each JACK client can start and stop the transport or relocate its
 * current position. The request will take place in cycles. During the
 * first the status of the transport changes to
 * _JackTransportStarting_ to inform all clients a change is about to
 * happen. During the second the status is again
 * _JackTransportRolling_ and the transport position is updated
 * according to the request. But only its raw representation in frames
 * in the _frame_ member. The current timebase master, if present,
 * needs another cycle to update the additional transport
 * information. 
 *
 * Such a relocation request is also triggered when clicking on the
 * timeline or the player control buttons of Hydrogen. Internally,
 * audioEngine_stop() is called during the cycle in which the JACK
 * transport status is _JackTransportStarting_ and started again by
 * audioEngine_start() when in _JackTransportRolling_ in the next
 * cycle. Note that if there are slow synchronizing client in JACK's
 * connection graph, it can take multiple cycles until the JACK
 * transport is rolling again.
 *
 * Also note that Hydrogen overwrites its local TransportInfo stored
 * in AudioOutput::m_transport only with the transport position of the
 * JACK server if a relocation did happened or another timebase master
 * did change the speed. During normal transport the current position
 * TransportInfo::m_nFrames will be always the same as the one of JACK
 * during a cycle and incremented by the buffer size in
 * audioEngine_process() at the very end of the cycle. The same
 * happens for the transport information of the JACK server but in
 * parallel. 
 *
 * This object will only be accessible if #H2CORE_HAVE_JACK was defined
 * during the configuration and the user enables the support of the
 * JACK server.
 */
class JackAudioDriver : public AudioOutput
{
public:
	/** \return #m_sClassName*/
	static const char* className() { return m_sClassName; }
	/** 
	 * Object holding the external client session with the JACK
	 * server. 
	 *
	 * It is set via init().
	 */
	jack_client_t* m_pClient;
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
	 * In addition, it also assigned the provided @a
	 * processCallback argument, the audioEngine_process()
	 * function, to the corresponding #processCallback variable
	 * and overwrites the memory allocated by the
	 * #track_output_ports_L and #track_output_ports_R variable
	 * with zeros.
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
	 * look up all ports containing the _JackPortIsInput_
	 * (jack/types.h) flag using _jack_get_ports()_ (jack/jack.h)
	 * and attempts to connect to the first two it found.
	 *
	 * @return 
	 * - __0__ : if either the connection of the output ports
	 *       did work, two ports having the _JackPortIsInput_ flag
	 *       where found and the #output_port_1 and #output_port_2
	 *       ports where successfully connected to them, or the
	 *       user enabled Lash during compilation and an
	 *       established project was used.
	 * - __1__ : The activation of the JACK client using
	 *       _jack_activate()_ (jack/jack.h) did fail.
	 * - __2__ : The connections to #output_port_name_1 and
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
	/** Accesses the number of output ports currently in use.
	 * \return #track_port_count */
	int getNumTracks();

	/** Returns #m_JackTransportState */
	jack_transport_state_t getTransportState() {
		return m_JackTransportState;
	}
	/** Returns #m_JackTransportPos */
	jack_position_t getTransportPos() {
		return m_JackTransportPos;
	}
	
	/**
	 * Creates per component output ports for each instrument.
	 *
	 * Firstly, it resets #track_map with zeros. Then, it loops
	 * through all the instruments and their components, creates a
	 * new output or resets an existing one for each of them using
	 * setTrackOutput(), and stores the corresponding track number
	 * in #track_map. Finally, all ports in #track_output_ports_L
	 * and #track_output_ports_R, which haven't been used in the
	 * previous step, are unregistered using
	 * _jack_port_unregister()_ (jack/jack.h) and overwritten with
	 * 0. #track_port_count will be set to biggest track number
	 * encountered during the creation/reassignment step.
	 *
	 * The function will only perform its tasks if the
	 * Preferences::m_bJackTrackOuts is set to true.
	 */
	void makeTrackOutputs( Song* pSong );
	/**
	 * Renames the @a n 'th port of JACK client and creates it if
	 * its not already present. 
	 *
	 * If the track number @a n is bigger than the number of ports
	 * currently in usage #track_port_count, @a n + 1 -
	 * #track_port_count new stereo ports will be created using
	 * _jack_port_register()_ (jack/jack.h) and #track_port_count
	 * updated to @a n + 1.
	 *
	 * Afterwards, the @a n 'th port is renamed to a concatenation
	 * of "Track_", DrumkitComponent::__name, "_", @a n + 1, "_",
	 * Instrument::__name, and "_L", or "_R" using either
	 * _jack_port_rename()_ (if HAVE_JACK_PORT_RENAME is defined)
	 * or _jack_port_set_name()_ (both jack/jack.h). The former
	 * renaming function triggers a _PortRename_ notifications to
	 * clients that have registered a port rename handler.
	 *
	 * \param n Track number for which a port should be renamed
	 *   (and created).
	 * \param instr Pointer to the corresponding Instrument.
	 * \param pCompo Pointer to the corresponding
	 *   InstrumentComponent.
	 * \param pSong Pointer to the corresponding Song.
	 */
	void setTrackOutput( int n, Instrument* instr, InstrumentComponent* pCompo, Song* pSong );

	/** Sets #m_bConnectOutFlag to @a flag.*/
	void setConnectDefaults( bool flag ) {
		m_bConnectOutFlag = flag;
	}
	/** Returns #m_bConnectOutFlag */
	bool getConnectDefaults() {
		return m_bConnectOutFlag;
	}

	/**
	 * Get and return the content in the left stereo output
	 * port. It calls _jack_port_get_buffer()_ (jack/jack.h) with
	 * both the port name #output_port_1 and buffer size
	 * #jack_server_bufferSize.
	 * \return Pointer to buffer content of type
	 * _jack_default_audio_sample_t*_ (jack/types.h)
	 */
	float* getOut_L();
	/**
	 * Get and return the content in the right stereo output
	 * port. It calls _jack_port_get_buffer()_ (jack/jack.h) with
	 * both the port name #output_port_2 and buffer size
	 * #jack_server_bufferSize.
	 * \return Pointer to buffer content of type
	 * _jack_default_audio_sample_t*_ (jack/types.h)
	 */
	float* getOut_R();
	/**
	 * Get and return the content of a specific left output
	 * port. It calls _jack_port_get_buffer()_ (jack/jack.h) with
	 * the port in @a nTrack element of #track_output_ports_L and
	 * buffer size #jack_server_bufferSize.
	 * \param nTrack Track number. Must not be bigger than
	 * #track_port_count.
	 * \return Pointer to buffer content of type
	 * _jack_default_audio_sample_t*_ (jack/types.h)
	 */
	float* getTrackOut_L( unsigned nTrack );
	/**
	 * Get and return the content of a specific right output
	 * port. It calls _jack_port_get_buffer()_ (jack/jack.h) with
	 * the port in @a nTrack element of #track_output_ports_R and
	 * buffer size #jack_server_bufferSize.
	 * \param nTrack Track number. Must not be bigger than
	 * #track_port_count.
	 * \return Pointer to buffer content of type
	 * _jack_default_audio_sample_t*_ (jack/types.h)
	 */
	float* getTrackOut_R( unsigned nTrack );
	/** 
	 * Convenience function looking up the track number of a
	 * component of an instrument using in #track_map using their
	 * IDs Instrument::__id and
	 * InstrumentComponent::__related_drumkit_componentID. Using
	 * the track number it then calls getTrackOut_L( unsigned )
	 * and returns its result.
	 * \param instr Pointer to an instrument
	 * \param pCompo Pointer to one of the instrument's components.
	 * \return Pointer to buffer content of type
	 * _jack_default_audio_sample_t*_ (jack/types.h)
	 */
	float* getTrackOut_L( Instrument* instr, InstrumentComponent* pCompo );
	/** 
	 * Convenience function looking up the track number of a
	 * component of an instrument using in #track_map using their
	 * IDs Instrument::__id and
	 * InstrumentComponent::__related_drumkit_componentID. Using
	 * the track number it then calls getTrackOut_R( unsigned )
	 * and returns its result.
	 * \param instr Pointer to an instrument
	 * \param pCompo Pointer to one of the instrument's components.
	 * \return Pointer to buffer content of type
	 * _jack_default_audio_sample_t*_ (jack/types.h)
	 */
	float* getTrackOut_R( Instrument* instr, InstrumentComponent* pCompo );

	/**
	 * Initializes the JACK audio driver.
	 *
	 * Firstly, it determines the destination ports
	 * #output_port_name_1 and #output_port_name_2 the output
	 * ports of Hydrogen will be connected to in connect() from
	 * Preferences::m_sJackPortName1 and
	 * Preferences::m_sJackPortName2. The name of the JACK client
	 * is either set to "Hydrogen" or, if #H2CORE_HAVE_OSC was
	 * defined during compilation and OSC support is enabled, to
	 * Preferences::m_sNsmClientId via
	 * Preferences::getNsmClientId(). 
	 *
	 * Next, the function attempts to open an external client
	 * session with the JACK server using _jack_client_open()_
	 * (jack/jack.h) and saves it to the pointer #m_pClient. In
	 * case this didn't work properly, it will start two more
	 * attempts. Sometime JACK doesn't stop and start fast
	 * enough. If the compiler flag #H2CORE_HAVE_JACKSESSION was
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
	 * Finally, the function will check whether Hydrogen should be
	 * the JACK timebase master or not via
	 * Preferences::m_bJackMasterMode and calls initTimeMaster()
	 * if its indeed the case. It sets #m_nJackConditionalTakeOver
	 * to 1 so Hydrogen will become the timebase master regardless if
	 * there is already one present or not.
	 *
	 * \param bufferSize Unused and only present to assure
	 * compatibility with the JACK API.
	 *
	 * \return
	 * -  __0__ : on success.
	 * - __-1__ : if the pointer #m_pClient obtained via
	 * _jack_client_open()_ (jack/jack.h) is 0.
	 * - __4__ : unable to register the "out_L" and/or "out_R"
	 * output ports for the JACK client using
	 * _jack_port_register()_ (jack/jack.h).
	 */
	int init( unsigned bufferSize );

	/**
	 * Starts the JACK transport.
	 *
	 * If either Preferences::m_bJackTransportMode equals
	 * Preferences::USE_JACK_TRANSPORT or
	 * Preferences::m_bJackMasterMode equals
	 * Preferences::USE_JACK_TIME_MASTER the
	 * _jack_transport_start()_ (jack/transport.h) function will
	 * be called to start the JACK transport. If neither
	 * of the two equalities is true, TransportInfo::m_status is
	 * set to TransportInfo::ROLLING instead.
	 */
	virtual void play();
	/**
	 * Stops the JACK transport.
	 *
	 * If either Preferences::m_bJackTransportMode equals
	 * Preferences::USE_JACK_TRANSPORT or
	 * Preferences::m_bJackMasterMode equals
	 * Preferences::USE_JACK_TIME_MASTER the
	 * _jack_transport_stop()_ (jack/transport.h) function will
	 * be called to start the JACK transport. If neither
	 * of the two equalities is true, TransportInfo::m_status is
	 * set to TransportInfo::STOPPED instead.
	 */
	virtual void stop();
	/**
	 * Re-positions the transport position to @a nFrame.
	 *
	 * If the Preferences::USE_JACK_TRANSPORT mode is chosen in
	 * Preferences::m_bJackTransportMode, the
	 * _jack_transport_locate()_ (jack/transport.h) function will
	 * be used to request the new transport position. If not, @a
	 * nFrame will be assigned to TransportInfo::m_nFrames of the
	 * local instance of the TransportInfo
	 * AudioOutput::m_transport.
	 *   
	 * \param nFrame Requested new transport position.
	 */
	virtual void locate( unsigned long nFrame );
	/**
	 * Updating the local instance of the TransportInfo
	 * AudioOutput::m_transport.
	 *
	 * The function queries the transport position and addition
	 * information from the JACK server and adjusts the
	 * information stored in AudioOutput::m_transport if there is
	 * a mismatch.
	 *
	 * - Calls _jack_transport_query()_ (jack/transport.h), stores
             the current transport position in #m_JackTransportPos,
             and its state in #m_JackTransportState.
	 * - If #m_JackTransportState is either _JackTransportStopped_
             or _JackTransportStarting_, TransportInfo::m_status will
             be set to TransportInfo::STOPPED. If its
             _JackTransportRolling_, TransportInfo::m_status will be
             set to TransportInfo::ROLLING and in case
             TransportInfo::m_status was not in this state beforehand,
             #must_relocate will be set to 2. This will trigger a
             relocation in relocateBBT() based on the updated
             information of the JACK timebase master (not ourselves) at
             the end of updateTransportInfo() during the _next_
             cycle. We have to wait one cycle since the client being
             the current JACK timebase master also just started playing
             and it needs this cycle to update the bar, beat, and tick
             information.
	 * - Calls Hydrogen::setTimelineBpm() to update both the
             global speed of the Song Song::__bpm, as well as the
	     fallback speed Hydrogen::m_nNewBpmJTM() with the local
             tempo at the current position on the timeline.
	 * - Checks whether the speed was changed by another JACK time
             master (there can only be one). In that case we will just
             store the changes in TransportInfo::m_nBpm of the current
             instance of the TransportInfo
             AudioOutput::m_transport. The tempo will be set by the
             calling function audioEngine_process_transport()
             afterwards. In addition, #must_relocate will be set to 1
             in order to relocate using relocateBBT() at the end of
             the function call. After all we learned that the time
             master applied some changes. So let's be sure things are
             set right.
	 * - In case the transport position changed due to an user
             interaction (e.g. clicking somewhere at the Timeline) or
             a relocation triggered by another JACK client, we will
             detect this change since position stored in
             TransportInfo::m_nFrames plus the constant offset
             #bbt_frame_offset does not equate to the _frame_ member
             of #m_JackTransportPos anymore. Then we will check
             whether another JACK timebase master is present and set
             #must_relocate to 2 if the timebase master did not provided a
             different speed beforehand. So, we wait one cycle for him
             to update all information and call relocateBBT()
             afterwards. If Hydrogen is the JACK timebase master itself,
             TransportInfo::m_nFrames is set to its value of the
             previous cycle by assigned the #m_nHumantimeFrames. If,
             on the other hand, no Jack timebase master is present at all,
             TransportInfo::m_nFrames is set to the current position
             of the JACK server (_frame_ component of
             #m_JackTransportPos), #bbt_frame_offset is reset to 0,
             and Hydrogen::triggerRelocateDuringPlay() is called if
             the transport is rolling.
	 * - Finally, #m_nHumantimeFrames will be set to the _frame_
             member of #m_JackTransportPos, relocateBBT() called if
             #must_relocate equals one,
             Hydrogen::triggerRelocateDuringPlay() be called if the
             transport is also rolling, and #must_relocate decremented
             by one.
	 *
	 * If Preferences::USE_JACK_TRANSPORT was not selected in
	 * Preferences::m_bJackTransportMode, the function will return
	 * without performing any action.
	 */
	virtual void updateTransportInfo();
	/** Set the tempo stored TransportInfo::m_nBPM of the local
	 * instance of the TransportInfo AudioOutput::m_transport.
	 * \param fBPM new tempo. 
	 */
	virtual void setBpm( float fBPM );
	/**
	 * Calculates the difference between the transport position
	 * obtained by querying JACK and the value stored in
	 * TransportInfo::m_nFrames and stores it in
	 * #bbt_frame_offset.
	 *
	 * It is triggered in audioEngine_process_checkBPMChanged() in
	 * case the tick size did changed. Imagine the following
	 * setting: During the playback you decide to change the speed
	 * of the song. This would cause a lot of position information
	 * within Hydrogen, which are given in ticks, to be off since
	 * the tick size depends on the speed and just got changed
	 * too. Instead, TransportInfo::m_nFrames will scaled to
	 * reflect the changes and everything will be still in place
	 * with the user to not note a single thing. Unfortunately,
	 * now the transport position in frames of the audio engine
	 * and of the JACK server are off by a constant offset. To
	 * nevertheless be able to identify relocation in
	 * updateTransportInfo(), this constant offset is stored in
	 * #bbt_frame_offset.
	 */
	void calculateFrameOffset();
	/**
	 * Relocate the transport position to @a frame @a
	 * cycles_to_wait cycles.
	 *
	 * The function is triggered in audioEngine_process() after
	 * audioEngine_updateNoteQueue() returned -1, stop() was
	 * called, and transport location was moved to the beginning
	 * of the Song using locate().
	 *
	 * Judging from comments in the code, this additional
	 * repositioning was introduced to keep Hydrogen synchronized
	 * with Ardour.
	 *
	 * Internally, it sets the variable #locate_countdown to @a
	 * cycles_to_wait and #locate_frame to @a frame.
	 *
	 * The audioEngine_updateNoteQueue() function is called after
	 * audioEngine_process_transport() and its decrement of
	 * #locate_countdown. It thus only take @a cycles_to_wait - 1
	 * cycles calls to audioEngine_process() for the relocation to
	 * happen.
	 *
	 * \param frame New position
	 * \param cycles_to_wait In how many cycles the repositioning
	 *   should take place.
	 */
	void locateInNCycles( unsigned long frame, int cycles_to_wait = 2 );

	/**
	 * Registers Hydrogen as JACK timebase master.
	 *
	 * Called at the very end of init() if
	 * Preferences::m_bJackMasterMode is equal to
	 * Preferences::USE_JACK_TIME_MASTER. In this case
	 * _jack_set_timebase_callback()_ (jack/transport.h) is called
	 * to set Hydrogen as timebase master.
	 *
	 * If for some reason registering Hydrogen as timebase master
	 * does not work, the function sets
	 * Preferences::m_bJackMasterMode to
	 * Preferences::NO_JACK_TIME_MASTER.
	 *
	 * If the function is called with Preferences::m_bJackMasterMode
	 * set to Preferences::NO_JACK_TIME_MASTER,
	 * _jack_release_timebase()_ (jack/transport.h) will be called
	 * instead.
	 */ 
	void initTimeMaster();
	/**
	 * Calls _jack_release_timebase()_ (jack/transport.h) to
	 * release Hydrogen from the JACK timebase master
	 * responsibilities. This causes the jack_timebase_callback()
	 * callback function to not be called by the JACK server
	 * anymore.
	 */
	void com_release();

protected:
	/**
	 * Callback function registered to the JACK server in
	 * initTimeMaster() if Hydrogen is set as JACK timebase
	 * master.
	 *
	 * It will update the current position not just in frames till
	 * the beginning of the song, but also in terms of beat, bar,
	 * and tick values. 
	 *
	 * The function it will be called after the
	 * audioEngine_process() function and only if the
	 * #m_JackTransportState is _JackTransportRolling_. It sets
	 * the following members of the JACK position object @a pos is
	 * pointing to:
	 * - __bar__ : current position corrected by the #bbt_frame_offset
	 * \code{.cpp} 
	 * Hydrogen::getPosForTick( ( pos->frame - bbt_frame_offset )/ 
	 *                          m_transport.m_nTickSize ) ) 
	 * \endcode
	 * - __ticks_per_beat__ :  the output of
	 * Hydrogen::getTickForHumanPosition() with the __bar__ member
	 * supplied as input argument.
	 * - __valid__ : to ( _JackPositionBBT_ | _JackBBTFrameOffset_
	 * ), transport states defined
	 * in jack/types.h telling JACK we are willing to provide bar,
	 * beat, and trick information as well as an constant offset.
	 * - __beats_per_bar__ : the __ticks_per_beat__ member divided
	 * by 48.
	 * - __beat_type__ : 4.0
	 * - __beats_per_minute__ : the local speed returned by
	 * Hydrogen::setTimelineBpm() at the __bar__ member.
	 * - __bbt_offset__ : 0
	 *
	 * Afterwards the __bar__ member is incremented by 1 the
	 * following information will be set as well.
	 * - __bar_start_tick__ : the corrected position used in the
	 * first __bar__ assignment minus the number of ticks elapsed
	 * from the last bar
	 * - __beat__ : The number of elapsed ticks from the last bar
	 * divided by the __ticks_per_beat__ member plus one.
	 * - __tick__ : The number of elapsed ticks from the last bar
	 * modulo the __ticks_per_beat__ member.
	 *
	 * If Hydrogen::getHumantimeFrames() returns a number smaller
	 * than 1, we are at the beginning of the Song and __beat__ =
	 * 1, __tick__ = 0, and __bar_start_tick__ = 0 will be used
	 * instead.
	 *
	 * \param state Current transport state. Not used within the
	 * function but to ensure compatibility.
	 * \param nframes Buffer size. Not used within the function but
	 * to ensure compatibility.
	 * \param pos Current transport position.
	 * \param new_pos Updated transport position in frames. Not
	 * used within the function but to ensure compatibility.
	 * \param arg Pointer to a JackAudioDriver instance.
	 */
	static void jack_timebase_callback( jack_transport_state_t state,
					    jack_nframes_t nframes,
					    jack_position_t* pos,
					    int new_pos,
					    void* arg );

#if defined(H2CORE_HAVE_JACKSESSION) || _DOXYGEN_
	/**
	 * Function to call by the JACK server when a session event is
	 * to be delivered.
	 *
	 * It is registered to the JACK client in init() using
	 * _jack_set_session_callback()_ (jack/session.h) if
	 * #H2CORE_HAVE_JACKSESSION was defined during compilation.
	 *
	 * Internally it hands the @a event to
	 * jack_session_callback_impl().
	 *
	 * \param event Jack session event (see jack/session.h)
	 * \param arg Pointer to an instance of the JackAudioDriver.
	 */
	static void jack_session_callback( jack_session_event_t* event,
					   void* arg );
	
	void jack_session_callback_impl( jack_session_event_t* event );
#endif

private:
	/** Contains the name of the class.
	 *
	 * This variable allows from more informative log messages
	 * with the name of the class the message is generated in
	 * being displayed as well. Queried using className().*/
	static const char* m_sClassName;
	/**
	 * Updates the tick size TransportInfo::m_nTickSize and frame
	 * position TransportInfo::m_nFrames using the transport
	 * position information obtained from another JACK timebase master.
	 *
	 * It is only entered via updateTransportInfo() if there is
	 * JACK timebase master and returns without performing any actions
	 * if Hydrogen itself is the timebase master.
	 */
	void relocateBBT();

	/**
	 * Constant offset between the transport position in
	 * TransportInfo::m_nFrames and the one obtained by the JACK
	 * server query. 
	 *
	 * This can happen when changing the speed during
	 * playback. Note that contrary its name, which involves
	 * bar-beat-tick set by the JACK timebase master, this
	 * constant will be assigned and used even if no JACK timebase
	 * master is present at all! Positive values correspond to a
	 * position ahead of the current transport information. The
	 * variable is initialized with 0 in JackAudioDriver() and
	 * updated in calculateFrameOffset().
	 */
	long long			bbt_frame_offset;
	/** Triggering relocateBBT() in #must_relocate -1 cycles at
	    the end of updateTransportInfo().
	*
	* This is either done immediately if another JACK timebase
	* master (not Hydrogen itself!) changed the speed or during
	* the next cycle if the other timebase master changed the
	* transport position.
	*/
	int				must_relocate;
	/** 
	 * #locate_countdown - 1 of cycles (calls to audioEngine_process())
	 * until to locate() to #locate_frame in
	 * updateTransportInfo(). It is set in locateInNCycles().
	 */
	int				locate_countdown;
	/** 
	 * Frame to locate() to in updateTransportInfo() after
	    #locate_countdown cycles - 1 cycles.   
	 * It is set in locateInNCycles(). 
	 */
	unsigned long			locate_frame;
	/**
	 * Function the JACK server will call whenever there is work
	 * to do. 
	 *
	 * The audioEngine_process() function will be used and
	 * registered using _jack_set_process_callback()_
	 * (jack/jack.h) in init().
	 *
	 * The code must be suitable for real-time execution.  That
	 * means that it cannot call functions that might block for a
	 * long time. This includes _malloc()_, _free()_, _printf()_,
	 * _pthread_mutex_lock()_, _sleep()_, _wait()_, _poll()_,
	 * _select()_, _pthread_join()_, _pthread_cond_wait()_, etc,
	 * etc.
	 */
	JackProcessCallback		processCallback;
	/**
	 * Left source port for which a connection to
	 * #output_port_name_1 will be established in connect() via
	 * the JACK server.
	 */
	jack_port_t*			output_port_1;
	/**
	 * Right source port for which a connection to
	 * #output_port_name_2 will be established in connect() via
	 * the JACK server.
	 */
	jack_port_t*			output_port_2;
	/**
	 * Destination of the left source port #output_port_1, for
	 * which a connection will be established in connect(). It is
	 * set to Preferences::m_sJackPortName1 during the call of
	 * init(). 
	 */
	QString				output_port_name_1;
	/**
	 * Destination of the right source port #output_port_2, for
	 * which a connection will be established in connect(). It is
	 * set to Preferences::m_sJackPortName2 during the call of
	 * init(). 
	 */
	QString				output_port_name_2;
	/**
	 * Matrix containing the track number of each component of
	 * of all instruments. Its rows represent the instruments and
	 * its columns their components. _track_map[2][1]=6_ thus
	 * therefore mean the output of the second component of the
	 * third instrument is assigned the seventh output port. Since
	 * its total size is defined by #MAX_INSTRUMENTS and
	 * #MAX_COMPONENTS, most of its entries will be zero.
	 *
	 * It gets updated by makeTrackOutputs().
	 */
	int				track_map[MAX_INSTRUMENTS][MAX_COMPONENTS];
	/**
	 * Total number of output ports currently in use. It gets
	 * updated by makeTrackOutputs().
	 */
	int				track_port_count;
	/**
	 * Vector of all left audio output ports currently used by the
	 * local JACK client. 
	 *
	 * They will be initialized with all zeros in both
	 * JackAudioDriver(), deactivate(), and connect(). Individual
	 * components will be created, renamed, or reassigned in
	 * setTrackOutput(), deleted in makeTrackOutputs(), and
	 * accessed via getTrackOut_L().
	 * It is set to a length of #MAX_INSTRUMENTS.
	 */
	jack_port_t*			track_output_ports_L[MAX_INSTRUMENTS];
	/**
	 * Vector of all right audio output ports currently used by the
	 * local JACK client. 
	 *
	 * They will be initialized with all zeros in both
	 * JackAudioDriver(), deactivate(), and connect(). Individual
	 * components will be created, renamed, or reassigned in
	 * setTrackOutput(), deleted in makeTrackOutputs(), and
	 * accessed via getTrackOut_R().
	 * It is set to a length of #MAX_INSTRUMENTS.
	 */
	jack_port_t*		 	track_output_ports_R[MAX_INSTRUMENTS];

	/**
	 * Current transport state returned by
	 * _jack_transport_query()_ (jack/transport.h).  
	 *
	 * It is valid for the entire cycle and can have five
	 * different values:
	 * - _JackTransportStopped_ = 0 : Transport is halted
	 * - _JackTransportRolling_ = 1 : Transport is playing
	 * - _JackTransportLooping_ = 2 : For OLD_TRANSPORT, now
	 *   ignored
	 * - _JackTransportStarting_ = 3 : Waiting for sync ready 
	 * - _JackTransportNetStarting_ = 4 : Waiting for sync ready
	 *   on the network
	 *
	 * The actual number of states depends on your JACK
	 * version. The listing above was done for version 1.9.12.
	 */
	jack_transport_state_t		m_JackTransportState;
	/**
	 * Current transport position obtained using
	 * _jack_transport_query()_ (jack/transport.h).
	 *
	 * It corresponds to the first frame of the current cycle. If
	 * it is NULL, _jack_transport_query()_ won't return any
	 * position information.
	 *
	 * The __valid__ member of #m_JackTransportPos will show which
	 * fields contain valid data. Thus, if it is set to
	 * _JackPositionBBT_, bar, beat, and tick information are
	 * provided by the current timebase master in addition to the
	 * transport information in frames. It is of class
	 * _jack_position_bits_t_ (jack/types.h) and is an enumerator
	 * with five different options:
	 * - _JackPositionBBT_ = 0x10 : Bar, Beat, Tick 
	 * - _JackPositionTimecode_ = 0x20 : External timecode 
	 * - _JackBBTFrameOffset_ = 0x40 : Frame offset of BBT
	 *   information
	 * - _JackAudioVideoRatio_ = 0x80 : audio frames per video
	 *   frame
	 * - _JackVideoFrameOffset_ = 0x100 : frame offset of first
	 *   video frame
	 *
	 * The __frame__ member contains the current transport
	 * position. 
	 *
	 * It is set in updateTransportInfo(). Please see the
	 * documentation of jack_timebase_callback() for more
	 * information about its different members.
	 */
	jack_position_t			m_JackTransportPos;

	/**
	 * Probably tells whether or not the output ports of the
	 * current session ARE already properly connected in the JACK
	 * server. Or maybe if they SHOULD be connected.
	 *
	 * After the JackAudioDriver has been created by
	 * createDriver(), Preferences::m_bJackConnectDefaults will be
	 * used to initialize this variable.
	 */
	bool				m_bConnectOutFlag;
	/**
	 * Specifies whether to use a conditional take over in the
	 * switching of the JACK timebase master. If set to non-zero
	 * the take over will fail if there is already a timebase
	 * master present. It will be initialized with 0 in init().
	 */
	int				m_nJackConditionalTakeOver;

};

}; // H2Core namespace


#else // H2CORE_HAVE_JACK
// JACK is disabled

namespace H2Core {
class JackAudioDriver : public NullDriver {
public:
	/** \return #m_sClassName*/
	static const char* className() { return m_sClassName; }
	/**
	 * Fallback version of the JackAudioDriver in case
	 * #H2CORE_HAVE_JACK was not defined during the configuration
	 * and the usage of the JACK audio server is not intended by
	 * the user.
	 */
	JackAudioDriver( audioProcessCallback processCallback ) : NullDriver( processCallback ) {}
private:
	/** Contains the name of the class.
	 *
	 * This variable allows from more informative log messages
	 * with the name of the class the message is generated in
	 * being displayed as well. Queried using className().*/
	static const char* m_sClassName;

};

}; // H2Core namespace


#endif // H2CORE_HAVE_JACK

#endif

