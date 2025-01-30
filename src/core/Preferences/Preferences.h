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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <list>
#include <vector>
#include <cassert>
#include <memory>

#include "Theme.h"
#include <core/MidiAction.h>
#include <core/Globals.h>
#include <core/Helpers/Filesystem.h>
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

	void set(int _x, int _y, int _width, int _height, bool _visible, QByteArray geometry = QByteArray() ) {
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
	       * Specifies that Hydrogen should attempt to acquire JACK Timebase
	       * control.
	       *
	       * This represent the state desired by the user. The actual one is
	       * stored in H2Core::JackAudioDriver::m_timebaseState.
	       *
	       * Its counterpart is #NO_JACK_TIMEBASE_CONTROL.
	       */
	      USE_JACK_TIMEBASE_CONTROL = 0,
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
	       * Specifies that Hydrogen should not be in control of JACK Timebase
	       * information. This could mean both that there is an external
	       * application controlling position and tempo of Hydrogen and that
	       * there are just equal JACK clients.
	       *
	       * This represent the state desired by the user. The actual one is
	       * stored in H2Core::JackAudioDriver::m_timebaseState.
	       *
	       * Its counterpart is #USE_JACK_TIMEBASE_CONTROL.
	       */
	      NO_JACK_TIMEBASE_CONTROL = 1,
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
		OscTab = 0x040
	};

	bool				__playsamplesonclicking; // audio file browser

	bool				__playselectedinstrument; // midi keys and keys play instrument or drumset

	bool				m_bFollowPlayhead;

	// switch to enable / disable lash, only on h2 startup
	bool				m_brestartLash;
	bool				m_bsetLash;

	//soundlibrarypanel expand song and pattern item
	bool				__expandSongItem;
	bool				__expandPatternItem;

	//beatcounter
	bool				m_bbc;
	bool				m_mmcsetplay;

	int					m_countOffset;
	int					m_startOffset;
	// ~ beatcounter

	std::list<QString> 		sServerList;
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

	/**
	 * External applications with a faulty JACK Timebase implementation can mess
	 * up the transport within Hydrogen. To guarantee the basic functionality,
	 * the user can disable Timebase support and make Hydrogen only listen to
	 * the frame number broadcast by the JACK server.
	 */
	bool				m_bJackTimebaseEnabled;
	/**
	 * Specifies if Hydrogen support the of JACK Timebase protocol. It has two
	 * states: Preferences::USE_JACK_TIMEBASE_CONTROL and Preferences::NO_JACK_TIMEBASE_CONTROL.
	 * It is set to Preferences::NO_JACK_TIMEBASE_CONTROL by the
	 * JackAudioDriver::initTimebaseControl() if Hydrogen couldn't acquire
	 * Timebase control.
	 */
	int					m_bJackTimebaseMode;
	// ~ jack driver properties

	///Default text editor (used by Playlisteditor)
	QString				m_sDefaultEditor;
	int				m_nAutosavesPerHour;

	///Rubberband CLI
	QString				m_rubberBandCLIexecutable;

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
	static Preferences* 	get_instance(){ assert(__instance); return __instance; }

	~Preferences();

	/// Load the preferences file
	bool			loadPreferences( bool bGlobal );

	/// Save the preferences file
	bool			savePreferences();

	const QString&	getDataDirectory();

	const QString&	getDefaultEditor();
	void			setDefaultEditor( QString editor);

	InterfaceTheme::Layout	getDefaultUILayout();
	void			setDefaultUILayout( InterfaceTheme::Layout layout);

	InterfaceTheme::ScalingPolicy getUIScalingPolicy();
	void			setUIScalingPolicy( InterfaceTheme::ScalingPolicy policy );
	InterfaceTheme::IconColor getIconColor();
	void			setIconColor( InterfaceTheme::IconColor iconColor );

	// General
	const QString&	getPreferredLanguage();
	void			setPreferredLanguage( const QString& sLanguage );

	void			setRestoreLastSongEnabled( bool restore );
	void			setRestoreLastPlaylistEnabled( bool restore );
	void			setUseRelativeFilenamesForPlaylists( bool value );

	void			setShowDevelWarning( bool value );
	bool			getShowDevelWarning();

	bool			getShowNoteOverwriteWarning();
	void			setShowNoteOverwriteWarning( bool bValue );

	bool			isRestoreLastSongEnabled();
	bool			isRestoreLastPlaylistEnabled();
	bool			isPlaylistUsingRelativeFilenames();

	void			setLastSongFilename( const QString& filename );
	const QString&	getLastSongFilename();

	void			setLastPlaylistFilename( const QString& filename );
	const QString&	getLastPlaylistFilename();

	void			setHearNewNotes( bool value );
	bool			getHearNewNotes();

	void			setRecordEvents( bool value );
	bool			getRecordEvents();

	void			setPunchInPos ( unsigned pos );
	int				getPunchInPos();

	void			setPunchOutPos ( unsigned pos );
	int				getPunchOutPos();

	bool			inPunchArea (int pos);
	void			unsetPunchArea ();

	void			setQuantizeEvents( bool value );
	bool			getQuantizeEvents();

	std::vector<QString> 		getRecentFiles() const;
	void			setRecentFiles( const std::vector<QString> recentFiles );

	QStringList		getRecentFX();
	void			setMostRecentFX( QString );


	// GUI Properties
	const QString&	getQTStyle();
	void			setQTStyle( const QString& sStyle );

	const QString&	getApplicationFontFamily() const;
	void			setApplicationFontFamily( const QString& family );
	const QString&	getLevel2FontFamily() const;
	void			setLevel2FontFamily( const QString& family );
	const QString&	getLevel3FontFamily() const;
	void			setLevel3FontFamily( const QString& family );

	FontTheme::FontSize		getFontSize() const;
	void			setFontSize( FontTheme::FontSize fontSize );

	float			getMixerFalloffSpeed();
	void			setMixerFalloffSpeed( float value );
	bool			showInstrumentPeaks();
	void			setInstrumentPeaks( bool value );

	int				getPatternEditorGridResolution();
	void			setPatternEditorGridResolution( int value );

	bool			isPatternEditorUsingTriplets();
	void			setPatternEditorUsingTriplets( bool value );

	bool			isFXTabVisible();
	void			setFXTabVisible( bool value );
	
	bool			getShowAutomationArea();
	void			setShowAutomationArea( bool value );

	unsigned		getPatternEditorGridHeight();
	void			setPatternEditorGridHeight( unsigned value );

	unsigned		getPatternEditorGridWidth();
	void			setPatternEditorGridWidth( unsigned value );

	unsigned		getSongEditorGridHeight();
	void			setSongEditorGridHeight( unsigned value );

	unsigned		getSongEditorGridWidth();
	void			setSongEditorGridWidth( unsigned value );

	void			setColoringMethod( InterfaceTheme::ColoringMethod coloringMethod );
	InterfaceTheme::ColoringMethod	getColoringMethod() const;

	void			setPatternColors( std::vector<QColor> patternColors );
	std::vector<QColor> getPatternColors() const;
	void			setMaxPatternColors( int nValue );
	int				getMaxPatternColors() const;
	void			setVisiblePatternColors( int nValue );
	int				getVisiblePatternColors() const;

	WindowProperties	getMainFormProperties();
	void				setMainFormProperties( const WindowProperties& prop );

	WindowProperties	getMixerProperties();
	void				setMixerProperties( const WindowProperties& prop );

	WindowProperties	getPatternEditorProperties();
	void				setPatternEditorProperties( const WindowProperties& prop );

	WindowProperties	getSongEditorProperties();
	void				setSongEditorProperties( const WindowProperties& prop );

	WindowProperties	getInstrumentRackProperties();
	void				setInstrumentRackProperties( const WindowProperties& prop );

	WindowProperties	getAudioEngineInfoProperties();
	void				setAudioEngineInfoProperties( const WindowProperties& prop );

	WindowProperties	getLadspaProperties( unsigned nFX );
	void			setLadspaProperties( unsigned nFX, const WindowProperties& prop );

	WindowProperties	getPlaylistDialogProperties();
	void				setPlaylistDialogProperties( const WindowProperties& prop );

	WindowProperties	getDirectorProperties();
	void				setDirectorProperties( const WindowProperties& prop );

	const std::shared_ptr<ColorTheme>	getColorTheme() const;
	void			setColorTheme( const std::shared_ptr<ColorTheme> pNewColorTheme );

	bool			useLash();
	void			setUseLash( bool b );

	bool			hideKeyboardCursor();
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
	bool			getWaitForSessionHandler();

