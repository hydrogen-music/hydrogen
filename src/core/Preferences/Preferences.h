/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <memory>
#include <vector>

#include "Shortcuts.h"
#include "Theme.h"
#include "WindowProperties.h"

#include <core/Globals.h>
#include <core/Midi/Midi.h>
#include <core/Midi/MidiEventMap.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Helpers/Filesystem.h>
#include <core/Object.h>
#include <core/Sampler/Interpolation.h>

#include <QColor>
#include <QDir>
#include <QDomDocument>
#include <QStringList>

namespace H2Core {

class Hydrogen;

/** \brief Aggregated, fully serialized state of #Preferences.
 *
 * Plain aggregate (no user-declared constructors): every member carries its
 * default as a default member initializer, mirroring the former
 * Preferences::Preferences() initialization list verbatim. Environment
 * dependent defaults - MIDI driver platform detection, the ALSA device probe,
 * and the Rubberband CLI lookup - remain in the Preferences constructor body
 * and overwrite these values there.
 *
 * #Preferences derives from this struct, so all members stay accessible
 * unqualified from within Preferences and its inline API. The
 * PreferencesSchema table drives all XML persistence of these members: a
 * member not covered by a schema row fails the static_assert in
 * PreferencesSchema.cpp (build break) as well as the round-trip unit test.
 *
 * \ingroup H2CORE docCore docConfiguration */
struct PreferencesData {
	enum {
	/**
	 * Specifies whether or not to use JACK transport
	 * capabilities. If set, Hydrogen will start playing as
	 * soon as any over JACK client using its transport
	 * system is starting to play. Its counterpart is
	 * #NO_JACK_TRANSPORT.
	 */
	USE_JACK_TRANSPORT = 0,
	/**
	 * Specifies that Hydrogen should attempt to acquire JACK Timebase
	 * control.
	 *
	 * This represent the state desired by the user. The actual one is
	 * stored in H2Core::JackDriver::m_timebaseState.
	 *
	 * Its counterpart is #NO_JACK_TIMEBASE_CONTROL.
	 */
	USE_JACK_TIMEBASE_CONTROL = 0,
	/**
	 * Specifies whether or not to use JACK transport capabilities. If set,
	 * Hydrogen can be used independent of the JACK system while still using
	 * the JackDriver. Its counterpart is #USE_JACK_TRANSPORT.
	 */
	NO_JACK_TRANSPORT = 1,
	/**
	 * Specifies that Hydrogen should not be in control of JACK Timebase
	 * information. This could mean both that there is an external
	 * application controlling position and tempo of Hydrogen and that there
	 * are just equal JACK clients.
	 *
	 * This represent the state desired by the user. The actual one is
	 * stored in H2Core::JackDriver::m_timebaseState.
	 *
	 * Its counterpart is #USE_JACK_TIMEBASE_CONTROL.
	 */
	NO_JACK_TIMEBASE_CONTROL = 1
	};

	/** Specifies which tempo input widget will be displayed in
	 * #MainToolBar. Via keyboard or MIDI/OSC both TapTempo and BeatCounter
	 * are available at the same time. */
	enum class BpmTap {
		/** Plain averaging over the most recent tap activations. */
		TapTempo,
		/** Input a user-specified number of tabs and average once all of
		 * them have been received. */
		BeatCounter
	};

	/** These options aren't integrated in #BpmTap since the BeatCounter is
	 * always accessible via keyboard, MIDI, and OSC. */
	enum class BeatCounter {
		/** Input a user-specified number of tabs and average once all of
		 * them have been received. */
		Tap,
		/** As #Tap but also starts playback when done. */
		TapAndPlay
	};

	enum class AudioDriver {
		None,
		Null,
		Fake,
		Disk,
		Auto,
		Jack,
		Oss,
		Alsa,
		PulseAudio,
		CoreAudio,
		PortAudio,
		/** Host-driven driver used when Hydrogen runs as a plugin: the host
		 * supplies the output buffers and drives the process callback (ADR
		 * 0013). Not user-selectable. */
		Plugin
	};

	/** \c Plugin is host-driven: MIDI events are injected by the plugin host
	 * (ADR 0013). Not user-selectable. */
	enum class MidiDriver { Alsa, CoreMidi, Jack, None, PortMidi, LoopBack, Plugin };

	/** Specifies which audio settings will be applied to the sample
		supplied in the JACK per track output ports.*/
	enum class JackTrackOutputMode {
		/** Applies layer, component, and instrument gain, note and instrument
		 * pan, note velocity, and instrument volume to the samples. */
		postFader = 0,
		/** Only layer and component gain and note velocity will be applied to
		 * the samples.*/
		preFader = 1
	};

	/** Whether Hydrogen should pair a sent Note-On message with the
	 * corresponding Note-Off.
	 *
	 * Note that this does not affect stop notes (created using Shift + click).
	 * They will always result in a Note-Off event. */
	enum class MidiSendNoteOff {
		Always = 0,
		/** Only send Note-Off messages for notes featuring a user-defined
		 * length. */
		OnCustomLengths = 1,
		Never = 2
	};

	/**
	 * Choice of #m_sMidiPortName and #m_sMidiOutputPortName in case
	 * no port/device was selected.
	 *
	 * Pinning its value to "None" will prevent Hydrogen to connect to
	 * ports/devices using this exact name but is still done for
	 * backward compatibility.
	 */
	static constexpr const char* sNullMidiPort = "None";

	bool m_bPlaySamplesOnClicking = false;	///< audio file browser
	bool m_bFollowPlayhead = true;

	//___ BeatCounter ___
	BpmTap m_bpmTap = BpmTap::TapTempo;
	BeatCounter m_beatCounter = BeatCounter::Tap;
	int m_nBeatCounterDriftCompensation = 0;
	int m_nBeatCounterStartOffset = 0;

	//___ audio engine properties ___
	AudioDriver m_audioDriver = AudioDriver::Auto;
	/** If set to true, samples of the metronome will be added to
	 * #H2Core::AudioEngine::m_songNoteQueue and thus played back on a
	 * regular basis.*/
	bool m_bUseMetronome = false;
	/// Metronome volume FIXME: remove this volume!!
	float m_fMetronomeVolume = 0.5;
	/// max notes
	unsigned m_nMaxNotes = 256;
	/** Sample interpolation (resampling) quality used by the #H2Core::Sampler.
	 * Persistent; during audio export it can be temporarily overridden via
	 * #H2Core::Hydrogen::setInterpolateModeOverride(). */
	Interpolation::InterpolateMode m_interpolateMode =
		Interpolation::InterpolateMode::Linear;
	/** Buffer size of the audio. */
	unsigned m_nBufferSize = 1024;
	/** Sample rate of the audio. */
	unsigned m_nSampleRate = 44100;

	//	OSS driver properties ___
	QString m_sOSSDevice = "/dev/dsp";  ///< Device used for output

	/** Overwritten by the platform detection in the Preferences constructor
	 * body; Alsa serves as the fallback for builds without a detected
	 * driver. */
	MidiDriver m_midiDriver = MidiDriver::Alsa;
	QString m_sMidiPortName = QString( sNullMidiPort );
	QString m_sMidiOutputPortName = QString( sNullMidiPort );

