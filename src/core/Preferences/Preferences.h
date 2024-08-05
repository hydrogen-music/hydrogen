/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <list>
#include <vector>
#include <cassert>
#include <memory>

#include <core/Preferences/Theme.h>
#include <core/Preferences/Shortcuts.h>

#include <core/MidiAction.h>
#include <core/Globals.h>
#include <core/Object.h>

#include <QStringList>
#include <QDomDocument>
#include <QColor>

namespace H2Core
{


/**
\ingroup H2CORE
*/
/** \ingroup docCore docConfiguration*/
class WindowProperties : public H2Core::Object<WindowProperties>
{
	H2_OBJECT(WindowProperties)
public:
	int x;
	int y;
	int width;
	int height;
	bool visible;
	QByteArray m_geometry;

	WindowProperties();
	WindowProperties(const WindowProperties &other);
	~WindowProperties();

	void set(int _x, int _y, int _width, int _height, bool _visible, const QByteArray& geometry = QByteArray() ) {
		x = _x; y = _y;
		width = _width; height = _height;
		visible = _visible;
		m_geometry = geometry;
	}

};

/**
\ingroup H2CORE
\brief	Manager for User Preferences File (singleton)
*/
/** \ingroup docCore docConfiguration*/
class Preferences : public H2Core::Object<Preferences>
{
	H2_OBJECT(Preferences)
public:
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
	       * Specifies that Hydrogen is using in the time master
	       * mode and will thus control specific aspects of the
	       * transport like the overall tempo. Its counterpart is
	       * #NO_JACK_TIME_MASTER.
	       */
	      USE_JACK_TIME_MASTER = 0,
	      SET_PLAY_ON = 0,
	      BC_ON = 0,/** 
	       * Specifies whether or not to use JACK transport
	       * capabilities. If set, Hydrogen can be used
	       * independent of the JACK system while still using the
	       * JackAudioDriver. Its counterpart is
	       * #USE_JACK_TRANSPORT.
	       */
	      NO_JACK_TRANSPORT = 1,
	      /**
	       * Specifies that Hydrogen is note using in the time
	       * master mode. Its counterpart is
	       * #USE_JACK_TIME_MASTER.
	       */
	      NO_JACK_TIME_MASTER = 1,
	      SET_PLAY_OFF = 1,
	      BC_OFF = 1
	};

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
			or-ing the didicated option.*/
		AppearanceTab = 0x004,
		/** Any option in the General tab appeared.*/
		GeneralTab = 0x008,
		/** Any option in the Audio tab appeared.*/
		AudioTab = 0x010,
		/** Any option in the MIDI tab appeared.*/
		MidiTab = 0x020,
		/** Any option in the OSC tab appeared.*/
		OscTab = 0x040,
		/** At least one shortcut was changed.*/
		ShortcutTab = 0x080,
	};

	bool				m_bPlaySamplesOnClicking; // audio file browser

	bool				m_bPlaySelectedInstrument; // midi keys and keys play instrument or drumset

	bool				m_bFollowPlayhead;

	// switch to enable / disable lash, only on h2 startup
	bool				m_bRestartLash;
	bool				m_bSetLash;

	//soundlibrarypanel expand song and pattern item
	bool				m_bExpandSongItem;
	bool				m_bExpandPatternItem;

	//beatcounter
	bool				m_bBbc;
	bool				m_bMmcSetPlay;

	int					m_nCountOffset;
	int					m_nStartOffset;
	// ~ beatcounter

	std::list<QString> 		m_serverList;
	std::list<QString> 		m_patternCategories;

	//	audio engine properties ___
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
		PortAudio
	};
	static AudioDriver parseAudioDriver( const QString& sDriver );
	static QString audioDriverToQString( const AudioDriver& driver );
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
	 * Audio driver
	 *
	 * Used in the audioEngine_startAudioDrivers() to create an
	 * audio driver using createDriver(). 
	 *
	 * These choices are support:
	 * - "Auto" : audioEngine_startAudioDrivers() will try
	 *   different drivers itself.
	 * - "Jack" : createDriver() will create a JackAudioDriver.
	 * - "Alsa" : createDriver() will create a AlsaAudioDriver.
	 * - "CoreAudio" : createDriver() will create a CoreAudioDriver.
	 * - "PortAudio" : createDriver() will create a PortAudioDriver.
	 * - "Oss" : createDriver() will create a OssDriver.
	 * - "PulseAudio" : createDriver() will create a PulseAudioDriver.
	 * - "Fake" : createDriver() will create a FakeDriver.
	 */
	AudioDriver			m_audioDriver;
	/** If set to true, samples of the metronome will be added to
	 * #H2Core::AudioEngine::m_songNoteQueue and thus played back on a
	 * regular basis.*/
	bool				m_bUseMetronome;
	/// Metronome volume FIXME: remove this volume!!
	float				m_fMetronomeVolume;
	/// max notes
	unsigned			m_nMaxNotes;
	/** 
	 * Buffer size of the audio.
	 *
	 * It is set e.g. by JackAudioDriver::init() to the buffer
	 * size of the freshly opened JACK client.
	 */
	unsigned			m_nBufferSize;
	/** 
	 * Sample rate of the audio.
	 *
	 * It is set e.g. by JackAudioDriver::init() to the sample
	 * rate of the freshly opened JACK client.
	 */
	unsigned			m_nSampleRate;

	//	OSS driver properties ___
	QString				m_sOSSDevice;		///< Device used for output

	//	MIDI Driver properties
	/**
	 * MIDI driver
	 *
	 * Used in the audioEngine_startAudioDrivers() to create an
	 * MIDI driver. 
	 *
	 * These choices are support:
	 * - "JackMidi" : A JackMidiDriver will be called.
	 * - "ALSA" : An AlsaMidiDriver will be called.
	 * - "CoreMidi" : A CoreMidiDriver will be called.
	 * - "PortMidi" : A PortMidiDriver will be called.
	 */
	QString				m_sMidiDriver;
	QString				m_sMidiPortName;
	QString				m_sMidiOutputPortName;
	/**
	 * Choice of #m_sMidiPortName and #m_sMidiOutputPortName in case
	 * no port/device was selected.
	 *
	 * Pinning its value to "None" will prevent Hydrogen to connect to
	 * ports/devices using this exact name but is still done for
	 * backward compatibility.
	 */
	static QString getNullMidiPort() {
		return "None";
	}
	
	int					m_nMidiChannelFilter;
	bool				m_bMidiNoteOffIgnore;
	bool				m_bMidiFixedMapping;
	bool				m_bMidiDiscardNoteAfterAction;
	bool				m_bEnableMidiFeedback;
	
	// OSC Server properties
	/**
	 * Whether to start the OscServer thread.
	 *
	 * If set to true, the OscServer::start() function of the
	 * OscServer singleton will be called in
	 * Hydrogen::Hydrogen(). This will register all OSC message
	 * handlers and makes the server listen to port
	 * #m_nOscServerPort.
	 *
	 * Set by setOscServerEnabled() and queried by
	 * getOscServerEnabled().
	 */
	bool				m_bOscServerEnabled;
	/**
	 * Whether to send the current state of Hydrogen to the OSC
	 * clients.
	 *
	 * If set to true, the current state of Hydrogen will be sent to
	 * \e all known OSC clients using
	 * CoreActionController::initExternalControlInterfaces() and
	 * OscServer::handleAction() via OSC messages each time it gets
	 * updated..
	 
	 * Set by setOscFeedbackEnabled() and queried by
	 * getOscFeedbackEnabled().
	 */
	bool				m_bOscFeedbackEnabled;
	/**
	 * In case #m_nOscServerPort is already occupied by another
	 * client, the alternative - random - port number provided by the
	 * OSC server will be stored in this variable. If the connection
	 * using the default port succeeded, the variable will be set to
	 * -1.
	 */
	int					m_nOscTemporaryPort;
	/**
	 * Port number the OscServer will be started at.
	 
	 * Set by setOscServerPort() and queried by
	 * getOscServerPort().
	 */
	int					m_nOscServerPort;

	//	alsa audio driver properties ___
	QString				m_sAlsaAudioDevice;

	// PortAudio properties
	QString				m_sPortAudioDevice;
	QString				m_sPortAudioHostAPI;
	int					m_nLatencyTarget;

	// CoreAudio properties
	QString				m_sCoreAudioDevice;

	//	jack driver properties ___
	QString				m_sJackPortName1;
	QString				m_sJackPortName2;
	/**
	 * Specifies whether or not Hydrogen will use the JACK
	 * transport system. It has two different states:
	 * #USE_JACK_TRANSPORT and #NO_JACK_TRANSPORT.
	 */
	int					m_bJackTransportMode;
	/** Toggles auto-connecting of the main stereo output ports to the
		system's default ports when starting the JACK server.*/
	bool				m_bJackConnectDefaults;
	/** 
	 * If set to _true_, JackAudioDriver::makeTrackOutputs() will
	 * create two individual left and right output ports for every
	 * component of each instrument. If _false_, one usual stereo
	 * output will be created.
	 */
	bool				m_bJackTrackOuts;

	/** Specifies which audio settings will be applied to the sample
		supplied in the JACK per track output ports.*/
	enum class JackTrackOutputMode {
		/** Applies layer, component, and instrument gain, note and
		instrument pan, note velocity, and main component and
		instrument volume to the samples. */
		postFader = 0,
		/** Only layer gain and note velocity will be applied to the samples.*/
		preFader = 1 };

	/** Specifies which audio settings will be applied to the sample
		supplied in the JACK per track output ports.*/
	JackTrackOutputMode		m_JackTrackOutputMode;
	//jack time master

	/**
	 * External applications with a faulty JACK timebase master
	 * implementation can mess up the transport within Hydrogen. To
	 * guarantee the basic functionality, the user can disable
	 * timebase support and make Hydrogen only listen to the frame
	 * number broadcast by the JACK server.
	 */
	bool				m_bJackTimebaseEnabled;
	/**
	 * Specifies if Hydrogen should run as JACK time master. It
	 * has two states: Preferences::USE_JACK_TIME_MASTER and
	 * Preferences::NO_JACK_TIME_MASTER. It is set to
	 * Preferences::NO_JACK_TIME_MASTER by the
	 * JackAudioDriver::initTimebaseMaster() if Hydrogen couldn't be
	 * registered as time master.
	 */
	int					m_bJackMasterMode;
	// ~ jack driver properties

	///Default text editor (used by Playlisteditor)
	QString				m_sDefaultEditor;
	int				m_nAutosavesPerHour;

	///Rubberband CLI
	QString				m_sRubberBandCLIexecutable;

	/**
	 * If #__instance equals 0, a new Preferences singleton will
	 * be created and stored in it.
	 *
	 * It is called in Hydrogen::create_instance().
	 */
	static void				create_instance();
	/**
	 * Returns a pointer to the current Preferences singleton
	 * stored in #__instance.
	 */
	static std::shared_ptr<Preferences> get_instance(){ assert(__instance); return __instance; }

	~Preferences();

		/** Exchange the instance referenced by the current singleton with
		 * another one. */
		void replaceInstance( std::shared_ptr<Preferences> pOther );

		static std::shared_ptr<Preferences>	load( const QString& sPath, bool bSilent = false );
		/** Save the config to the user-level config file (or the one specified
		 * via CLI) */
		bool			save( const bool bSilent = false) const;
		/** Instead of a `saveAs` method #Preferences only provides a
		 * #saveCopyAs() method to indicate that corresponding file won't change
		 * and will always be the user-level config file. (Which can be altered
		 * using #Filesystem::m_sPreferencesOverwritePath) */
		bool			saveCopyAs( const QString& sPath,
									const bool bSilent = false ) const;

	const QString&	getDataDirectory() const;

	const QString&	getDefaultEditor() const;
	void			setDefaultEditor( const QString& editor);

	// General
	const QString&	getPreferredLanguage() const;
	void			setPreferredLanguage( const QString& sLanguage );

	void			setUseRelativeFilenamesForPlaylists( bool value );

	void			setShowDevelWarning( bool value );
	bool			getShowDevelWarning() const;

	bool			getShowNoteOverwriteWarning() const;
	void			setShowNoteOverwriteWarning( bool bValue );

	bool			isPlaylistUsingRelativeFilenames() const;

	void			setLastSongFilename( const QString& filename );
	const QString&	getLastSongFilename() const;

	void			setLastPlaylistFilename( const QString& filename );
	const QString&	getLastPlaylistFilename() const;

	void			setHearNewNotes( bool value );
	bool			getHearNewNotes() const;

	void			setRecordEvents( bool value );
	bool			getRecordEvents() const;

	void			setPunchInPos ( unsigned pos );
	int				getPunchInPos() const;

	void			setPunchOutPos ( unsigned pos );
	int				getPunchOutPos() const;

	bool			inPunchArea (int pos) const;
	void			unsetPunchArea();

	void			setQuantizeEvents( bool value );
	bool			getQuantizeEvents() const;

	const std::vector<QString>& 		getRecentFiles() const;
	void			setRecentFiles( const std::vector<QString>& recentFiles );

	const QStringList&	getRecentFX() const;
	void			setMostRecentFX( const QString& );


	// GUI Properties
	bool			showInstrumentPeaks() const;
	void			setInstrumentPeaks( bool value );

	int				getPatternEditorGridResolution() const;
	void			setPatternEditorGridResolution( int value );

	bool			isPatternEditorUsingTriplets() const;
	void			setPatternEditorUsingTriplets( bool value );

	bool			isFXTabVisible() const;
	void			setFXTabVisible( bool value );
	
	bool			getShowAutomationArea() const;
	void			setShowAutomationArea( bool value );

	unsigned		getPatternEditorGridHeight() const;
	void			setPatternEditorGridHeight( unsigned value );

	unsigned		getPatternEditorGridWidth() const;
	void			setPatternEditorGridWidth( unsigned value );

	unsigned		getSongEditorGridHeight() const;
	void			setSongEditorGridHeight( unsigned value );

	unsigned		getSongEditorGridWidth() const;
	void			setSongEditorGridWidth( unsigned value );

	const WindowProperties&	getMainFormProperties() const;
	void				setMainFormProperties( const WindowProperties& prop );

	const WindowProperties& getMixerProperties() const;
	void				setMixerProperties( const WindowProperties& prop );

	const WindowProperties& getPatternEditorProperties() const;
	void				setPatternEditorProperties( const WindowProperties& prop );

	const WindowProperties& getSongEditorProperties() const;
	void				setSongEditorProperties( const WindowProperties& prop );

	const WindowProperties& getInstrumentRackProperties() const;
	void				setInstrumentRackProperties( const WindowProperties& prop );

	const WindowProperties& getAudioEngineInfoProperties() const;
	void				setAudioEngineInfoProperties( const WindowProperties& prop );

	const WindowProperties& getLadspaProperties( unsigned nFX ) const;
	void			setLadspaProperties( unsigned nFX, const WindowProperties& prop );

	const WindowProperties& getPlaylistEditorProperties() const;
	void				setPlaylistEditorProperties( const WindowProperties& prop );

	const WindowProperties& getDirectorProperties() const;
	void				setDirectorProperties( const WindowProperties& prop );

	bool			useLash() const;
	void			setUseLash( bool b );

	bool			hideKeyboardCursor() const;
	void			setHideKeyboardCursor( bool b );

	/** @param bars Sets #m_nMaxBars.*/
	void				setMaxBars( const int bars );
	/** @return #m_nMaxBars.*/
	int				getMaxBars() const;

	/** @param layers Sets #m_nMaxLayers.*/
	void			setMaxLayers( const int layers );
	/** @return #m_nMaxLayers.*/
	int				getMaxLayers() const;

	void			setWaitForSessionHandler(bool value);
	bool			getWaitForSessionHandler() const;