#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_
	void			setNsmClientId(const QString& nsmClientId);
	QString			getNsmClientId(void);

	void			setNsmSongName(const QString& nsmSongName);
	QString			getNsmSongName(void);
#endif

	/** \return #m_bOscServerEnabled*/
	bool			getOscServerEnabled();
	/** \param val Sets #m_bOscServerEnabled*/
	void			setOscServerEnabled( bool val );
	/** \return #m_bOscFeedbackEnabled*/
	bool			getOscFeedbackEnabled();
	/** \param val Sets #m_bOscFeedbackEnabled*/
	void			setOscFeedbackEnabled( bool val );
	/** \return #m_nOscServerPort*/
	int				getOscServerPort();
	/** \param oscPort Sets #m_nOscServerPort*/
	void			setOscServerPort( int oscPort );

	/** Whether to use the bpm of the timeline.
	 * \return #__useTimelineBpm */
	bool			getUseTimelineBpm();
	/** Setting #__useTimelineBpm.
	 * \param val New choice. */
	void			setUseTimelineBpm( bool val );
	
	void			setShowPlaybackTrack( bool val);
	bool			getShowPlaybackTrack() const;

	int				getRubberBandBatchMode();
	void			setRubberBandBatchMode( int val );

	int				getLastOpenTab();
	void			setLastOpenTab(int n);

	void			setH2ProcessName(const QString& processName);

	QString			getH2ProcessName();

	QString			getLastExportPatternAsDirectory() const;
	QString			getLastExportSongDirectory() const;
	QString			getLastSaveSongAsDirectory() const;
	QString			getLastOpenSongDirectory() const;
	QString			getLastOpenPatternDirectory() const;
	QString			getLastExportLilypondDirectory() const;
	QString			getLastExportMidiDirectory() const;
	QString			getLastImportDrumkitDirectory() const;
	QString			getLastExportDrumkitDirectory() const;
	QString			getLastOpenLayerDirectory() const;
	QString			getLastOpenPlaybackTrackDirectory() const;
	QString			getLastAddSongToPlaylistDirectory() const;
	QString			getLastPlaylistDirectory() const;
	QString			getLastPlaylistScriptDirectory() const;
	QString			getLastImportThemeDirectory() const;
	QString			getLastExportThemeDirectory() const;
	void			setLastExportPatternAsDirectory( QString sPath );
	void			setLastExportSongDirectory( QString sPath );
	void			setLastSaveSongAsDirectory( QString sPath );
	void			setLastOpenSongDirectory( QString sPath );
	void			setLastOpenPatternDirectory( QString sPath );
	void			setLastExportLilypondDirectory( QString sPath );
	void			setLastExportMidiDirectory( QString sPath );
	void			setLastImportDrumkitDirectory( QString sPath );
	void			setLastExportDrumkitDirectory( QString sPath );
	void			setLastOpenLayerDirectory( QString sPath );
	void			setLastOpenPlaybackTrackDirectory( QString sPath );
	void			setLastAddSongToPlaylistDirectory( QString sPath );
	void			setLastPlaylistDirectory( QString sPath );
	void			setLastPlaylistScriptDirectory( QString sPath );
	void			setLastImportThemeDirectory( QString sPath );
	void			setLastExportThemeDirectory( QString sPath );

	int				getExportSampleDepthIdx() const;
	void			setExportSampleDepthIdx( int nExportSampleDepthIdx );
	
	int				getExportSampleRateIdx() const;
	void			setExportSampleRateIdx( int nExportSampleRateIdx );

	int				getExportModeIdx() const;
	void			setExportModeIdx(int nExportMode);
	
	Filesystem::AudioFormat getExportFormat() const;
	void			setExportFormat( Filesystem::AudioFormat format );
	float 			getExportCompressionLevel() const;
	void			setExportCompressionLevel( float fCompressionLevel );

    int				getMidiExportMode() const;
    void			setMidiExportMode(int nExportMode);

	bool			m_bShowExportSongLicenseWarning;
	bool			m_bShowExportDrumkitLicenseWarning;
	bool			m_bShowExportDrumkitCopyleftWarning;
	bool			m_bShowExportDrumkitAttributionWarning;

	/** Returns #m_sPreferencesOverwritePath
	 * \return #m_sPreferencesOverwritePath */
	QString			getPreferencesOverwritePath();
	/** Setting #m_sPreferencesOverwritePath.
	 * \param newPath Path to a local preferences file.*/
	void			setPreferencesOverwritePath( const QString& newPath );

	const std::shared_ptr<Theme> getTheme() const;
	void setTheme( const std::shared_ptr<Theme> pTheme );

	bool getLoadingSuccessful() const;
	