	Midi::Channel m_midiActionChannel = Midi::ChannelAll;
	bool m_bMidiNoteOffIgnore = true;
	bool m_bEnableMidiFeedback = false;

	//___ OSC Server properties ___
	/**
	 * Whether to start the OscServer thread.
	 *
	 * If set to true, the OscServer::start() function of the
	 * OscServer singleton will be called in
	 * Hydrogen::Hydrogen(). This will register all OSC message
	 * handlers and makes the server listen to port
	 * #m_nOscServerPort.
	 */
	bool m_bOscServerEnabled = false;
	/**
	 * Whether to send the current state of Hydrogen to the OSC
	 * clients.
	 *
	 * If set to true, the current state of Hydrogen will be sent to
	 * \e all known OSC clients using
	 * #H2Core::CoreActionController::initExternalControlInterfaces() and
	 * #H2Core::OscServer::handleAction() via OSC messages each time it gets
	 * updated.
	 */
	bool m_bOscFeedbackEnabled = true;
	/** Port number the OscServer will be started at. */
	int m_nOscServerPort = 9000;

	//	alsa audio driver properties ___
	/** Probed in the Preferences constructor body when ALSA support is
	 * enabled; "hw:0" is the fallback otherwise. */
	QString m_sAlsaAudioDevice = "hw:0";

	//___ PortAudio properties ___
	QString m_sPortAudioDevice = "";
	QString m_sPortAudioHostAPI = "";
	int m_nLatencyTarget = 0;

	//___ CoreAudio properties ___
	QString m_sCoreAudioDevice = "";

	//	jack driver properties ___
	QString m_sJackPortName1 = "alsa_pcm:playback_1";
	QString m_sJackPortName2 = "alsa_pcm:playback_2";
	/**
	 * Specifies whether or not Hydrogen will use the JACK
	 * transport system. It has two different states:
	 * #USE_JACK_TRANSPORT and #NO_JACK_TRANSPORT.
	 */
	int m_nJackTransportMode = USE_JACK_TRANSPORT;
	/** Toggles auto-connecting of the main stereo output ports to the
	 * system's default ports when starting the JACK server.*/
	bool m_bJackConnectDefaults = true;
	/** If set to _true_, JackDriver::createPerTrackAudioPorts() will create two
	 * individual left and right output ports for every component of each
	 * instrument. If _false_, one usual stereo output will be created. */
	bool m_bJackTrackOuts = false;

	/** Specifies which audio settings will be applied to the sample
		supplied in the JACK per track output ports.*/
	JackTrackOutputMode m_JackTrackOutputMode = JackTrackOutputMode::postFader;

	/**
	 * External applications with a faulty JACK Timebase implementation can mess
	 * up the transport within Hydrogen. To guarantee the basic functionality,
	 * the user can disable Timebase support and make Hydrogen only listen to
	 * the frame number broadcast by the JACK server.
	 */
	bool m_bJackTimebaseEnabled = false;
	/** Specifies if Hydrogen support the of JACK Timebase protocol. It has two
	 * states: Preferences::USE_JACK_TIMEBASE_CONTROL and
	 * Preferences::NO_JACK_TIMEBASE_CONTROL. It is set to
	 * Preferences::NO_JACK_TIMEBASE_CONTROL by the
	 * JackDriver::initTimebaseControl() if Hydrogen couldn't acquire Timebase
	 * control. */
	int m_bJackTimebaseMode = NO_JACK_TIMEBASE_CONTROL;
	// ~ jack driver properties

	int m_nAutosavesPerHour = 60;

	/// Rubberband CLI - searched in $PATH by the Preferences constructor body.
	QString m_sRubberBandCLIexecutable = "Path to Rubberband-CLI";

	/** Not set in the #PreferencesDialog but by chosing the appropriate
	 * action in #MainToolBar. */
	bool m_bCountIn = false;

	/** Default text editor (used by Playlisteditor) */
	QString m_sDefaultEditor = "";

	QString m_sPreferredLanguage = "";

	bool m_bUseRelativeFileNamesForPlaylists = false;

	///< Show development version warning?
	bool m_bShowDevelWarning = false;
	bool m_bShowNoteOverwriteWarning = true;

	///< Last song or project used
	QString m_sLastSongPath = "";
	QString m_sLastPlaylistPath = "";

	QStringList m_customSoundLibraryDirs;
	QStringList m_onlineRepos;

	bool m_bHearNewNotes = true;
	bool m_bQuantizeEvents = true;

	QStringList m_recentFiles;

	/** Maximum number of bars shown in the Song Editor at
	 * once. */
	int m_nMaxBars = 400;

	/** MIDI channel which to use for both MIDI feedback and MIDI clock
		  signals. */
	Midi::Channel m_midiFeedbackChannel = Midi::ChannelMinimum;
	/** Whether Hydrogen will set its tempo according to incoming MIDI clock
	 * ticks. */
	bool m_bMidiClockInputHandling = false;
	/** Whether Hydrogen will handle incoming MIDI START, STOP, CONTINUE,
	 * and SONG_POSITION_POINTER events. */
	bool m_bMidiTransportInputHandling = false;
	/** Whether Hydrogen will send outgoing MIDI clock messages based on the
	 * current tempo. */
	bool m_bMidiClockOutputSend = false;
	/** Whether Hydrogen will send outgoing MIDI START, STOP, CONTINUE,
	 * and SONG_POSITION_POINTER messages on transport changes. */
	bool m_bMidiTransportOutputSend = false;
	MidiSendNoteOff m_midiSendNoteOff = MidiSendNoteOff::Always;

	/// rubberband bpm change queue
	bool m_bUseTheRubberbandBpmChangeEvent = false;

	/** Whether the names of the per-instrument output ports should be set
	 * according to the instrument type of the corresponding instrument or
	 * according to our classical name scheme include track number and
	 * instrument name. */
	bool m_bJackEnforceInstrumentName = false;

	//___ GUI properties ___
	bool m_bShowInstrumentPeaks = true;
	int m_nPatternEditorGridResolution = 8;
	bool m_bPatternEditorUsingTriplets = false;
	bool m_bPatternEditorAlwaysShowTypeLabels = false;

	bool m_bHideKeyboardCursor = false;
	bool m_bShowPlaybackTrack = true;
	int m_nLastOpenTab = 0;
	bool m_bShowAutomationArea = false;
	unsigned m_nPatternEditorGridHeight = 21;
	unsigned m_nPatternEditorGridWidth = 3;
	unsigned m_nSongEditorGridHeight = 18;
	unsigned m_nSongEditorGridWidth = 16;
	WindowProperties m_mainFormProperties =
		WindowProperties( 0, 0, 1000, 700, true );
	WindowProperties m_mixerProperties =
		WindowProperties( 10, 350, 829, 276, true );
	WindowProperties m_patternEditorProperties =
		WindowProperties( 280, 100, 706, 439, true );
	WindowProperties m_songEditorProperties =
		WindowProperties( 10, 10, 600, 250, true );
	WindowProperties m_rackProperties =
		WindowProperties( 500, 20, 526, 437, true );
	WindowProperties m_audioEngineInfoProperties =
		WindowProperties( 720, 120, 0, 0, false );
	WindowProperties m_playlistEditorProperties =
		WindowProperties( 200, 300, 921, 703, false );
	WindowProperties m_directorProperties =
		WindowProperties( 200, 300, 423, 377, false );

