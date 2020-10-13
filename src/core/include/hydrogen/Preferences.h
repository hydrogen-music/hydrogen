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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <list>
#include <vector>
#include <cassert>

#include <hydrogen/midi_action.h>
#include <hydrogen/globals.h>
#include <hydrogen/object.h>

#include <QStringList>
#include <QDomDocument>

namespace H2Core
{

const float FALLOFF_SLOW = 	1.08f;
const float FALLOFF_NORMAL=	1.1f;
const float FALLOFF_FAST =	1.5f;


/**
\ingroup H2CORE
*/
class WindowProperties : public H2Core::Object
{
	H2_OBJECT
public:
	int x;
	int y;
	int width;
	int height;
	bool visible;

	WindowProperties();
	~WindowProperties();

	void set(int _x, int _y, int _width, int _height, bool _visible) {
		x = _x; y = _y;
		width = _width; height = _height;
		visible = _visible;
	}

};


/**
\ingroup H2CORE
*/
class H2RGBColor : public H2Core::Object
{
	H2_OBJECT
public:
	H2RGBColor( int r = -1, int g = -1, int b = -1 );
	H2RGBColor( const QString& sColor );
	~H2RGBColor();

	QString toStringFmt();

	int getRed() const {
		return m_red;
	}
	int getGreen() const {
		return m_green;
	}
	int getBlue() const {
		return m_blue;
	}

private:
	int m_red;
	int m_green;
	int m_blue;

};


/**
\ingroup H2CORE
\brief	Colors for hydrogen
*/
class UIStyle : public H2Core::Object
{
	H2_OBJECT
public:
	UIStyle();
	H2RGBColor m_songEditor_backgroundColor;
	H2RGBColor m_songEditor_alternateRowColor;
	H2RGBColor m_songEditor_selectedRowColor;
	H2RGBColor m_songEditor_lineColor;
	H2RGBColor m_songEditor_textColor;
	H2RGBColor m_songEditor_pattern1Color;

	H2RGBColor m_patternEditor_backgroundColor;
	H2RGBColor m_patternEditor_alternateRowColor;
	H2RGBColor m_patternEditor_selectedRowColor;
	H2RGBColor m_patternEditor_textColor;
	H2RGBColor m_patternEditor_noteColor;
	H2RGBColor m_patternEditor_noteoffColor;
	H2RGBColor m_patternEditor_lineColor;
	H2RGBColor m_patternEditor_line1Color;
	H2RGBColor m_patternEditor_line2Color;
	H2RGBColor m_patternEditor_line3Color;
	H2RGBColor m_patternEditor_line4Color;
	H2RGBColor m_patternEditor_line5Color;

	H2RGBColor m_selectionHighlightColor;
	H2RGBColor m_selectionInactiveColor;
};



/**
\ingroup H2CORE
\brief	Manager for User Preferences File (singleton)
*/
class Preferences : public H2Core::Object
{
	H2_OBJECT
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
	      POST_FADER = 0,
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
	      PRE_FADER = 1,
	      SET_PLAY_OFF = 1,
	      BC_OFF = 1
	};


	enum UI_LAYOUT_TYPES {
			UI_LAYOUT_SINGLE_PANE,
			UI_LAYOUT_TABBED
	};

	enum UI_SCALING_POLICY {
		UI_SCALING_SMALLER,
		UI_SCALING_SYSTEM,
		UI_SCALING_LARGER
	};

	QString				__lastspatternDirectory;
	QString				__lastsampleDirectory; // audio file browser
	bool				__playsamplesonclicking; // audio file browser

	bool				__playselectedinstrument; // midi keys and keys play instrument or drumset

	int					m_nRecPreDelete; //index of record note pre delete function 0 = off
	int					m_nRecPostDelete;

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
	//~ beatcounter

	std::list<QString> 		sServerList;
	std::list<QString> 		m_patternCategories;

	//	audio engine properties ___
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
	QString				m_sAudioDriver;
	/** If set to true, samples of the metronome will be added to
	 * #m_songNoteQueue in audioEngine_updateNoteQueue() and thus
	 * played back on a regular basis.*/
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
	 * Port number the OscServer will be started at.
	 
	 * Set by setOscServerPort() and queried by
	 * getOscServerPort().
	 */
	int					m_nOscServerPort;

	//	alsa audio driver properties ___
	QString				m_sAlsaAudioDevice;