private:
	/**
	 * Object holding the current Preferences singleton. It is
	 * initialized with NULL, set with create_instance(), and
	 * accessed with get_instance().
	 */
	static Preferences *		__instance;

	std::shared_ptr<Theme>		m_pTheme;
	
	//___ General properties ___
	QString				m_sH2ProcessName; //Name of hydrogen's main process
	 ///rubberband bpm change queue
	bool				m_useTheRubberbandBpmChangeEvent;

	///< Restore last song?
	bool				m_brestoreLastSong;
	bool				m_brestoreLastPlaylist;
	bool				m_bUseLash;
	///< Show development version warning?
	bool				m_bShowDevelWarning;
	bool				m_bShowNoteOverwriteWarning;
	///< Last song used
	QString				m_lastSongFilename;
	QString				m_lastPlaylistFilename;

	bool				quantizeEvents;
	bool				recordEvents;
	bool				readPrefFileforotherplaces;
	int					punchInPos;
	int					punchOutPos;
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
	bool				hearNewNotes;

	QStringList			m_recentFX;
	std::vector<QString> 		m_recentFiles;

#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_
		QString			m_sNsmClientId;
		QString			m_sNsmSongName;
#endif

	bool					waitingForSessionHandler;
	/**
	 * Whether to use local speeds specified along the Timeline or
	 * a constant tempo for the whole Song in
	 * Hydrogen::getTimelineBpm() and Hydrogen::getTimelineBpm().
	 *
	 * It is set using setUseTimelineBpm() and accessed via
	 * getUseTimelineBpm().
	 */
	bool					__useTimelineBpm;


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
	WindowProperties		mainFormProperties;
	WindowProperties		mixerProperties;
	WindowProperties		patternEditorProperties;
	WindowProperties		songEditorProperties;
	WindowProperties		instrumentRackProperties;
	WindowProperties		audioEngineInfoProperties;
	WindowProperties		m_ladspaProperties[MAX_FX];
	WindowProperties		m_playlistDialogProperties;
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
	Filesystem::AudioFormat m_exportFormat;
	float					m_fExportCompressionLevel;
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

	WindowProperties readWindowProperties( XMLNode parent, const QString& windowName, WindowProperties defaultProp );
	void writeWindowProperties( XMLNode parent, const QString& windowName, const WindowProperties& prop );

	bool m_bLoadingSuccessful;
};

