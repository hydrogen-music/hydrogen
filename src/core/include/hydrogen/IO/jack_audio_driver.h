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
 * __Transport Control__:
 *
 * Each JACK client can start and stop the transport or relocate the
 * current transport position. The request will take place in
 * cycles. During the first the status of the transport changes to
 * _JackTransportStarting_ to inform all clients a change is about to
 * happen. During the second the status is again
 * _JackTransportRolling_ and the transport position is updated
 * according to the request. The current timebase master (see below),
 * if present, needs another cycle to update the additional transport
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
 * __Timebase Master__:
 *
 * The timebase master is responsible to update additional information
 * in the transport information of the JACK server apart from the
 * transport position in frames (see TransportInfo::m_nFrames if you
 * aren't familiar with frames), like the current beat, bar, tick,
 * tick size, speed etc. Every client can be registered as timebase
 * master by supplying a callback (for Hydrogen this would be
 * JackTimebaseCallback()) but there can be at most one timebase
 * master at a time. Having none at all is perfectly fine too. Apart
 * from this additional responsibility, the registered client has no
 * other rights compared to others.
 *
 * After the status of the JACK transport has changed from
 * _JackTransportStarting_ to _JackTransportRolling_, the timebase
 * master needs an additional cycle to update its information.
 *
 * Having an external timebase master present will change the general
 * behavior of Hydrogen. All local tempo settings on the Timeline will
 * be disregarded and the tempo broadcasted by the JACK server will be
 * used instead.
 *
 * This object will only be accessible if #H2CORE_HAVE_JACK was defined
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
	jack_client_t* m_pClient;
	/**
	 * Constructor of the JACK server driver.
	 *
	 * @param m_processCallback Prototype for the client supplied
			 function that is called by the engine anytime there is
			 work to be done. It gets two input arguments _nframes_ of
			 type jack_nframes_t, which specifies the number of frames
			 to process, and a void pointer called _arg_, which points
			 to a client supplied structure.  Two preconditions do act
			 on the _nframes_ argument: _nframes_ == getBufferSize()
			 and _nframes_ == pow(2,x) It returns zero on success and
			 non-zero on error.
	 */
	JackAudioDriver( JackProcessCallback m_processCallback );
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
	 * #m_pTrackOutputPortsL and #m_pTrackOutputPortsR with zeros. If
	 * the #m_bConnectDefaults variable is true or LashClient is used
	 * and Hydrogen is not within a new Lash project, the function
	 * attempts to connect the #m_pOutputPort1 port with
	 * #m_sOutputPortName1 and the #m_pOutputPort2 port with
	 * #m_sOutputPortName2. To establish the connection,
	 * _jack_connect()_ (jack/jack.h) will be used. In case this was
	 * not successful, the function will look up all ports containing
	 * the _JackPortIsInput_ (jack/types.h) flag using
	 * _jack_get_ports()_ (jack/jack.h) and attempts to connect to the
	 * first two it found.
	 *
	 * @return 
	 * - __0__ : if either the connection of the output ports did
	 *       work, two ports having the _JackPortIsInput_ flag where
	 *       found and the #m_pOutputPort1 and #m_pOutputPort2 ports
	 *       where successfully connected to them, or the user enabled
	 *       Lash during compilation and an established project was
	 *       used.
	 * - __1__ : The activation of the JACK client using
	 *       _jack_activate()_ (jack/jack.h) did fail.
	 * - __2__ : The connections to #m_sOutputPortName1 and
	 *       #m_sOutputPortName2 could not be established and the
	 *       there were either no JACK ports holding the
	 *       JackPortIsInput flag found or no connection to them could
	 *       be established.
	 */
	int connect();
	/**
	 * Disconnects the JACK client of the Hydrogen from the JACK
	 * server.
	 *
	 * Firstly, it calls deactivate(). Then, it closes the connection
	 * between the JACK server and the local client using
	 * jack_client_close (jack/jack.h), and sets the #m_pClient
	 * pointer to nullptr.
	 */
	void disconnect();
	/**
	 * Deactivates the JACK client of Hydrogen and disconnects all
	 * ports belonging to it.
	 *
	 * It calls the _jack_deactivate()_ (jack/jack.h) function on the
	 * current client #m_pClient and overwrites the memory allocated
	 * by #m_pTrackOutputPortsL and #m_pTrackOutputPortsR with zeros.
	 */
	void deactivate();
	/** \return Global variable #jackServerBufferSize. */
	unsigned getBufferSize();
	/** \return Global variable #jackServerSampleRate. */
	unsigned getSampleRate();
	/** Accesses the number of output ports currently in use.
	 * \return #m_nTrackPortCount */
	int getNumTracks();

	/** \return #m_JackTransportState */
	jack_transport_state_t getTransportState() {
		return m_JackTransportState;
	}
	/** \return #m_JackTransportPos */
	jack_position_t getTransportPos() {
		return m_JackTransportPos;
	}
	
	/**
	 * Creates per component output ports for each instrument.
	 *
	 * Firstly, it resets #m_trackMap with zeros. Then, it loops
	 * through all the instruments and their components, creates a new
	 * output or resets an existing one for each of them using
	 * setTrackOutput(), and stores the corresponding track number in
	 * #m_trackMap. Finally, all ports in #m_pTrackOutputPortsL and
	 * #m_pTrackOutputPortsR, which haven't been used in the previous
	 * step, are unregistered using _jack_port_unregister()_
	 * (jack/jack.h) and overwritten with 0. #m_nTrackPortCount will
	 * be set to biggest track number encountered during the
	 * creation/reassignment step.
	 *
	 * The function will only perform its tasks if the
	 * Preferences::m_bJackTrackOuts is set to true.
	 */
	void makeTrackOutputs( Song* pSong );
	/**
	 * Renames the @a n 'th port of JACK client and creates it if
	 * it's not already present. 
	 *
	 * If the track number @a n is bigger than the number of ports
	 * currently in usage #m_nTrackPortCount, @a n + 1 -
	 * #m_nTrackPortCount new stereo ports will be created using
	 * _jack_port_register()_ (jack/jack.h) and #m_nTrackPortCount
	 * updated to @a n + 1.
	 *
	 * Afterwards, the @a n 'th port is renamed to a concatenation of
	 * "Track_", DrumkitComponent::__name, "_", @a n + 1, "_",
	 * Instrument::__name, and "_L", or "_R" using either
	 * _jack_port_rename()_ (if HAVE_JACK_PORT_RENAME is defined) or
	 * _jack_port_set_name()_ (both jack/jack.h). The former renaming
	 * function triggers a _PortRename_ notifications to clients that
	 * have registered a port rename handler.
	 *
	 * \param n Track number for which a port should be renamed
	 *   (and created).
	 * \param instr Pointer to the corresponding Instrument.
	 * \param pCompo Pointer to the corresponding
	 *   InstrumentComponent.
	 * \param pSong Pointer to the corresponding Song.
	 */
	void setTrackOutput( int n, Instrument* instr, InstrumentComponent* pCompo, Song* pSong );

	/** \param flag Sets #m_bConnectDefaults*/
	void setConnectDefaults( bool flag ) {
		m_bConnectDefaults = flag;
	}
	/** \return #m_bConnectDefaults */
	bool getConnectDefaults() {
		return m_bConnectDefaults;
	}

	/**
	 * Get content in the left stereo output port.
	 *
	 * It calls _jack_port_get_buffer()_ (jack/jack.h) with both the
	 * port name #m_pOutputPort1 and buffer size
	 * #jackServerBufferSize.
	 *
	 * \return Pointer to buffer content of type
	 * _jack_default_audio_sample_t*_ (jack/types.h)
	 */
	float* getOut_L();
	/**
	 * Get content in the right stereo output port.
	 *
	 * It calls _jack_port_get_buffer()_ (jack/jack.h) with both the
	 * port name #m_pOutputPort2 and buffer size
	 * #jackServerBufferSize.
	 *
	 * \return Pointer to buffer content of type
	 * _jack_default_audio_sample_t*_ (jack/types.h)
	 */
	float* getOut_R();
	/**
	 * Get content of left output port of a specific track.
	 *
	 * It calls _jack_port_get_buffer()_ (jack/jack.h) with the port
	 * in the @a nTrack element of #m_pTrackOutputPortsL and buffer
	 * size #jackServerBufferSize.
	 *
	 * \param nTrack Track number. Must not be bigger than
	 * #m_nTrackPortCount.
	 *
	 * \return Pointer to buffer content of type
	 * _jack_default_audio_sample_t*_ (jack/types.h)
	 */
	float* getTrackOut_L( unsigned nTrack );
	/**
	 * Get content of right output port of a specific track.
	 *
	 * It calls _jack_port_get_buffer()_ (jack/jack.h) with the port
	 * in the @a nTrack element of #m_pTrackOutputPortsR and buffer
	 * size #jackServerBufferSize.
	 *
	 * \param nTrack Track number. Must not be bigger than
	 * #m_nTrackPortCount.
	 *
	 * \return Pointer to buffer content of type
	 * _jack_default_audio_sample_t*_ (jack/types.h)
	 */
	float* getTrackOut_R( unsigned nTrack );
	/** 
	 * Convenience function looking up the track number of a component
	 * of an instrument using in #m_trackMap using their IDs
	 * Instrument::__id and
	 * InstrumentComponent::__related_drumkit_componentID. Using the
	 * track number it then calls getTrackOut_L( unsigned ) and
	 * returns its result.
	 *
	 * \param instr Pointer to an Instrument
	 * \param pCompo Pointer to one of the instrument's components.
	 *
	 * \return Pointer to buffer content of type
	 * _jack_default_audio_sample_t*_ (jack/types.h)
	 */
	float* getTrackOut_L( Instrument* instr, InstrumentComponent* pCompo );
	/** 
	 * Convenience function looking up the track number of a component
	 * of an instrument using in #m_trackMap using their IDs
	 * Instrument::__id and
	 * InstrumentComponent::__related_drumkit_componentID. Using the
	 * track number it then calls getTrackOut_R( unsigned ) and
	 * returns its result.
	 *
	 * \param instr Pointer to an Instrument
	 * \param pCompo Pointer to one of the instrument's components.
	 *
	 * \return Pointer to buffer content of type
	 * _jack_default_audio_sample_t*_ (jack/types.h)
	 */
	float* getTrackOut_R( Instrument* instr, InstrumentComponent* pCompo );

	/**
	 * Initializes the JACK audio driver.
	 *
	 * Firstly, it determines the destination ports
	 * #m_sOutputPortName1 and #m_sOutputPortName2 the output ports of
	 * Hydrogen will be connected to in connect() from
	 * Preferences::m_sJackPortName1 and
	 * Preferences::m_sJackPortName2. The name of the JACK client is
	 * either set to "Hydrogen" or, if #H2CORE_HAVE_OSC was defined
	 * during compilation and OSC support is enabled, to
	 * Preferences::m_sNsmClientId via Preferences::getNsmClientId().
	 *
	 * Next, the function attempts to open an external client session
	 * with the JACK server using _jack_client_open()_ (jack/jack.h)
	 * and saves it to the pointer #m_pClient. In case this didn't
	 * work properly, it will start two more attempts. Sometime JACK
	 * doesn't stop and start fast enough. If the compiler flag
	 * #H2CORE_HAVE_JACKSESSION was set and the user enabled the usage
	 * of JACK session, the client will be opened using the
	 * corresponding option and the sessionID Token
	 * Preferences::jackSessionUUID, obtained via
	 * Preferences::getJackSessionUUID(), will be provided so the
	 * sessionmanager can identify the client again.
	 *
	 * If the client was opened properly, the function will get its
	 * sample rate using _jack_get_sample_rate()_ and buffer size
	 * using _jack_get_buffer_size()_ (both jack/jack.h) and stores
	 * them in #jackServerSampleRate, Preferences::m_nSampleRate,
	 * #jackServerBufferSize, and Preferences::m_nBufferSize. In
	 * addition, it also registers JackAudioDriver::m_processCallback,
	 * H2Core::jackDriverSampleRate, H2Core::jackDriverBufferSize, and
	 * H2Core::jackDriverShutdown using _jack_set_process_callback()_,
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
	 * LashClient::isConnected() the name of the client will be stored
	 * in LashClient::jackClientName using
	 * LashClient::setJackClientName. If JACK session was enabled, the
	 * jack_session_callback() will be registered using
	 * _jack_set_session_callback()_ (jack/session.h).
	 *
	 * Finally, the function will check whether Hydrogen should be the
	 * JACK timebase master or not via Preferences::m_bJackMasterMode
	 * and calls initTimebaseMaster() if its indeed the case.
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
	 * If the JACK transport was activated in the GUI by clicking
	 * either the "J.TRANS" or "J.MASTER" button, the
	 * _jack_transport_start()_ (jack/transport.h) function will be
	 * called to start the JACK transport. Else, the internal
	 * TransportInfo::m_status will be set to TransportInfo::ROLLING
	 * instead.
	 */
	virtual void play();
	/**
	 * Stops the JACK transport.
	 *
	 * If the JACK transport was activated in the GUI by clicking
	 * either the "J.TRANS" or "J.MASTER" button, the
	 * _jack_transport_stop()_ (jack/transport.h) function will be
	 * called to stop the JACK transport. Else, the internal
	 * TransportInfo::m_status will be set to TransportInfo::STOPPED
	 * instead.
	 */
	virtual void stop();
	/**
	 * Re-positions the transport position to @a nFrame.
	 *
	 * If the Preferences::USE_JACK_TRANSPORT mode is chosen in
	 * Preferences::m_bJackTransportMode, the
	 * _jack_transport_locate()_ (jack/transport.h) function will be
	 * used to request the new transport position. If not, @a nFrame
	 * will be assigned to TransportInfo::m_nFrames of the local
	 * instance of the TransportInfo AudioOutput::m_transport.
	 *
	 * The new position takes effect in two process cycles during
	 * which JACK's state will be in JackTransportStarting and the
	 * transport won't be rolling.
	 *   
	 * \param nFrame Requested new transport position.
	 */
	virtual void locate( unsigned long nFrame );
	/**
	 * Updating the local instance of the TransportInfo
	 * AudioOutput::m_transport.
	 *
	 * The function queries the transport position and additional
	 * information from the JACK server, writes them to
	 * #m_JackTransportPos and in #m_JackTransportState, and updates
	 * the information stored in AudioOutput::m_transport in case of a
	 * mismatch.
	 *
	 * If #m_JackTransportState is either _JackTransportStopped_ or
     * _JackTransportStarting_, transport will be (temporarily)
     * stopped - TransportInfo::m_status will be set to
     * TransportInfo::STOPPED. If it's _JackTransportRolling_,
     * transport will be started - TransportInfo::m_status will be set
     * to TransportInfo::ROLLING.
	 *
     * The function will check whether a relocation took place by the
	 * JACK server and (afterwards) whether the current tempo did
	 * change with respect to the last transport cycle. In case of a
	 * relocation, #m_frameOffset will be reset to 0,
	 * TransportInfo::m_nFrames updated to the new value provided by
	 * JACK, and - if Hydrogen is in Song::PATTERN_MODE - the playback
	 * moved to the beginning of the pattern. A change in speed, on
	 * the other hand, depends on the local JACK setup. If there is a
	 * timebase master, which broadcasts tempo information via JACK,
	 * and it's not Hydrogen itself, all customizations in the
	 * Timeline will be disregarded and the tempo of the master will
	 * be used instead. If Hydrogen is the timebase master or there is
	 * none at all, Hydrogen::setTimelineBpm() will be used to keep
	 * the transport tempo aligned the settings in the Timeline.
	 *
	 * If Preferences::USE_JACK_TRANSPORT was not selected in
	 * Preferences::m_bJackTransportMode, the function will return
	 * without performing any action.
	 */
	virtual void updateTransportInfo();
	/** Set the tempo stored TransportInfo::m_fBPM of the local
	 * instance of the TransportInfo AudioOutput::m_transport.
	 *
	 * Only sets the tempo to @a fBPM if its value is at least
	 * 1. Sometime (especially during the first cycle after locating
	 * with transport stopped) the JACK server sends some artifacts
	 * (6.95334e-310) which should not be assigned.
	 * 
	 * \param fBPM new tempo. 
	 */
	virtual void setBpm( float fBPM );
	/**
	 * Calculates the difference between the true transport position
	 * and the internal one.
	 *
	 * The internal transport position used in most parts of Hydrogen
	 * is given in ticks. But since the size of a tick is
	 * tempo-dependent, passing a tempo marker in the Timeline will
	 * cause the corresponding internal transport position in frames
	 * to diverge from the external one by a constant offset. This
	 * function will calculate and store it in #m_frameOffset.
	 *
	 * \param oldFrame Provides the previous transport position in
	 * frames prior to the change in tick size. This is required if
	 * transport is not rolling during the relocation into a region of
	 * different speed since there is no up-to-date JACK query
	 * providing these information.
	 */
	void calculateFrameOffset(long long oldFrame);

	/**
	 * Registers Hydrogen as JACK timebase master.
	 *
	 * If for some reason registering Hydrogen as timebase master does
	 * not work, the function sets Preferences::m_bJackMasterMode to
	 * Preferences::NO_JACK_TIME_MASTER.
	 *
	 * If the function is called with Preferences::m_bJackMasterMode
	 * set to Preferences::NO_JACK_TIME_MASTER,
	 * releaseTimebaseMaster() will be called instead.
	 */ 
	void initTimebaseMaster();
	/**
	 * Calls _jack_release_timebase()_ (jack/transport.h) to release
	 * Hydrogen from the JACK timebase master responsibilities. This
	 * causes the JackTimebaseCallback() callback function to not be
	 * called by the JACK server anymore.
	 */
	void releaseTimebaseMaster();
	
	/**
	 * \return #m_nIsTimebaseMaster
	 */
	int getIsTimebaseMaster() const;
	/** Stores the latest transport position (for both rolling and
	 * stopped transport).
	 *
	 * In case the user is clicking on the
	 * SongEditor::mousePressEvent() will trigger both a relocation
	 * and a change in speed. The change in speed causes the
	 * audioEngine_checkBPMChange() function to update the ticksize in
	 * case transported got moved into a region of different tempo and
	 * triggers the calculateFrameOffset() function. But the latter
	 * can only work properly if transport is rolling since it has to
	 * know the frame position prior to the change in tick size and
	 * there is no up-to-date JACK query providing this information.
	 */
	int m_currentPos;
protected:
	/**
	 * Callback function registered to the JACK server in
	 * initTimebaseMaster() if Hydrogen is set as JACK timebase master.
	 *
	 * It will update the current position not just in frames till
	 * the beginning of the song, but also in terms of beat, bar,
	 * and tick values. 
	 *
	 * The function it will be called after the
	 * audioEngine_process() function and only if the
	 * #m_JackTransportState is _JackTransportRolling_.
	 *
	 * \param state Current transport state. Not used within the
	 * function but to ensure compatibility.
	 * \param nFrames Buffer size. Not used within the function but
	 * to ensure compatibility.
	 * \param pJackPosition Current transport position.
	 * \param new_pos Updated transport position in frames. Not
	 * used within the function but to ensure compatibility.
	 * \param arg Pointer to a JackAudioDriver instance.
	 */
	static void JackTimebaseCallback( jack_transport_state_t state,
					    jack_nframes_t nFrames,
					    jack_position_t* pJackPosition,
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
	/**
	 * Constant offset between the internal transport position in
	 * TransportInfo::m_nFrames and the external one.
	 *
	 * Imagine the following setting: During the playback you decide
	 * to change the speed of the song. This would cause a lot of
	 * position information within Hydrogen, which are given in ticks,
	 * to be off since the tick size depends on the speed and just got
	 * changed too. Instead, TransportInfo::m_nFrames will scaled to
	 * reflect the changes and everything will be still in place with
	 * the user to not note a single thing. Unfortunately, now the
	 * transport position in frames of the audio engine and of the
	 * JACK server are off by a constant offset. To nevertheless be
	 * able to identify relocation in updateTransportInfo(), this
	 * constant offset is stored in this variable and used in
	 * updateTransportInfo() to determine whether a relocation did
	 * happen.
	 *
	 * Positive values correspond to a position ahead of the current
	 * transport information. The variable is initialized with 0 in
	 * JackAudioDriver() and updated in calculateFrameOffset().
	 */
	long long			m_frameOffset;
	/**
	 * Function the JACK server will call whenever there is work to
	 * do.
	 *
	 * The audioEngine_process() function will be used and registered
	 * using _jack_set_process_callback()_ (jack/jack.h) in init().
	 *
	 * The code must be suitable for real-time execution.  That means
	 * that it cannot call functions that might block for a long
	 * time. This includes _malloc()_, _free()_, _printf()_,
	 * _pthread_mutex_lock()_, _sleep()_, _wait()_, _poll()_,
	 * _select()_, _pthread_join()_, _pthread_cond_wait()_, etc, etc.
	 */
	JackProcessCallback		m_processCallback;
	/**
	 * Left source port for which a connection to #m_sOutputPortName1
	 * will be established in connect() via the JACK server.
	 */
	jack_port_t*			m_pOutputPort1;
	/**
	 * Right source port for which a connection to #m_sOutputPortName2
	 * will be established in connect() via the JACK server.
	 */
	jack_port_t*			m_pOutputPort2;
	/**
	 * Destination of the left source port #m_pOutputPort1, for which
	 * a connection will be established in connect(). It is set to
	 * Preferences::m_sJackPortName1 during the call of init().
	 */
	QString				m_sOutputPortName1;
	/**
	 * Destination of the right source port #m_pOutputPort2, for which
	 * a connection will be established in connect(). It is set to
	 * Preferences::m_sJackPortName2 during the call of init().
	 */
	QString				m_sOutputPortName2;
	/**
	 * Matrix containing the track number of each component of of all
	 * instruments. Its rows represent the instruments and its columns
	 * their components. _m_trackMap[2][1]=6_ thus therefore mean the
	 * output of the second component of the third instrument is
	 * assigned the seventh output port. Since its total size is
	 * defined by #MAX_INSTRUMENTS and #MAX_COMPONENTS, most of its
	 * entries will be zero.
	 *
	 * It gets updated by makeTrackOutputs().
	 */
	int				m_trackMap[MAX_INSTRUMENTS][MAX_COMPONENTS];
	/**
	 * Total number of output ports currently in use. It gets updated
	 * by makeTrackOutputs().
	 */
	int				m_nTrackPortCount;
	/**
	 * Vector of all left audio output ports currently used by the
	 * local JACK client.
	 *
	 * They will be initialized with all zeros in both
	 * JackAudioDriver(), deactivate(), and connect(). Individual
	 * components will be created, renamed, or reassigned in
	 * setTrackOutput(), deleted in makeTrackOutputs(), and accessed
	 * via getTrackOut_L().  It is set to a length of
	 * #MAX_INSTRUMENTS.
	 */
	jack_port_t*			m_pTrackOutputPortsL[MAX_INSTRUMENTS];
	/**
	 * Vector of all right audio output ports currently used by the
	 * local JACK client.
	 *
	 * They will be initialized with all zeros in both
	 * JackAudioDriver(), deactivate(), and connect(). Individual
	 * components will be created, renamed, or reassigned in
	 * setTrackOutput(), deleted in makeTrackOutputs(), and accessed
	 * via getTrackOut_R().  It is set to a length of
	 * #MAX_INSTRUMENTS.
	 */
	jack_port_t*		 	m_pTrackOutputPortsR[MAX_INSTRUMENTS];

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
	 * It corresponds to the first frame of the current cycle. If it
	 * is NULL, _jack_transport_query()_ won't return any position
	 * information.
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
	 * documentation of JackTimebaseCallback() for more information
	 * about its different members.
	 */
	jack_position_t			m_JackTransportPos;

	/**
	 * Specifies whether the default left and right (master) audio
	 * JACK ports will be automatically connected to the system's sink
	 * when registering the JACK client in connect().
	 *
	 * After the JackAudioDriver has been created by createDriver(),
	 * the variable will be, again, set to
	 * Preferences::m_bJackConnectDefaults.
	 */
	bool				m_bConnectDefaults;
	/**
	 * Whether Hydrogen or another program is Jack timebase master.
	 *
	 * - #m_nIsTimebaseMaster > 0 - Hydrogen itself is timebase
          master.
	 * - #m_nIsTimebaseMaster == 0 - an external program is timebase
          master and Hydrogen will disregard all tempo marker on the
          Timeline and, instead, only use the BPM provided by JACK.
	 * - #m_nIsTimebaseMaster < 0 - only normal clients registered.
	 *
	 * While Hydrogen can unregister as timebase master on its own, it
	 * can not be observed directly whether another application has
	 * taken over as timebase master. When the JACK server is
	 * releasing Hydrogen in the later case, it won't advertise this
	 * fact but simply won't call the JackTimebaseCallback()
	 * anymore. But since this will be called in every cycle after
	 * updateTransportInfo(), we can use this variable to determine if
	 * Hydrogen is still timebase master.
	 *
	 * As Hydrogen registered as timebase master using
	 * initTimebaseMaster() it will be initialized with 1, decremented
	 * in updateTransportInfo(), and reset to 1 in
	 * JackTimebaseCallback(). Whenever it is zero in
	 * updateTransportInfo(), #m_nIsTimebaseMaster will be updated
	 * accordingly.
	 */
	int				m_nIsTimebaseMaster;

};
	
inline int JackAudioDriver::getIsTimebaseMaster() const {
	return m_nIsTimebaseMaster;
}
inline int JackAudioDriver::getNumTracks() {
	return m_nTrackPortCount;
}



}; // H2Core namespace


#else // H2CORE_HAVE_JACK
// JACK is disabled

namespace H2Core {
class JackAudioDriver : public NullDriver {
	H2_OBJECT
public:
	/**
	 * Fallback version of the JackAudioDriver in case
	 * #H2CORE_HAVE_JACK was not defined during the configuration
	 * and the usage of the JACK audio server is not intended by
	 * the user.
	 */
	JackAudioDriver( audioProcessCallback m_processCallback ) : NullDriver( m_processCallback ) {}

};

}; // H2Core namespace


#endif // H2CORE_HAVE_JACK

#endif

