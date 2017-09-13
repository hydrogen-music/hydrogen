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

#include <stdlib.h>
#include <hydrogen/Preferences.h>

#include <hydrogen/LocalFileMng.h>

#ifndef WIN32
#include <pwd.h>
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <list>

#include <hydrogen/midi_map.h>
#include "hydrogen/version.h"
#include "hydrogen/helpers/filesystem.h"

#include <QDir>
//#include <QApplication>

static bool shouldRemove(QString& first, QString& second)
{
    return (first.compare(second) == 0);
};

namespace H2Core
{

Preferences* Preferences::__instance = NULL;

void Preferences::create_instance()
{
	if ( __instance == 0 ) {
		__instance = new Preferences;
	}
}

const char* Preferences::__class_name = "Preferences";

Preferences::Preferences()
		: Object( __class_name )
		, demoPath( Filesystem::demos_dir()+"/")
{
	__instance = this;
	INFOLOG( "INIT" );

	//Default jack track-outputs are post fader
	m_nJackTrackOutputMode = POST_FADER;
	m_bJackTrackOuts = false;
	// switch to enable / disable lash, only on h2 startup
	m_brestartLash = false;
	m_bsetLash = false;

	//init pre delete default
	m_nRecPreDelete = 0;
	m_nRecPostDelete = 0;

	//server list
	std::list<QString> sServerList;

	//rubberband bpm change queue
	m_useTheRubberbandBpmChangeEvent = false;
	__rubberBandCalcTime = 5;

	QString rubberBandCLIPath = getenv( "PATH" );
	QStringList rubberBandCLIPathList = rubberBandCLIPath.split(":");//linux use ":" as seperator. maybe windows and osx use other seperators

	//find the Rubberband-CLI in system env
	//if this fails a second test will check individual user settings
	for(int i = 0; i < rubberBandCLIPathList.size(); ++i){
		m_rubberBandCLIexecutable = rubberBandCLIPathList[i] + "/rubberband";
		if ( QFile( m_rubberBandCLIexecutable ).exists() == true ){
			readPrefFileforotherplaces = false;
			break;
		}else
		{
			m_rubberBandCLIexecutable = "Path to Rubberband-CLI";
			readPrefFileforotherplaces = true;
		}
	}

	m_pDefaultUIStyle = new UIStyle();
	m_nDefaultUILayout = UI_LAYOUT_SINGLE_PANE;

#ifdef Q_OS_MACX
	m_sPreferencesFilename = QDir::homePath().append( "/Library/Application Support/Hydrogen/hydrogen.conf" );
	m_sPreferencesDirectory = QDir::homePath().append( "/Library/Application Support/Hydrogen/" );
	m_sDataDirectory = QDir::homePath().append( "/Library/Application Support/Hydrogen/data/" );
#else
	m_sPreferencesFilename = QDir::homePath().append( "/.hydrogen/hydrogen.conf" );
	m_sPreferencesDirectory = QDir::homePath().append( "/.hydrogen/" );
	m_sDataDirectory = QDir::homePath().append( "/.hydrogen/data/" );
#endif
	m_sTmpDirectory = QDir::tempPath().append( "/hydrogen/" );
	if ( !QDir(m_sTmpDirectory).exists() ) {
		QDir(m_sTmpDirectory).mkdir( m_sTmpDirectory );// create the tmp directory
	}
	
	char * ladpath = getenv( "LADSPA_PATH" );	// read the Environment variable LADSPA_PATH
	if ( ladpath ) {
		INFOLOG( "Found LADSPA_PATH environment variable" );
		QString sLadspaPath = QString::fromLocal8Bit(ladpath);
		int pos;
		while ( ( pos = sLadspaPath.indexOf( ":" ) ) != -1 ) {
			QString sPath = sLadspaPath.left( pos );
			m_ladspaPathVect.push_back( QFileInfo(sPath).canonicalFilePath() );
			sLadspaPath = sLadspaPath.mid( pos + 1, sLadspaPath.length() );
		}
		m_ladspaPathVect.push_back( QFileInfo(sLadspaPath).canonicalFilePath());
	} else {
#ifdef Q_OS_MACX
		m_ladspaPathVect.push_back( QFileInfo(qApp->applicationDirPath(), "/../Resources/plugins").canonicalFilePath() );
		m_ladspaPathVect.push_back( QFileInfo("/Library/Audio/Plug-Ins/LADSPA/").canonicalFilePath() );
		m_ladspaPathVect.push_back( QFileInfo(QDir::homePath(), "/Library/Audio/Plug-Ins/LADSPA").canonicalFilePath() );
#else
		m_ladspaPathVect.push_back( QFileInfo("/usr/lib/ladspa").canonicalFilePath() );
		m_ladspaPathVect.push_back( QFileInfo("/usr/local/lib/ladspa").canonicalFilePath() );
		m_ladspaPathVect.push_back( QFileInfo("/usr/lib64/ladspa").canonicalFilePath() );
		m_ladspaPathVect.push_back( QFileInfo("/usr/local/lib64/ladspa").canonicalFilePath() );
#endif
	}
	
	/*
	 *  Add .hydrogen/data/plugins to ladspa search path, no matter where LADSPA_PATH points to..
	 */
    m_ladspaPathVect.push_back( QFileInfo(m_sDataDirectory, "plugins").canonicalFilePath() );
    std::sort(m_ladspaPathVect.begin(), m_ladspaPathVect.end());

    auto last = std::unique(m_ladspaPathVect.begin(), m_ladspaPathVect.end(), shouldRemove);
    m_ladspaPathVect.erase(last, m_ladspaPathVect.end());

	__lastspatternDirectory = QDir::homePath();
	__lastsampleDirectory = QDir::homePath(); //audio file browser
	__playsamplesonclicking = false; // audio file browser
	__playselectedinstrument = false; // midi keyboard and keyboard play only selected instrument

	recordEvents = false; // not recording by default
	destructiveRecord = false; // not destructively recording by default
	punchInPos = 0;
	punchOutPos = -1;

	__expandSongItem = true; //SoundLibraryPanel
	__expandPatternItem = true; //SoundLibraryPanel
	__useTimelineBpm = false;		// use timeline
	
	//export dialog
	m_sExportDirectory = QDir::homePath();
	m_nExportMode = 0;
	m_nExportSampleRate = 44100;
	m_nExportSampleDepth = 0;


	/////////////////////////////////////////////////////////////////////////
	/////////////////// DEFAULT SETTINGS ////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////
	m_bFollowPlayhead = true;
	// SEE ABOVE: m_brestartLash
	// SEE ABOVE: m_bsetLash

	m_bbc = false;
	m_mmcsetplay = false;

	m_countOffset = 0;  // beatcounter
	m_startOffset = 0;  // beatcounter

	sServerList.push_back( QString("http://www.hydrogen-music.org/feeds/drumkit_list.php") );
	m_patternCategories.push_back( QString("not_categorized") );

	//___ audio engine properties ___
	m_sAudioDriver = QString("Auto");
	m_bUseMetronome = false;
	m_fMetronomeVolume = 0.5;
	m_nMaxNotes = 256;
	m_nBufferSize = 1024;
	m_nSampleRate = 44100;

	//___ oss driver properties ___
	m_sOSSDevice = QString("/dev/dsp");

	//___ MIDI Driver properties
	m_sMidiDriver = QString("ALSA");
	m_sMidiPortName = QString("None");
	m_nMidiChannelFilter = -1;
	m_bMidiNoteOffIgnore = false;
	m_bMidiFixedMapping = false;
	m_bMidiDiscardNoteAfterAction = false;

	//___  alsa audio driver properties ___
	m_sAlsaAudioDevice = QString("hw:0");

	//___  jack driver properties ___
	m_sJackPortName1 = QString("alsa_pcm:playback_1");
	m_sJackPortName2 = QString("alsa_pcm:playback_2");
	m_bJackTransportMode = true;
	m_bJackConnectDefaults = true;
	m_bJackTrackOuts = false;
	m_nJackTrackOutputMode = 0;
	m_bJackMasterMode = false ;

	// OSC configuration
	m_bOscServerEnabled = false;
	m_nOscServerPort = 9000;

	// None: m_sDefaultEditor;
	// SEE ABOVE: m_sDataDirectory
	// SEE ABOVE: demoPath

	//___ General properties ___
	m_bPatternModePlaysSelected = true;
	m_brestoreLastSong = true;
	m_brestoreLastPlaylist = false;
	m_bUseLash = false;
	m_bShowDevelWarning = false;
	// NONE: lastSongFilename;
	hearNewNotes = true;
	// NONE: m_recentFiles;
	// NONE: m_recentFX;
	// NONE: m_ladspaPathVect;
	quantizeEvents = true;
	recordEvents = false;
	m_bUseRelativeFilenamesForPlaylists = false;

	//___ GUI properties ___
	m_sQTStyle = "Fusion";
	applicationFontFamily = "Lucida Grande";
	applicationFontPointSize = 10;
	mixerFontFamily = "Lucida Grande";
	mixerFontPointSize = 11;
	mixerFalloffSpeed = 1.1;
	m_nPatternEditorGridResolution = 8;
	m_bPatternEditorUsingTriplets = false;
	m_bShowInstrumentPeaks = true;
	m_bIsFXTabVisible = true;
	m_bShowAutomationArea = false;
	m_nPatternEditorGridHeight = 21;
	m_nPatternEditorGridWidth = 3;
	mainFormProperties.set(0, 0, 1000, 700, true);
	mixerProperties.set(10, 350, 829, 276, true);
	patternEditorProperties.set(280, 100, 706, 439, true);
	songEditorProperties.set(10, 10, 600, 250, true);
	drumkitManagerProperties.set(500, 20, 526, 437, true);
	audioEngineInfoProperties.set(720, 120, 0, 0, false);
	m_ladspaProperties[0].set(2, 20, 0, 0, false);
	m_ladspaProperties[1].set(2, 20, 0, 0, false);
	m_ladspaProperties[2].set(2, 20, 0, 0, false);
	m_ladspaProperties[3].set(2, 20, 0, 0, false);

	m_nColoringMethod = 2;
	m_nColoringMethodAuxValue = 213;


	UIStyle* uis = m_pDefaultUIStyle;
	uis->m_songEditor_backgroundColor = H2RGBColor(95, 101, 117);
	uis->m_songEditor_alternateRowColor = H2RGBColor(128, 134, 152);
	uis->m_songEditor_selectedRowColor = H2RGBColor(128, 134, 152);
	uis->m_songEditor_lineColor = H2RGBColor(72, 76, 88);
	uis->m_songEditor_textColor = H2RGBColor(196, 201, 214);
	uis->m_songEditor_pattern1Color = H2RGBColor(97, 167, 251);
	uis->m_patternEditor_backgroundColor = H2RGBColor(167, 168, 163);
	uis->m_patternEditor_alternateRowColor = H2RGBColor(167, 168, 163);
	uis->m_patternEditor_selectedRowColor = H2RGBColor(207, 208, 200);
	uis->m_patternEditor_textColor = H2RGBColor(40, 40, 40);
	uis->m_patternEditor_noteColor = H2RGBColor(40, 40, 40);
	uis->m_patternEditor_lineColor = H2RGBColor(65, 65, 65);
	uis->m_patternEditor_line1Color = H2RGBColor(75, 75, 75);
	uis->m_patternEditor_line2Color = H2RGBColor(95, 95, 95);
	uis->m_patternEditor_line3Color = H2RGBColor(115, 115, 115);
	uis->m_patternEditor_line4Color = H2RGBColor(125, 125, 125);
	uis->m_patternEditor_line5Color = H2RGBColor(135, 135, 135);

	/////////////////////////////////////////////////////////////////////////
	//////////////// END OF DEFAULT SETTINGS ////////////////////////////////
	/////////////////////////////////////////////////////////////////////////

	loadPreferences( true );	// Global settings
	loadPreferences( false );	// User settings
}



Preferences::~Preferences()
{
	savePreferences();

	INFOLOG( "DESTROY" );
	__instance = NULL;
	delete m_pDefaultUIStyle;
}






///
/// Load the preferences file
///
void Preferences::loadPreferences( bool bGlobal )
{
	bool recreate = false;	// configuration file must be recreated?

	QString sPreferencesDirectory;
	QString sPreferencesFilename;
	QString sDataDirectory;
	if ( bGlobal ) {
		sPreferencesDirectory = Filesystem::sys_data_path();
		sPreferencesFilename = sPreferencesDirectory + "/hydrogen.default.conf";
		INFOLOG( "Loading preferences file (GLOBAL) [" + sPreferencesFilename + "]" );
	} else {
		sPreferencesFilename = m_sPreferencesFilename;
		sPreferencesDirectory = m_sPreferencesDirectory;
		sDataDirectory = QDir::homePath().append( "/.hydrogen/data" );
		INFOLOG( "Loading preferences file (USER) [" + sPreferencesFilename + "]" );
	}

	// preferences directory exists?
	QDir prefDir( sPreferencesDirectory );
	if ( !prefDir.exists() ) {
		if ( bGlobal ) {
			WARNINGLOG( "System configuration directory '" + sPreferencesDirectory + "' not found." );
		} else {
			ERRORLOG( "Configuration directory '" + sPreferencesDirectory + "' not found." );
			createPreferencesDirectory();
		}
	}

	// data directory exists?
	QDir dataDir( sDataDirectory );
	if ( !dataDir.exists() ) {
		WARNINGLOG( "Data directory not found." );
		createDataDirectory();
	}

	// soundLibrary directory exists?
	QString sDir = sDataDirectory;
	QString sDrumkitDir;
	QString sSongDir;
	QString sPatternDir;

	INFOLOG( "Creating soundLibrary directories in " + sDir );

	sDrumkitDir = sDir + "/drumkits";
	sSongDir = sDir + "/songs";
	sPatternDir = sDir + "/patterns";

	QDir drumkitDir( sDrumkitDir );
	QDir songDir( sSongDir );
	QDir patternDir( sPatternDir );

	if ( ! drumkitDir.exists() || ! songDir.exists() || ! patternDir.exists() )
	{
		createSoundLibraryDirectories();
	}

	// pref file exists?
	std::ifstream input( sPreferencesFilename.toLocal8Bit() , std::ios::in | std::ios::binary );
	if ( input ) {

		// read preferences file
		QDomDocument doc = LocalFileMng::openXmlDocument( sPreferencesFilename );
		QDomNode rootNode = doc.firstChildElement( "hydrogen_preferences" );

		if ( !rootNode.isNull() ) {

			// version
			QString version = LocalFileMng::readXmlString( rootNode, "version", "" );
			if ( version.isEmpty() ) {
				recreate = true;
			}

			//////// GENERAL ///////////
			//m_sLadspaPath = LocalFileMng::readXmlString( this, rootNode, "ladspaPath", m_sLadspaPath );
			__playselectedinstrument = LocalFileMng::readXmlBool( rootNode, "instrumentInputMode", __playselectedinstrument );
			m_bShowDevelWarning = LocalFileMng::readXmlBool( rootNode, "showDevelWarning", m_bShowDevelWarning );
			m_brestoreLastSong = LocalFileMng::readXmlBool( rootNode, "restoreLastSong", m_brestoreLastSong );
			m_brestoreLastPlaylist = LocalFileMng::readXmlBool( rootNode, "restoreLastPlaylist", m_brestoreLastPlaylist );
			m_bPatternModePlaysSelected = LocalFileMng::readXmlBool( rootNode, "patternModePlaysSelected", true );
			m_bUseLash = LocalFileMng::readXmlBool( rootNode, "useLash", false );
			__useTimelineBpm = LocalFileMng::readXmlBool( rootNode, "useTimeLine", __useTimelineBpm );
			maxBars = LocalFileMng::readXmlInt( rootNode, "maxBars", 400 );
			m_nDefaultUILayout =  LocalFileMng::readXmlInt( rootNode, "defaultUILayout", UI_LAYOUT_SINGLE_PANE );
			m_nLastOpenTab =  LocalFileMng::readXmlInt( rootNode, "lastOpenTab", 0 );
			m_bUseRelativeFilenamesForPlaylists = LocalFileMng::readXmlBool( rootNode, "useRelativeFilenamesForPlaylists", false );

			//restore the right m_bsetlash value
			m_bsetLash = m_bUseLash;
			m_useTheRubberbandBpmChangeEvent = LocalFileMng::readXmlBool( rootNode, "useTheRubberbandBpmChangeEvent", m_useTheRubberbandBpmChangeEvent );
			m_nRecPreDelete = LocalFileMng::readXmlInt( rootNode, "preDelete", 0 );
			m_nRecPostDelete = LocalFileMng::readXmlInt( rootNode, "postDelete", 0 );

			hearNewNotes = LocalFileMng::readXmlBool( rootNode, "hearNewNotes", hearNewNotes );
			quantizeEvents = LocalFileMng::readXmlBool( rootNode, "quantizeEvents", quantizeEvents );

			//rubberband
			if( readPrefFileforotherplaces ){
				//this scond test will check individual user settings
				QString test = LocalFileMng::readXmlString( rootNode, "path_to_rubberband", "");
				if ( QFile( test ).exists() == true ){
					m_rubberBandCLIexecutable = test;
				}else
				{
					m_rubberBandCLIexecutable = "Path to Rubberband-CLI";
				}
			}

			QDomNode pRecentUsedSongsNode = rootNode.firstChildElement( "recentUsedSongs" );
			if ( !pRecentUsedSongsNode.isNull() ) {
				QDomElement pSongElement = pRecentUsedSongsNode.firstChildElement( "song" );
				while( !pSongElement.isNull() && !pSongElement.text().isEmpty() ){
					m_recentFiles.push_back( pSongElement.text() );
					pSongElement = pSongElement.nextSiblingElement( "song" );
				}

			} else {
				WARNINGLOG( "recentUsedSongs node not found" );
			}

			QDomNode pRecentFXNode = rootNode.firstChildElement( "recentlyUsedEffects" );
			if ( ! pRecentFXNode.isNull() ) {
				QDomElement pFXElement = pRecentFXNode.firstChildElement( "FX" );
				while ( !pFXElement.isNull()  && ! pFXElement.text().isEmpty()) {
					m_recentFX.push_back( pFXElement.text() );
					pFXElement = pFXElement.nextSiblingElement( "FX" );
				}
			} else {
				WARNINGLOG( "recentlyUsedEffects node not found" );
			}

			sServerList.clear();
			QDomNode pServerListNode = rootNode.firstChildElement( "serverList" );
			if ( !pServerListNode.isNull() ) {
				QDomElement pServerElement = pServerListNode.firstChildElement( "server" );
				while ( !pServerElement.isNull() && !pServerElement.text().isEmpty() ) {
					sServerList.push_back( pServerElement.text() );
					pServerElement = pServerElement.nextSiblingElement( "server" );
				}
			} else {
				WARNINGLOG( "serverList node not found" );
			}

			m_patternCategories.clear();
			QDomNode pPatternCategoriesNode = rootNode.firstChildElement( "patternCategories" );
			if ( !pPatternCategoriesNode.isNull() ) {
				QDomElement pPatternCategoriesElement = pPatternCategoriesNode.firstChildElement( "categories" );
				while ( !pPatternCategoriesElement.isNull() && !pPatternCategoriesElement.text().isEmpty() ) {
					m_patternCategories.push_back( pPatternCategoriesElement.text() );
					pPatternCategoriesElement = pPatternCategoriesElement.nextSiblingElement( "categories" );
				}
			} else {
				WARNINGLOG( "patternCategories node not found" );
			}


			/////////////// AUDIO ENGINE //////////////
			QDomNode audioEngineNode = rootNode.firstChildElement( "audio_engine" );
			if ( audioEngineNode.isNull() ) {
				WARNINGLOG( "audio_engine node not found" );
				recreate = true;
			} else {
				m_sAudioDriver = LocalFileMng::readXmlString( audioEngineNode, "audio_driver", m_sAudioDriver );
				m_bUseMetronome = LocalFileMng::readXmlBool( audioEngineNode, "use_metronome", m_bUseMetronome );
				m_fMetronomeVolume = LocalFileMng::readXmlFloat( audioEngineNode, "metronome_volume", 0.5f );
				m_nMaxNotes = LocalFileMng::readXmlInt( audioEngineNode, "maxNotes", m_nMaxNotes );
				m_nBufferSize = LocalFileMng::readXmlInt( audioEngineNode, "buffer_size", m_nBufferSize );
				m_nSampleRate = LocalFileMng::readXmlInt( audioEngineNode, "samplerate", m_nSampleRate );

				//// OSS DRIVER ////
				QDomNode ossDriverNode = audioEngineNode.firstChildElement( "oss_driver" );
				if ( ossDriverNode.isNull()  ) {
					WARNINGLOG( "oss_driver node not found" );
					recreate = true;
				} else {
					m_sOSSDevice = LocalFileMng::readXmlString( ossDriverNode, "ossDevice", m_sOSSDevice );
				}

				//// JACK DRIVER ////
				QDomNode jackDriverNode = audioEngineNode.firstChildElement( "jack_driver" );
				if ( jackDriverNode.isNull() ) {
					WARNINGLOG( "jack_driver node not found" );
					recreate = true;
				} else {
					m_sJackPortName1 = LocalFileMng::readXmlString( jackDriverNode, "jack_port_name_1", m_sJackPortName1 );
					m_sJackPortName2 = LocalFileMng::readXmlString( jackDriverNode, "jack_port_name_2", m_sJackPortName2 );
					QString sMode = LocalFileMng::readXmlString( jackDriverNode, "jack_transport_mode", "NO_JACK_TRANSPORT" );
					if ( sMode == "NO_JACK_TRANSPORT" ) {
						m_bJackTransportMode = NO_JACK_TRANSPORT;
					} else if ( sMode == "USE_JACK_TRANSPORT" ) {
						m_bJackTransportMode = USE_JACK_TRANSPORT;
					}

					//jack time master
					QString tmMode = LocalFileMng::readXmlString( jackDriverNode, "jack_transport_mode_master", "NO_JACK_TIME_MASTER" );
					if ( tmMode == "NO_JACK_TIME_MASTER" ) {
						m_bJackMasterMode = NO_JACK_TIME_MASTER;
					} else if ( tmMode == "USE_JACK_TIME_MASTER" ) {
						m_bJackMasterMode = USE_JACK_TIME_MASTER;
					}
					//~ jack time master

					m_bJackTrackOuts = LocalFileMng::readXmlBool( jackDriverNode, "jack_track_outs", m_bJackTrackOuts );
					m_bJackConnectDefaults = LocalFileMng::readXmlBool( jackDriverNode, "jack_connect_defaults", m_bJackConnectDefaults );

					m_nJackTrackOutputMode = LocalFileMng::readXmlInt( jackDriverNode, "jack_track_output_mode", m_nJackTrackOutputMode );
				}


				/// ALSA AUDIO DRIVER ///
				QDomNode alsaAudioDriverNode = audioEngineNode.firstChildElement( "alsa_audio_driver" );
				if ( alsaAudioDriverNode.isNull() ) {
					WARNINGLOG( "alsa_audio_driver node not found" );
					recreate = true;
				} else {
					m_sAlsaAudioDevice = LocalFileMng::readXmlString( alsaAudioDriverNode, "alsa_audio_device", m_sAlsaAudioDevice );
				}

				/// MIDI DRIVER ///
				QDomNode midiDriverNode = audioEngineNode.firstChildElement( "midi_driver" );
				if ( midiDriverNode.isNull() ) {
					WARNINGLOG( "midi_driver node not found" );
					recreate = true;
				} else {
					m_sMidiDriver = LocalFileMng::readXmlString( midiDriverNode, "driverName", "ALSA" );
					m_sMidiPortName = LocalFileMng::readXmlString( midiDriverNode, "port_name", "None" );
					m_nMidiChannelFilter = LocalFileMng::readXmlInt( midiDriverNode, "channel_filter", -1 );
					m_bMidiNoteOffIgnore = LocalFileMng::readXmlBool( midiDriverNode, "ignore_note_off", true );
					m_bMidiDiscardNoteAfterAction = LocalFileMng::readXmlBool( midiDriverNode, "discard_note_after_action", true);
					m_bMidiFixedMapping = LocalFileMng::readXmlBool( midiDriverNode, "fixed_mapping", false, true );
					m_bEnableMidiFeedback = LocalFileMng::readXmlBool( midiDriverNode, "enable_midi_feedback", false, true );
				}

				/// OSC ///
				QDomNode oscServerNode = audioEngineNode.firstChildElement( "osc_configuration" );
				if ( oscServerNode.isNull() ) {
					WARNINGLOG( "osc_configuration node not found" );
					recreate = true;
				} else {
					m_bOscServerEnabled = LocalFileMng::readXmlBool( oscServerNode, "oscEnabled", false );
					m_nOscServerPort = LocalFileMng::readXmlInt( oscServerNode, "oscServerPort", 9000 );
				}
			}

			/////////////// GUI //////////////
			QDomNode guiNode = rootNode.firstChildElement( "gui" );
			if ( guiNode.isNull() ) {
				WARNINGLOG( "gui node not found" );
				recreate = true;
			} else {
				// QT Style
				m_sQTStyle = LocalFileMng::readXmlString( guiNode, "QTStyle", m_sQTStyle, true );

				if(m_sQTStyle == "Plastique"){
					m_sQTStyle = "Fusion";
				}

				// Application font family
				applicationFontFamily = LocalFileMng::readXmlString( guiNode, "application_font_family", applicationFontFamily );

				// Application font pointSize
				applicationFontPointSize = LocalFileMng::readXmlInt( guiNode, "application_font_pointsize", applicationFontPointSize );

				// mixer font family
				mixerFontFamily = LocalFileMng::readXmlString( guiNode, "mixer_font_family", mixerFontFamily );

				// mixer font pointSize
				mixerFontPointSize = LocalFileMng::readXmlInt( guiNode, "mixer_font_pointsize", mixerFontPointSize );

				// Mixer falloff speed
				mixerFalloffSpeed = LocalFileMng::readXmlFloat( guiNode, "mixer_falloff_speed", 1.1f );

				// pattern editor grid resolution
				m_nPatternEditorGridResolution = LocalFileMng::readXmlInt( guiNode, "patternEditorGridResolution", m_nPatternEditorGridResolution );
				m_bPatternEditorUsingTriplets = LocalFileMng::readXmlBool( guiNode, "patternEditorUsingTriplets", m_bPatternEditorUsingTriplets );
				
				m_bShowInstrumentPeaks = LocalFileMng::readXmlBool( guiNode, "showInstrumentPeaks", m_bShowInstrumentPeaks );
				m_bIsFXTabVisible = LocalFileMng::readXmlBool( guiNode, "isFXTabVisible", m_bIsFXTabVisible );
				m_bShowAutomationArea = LocalFileMng::readXmlBool( guiNode, "showAutomationArea", m_bShowAutomationArea );


				// pattern editor grid height
				m_nPatternEditorGridHeight = LocalFileMng::readXmlInt( guiNode, "patternEditorGridHeight", m_nPatternEditorGridHeight );

				// pattern editor grid width
				m_nPatternEditorGridWidth = LocalFileMng::readXmlInt( guiNode, "patternEditorGridWidth", m_nPatternEditorGridWidth );

				// mainForm window properties
				setMainFormProperties( readWindowProperties( guiNode, "mainForm_properties", mainFormProperties ) );
				setMixerProperties( readWindowProperties( guiNode, "mixer_properties", mixerProperties ) );
				setPatternEditorProperties( readWindowProperties( guiNode, "patternEditor_properties", patternEditorProperties ) );
				setSongEditorProperties( readWindowProperties( guiNode, "songEditor_properties", songEditorProperties ) );
				setAudioEngineInfoProperties( readWindowProperties( guiNode, "audioEngineInfo_properties", audioEngineInfoProperties ) );

				//export dialog properties
				m_nExportTemplate = LocalFileMng::readXmlInt( guiNode, "exportDialogTemplate", 0 );
				m_nExportSampleRate = LocalFileMng::readXmlInt( guiNode, "exportDialogSampleRate", 44100 );
				m_nExportSampleDepth = LocalFileMng::readXmlInt( guiNode, "exportDialogSampleDepth", 0 );
				m_sExportDirectory = LocalFileMng::readXmlString( guiNode, "exportDialogDirectory", QDir::homePath() );
					
				m_bFollowPlayhead = LocalFileMng::readXmlBool( guiNode, "followPlayhead", true );


				//beatcounter
				QString bcMode = LocalFileMng::readXmlString( guiNode, "bc", "BC_OFF" );
					if ( bcMode == "BC_OFF" ) {
						m_bbc = BC_OFF;
					} else if ( bcMode == "BC_ON" ) {
						m_bbc = BC_ON;
					}


				QString setPlay = LocalFileMng::readXmlString( guiNode, "setplay", "SET_PLAY_OFF" );
					if ( setPlay == "SET_PLAY_OFF" ) {
						m_mmcsetplay = SET_PLAY_OFF;
					} else if ( setPlay == "SET_PLAY_ON" ) {
						m_mmcsetplay = SET_PLAY_ON;
					}

				m_countOffset = LocalFileMng::readXmlInt( guiNode, "countoffset", 0 );
				m_startOffset = LocalFileMng::readXmlInt( guiNode, "playoffset", 0 );

				//~ beatcounter

				//SoundLibraryPanel expand items
				__expandSongItem = LocalFileMng::readXmlBool( guiNode, "expandSongItem", __expandSongItem );
				__expandPatternItem = LocalFileMng::readXmlBool( guiNode, "expandPatternItem", __expandPatternItem );

				for ( unsigned nFX = 0; nFX < MAX_FX; nFX++ ) {
					QString sNodeName = QString("ladspaFX_properties%1").arg( nFX );
					setLadspaProperties( nFX, readWindowProperties( guiNode, sNodeName, m_ladspaProperties[nFX] ) );
				}

				QDomNode pUIStyle = guiNode.firstChildElement( "UI_Style" );
				if ( !pUIStyle.isNull() ) {
					readUIStyle( pUIStyle );
				} else {
					WARNINGLOG( "UI_Style node not found" );
					recreate = true;
				}

				//SongEditor coloring
				m_nColoringMethod = LocalFileMng::readXmlInt( guiNode, "SongEditor_ColoringMethod", 2 );
				m_nColoringMethodAuxValue = LocalFileMng::readXmlInt( guiNode, "SongEditor_ColoringMethodAuxValue", 213 );

			}

			/////////////// FILES //////////////
			QDomNode filesNode = rootNode.firstChildElement( "files" );
			if ( filesNode.isNull() ) {
				WARNINGLOG( "files node not found" );
				recreate = true;
			} else {
				// last used song
				m_lastSongFilename = LocalFileMng::readXmlString( filesNode, "lastSongFilename", m_lastSongFilename, true );
				m_lastPlaylistFilename = LocalFileMng::readXmlString( filesNode, "lastPlaylistFilename", m_lastPlaylistFilename, true );
				m_sDefaultEditor = LocalFileMng::readXmlString( filesNode, "defaulteditor", m_sDefaultEditor, true );
			}

			MidiMap::reset_instance();
			MidiMap* mM = MidiMap::get_instance();


			QDomNode pMidiEventMapNode = rootNode.firstChildElement( "midiEventMap" );
			if ( !pMidiEventMapNode.isNull() ) {
				QDomNode pMidiEventNode = pMidiEventMapNode.firstChildElement( "midiEvent" );
				while ( !pMidiEventNode.isNull() ) {
					if( pMidiEventNode.firstChildElement().nodeName() == QString("mmcEvent")){
						QString event = pMidiEventNode.firstChildElement("mmcEvent").text();
						QString s_action = pMidiEventNode.firstChildElement("action").text();
						QString s_param = pMidiEventNode.firstChildElement("parameter").text();

												Action* pAction = new Action( s_action );
						pAction->setParameter1( s_param );
						mM->registerMMCEvent(event, pAction);
					}

					if( pMidiEventNode.firstChildElement().nodeName() == QString("noteEvent")){
						QString event = pMidiEventNode.firstChildElement("noteEvent").text();
						QString s_action = pMidiEventNode.firstChildElement("action").text();
						QString s_param = pMidiEventNode.firstChildElement("parameter").text();
						QString s_eventParameter = pMidiEventNode.firstChildElement("eventParameter").text();
						Action* pAction = new Action( s_action );
						pAction->setParameter1( s_param );
						mM->registerNoteEvent(s_eventParameter.toInt(), pAction);
					}

					if( pMidiEventNode.firstChildElement().nodeName() == QString("ccEvent") ){
						QString event = pMidiEventNode.firstChildElement("ccEvent").text();
						QString s_action = pMidiEventNode.firstChildElement("action").text();
						QString s_param = pMidiEventNode.firstChildElement("parameter").text();
						QString s_eventParameter = pMidiEventNode.firstChildElement("eventParameter").text();
						Action * pAction = new Action( s_action );
						pAction->setParameter1( s_param );
						mM->registerCCEvent( s_eventParameter.toInt(), pAction );
					}

					if( pMidiEventNode.firstChildElement().nodeName() == QString("pcEvent") ){
						QString event = pMidiEventNode.firstChildElement("pcEvent").text();
						QString s_action = pMidiEventNode.firstChildElement("action").text();
						QString s_param = pMidiEventNode.firstChildElement("parameter").text();
						Action * pAction = new Action( s_action );
						pAction->setParameter1( s_param );
						mM->registerPCEvent( pAction );
					}

					pMidiEventNode = pMidiEventNode.nextSiblingElement( "midiEvent" );
				}

			} else {
				WARNINGLOG( "midiMap node not found" );
			}
		} // rootNode
		else {
			WARNINGLOG( "hydrogen_preferences node not found" );
			recreate = true;
		}
	} else {
		if ( bGlobal ) {
			WARNINGLOG( "System configuration file not found." );
		} else {
			WARNINGLOG( "Configuration file not found." );
			recreate = true;
		}
	}


	// The preferences file should be recreated?
	if ( recreate == true && !bGlobal ) {
		WARNINGLOG( "Recreating configuration file." );
		savePreferences();
	}
}



///
/// Save the preferences file
///
void Preferences::savePreferences()
{
	QString filename = m_sPreferencesFilename;

	INFOLOG( "Saving preferences file: " + filename );

	QDomDocument doc;
	QDomProcessingInstruction header = doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	doc.appendChild( header );

	QDomNode rootNode = doc.createElement( "hydrogen_preferences" );

	// hydrogen version
	LocalFileMng::writeXmlString( rootNode, "version", QString( get_version().c_str() ) );

	////// GENERAL ///////
	LocalFileMng::writeXmlString( rootNode, "restoreLastSong", m_brestoreLastSong ? "true": "false" );
	LocalFileMng::writeXmlString( rootNode, "restoreLastPlaylist", m_brestoreLastPlaylist ? "true": "false" );

	LocalFileMng::writeXmlString( rootNode, "patternModePlaysSelected", m_bPatternModePlaysSelected ? "true": "false" );

	LocalFileMng::writeXmlString( rootNode, "useLash", m_bsetLash ? "true": "false" );
	LocalFileMng::writeXmlString( rootNode, "useTimeLine", __useTimelineBpm ? "true": "false" );

	LocalFileMng::writeXmlString( rootNode, "maxBars", QString::number( maxBars ) );

	LocalFileMng::writeXmlString( rootNode, "defaultUILayout", QString::number( m_nDefaultUILayout ) );
	LocalFileMng::writeXmlString( rootNode, "lastOpenTab", QString::number( m_nLastOpenTab ) );

	LocalFileMng::writeXmlString( rootNode, "useTheRubberbandBpmChangeEvent", m_useTheRubberbandBpmChangeEvent ? "true": "false" );

	LocalFileMng::writeXmlString( rootNode, "preDelete", QString("%1").arg(m_nRecPreDelete) );
	LocalFileMng::writeXmlString( rootNode, "postDelete", QString("%1").arg(m_nRecPostDelete) );
	LocalFileMng::writeXmlString( rootNode, "useRelativeFilenamesForPlaylists", m_bUseRelativeFilenamesForPlaylists ? "true": "false" );
	
	// instrument input mode
	LocalFileMng::writeXmlString( rootNode, "instrumentInputMode", __playselectedinstrument ? "true": "false" );
	
	//show development version warning
	LocalFileMng::writeXmlString( rootNode, "showDevelWarning", m_bShowDevelWarning ? "true": "false" );

	// hear new notes in the pattern editor
	LocalFileMng::writeXmlString( rootNode, "hearNewNotes", hearNewNotes ? "true": "false" );

	// key/midi event prefs
	//LocalFileMng::writeXmlString( rootNode, "recordEvents", recordEvents ? "true": "false" );
	LocalFileMng::writeXmlString( rootNode, "quantizeEvents", quantizeEvents ? "true": "false" );

	//extern executables
	if ( QFile( m_rubberBandCLIexecutable ).exists() == false ) {
		m_rubberBandCLIexecutable = "Path to Rubberband-CLI";
	}
	LocalFileMng::writeXmlString( rootNode, "path_to_rubberband", QString(m_rubberBandCLIexecutable));

	// Recent used songs
	QDomNode recentUsedSongsNode = doc.createElement( "recentUsedSongs" );
	{
		unsigned nSongs = 5;
		if ( m_recentFiles.size() < 5 ) {
			nSongs = m_recentFiles.size();
		}
		for ( unsigned i = 0; i < nSongs; i++ ) {
			LocalFileMng::writeXmlString( recentUsedSongsNode, "song", m_recentFiles[ i ] );
		}
	}
	rootNode.appendChild( recentUsedSongsNode );

	QDomNode recentFXNode = doc.createElement( "recentlyUsedEffects" );
	{
		int nFX = 0;
		QString FXname;
		foreach( FXname, m_recentFX ) {
			LocalFileMng::writeXmlString( recentFXNode, "FX", FXname );
			if ( ++nFX > 10 ) break;
		}
	}
	rootNode.appendChild( recentFXNode );


	std::list<QString>::const_iterator cur_Server;

	QDomNode serverListNode = doc.createElement( "serverList" );
	for( cur_Server = sServerList.begin(); cur_Server != sServerList.end(); ++cur_Server ){
		LocalFileMng::writeXmlString( serverListNode , QString("server") , QString( *cur_Server ) );
	}
	rootNode.appendChild( serverListNode );


	std::list<QString>::const_iterator cur_patternCategories;

	QDomNode patternCategoriesNode = doc.createElement( "patternCategories" );
	for( cur_patternCategories = m_patternCategories.begin(); cur_patternCategories != m_patternCategories.end(); ++cur_patternCategories ){
		LocalFileMng::writeXmlString( patternCategoriesNode , QString("categories") , QString( *cur_patternCategories ) );
	}
	rootNode.appendChild( patternCategoriesNode );



	//---- AUDIO ENGINE ----
	QDomNode audioEngineNode = doc.createElement( "audio_engine" );
	{
		// audio driver
		LocalFileMng::writeXmlString( audioEngineNode, "audio_driver", m_sAudioDriver );

		// use metronome
		LocalFileMng::writeXmlString( audioEngineNode, "use_metronome", m_bUseMetronome ? "true": "false" );
		LocalFileMng::writeXmlString( audioEngineNode, "metronome_volume", QString("%1").arg( m_fMetronomeVolume ) );
		LocalFileMng::writeXmlString( audioEngineNode, "maxNotes", QString("%1").arg( m_nMaxNotes ) );
		LocalFileMng::writeXmlString( audioEngineNode, "buffer_size", QString("%1").arg( m_nBufferSize ) );
		LocalFileMng::writeXmlString( audioEngineNode, "samplerate", QString("%1").arg( m_nSampleRate ) );

		//// OSS DRIVER ////
		QDomNode ossDriverNode = doc.createElement( "oss_driver" );
		{
			LocalFileMng::writeXmlString( ossDriverNode, "ossDevice", m_sOSSDevice );
		}
		audioEngineNode.appendChild( ossDriverNode );

		//// JACK DRIVER ////
		QDomNode jackDriverNode = doc.createElement( "jack_driver" );
		{
			LocalFileMng::writeXmlString( jackDriverNode, "jack_port_name_1", m_sJackPortName1 );	// jack port name 1
			LocalFileMng::writeXmlString( jackDriverNode, "jack_port_name_2", m_sJackPortName2 );	// jack port name 2

			// jack transport slave
			QString sMode;
			if ( m_bJackTransportMode == NO_JACK_TRANSPORT ) {
				sMode = "NO_JACK_TRANSPORT";
			} else if ( m_bJackTransportMode == USE_JACK_TRANSPORT ) {
				sMode = "USE_JACK_TRANSPORT";
			}
			LocalFileMng::writeXmlString( jackDriverNode, "jack_transport_mode", sMode );

			//jack time master
			QString tmMode;
			if ( m_bJackMasterMode == NO_JACK_TIME_MASTER ) {
				tmMode = "NO_JACK_TIME_MASTER";
			} else if (  m_bJackMasterMode == USE_JACK_TIME_MASTER ) {
				tmMode = "USE_JACK_TIME_MASTER";
			}
			LocalFileMng::writeXmlString( jackDriverNode, "jack_transport_mode_master", tmMode );
			//~ jack time master

			// jack default connection
			QString jackConnectDefaultsString = "false";
			if ( m_bJackConnectDefaults ) {
				jackConnectDefaultsString = "true";
			}
			LocalFileMng::writeXmlString( jackDriverNode, "jack_connect_defaults", jackConnectDefaultsString );

			//pre-fader or post-fader track outputs ?
			LocalFileMng::writeXmlString( jackDriverNode, "jack_track_output_mode", QString("%1").arg( m_nJackTrackOutputMode ));

			// jack track outs
			QString jackTrackOutsString = "false";
			if ( m_bJackTrackOuts ) {
				jackTrackOutsString = "true";
			}
			LocalFileMng::writeXmlString( jackDriverNode, "jack_track_outs", jackTrackOutsString );
		}
		audioEngineNode.appendChild( jackDriverNode );

		//// ALSA AUDIO DRIVER ////
		QDomNode alsaAudioDriverNode = doc.createElement( "alsa_audio_driver" );
		{
			LocalFileMng::writeXmlString( alsaAudioDriverNode, "alsa_audio_device", m_sAlsaAudioDevice );
		}
		audioEngineNode.appendChild( alsaAudioDriverNode );

		/// MIDI DRIVER ///
		QDomNode midiDriverNode = doc.createElement( "midi_driver" );
		{
			LocalFileMng::writeXmlString( midiDriverNode, "driverName", m_sMidiDriver );
			LocalFileMng::writeXmlString( midiDriverNode, "port_name", m_sMidiPortName );
			LocalFileMng::writeXmlString( midiDriverNode, "channel_filter", QString("%1").arg( m_nMidiChannelFilter ) );

			if ( m_bMidiNoteOffIgnore ) {
				LocalFileMng::writeXmlString( midiDriverNode, "ignore_note_off", "true" );
			} else {
				LocalFileMng::writeXmlString( midiDriverNode, "ignore_note_off", "false" );
			}
			
			if ( m_bEnableMidiFeedback ) {
				LocalFileMng::writeXmlString( midiDriverNode, "enable_midi_feedback", "true" );
			} else {
				LocalFileMng::writeXmlString( midiDriverNode, "enable_midi_feedback", "false" );
			}

			if ( m_bMidiDiscardNoteAfterAction ) {
				LocalFileMng::writeXmlString( midiDriverNode, "discard_note_after_action", "true" );
			} else {
				LocalFileMng::writeXmlString( midiDriverNode, "discard_note_after_action", "false" );
			}

			if ( m_bMidiFixedMapping ) {
				LocalFileMng::writeXmlString( midiDriverNode, "fixed_mapping", "true" );
				INFOLOG("Saving fixed mapping\n");
			} else {
				LocalFileMng::writeXmlString( midiDriverNode, "fixed_mapping", "false" );
				INFOLOG("Saving fixed mapping false\n");
			}
		}
		audioEngineNode.appendChild( midiDriverNode );
		
		/// OSC ///
		QDomNode oscNode = doc.createElement( "osc_configuration" );
		{
			LocalFileMng::writeXmlString( oscNode, "oscServerPort", QString("%1").arg( m_nOscServerPort ) );

			if ( m_bOscServerEnabled ) {
				LocalFileMng::writeXmlString( oscNode, "oscEnabled", "true" );
			} else {
				LocalFileMng::writeXmlString( oscNode, "oscEnabled", "false" );
			}
		}
		audioEngineNode.appendChild( oscNode );
		
	}
	rootNode.appendChild( audioEngineNode );

	//---- GUI ----
	QDomNode guiNode = doc.createElement( "gui" );
	{
		LocalFileMng::writeXmlString( guiNode, "QTStyle", m_sQTStyle );
		LocalFileMng::writeXmlString( guiNode, "application_font_family", applicationFontFamily );
		LocalFileMng::writeXmlString( guiNode, "application_font_pointsize", QString("%1").arg( applicationFontPointSize ) );
		LocalFileMng::writeXmlString( guiNode, "mixer_font_family", mixerFontFamily );
		LocalFileMng::writeXmlString( guiNode, "mixer_font_pointsize", QString("%1").arg( mixerFontPointSize ) );
		LocalFileMng::writeXmlString( guiNode, "mixer_falloff_speed", QString("%1").arg( mixerFalloffSpeed ) );
		LocalFileMng::writeXmlString( guiNode, "patternEditorGridResolution", QString("%1").arg( m_nPatternEditorGridResolution ) );
		LocalFileMng::writeXmlString( guiNode, "patternEditorGridHeight", QString("%1").arg( m_nPatternEditorGridHeight ) );
		LocalFileMng::writeXmlString( guiNode, "patternEditorGridWidth", QString("%1").arg( m_nPatternEditorGridWidth ) );
		LocalFileMng::writeXmlBool( guiNode, "patternEditorUsingTriplets", m_bPatternEditorUsingTriplets );
		LocalFileMng::writeXmlBool( guiNode, "showInstrumentPeaks", m_bShowInstrumentPeaks );
		LocalFileMng::writeXmlBool( guiNode, "isFXTabVisible", m_bIsFXTabVisible );
		LocalFileMng::writeXmlBool( guiNode, "showAutomationArea", m_bShowAutomationArea );

		// MainForm window properties
		writeWindowProperties( guiNode, "mainForm_properties", mainFormProperties );
		writeWindowProperties( guiNode, "mixer_properties", mixerProperties );
		writeWindowProperties( guiNode, "patternEditor_properties", patternEditorProperties );
		writeWindowProperties( guiNode, "songEditor_properties", songEditorProperties );
		writeWindowProperties( guiNode, "drumkitManager_properties", drumkitManagerProperties );
		writeWindowProperties( guiNode, "audioEngineInfo_properties", audioEngineInfoProperties );
		for ( unsigned nFX = 0; nFX < MAX_FX; nFX++ ) {
			QString sNode = QString("ladspaFX_properties%1").arg( nFX );
			writeWindowProperties( guiNode, sNode, m_ladspaProperties[nFX] );
		}
		
		
		//ExportSongDialog
		LocalFileMng::writeXmlString( guiNode, "exportDialogTemplate", QString("%1").arg( m_nExportTemplate ) );
		LocalFileMng::writeXmlString( guiNode, "exportDialogSampleRate",  QString("%1").arg( m_nExportSampleRate ) );
		LocalFileMng::writeXmlString( guiNode, "exportDialogSampleDepth", QString("%1").arg( m_nExportSampleDepth ) );
		LocalFileMng::writeXmlString( guiNode, "exportDialogDirectory", m_sExportDirectory );


		//beatcounter
		QString bcMode;

		if ( m_bbc == BC_OFF ) {
			bcMode = "BC_OFF";
		} else if ( m_bbc  == BC_ON ) {
			bcMode = "BC_ON";
		}
		LocalFileMng::writeXmlString( guiNode, "bc", bcMode );



		QString setPlay;
		if ( m_mmcsetplay == SET_PLAY_OFF ) {
			setPlay = "SET_PLAY_OFF";
		} else if ( m_mmcsetplay == SET_PLAY_ON ) {
			setPlay = "SET_PLAY_ON";
		}
		LocalFileMng::writeXmlString( guiNode, "setplay", setPlay );

		LocalFileMng::writeXmlString( guiNode, "countoffset", QString("%1").arg(m_countOffset) );
		LocalFileMng::writeXmlString( guiNode, "playoffset", QString("%1").arg(m_startOffset) );
		//~ beatcounter


		//SoundLibraryPanel expand items
		LocalFileMng::writeXmlString( guiNode, "expandSongItem", __expandSongItem ? "true": "false" );
		LocalFileMng::writeXmlString( guiNode, "expandPatternItem", __expandPatternItem ? "true": "false" );

		// User interface style
		writeUIStyle( guiNode );

		//SongEditor coloring method
		LocalFileMng::writeXmlString( guiNode, "SongEditor_ColoringMethod", QString::number( m_nColoringMethod ) );
		LocalFileMng::writeXmlString( guiNode, "SongEditor_ColoringMethodAuxValue", QString::number( m_nColoringMethodAuxValue ) );

	}
	rootNode.appendChild( guiNode );

	//---- FILES ----
	QDomNode filesNode = doc.createElement( "files" );
	{
		// last used song
		LocalFileMng::writeXmlString( filesNode, "lastSongFilename", m_lastSongFilename );
		LocalFileMng::writeXmlString( filesNode, "lastPlaylistFilename", m_lastPlaylistFilename );
		LocalFileMng::writeXmlString( filesNode, "defaulteditor", m_sDefaultEditor );
	}
	rootNode.appendChild( filesNode );

	MidiMap * mM = MidiMap::get_instance();
	std::map< QString, Action* > mmcMap = mM->getMMCMap();

	//---- MidiMap ----
	QDomNode midiEventMapNode = doc.createElement( "midiEventMap" );

		std::map< QString, Action* >::iterator dIter( mmcMap.begin() );
	for( dIter = mmcMap.begin(); dIter != mmcMap.end(); dIter++ ){
		QString event = dIter->first;
		Action * pAction = dIter->second;
		if ( pAction->getType() != "NOTHING" ){
			QDomNode midiEventNode = doc.createElement( "midiEvent" );

			LocalFileMng::writeXmlString( midiEventNode, "mmcEvent" , event );
			LocalFileMng::writeXmlString( midiEventNode, "action" , pAction->getType());
			LocalFileMng::writeXmlString( midiEventNode, "parameter" , pAction->getParameter1() );

			midiEventMapNode.appendChild( midiEventNode );
		}
	}

	for( int note=0; note < 128; note++ ){
		Action * pAction = mM->getNoteAction( note );
		if( pAction != NULL && pAction->getType() != "NOTHING") {
			QDomNode midiEventNode = doc.createElement( "midiEvent" );

			LocalFileMng::writeXmlString( midiEventNode, "noteEvent" , QString("NOTE") );
			LocalFileMng::writeXmlString( midiEventNode, "eventParameter" , QString::number( note ) );
			LocalFileMng::writeXmlString( midiEventNode, "action" , pAction->getType() );
			LocalFileMng::writeXmlString( midiEventNode, "parameter" , pAction->getParameter1() );
			midiEventMapNode.appendChild( midiEventNode );
		}
	}

	for( int parameter=0; parameter < 128; parameter++ ){
		Action * pAction = mM->getCCAction( parameter );
		if( pAction != NULL && pAction->getType() != "NOTHING") {
			QDomNode midiEventNode = doc.createElement( "midiEvent" );

			LocalFileMng::writeXmlString( midiEventNode, "ccEvent" , QString("CC") );
			LocalFileMng::writeXmlString( midiEventNode, "eventParameter" , QString::number( parameter ) );
			LocalFileMng::writeXmlString( midiEventNode, "action" , pAction->getType() );
			LocalFileMng::writeXmlString( midiEventNode, "parameter" , pAction->getParameter1() );
			midiEventMapNode.appendChild( midiEventNode );
		}
	}

	{
		Action * pAction = mM->getPCAction();
		if( pAction != NULL && pAction->getType() != "NOTHING") {
			QDomNode midiEventNode = doc.createElement( "midiEvent" );

			LocalFileMng::writeXmlString( midiEventNode, "pcEvent" , QString("PROGRAM_CHANGE") );
			LocalFileMng::writeXmlString( midiEventNode, "action" , pAction->getType() );
			LocalFileMng::writeXmlString( midiEventNode, "parameter" , pAction->getParameter1() );
			midiEventMapNode.appendChild( midiEventNode );
		}
	}

	rootNode.appendChild( midiEventMapNode );

	doc.appendChild( rootNode );

	QFile file( filename );
	if ( !file.open(QIODevice::WriteOnly) )
		return;

	QTextStream TextStream( &file );
	doc.save( TextStream, 1 );

	file.close();
}



///
/// Create preferences directory
///
void Preferences::createPreferencesDirectory()
{
	QString prefDir = m_sPreferencesDirectory;
	INFOLOG( "Creating preference file directory in " + prefDir );

	QDir dir;
	dir.mkdir( prefDir );
}



///
/// Create data directory
///
void Preferences::createDataDirectory()
{
	QString sDir = m_sDataDirectory;
	INFOLOG( "Creating data directory in " + sDir );

	QDir dir;
	dir.mkdir( sDir );
//	mkdir(dir.c_str(),S_IRWXU);
}

void Preferences::createSoundLibraryDirectories()
{
	QString sDir = m_sDataDirectory;
	QString sDrumkitDir;
	QString sSongDir;
	QString sPatternDir;
	QString sPlaylistDir;
	QString sPluginsDir;

	INFOLOG( "Creating soundLibrary directories in " + sDir );

	sDrumkitDir = sDir + "/drumkits";
	sSongDir = sDir + "/songs";
	sPatternDir = sDir + "/patterns";
	sPlaylistDir = sDir + "/playlists";
	sPluginsDir = sDir + "/plugins";

	QDir dir;
	dir.mkdir( sDrumkitDir );
	dir.mkdir( sSongDir );
	dir.mkdir( sPatternDir );
	dir.mkdir( sPlaylistDir );
	dir.mkdir( sPluginsDir );
}


void Preferences::setMostRecentFX( QString FX_name )
{
	int pos = m_recentFX.indexOf( FX_name );

	if ( pos != -1 )
		m_recentFX.removeAt( pos );

	m_recentFX.push_front( FX_name );
}

void Preferences::setRecentFiles( std::vector<QString> recentFiles )
{
	// find single filenames. (skip duplicates)
	std::vector<QString> temp;
	for ( unsigned i = 0; i < recentFiles.size(); i++ ) {
		QString sFilename = recentFiles[ i ];

		bool bExists = false;
		for ( unsigned j = 0; j < temp.size(); j++ ) {
			if ( sFilename == temp[ j ] ) {
				bExists = true;
				break;
			}
		}
		if ( !bExists ) {
			temp.push_back( sFilename );
		}
	}

	m_recentFiles = temp;
}



/// Read the xml nodes related to window properties
WindowProperties Preferences::readWindowProperties( QDomNode parent, const QString& windowName, WindowProperties defaultProp )
{
	WindowProperties prop = defaultProp;

	QDomNode windowPropNode  = parent.firstChildElement( windowName );
	if ( windowPropNode.isNull() ) {
		WARNINGLOG( "Error reading configuration file: " + windowName + " node not found" );
	} else {
		prop.visible = LocalFileMng::readXmlBool( windowPropNode, "visible", true );
		prop.x = LocalFileMng::readXmlInt( windowPropNode, "x", prop.x );
		prop.y = LocalFileMng::readXmlInt( windowPropNode, "y", prop.y );
		prop.width = LocalFileMng::readXmlInt( windowPropNode, "width", prop.width );
		prop.height = LocalFileMng::readXmlInt( windowPropNode, "height", prop.height );
	}

	return prop;
}



/// Write the xml nodes related to window properties
void Preferences::writeWindowProperties( QDomNode parent, const QString& windowName, const WindowProperties& prop )
{
	QDomDocument doc;
	QDomNode windowPropNode = doc.createElement( windowName );
	if ( prop.visible ) {
		LocalFileMng::writeXmlString( windowPropNode, "visible", "true" );
	} else {
		LocalFileMng::writeXmlString( windowPropNode, "visible", "false" );
	}

	LocalFileMng::writeXmlString( windowPropNode, "x", QString("%1").arg( prop.x ) );
	LocalFileMng::writeXmlString( windowPropNode, "y", QString("%1").arg( prop.y ) );
	LocalFileMng::writeXmlString( windowPropNode, "width", QString("%1").arg( prop.width ) );
	LocalFileMng::writeXmlString( windowPropNode, "height", QString("%1").arg( prop.height ) );
	parent.appendChild( windowPropNode );
}



void Preferences::writeUIStyle( QDomNode parent )
{
	QDomDocument doc;
	QDomNode node = doc.createElement( "UI_Style" );

	// SONG EDITOR
	QDomNode songEditorNode = doc.createElement( "songEditor" );
	LocalFileMng::writeXmlString( songEditorNode, "backgroundColor", m_pDefaultUIStyle->m_songEditor_backgroundColor.toStringFmt() );
	LocalFileMng::writeXmlString( songEditorNode, "alternateRowColor", m_pDefaultUIStyle->m_songEditor_alternateRowColor.toStringFmt() );
	LocalFileMng::writeXmlString( songEditorNode, "selectedRowColor", m_pDefaultUIStyle->m_songEditor_selectedRowColor.toStringFmt() );
	LocalFileMng::writeXmlString( songEditorNode, "lineColor", m_pDefaultUIStyle->m_songEditor_lineColor.toStringFmt() );
	LocalFileMng::writeXmlString( songEditorNode, "textColor", m_pDefaultUIStyle->m_songEditor_textColor.toStringFmt() );
	LocalFileMng::writeXmlString( songEditorNode, "pattern1Color", m_pDefaultUIStyle->m_songEditor_pattern1Color.toStringFmt() );
	node.appendChild( songEditorNode );

	// PATTERN EDITOR
	QDomNode patternEditorNode = doc.createElement( "patternEditor" );
	LocalFileMng::writeXmlString( patternEditorNode, "backgroundColor", m_pDefaultUIStyle->m_patternEditor_backgroundColor.toStringFmt() );
	LocalFileMng::writeXmlString( patternEditorNode, "alternateRowColor", m_pDefaultUIStyle->m_patternEditor_alternateRowColor.toStringFmt() );
	LocalFileMng::writeXmlString( patternEditorNode, "selectedRowColor", m_pDefaultUIStyle->m_patternEditor_selectedRowColor.toStringFmt() );
	LocalFileMng::writeXmlString( patternEditorNode, "textColor", m_pDefaultUIStyle->m_patternEditor_textColor.toStringFmt() );
	LocalFileMng::writeXmlString( patternEditorNode, "noteColor", m_pDefaultUIStyle->m_patternEditor_noteColor.toStringFmt() );

	if (m_pDefaultUIStyle->m_patternEditor_noteoffColor.toStringFmt() == "-1,-1,-1" ){
		m_pDefaultUIStyle->m_patternEditor_noteoffColor = H2RGBColor( "100, 100, 200" );
	}
	LocalFileMng::writeXmlString( patternEditorNode, "noteoffColor", m_pDefaultUIStyle->m_patternEditor_noteoffColor.toStringFmt() );

	LocalFileMng::writeXmlString( patternEditorNode, "lineColor", m_pDefaultUIStyle->m_patternEditor_lineColor.toStringFmt() );
	LocalFileMng::writeXmlString( patternEditorNode, "line1Color", m_pDefaultUIStyle->m_patternEditor_line1Color.toStringFmt() );
	LocalFileMng::writeXmlString( patternEditorNode, "line2Color", m_pDefaultUIStyle->m_patternEditor_line2Color.toStringFmt() );
	LocalFileMng::writeXmlString( patternEditorNode, "line3Color", m_pDefaultUIStyle->m_patternEditor_line3Color.toStringFmt() );
	LocalFileMng::writeXmlString( patternEditorNode, "line4Color", m_pDefaultUIStyle->m_patternEditor_line4Color.toStringFmt() );
	LocalFileMng::writeXmlString( patternEditorNode, "line5Color", m_pDefaultUIStyle->m_patternEditor_line5Color.toStringFmt() );
	node.appendChild( patternEditorNode );

	parent.appendChild( node );
}



void Preferences::readUIStyle( QDomNode parent )
{
	// SONG EDITOR
	QDomNode pSongEditorNode = parent.firstChildElement( "songEditor" );
	if ( !pSongEditorNode.isNull() ) {
		m_pDefaultUIStyle->m_songEditor_backgroundColor = H2RGBColor( LocalFileMng::readXmlString( pSongEditorNode, "backgroundColor", m_pDefaultUIStyle->m_songEditor_backgroundColor.toStringFmt() ) );
		m_pDefaultUIStyle->m_songEditor_alternateRowColor = H2RGBColor( LocalFileMng::readXmlString( pSongEditorNode, "alternateRowColor", m_pDefaultUIStyle->m_songEditor_alternateRowColor.toStringFmt() ) );
		m_pDefaultUIStyle->m_songEditor_selectedRowColor = H2RGBColor( LocalFileMng::readXmlString( pSongEditorNode, "selectedRowColor", m_pDefaultUIStyle->m_songEditor_selectedRowColor.toStringFmt() ) );
		m_pDefaultUIStyle->m_songEditor_lineColor = H2RGBColor( LocalFileMng::readXmlString( pSongEditorNode, "lineColor", m_pDefaultUIStyle->m_songEditor_lineColor.toStringFmt() ) );
		m_pDefaultUIStyle->m_songEditor_textColor = H2RGBColor( LocalFileMng::readXmlString( pSongEditorNode, "textColor", m_pDefaultUIStyle->m_songEditor_textColor.toStringFmt() ) );
		m_pDefaultUIStyle->m_songEditor_pattern1Color = H2RGBColor( LocalFileMng::readXmlString( pSongEditorNode, "pattern1Color", m_pDefaultUIStyle->m_songEditor_pattern1Color.toStringFmt() ) );
	} else {
		WARNINGLOG( "songEditor node not found" );
	}

	// PATTERN EDITOR
	QDomNode pPatternEditorNode = parent.firstChildElement( "patternEditor" );
	if ( !pPatternEditorNode.isNull() ) {
		m_pDefaultUIStyle->m_patternEditor_backgroundColor = H2RGBColor( LocalFileMng::readXmlString( pPatternEditorNode, "backgroundColor", m_pDefaultUIStyle->m_patternEditor_backgroundColor.toStringFmt() ) );
		m_pDefaultUIStyle->m_patternEditor_alternateRowColor = H2RGBColor( LocalFileMng::readXmlString( pPatternEditorNode, "alternateRowColor", m_pDefaultUIStyle->m_patternEditor_alternateRowColor.toStringFmt() ) );
		m_pDefaultUIStyle->m_patternEditor_selectedRowColor = H2RGBColor( LocalFileMng::readXmlString( pPatternEditorNode, "selectedRowColor", m_pDefaultUIStyle->m_patternEditor_selectedRowColor.toStringFmt() ) );
		m_pDefaultUIStyle->m_patternEditor_textColor = H2RGBColor( LocalFileMng::readXmlString( pPatternEditorNode, "textColor", m_pDefaultUIStyle->m_patternEditor_textColor.toStringFmt() ) );
		m_pDefaultUIStyle->m_patternEditor_noteColor = H2RGBColor( LocalFileMng::readXmlString( pPatternEditorNode, "noteColor", m_pDefaultUIStyle->m_patternEditor_noteColor.toStringFmt() ) );
		m_pDefaultUIStyle->m_patternEditor_noteoffColor = H2RGBColor( LocalFileMng::readXmlString( pPatternEditorNode, "noteoffColor", m_pDefaultUIStyle->m_patternEditor_noteoffColor.toStringFmt() ) );
		m_pDefaultUIStyle->m_patternEditor_lineColor = H2RGBColor( LocalFileMng::readXmlString( pPatternEditorNode, "lineColor", m_pDefaultUIStyle->m_patternEditor_lineColor.toStringFmt() ) );
		m_pDefaultUIStyle->m_patternEditor_line1Color = H2RGBColor( LocalFileMng::readXmlString( pPatternEditorNode, "line1Color", m_pDefaultUIStyle->m_patternEditor_line1Color.toStringFmt() ) );
		m_pDefaultUIStyle->m_patternEditor_line2Color = H2RGBColor( LocalFileMng::readXmlString( pPatternEditorNode, "line2Color", m_pDefaultUIStyle->m_patternEditor_line2Color.toStringFmt() ) );
		m_pDefaultUIStyle->m_patternEditor_line3Color = H2RGBColor( LocalFileMng::readXmlString( pPatternEditorNode, "line3Color", m_pDefaultUIStyle->m_patternEditor_line3Color.toStringFmt() ) );
		m_pDefaultUIStyle->m_patternEditor_line4Color = H2RGBColor( LocalFileMng::readXmlString( pPatternEditorNode, "line4Color", m_pDefaultUIStyle->m_patternEditor_line4Color.toStringFmt() ) );
		m_pDefaultUIStyle->m_patternEditor_line5Color = H2RGBColor( LocalFileMng::readXmlString( pPatternEditorNode, "line5Color", m_pDefaultUIStyle->m_patternEditor_line5Color.toStringFmt() ) );
	} else {
		WARNINGLOG( "patternEditor node not found" );
	}
}


// -----------------------



const char* WindowProperties::__class_name = "WindowProperties";

WindowProperties::WindowProperties()
		: Object( __class_name )
{
//	infoLog( "INIT" );
	x = 0;
	y = 0;
	width = 0;
	height = 0;
	visible = true;
}



WindowProperties::~WindowProperties()
{
//	infoLog( "DESTROY" );
}




// :::::::::::::::::::::::::::::::


const char* UIStyle::__class_name = "UIStyle";

UIStyle::UIStyle()
		: Object( __class_name )
{
//	infoLog( "INIT" );
}



// ::::::::::::::::::::::::::::::::::::::


const char* H2RGBColor::__class_name = "H2RGBColor";

H2RGBColor::H2RGBColor( int r, int g, int b )
		: Object( __class_name )
		, m_red( r )
		, m_green( g )
		, m_blue( b )
{
//	infoLog( "INIT" );
	m_red %= 256;
	m_green %= 256;
	m_blue %= 256;
}



H2RGBColor::~H2RGBColor()
{
//	infoLog( "DESTROY" );
}


H2RGBColor::H2RGBColor( const QString& sColor )
		: Object( __class_name )
{
//	infoLog( "INIT " + sColor );
	QString temp = sColor;

	QStringList list = temp.split(",");
	m_red = list[0].toInt();
	m_green = list[1].toInt();
	m_blue = list[2].toInt();

	m_red %= 256;
	m_green %= 256;
	m_blue %= 256;
}



QString H2RGBColor::toStringFmt()
{
	char tmp[255];
	sprintf( tmp, "%d,%d,%d", m_red, m_green, m_blue );

	return QString( tmp );
}

};