#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_
	void			setNsmClientId(const QString& nsmClientId);
	const QString&		getNsmClientId(void) const;

	void			setNsmSongName(const QString& nsmSongName);
	const QString& 		getNsmSongName(void) const;
#endif

	/** \return #m_bOscServerEnabled*/
	bool			getOscServerEnabled() const;
	/** \param val Sets #m_bOscServerEnabled*/
	void			setOscServerEnabled( bool val );
	/** \return #m_bOscFeedbackEnabled*/
	bool			getOscFeedbackEnabled() const;
	/** \param val Sets #m_bOscFeedbackEnabled*/
	void			setOscFeedbackEnabled( bool val );
	/** \return #m_nOscServerPort*/
	int				getOscServerPort() const;
	/** \param oscPort Sets #m_nOscServerPort*/
	void			setOscServerPort( int oscPort );

	/** Whether to use the bpm of the timeline.*/
	bool			getUseTimelineBpm() const;
	void			setUseTimelineBpm( bool val );
	
	void			setShowPlaybackTrack( bool val);
	bool			getShowPlaybackTrack() const;

	int				getRubberBandBatchMode() const;
	void			setRubberBandBatchMode( int val );

	int				getLastOpenTab() const;
	void			setLastOpenTab(int n);

	void			setH2ProcessName(const QString& processName);

	const QString&	getH2ProcessName() const;

	const QString&	getLastExportPatternAsDirectory() const;
	const QString&	getLastExportSongDirectory() const;
	const QString&	getLastSaveSongAsDirectory() const;
	const QString&	getLastOpenSongDirectory() const;
	const QString&	getLastOpenPatternDirectory() const;
	const QString&	getLastExportLilypondDirectory() const;
	const QString&	getLastExportMidiDirectory() const;
	const QString&	getLastImportDrumkitDirectory() const;
	const QString&	getLastExportDrumkitDirectory() const;
	const QString&	getLastOpenLayerDirectory() const;
	const QString&	getLastOpenPlaybackTrackDirectory() const;
	const QString&	getLastAddSongToPlaylistDirectory() const;
	const QString&	getLastPlaylistDirectory() const;
	const QString&	getLastPlaylistScriptDirectory() const;
	const QString&	getLastImportThemeDirectory() const;
	const QString&	getLastExportThemeDirectory() const;
	void			setLastExportPatternAsDirectory( const QString& sPath );
	void			setLastExportSongDirectory( const QString& sPath );
	void			setLastSaveSongAsDirectory( const QString& sPath );
	void			setLastOpenSongDirectory( const QString& sPath );
	void			setLastOpenPatternDirectory( const QString& sPath );
	void			setLastExportLilypondDirectory( const QString& sPath );
	void			setLastExportMidiDirectory( const QString& sPath );
	void			setLastImportDrumkitDirectory( const QString& sPath );
	void			setLastExportDrumkitDirectory( const QString& sPath );
	void			setLastOpenLayerDirectory( const QString& sPath );
	void			setLastOpenPlaybackTrackDirectory( const QString& sPath );
	void			setLastAddSongToPlaylistDirectory( const QString& sPath );
	void			setLastPlaylistDirectory( const QString& sPath );
	void			setLastPlaylistScriptDirectory( const QString& sPath );
	void			setLastImportThemeDirectory( const QString& sPath );
	void			setLastExportThemeDirectory( const QString& sPath );

	int				getExportSampleDepthIdx() const;
	void			setExportSampleDepthIdx( int nExportSampleDepthIdx );
	
	int				getExportSampleRateIdx() const;
	void			setExportSampleRateIdx( int nExportSampleRateIdx );

	int				getExportModeIdx() const;
	void			setExportModeIdx(int nExportMode);
	
	int				getExportTemplateIdx() const;
	void			setExportTemplateIdx( int nExportTemplateIdx );

    int				getMidiExportMode() const;
    void			setMidiExportMode(int nExportMode);

	bool			m_bShowExportSongLicenseWarning;
	bool			m_bShowExportDrumkitLicenseWarning;
	bool			m_bShowExportDrumkitCopyleftWarning;
	bool			m_bShowExportDrumkitAttributionWarning;

	const Theme& getTheme() const;
	Theme& getThemeWritable();
	void setTheme( const Theme& pTheme );

	const std::shared_ptr<Shortcuts> getShortcuts() const;
	void setShortcuts( const std::shared_ptr<Shortcuts> pShortcuts );

	bool getLoadingSuccessful() const;
	