	//	jack driver properties ___
	QString				m_sJackPortName1;
	QString				m_sJackPortName2;
	/**
	 * Specifies whether or not Hydrogen will use the JACK
	 * transport system. It has two different states:
	 * #USE_JACK_TRANSPORT and #NO_JACK_TRANSPORT.
	 */
	int					m_bJackTransportMode;
	bool				m_bJackConnectDefaults;
	/** 
	 * If set to _true_, JackAudioDriver::makeTrackOutputs() will
	 * create two individual left and right output ports for every
	 * component of each instrument. If _false_, one usual stereo
	 * output will be created.
	 */
	bool				m_bJackTrackOuts;
	int					m_nJackTrackOutputMode;
	//jack time master
	/**
	 * Specifies if Hydrogen should run as JACK time master. It
	 * has two states: Preferences::USE_JACK_TIME_MASTER and
	 * Preferences::NO_JACK_TIME_MASTER. It is set to
	 * Preferences::NO_JACK_TIME_MASTER by the
	 * JackAudioDriver::initTimebaseMaster() if Hydrogen couldn't be
	 * registered as time master.
	 */
	int				m_bJackMasterMode;
	//~ jack driver properties

	///Default text editor (used by Playlisteditor)
	QString				m_sDefaultEditor;

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
	void			loadPreferences( bool bGlobal );

	/// Save the preferences file
	void			savePreferences();

	const QString&	getDataDirectory();

	const QString&	getDefaultEditor();
	void			setDefaultEditor( QString editor);

	int				getDefaultUILayout();
	void			setDefaultUILayout( int layout);

	int				getUIScalingPolicy();
	void			setUIScalingPolicy( int nPolicy );

	// General
	const QString&	getPreferredLanguage();
	void			setPreferredLanguage( const QString& sLanguage );

	void			setRestoreLastSongEnabled( bool restore );
	void			setRestoreLastPlaylistEnabled( bool restore );
	void			setUseRelativeFilenamesForPlaylists( bool value );

	void			setShowDevelWarning( bool value );
	bool			getShowDevelWarning();

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

	void			setDestructiveRecord ( bool value );
	bool			getDestructiveRecord();

	void			setPunchInPos ( unsigned pos );
	int				getPunchInPos();

	void			setPunchOutPos ( unsigned pos );
	int				getPunchOutPos();

	bool			inPunchArea (int pos);
	void			unsetPunchArea ();

	void			setQuantizeEvents( bool value );
	bool			getQuantizeEvents();

	std::vector<QString> 		getRecentFiles();
	void				setRecentFiles( std::vector<QString> recentFiles );

	QStringList		getRecentFX();
	void			setMostRecentFX( QString );


	// GUI Properties
	const QString&	getQTStyle();
	void			setQTStyle( const QString& sStyle );

	const QString&	getApplicationFontFamily();
	void			setApplicationFontFamily( const QString& family );

	int				getApplicationFontPointSize();
	void			setApplicationFontPointSize( int size );

	QString			getMixerFontFamily();
	void			setMixerFontFamily( const QString& family );
	int				getMixerFontPointSize();
	void			setMixerFontPointSize( int size );
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

	void			setColoringMethodAuxValue( int value );
	int				getColoringMethodAuxValue() const;

	void			setColoringMethod( int value );
	int				getColoringMethod() const;

	WindowProperties	getMainFormProperties();
	void				setMainFormProperties( const WindowProperties& prop );

	WindowProperties	getMixerProperties();
	void				setMixerProperties( const WindowProperties& prop );

	WindowProperties	getPatternEditorProperties();
	void				setPatternEditorProperties( const WindowProperties& prop );

	WindowProperties	getSongEditorProperties();
	void				setSongEditorProperties( const WindowProperties& prop );

	WindowProperties	getAudioEngineInfoProperties();
	void				setAudioEngineInfoProperties( const WindowProperties& prop );

	WindowProperties	getLadspaProperties( unsigned nFX );
	void			setLadspaProperties( unsigned nFX, const WindowProperties& prop );

	UIStyle*		getDefaultUIStyle();

	/** \return #m_bPatternModePlaysSelected*/
	bool			patternModePlaysSelected();
	/** \param b Sets #m_bPatternModePlaysSelected*/
	void			setPatternModePlaysSelected( bool b );
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

#if defined(H2CORE_HAVE_JACKSESSION) || _DOXYGEN_
	QString			getJackSessionUUID();
	void			setJackSessionUUID( QString uuid );

	QString			getJackSessionApplicationPath();
	void			setJackSessionApplicationPath( QString path );
#endif


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

	int				getRubberBandCalcTime();
	void			setRubberBandCalcTime( int val );

	int				getRubberBandBatchMode();
	void			setRubberBandBatchMode( int val );