inline QString			Preferences::getLastExportPatternAsDirectory() const {
	return m_sLastExportPatternAsDirectory;
}
inline QString			Preferences::getLastExportSongDirectory() const {
	return m_sLastExportSongDirectory;
}
inline QString			Preferences::getLastSaveSongAsDirectory() const {
	return m_sLastSaveSongAsDirectory;
}
inline QString			Preferences::getLastOpenSongDirectory() const {
	return m_sLastOpenSongDirectory;
}
inline QString			Preferences::getLastOpenPatternDirectory() const {
	return m_sLastOpenPatternDirectory;
}
inline QString			Preferences::getLastExportLilypondDirectory() const {
	return m_sLastExportLilypondDirectory;
}
inline QString			Preferences::getLastExportMidiDirectory() const {
	return m_sLastExportMidiDirectory;
}
inline QString			Preferences::getLastImportDrumkitDirectory() const {
	return m_sLastImportDrumkitDirectory;
}
inline QString			Preferences::getLastExportDrumkitDirectory() const {
	return m_sLastExportDrumkitDirectory;
}
inline QString			Preferences::getLastOpenLayerDirectory() const {
	return m_sLastOpenLayerDirectory;
}
inline QString			Preferences::getLastOpenPlaybackTrackDirectory() const {
	return m_sLastOpenPlaybackTrackDirectory;
}
inline QString			Preferences::getLastAddSongToPlaylistDirectory() const {
	return m_sLastAddSongToPlaylistDirectory;
}
inline QString			Preferences::getLastPlaylistDirectory() const {
	return m_sLastPlaylistDirectory;
}
inline QString			Preferences::getLastPlaylistScriptDirectory() const {
	return m_sLastPlaylistScriptDirectory;
}
inline QString			Preferences::getLastImportThemeDirectory() const {
	return m_sLastImportThemeDirectory;
}
inline QString			Preferences::getLastExportThemeDirectory() const {
	return m_sLastExportThemeDirectory;
}
inline void Preferences::setLastExportPatternAsDirectory( QString sPath )
{
	m_sLastExportPatternAsDirectory = sPath;
}
inline void Preferences::setLastExportSongDirectory( QString sPath )
{
	m_sLastExportSongDirectory = sPath;
}
inline void Preferences::setLastSaveSongAsDirectory( QString sPath )
{
	m_sLastSaveSongAsDirectory = sPath;
}
inline void Preferences::setLastOpenSongDirectory( QString sPath )
{
	m_sLastOpenSongDirectory = sPath;
}
inline void Preferences::setLastOpenPatternDirectory( QString sPath )
{
	m_sLastOpenPatternDirectory = sPath;
}
inline void Preferences::setLastExportLilypondDirectory( QString sPath )
{
	m_sLastExportLilypondDirectory = sPath;
}
inline void Preferences::setLastExportMidiDirectory( QString sPath )
{
	m_sLastExportMidiDirectory = sPath;
}
inline void Preferences::setLastImportDrumkitDirectory( QString sPath )
{
	m_sLastImportDrumkitDirectory = sPath;
}
inline void Preferences::setLastExportDrumkitDirectory( QString sPath )
{
	m_sLastExportDrumkitDirectory = sPath;
}
inline void Preferences::setLastOpenLayerDirectory( QString sPath )
{
	m_sLastOpenLayerDirectory = sPath;
}
inline void Preferences::setLastOpenPlaybackTrackDirectory( QString sPath )
{
	m_sLastOpenPlaybackTrackDirectory = sPath;
}
inline void Preferences::setLastAddSongToPlaylistDirectory( QString sPath )
{
	m_sLastAddSongToPlaylistDirectory = sPath;
}
inline void Preferences::setLastPlaylistDirectory( QString sPath )
{
	m_sLastPlaylistDirectory = sPath;
}
inline void Preferences::setLastPlaylistScriptDirectory( QString sPath )
{
	m_sLastPlaylistScriptDirectory = sPath;
}
inline void Preferences::setLastImportThemeDirectory( QString sPath )
{
	m_sLastImportThemeDirectory = sPath;
}
inline void Preferences::setLastExportThemeDirectory( QString sPath )
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