private:
	/**
	 * Object holding the current Preferences singleton. It is
	 * initialized with NULL, set with create_instance(), and
	 * accessed with get_instance().
	 */
	static std::shared_ptr<Preferences>		__instance;

		bool saveTo( const QString& sPath, const bool bSilent ) const;

	Theme		m_theme;
	std::shared_ptr<Shortcuts>  m_pShortcuts;
	
	//___ General properties ___
	QString				m_sH2ProcessName; //Name of hydrogen's main process
	 ///rubberband bpm change queue
	bool				m_bUseTheRubberbandBpmChangeEvent;

	bool				m_bUseLash;
	///< Show development version warning?
	bool				m_bShowDevelWarning;
	bool				m_bShowNoteOverwriteWarning;
	///< Last song used
	QString				m_sLastSongFilename;
	QString				m_sLastPlaylistFilename;

	bool				m_bQuantizeEvents;
	bool				m_bRecordEvents;
		/** In case the rubberband binary was not found in common places, this
		 * variable indicated - if `true` - that Hydrogen should continue
		 * searching for it in places provided during #load() */
	bool				m_bSearchForRubberbandOnLoad;
	int					m_nPunchInPos;
	int					m_nPunchOutPos;
	bool				m_bHideKeyboardCursor;
	/** Maximum number of bars shown in the Song Editor at
	 * once. 
	 *
	 * It is set by setMaxBars() and queried by
	 * getMaxBars(). In order to change this value, you have to
	 * manually edit the \<maxBars\> tag in the configuration file
	 * of Hydrogen in your home folder. Default value assigned in
	 * constructor: 400.*/
	int					m_nMaxBars;
	/** Maximum number of layers to be used in the Instrument
	 *  editor. 
	 *
	 * It is set by setMaxLayers() and queried by
	 * getMaxLayers(). It is setIn order to change this value, you
	 * have to manually edit the \<maxLayers\> tag in the
	 * configuration file of Hydrogen in your home folder. Default
	 * value assigned in constructor: 16. */
	int					m_nMaxLayers;
	bool				m_bHearNewNotes;

	QStringList			m_recentFX;
	std::vector<QString> 		m_recentFiles;