	//___ Last directories used in QFileDialogs ___
	QString m_sLastExportPatternAsDirectory = QDir::homePath();
	QString m_sLastExportSongDirectory = QDir::homePath();
	QString m_sLastSaveSongAsDirectory = QDir::homePath();
	QString m_sLastOpenSongDirectory = Filesystem::userSongsDir();
	QString m_sLastOpenPatternDirectory = Filesystem::userPatternsDir();
	QString m_sLastExportLilypondDirectory = QDir::homePath();
	QString m_sLastExportMidiDirectory = QDir::homePath();
	QString m_sLastImportDrumkitDirectory = QDir::homePath();
	QString m_sLastExportDrumkitDirectory = QDir::homePath();
	QString m_sLastSaveDrumkitAsDirectory = Filesystem::userDrumkitsDir();
	QString m_sLastOpenLayerDirectory = QDir::homePath();
	QString m_sLastOpenPlaybackTrackDirectory = QDir::homePath();
	QString m_sLastAddSongToPlaylistDirectory = Filesystem::userSongsDir();
	QString m_sLastPlaylistDirectory = Filesystem::userPlaylistsDir();
	QString m_sLastPlaylistScriptDirectory = QDir::homePath();
	QString m_sLastImportThemeDirectory = QDir::homePath();
	QString m_sLastExportThemeDirectory = QDir::homePath();

	//___ Export dialog ___
	int m_nExportSampleDepthIdx = 0;
	int m_nExportSampleRateIdx = 0;
	int m_nExportModeIdx = 0;
	Filesystem::AudioFormat m_exportFormat = Filesystem::AudioFormat::Flac;
	float m_fExportCompressionLevel = 0.0;
	// ~ Export dialog

	//___ Export midi dialog ___
	int m_nMidiExportMode = 0;
	bool m_bMidiExportUseHumanization = false;

	bool m_bSoundLibraryShowName = true;
	bool m_bSoundLibraryShowAuthor = false;
	bool m_bSoundLibraryShowInfo = true;
	bool m_bSoundLibraryShowLicense = false;
	bool m_bSoundLibraryShowPath = false;
	bool m_bSoundLibraryShowTags = true;
	bool m_bSoundLibraryShowVersion = false;
	int m_nSoundLibraryLastTab = 0;
	int m_nRackLastTab = 0;

	bool m_bShowExportSongLicenseWarning = true;
	bool m_bShowExportDrumkitLicenseWarning = true;
	bool m_bShowExportDrumkitCopyleftWarning = true;
	bool m_bShowExportDrumkitAttributionWarning = true;

	std::shared_ptr<Theme> m_pTheme = std::make_shared<Theme>(
		std::make_shared<ColorTheme>(), std::make_shared<InterfaceTheme>(),
		std::make_shared<FontTheme>() );

	std::shared_ptr<Shortcuts> m_pShortcuts = std::make_shared<Shortcuts>();
	std::shared_ptr<MidiEventMap> m_pMidiEventMap =
		std::make_shared<MidiEventMap>();
	std::shared_ptr<MidiInstrumentMap> m_pMidiInstrumentMap =
		std::make_shared<MidiInstrumentMap>();
};

/** \brief Manager for User Preferences File.
 *
 * Owned per Hydrogen instance (ADR 0015): any number of Preferences may
 * coexist independently.
 * \ingroup H2CORE docCore docConfiguration */
class Preferences : public H2Core::Object<Preferences>, public PreferencesData {
	H2_OBJECT( Preferences )
   public:
	/** \name Aliases for types and enumerators moved into #PreferencesData
	 *
	 * The enums backing the serialized members now live in
	 * #PreferencesData so the standalone aggregate can default-construct
	 * them. These aliases keep all `Preferences::X` qualified references
	 * compiling unchanged. */
	// @{
	using PreferencesData::BpmTap;
	using PreferencesData::BeatCounter;
	using PreferencesData::AudioDriver;
	using PreferencesData::MidiDriver;
	using PreferencesData::JackTrackOutputMode;
	using PreferencesData::MidiSendNoteOff;
	/** Enumerator aliases for the unscoped JACK transport and Timebase
	 * modes (stored as plain int in #m_nJackTransportMode and
	 * #m_bJackTimebaseMode). */
	using PreferencesData::USE_JACK_TRANSPORT;
	using PreferencesData::NO_JACK_TRANSPORT;
	using PreferencesData::USE_JACK_TIMEBASE_CONTROL;
	using PreferencesData::NO_JACK_TIMEBASE_CONTROL;
	// @}

	/** Bitwise or-able options showing which part of the Preferences
	 * were altered using the PreferencesDialog.*/
	enum Changes {
		None = 0x000,
		/** Either the font size or font family have changed.*/
		Font = 0x001,
		/** At least one of the colors has changed.*/
		Colors = 0x002,
		/** Any option in the Appearance tab excluding colors, font
			size, or font family. Those have to be indicated by
			or-ing the dedicated option.*/
		AppearanceTab = 0x004,
		/** Any option in the General tab appeared.*/
		GeneralTab = 0x008,
		/** Any option in the Audio tab appeared.*/
		AudioTab = 0x010,
		/** Any option in the MIDI tab or selected ones in the
			#MidiControlDialog.*/
		MidiTab = 0x020,
		/** Any option in the OSC tab appeared.*/
		OscTab = 0x040,
		/** At least one shortcut was changed.*/
		ShortcutTab = 0x080,
	};
	static QString ChangesToQString( Changes changes );

	static AudioDriver parseAudioDriver( const QString& sDriver );
	static QString audioDriverToQString( const AudioDriver& driver );
	static MidiDriver parseMidiDriver( const QString& sDriver );
	static QString midiDriverToQString( const MidiDriver& driver );

	/** Loads the user (or, failing that, system) config file and returns a
	 * freshly-owned Preferences. No process-wide singleton is involved; the
	 * caller owns the result (ADR 0015). */
	static std::shared_ptr<Preferences> create_instance();

	Preferences();
	Preferences( std::shared_ptr<Preferences> pOther );
	~Preferences();

	static std::shared_ptr<Preferences>
	load( const QString& sPath, bool bSilent, Hydrogen* pHydrogen );
	/** Which rows this instance may write to the shared user config
	 * (ADR 0022/0023). Tagged once at process startup; the default, #All,
	 * fits standalone and headless processes. */
	enum class FieldOwnership {
		/** Every row (standalone GUI, headless engine). */
		All,
		/** Only base-layer rows (GUI running as a plugin guest: the host
		 * owns the override layer). */
		BaseLayer,
		/** Only GUI-owned base-layer rows (editor mirror: the
		 * authoritative headless engine owns the core rows). */
		GuiOwned
	};