inline const QString& Preferences::getDefaultEditor() {
	return m_sDefaultEditor;
}

inline void Preferences::setDefaultEditor( QString editor){
	m_sDefaultEditor = editor;
}

inline InterfaceTheme::Layout Preferences::getDefaultUILayout(){
	return m_pTheme->getInterfaceTheme()->m_layout;
}

inline void Preferences::setDefaultUILayout( InterfaceTheme::Layout layout){
	m_pTheme->getInterfaceTheme()->m_layout = layout;
}

inline InterfaceTheme::ScalingPolicy Preferences::getUIScalingPolicy(){
	return m_pTheme->getInterfaceTheme()->m_scalingPolicy;
}

inline void Preferences::setUIScalingPolicy( InterfaceTheme::ScalingPolicy scalingPolicy){
	m_pTheme->getInterfaceTheme()->m_scalingPolicy = scalingPolicy;
}

inline InterfaceTheme::IconColor Preferences::getIconColor(){
	return m_pTheme->getInterfaceTheme()->m_iconColor;
}

inline void Preferences::setIconColor( InterfaceTheme::IconColor iconColor){
	m_pTheme->getInterfaceTheme()->m_iconColor = iconColor;
}

// General
inline const QString& Preferences::getPreferredLanguage() {
	return m_sPreferredLanguage;
}

inline void Preferences::setPreferredLanguage( const QString& sLanguage ) {
	m_sPreferredLanguage = sLanguage;
}

inline void Preferences::setRestoreLastSongEnabled( bool restore ) {
	m_brestoreLastSong = restore;
}

inline void Preferences::setRestoreLastPlaylistEnabled( bool restore ) {
	m_brestoreLastPlaylist = restore;
}

inline void Preferences::setUseRelativeFilenamesForPlaylists( bool value ) {
	m_bUseRelativeFilenamesForPlaylists= value;
}

inline void Preferences::setShowDevelWarning( bool value ) {
	m_bShowDevelWarning = value;
}