	int				getLastOpenTab();
	void			setLastOpenTab(int n);

	void			setH2ProcessName(const QString& processName);

	QString			getH2ProcessName();

	int				getExportSampleDepthIdx() const;
	void			setExportSampleDepthIdx( int nExportSampleDepthIdx );
	
	int				getExportSampleRateIdx() const;
	void			setExportSampleRateIdx( int nExportSampleRateIdx );

	int				getExportModeIdx() const;
	void			setExportModeIdx(int nExportMode);
	
	QString			getExportDirectory() const;
	void			setExportDirectory( const QString &sExportDirectory );
	
	int				getExportTemplateIdx() const;
	void			setExportTemplateIdx( int nExportTemplateIdx );

    int				getMidiExportMode() const;
    void			setMidiExportMode(int nExportMode);

    QString			getMidiExportDirectory() const;
    void			setMidiExportDirectory( const QString &sExportDirectory );

	/** Returns #m_sPreferencesOverwritePath
	 * \return #m_sPreferencesOverwritePath */
	QString			getPreferencesOverwritePath();
	/** Setting #m_sPreferencesOverwritePath.
	 * \param newPath Path to a local preferences file.*/
	void			setPreferencesOverwritePath( const QString& newPath );
	
private:
	/**
	 * Object holding the current Preferences singleton. It is
	 * initialized with NULL, set with create_instance(), and
	 * accessed with get_instance().
	 */
	static Preferences *		__instance;
	
	//___ General properties ___
	QString				m_sH2ProcessName; //Name of hydrogen's main process
	int					__rubberBandCalcTime;
	 ///rubberband bpm change queue
	bool				m_useTheRubberbandBpmChangeEvent;
	/**
	 * When transport is in Song::PATTERN_MODE and this variable is
	 * set to true, the currently focused Pattern will be used for
	 * playback.
	 *
	 * It is set by setPatternModePlaysSelected() and queried by
	 * patternModePlaysSelected().
	 */
	bool				m_bPatternModePlaysSelected;
	///< Restore last song?
	bool				m_brestoreLastSong;
	bool				m_brestoreLastPlaylist;
	bool				m_bUseLash;
	///< Show development version warning?
	bool				m_bShowDevelWarning;
	///< Last song used
	QString				m_lastSongFilename;
	QString				m_lastPlaylistFilename;

	bool				quantizeEvents;
	bool				recordEvents;
	bool				destructiveRecord;
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

#if defined(H2CORE_HAVE_JACKSESSION) || _DOXYGEN_
		QString			jackSessionUUID;
		QString			jackSessionApplicationPath;
#endif

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
	QString					m_sQTStyle;
	int						m_nLastOpenTab;
	int						m_nDefaultUILayout;
	int						m_nUIScalingPolicy;
	bool					m_bShowPlaybackTrack;

	QString					applicationFontFamily;
	int						applicationFontPointSize;
	QString					mixerFontFamily;
	int						mixerFontPointSize;
	float					mixerFalloffSpeed;
	int						m_nPatternEditorGridResolution;
	bool					m_bPatternEditorUsingTriplets;
	bool					m_bShowInstrumentPeaks;
	bool					m_bIsFXTabVisible;
	bool					m_bShowAutomationArea;
	bool					m_bUseRelativeFilenamesForPlaylists;
	unsigned				m_nPatternEditorGridHeight;
	unsigned				m_nPatternEditorGridWidth;
	WindowProperties		mainFormProperties;
	WindowProperties		mixerProperties;
	WindowProperties		patternEditorProperties;
	WindowProperties		songEditorProperties;
	WindowProperties		drumkitManagerProperties;
	WindowProperties		audioEngineInfoProperties;
	WindowProperties		m_ladspaProperties[MAX_FX];

	UIStyle*				m_pDefaultUIStyle;
	QString					m_sPreferredLanguage;

	//Appearance: SongEditor coloring
	int						m_nColoringMethod;
	int						m_nColoringMethodAuxValue;

	//Export dialog
	QString					m_sExportDirectory;
	int						m_nExportModeIdx;
	int						m_nExportSampleRateIdx;
	int						m_nExportSampleDepthIdx;
	int						m_nExportTemplateIdx;
	//~ Export dialog

    // Export midi dialog
    QString					m_sMidiExportDirectory;
    int						m_nMidiExportMode;
    //~ Export midi dialog
	
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

	WindowProperties readWindowProperties( QDomNode parent, const QString& windowName, WindowProperties defaultProp );
	void writeWindowProperties( QDomNode parent, const QString& windowName, const WindowProperties& prop );