	/** Save the config to the user-level config file (or the one specified
	 * via CLI). Concurrency-safe (ADR 0023): under a cross-process lock the
	 * file is re-read and only this instance's own changed,
	 * ownership-eligible rows are merged onto it, then written back
	 * atomically — so concurrent edits other processes made to other
	 * fields survive.
	 *
	 * @param bSilent whether log messages should be suppressed. */
	bool save( const bool bSilent = false ) const;
	void setFieldOwnership( FieldOwnership ownership ) {
		m_fieldOwnership = ownership;
	}
	FieldOwnership getFieldOwnership() const { return m_fieldOwnership; }
	/** Instead of a `saveAs` method #Preferences only provides a
	 * #saveCopyAs() method to indicate that corresponding file won't change
	 * and will always be the user-level config file. (Which can be altered
	 * using #Filesystem::m_sPreferencesOverwritePath) */
	bool saveCopyAs( const QString& sPath, const bool bSilent = false ) const;

	/** The XML baseline retained from load (ADR 0023). Empty when this instance
	 * was never loaded from disk. See #m_baselineXml. */
	const QByteArray& getBaselineXml() const { return m_baselineXml; }

	/** Serialize only the core-related preferences (audio engine, MIDI, JACK,
	 * OSC, export, beat-counter, rubberband, metronome, max bars, hear new
	 * notes, quantize events, MIDI maps) to an XML fragment. Used by the
	 * headless engine to send its preferences state to the GUI mirror over
	 * IPC. */
	QByteArray corePropsToXml() const;

	/** Apply core-related preferences from an XML fragment (received from the
	 * headless engine via IPC). Only the core-owned fields are overwritten; GUI
	 * fields (window geometry, theme, shortcuts, recent files) are preserved. */
	void applyCorePropsFromXml( const QByteArray& xml );

	static std::vector<AudioDriver> getSupportedAudioDrivers();

	/**
	 * Attempts to call several JACK executables in order to check for
	 * existing JACK support.
	 *
	 * In an earlier version I tried checking the presence of the
	 * `libjack.so` shared library. But this one comes preinstalled
	 * with most Linux distribution regardless of JACK itself is
	 * present or not.
	 *
	 * @return Whether or not JACK support appears to be functional.
	 */
	static bool checkJackSupport();

	/**
	 * Choice of #m_sMidiPortName and #m_sMidiOutputPortName in case
	 * no port/device was selected.
	 *
	 * Pinning its value to "None" will prevent Hydrogen to connect to
	 * ports/devices using this exact name but is still done for
	 * backward compatibility.
	 */
	static QString getNullMidiPort() { return QString( sNullMidiPort ); }

	Midi::Channel getMidiFeedbackChannel() const;
	void setMidiFeedbackChannel( Midi::Channel nChannel );
	bool getMidiClockInputHandling() const;
	void setMidiClockInputHandling( bool bHandle );
	bool getMidiTransportInputHandling() const;
	void setMidiTransportInputHandling( bool bHandle );
	bool getMidiClockOutputSend() const;
	void setMidiClockOutputSend( bool bHandle );
	bool getMidiTransportOutputSend() const;
	void setMidiTransportOutputSend( bool bHandle );
	MidiSendNoteOff getMidiSendNoteOff() const;
	void setMidiSendNoteOff( MidiSendNoteOff noteOff );

	// OSC Server properties
	/** \return #m_bOscServerEnabled*/
	bool getOscServerEnabled() const;
	/** \param val Sets #m_bOscServerEnabled*/
	void setOscServerEnabled( bool val );
	/** \return #m_bOscFeedbackEnabled*/
	bool getOscFeedbackEnabled() const;
	/** \param val Sets #m_bOscFeedbackEnabled*/
	void setOscFeedbackEnabled( bool val );
	/** \return #m_nOscServerPort*/
	int getOscServerPort() const;
	/** \param oscPort Sets #m_nOscServerPort*/
	void setOscServerPort( int oscPort );

	/** Not set in the #PreferencesDialog but by chosing the appropriate
	 * action in #MainToolBar. */
	bool getCountIn() const;
	void setCountIn( bool value );

	const QString& getDefaultEditor() const;
	void setDefaultEditor( const QString& editor );

	// General
	const QString& getPreferredLanguage() const;
	void setPreferredLanguage( const QString& sLanguage );

	bool getUseRelativeFileNamesForPlaylists() const;
	void setUseRelativeFileNamesForPlaylists( bool value );

	bool getShowDevelWarning() const;
	void setShowDevelWarning( bool value );
	bool getShowNoteOverwriteWarning() const;
	void setShowNoteOverwriteWarning( bool bValue );

	const QString& getLastSongPath() const;
	void setLastSongPath( const QString& sPath );
	const QString& getLastPlaylistPath() const;
	void setLastPlaylistPath( const QString& sPath );

	const QStringList& getCustomSoundLibraryDirs() const;
	void setCustomSoundLibraryDirs( const QStringList& folders );

	const QStringList& getOnlineRepos() const;
	void setOnlineRepos( const QStringList& repos );

	bool getHearNewNotes() const;
	void setHearNewNotes( bool value );

	int getPunchInPos() const;
	void setPunchInPos( unsigned pos );
	int getPunchOutPos() const;
	void setPunchOutPos( unsigned pos );
	bool inPunchArea( int pos ) const;
	void unsetPunchArea();

	bool getQuantizeEvents() const;
	void setQuantizeEvents( bool value );

	const QStringList& getRecentFiles() const;
	void setRecentFiles( const QStringList& recentFiles );

	/** @return #m_nMaxBars.*/
	int getMaxBars() const;
	/** @param bars Sets #m_nMaxBars.*/
	void setMaxBars( const int bars );

	int getRubberBandBatchMode() const;
	void setRubberBandBatchMode( int val );

	bool getJackEnforceInstrumentName() const;
	void setJackEnforceInstrumentName( bool bEnforce );

	// GUI Properties
	bool showInstrumentPeaks() const;
	void setInstrumentPeaks( bool value );

	int getPatternEditorGridResolution() const;
	void setPatternEditorGridResolution( int value );

	bool isPatternEditorUsingTriplets() const;
	void setPatternEditorUsingTriplets( bool value );

	bool getPatternEditorAlwaysShowTypeLabels() const;
	void setPatternEditorAlwaysShowTypeLabels( bool bNew );

	bool getHideKeyboardCursor() const;
	void setHideKeyboardCursor( bool b );

	void setShowPlaybackTrack( bool val );
	bool getShowPlaybackTrack() const;

	int getLastOpenTab() const;
	void setLastOpenTab( int n );

	bool getShowAutomationArea() const;
	void setShowAutomationArea( bool value );

	unsigned getPatternEditorGridHeight() const;
	void setPatternEditorGridHeight( unsigned value );

	unsigned getPatternEditorGridWidth() const;
	void setPatternEditorGridWidth( unsigned value );

	unsigned getSongEditorGridHeight() const;
	void setSongEditorGridHeight( unsigned value );

	unsigned getSongEditorGridWidth() const;
	void setSongEditorGridWidth( unsigned value );

	const WindowProperties& getMainFormProperties() const;
	void setMainFormProperties( const WindowProperties& prop );

	const WindowProperties& getMixerProperties() const;
	void setMixerProperties( const WindowProperties& prop );