inline bool Preferences::getShowDevelWarning() {
	return m_bShowDevelWarning;
}

inline bool Preferences::getShowNoteOverwriteWarning() {
	return m_bShowNoteOverwriteWarning;
}

inline void Preferences::setShowNoteOverwriteWarning( bool bValue ) {
	m_bShowNoteOverwriteWarning = bValue;
}

inline void Preferences::setHideKeyboardCursor( bool value ) {
	m_bHideKeyboardCursor = value;
}

inline bool Preferences::hideKeyboardCursor() {
	return m_bHideKeyboardCursor;
}

inline bool Preferences::isRestoreLastSongEnabled() {
	return m_brestoreLastSong;
}

inline bool Preferences::isRestoreLastPlaylistEnabled() {
	return m_brestoreLastPlaylist;
}

inline bool Preferences::isPlaylistUsingRelativeFilenames() {
	return m_bUseRelativeFilenamesForPlaylists;
}

inline void Preferences::setLastSongFilename( const QString& filename ) {
	m_lastSongFilename = filename;
}
inline const QString& Preferences::getLastSongFilename() {
	return m_lastSongFilename;
}

inline void Preferences::setLastPlaylistFilename( const QString& filename ) {
	m_lastPlaylistFilename = filename;
}
inline const QString& Preferences::getLastPlaylistFilename() {
	return m_lastPlaylistFilename;
}

inline void Preferences::setHearNewNotes( bool value ) {
	hearNewNotes = value;
}
inline bool Preferences::getHearNewNotes() {
	return hearNewNotes;
}

inline void Preferences::setRecordEvents( bool value ) {
	recordEvents = value;
}
inline bool Preferences::getRecordEvents() {
	return recordEvents;
}

inline void Preferences::setPunchInPos ( unsigned pos ) {
	punchInPos = pos;
}
inline int Preferences::getPunchInPos() {
	return punchInPos;
}

inline void Preferences::setPunchOutPos ( unsigned pos ) {
	punchOutPos = pos;
}
inline int Preferences::getPunchOutPos() {
	return punchOutPos;
}

inline bool Preferences::inPunchArea (int pos) {
	// Return true if punch area not defined
	if ( punchInPos <= punchOutPos ) {
		if ( pos < punchInPos || punchOutPos < pos ) {
			return false;
		}
	}
	return true;
}

inline void Preferences::unsetPunchArea () {
	punchInPos = 0;
	punchOutPos = -1;
}

inline void Preferences::setQuantizeEvents( bool value ) {
	quantizeEvents = value;
}
inline bool Preferences::getQuantizeEvents() {
	return quantizeEvents;
}

inline void Preferences::setRecentFiles( const std::vector<QString> recentFiles ) {
	m_recentFiles = recentFiles;
}
inline std::vector<QString> Preferences::getRecentFiles() const {
	return m_recentFiles;
}

inline QStringList Preferences::getRecentFX() {
	return m_recentFX;
}


// GUI Properties
inline const QString& Preferences::getQTStyle() {
	return m_pTheme->getInterfaceTheme()->m_sQTStyle;
}
inline void Preferences::setQTStyle( const QString& sStyle ) {
	m_pTheme->getInterfaceTheme()->m_sQTStyle = sStyle;
}
inline const QString& Preferences::getApplicationFontFamily() const {
	return m_pTheme->getFontTheme()->m_sApplicationFontFamily;
}
inline void Preferences::setApplicationFontFamily( const QString& family ) {
	m_pTheme->getFontTheme()->m_sApplicationFontFamily = family;
}

inline const QString& Preferences::getLevel2FontFamily() const {
	return m_pTheme->getFontTheme()->m_sLevel2FontFamily;
}
inline void Preferences::setLevel2FontFamily( const QString& family ) {
	m_pTheme->getFontTheme()->m_sLevel2FontFamily = family;
}

inline const QString& Preferences::getLevel3FontFamily() const {
	return m_pTheme->getFontTheme()->m_sLevel3FontFamily;
}
inline void Preferences::setLevel3FontFamily( const QString& family ) {
	m_pTheme->getFontTheme()->m_sLevel3FontFamily = family;
}

inline FontTheme::FontSize Preferences::getFontSize() const {
	return m_pTheme->getFontTheme()->m_fontSize;
}
inline void Preferences::setFontSize( FontTheme::FontSize fontSize ) {
	m_pTheme->getFontTheme()->m_fontSize = fontSize;
}