#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_
		QString			m_sNsmClientId;
		QString			m_sNsmSongName;
#endif

	bool					m_bWaitingForSessionHandler;
	/**
	 * Whether to use local speeds specified along the Timeline or
	 * a constant tempo for the whole Song in
	 * Hydrogen::getTimelineBpm() and Hydrogen::getTimelineBpm().
	 *
	 * It is set using setUseTimelineBpm() and accessed via
	 * getUseTimelineBpm().
	 */
	bool					m_bUseTimelineBpm;


	//___ GUI properties ___
	int						m_nLastOpenTab;
	bool					m_bShowPlaybackTrack;

	int						m_nPatternEditorGridResolution;
	bool					m_bPatternEditorUsingTriplets;
	bool					m_bShowInstrumentPeaks;
	bool					m_bIsFXTabVisible;
	bool					m_bShowAutomationArea;
	bool					m_bUseRelativeFilenamesForPlaylists;
	unsigned				m_nPatternEditorGridHeight;
	unsigned				m_nPatternEditorGridWidth;
	unsigned				m_nSongEditorGridHeight;
	unsigned				m_nSongEditorGridWidth;
	WindowProperties		m_mainFormProperties;
	WindowProperties		m_mixerProperties;
	WindowProperties		m_patternEditorProperties;
	WindowProperties		m_songEditorProperties;
	WindowProperties		m_instrumentRackProperties;
	WindowProperties		m_audioEngineInfoProperties;
	WindowProperties		m_ladspaProperties[MAX_FX];
	WindowProperties		m_playlistEditorProperties;
	WindowProperties		m_directorProperties;

	QString					m_sPreferredLanguage;

	// Last directories used in QFileDialogs
	QString					m_sLastExportPatternAsDirectory;
	QString					m_sLastExportSongDirectory;
	QString					m_sLastSaveSongAsDirectory;
	QString					m_sLastOpenSongDirectory;
	QString					m_sLastOpenPatternDirectory;
	QString					m_sLastExportLilypondDirectory;
	QString					m_sLastExportMidiDirectory;
	QString					m_sLastImportDrumkitDirectory;
	QString					m_sLastExportDrumkitDirectory;
	QString					m_sLastOpenLayerDirectory;
	QString					m_sLastOpenPlaybackTrackDirectory;
	QString					m_sLastAddSongToPlaylistDirectory;
	QString					m_sLastPlaylistDirectory;
	QString					m_sLastPlaylistScriptDirectory;
	QString					m_sLastImportThemeDirectory;
	QString					m_sLastExportThemeDirectory;

	//Export dialog
	int						m_nExportModeIdx;
	int						m_nExportSampleRateIdx;
	int						m_nExportSampleDepthIdx;
	int						m_nExportTemplateIdx;
	// ~ Export dialog

    // Export midi dialog
    int						m_nMidiExportMode;
    // ~ Export midi dialog
	
	/** Full path to local preferences file.
	 *
	 * Used in nsm_open_cb() to specify a preferences file specific to
	 * the current session.
	 *
	 * If non-empty, the local file will be loaded instead of
	 * Filesystem::usr_config_path() or
	 * Filesystem::sys_config_path(). In general the underlying file
	 * does not have to be named "hydrogen.conf". But for the sake of
	 * consistency the latter naming is strongly recommended.
	 *
	 * Note that this variable is a session variable, which won't be
	 * stored in the hydrogen.conf preferences file!
	 */
	QString					m_sPreferencesOverwritePath;
	
	Preferences();

	static WindowProperties loadWindowPropertiesFrom( const XMLNode& parent,
													  const QString& sWindowName,
													  const WindowProperties& defaultProp,
													  const bool bSilent = false );
	static void saveWindowPropertiesTo( XMLNode& parent, const QString& sWindowName,
										const WindowProperties& prop );

	bool m_bLoadingSuccessful;

		/** Used to indicate changes in the underlying XSD file. */
		static constexpr int nCurrentFormatVersion = 2;
};