	const WindowProperties& getPatternEditorProperties() const;
	void setPatternEditorProperties( const WindowProperties& prop );

	const WindowProperties& getSongEditorProperties() const;
	void setSongEditorProperties( const WindowProperties& prop );

	const WindowProperties& getRackProperties() const;
	void setRackProperties( const WindowProperties& prop );

	const WindowProperties& getAudioEngineInfoProperties() const;
	void setAudioEngineInfoProperties( const WindowProperties& prop );

	const WindowProperties& getPlaylistEditorProperties() const;
	void setPlaylistEditorProperties( const WindowProperties& prop );

	const WindowProperties& getDirectorProperties() const;
	void setDirectorProperties( const WindowProperties& prop );

	const QString& getLastExportPatternAsDirectory() const;
	void setLastExportPatternAsDirectory( const QString& sPath );
	const QString& getLastExportSongDirectory() const;
	void setLastExportSongDirectory( const QString& sPath );
	const QString& getLastSaveSongAsDirectory() const;
	void setLastSaveSongAsDirectory( const QString& sPath );
	const QString& getLastOpenSongDirectory() const;
	void setLastOpenSongDirectory( const QString& sPath );
	const QString& getLastOpenPatternDirectory() const;
	void setLastOpenPatternDirectory( const QString& sPath );
	const QString& getLastExportLilypondDirectory() const;
	void setLastExportLilypondDirectory( const QString& sPath );
	const QString& getLastExportMidiDirectory() const;
	void setLastExportMidiDirectory( const QString& sPath );
	const QString& getLastImportDrumkitDirectory() const;
	void setLastImportDrumkitDirectory( const QString& sPath );
	const QString& getLastExportDrumkitDirectory() const;
	void setLastExportDrumkitDirectory( const QString& sPath );
	const QString& getLastSaveDrumkitAsDirectory() const;
	void setLastSaveDrumkitAsDirectory( const QString& sPath );
	const QString& getLastOpenLayerDirectory() const;
	void setLastOpenLayerDirectory( const QString& sPath );
	const QString& getLastOpenPlaybackTrackDirectory() const;
	void setLastOpenPlaybackTrackDirectory( const QString& sPath );
	const QString& getLastAddSongToPlaylistDirectory() const;
	void setLastAddSongToPlaylistDirectory( const QString& sPath );
	const QString& getLastPlaylistDirectory() const;
	void setLastPlaylistDirectory( const QString& sPath );
	const QString& getLastPlaylistScriptDirectory() const;
	void setLastPlaylistScriptDirectory( const QString& sPath );
	const QString& getLastImportThemeDirectory() const;
	void setLastImportThemeDirectory( const QString& sPath );
	const QString& getLastExportThemeDirectory() const;
	void setLastExportThemeDirectory( const QString& sPath );

	// Export song dialog
	int getExportSampleDepthIdx() const;
	void setExportSampleDepthIdx( int nExportSampleDepthIdx );
	int getExportSampleRateIdx() const;
	void setExportSampleRateIdx( int nExportSampleRateIdx );
	int getExportModeIdx() const;
	void setExportModeIdx( int nExportMode );
	Filesystem::AudioFormat getExportFormat() const;
	void setExportFormat( Filesystem::AudioFormat format );
	float getExportCompressionLevel() const;
	void setExportCompressionLevel( float fCompressionLevel );

	// Export MIDI dialog
	int getMidiExportMode() const;
	void setMidiExportMode( int nExportMode );
	bool getMidiExportUseHumanization() const;
	void setMidiExportUseHumanization( bool bHumanization );

	bool getSoundLibraryShowName() const;
	void setSoundLibraryShowName( bool bShow );
	bool getSoundLibraryShowAuthor() const;
	void setSoundLibraryShowAuthor( bool bShow );
	bool getSoundLibraryShowInfo() const;
	void setSoundLibraryShowInfo( bool bShow );
	bool getSoundLibraryShowLicense() const;
	void setSoundLibraryShowLicense( bool bShow );
	bool getSoundLibraryShowPath() const;
	void setSoundLibraryShowPath( bool bShow );
	bool getSoundLibraryShowTags() const;
	void setSoundLibraryShowTags( bool bShow );
	bool getSoundLibraryShowVersion() const;
	void setSoundLibraryShowVersion( bool bShow );
	int getSoundLibraryLastTab() const;
	void setSoundLibraryLastTab( int nTab );
	int getRackLastTab() const;
	void setRackLastTab( int nTab );

	const std::shared_ptr<const Theme> getTheme() const;
	const std::shared_ptr<const ColorTheme> getColorTheme() const;
	const std::shared_ptr<const InterfaceTheme> getInterfaceTheme() const;
	const std::shared_ptr<const FontTheme> getFontTheme() const;
	std::shared_ptr<Theme> getThemeWritable();
	void setTheme( std::shared_ptr<Theme> pTheme );

	const std::shared_ptr<Shortcuts> getShortcuts() const;
	void setShortcuts( const std::shared_ptr<Shortcuts> pShortcuts );
	const std::shared_ptr<MidiEventMap> getMidiEventMap() const;
	void setMidiEventMap( const std::shared_ptr<MidiEventMap> pMidiEventMap );
	const std::shared_ptr<MidiInstrumentMap> getMidiInstrumentMap() const;
	void setMidiInstrumentMap( std::shared_ptr<MidiInstrumentMap> pMap );

	bool getLoadingSuccessful() const;

	QString toQString( const QString& sPrefix = "", bool bShort = true )
		const override;

    private:
	/** Used to indicate changes in the underlying XSD file. */
	static constexpr int nCurrentFormatVersion = 2;

	/** Runtime-only punch area markers (not serialized to disk). */
	int m_nPunchInPos;
	int m_nPunchOutPos;

	/** In case the rubberband binary was not found in common places, this
	 * variable indicated - if `true` - that Hydrogen should continue
	 * searching for it in places provided during #load() */
	bool m_bSearchForRubberbandOnLoad;

	bool m_bLoadingSuccessful;

	/** The on-disk XML as parsed at load time (ADR 0023): the baseline against
	 * which save() diffs to find this instance's own changes, so the
	 * concurrency-safe persist of the shared user config writes only those and
	 * preserves concurrent edits other processes made to other fields. Empty for
	 * a never-loaded (freshly created) Preferences, which then falls back to a
	 * plain snapshot write. Never refreshed after a save: it keeps representing
	 * the values this instance loaded, so foreign changes adopted into the
	 * merged document never register as this instance's own. */
	QByteArray m_baselineXml;