inline float Preferences::getMixerFalloffSpeed() {
	return m_pTheme->getInterfaceTheme()->m_fMixerFalloffSpeed;
}
inline void Preferences::setMixerFalloffSpeed( float value ) {
	m_pTheme->getInterfaceTheme()->m_fMixerFalloffSpeed = value;
}
inline bool Preferences::showInstrumentPeaks() {
	return m_bShowInstrumentPeaks;
}
inline void Preferences::setInstrumentPeaks( bool value ) {
	m_bShowInstrumentPeaks = value;
}

inline int Preferences::getPatternEditorGridResolution() {
	return m_nPatternEditorGridResolution;
}
inline void Preferences::setPatternEditorGridResolution( int value ) {
	m_nPatternEditorGridResolution = value;
}

inline bool Preferences::isPatternEditorUsingTriplets() {
	return m_bPatternEditorUsingTriplets;
}
inline void Preferences::setPatternEditorUsingTriplets( bool value ) {
	m_bPatternEditorUsingTriplets = value;
}

inline bool Preferences::isFXTabVisible() {
	return m_bIsFXTabVisible;
}
inline void Preferences::setFXTabVisible( bool value ) {
	m_bIsFXTabVisible = value;
}

inline bool Preferences::getShowAutomationArea() {
	return m_bShowAutomationArea;
}
inline void Preferences::setShowAutomationArea( bool value ) {
	m_bShowAutomationArea = value;
}


inline unsigned Preferences::getSongEditorGridHeight() {
	return m_nSongEditorGridHeight;
}
inline void Preferences::setSongEditorGridHeight( unsigned value ) {
	m_nSongEditorGridHeight = value;
}
inline unsigned Preferences::getSongEditorGridWidth() {
	return m_nSongEditorGridWidth;
}
inline void Preferences::setSongEditorGridWidth( unsigned value ) {
	m_nSongEditorGridWidth = value;
}

inline unsigned Preferences::getPatternEditorGridHeight() {
	return m_nPatternEditorGridHeight;
}
inline void Preferences::setPatternEditorGridHeight( unsigned value ) {
	m_nPatternEditorGridHeight = value;
}
inline unsigned Preferences::getPatternEditorGridWidth() {
	return m_nPatternEditorGridWidth;
}
inline void Preferences::setPatternEditorGridWidth( unsigned value ) {
	m_nPatternEditorGridWidth = value;
}

inline void	Preferences::setPatternColors( std::vector<QColor> patternColors ) {
	m_pTheme->getInterfaceTheme()->m_patternColors = patternColors;
}
inline std::vector<QColor> Preferences::getPatternColors() const {
	return m_pTheme->getInterfaceTheme()->m_patternColors;
}
inline void	Preferences::setVisiblePatternColors( int nValue ) {
	m_pTheme->getInterfaceTheme()->m_nVisiblePatternColors = nValue;
}
inline int Preferences::getVisiblePatternColors() const {
	return m_pTheme->getInterfaceTheme()->m_nVisiblePatternColors;
}
inline void	Preferences::setMaxPatternColors( int nValue ) {
	m_pTheme->getInterfaceTheme()->m_nMaxPatternColors = nValue;
}
inline int Preferences::getMaxPatternColors() const {
	return m_pTheme->getInterfaceTheme()->m_nMaxPatternColors;
}

inline void Preferences::setColoringMethod( InterfaceTheme::ColoringMethod coloringMethod ){
	m_pTheme->getInterfaceTheme()->m_coloringMethod = coloringMethod;
}

inline InterfaceTheme::ColoringMethod Preferences::getColoringMethod() const {
	return m_pTheme->getInterfaceTheme()->m_coloringMethod;
}

inline WindowProperties Preferences::getMainFormProperties() {
	return mainFormProperties;
}
inline void Preferences::setMainFormProperties( const WindowProperties& prop ) {
	mainFormProperties = prop;
}

inline WindowProperties Preferences::getMixerProperties() {
	return mixerProperties;
}
inline void Preferences::setMixerProperties( const WindowProperties& prop ) {
	mixerProperties = prop;
}

inline WindowProperties Preferences::getPatternEditorProperties() {
	return patternEditorProperties;
}
inline void Preferences::setPatternEditorProperties( const WindowProperties& prop ) {
	patternEditorProperties = prop;
}