inline const QString&			Preferences::getLastExportPatternAsDirectory() const {
	return m_sLastExportPatternAsDirectory;
}
inline const QString&			Preferences::getLastExportSongDirectory() const {
	return m_sLastExportSongDirectory;
}
inline const QString&			Preferences::getLastSaveSongAsDirectory() const {
	return m_sLastSaveSongAsDirectory;
}
inline const QString&			Preferences::getLastOpenSongDirectory() const {
	return m_sLastOpenSongDirectory;
}
inline const QString&			Preferences::getLastOpenPatternDirectory() const {
	return m_sLastOpenPatternDirectory;
}
inline const QString&			Preferences::getLastExportLilypondDirectory() const {
	return m_sLastExportLilypondDirectory;
}
inline const QString&			Preferences::getLastExportMidiDirectory() const {
	return m_sLastExportMidiDirectory;
}
inline const QString&			Preferences::getLastImportDrumkitDirectory() const {
	return m_sLastImportDrumkitDirectory;
}
inline const QString&			Preferences::getLastExportDrumkitDirectory() const {
	return m_sLastExportDrumkitDirectory;
}
inline const QString&			Preferences::getLastOpenLayerDirectory() const {
	return m_sLastOpenLayerDirectory;
}
inline const QString&			Preferences::getLastOpenPlaybackTrackDirectory() const {
	return m_sLastOpenPlaybackTrackDirectory;
}
inline const QString&			Preferences::getLastAddSongToPlaylistDirectory() const {
	return m_sLastAddSongToPlaylistDirectory;
}
inline const QString&			Preferences::getLastPlaylistDirectory() const {
	return m_sLastPlaylistDirectory;
}
inline const QString&			Preferences::getLastPlaylistScriptDirectory() const {
	return m_sLastPlaylistScriptDirectory;
}
inline const QString&			Preferences::getLastImportThemeDirectory() const {
	return m_sLastImportThemeDirectory;
}
inline const QString&			Preferences::getLastExportThemeDirectory() const {
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
inline void Preferences::setLastOpenLayerDirectory( const QString& sPath )
{
	m_sLastOpenLayerDirectory = sPath;
}
inline void Preferences::setLastOpenPlaybackTrackDirectory( const QString& sPath )
{
	m_sLastOpenPlaybackTrackDirectory = sPath;
}
inline void Preferences::setLastAddSongToPlaylistDirectory( const QString& sPath )
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

inline void Preferences::setMidiExportMode(int ExportMode)
{
	m_nMidiExportMode = ExportMode;
}

inline int Preferences::getExportSampleDepthIdx() const
{
	return m_nExportSampleDepthIdx;
}

inline void Preferences::setExportSampleDepthIdx(int ExportSampleDepth)
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

inline void Preferences::setExportModeIdx(int ExportModeIdx)
{
	m_nExportModeIdx = ExportModeIdx;
}

inline void Preferences::setExportSampleRateIdx(int ExportSampleRate)
{
	m_nExportSampleRateIdx = ExportSampleRate;
}

inline int Preferences::getExportTemplateIdx() const
{
	return m_nExportTemplateIdx;
}

inline void Preferences::setExportTemplateIdx(int ExportTemplateIdx)
{
	m_nExportTemplateIdx = ExportTemplateIdx;
}

inline const QString& Preferences::getDefaultEditor() const {
	return m_sDefaultEditor;
}

inline void Preferences::setDefaultEditor( const QString& editor){
	m_sDefaultEditor = editor;
}

// General
inline const QString& Preferences::getPreferredLanguage() const {
	return m_sPreferredLanguage;
}

inline void Preferences::setPreferredLanguage( const QString& sLanguage ) {
	m_sPreferredLanguage = sLanguage;
}

inline void Preferences::setUseRelativeFilenamesForPlaylists( bool value ) {
	m_bUseRelativeFilenamesForPlaylists= value;
}

inline void Preferences::setShowDevelWarning( bool value ) {
	m_bShowDevelWarning = value;
}

inline bool Preferences::getShowDevelWarning() const {
	return m_bShowDevelWarning;
}

inline bool Preferences::getShowNoteOverwriteWarning() const {
	return m_bShowNoteOverwriteWarning;
}

inline void Preferences::setShowNoteOverwriteWarning( bool bValue ) {
	m_bShowNoteOverwriteWarning = bValue;
}

inline void Preferences::setHideKeyboardCursor( bool value ) {
	m_bHideKeyboardCursor = value;
}

inline bool Preferences::hideKeyboardCursor() const {
	return m_bHideKeyboardCursor;
}

inline bool Preferences::isPlaylistUsingRelativeFilenames() const {
	return m_bUseRelativeFilenamesForPlaylists;
}

inline void Preferences::setLastSongFilename( const QString& filename ) {
	m_sLastSongFilename = filename;
}
inline const QString& Preferences::getLastSongFilename() const {
	return m_sLastSongFilename;
}

inline void Preferences::setLastPlaylistFilename( const QString& filename ) {
	m_sLastPlaylistFilename = filename;
}
inline const QString& Preferences::getLastPlaylistFilename() const {
	return m_sLastPlaylistFilename;
}

inline void Preferences::setHearNewNotes( bool value ) {
	m_bHearNewNotes = value;
}
inline bool Preferences::getHearNewNotes() const {
	return m_bHearNewNotes;
}

inline void Preferences::setRecordEvents( bool value ) {
	m_bRecordEvents = value;
}
inline bool Preferences::getRecordEvents() const {
	return m_bRecordEvents;
}

inline void Preferences::setPunchInPos ( unsigned pos ) {
	m_nPunchInPos = pos;
}
inline int Preferences::getPunchInPos() const {
	return m_nPunchInPos;
}

inline void Preferences::setPunchOutPos ( unsigned pos ) {
	m_nPunchOutPos = pos;
}
inline int Preferences::getPunchOutPos() const {
	return m_nPunchOutPos;
}

inline bool Preferences::inPunchArea (int pos) const {
	// Return true if punch area not defined
	if ( m_nPunchInPos <= m_nPunchOutPos ) {
		if ( pos < m_nPunchInPos || m_nPunchOutPos < pos ) {
			return false;
		}
	}
	return true;
}

inline void Preferences::unsetPunchArea () {
	m_nPunchInPos = 0;
	m_nPunchOutPos = -1;
}

inline void Preferences::setQuantizeEvents( bool value ) {
	m_bQuantizeEvents = value;
}
inline bool Preferences::getQuantizeEvents() const {
	return m_bQuantizeEvents;
}

inline void Preferences::setRecentFiles( const std::vector<QString>& recentFiles ) {
	m_recentFiles = recentFiles;
}
inline const std::vector<QString>& Preferences::getRecentFiles() const {
	return m_recentFiles;
}

inline const QStringList& Preferences::getRecentFX() const {
	return m_recentFX;
}


// GUI Properties
inline bool Preferences::showInstrumentPeaks() const {
	return m_bShowInstrumentPeaks;
}
inline void Preferences::setInstrumentPeaks( bool value ) {
	m_bShowInstrumentPeaks = value;
}

inline int Preferences::getPatternEditorGridResolution() const {
	return m_nPatternEditorGridResolution;
}
inline void Preferences::setPatternEditorGridResolution( int value ) {
	m_nPatternEditorGridResolution = value;
}

inline bool Preferences::isPatternEditorUsingTriplets() const {
	return m_bPatternEditorUsingTriplets;
}
inline void Preferences::setPatternEditorUsingTriplets( bool value ) {
	m_bPatternEditorUsingTriplets = value;
}

inline bool Preferences::isFXTabVisible() const {
	return m_bIsFXTabVisible;
}
inline void Preferences::setFXTabVisible( bool value ) {
	m_bIsFXTabVisible = value;
}

inline bool Preferences::getShowAutomationArea() const {
	return m_bShowAutomationArea;
}
inline void Preferences::setShowAutomationArea( bool value ) {
	m_bShowAutomationArea = value;
}


inline unsigned Preferences::getSongEditorGridHeight() const {
	return m_nSongEditorGridHeight;
}
inline void Preferences::setSongEditorGridHeight( unsigned value ) {
	m_nSongEditorGridHeight = value;
}
inline unsigned Preferences::getSongEditorGridWidth() const {
	return m_nSongEditorGridWidth;
}
inline void Preferences::setSongEditorGridWidth( unsigned value ) {
	m_nSongEditorGridWidth = value;
}

inline unsigned Preferences::getPatternEditorGridHeight() const {
	return m_nPatternEditorGridHeight;
}
inline void Preferences::setPatternEditorGridHeight( unsigned value ) {
	m_nPatternEditorGridHeight = value;
}
inline unsigned Preferences::getPatternEditorGridWidth() const {
	return m_nPatternEditorGridWidth;
}
inline void Preferences::setPatternEditorGridWidth( unsigned value ) {
	m_nPatternEditorGridWidth = value;
}

inline const WindowProperties& Preferences::getMainFormProperties() const {
	return m_mainFormProperties;
}
inline void Preferences::setMainFormProperties( const WindowProperties& prop ) {
	m_mainFormProperties = prop;
}

inline const WindowProperties& Preferences::getMixerProperties() const {
	return m_mixerProperties;
}
inline void Preferences::setMixerProperties( const WindowProperties& prop ) {
	m_mixerProperties = prop;
}

inline const WindowProperties& Preferences::getPatternEditorProperties() const {
	return m_patternEditorProperties;
}
inline void Preferences::setPatternEditorProperties( const WindowProperties& prop ) {
	m_patternEditorProperties = prop;
}

inline const WindowProperties& Preferences::getSongEditorProperties() const {
	return m_songEditorProperties;
}
inline void Preferences::setSongEditorProperties( const WindowProperties& prop ) {
	m_songEditorProperties = prop;
}


inline const WindowProperties& Preferences::getInstrumentRackProperties() const {
	return m_instrumentRackProperties;
}
inline void Preferences::setInstrumentRackProperties( const WindowProperties& prop ) {
	m_instrumentRackProperties = prop;
}
 
inline const WindowProperties& Preferences::getAudioEngineInfoProperties() const {
	return m_audioEngineInfoProperties;
}
inline void Preferences::setAudioEngineInfoProperties( const WindowProperties& prop ) {
	m_audioEngineInfoProperties = prop;
}

inline const WindowProperties& Preferences::getLadspaProperties( unsigned nFX ) const {
	return m_ladspaProperties[nFX];
}
inline void Preferences::setLadspaProperties( unsigned nFX, const WindowProperties& prop ) {
	m_ladspaProperties[nFX] = prop;
}

inline const WindowProperties& Preferences::getPlaylistEditorProperties() const {
	return m_playlistEditorProperties;
}
inline void Preferences::setPlaylistEditorProperties( const WindowProperties& prop ) {
	m_playlistEditorProperties = prop;
}

inline const WindowProperties& Preferences::getDirectorProperties() const {
	return m_directorProperties;
}
inline void Preferences::setDirectorProperties( const WindowProperties& prop ) {
	m_directorProperties = prop;
}

inline bool Preferences::useLash() const {
	return m_bUseLash;
}
inline void Preferences::setUseLash( bool b ){
	m_bUseLash = b;
}

inline void Preferences::setMaxBars( const int bars ){
	m_nMaxBars = bars;
}

inline int Preferences::getMaxBars() const {
	return m_nMaxBars;
}

inline void Preferences::setMaxLayers( const int layers ){
	m_nMaxLayers = layers;
}

inline int Preferences::getMaxLayers() const {
	return m_nMaxLayers;
}

inline void Preferences::setWaitForSessionHandler(bool value){
	m_bWaitingForSessionHandler = value;
}

inline bool Preferences::getWaitForSessionHandler() const {
		return m_bWaitingForSessionHandler;
}

#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_
inline void Preferences::setNsmClientId(const QString& nsmClientId){
	m_sNsmClientId = nsmClientId;
}

inline const QString& Preferences::getNsmClientId(void) const {
	return m_sNsmClientId;
}

inline void Preferences::setNsmSongName(const QString& nsmSongName){
	m_sNsmSongName = nsmSongName;
}

inline const QString& Preferences::getNsmSongName(void) const {
	return m_sNsmSongName;
}

#endif

inline bool Preferences::getOscServerEnabled() const {
	return m_bOscServerEnabled;
}
inline void Preferences::setOscServerEnabled( bool val ){
	m_bOscServerEnabled = val;
}

inline bool Preferences::getOscFeedbackEnabled() const {
	return m_bOscFeedbackEnabled;
}
inline void Preferences::setOscFeedbackEnabled( bool val ){
	m_bOscFeedbackEnabled = val;
}

inline int Preferences::getOscServerPort() const {
	return m_nOscServerPort;
}
inline void Preferences::setOscServerPort( int oscPort ){
	m_nOscServerPort = oscPort;
}

inline bool Preferences::getUseTimelineBpm() const {
	return m_bUseTimelineBpm;
}
inline void Preferences::setUseTimelineBpm( bool val ){
	m_bUseTimelineBpm = val;
}

inline void Preferences::setShowPlaybackTrack( bool val ) {
	m_bShowPlaybackTrack = val; 
}
inline bool Preferences::getShowPlaybackTrack() const {
	return m_bShowPlaybackTrack;
}

inline int Preferences::getRubberBandBatchMode() const {
	return m_bUseTheRubberbandBpmChangeEvent;
}
inline void Preferences::setRubberBandBatchMode( int val ){
	m_bUseTheRubberbandBpmChangeEvent = val;
}

inline int Preferences::getLastOpenTab() const {
	return m_nLastOpenTab;
}
inline void Preferences::setLastOpenTab(int n){
	m_nLastOpenTab = n;
}

inline void Preferences::setH2ProcessName(const QString& processName){
	m_sH2ProcessName = processName;
}

inline const QString& Preferences::getH2ProcessName() const {
	return m_sH2ProcessName;
}
inline void Preferences::setTheme( const Theme& theme ) {
	m_theme = theme;
}
inline const Theme& Preferences::getTheme() const {
	return m_theme;
}
inline Theme& Preferences::getThemeWritable() {
	return m_theme;
}

inline const std::shared_ptr<Shortcuts> Preferences::getShortcuts() const {
	return m_pShortcuts;
}
inline void Preferences::setShortcuts( const std::shared_ptr<Shortcuts> pShortcuts ) {
	m_pShortcuts = pShortcuts;
}
inline bool Preferences::getLoadingSuccessful() const {
	return m_bLoadingSuccessful;
}
};

#endif