	/** Which rows this instance may write to the shared user config; see
	 * #FieldOwnership. Runtime process state, never serialized. */
	FieldOwnership m_fieldOwnership = FieldOwnership::All;
};

inline const QString& Preferences::getLastExportPatternAsDirectory() const
{
	return m_sLastExportPatternAsDirectory;
}
inline const QString& Preferences::getLastExportSongDirectory() const
{
	return m_sLastExportSongDirectory;
}
inline const QString& Preferences::getLastSaveSongAsDirectory() const
{
	return m_sLastSaveSongAsDirectory;
}
inline const QString& Preferences::getLastOpenSongDirectory() const
{
	return m_sLastOpenSongDirectory;
}
inline const QString& Preferences::getLastOpenPatternDirectory() const
{
	return m_sLastOpenPatternDirectory;
}
inline const QString& Preferences::getLastExportLilypondDirectory() const
{
	return m_sLastExportLilypondDirectory;
}
inline const QString& Preferences::getLastExportMidiDirectory() const
{
	return m_sLastExportMidiDirectory;
}
inline const QString& Preferences::getLastImportDrumkitDirectory() const
{
	return m_sLastImportDrumkitDirectory;
}
inline const QString& Preferences::getLastExportDrumkitDirectory() const
{
	return m_sLastExportDrumkitDirectory;
}
inline const QString& Preferences::getLastSaveDrumkitAsDirectory() const
{
	return m_sLastSaveDrumkitAsDirectory;
}
inline const QString& Preferences::getLastOpenLayerDirectory() const
{
	return m_sLastOpenLayerDirectory;
}
inline const QString& Preferences::getLastOpenPlaybackTrackDirectory() const
{
	return m_sLastOpenPlaybackTrackDirectory;
}
inline const QString& Preferences::getLastAddSongToPlaylistDirectory() const
{
	return m_sLastAddSongToPlaylistDirectory;
}
inline const QString& Preferences::getLastPlaylistDirectory() const
{
	return m_sLastPlaylistDirectory;
}
inline const QString& Preferences::getLastPlaylistScriptDirectory() const
{
	return m_sLastPlaylistScriptDirectory;
}
inline const QString& Preferences::getLastImportThemeDirectory() const
{
	return m_sLastImportThemeDirectory;
}
inline const QString& Preferences::getLastExportThemeDirectory() const
{
	return m_sLastExportThemeDirectory;
}
inline void Preferences::setLastExportPatternAsDirectory( const QString& sPath )
{
	m_sLastExportPatternAsDirectory = sPath;
}
inline void Preferences::setLastExportSongDirectory( const QString& sPath )
{
	m_sLastExportSongDirectory = sPath;
}
inline void Preferences::setLastSaveSongAsDirectory( const QString& sPath )
{
	m_sLastSaveSongAsDirectory = sPath;
}
inline void Preferences::setLastOpenSongDirectory( const QString& sPath )
{
	m_sLastOpenSongDirectory = sPath;
}
inline void Preferences::setLastOpenPatternDirectory( const QString& sPath )
{
	m_sLastOpenPatternDirectory = sPath;
}
inline void Preferences::setLastExportLilypondDirectory( const QString& sPath )
{
	m_sLastExportLilypondDirectory = sPath;
}
inline void Preferences::setLastExportMidiDirectory( const QString& sPath )
{
	m_sLastExportMidiDirectory = sPath;
}
inline void Preferences::setLastImportDrumkitDirectory( const QString& sPath )
{
	m_sLastImportDrumkitDirectory = sPath;
}
inline void Preferences::setLastExportDrumkitDirectory( const QString& sPath )
{
	m_sLastExportDrumkitDirectory = sPath;
}
inline void Preferences::setLastSaveDrumkitAsDirectory( const QString& sPath )
{
	m_sLastSaveDrumkitAsDirectory = sPath;
}
inline void Preferences::setLastOpenLayerDirectory( const QString& sPath )
{
	m_sLastOpenLayerDirectory = sPath;
}
inline void Preferences::setLastOpenPlaybackTrackDirectory( const QString& sPath
)
{
	m_sLastOpenPlaybackTrackDirectory = sPath;
}
inline void Preferences::setLastAddSongToPlaylistDirectory( const QString& sPath
)
{
	m_sLastAddSongToPlaylistDirectory = sPath;
}
inline void Preferences::setLastPlaylistDirectory( const QString& sPath )
{
	m_sLastPlaylistDirectory = sPath;
}
inline void Preferences::setLastPlaylistScriptDirectory( const QString& sPath )
{
	m_sLastPlaylistScriptDirectory = sPath;
}
inline void Preferences::setLastImportThemeDirectory( const QString& sPath )
{
	m_sLastImportThemeDirectory = sPath;
}
inline void Preferences::setLastExportThemeDirectory( const QString& sPath )
{
	m_sLastExportThemeDirectory = sPath;
}

inline int Preferences::getMidiExportMode() const
{
	return m_nMidiExportMode;
}
inline void Preferences::setMidiExportMode( int ExportMode )
{
	m_nMidiExportMode = ExportMode;
}
inline bool Preferences::getMidiExportUseHumanization() const
{
	return m_bMidiExportUseHumanization;
}
inline void Preferences::setMidiExportUseHumanization( bool bUseHumanization )
{
	m_bMidiExportUseHumanization = bUseHumanization;
}

inline bool Preferences::getSoundLibraryShowName() const
{
	return m_bSoundLibraryShowName;
}
inline void Preferences::setSoundLibraryShowName( bool bShow )
{
	m_bSoundLibraryShowName = bShow;
}
inline bool Preferences::getSoundLibraryShowAuthor() const
{
	return m_bSoundLibraryShowAuthor;
}
inline void Preferences::setSoundLibraryShowAuthor( bool bShow )
{
	m_bSoundLibraryShowAuthor = bShow;
}
inline bool Preferences::getSoundLibraryShowInfo() const
{
	return m_bSoundLibraryShowInfo;
}
inline void Preferences::setSoundLibraryShowInfo( bool bShow )
{
	m_bSoundLibraryShowInfo = bShow;
}
inline bool Preferences::getSoundLibraryShowLicense() const
{
	return m_bSoundLibraryShowLicense;
}
inline void Preferences::setSoundLibraryShowLicense( bool bShow )
{
	m_bSoundLibraryShowLicense = bShow;
}
inline bool Preferences::getSoundLibraryShowPath() const
{
	return m_bSoundLibraryShowPath;
}
inline void Preferences::setSoundLibraryShowPath( bool bShow )
{
	m_bSoundLibraryShowPath = bShow;
}
inline bool Preferences::getSoundLibraryShowTags() const
{
	return m_bSoundLibraryShowTags;
}
inline void Preferences::setSoundLibraryShowTags( bool bShow )
{
	m_bSoundLibraryShowTags = bShow;
}
inline bool Preferences::getSoundLibraryShowVersion() const
{
	return m_bSoundLibraryShowVersion;
}
inline void Preferences::setSoundLibraryShowVersion( bool bShow )
{
	m_bSoundLibraryShowVersion = bShow;
}
inline int Preferences::getSoundLibraryLastTab() const
{
	return m_nSoundLibraryLastTab;
}
inline void Preferences::setSoundLibraryLastTab( int nTab )
{
	m_nSoundLibraryLastTab = nTab;
}
inline int Preferences::getRackLastTab() const
{
	return m_nRackLastTab;
}
inline void Preferences::setRackLastTab( int nTab )
{
	m_nRackLastTab = nTab;
}

inline int Preferences::getExportSampleDepthIdx() const
{
	return m_nExportSampleDepthIdx;
}

inline void Preferences::setExportSampleDepthIdx( int ExportSampleDepth )
{
	m_nExportSampleDepthIdx = ExportSampleDepth;
}

inline int Preferences::getExportSampleRateIdx() const
{
	return m_nExportSampleRateIdx;
}

inline int Preferences::getExportModeIdx() const
{
	return m_nExportModeIdx;
}

inline void Preferences::setExportModeIdx( int ExportModeIdx )
{
	m_nExportModeIdx = ExportModeIdx;
}

inline void Preferences::setExportSampleRateIdx( int ExportSampleRate )
{
	m_nExportSampleRateIdx = ExportSampleRate;
}

inline Filesystem::AudioFormat Preferences::getExportFormat() const
{
	return m_exportFormat;
}

inline void Preferences::setExportFormat( Filesystem::AudioFormat format )
{
	m_exportFormat = format;
}

inline float Preferences::getExportCompressionLevel() const
{
	return m_fExportCompressionLevel;
}

inline void Preferences::setExportCompressionLevel( float fCompressionLevel )
{
	m_fExportCompressionLevel = fCompressionLevel;
}

inline bool Preferences::getCountIn() const
{
	return m_bCountIn;
}
inline void Preferences::setCountIn( bool bActivate )
{
	m_bCountIn = bActivate;
}

inline const QString& Preferences::getDefaultEditor() const
{
	return m_sDefaultEditor;
}

inline void Preferences::setDefaultEditor( const QString& editor )
{
	m_sDefaultEditor = editor;
}

// General
inline const QString& Preferences::getPreferredLanguage() const
{
	return m_sPreferredLanguage;
}

inline void Preferences::setPreferredLanguage( const QString& sLanguage )
{
	m_sPreferredLanguage = sLanguage;
}

inline void Preferences::setUseRelativeFileNamesForPlaylists( bool value )
{
	m_bUseRelativeFileNamesForPlaylists = value;
}

inline void Preferences::setShowDevelWarning( bool value )
{
	m_bShowDevelWarning = value;
}

inline bool Preferences::getShowDevelWarning() const
{
	return m_bShowDevelWarning;
}

inline bool Preferences::getShowNoteOverwriteWarning() const
{
	return m_bShowNoteOverwriteWarning;
}

inline void Preferences::setShowNoteOverwriteWarning( bool bValue )
{
	m_bShowNoteOverwriteWarning = bValue;
}

inline void Preferences::setHideKeyboardCursor( bool value )
{
	m_bHideKeyboardCursor = value;
}

inline bool Preferences::getHideKeyboardCursor() const
{
	return m_bHideKeyboardCursor;
}

inline bool Preferences::getUseRelativeFileNamesForPlaylists() const
{
	return m_bUseRelativeFileNamesForPlaylists;
}

inline void Preferences::setLastSongPath( const QString& sPath )
{
	m_sLastSongPath = sPath;
}
inline const QString& Preferences::getLastSongPath() const
{
	return m_sLastSongPath;
}

inline void Preferences::setLastPlaylistPath( const QString& sPath )
{
	m_sLastPlaylistPath = sPath;
}
inline const QString& Preferences::getLastPlaylistPath() const
{
	return m_sLastPlaylistPath;
}

inline void Preferences::setCustomSoundLibraryDirs( const QStringList& folders )
{
	m_customSoundLibraryDirs = folders;
}
inline const QStringList& Preferences::getCustomSoundLibraryDirs() const
{
	return m_customSoundLibraryDirs;
}

inline void Preferences::setOnlineRepos( const QStringList& repos )
{
	m_onlineRepos = repos;
}
inline const QStringList& Preferences::getOnlineRepos() const
{
	return m_onlineRepos;
}

inline void Preferences::setHearNewNotes( bool value )
{
	m_bHearNewNotes = value;
}
inline bool Preferences::getHearNewNotes() const
{
	return m_bHearNewNotes;
}

inline void Preferences::setPunchInPos( unsigned pos )
{
	m_nPunchInPos = pos;
}
inline int Preferences::getPunchInPos() const
{
	return m_nPunchInPos;
}

inline void Preferences::setPunchOutPos( unsigned pos )
{
	m_nPunchOutPos = pos;
}
inline int Preferences::getPunchOutPos() const
{
	return m_nPunchOutPos;
}

inline bool Preferences::inPunchArea( int pos ) const
{
	// Return true if punch area not defined
	if ( m_nPunchInPos <= m_nPunchOutPos ) {
		if ( pos < m_nPunchInPos || m_nPunchOutPos < pos ) {
			return false;
		}
	}
	return true;
}

inline void Preferences::unsetPunchArea()
{
	m_nPunchInPos = 0;
	m_nPunchOutPos = -1;
}

inline void Preferences::setQuantizeEvents( bool value )
{
	m_bQuantizeEvents = value;
}
inline bool Preferences::getQuantizeEvents() const
{
	return m_bQuantizeEvents;
}

inline void Preferences::setRecentFiles( const QStringList& recentFiles )
{
	m_recentFiles = recentFiles;
}
inline const QStringList& Preferences::getRecentFiles() const
{
	return m_recentFiles;
}

inline bool Preferences::getJackEnforceInstrumentName() const
{
	return m_bJackEnforceInstrumentName;
}
inline void Preferences::setJackEnforceInstrumentName( bool bEnforce )
{
	m_bJackEnforceInstrumentName = bEnforce;
}

// GUI Properties
inline bool Preferences::showInstrumentPeaks() const
{
	return m_bShowInstrumentPeaks;
}
inline void Preferences::setInstrumentPeaks( bool value )
{
	m_bShowInstrumentPeaks = value;
}

inline int Preferences::getPatternEditorGridResolution() const
{
	return m_nPatternEditorGridResolution;
}
inline void Preferences::setPatternEditorGridResolution( int value )
{
	m_nPatternEditorGridResolution = value;
}

inline bool Preferences::isPatternEditorUsingTriplets() const
{
	return m_bPatternEditorUsingTriplets;
}
inline void Preferences::setPatternEditorUsingTriplets( bool value )
{
	m_bPatternEditorUsingTriplets = value;
}
inline bool Preferences::getPatternEditorAlwaysShowTypeLabels() const
{
	return m_bPatternEditorAlwaysShowTypeLabels;
}
inline void Preferences::setPatternEditorAlwaysShowTypeLabels( bool bNew )
{
	m_bPatternEditorAlwaysShowTypeLabels = bNew;
}

inline bool Preferences::getShowAutomationArea() const
{
	return m_bShowAutomationArea;
}
inline void Preferences::setShowAutomationArea( bool value )
{
	m_bShowAutomationArea = value;
}

inline unsigned Preferences::getSongEditorGridHeight() const
{
	return m_nSongEditorGridHeight;
}
inline void Preferences::setSongEditorGridHeight( unsigned value )
{
	m_nSongEditorGridHeight = value;
}
inline unsigned Preferences::getSongEditorGridWidth() const
{
	return m_nSongEditorGridWidth;
}
inline void Preferences::setSongEditorGridWidth( unsigned value )
{
	m_nSongEditorGridWidth = value;
}

inline unsigned Preferences::getPatternEditorGridHeight() const
{
	return m_nPatternEditorGridHeight;
}
inline void Preferences::setPatternEditorGridHeight( unsigned value )
{
	m_nPatternEditorGridHeight = value;
}
inline unsigned Preferences::getPatternEditorGridWidth() const
{
	return m_nPatternEditorGridWidth;
}
inline void Preferences::setPatternEditorGridWidth( unsigned value )
{
	m_nPatternEditorGridWidth = value;
}

inline const WindowProperties& Preferences::getMainFormProperties() const
{
	return m_mainFormProperties;
}
inline void Preferences::setMainFormProperties( const WindowProperties& prop )
{
	m_mainFormProperties = prop;
}

inline const WindowProperties& Preferences::getMixerProperties() const
{
	return m_mixerProperties;
}
inline void Preferences::setMixerProperties( const WindowProperties& prop )
{
	m_mixerProperties = prop;
}

inline const WindowProperties& Preferences::getPatternEditorProperties() const
{
	return m_patternEditorProperties;
}
inline void Preferences::setPatternEditorProperties(
	const WindowProperties& prop
)
{
	m_patternEditorProperties = prop;
}

inline const WindowProperties& Preferences::getSongEditorProperties() const
{
	return m_songEditorProperties;
}
inline void Preferences::setSongEditorProperties( const WindowProperties& prop )
{
	m_songEditorProperties = prop;
}

inline const WindowProperties& Preferences::getRackProperties() const
{
	return m_rackProperties;
}
inline void Preferences::setRackProperties( const WindowProperties& prop )
{
	m_rackProperties = prop;
}

inline const WindowProperties& Preferences::getAudioEngineInfoProperties() const
{
	return m_audioEngineInfoProperties;
}
inline void Preferences::setAudioEngineInfoProperties(
	const WindowProperties& prop
)
{
	m_audioEngineInfoProperties = prop;
}

inline const WindowProperties& Preferences::getPlaylistEditorProperties() const
{
	return m_playlistEditorProperties;
}
inline void Preferences::setPlaylistEditorProperties(
	const WindowProperties& prop
)
{
	m_playlistEditorProperties = prop;
}

inline const WindowProperties& Preferences::getDirectorProperties() const
{
	return m_directorProperties;
}
inline void Preferences::setDirectorProperties( const WindowProperties& prop )
{
	m_directorProperties = prop;
}

inline void Preferences::setMaxBars( const int bars )
{
	m_nMaxBars = bars;
}

inline int Preferences::getMaxBars() const
{
	return m_nMaxBars;
}

inline Midi::Channel Preferences::getMidiFeedbackChannel() const {
	return m_midiFeedbackChannel;
}
inline void Preferences::setMidiFeedbackChannel( Midi::Channel channel )
{
	m_midiFeedbackChannel = channel;
}
inline bool Preferences::getMidiClockInputHandling() const
{
	return m_bMidiClockInputHandling;
}
inline void Preferences::setMidiClockInputHandling( bool bHandle )
{
	m_bMidiClockInputHandling = bHandle;
}
inline bool Preferences::getMidiTransportInputHandling() const
{
	return m_bMidiTransportInputHandling;
}
inline void Preferences::setMidiTransportInputHandling( bool bHandle )
{
	m_bMidiTransportInputHandling = bHandle;
}
inline bool Preferences::getMidiClockOutputSend() const
{
	return m_bMidiClockOutputSend;
}
inline void Preferences::setMidiClockOutputSend( bool bHandle )
{
	m_bMidiClockOutputSend = bHandle;
}
inline bool Preferences::getMidiTransportOutputSend() const
{
	return m_bMidiTransportOutputSend;
}
inline void Preferences::setMidiTransportOutputSend( bool bHandle )
{
	m_bMidiTransportOutputSend = bHandle;
}
inline Preferences::MidiSendNoteOff Preferences::getMidiSendNoteOff() const
{
	return m_midiSendNoteOff;
}
inline void Preferences::setMidiSendNoteOff(
	Preferences::MidiSendNoteOff noteOff
)
{
	m_midiSendNoteOff = noteOff;
}
inline bool Preferences::getOscServerEnabled() const
{
	return m_bOscServerEnabled;
}
inline void Preferences::setOscServerEnabled( bool val )
{
	m_bOscServerEnabled = val;
}

inline bool Preferences::getOscFeedbackEnabled() const
{
	return m_bOscFeedbackEnabled;
}
inline void Preferences::setOscFeedbackEnabled( bool val )
{
	m_bOscFeedbackEnabled = val;
}

inline int Preferences::getOscServerPort() const
{
	return m_nOscServerPort;
}
inline void Preferences::setOscServerPort( int oscPort )
{
	m_nOscServerPort = oscPort;
}

inline void Preferences::setShowPlaybackTrack( bool val )
{
	m_bShowPlaybackTrack = val;
}
inline bool Preferences::getShowPlaybackTrack() const
{
	return m_bShowPlaybackTrack;
}

inline int Preferences::getRubberBandBatchMode() const
{
	return m_bUseTheRubberbandBpmChangeEvent;
}
inline void Preferences::setRubberBandBatchMode( int val )
{
	m_bUseTheRubberbandBpmChangeEvent = val;
}

inline int Preferences::getLastOpenTab() const
{
	return m_nLastOpenTab;
}
inline void Preferences::setLastOpenTab( int n )
{
	m_nLastOpenTab = n;
}

inline void Preferences::setTheme( std::shared_ptr<Theme> pTheme )
{
	m_pTheme = pTheme;
}
inline const std::shared_ptr<const Theme> Preferences::getTheme() const
{
	return m_pTheme;
}
inline const std::shared_ptr<const ColorTheme> Preferences::getColorTheme(
) const
{
	return m_pTheme->m_pColor;
}
inline const std::shared_ptr<const InterfaceTheme>
Preferences::getInterfaceTheme() const
{
	return m_pTheme->m_pInterface;
}
inline const std::shared_ptr<const FontTheme> Preferences::getFontTheme() const
{
	return m_pTheme->m_pFont;
}
inline std::shared_ptr<Theme> Preferences::getThemeWritable()
{
	return m_pTheme;
}

inline const std::shared_ptr<Shortcuts> Preferences::getShortcuts() const
{
	return m_pShortcuts;
}
inline void Preferences::setShortcuts(
	const std::shared_ptr<Shortcuts> pShortcuts
)
{
	m_pShortcuts = pShortcuts;
}
inline const std::shared_ptr<MidiEventMap> Preferences::getMidiEventMap() const
{
	return m_pMidiEventMap;
}
inline void Preferences::setMidiEventMap(
	const std::shared_ptr<MidiEventMap> pMidiEventMap
)
{
	m_pMidiEventMap = pMidiEventMap;
}
inline const std::shared_ptr<MidiInstrumentMap>
Preferences::getMidiInstrumentMap() const
{
	return m_pMidiInstrumentMap;
}
inline void Preferences::setMidiInstrumentMap(
	const std::shared_ptr<MidiInstrumentMap> pMidiInstrumentMap
)
{
	m_pMidiInstrumentMap = pMidiInstrumentMap;
}
inline bool Preferences::getLoadingSuccessful() const
{
	return m_bLoadingSuccessful;
}
};	// namespace H2Core

#endif