inline WindowProperties Preferences::getSongEditorProperties() {
	return songEditorProperties;
}
inline void Preferences::setSongEditorProperties( const WindowProperties& prop ) {
	songEditorProperties = prop;
}


inline WindowProperties Preferences::getInstrumentRackProperties() {
	return instrumentRackProperties;
}
inline void Preferences::setInstrumentRackProperties( const WindowProperties& prop ) {
	instrumentRackProperties = prop;
}
 
inline WindowProperties Preferences::getAudioEngineInfoProperties() {
	return audioEngineInfoProperties;
}
inline void Preferences::setAudioEngineInfoProperties( const WindowProperties& prop ) {
	audioEngineInfoProperties = prop;
}

inline WindowProperties Preferences::getLadspaProperties( unsigned nFX ) {
	return m_ladspaProperties[nFX];
}
inline void Preferences::setLadspaProperties( unsigned nFX, const WindowProperties& prop ) {
	m_ladspaProperties[nFX] = prop;
}

inline WindowProperties Preferences::getPlaylistDialogProperties() {
	return m_playlistDialogProperties;
}
inline void Preferences::setPlaylistDialogProperties( const WindowProperties& prop ) {
	m_playlistDialogProperties = prop;
}

inline WindowProperties Preferences::getDirectorProperties() {
	return m_directorProperties;
}
inline void Preferences::setDirectorProperties( const WindowProperties& prop ) {
	m_directorProperties = prop;
}

inline const std::shared_ptr<ColorTheme> Preferences::getColorTheme() const {
	return m_pTheme->getColorTheme();
}
inline void Preferences::setColorTheme( const std::shared_ptr<ColorTheme> pNewColorTheme ) {
	m_pTheme->setColorTheme( pNewColorTheme );
}

inline bool Preferences::useLash(){
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
	waitingForSessionHandler = value;
}

inline bool Preferences::getWaitForSessionHandler(){
		return waitingForSessionHandler;
}

#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_
inline void Preferences::setNsmClientId(const QString& nsmClientId){
	m_sNsmClientId = nsmClientId;
}

inline QString Preferences::getNsmClientId(void){
	return m_sNsmClientId;
}

inline void Preferences::setNsmSongName(const QString& nsmSongName){
	m_sNsmSongName = nsmSongName;
}

inline QString Preferences::getNsmSongName(void){
	return m_sNsmSongName;
}

#endif

inline bool Preferences::getOscServerEnabled(){
	return m_bOscServerEnabled;
}
inline void Preferences::setOscServerEnabled( bool val ){
	m_bOscServerEnabled = val;
}

inline bool Preferences::getOscFeedbackEnabled(){
	return m_bOscFeedbackEnabled;
}
inline void Preferences::setOscFeedbackEnabled( bool val ){
	m_bOscFeedbackEnabled = val;
}

inline int Preferences::getOscServerPort(){
	return m_nOscServerPort;
}
inline void Preferences::setOscServerPort( int oscPort ){
	m_nOscServerPort = oscPort;
}

inline bool Preferences::getUseTimelineBpm(){
	return __useTimelineBpm;
}
inline void Preferences::setUseTimelineBpm( bool val ){
	__useTimelineBpm = val;
}

inline void Preferences::setShowPlaybackTrack( bool val ) {
	m_bShowPlaybackTrack = val; 
}
inline bool Preferences::getShowPlaybackTrack() const {
	return m_bShowPlaybackTrack;
}

inline int Preferences::getRubberBandBatchMode(){
	return m_useTheRubberbandBpmChangeEvent;
}
inline void Preferences::setRubberBandBatchMode( int val ){
	m_useTheRubberbandBpmChangeEvent = val;
}

inline int Preferences::getLastOpenTab(){
	return m_nLastOpenTab;
}
inline void Preferences::setLastOpenTab(int n){
	m_nLastOpenTab = n;
}

inline void Preferences::setH2ProcessName(const QString& processName){
	m_sH2ProcessName = processName;
}

inline QString Preferences::getH2ProcessName() {
	return m_sH2ProcessName;
}
inline void Preferences::setTheme( const std::shared_ptr<Theme> pTheme ) {
	m_pTheme->setTheme( pTheme );
}
inline const std::shared_ptr<Theme> Preferences::getTheme() const {
	return m_pTheme;
}
inline bool Preferences::getLoadingSuccessful() const {
	return m_bLoadingSuccessful;
}
};

#endif

