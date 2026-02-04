/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef H2_JACK_DRIVER_H
#define H2_JACK_DRIVER_H

#include <core/IO/AudioDriver.h>
#include <core/IO/MidiBaseDriver.h>
#include <core/IO/NullDriver.h>

// check if jack support is disabled
#if defined( H2CORE_HAVE_JACK ) || _DOXYGEN_
// JACK support is enabled.

#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>
#include <jack/transport.h>
#include <map>
#include <memory>
#include <mutex>

#include <core/Globals.h>

namespace H2Core {

class Drumkit;
class Instrument;
class Song;
class TransportPosition;

/**
 * JACK (Jack Audio Connection Kit) server driver. A holistic driver, which can
 * be used both as audio and MIDI driver.
 *
 * Due to its design the JackDriver is a special case within Hydrogen. All other
 * drivers are either audio or MIDI ones but this one can be either of them or
 * both at the same time. Which of those options is used, will be determined
 * with the constructor and stored as #JackDriver::Mode. Starting the driver via
 * both the audio and the MIDI interface results in the same setup.
 *
 * __Transport Control__:
 *
 * Each JACK client can start and stop the transport or relocate the current
 * transport position. The request will take place in cycles. During the first
 * the status of the transport changes to _JackTransportStarting_ to inform all
 * clients a change is about to happen. During the second the status is again
 * _JackTransportRolling_ and the transport position is updated according to the
 * request.
 *
 * Also note that Hydrogen overwrites its local TransportPosition only with the
 * transport position of the JACK server if there is a mismatch due to a
 * relocation triggered by another JACK client. During normal transport the
 * current position TransportPosition::m_nFrames will be always the same as the
 * one of JACK during a cycle and incremented by the buffer size in
 * audioEngine_process() at the very end of the cycle. The same happens for the
 * transport information of the JACK server but in parallel.
 *
 * __Timebase support__:
 *
 * The JACK Timebase controller is responsible for providing additional
 * transport information to the JACK server apart from the transport position in
 * frames, like current beat, bar, tick, tick size, speed etc. Unlike many other
 * application, Hydrogen does _not_ respond to changes in measure since these
 * would have to be mapped to the length of the current pattern (In case this
 * leads to repeated glitches or unwanted behavior Timebase synchronization can
 * be turned off entirely using #Preferences::m_bJackTimebaseEnabled). Every
 * client can take control by supplying a callback (for Hydrogen this would be
 * JackTimebaseCallback()) but there can be at most one Timebase controller at a
 * time. Having none at all is perfectly fine too. Apart from this additional
 * responsibility, the registered client has no other rights compared to others.
 *
 * After the status of the JACK transport has changed from
 * _JackTransportStarting_ to _JackTransportRolling_, the Timebase controller
 * needs an additional cycle to update its information.
 *
 * Note that we do not use the original terms coined in the Timebase API.
 * Instead, we refer to controller and listener.
 */
/** \ingroup docCore docAudioDriver docMIDI */
class JackDriver : public Object<JackDriver>,
				   public MidiBaseDriver,
				   public AudioDriver {
	H2_OBJECT( JackDriver )
   public:
	/** Whether Hydrogen or another program is in Timebase control. */
	enum class Mode {
		None,
		/** Only the audio part of the driver is used. */
		Audio,
		/** Only the MIDI part of the driver is used. */
		Midi,
		/** Both audio and MIDI part of the driver are used. */
		Combined
	};
	static QString ModeToQString( const Mode& m );

	/** Whether Hydrogen or another program is in Timebase control. */
	enum class Timebase {
		/** Hydrogen itself is Timebase controller and provides its current
		 * tempo to other Timebase listeners.*/
		Controller = 1,
		/** An external program is Timebase controller and Hydrogen will
		 * disregard all tempo markers on the Timeline and, instead, only use
		 * the BPM provided by JACK.
		 *
		 * Note: the JACK standard is using a different term we do not want to
		 * repeat or spread. */
		Listener = 0,
		/** Only normal clients registered */
		None = -1
	};
	static QString TimebaseToQString( const Timebase& t );
	static Timebase TimebaseFromInt( int nState );

	enum class Channel { Left, Right };

	struct InstrumentPorts {
		InstrumentPorts();
		InstrumentPorts( const InstrumentPorts& other );

		/** When switching kits/manipulating instruments we check whether they
		 * are currently used to render a sample. If so, they are added to the
		 * Hydrogen::m_instrumentDeathRow. This way they can live on until all
		 * rendering using their samples is done and we avoid audible glitches
		 * when switching kits.
		 *
		 * But when using per-track JACK audio output ports we have to ensure
		 * too, that the ports associated with those instruments are kept open
		 * till all rendering is done. Instruments which's ports are mapped to
		 * the ones of another instrument will continue using those ports (but
		 * we must not remove them since the new instruments are still using
		 * them - Marked::ForRemoval). Instruments not having a corresponding
		 * mapping target will keep their ports till all rendering is done
		 * (Marked::ForDeath). See cleanupPerTrackPorts(). */
		enum class Marked { ForDeath, ForRemoval, None };

		QString sPortNameBase;
		jack_port_t* Left;
		jack_port_t* Right;
		Marked marked;
	};

	/** Maximum number of supported event. */
	static constexpr uint32_t jackMidiBufferMax = 64;

	static double bbtToTick( const jack_position_t& pos );
	static bool isBBTValid( const jack_position_t& pos );
	static void transportToBBT(
		const TransportPosition& transportPos,
		jack_position_t* pPos
	);
	static QString JackTransportPosToQString( const jack_position_t& pPos );

	typedef std::map<std::shared_ptr<Instrument>, InstrumentPorts> PortMap;

	JackDriver( JackProcessCallback m_processCallback );
	~JackDriver();

	/** Reports whether the driver was successfully activate and is usable. */
	bool isActive() const;

	void deactivate();
	float* getTrackBuffer(
		std::shared_ptr<Instrument> pInstrument,
		Channel channel
	) const;
	Mode getMode() const;

	/** Re-positions the transport position to @a nFrame.
	 *
	 * The new position takes effect in two process cycles during which JACK's
	 * state will be in JackTransportStarting and the transport won't be
	 * rolling.
	 *
	 * \param nFrame Requested new transport position.
	 */
	void locateTransport( long long nFrame );
	/** Tells the JACK server to start transport. */
	void startTransport();
	/** Tells the JACK server to stop transport. */
	void stopTransport();
	/**
	 * The function queries the transport position and additional information
	 * from the JACK server, writes them to #m_JackTransportPos and in
	 * #m_JackTransportState, and updates the AudioEngine in case of a mismatch.
	 */
	void updateTransportPosition();

	/** \return the BPM reported by the current (external) Timebase controller
		or NAN if there is none.*/
	float getTimebaseControllerBpm() const;
	Timebase getTimebaseState() const;
	/** Acquires control of JACK Timebase. */
	void initTimebaseControl();
	/** Release Hydrogen from the JACK Timebase control.
	 *
	 * This causes the JackTimebaseCallback() callback function to not be called
	 * by the JACK server anymore. */
	void releaseTimebaseControl();
	/** Uses the bar-beat-tick information to relocate the transport position.
	 *
	 * This type of operation is triggered whenever the transport position gets
	 * relocated or the tempo is changed using JACK in the presence of an
	 * external Timebase controller. */
	void relocateUsingBBT();

	const jack_position_t& getJackPosition() const;

	/** Implementation of the AudioOuput interface @{*/
	/**
	 * Initializes the JACK audio driver.
	 *
	 * \param bufferSize Unused and only present to assure compatibility with
	 * the JACK API.
	 *
	 * \return
	 * -  __0__ : on success.
	 * - __-1__ : if the pointer #m_pClient obtained via
	 * _jack_client_open()_ (jack/jack.h) is 0.
	 * - __4__ : unable to register the "out_L" and/or "out_R"
	 * output ports for the JACK client using
	 * _jack_port_register()_ (jack/jack.h).
	 */
	int init( unsigned bufferSize ) override;
	/** Connects to output ports via the JACK server.
	 *	 *
	 * @return
	 * - __0__ : on success.
	 * - __1__ : The activation of the JACK client using
	 *           did fail.
	 * - __2__ : The connections to #m_sOutputPortName1 and
	 *       #m_sOutputPortName2 could not be established and the
	 *       there were either no JACK ports holding the
	 *       JackPortIsInput flag found or no connection to them could
	 *       be established.
	 */
	int connect() override;
	void disconnect() override;
	unsigned getBufferSize() override;
	unsigned getSampleRate() override;
	int getXRuns() const override;
	float* getOut_L() override;
	float* getOut_R() override;
	/** @} */

	/** Members required to implement the Audio part of the driver. @{ */
	/** Callback function for the JACK audio server to set the buffer size
	 * #jackServerBufferSize.
	 *
	 * \param nframes New buffer size.
	 * \param pInstance current instance of JackDriver.
	 *
	 * @return 0 on success */
	static int jackDriverBufferSize( jack_nframes_t nframes, void* pInstance );
	/** Callback function for the JACK audio server to set the sample
	 * rate #jackServerSampleRate.
	 *
	 * \param nframes New sample rate.
	 * \param pInstance current instance of JackDriver.
	 *
	 * @return 0 on success */
	static int jackDriverSampleRate( jack_nframes_t nframes, void* pInstance );
	/** Report an XRun event to the GUI.*/
	static int jackXRunCallback( void* pInstance );
	/** Checks whether there are ports associated with instrument in
	 * #Hydrogen::m_instrumentDeathRow and whether they can be torn down. */
	void cleanUpPerTrackAudioPorts();
	void clearPerTrackAudioBuffers( uint32_t nFrames );
	/** In case the previous drumkit is provided as well, a more sophisticated
	 * mapping between the instrument corresponding to the ports can be done. */
	void createPerTrackAudioPorts(
		std::shared_ptr<Song> pSong,
		std::shared_ptr<Drumkit> pOldDrumkit = nullptr
	);
	bool getConnectDefaults() { return m_bConnectDefaults; }
	void setConnectDefaults( bool flag ) { m_bConnectDefaults = flag; }
	/** @} */

	/** Methods to implement #MidiBaseDriver @{ */
	void close() override;
	std::vector<QString> getExternalPortList( const PortType& portType
	) override;
	bool isInputActive() const override;
	bool isOutputActive() const override;
	void open() override;
	/** @} */

	/** Methods handling the MIDI part of the driver @{ */
	void getPortInfo( const QString& sPortName, int& nClient, int& nPort );
	void handleJackMidiOutput( jack_nframes_t nframes );
	void handleJackMidiInput( jack_nframes_t nframes );
	/** @} */

	QString toQString( const QString& sPrefix = "", bool bShort = true )
		const override;

	friend class AudioEngineTests;

   private:
	/** Used internally to keep track of the current Timebase state.
	 *
	 * While Hydrogen can drop Timebase control on its own, it can not be
	 * observed directly whether another application has taken over as Timebase
	 * controller. When the JACK server is releasing Hydrogen in the later case,
	 * it won't advertise this fact but simply won't call the
	 * JackTimebaseCallback() anymore. But since this will be called in every
	 * cycle after updateTransportPosition(), we make the former set the
	 * tracking state to #Valid and the latter to #OnHold. If we encounter a
	 * second process cycle with #OnHold, we have been released.
	 *
	 * A second use case is relocation triggered by a JACK client other than the
	 * Timebase controller. The JACK server won't have any corresponding BBT
	 * information at hand and distribute the frame information without the BBT
	 * capability. The process cycle afterwards the controller starts again to
	 * send BBT. This intermediate state will be covered by #OnHold too, as we
	 * do not want the timebase state (and timeline support) to glitch on each
	 * relocation. Instead, we cache the last tempo and pretend to still have
	 * BBT information. will be updated accordingly.
	 */
	enum class TimebaseTracking {
		/** Current timebase state is on par with JACK server. */
		Valid,
		/** We are uncertain of the current timebase state and wait a processing
		 * cycle to determine what to do next.*/
		OnHold,
		/** Null element */
		None
	};
	static QString TimebaseTrackingToQString( const TimebaseTracking& t );

	/**
	 * Callback function for the JACK server to supply additional timebase
	 * information.
	 *
	 * The function it will be called after the audioEngine_process() function
	 * and only if the #m_JackTransportState is _JackTransportRolling_.
	 *
	 * What is the BBT information?
	 *
	 * There is no formal definition in the JACK API but the way it is
	 * interpreted by Hydrogen is the following:
	 *
	 * bar: Number of patterns passed since the beginning of the
	 * song.
	 * beat: Number of quarters passed since the beginning of the
	 *     pattern.
	 * tick: Number of ticks passed since the last beat (with respect
	 *     to the current frame).
	 *
	 * A tick is an internal measure representing the smallest resolution of the
	 * transport position in terms of the patterns. It consist of
	 * pAudioEngine->getTickSize() frames, which changes depending on the
	 * current tempo.
	 *
	 * \param state Unused.
	 * \param nFrames Unused.
	 * \param pJackPosition Current transport position.
	 * \param new_pos Unused.
	 * \param pInstance Pointer to a JackDriver instance.
	 */
	static void JackTimebaseCallback(
		jack_transport_state_t state,
		jack_nframes_t nFrames,
		jack_position_t* pJackPosition,
		int new_pos,
		void* pInstance
	);
	/**
	 * Callback function for the JACK audio server to shutting down the JACK
	 * driver.
	 *
	 * \param arg The current instance of the JackDriver.
	 */
	static void jackDriverShutdown( void* pInstance );

	void unregisterPerTrackAudioPorts( InstrumentPorts ports );

	/** Methods handling the MIDI part of the driver @{ */
	void sendJackMidiMessage( uint8_t* buf, uint8_t len );

	void sendControlChangeMessage( const MidiMessage& msg ) override;
	void sendNoteOnMessage( const MidiMessage& msg ) override;
	void sendNoteOffMessage( const MidiMessage& msg ) override;
	void sendSystemRealTimeMessage( const MidiMessage& msg ) override;
	/** @} */

	static QString JackTransportStateToQString(
		const jack_transport_state_t& pPos
	);

	/** Show debugging information.*/
	void printState() const;

	Mode m_mode;

	QString m_sClientName;

	/** Buffer size of the JACK audio server. */
	jack_nframes_t m_jackServerBufferSize;
	/** Sample rate of the JACK audio server. */
	jack_nframes_t m_jackServerSampleRate;
	/** Number of XRuns since the driver started.*/
	int m_nJackServerXRuns;
	jack_client_t* m_pClient;

	/** Main process callback. */
	JackProcessCallback m_processCallback;
	/** Left source port. */
	jack_port_t* m_pAudioOutputPort1;
	/** Right source port. */
	jack_port_t* m_pAudioOutputPort2;
	/** Destination of the left source port #m_pOutputPort1, for which a
	 * connection will be established in connect(). */
	QString m_sAudioOutputPortName1;
	/** Destination of the right source port #m_pOutputPort2, for which a
	 * connection will be established in connect(). */
	QString m_sAudioOutputPortName2;

	/** The left and right jack port (in that order) associated with a
	 * channel of an instrument. */
	PortMap m_audioPortMap;

	/** Contains the ports for the metronome and the playback track, which do
	 * not change when e.g. switching drumkits or loading a different song. They
	 * will stay till teardown. */
	PortMap m_audioPortMapStatic;

	/** Since #Sampler::m_pPreviewInstrument is changed with each new sample to
	 * preview, this one serves as a dummy instrument mapping all instruments
	 * not found in #m_portMap but with #Instrument::m_bIsPreviewInstrument set
	 * to the per-track output ports of the sample preview. */
	std::shared_ptr<Instrument> m_pDummyPreviewInstrument;

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
	 * The actual number of states depends on your JACK version. The listing
	 * above was done for version 1.9.12.
	 */
	jack_transport_state_t m_JackTransportState;
	/**
	 * Current transport position obtained using _jack_transport_query()_
	 * (jack/transport.h).
	 *
	 * It corresponds to the first frame of the current cycle.
	 *
	 * The __valid__ member of #m_JackTransportPos will show which fields
	 * contain valid data. Thus, if it is set to _JackPositionBBT_, bar, beat,
	 * and tick information are provided by the current Timebase controller in
	 * addition to the transport information in frames. It is of class
	 * _jack_position_bits_t_ (jack/types.h) and is an enumerator with five
	 * different options:
	 *
	 * - _JackPositionBBT_ = 0x10 : Bar, Beat, Tick
	 * - _JackPositionTimecode_ = 0x20 : External timecode
	 * - _JackBBTFrameOffset_ = 0x40 : Frame offset of BBT
	 *   information
	 * - _JackAudioVideoRatio_ = 0x80 : audio frames per video
	 *   frame
	 * - _JackVideoFrameOffset_ = 0x100 : frame offset of first
	 *   video frame
	 */
	jack_position_t m_JackTransportPos;

	/** Use for relocation if Hydrogen is Timebase controller (and needs to
	 * provide valid BBT information in addition to just a frame). */
	jack_position_t m_nextJackTransportPos;

	/**
	 * Specifies whether the default left and right (master) audio JACK ports
	 * will be automatically connected to the system's sink when registering the
	 * JACK client in connect().
	 */
	bool m_bConnectDefaults;

	/**
	 * Whether Hydrogen is receiving relocation and tempo changes as part of BBT
	 * information, it sends them itself, or just uses internal position
	 * information.
	 */
	Timebase m_timebaseState;

	/** Whether the current timebase state is stable or about to change. */
	TimebaseTracking m_timebaseTracking;

	/** Stores the last tempo sent by an external Timebase controller.
	 *
	 * In case of #Timebase::Listener and #TimebaseTracking::OnHold - a
	 * relocation was done by a client other than the current Timebase
	 * controller - the JACK server does not have any BBT information to share
	 * for at least one cycle. We have guard against this or else we have some
	 * spurious state changes. If such a thing happens, it is very likely that
	 * the controller will still be controller and we will still be listener
	 * once transport is starting again. Therefore, we pretend to still be in
	 * this state instead of dropping Timebase state too and offer the last
	 * tempo to the remainder of Hydrogen. */
	float m_fLastTimebaseBpm;

	/** Stores an intended deviation of our transport position from the one hold
	 * by the JACK server.
	 *
	 * In case we act as listener we will relocate based on the provided BBT
	 * information. This is done by converting them into a tick and calculating
	 * the corresponding frame. That resultant frame does not necessarily have
	 * to coincide with the one broadcasted by the JACK server. But this is no
	 * problems as BBT takes precedeence. */
	long long m_nTimebaseFrameOffset;

	/** Remembers the BBT capability bit received in a JACK process cycle.
	 *
	 * In case a regular client triggeres a relocation, the transport bit will
	 * be 0 and we rely on just the frame position to relocate internally.
	 * However, in the next process cycle the JACK Timebase controller will have
	 * added additional BBT information to that location. Since we want to use
	 * its tempo, we also have to use the remainder of the BBT information and
	 * trigger a relocation (although the overall frame might not even have
	 * changed). In addition, the Timebase controller could alter its
	 * capabilities.
	 *
	 * The behavior above has the negativ side effect that we might not relocate
	 * to the exact frame we requested ourselves. But AFAICS this is a bug in
	 * the JACK API. */
	int m_lastTransportBits;

	/** MIDI-related members @{ */
	jack_port_t* m_pMidiOutputPort;
	jack_port_t* m_pMidiInputPort;
	std::mutex m_midiMutex;
	uint8_t m_jackMidiBuffer[jackMidiBufferMax * 4];
	uint32_t m_midiRxInPosition;
	uint32_t m_midiRxOutPosition;
	/** @} */

#ifdef HAVE_INTEGRATION_TESTS
	/** Remember the last location we relocate to in order to detect relocation
	 * loops during the integration tests.*/
	static long m_nIntegrationLastRelocationFrame;
	/** Whether a relocation loop took place (the same position is considered a
	 * relocation over and over again.)*/
	bool m_bIntegrationRelocationLoop;
	bool m_bIntegrationCheckRelocationLoop;
#endif
};

inline JackDriver::Mode JackDriver::getMode() const
{
	return m_mode;
}

inline unsigned JackDriver::getBufferSize()
{
	return static_cast<unsigned>( m_jackServerBufferSize );
}

inline unsigned JackDriver::getSampleRate()
{
	return static_cast<unsigned>( m_jackServerSampleRate );
}

inline int JackDriver::getXRuns() const
{
	return m_nJackServerXRuns;
}

};	// namespace H2Core

#else  // H2CORE_HAVE_JACK
// JACK is disabled

namespace H2Core {
/** \ingroup docCore docAudioDriver docMIDI */
class JackDriver : public NullDriver {
	H2_OBJECT( JackDriver )
   public:
	enum class Timebase { Controller = 1, Listener = 0, None = -1 };
	static QString TimebaseToQString( const Timebase& t )
	{
		return "Not supported";
	};
	static Timebase TimebaseFromInt( int nState ) { return Timebase::None; };
	/**
	 * Fallback version of the JackDriver in case
	 * #H2CORE_HAVE_JACK was not defined during the configuration
	 * and the usage of the JACK audio server is not intended by
	 * the user.
	 */
	JackDriver( audioProcessCallback m_processCallback )
		: NullDriver( m_processCallback )
	{
	}

	// Required since these functions are a friend of AudioEngine which
	// need to be build even if no JACK support is desired.
	void updateTransportPosition() {}
	void relocateUsingBBT() {}
};

};	// namespace H2Core

#endif	// H2CORE_HAVE_JACK

#endif
