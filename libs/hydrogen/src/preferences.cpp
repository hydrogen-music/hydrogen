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
#include <hydrogen/midiMap.h>

#include <hydrogen/data_path.h>
#include "config.h"
#include "version.h"

#include "xml/tinyxml.h"
#include <QDir>
#include <QApplication>

namespace H2Core
{

Preferences* Preferences::__instance = NULL;

void Preferences::create_instance()
{
	if ( __instance == 0 ) {
		__instance = new Preferences;
	}
}


Preferences::Preferences()
		: Object( "Preferences" )
		, demoPath( QString( DataPath::get_data_path() ) + "/demo_songs/" )
		, m_sLastNews( "" )
{
	__instance = this;
	INFOLOG( "INIT" );
	
	//Default jack track-outputs are post fader
	m_nJackTrackOutputMode = POST_FADER;
	m_bJackTrackOuts = false;
	// switch to enable / disable lash, only on h2 startup
	m_brestartLash = false;
	m_bsetLash = false;

	//server list
	std::list<QString> sServerList;

//	//musicCategories
//	std::list<QString> m_musicCategories;

	char * ladpath = getenv( "LADSPA_PATH" );	// read the Environment variable LADSPA_PATH
	if ( ladpath ) {
		INFOLOG( "Found LADSPA_PATH enviroment variable" );
		QString sLadspaPath = ladpath;
		int pos;
		while ( ( pos = sLadspaPath.indexOf( ":" ) ) != -1 ) {
			QString sPath = sLadspaPath.left( pos );
			m_ladspaPathVect.push_back( sPath );
			sLadspaPath = sLadspaPath.mid( pos + 1, sLadspaPath.length() );
		}
		m_ladspaPathVect.push_back( sLadspaPath );
	} else {
#ifdef Q_OS_MACX
		m_ladspaPathVect.push_back( qApp->applicationDirPath() + "/../Resources/plugins" );
		m_ladspaPathVect.push_back( "/Library/Audio/Plug-Ins/LADSPA/" );
		m_ladspaPathVect.push_back( QDir::homePath().append( "/Library/Audio/Plug-Ins/LADSPA" ));
#else
		m_ladspaPathVect.push_back( "/usr/lib/ladspa" );
		m_ladspaPathVect.push_back( "/usr/local/lib/ladspa" );
#endif

	}

	


	m_pDefaultUIStyle = new UIStyle();

#ifdef Q_OS_MACX
	m_sPreferencesFilename = QDir::homePath().append( "/Library/Application Support/Hydrogen/hydrogen.conf" );
	m_sPreferencesDirectory = QDir::homePath().append( "/Library/Application Support/Hydrogen/" );
	m_sDataDirectory = QDir::homePath().append( "/Library/Application Support/Hydrogen/data/" );
#else
	m_sPreferencesFilename = QDir::homePath().append( "/.hydrogen/hydrogen.conf" );
	m_sPreferencesDirectory = QDir::homePath().append( "/.hydrogen/" );
	m_sDataDirectory = QDir::homePath().append( "/.hydrogen/data/" );
#endif

	__lastspatternDirectory = QDir::homePath();
	__lastsampleDirectory = QDir::homePath(); //audio file browser
	__playsamplesonclicking = false; // audio file browser

	__expandSongItem = true; //SoundLibraryPanel
	__expandPatternItem = true; //SoundLibraryPanel
 

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

	// None: m_sDefaultEditor;
	// SEE ABOVE: m_sDataDirectory
	// SEE ABOVE: demoPath

	//___ General properties ___
	m_bPatternModePlaysSelected = true;
	restoreLastSong = true;
	m_bUseLash = false;
	m_bShowDevelWarning = false;
	// NONE: lastSongFilename;
	hearNewNotes = true;
	// NONE: m_recentFiles;
	// NONE: m_recentFX;
	// NONE: m_ladspaPathVect;
	quantizeEvents = true;
	recordEvents = false;
	m_sLastNews = QString("-");

	//___ GUI properties ___
	m_sQTStyle = "Plastique";
	applicationFontFamily = "Lucida Grande";
	applicationFontPointSize = 10;
	mixerFontFamily = "Lucida Grande";
	mixerFontPointSize = 11;
	mixerFalloffSpeed = 1.1;
	m_nPatternEditorGridResolution = 8;
	m_bPatternEditorUsingTriplets = false;
	m_bShowInstrumentPeaks = true;
	m_bIsFXTabVisible = true;
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
		sPreferencesDirectory = DataPath::get_data_path();
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
	std::ifstream input( sPreferencesFilename.toAscii() , std::ios::in | std::ios::binary );
	if ( input ) {
		// read preferences file
		TiXmlDocument doc( sPreferencesFilename.toAscii() );
		doc.LoadFile();

		TiXmlNode* rootNode;
		if ( ( rootNode = doc.FirstChild( "hydrogen_preferences" ) ) ) {

			// version
			QString version = LocalFileMng::readXmlString( rootNode, "version", "" );
			if ( version == "" ) {
				recreate = true;
			}

			//////// GENERAL ///////////
			//m_sLadspaPath = LocalFileMng::readXmlString( this, rootNode, "ladspaPath", m_sLadspaPath );
			m_bShowDevelWarning = LocalFileMng::readXmlBool( rootNode, "showDevelWarning", m_bShowDevelWarning );
			restoreLastSong = LocalFileMng::readXmlBool( rootNode, "restoreLastSong", restoreLastSong );
			m_bPatternModePlaysSelected = LocalFileMng::readXmlBool( rootNode, "patternModePlaysSelected", TRUE );
			m_bUseLash = LocalFileMng::readXmlBool( rootNode, "useLash", FALSE );
			
			//restore the right m_bsetlash value
			m_bsetLash = m_bUseLash;

			hearNewNotes = LocalFileMng::readXmlBool( rootNode, "hearNewNotes", hearNewNotes );
			recordEvents = LocalFileMng::readXmlBool( rootNode, "recordEvents", recordEvents );
			quantizeEvents = LocalFileMng::readXmlBool( rootNode, "quantizeEvents", quantizeEvents );

			TiXmlNode* pRecentUsedSongsNode = rootNode->FirstChild( "recentUsedSongs" );
			if ( pRecentUsedSongsNode ) {
				TiXmlNode* pSongNode = 0;
				for ( pSongNode = pRecentUsedSongsNode->FirstChild( "song" ); pSongNode; pSongNode = pSongNode->NextSibling( "song" ) ) {
					QString sFilename = pSongNode->FirstChild()->Value();
					m_recentFiles.push_back( sFilename );
				}
			} else {
				WARNINGLOG( "recentUsedSongs node not found" );
			}

			TiXmlNode* pRecentFXNode = rootNode->FirstChild( "recentlyUsedEffects" );
			if ( pRecentFXNode ) {
				TiXmlNode* pFXNode = 0;
				for ( pFXNode = pRecentFXNode->FirstChild( "FX" ); pFXNode; pFXNode = pFXNode->NextSibling( "FX" ) ) {
					QString sFXName = pFXNode->FirstChild()->Value();
					m_recentFX.push_back( sFXName );
				}
			} else {
				WARNINGLOG( "recentlyUsedEffects node not found" );
			}

			sServerList.clear();
			TiXmlNode* pServerListNode = rootNode->FirstChild( "serverList" );
			if ( pServerListNode ) {
				TiXmlNode* pServerNode = 0;
				for ( pServerNode = pServerListNode->FirstChild( "server" ); pServerNode; pServerNode = pServerNode->NextSibling( "server" ) ) {
					QString sFilename = pServerNode->FirstChild()->Value();
					sServerList.push_back( sFilename );
				}
			} else {
				WARNINGLOG( "serverList node not found" );
			}

			m_patternCategories.clear();
			TiXmlNode* pPatternCategoriesNode = rootNode->FirstChild( "patternCategories" );
			if ( pPatternCategoriesNode ) {
				TiXmlNode* pCategoriesNode = 0;
				for ( pCategoriesNode = pPatternCategoriesNode->FirstChild( "categories" ); pCategoriesNode; pCategoriesNode = pCategoriesNode->NextSibling( "categories" ) ) {
					QString sFilename = pCategoriesNode->FirstChild()->Value();
					m_patternCategories.push_back( sFilename );
				}
			} else {
				WARNINGLOG( "patternCategories node not found" );
			}



			m_sLastNews = LocalFileMng::readXmlString( rootNode, "lastNews", "-", true );

			/////////////// AUDIO ENGINE //////////////
			TiXmlNode* audioEngineNode;
			if ( !( audioEngineNode = rootNode->FirstChild( "audio_engine" ) ) ) {
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
				TiXmlNode* ossDriverNode;
				if ( !( ossDriverNode = audioEngineNode->FirstChild( "oss_driver" ) ) ) {
					WARNINGLOG( "oss_driver node not found" );
					recreate = true;
				} else {
					m_sOSSDevice = LocalFileMng::readXmlString( ossDriverNode, "ossDevice", m_sOSSDevice );
				}

				//// JACK DRIVER ////
				TiXmlNode* jackDriverNode;
				if ( !( jackDriverNode = audioEngineNode->FirstChild( "jack_driver" ) ) ) {
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
				TiXmlNode* alsaAudioDriverNode;
				if ( !( alsaAudioDriverNode = audioEngineNode->FirstChild( "alsa_audio_driver" ) ) ) {
					WARNINGLOG( "alsa_audio_driver node not found" );
					recreate = true;
				} else {
					m_sAlsaAudioDevice = LocalFileMng::readXmlString( alsaAudioDriverNode, "alsa_audio_device", m_sAlsaAudioDevice );
				}

				/// MIDI DRIVER ///
				TiXmlNode* midiDriverNode;
				if ( !( midiDriverNode = audioEngineNode->FirstChild( "midi_driver" ) ) ) {
					WARNINGLOG( "midi_driver node not found" );
					recreate = true;
				} else {
					m_sMidiDriver = LocalFileMng::readXmlString( midiDriverNode, "driverName", "ALSA" );
					m_sMidiPortName = LocalFileMng::readXmlString( midiDriverNode, "port_name", "None" );
					m_nMidiChannelFilter = LocalFileMng::readXmlInt( midiDriverNode, "channel_filter", -1 );
					m_bMidiNoteOffIgnore = LocalFileMng::readXmlBool( midiDriverNode, "ignore_note_off", true );
				}



			}

			/////////////// GUI //////////////
			TiXmlNode* guiNode;
			if ( !( guiNode = rootNode->FirstChild( "gui" ) ) ) {
				WARNINGLOG( "gui node not found" );
				recreate = true;
			} else {
				// QT Style
				m_sQTStyle = LocalFileMng::readXmlString( guiNode, "QTStyle", m_sQTStyle, true );

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

				TiXmlNode *pUIStyle = guiNode->FirstChild( "UI_Style" );
				if ( pUIStyle ) {
					readUIStyle( *pUIStyle );
				} else {
					WARNINGLOG( "UI_Style node not found" );
					recreate = true;
				}
			}

			/////////////// FILES //////////////
			TiXmlNode* filesNode;
			if ( !( filesNode = rootNode->FirstChild( "files" ) ) ) {
				WARNINGLOG( "files node not found" );
				recreate = true;
			} else {
				// last used song
				lastSongFilename = LocalFileMng::readXmlString( filesNode, "lastSongFilename", lastSongFilename, true );
				m_sDefaultEditor = LocalFileMng::readXmlString( filesNode, "defaulteditor", m_sDefaultEditor, true );
			}

			MidiMap::reset_instance();
			MidiMap* mM = MidiMap::get_instance();
			
			
			TiXmlNode* pMidiEventMapNode = rootNode->FirstChild( "midiEventMap" );
			if ( pMidiEventMapNode ) {
				TiXmlNode* pMidiEventNode = 0;
				
				for ( pMidiEventNode = pMidiEventMapNode->FirstChild( "midiEvent" ); pMidiEventNode; pMidiEventNode = pMidiEventNode->NextSibling( "midiEvent" ) ) {
					
					if( pMidiEventNode->FirstChild()->Value() == QString("mmcEvent")){
						QString event = pMidiEventNode->FirstChild("mmcEvent")->FirstChild()->Value();
						QString s_action = pMidiEventNode->FirstChild("action")->FirstChild()->Value();
						QString s_param = pMidiEventNode->FirstChild("parameter")->FirstChild()->Value();
						Action* pAction = new Action( s_action );
						pAction->setParameter1( s_param );
						mM->registerMMCEvent(event, pAction);
					}

					if( pMidiEventNode->FirstChild()->Value() == QString("noteEvent")){
						QString event = pMidiEventNode->FirstChild("noteEvent")->FirstChild()->Value();
						QString s_action = pMidiEventNode->FirstChild("action")->FirstChild()->Value();
						QString s_param = pMidiEventNode->FirstChild("parameter")->FirstChild()->Value();
						QString s_eventParameter = pMidiEventNode->FirstChild("eventParameter")->FirstChild()->Value();
						Action* pAction = new Action( s_action );
						pAction->setParameter1( s_param );
						mM->registerNoteEvent(s_eventParameter.toInt(), pAction);
					}

					if( pMidiEventNode->FirstChild()->Value() == QString("ccEvent") ){
						QString event = pMidiEventNode->FirstChild("ccEvent")->FirstChild()->Value();
						QString s_action = pMidiEventNode->FirstChild("action")->FirstChild()->Value();
						QString s_param = pMidiEventNode->FirstChild("parameter")->FirstChild()->Value();
						QString s_eventParameter = pMidiEventNode->FirstChild("eventParameter")->FirstChild()->Value();
						Action * pAction = new Action( s_action );
						pAction->setParameter1( s_param );
						mM->registerCCEvent( s_eventParameter.toInt(), pAction );
					}

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
	if ( recreate == true ) {
		WARNINGLOG( "Recreating configuration file." );
		savePreferences();
	}

}



///
/// Save the preferences file
///
void Preferences::savePreferences()
{
	//string prefDir = QDir::homePath().append("/.hydrogen").toStdString();
	QString filename = m_sPreferencesFilename;

	INFOLOG( "Saving preferences file: " + filename );

	TiXmlDocument doc( filename.toAscii() );

	TiXmlElement rootNode( "hydrogen_preferences" );

	// hydrogen version
	LocalFileMng::writeXmlString( &rootNode, "version", QString( get_version().c_str() ) );

	////// GENERAL ///////
	LocalFileMng::writeXmlString( &rootNode, "restoreLastSong", restoreLastSong ? "true": "false" );
	
	LocalFileMng::writeXmlString( &rootNode, "patternModePlaysSelected", m_bPatternModePlaysSelected ? "true": "false" );

	LocalFileMng::writeXmlString( &rootNode, "useLash", m_bsetLash ? "true": "false" );

	//show development version warning
	LocalFileMng::writeXmlString( &rootNode, "showDevelWarning", m_bShowDevelWarning ? "true": "false" );

	// hear new notes in the pattern editor
	LocalFileMng::writeXmlString( &rootNode, "hearNewNotes", hearNewNotes ? "true": "false" );

	// key/midi event prefs
	LocalFileMng::writeXmlString( &rootNode, "recordEvents", recordEvents ? "true": "false" );
	LocalFileMng::writeXmlString( &rootNode, "quantizeEvents", quantizeEvents ? "true": "false" );

	// Recent used songs
	TiXmlElement recentUsedSongsNode( "recentUsedSongs" );
	{
		unsigned nSongs = 5;
		if ( m_recentFiles.size() < 5 ) {
			nSongs = m_recentFiles.size();
		}
		for ( unsigned i = 0; i < nSongs; i++ ) {
			LocalFileMng::writeXmlString( &recentUsedSongsNode, "song", m_recentFiles[ i ] );
		}
	}
	rootNode.InsertEndChild( recentUsedSongsNode );
	
	TiXmlElement recentFXNode( "recentlyUsedEffects" );
	{
		int nFX = 0;
		QString FXname;
		foreach( FXname, m_recentFX ) {
			LocalFileMng::writeXmlString( &recentFXNode, "FX", FXname );
			if ( ++nFX > 10 ) break;
		}
	}
	rootNode.InsertEndChild( recentFXNode );


	std::list<QString>::const_iterator cur_Server;

	TiXmlElement serverListNode( "serverList" );
	for( cur_Server = sServerList.begin(); cur_Server != sServerList.end(); ++cur_Server ){
		LocalFileMng::writeXmlString( &serverListNode , QString("server") , QString( *cur_Server ) );
	}
	rootNode.InsertEndChild( serverListNode );


	std::list<QString>::const_iterator cur_patternCategories;

	TiXmlElement patternCategoriesNode( "patternCategories" );
	for( cur_patternCategories = m_patternCategories.begin(); cur_patternCategories != m_patternCategories.end(); ++cur_patternCategories ){
		LocalFileMng::writeXmlString( &patternCategoriesNode , QString("categories") , QString( *cur_patternCategories ) );
	}
	rootNode.InsertEndChild( patternCategoriesNode );




	LocalFileMng::writeXmlString( &rootNode, "lastNews", m_sLastNews );


	//---- AUDIO ENGINE ----
	TiXmlElement audioEngineNode( "audio_engine" );
	{
		// audio driver
		LocalFileMng::writeXmlString( &audioEngineNode, "audio_driver", m_sAudioDriver );

		// use metronome
		LocalFileMng::writeXmlString( &audioEngineNode, "use_metronome", m_bUseMetronome ? "true": "false" );
		LocalFileMng::writeXmlString( &audioEngineNode, "metronome_volume", QString("%1").arg( m_fMetronomeVolume ) );
		LocalFileMng::writeXmlString( &audioEngineNode, "maxNotes", QString("%1").arg( m_nMaxNotes ) );
		LocalFileMng::writeXmlString( &audioEngineNode, "buffer_size", QString("%1").arg( m_nBufferSize ) );
		LocalFileMng::writeXmlString( &audioEngineNode, "samplerate", QString("%1").arg( m_nSampleRate ) );

		//// OSS DRIVER ////
		TiXmlElement ossDriverNode( "oss_driver" );
		{
			LocalFileMng::writeXmlString( &ossDriverNode, "ossDevice", m_sOSSDevice );
		}
		audioEngineNode.InsertEndChild( ossDriverNode );

		//// JACK DRIVER ////
		TiXmlElement jackDriverNode( "jack_driver" );
		{
			LocalFileMng::writeXmlString( &jackDriverNode, "jack_port_name_1", m_sJackPortName1 );	// jack port name 1
			LocalFileMng::writeXmlString( &jackDriverNode, "jack_port_name_2", m_sJackPortName2 );	// jack port name 2

			// jack transport slave
			QString sMode;
			if ( m_bJackTransportMode == NO_JACK_TRANSPORT ) {
				sMode = "NO_JACK_TRANSPORT";
			} else if ( m_bJackTransportMode == USE_JACK_TRANSPORT ) {
				sMode = "USE_JACK_TRANSPORT";
			}
			LocalFileMng::writeXmlString( &jackDriverNode, "jack_transport_mode", sMode );

			//jack time master
			QString tmMode;
			if ( m_bJackMasterMode == NO_JACK_TIME_MASTER ) {
				tmMode = "NO_JACK_TIME_MASTER";
			} else if (  m_bJackMasterMode == USE_JACK_TIME_MASTER ) {
				tmMode = "NO_JACK_TIME_MASTER";
			}
			LocalFileMng::writeXmlString( &jackDriverNode, "jack_transport_mode_master", tmMode );
			//~ jack time master

			// jack default connection
			QString jackConnectDefaultsString = "false";
			if ( m_bJackConnectDefaults ) {
				jackConnectDefaultsString = "true";
			}
			LocalFileMng::writeXmlString( &jackDriverNode, "jack_connect_defaults", jackConnectDefaultsString );

			//pre-fader or post-fader track outputs ?
			LocalFileMng::writeXmlString( &jackDriverNode, "jack_track_output_mode", QString("%1").arg( m_nJackTrackOutputMode ));

			// jack track outs
			QString jackTrackOutsString = "false";
			if ( m_bJackTrackOuts ) {
				jackTrackOutsString = "true";
			}
			LocalFileMng::writeXmlString( &jackDriverNode, "jack_track_outs", jackTrackOutsString );
		}
		audioEngineNode.InsertEndChild( jackDriverNode );

		//// ALSA AUDIO DRIVER ////
		TiXmlElement alsaAudioDriverNode( "alsa_audio_driver" );
		{
			LocalFileMng::writeXmlString( &alsaAudioDriverNode, "alsa_audio_device", m_sAlsaAudioDevice );
		}
		audioEngineNode.InsertEndChild( alsaAudioDriverNode );

		/// MIDI DRIVER ///
		TiXmlElement midiDriverNode( "midi_driver" );
		{
			LocalFileMng::writeXmlString( &midiDriverNode, "driverName", m_sMidiDriver );
			LocalFileMng::writeXmlString( &midiDriverNode, "port_name", m_sMidiPortName );
			LocalFileMng::writeXmlString( &midiDriverNode, "channel_filter", QString("%1").arg( m_nMidiChannelFilter ) );

			if ( m_bMidiNoteOffIgnore ) {
				LocalFileMng::writeXmlString( &midiDriverNode, "ignore_note_off", "true" );
			} else {
				LocalFileMng::writeXmlString( &midiDriverNode, "ignore_note_off", "false" );
			}

			if ( m_bMidiDiscardNoteAfterAction ) {
				LocalFileMng::writeXmlString( &midiDriverNode, "discard_note_after_action", "true" );
			} else {
				LocalFileMng::writeXmlString( &midiDriverNode, "discard_note_after_action", "false" );
			}
		}
		audioEngineNode.InsertEndChild( midiDriverNode );



	}
	rootNode.InsertEndChild( audioEngineNode );

	//---- GUI ----
	TiXmlElement guiNode( "gui" );
	{
		LocalFileMng::writeXmlString( &guiNode, "QTStyle", m_sQTStyle );
		LocalFileMng::writeXmlString( &guiNode, "application_font_family", applicationFontFamily );
		LocalFileMng::writeXmlString( &guiNode, "application_font_pointsize", QString("%1").arg( applicationFontPointSize ) );
		LocalFileMng::writeXmlString( &guiNode, "mixer_font_family", mixerFontFamily );
		LocalFileMng::writeXmlString( &guiNode, "mixer_font_pointsize", QString("%1").arg( mixerFontPointSize ) );
		LocalFileMng::writeXmlString( &guiNode, "mixer_falloff_speed", QString("%1").arg( mixerFalloffSpeed ) );
		LocalFileMng::writeXmlString( &guiNode, "patternEditorGridResolution", QString("%1").arg( m_nPatternEditorGridResolution ) );
		LocalFileMng::writeXmlString( &guiNode, "patternEditorGridHeight", QString("%1").arg( m_nPatternEditorGridHeight ) );
		LocalFileMng::writeXmlString( &guiNode, "patternEditorGridWidth", QString("%1").arg( m_nPatternEditorGridWidth ) );
		LocalFileMng::writeXmlBool( &guiNode, "patternEditorUsingTriplets", m_bPatternEditorUsingTriplets );
		LocalFileMng::writeXmlBool( &guiNode, "showInstrumentPeaks", m_bShowInstrumentPeaks );
		LocalFileMng::writeXmlBool( &guiNode, "isFXTabVisible", m_bIsFXTabVisible );


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

		LocalFileMng::writeXmlBool( &guiNode, "followPlayhead", m_bFollowPlayhead );


		//beatcounter
		QString bcMode;
			if ( m_bbc == BC_OFF ) {
				bcMode = "BC_OFF";
			} else if ( m_bbc  == BC_ON ) {
				bcMode = "BC_ON";
			}
			LocalFileMng::writeXmlString( &guiNode, "bc", bcMode );


		
		QString setPlay;
			if ( m_mmcsetplay == SET_PLAY_OFF ) {
				setPlay = "SET_PLAY_OFF";
			} else if ( m_mmcsetplay == SET_PLAY_ON ) {
				setPlay = "SET_PLAY_ON";
			}
			LocalFileMng::writeXmlString( &guiNode, "setplay", setPlay );

			LocalFileMng::writeXmlString( &guiNode, "countoffset", QString("%1").arg(m_countOffset) );
			LocalFileMng::writeXmlString( &guiNode, "playoffset", QString("%1").arg(m_startOffset) );
		//~ beatcounter

		//SoundLibraryPanel expand items
		LocalFileMng::writeXmlString( &guiNode, "expandSongItem", __expandSongItem ? "true": "false" );
		LocalFileMng::writeXmlString( &guiNode, "expandPatternItem", __expandPatternItem ? "true": "false" );

		// User interface style
		writeUIStyle( guiNode );
	}
	rootNode.InsertEndChild( guiNode );

	//---- FILES ----
	TiXmlElement filesNode( "files" );
	{
		// last used song
		LocalFileMng::writeXmlString( &filesNode, "lastSongFilename", lastSongFilename );
		LocalFileMng::writeXmlString( &filesNode, "defaulteditor", m_sDefaultEditor );
	}
	rootNode.InsertEndChild( filesNode );

	MidiMap * mM = MidiMap::get_instance();
	std::map< QString, Action* > mmcMap = mM->getMMCMap();

	//---- MidiMap ----
	TiXmlElement midiEventMapNode( "midiEventMap" );
	{
		std::map< QString, Action* >::iterator dIter( mmcMap.begin() );
		for( dIter = mmcMap.begin(); dIter != mmcMap.end(); dIter++ ){
			
			QString event = dIter->first;
			Action * pAction = dIter->second;

			if ( pAction->getType() != "NOTHING" ){
				TiXmlElement midiEventNode( "midiEvent" );

				LocalFileMng::writeXmlString( &midiEventNode, "mmcEvent" , event );
				LocalFileMng::writeXmlString( &midiEventNode, "action" , pAction->getType());
				LocalFileMng::writeXmlString( &midiEventNode, "parameter" , pAction->getParameter1() );

				midiEventMapNode.InsertEndChild(midiEventNode);
			}
		}
		
		for( int note=0; note < 128; note++ ){
			Action * pAction = mM->getNoteAction( note );
			if( pAction != NULL && pAction->getType() != "NOTHING") {
				TiXmlElement midiEventNode( "midiEvent" );
				
				LocalFileMng::writeXmlString( &midiEventNode, "noteEvent" , QString("NOTE") );
				LocalFileMng::writeXmlString( &midiEventNode, "eventParameter" , QString::number( note ) );
				LocalFileMng::writeXmlString( &midiEventNode, "action" , pAction->getType() );
				LocalFileMng::writeXmlString( &midiEventNode, "parameter" , pAction->getParameter1() );
	

				midiEventMapNode.InsertEndChild(midiEventNode);
			}
		}

		for( int parameter=0; parameter < 128; parameter++ ){
			Action * pAction = mM->getCCAction( parameter );
			if( pAction != NULL && pAction->getType() != "NOTHING")
			{
				TiXmlElement midiEventNode( "midiEvent" );
				
				LocalFileMng::writeXmlString( &midiEventNode, "ccEvent" , QString("CC") );
				LocalFileMng::writeXmlString( &midiEventNode, "eventParameter" , QString::number( parameter ) );
				

				LocalFileMng::writeXmlString( &midiEventNode, "action" , pAction->getType() );

				LocalFileMng::writeXmlString( &midiEventNode, "parameter" , pAction->getParameter1() );

				midiEventMapNode.InsertEndChild(midiEventNode);
			}
		}
	}
	
	rootNode.InsertEndChild( midiEventMapNode );

	doc.InsertEndChild( rootNode );
	doc.SaveFile();
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

	INFOLOG( "Creating soundLibrary directories in " + sDir );
	
	sDrumkitDir = sDir + "/drumkits";
	sSongDir = sDir + "/songs";
	sPatternDir = sDir + "/patterns";
	sPlaylistDir = sDir + "/playlists";

	QDir dir;
	dir.mkdir( sDrumkitDir );
	dir.mkdir( sSongDir );
	dir.mkdir( sPatternDir );
	dir.mkdir( sPlaylistDir );
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
WindowProperties Preferences::readWindowProperties( TiXmlNode *parent, const QString& windowName, WindowProperties defaultProp )
{
	WindowProperties prop = defaultProp;

	TiXmlNode* windowPropNode;
	if ( !( windowPropNode = parent->FirstChild( windowName.toAscii() ) ) ) {
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
void Preferences::writeWindowProperties( TiXmlNode& parent, const QString& windowName, const WindowProperties& prop )
{
	TiXmlElement windowPropNode( windowName.toAscii() );
	if ( prop.visible ) {
		LocalFileMng::writeXmlString( &windowPropNode, "visible", "true" );
	} else {
		LocalFileMng::writeXmlString( &windowPropNode, "visible", "false" );
	}

	LocalFileMng::writeXmlString( &windowPropNode, "x", QString("%1").arg( prop.x ) );
	LocalFileMng::writeXmlString( &windowPropNode, "y", QString("%1").arg( prop.y ) );
	LocalFileMng::writeXmlString( &windowPropNode, "width", QString("%1").arg( prop.width ) );
	LocalFileMng::writeXmlString( &windowPropNode, "height", QString("%1").arg( prop.height ) );
	parent.InsertEndChild( windowPropNode );
}



void Preferences::writeUIStyle( TiXmlNode& parent )
{
	TiXmlElement node( "UI_Style" );

	// SONG EDITOR
	TiXmlElement songEditorNode( "songEditor" );
	LocalFileMng::writeXmlString( &songEditorNode, "backgroundColor", m_pDefaultUIStyle->m_songEditor_backgroundColor.toStringFmt() );
	LocalFileMng::writeXmlString( &songEditorNode, "alternateRowColor", m_pDefaultUIStyle->m_songEditor_alternateRowColor.toStringFmt() );
	LocalFileMng::writeXmlString( &songEditorNode, "selectedRowColor", m_pDefaultUIStyle->m_songEditor_selectedRowColor.toStringFmt() );
	LocalFileMng::writeXmlString( &songEditorNode, "lineColor", m_pDefaultUIStyle->m_songEditor_lineColor.toStringFmt() );
	LocalFileMng::writeXmlString( &songEditorNode, "textColor", m_pDefaultUIStyle->m_songEditor_textColor.toStringFmt() );
	LocalFileMng::writeXmlString( &songEditorNode, "pattern1Color", m_pDefaultUIStyle->m_songEditor_pattern1Color.toStringFmt() );
	node.InsertEndChild( songEditorNode );

	// PATTERN EDITOR
	TiXmlElement patternEditorNode( "patternEditor" );
	LocalFileMng::writeXmlString( &patternEditorNode, "backgroundColor", m_pDefaultUIStyle->m_patternEditor_backgroundColor.toStringFmt() );
	LocalFileMng::writeXmlString( &patternEditorNode, "alternateRowColor", m_pDefaultUIStyle->m_patternEditor_alternateRowColor.toStringFmt() );
	LocalFileMng::writeXmlString( &patternEditorNode, "selectedRowColor", m_pDefaultUIStyle->m_patternEditor_selectedRowColor.toStringFmt() );
	LocalFileMng::writeXmlString( &patternEditorNode, "textColor", m_pDefaultUIStyle->m_patternEditor_textColor.toStringFmt() );
	LocalFileMng::writeXmlString( &patternEditorNode, "noteColor", m_pDefaultUIStyle->m_patternEditor_noteColor.toStringFmt() );
	LocalFileMng::writeXmlString( &patternEditorNode, "lineColor", m_pDefaultUIStyle->m_patternEditor_lineColor.toStringFmt() );
	LocalFileMng::writeXmlString( &patternEditorNode, "line1Color", m_pDefaultUIStyle->m_patternEditor_line1Color.toStringFmt() );
	LocalFileMng::writeXmlString( &patternEditorNode, "line2Color", m_pDefaultUIStyle->m_patternEditor_line2Color.toStringFmt() );
	LocalFileMng::writeXmlString( &patternEditorNode, "line3Color", m_pDefaultUIStyle->m_patternEditor_line3Color.toStringFmt() );
	LocalFileMng::writeXmlString( &patternEditorNode, "line4Color", m_pDefaultUIStyle->m_patternEditor_line4Color.toStringFmt() );
	LocalFileMng::writeXmlString( &patternEditorNode, "line5Color", m_pDefaultUIStyle->m_patternEditor_line5Color.toStringFmt() );
	node.InsertEndChild( patternEditorNode );

	parent.InsertEndChild( node );
}



void Preferences::readUIStyle( TiXmlNode& parent )
{
	// SONG EDITOR
	TiXmlNode* pSongEditorNode = parent.FirstChild( "songEditor" );
	if ( pSongEditorNode ) {
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
	TiXmlNode* pPatternEditorNode = parent.FirstChild( "patternEditor" );
	if ( pPatternEditorNode ) {
		m_pDefaultUIStyle->m_patternEditor_backgroundColor = H2RGBColor( LocalFileMng::readXmlString( pPatternEditorNode, "backgroundColor", m_pDefaultUIStyle->m_patternEditor_backgroundColor.toStringFmt() ) );
		m_pDefaultUIStyle->m_patternEditor_alternateRowColor = H2RGBColor( LocalFileMng::readXmlString( pPatternEditorNode, "alternateRowColor", m_pDefaultUIStyle->m_patternEditor_alternateRowColor.toStringFmt() ) );
		m_pDefaultUIStyle->m_patternEditor_selectedRowColor = H2RGBColor( LocalFileMng::readXmlString( pPatternEditorNode, "selectedRowColor", m_pDefaultUIStyle->m_patternEditor_selectedRowColor.toStringFmt() ) );
		m_pDefaultUIStyle->m_patternEditor_textColor = H2RGBColor( LocalFileMng::readXmlString( pPatternEditorNode, "textColor", m_pDefaultUIStyle->m_patternEditor_textColor.toStringFmt() ) );
		m_pDefaultUIStyle->m_patternEditor_noteColor = H2RGBColor( LocalFileMng::readXmlString( pPatternEditorNode, "noteColor", m_pDefaultUIStyle->m_patternEditor_noteColor.toStringFmt() ) );
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




WindowProperties::WindowProperties()
		: Object( "WindowProperties" )
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



UIStyle::UIStyle()
		: Object( "UIStyle" )
{
//	infoLog( "INIT" );
}



// ::::::::::::::::::::::::::::::::::::::



H2RGBColor::H2RGBColor( int r, int g, int b )
		: Object( "H2RGBColor" )
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
		: Object( "H2RGBColor" )
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

/*
	int nPos = temp.indexOf( ',' );
	QString sRed = temp.substr( 0, nPos );
	temp.erase( 0, nPos + 1 );

	nPos = temp.find( ',' );
	QString sGreen = temp.substr( 0, nPos );
	temp.erase( 0, nPos + 1 );

	nPos = temp.find( ',' );
	QString sBlue = temp.substr( 0, nPos );

	m_red = atoi( sRed.c_str() );
	m_green = atoi( sGreen.c_str() );
	m_blue = atoi( sBlue.c_str() );
*/
}



QString H2RGBColor::toStringFmt()
{
	char tmp[255];
	sprintf( tmp, "%d,%d,%d", m_red, m_green, m_blue );

	//string sRes = to_string( m_red ) + "," + to_string( m_green ) + "," + to_string( m_blue );
//	return sRes;

	return QString( tmp );
}

};