	void writeUIStyle( QDomNode parent );
	void readUIStyle( QDomNode parent );
};

inline QString Preferences::getMidiExportDirectory() const
{
	return m_sMidiExportDirectory;
}

inline void Preferences::setMidiExportDirectory(const QString &ExportDirectory)
{
	m_sMidiExportDirectory = ExportDirectory;
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

inline QString Preferences::getExportDirectory() const
{
	return m_sExportDirectory;
}

inline void Preferences::setExportDirectory(const QString &ExportDirectory)
{
	m_sExportDirectory = ExportDirectory;
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

inline const QString& Preferences::getDefaultEditor() {
	return m_sDefaultEditor;
}

inline void Preferences::setDefaultEditor( QString editor){
	m_sDefaultEditor = editor;
}

inline int Preferences::getDefaultUILayout(){
	return m_nDefaultUILayout;
}

inline void Preferences::setDefaultUILayout( int layout){
	m_nDefaultUILayout = layout;
}

inline int Preferences::getUIScalingPolicy() {
	return m_nUIScalingPolicy;
}

inline void Preferences::setUIScalingPolicy( int nPolicy ) {
	m_nUIScalingPolicy = nPolicy;
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

inline void Preferences::setDestructiveRecord ( bool value ) {
	destructiveRecord = value;
}
inline bool Preferences::getDestructiveRecord() {
	return destructiveRecord;
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

inline std::vector<QString> Preferences::getRecentFiles() {
	return m_recentFiles;
}

inline QStringList Preferences::getRecentFX() {
	return m_recentFX;
}


// GUI Properties
inline const QString& Preferences::getQTStyle() {
	return m_sQTStyle;
}
inline void Preferences::setQTStyle( const QString& sStyle ) {
	m_sQTStyle = sStyle;
}


inline const QString& Preferences::getApplicationFontFamily() {
	return applicationFontFamily;
}
inline void Preferences::setApplicationFontFamily( const QString& family ) {
	applicationFontFamily = family;
}

inline int Preferences::getApplicationFontPointSize() {
	return applicationFontPointSize;
}
inline void Preferences::setApplicationFontPointSize( int size ) {
	applicationFontPointSize = size;
}

inline QString Preferences::getMixerFontFamily() {
	return mixerFontFamily;
}
inline void Preferences::setMixerFontFamily( const QString& family ) {
	mixerFontFamily = family;
}
inline int Preferences::getMixerFontPointSize() {
	return mixerFontPointSize;
}
inline void Preferences::setMixerFontPointSize( int size ) {
	mixerFontPointSize = size;
}
inline float Preferences::getMixerFalloffSpeed() {
	return mixerFalloffSpeed;
}
inline void Preferences::setMixerFalloffSpeed( float value ) {
	mixerFalloffSpeed = value;
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

inline void Preferences::setColoringMethodAuxValue( int value ){
	m_nColoringMethodAuxValue = value;
}

inline int Preferences::getColoringMethodAuxValue() const{
	return m_nColoringMethodAuxValue;
}

inline void Preferences::setColoringMethod( int value ){
	m_nColoringMethod = value;
}

inline int Preferences::getColoringMethod() const{
	return m_nColoringMethod;
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

inline UIStyle* Preferences::getDefaultUIStyle() {
	return m_pDefaultUIStyle;
}

inline bool Preferences::patternModePlaysSelected() {
	return m_bPatternModePlaysSelected;
}
inline void Preferences::setPatternModePlaysSelected( bool b ) {
	m_bPatternModePlaysSelected = b;
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

#if defined(H2CORE_HAVE_JACKSESSION) || _DOXYGEN_
inline QString Preferences::getJackSessionUUID(){
	return jackSessionUUID;
}

inline void Preferences::setJackSessionUUID( QString uuid ){
	jackSessionUUID = uuid;
}

inline QString Preferences::getJackSessionApplicationPath(){
	return jackSessionApplicationPath;
}

inline void Preferences::setJackSessionApplicationPath( QString path ){
	jackSessionApplicationPath = path;
}

#endif


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

inline int Preferences::getRubberBandCalcTime(){
	return __rubberBandCalcTime;
}
inline void Preferences::setRubberBandCalcTime( int val ){
	__rubberBandCalcTime = val;
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

inline QString Preferences::getPreferencesOverwritePath() {
	return m_sPreferencesOverwritePath;
}
inline void Preferences::setPreferencesOverwritePath( const QString& newPath ) {
	m_sPreferencesOverwritePath = newPath;
}



};

#endif

