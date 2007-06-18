/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: Preferences.cpp,v 1.21 2005/06/19 08:27:44 comix Exp $
 *
 */

#include <stdlib.h>
#include "Preferences.h"

#include "config.h"
#include "LocalFileMng.h"

#ifndef WIN32
	#include <pwd.h>
	#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <qdir.h>
#include <stdio.h>

#include "DataPath.h"

#include "xml/tinyxml.h"

Preferences* Preferences::instance = NULL;


/// Return an instance of Preferences
Preferences* Preferences::getInstance() {
	if (instance == NULL) {
		instance = new Preferences();
	}

	return instance;
}




Preferences::Preferences()
 : Object( "Preferences" )
 , demoPath( string(DataPath::getDataPath()) + "/demo_songs/" )
 , m_sLastNews( "" )
{
	infoLog( "INIT" );

	char * ladpath = getenv("LADSPA_PATH");	// read the Environment variable LADSPA_PATH
	if (ladpath) {
		infoLog( "Found LADSPA_PATH enviroment variable" );
		string sLadspaPath = string(ladpath);
		int pos;
		while ( (pos = sLadspaPath.find(":")) != -1) {
			string sPath = sLadspaPath.substr(0, pos);
			m_ladspaPathVect.push_back( sPath );
			sLadspaPath = sLadspaPath.substr( pos + 1, sLadspaPath.length() );
		}
		m_ladspaPathVect.push_back(sLadspaPath);
	}
	else {
		m_ladspaPathVect.push_back( "/usr/lib/ladspa" );
		m_ladspaPathVect.push_back( string( CONFIG_PREFIX ).append( "/lib/hydrogen/plugins" ) );
		m_ladspaPathVect.push_back( "/usr/local/lib/ladspa" );
	}

	m_pDefaultUIStyle = new UIStyle();

	loadPreferences( true );	// Global settings
	loadPreferences( false );	// User settings
}



Preferences::~Preferences() {
	savePreferences();
	infoLog( "DESTROY" );
	instance = NULL;
	delete m_pDefaultUIStyle;
}






///
/// Load the preferences file
///
void Preferences::loadPreferences( bool bGlobal ) {
	bool recreate = false;	// configuration file must be recreated?

	string sPreferencesDirectory;
	string filename;
	if ( bGlobal ) {
		sPreferencesDirectory = string(DataPath::getDataPath());
		filename = sPreferencesDirectory + "/hydrogen.default.conf";
		infoLog( "[loadPreferences] Loading preferences file (GLOBAL) [" + filename + "]" );
	}
	else {
		sPreferencesDirectory = QDir::homeDirPath().append( "/.hydrogen" ).ascii();
		filename = sPreferencesDirectory + "/hydrogen.conf";
		infoLog( "[loadPreferences] Loading preferences file (USER) [" + filename + "]" );
	}



	// preferences directory exists?
	QDir prefDir( QString( sPreferencesDirectory.c_str() ) );
	if (!prefDir.exists()) {
		if ( bGlobal ) {
			errorLog( "[loadPreferences] Configuration directory not found." );
			exit( 1 );
		}
		else {
			warningLog( "[loadPreferences] Configuration directory not found." );
			createPreferencesDirectory();
		}
	}

	// data directory exists?
	string sDataDir = QDir::homeDirPath().append( "/.hydrogen/data" ).ascii();
	QDir dataDir( QString( sDataDir.c_str() ) );
	if ( !dataDir.exists() ) {
		warningLog( "[loadPreferences] Data directory not found." );
		createDataDirectory();
	}


	// pref file exists?
	std::ifstream input(filename.c_str() , std::ios::in | std::ios::binary);
	if (input){
		// read preferences file
		TiXmlDocument doc(filename.c_str());
		doc.LoadFile();

		TiXmlNode* rootNode;
		if ( (rootNode = doc.FirstChild("hydrogen_preferences")) ) {

			// version
			string version = LocalFileMng::readXmlString( this, rootNode, "version", "" );
			if ( version == "" ) {
				recreate = true;
			}

			//////// GENERAL ///////////
			//m_sLadspaPath = LocalFileMng::readXmlString( this, rootNode, "ladspaPath", m_sLadspaPath );
			restoreLastSong = LocalFileMng::readXmlBool( this, rootNode, "restoreLastSong", restoreLastSong );
			hearNewNotes = LocalFileMng::readXmlBool( this, rootNode, "hearNewNotes", hearNewNotes );
			recordEvents = LocalFileMng::readXmlBool( this, rootNode, "recordEvents", recordEvents );
			quantizeEvents = LocalFileMng::readXmlBool( this, rootNode, "quantizeEvents", quantizeEvents );

			// experimental...
			m_bUsePitchEditor = LocalFileMng::readXmlBool( this, rootNode, "usePitchEditor", false );
			
			TiXmlNode* pRecentUsedSongsNode = rootNode->FirstChild( "recentUsedSongs" );
			if ( pRecentUsedSongsNode ) {
				TiXmlNode* pSongNode = 0;
				for( pSongNode = pRecentUsedSongsNode->FirstChild("song"); pSongNode; pSongNode = pSongNode->NextSibling( "song" ) ) {
					string sFilename = pSongNode->FirstChild()->Value();
					m_recentFiles.push_back( sFilename );
				}
			}
			else {
				warningLog( "[loadPreferences] recentUsedSongs node not found" );
			}

			m_sLastNews = LocalFileMng::readXmlString( this, rootNode, "lastNews", "-", true );

			/////////////// AUDIO ENGINE //////////////
			TiXmlNode* audioEngineNode;
			if ( !(audioEngineNode = rootNode->FirstChild( "audio_engine" ) ) ) {
				warningLog( "[loadPreferences] audio_engine node not found" );
				recreate = true;
			}
			else {
				m_sAudioDriver = LocalFileMng::readXmlString( this, audioEngineNode, "audio_driver", m_sAudioDriver );
				m_bUseMetronome = LocalFileMng::readXmlBool( this, audioEngineNode, "use_metronome", m_bUseMetronome );
				m_fMetronomeVolume = LocalFileMng::readXmlFloat( this, audioEngineNode, "metronome_volume", m_fMetronomeVolume );
				m_nMaxNotes = LocalFileMng::readXmlInt( this, audioEngineNode, "maxNotes", m_nMaxNotes );
				m_nBufferSize = LocalFileMng::readXmlInt( this, audioEngineNode, "buffer_size", m_nBufferSize );
				m_nSampleRate = LocalFileMng::readXmlInt( this, audioEngineNode, "samplerate", m_nSampleRate );

				//// OSS DRIVER ////
				TiXmlNode* ossDriverNode;
				if ( !(ossDriverNode = audioEngineNode->FirstChild( "oss_driver" ) ) ) {
					warningLog("[loadPreferences] oss_driver node not found");
					recreate = true;
				}
				else {
					m_sOSSDevice = LocalFileMng::readXmlString( this, ossDriverNode, "ossDevice", m_sOSSDevice );
				}

				//// JACK DRIVER ////
				TiXmlNode* jackDriverNode;
				if ( !(jackDriverNode = audioEngineNode->FirstChild( "jack_driver" ) ) ) {
					warningLog("[loadPreferences] jack_driver node not found");
					recreate = true;
				}
				else {
					m_sJackPortName1 = LocalFileMng::readXmlString( this, jackDriverNode, "jack_port_name_1", m_sJackPortName1 );
					m_sJackPortName2 = LocalFileMng::readXmlString( this, jackDriverNode, "jack_port_name_2", m_sJackPortName2 );
					string sMode = LocalFileMng::readXmlString( this, jackDriverNode, "jack_transport_mode", "NO_JACK_TRANSPORT" );
					if (sMode == "NO_JACK_TRANSPORT") {
						m_bJackTransportMode = NO_JACK_TRANSPORT;
					}
					else if (sMode == "USE_JACK_TRANSPORT") {
						m_bJackTransportMode = USE_JACK_TRANSPORT;
					}
					m_bJackTrackOuts = LocalFileMng::readXmlBool( this, jackDriverNode, "jack_track_outs", m_bJackTrackOuts );
					m_bJackConnectDefaults = LocalFileMng::readXmlBool( this, jackDriverNode, "jack_connect_defaults", m_bJackConnectDefaults );
				}


				/// ALSA AUDIO DRIVER ///
				TiXmlNode* alsaAudioDriverNode;
				if ( !( alsaAudioDriverNode = audioEngineNode->FirstChild( "alsa_audio_driver" ) ) ) {
					warningLog("[loadPreferences] alsa_audio_driver node not found");
					recreate = true;
				}
				else {
					m_sAlsaAudioDevice = LocalFileMng::readXmlString( this, alsaAudioDriverNode, "alsa_audio_device", m_sAlsaAudioDevice );
				}

				/// MIDI DRIVER ///
				TiXmlNode* midiDriverNode;
				if ( !(midiDriverNode = audioEngineNode->FirstChild( "midi_driver" ) ) ) {
					warningLog( "[loadPreferences] midi_driver node not found" );
					recreate = true;
				}
				else {
					m_sMidiDriver = LocalFileMng::readXmlString( this, midiDriverNode, "driverName", "ALSA" );
					m_sMidiPortName = LocalFileMng::readXmlString( this, midiDriverNode, "port_name", "None" );
					m_nMidiChannelFilter = LocalFileMng::readXmlInt( this, midiDriverNode, "channel_filter", -1 );
					m_bMidiNoteOffIgnore = LocalFileMng::readXmlBool( this, midiDriverNode, "ignore_note_off", true );
				}


/*				//// ALSA MIDI DRIVER ////
				TiXmlNode* alsaMidiDriverNode;
				if ( !(alsaMidiDriverNode = audioEngineNode->FirstChild( "alsa_midi_driver" ) ) ) {
					warningLog( "[loadPreferences] alsa_midi_driver node not found" );
					recreate = true;
				}
				else {
					// midi port channel
					midiPortChannel = LocalFileMng::readXmlInt( this, alsaMidiDriverNode, "midi_port_channel", midiPortChannel );

					// midi destination name
					midiDest_name = LocalFileMng::readXmlString( this, alsaMidiDriverNode, "midi_dest_name", midiDest_name );

					// midi destination client
					midiDest_client = LocalFileMng::readXmlInt( this, alsaMidiDriverNode, "midi_dest_client", midiDest_client );

					// midi destination port
					midiDest_port = LocalFileMng::readXmlInt( this, alsaMidiDriverNode, "midi_dest_port", midiDest_port );

					// ignore note off
					m_bIgnoreMidiNoteOff = LocalFileMng::readXmlBool( this, alsaMidiDriverNode, "ignoreMidiNoteOff", m_bIgnoreMidiNoteOff );
				}
*/
			}

			/////////////// GUI //////////////
			TiXmlNode* guiNode;
			if ( !(guiNode = rootNode->FirstChild( "gui" ) ) ) {
				warningLog("[loadPreferences] gui node not found");
				recreate = true;
			}
			else {
				// QT Style
				m_sQTStyle = LocalFileMng::readXmlString( this, guiNode, "QTStyle", m_sQTStyle, true );

				// Interface mode

				string sMode = LocalFileMng::readXmlString( this, guiNode, "interface_mode", "" );
				if (sMode == "Child frame") {
					m_interfaceMode = MDI;
				}
				else if ( sMode == "Top level" ) {
					m_interfaceMode = TOP_LEVEL;
				}
				else if ( sMode == "Single paned" ) {
					m_interfaceMode = SINGLE_PANED;
				}
				else {
					m_interfaceMode = SINGLE_PANED;
				}

				// Application font family
				applicationFontFamily = LocalFileMng::readXmlString( this, guiNode, "application_font_family", applicationFontFamily );

				// Application font pointSize
				applicationFontPointSize = LocalFileMng::readXmlInt( this, guiNode, "application_font_pointsize", applicationFontPointSize );

				// mixer font family
				mixerFontFamily = LocalFileMng::readXmlString( this, guiNode, "mixer_font_family", mixerFontFamily );

				// mixer font pointSize
				mixerFontPointSize = LocalFileMng::readXmlInt( this, guiNode, "mixer_font_pointsize", mixerFontPointSize );

				// Mixer falloff speed
				mixerFalloffSpeed = LocalFileMng::readXmlFloat( this, guiNode, "mixer_falloff_speed", mixerFalloffSpeed );

				// pattern editor grid resolution
				m_nPatternEditorGridResolution = LocalFileMng::readXmlInt( this, guiNode, "patternEditorGridResolution", m_nPatternEditorGridResolution );
				m_bPatternEditorUsingTriplets = LocalFileMng::readXmlBool( this, guiNode, "patternEditorUsingTriplets", m_bPatternEditorUsingTriplets );
				m_bShowInstrumentPeaks = LocalFileMng::readXmlBool( this, guiNode, "showInstrumentPeaks", m_bShowInstrumentPeaks );

				// pattern editor grid height
				m_nPatternEditorGridHeight = LocalFileMng::readXmlInt( this, guiNode, "patternEditorGridHeight", m_nPatternEditorGridHeight );

				// pattern editor grid width
				m_nPatternEditorGridWidth = LocalFileMng::readXmlInt( this, guiNode, "patternEditorGridWidth", m_nPatternEditorGridWidth );

				// mainForm window properties
				setMainFormProperties( readWindowProperties( guiNode, "mainForm_properties", mainFormProperties ) );
				setMixerProperties( readWindowProperties( guiNode, "mixer_properties", mixerProperties ) );
				setPatternEditorProperties( readWindowProperties( guiNode, "patternEditor_properties", patternEditorProperties ) );
				setSongEditorProperties( readWindowProperties( guiNode, "songEditor_properties", songEditorProperties ) );
				setDrumkitManagerProperties( readWindowProperties( guiNode, "drumkitManager_properties", drumkitManagerProperties ) );
				setAudioEngineInfoProperties( readWindowProperties( guiNode, "audioEngineInfo_properties", audioEngineInfoProperties ) );
				setInstrumentEditorProperties( readWindowProperties( guiNode, "instrumentEditor_properties", audioEngineInfoProperties ) );



				m_bFollowPlayhead = LocalFileMng::readXmlBool( this, guiNode, "followPlayhead", true );
				
				for (unsigned nFX = 0; nFX < MAX_FX; nFX++) {
					string sNodeName = "ladspaFX_properties" + toString(nFX);
					setLadspaProperties(nFX, readWindowProperties( guiNode, sNodeName, m_ladspaProperties[nFX] ) );
				}

				TiXmlNode *pUIStyle = guiNode->FirstChild( "UI_Style" );
				if ( pUIStyle ) {
					readUIStyle( *pUIStyle );
				}
				else {
					warningLog( "[loadPreferences] UI_Style node not found" );
					recreate = true;
				}
			}

			/////////////// FILES //////////////
			TiXmlNode* filesNode;
			if ( !(filesNode = rootNode->FirstChild( "files" ) ) ) {
				warningLog( "[loadPreferences] files node not found");
				recreate = true;
			}
			else {
				// last used song
				lastSongFilename = LocalFileMng::readXmlString( this, filesNode, "lastSongFilename", lastSongFilename, true );
			}
		} // rootNode
		else {
			warningLog( "[loadPreferences] hydrogen_preferences node not found" );
			recreate = true;
		}
	}
	else {
		if ( bGlobal ) {
			errorLog( "[loadPreferences] Configuration file not found." );
			exit( 1 );
		}
		else {
			warningLog( "[loadPreferences] Configuration file not found." );
			recreate = true;
		}
	}


	// The preferences file should be recreated?
	if (recreate == true) {
		warningLog( "[loadPreferences] Recreating configuration file.");
		savePreferences();
	}

}



///
/// Save the preferences file
///
void Preferences::savePreferences() {
	string prefDir = QDir::homeDirPath().append("/.hydrogen").ascii();
	string filename = prefDir + "/hydrogen.conf";

	infoLog( "Saving preferences file: " + filename );

	TiXmlDocument doc(filename.c_str());

	TiXmlElement rootNode("hydrogen_preferences");

	// hydrogen version
	LocalFileMng::writeXmlString( &rootNode, "version", string(VERSION) );

	////// GENERAL ///////
	LocalFileMng::writeXmlString( &rootNode, "restoreLastSong", restoreLastSong ? "true": "false" );

	// hear new notes in the pattern editor
	LocalFileMng::writeXmlString( &rootNode, "hearNewNotes", hearNewNotes ? "true": "false" );

	// key/midi event prefs
	LocalFileMng::writeXmlString( &rootNode, "recordEvents", recordEvents ? "true": "false" );
	LocalFileMng::writeXmlString( &rootNode, "quantizeEvents", quantizeEvents ? "true": "false" );

	LocalFileMng::writeXmlString( &rootNode, "usePitchEditor", m_bUsePitchEditor ? "true": "false" );

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

	LocalFileMng::writeXmlString( &rootNode, "lastNews", m_sLastNews );


	//---- AUDIO ENGINE ----
	TiXmlElement audioEngineNode( "audio_engine" );
	{
		// audio driver
		LocalFileMng::writeXmlString( &audioEngineNode, "audio_driver", m_sAudioDriver );

		// use metronome
		LocalFileMng::writeXmlString( &audioEngineNode, "use_metronome", m_bUseMetronome ? "true": "false" );
		LocalFileMng::writeXmlString( &audioEngineNode, "metronome_volume", toString( m_fMetronomeVolume ) );
		LocalFileMng::writeXmlString( &audioEngineNode, "maxNotes", toString( m_nMaxNotes ) );
		LocalFileMng::writeXmlString( &audioEngineNode, "buffer_size", toString( m_nBufferSize ) );
		LocalFileMng::writeXmlString( &audioEngineNode, "samplerate", toString( m_nSampleRate ) );

		//// OSS DRIVER ////
		TiXmlElement ossDriverNode("oss_driver");
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
			string sMode;
			if ( m_bJackTransportMode == NO_JACK_TRANSPORT) {
				sMode = "NO_JACK_TRANSPORT";
			}
			else if ( m_bJackTransportMode == USE_JACK_TRANSPORT) {
				sMode = "USE_JACK_TRANSPORT";
			}
			LocalFileMng::writeXmlString( &jackDriverNode, "jack_transport_mode", sMode );

			// jack default connection
			string jackConnectDefaultsString = "false";
			if (m_bJackConnectDefaults) {
				jackConnectDefaultsString = "true";
			}
			LocalFileMng::writeXmlString( &jackDriverNode, "jack_connect_defaults", jackConnectDefaultsString );

			// jack track outs
			string jackTrackOutsString = "false";
			if (m_bJackTrackOuts) {
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
			LocalFileMng::writeXmlString( &midiDriverNode, "channel_filter", toString( m_nMidiChannelFilter ) );

			if ( m_bMidiNoteOffIgnore ) {
				LocalFileMng::writeXmlString( &midiDriverNode, "ignore_note_off", "true" );
			}
			else {
				LocalFileMng::writeXmlString( &midiDriverNode, "ignore_note_off", "false" );
			}
		}
		audioEngineNode.InsertEndChild( midiDriverNode );


/*		//// ALSA MIDI DRIVER ////
		TiXmlElement alsaMidiDriverNode( "alsa_midi_driver" );
		{
			LocalFileMng::writeXmlString( &alsaMidiDriverNode, "midi_port_channel", toString( midiPortChannel ) );	// Midi port channel
			LocalFileMng::writeXmlString( &alsaMidiDriverNode, "midi_dest_name", midiDest_name );		// Midi destination name
			LocalFileMng::writeXmlString( &alsaMidiDriverNode, "midi_dest_client", toString( midiDest_client ) );		// Midi destination client
			LocalFileMng::writeXmlString( &alsaMidiDriverNode, "midi_dest_port", toString( midiDest_port ) );		// Midi destination port

			// Ignore midi note off
			string sIgnore = "false";
			if (m_bIgnoreMidiNoteOff) {
				sIgnore = "true";
			}
			LocalFileMng::writeXmlString( &alsaMidiDriverNode, "ignoreMidiNoteOff", sIgnore );
		}
		audioEngineNode.InsertEndChild( alsaMidiDriverNode );
*/
	}
	rootNode.InsertEndChild( audioEngineNode );

	//---- GUI ----
	TiXmlElement guiNode("gui");
	{
		LocalFileMng::writeXmlString( &guiNode, "QTStyle", m_sQTStyle );

		// Interface mode
		switch ( m_interfaceMode ) {
			case TOP_LEVEL:
				LocalFileMng::writeXmlString( &guiNode, "interface_mode", "Top level" );
				break;

			case MDI:
				LocalFileMng::writeXmlString( &guiNode, "interface_mode", "Child frame" );
				break;

			case SINGLE_PANED:
			default:
				LocalFileMng::writeXmlString( &guiNode, "interface_mode", "Single paned" );
				break;
		}

		LocalFileMng::writeXmlString( &guiNode, "application_font_family", applicationFontFamily );
		LocalFileMng::writeXmlString( &guiNode, "application_font_pointsize", toString( applicationFontPointSize ) );
		LocalFileMng::writeXmlString( &guiNode, "mixer_font_family", mixerFontFamily );
		LocalFileMng::writeXmlString( &guiNode, "mixer_font_pointsize", toString( mixerFontPointSize ) );
		LocalFileMng::writeXmlString( &guiNode, "mixer_falloff_speed", toString( mixerFalloffSpeed ) );
		LocalFileMng::writeXmlString( &guiNode, "patternEditorGridResolution", toString( m_nPatternEditorGridResolution ) );
		LocalFileMng::writeXmlString( &guiNode, "patternEditorGridHeight", toString( m_nPatternEditorGridHeight ) );
		LocalFileMng::writeXmlString( &guiNode, "patternEditorGridWidth", toString( m_nPatternEditorGridWidth ) );
		LocalFileMng::writeXmlBool( &guiNode, "patternEditorUsingTriplets", m_bPatternEditorUsingTriplets );
		LocalFileMng::writeXmlBool( &guiNode, "showInstrumentPeaks", m_bShowInstrumentPeaks );

		// MainForm window properties
		writeWindowProperties( guiNode, "mainForm_properties", mainFormProperties );
		writeWindowProperties( guiNode, "mixer_properties", mixerProperties );
		writeWindowProperties( guiNode, "patternEditor_properties", patternEditorProperties );
		writeWindowProperties( guiNode, "songEditor_properties", songEditorProperties );
		writeWindowProperties( guiNode, "drumkitManager_properties", drumkitManagerProperties );
		writeWindowProperties( guiNode, "audioEngineInfo_properties", audioEngineInfoProperties );
		writeWindowProperties( guiNode, "instrumentEditor_properties", m_instrumentEditorProperties );
		for (unsigned nFX = 0; nFX < MAX_FX; nFX++) {
			string sNode = "ladspaFX_properties" + toString(nFX);
			writeWindowProperties( guiNode, sNode, m_ladspaProperties[nFX] );
		}

		LocalFileMng::writeXmlBool( &guiNode, "followPlayhead", m_bFollowPlayhead );

		// User interface style
		writeUIStyle( guiNode );
	}
	rootNode.InsertEndChild( guiNode );

	//---- FILES ----
	TiXmlElement filesNode( "files" );
	{
		// last used song
		LocalFileMng::writeXmlString( &filesNode, "lastSongFilename", lastSongFilename );
	}
	rootNode.InsertEndChild( filesNode );

	doc.InsertEndChild(rootNode);
	doc.SaveFile();
}



///
/// Create preferences directory
///
void Preferences::createPreferencesDirectory() {
	string prefDir = QDir::homeDirPath().append( "/.hydrogen" ).ascii();

	warningLog("Creating preference file directory in " + prefDir);

	QDir dir;
	dir.mkdir( prefDir.c_str() );

//	mkdir(prefDir.c_str(),S_IRWXU);
}



///
/// Create data directory
///
void Preferences::createDataDirectory() {
	string sDir = QDir::homeDirPath().append( "/.hydrogen/data" ).ascii();

	warningLog("Creating data directory in " + sDir);

	QDir dir;
	dir.mkdir( QString( sDir.c_str() ) );
//	mkdir(dir.c_str(),S_IRWXU);
}



void Preferences::setRecentFiles( vector<string> recentFiles )
{
	// find single filenames. (skip duplicates)
	vector<string> temp;
	for (unsigned i = 0; i < recentFiles.size(); i++) {
		string sFilename = recentFiles[ i ];

		bool bExists = false;
		for (unsigned j = 0; j < temp.size(); j++) {
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
WindowProperties Preferences::readWindowProperties( TiXmlNode *parent, string windowName, WindowProperties defaultProp ) {
	WindowProperties prop = defaultProp;

	TiXmlNode* windowPropNode;
	if ( !(windowPropNode = parent->FirstChild( windowName.c_str() ) ) ) {
		warningLog( "Error reading configuration file: " + windowName + " node not found" );
	}
	else {
		prop.visible = LocalFileMng::readXmlBool( this, windowPropNode, "visible", true );
		prop.x = LocalFileMng::readXmlInt( this, windowPropNode, "x", prop.x );
		prop.y = LocalFileMng::readXmlInt( this, windowPropNode, "y", prop.y );
		prop.width = LocalFileMng::readXmlInt( this, windowPropNode, "width", prop.width );
		prop.height = LocalFileMng::readXmlInt( this, windowPropNode, "height", prop.height );
	}

	return prop;
}



/// Write the xml nodes related to window properties
void Preferences::writeWindowProperties( TiXmlNode& parent, const string& windowName, const WindowProperties& prop )
{
	TiXmlElement windowPropNode( windowName.c_str() );
		if (prop.visible) {
			LocalFileMng::writeXmlString( &windowPropNode, "visible", "true" );
		}
		else {
			LocalFileMng::writeXmlString( &windowPropNode, "visible", "false" );
		}

		LocalFileMng::writeXmlString( &windowPropNode, "x", toString( prop.x ) );
		LocalFileMng::writeXmlString( &windowPropNode, "y", toString( prop.y ) );
		LocalFileMng::writeXmlString( &windowPropNode, "width", toString( prop.width ) );
		LocalFileMng::writeXmlString( &windowPropNode, "height", toString( prop.height ) );
	parent.InsertEndChild( windowPropNode );
}



void Preferences::writeUIStyle( TiXmlNode& parent )
{
	TiXmlElement node( "UI_Style" );

	// SONG EDITOR
	TiXmlElement songEditorNode( "songEditor" );
	LocalFileMng::writeXmlString( &songEditorNode, "backgroundColor", m_pDefaultUIStyle->m_songEditor_backgroundColor.toString() );
	LocalFileMng::writeXmlString( &songEditorNode, "alternateRowColor", m_pDefaultUIStyle->m_songEditor_alternateRowColor.toString() );
	LocalFileMng::writeXmlString( &songEditorNode, "selectedRowColor", m_pDefaultUIStyle->m_songEditor_selectedRowColor.toString() );
	LocalFileMng::writeXmlString( &songEditorNode, "lineColor", m_pDefaultUIStyle->m_songEditor_lineColor.toString() );
	LocalFileMng::writeXmlString( &songEditorNode, "textColor", m_pDefaultUIStyle->m_songEditor_textColor.toString() );
	LocalFileMng::writeXmlString( &songEditorNode, "pattern1Color", m_pDefaultUIStyle->m_songEditor_pattern1Color.toString() );
	node.InsertEndChild( songEditorNode );

	// PATTERN EDITOR PANEL
//	TiXmlElement patternEditorPanelNode( "patternEditorPanel" );
//	LocalFileMng::writeXmlString( &patternEditorPanelNode, "backgroundColor", m_pDefaultUIStyle->m_patternEditorPanel_backgroundColor.toString() );
//	node.InsertEndChild( patternEditorPanelNode );

	// PATTERN EDITOR
	TiXmlElement patternEditorNode( "patternEditor" );
	LocalFileMng::writeXmlString( &patternEditorNode, "backgroundColor", m_pDefaultUIStyle->m_patternEditor_backgroundColor.toString() );
	LocalFileMng::writeXmlString( &patternEditorNode, "alternateRowColor", m_pDefaultUIStyle->m_patternEditor_alternateRowColor.toString() );
	LocalFileMng::writeXmlString( &patternEditorNode, "selectedMuteRowColor", m_pDefaultUIStyle->m_patternEditor_selectedMuteRowColor.toString() );
	LocalFileMng::writeXmlString( &patternEditorNode, "selectedRowColor", m_pDefaultUIStyle->m_patternEditor_selectedRowColor.toString() );
	LocalFileMng::writeXmlString( &patternEditorNode, "muteRowColor", m_pDefaultUIStyle->m_patternEditor_muteRowColor.toString() );
	LocalFileMng::writeXmlString( &patternEditorNode, "selectedMuteLockRowColor", m_pDefaultUIStyle->m_patternEditor_selectedMuteLockRowColor.toString() );
	LocalFileMng::writeXmlString( &patternEditorNode, "selectedLockRowColor", m_pDefaultUIStyle->m_patternEditor_selectedLockRowColor.toString() );
	LocalFileMng::writeXmlString( &patternEditorNode, "muteLockRowColor", m_pDefaultUIStyle->m_patternEditor_muteLockRowColor.toString() );
	LocalFileMng::writeXmlString( &patternEditorNode, "lockRowColor", m_pDefaultUIStyle->m_patternEditor_lockRowColor.toString() );
	LocalFileMng::writeXmlString( &patternEditorNode, "textColor", m_pDefaultUIStyle->m_patternEditor_textColor.toString() );
	LocalFileMng::writeXmlString( &patternEditorNode, "noteColor", m_pDefaultUIStyle->m_patternEditor_noteColor.toString() );
	LocalFileMng::writeXmlString( &patternEditorNode, "lineColor", m_pDefaultUIStyle->m_patternEditor_lineColor.toString() );
	LocalFileMng::writeXmlString( &patternEditorNode, "line1Color", m_pDefaultUIStyle->m_patternEditor_line1Color.toString() );
	LocalFileMng::writeXmlString( &patternEditorNode, "line2Color", m_pDefaultUIStyle->m_patternEditor_line2Color.toString() );
	LocalFileMng::writeXmlString( &patternEditorNode, "line3Color", m_pDefaultUIStyle->m_patternEditor_line3Color.toString() );
	LocalFileMng::writeXmlString( &patternEditorNode, "line4Color", m_pDefaultUIStyle->m_patternEditor_line4Color.toString() );
	LocalFileMng::writeXmlString( &patternEditorNode, "line5Color", m_pDefaultUIStyle->m_patternEditor_line5Color.toString() );
	node.InsertEndChild( patternEditorNode );

	parent.InsertEndChild( node );
}



void Preferences::readUIStyle( TiXmlNode& parent )
{
	// SONG EDITOR
	TiXmlNode* pSongEditorNode = parent.FirstChild( "songEditor" );
	if ( pSongEditorNode ) {
		m_pDefaultUIStyle->m_songEditor_backgroundColor = H2RGBColor( LocalFileMng::readXmlString( this, pSongEditorNode, "backgroundColor", m_pDefaultUIStyle->m_songEditor_backgroundColor.toString() ) );
		m_pDefaultUIStyle->m_songEditor_alternateRowColor = H2RGBColor( LocalFileMng::readXmlString( this, pSongEditorNode, "alternateRowColor", m_pDefaultUIStyle->m_songEditor_alternateRowColor.toString() ) );
		m_pDefaultUIStyle->m_songEditor_selectedRowColor = H2RGBColor( LocalFileMng::readXmlString( this, pSongEditorNode, "selectedRowColor", m_pDefaultUIStyle->m_songEditor_selectedRowColor.toString() ) );
		m_pDefaultUIStyle->m_songEditor_lineColor = H2RGBColor( LocalFileMng::readXmlString( this, pSongEditorNode, "lineColor", m_pDefaultUIStyle->m_songEditor_lineColor.toString() ) );
		m_pDefaultUIStyle->m_songEditor_textColor = H2RGBColor( LocalFileMng::readXmlString( this, pSongEditorNode, "textColor", m_pDefaultUIStyle->m_songEditor_textColor.toString() ) );
		m_pDefaultUIStyle->m_songEditor_pattern1Color = H2RGBColor( LocalFileMng::readXmlString( this, pSongEditorNode, "pattern1Color", m_pDefaultUIStyle->m_songEditor_pattern1Color.toString() ) );
	}
	else {
		warningLog( "[readUIStyle] songEditor node not found" );
	}

	// PATTERN EDITOR PANEL
//	TiXmlNode* pPatternEditorPanelNode = parent.FirstChild( "patternEditorPanel" );
//	if ( pPatternEditorPanelNode ) {
//		m_pDefaultUIStyle->m_patternEditorPanel_backgroundColor = H2RGBColor( LocalFileMng::readXmlString( this, pPatternEditorPanelNode, "backgroundColor", m_pDefaultUIStyle->m_patternEditorPanel_backgroundColor.toString() ) );
//	}
//	else {
//		warningLog( "[readUIStyle] songEditorPanel node not found" );
//	}

	// PATTERN EDITOR
	TiXmlNode* pPatternEditorNode = parent.FirstChild( "patternEditor" );
	if ( pPatternEditorNode ) {
		m_pDefaultUIStyle->m_patternEditor_backgroundColor = H2RGBColor( LocalFileMng::readXmlString( this, pPatternEditorNode, "backgroundColor", m_pDefaultUIStyle->m_patternEditor_backgroundColor.toString() ) );
		m_pDefaultUIStyle->m_patternEditor_alternateRowColor = H2RGBColor( LocalFileMng::readXmlString( this, pPatternEditorNode, "alternateRowColor", m_pDefaultUIStyle->m_patternEditor_alternateRowColor.toString() ) );
		m_pDefaultUIStyle->m_patternEditor_selectedRowColor = H2RGBColor( LocalFileMng::readXmlString( this, pPatternEditorNode, "selectedRowColor", m_pDefaultUIStyle->m_patternEditor_selectedRowColor.toString() ) );
		m_pDefaultUIStyle->m_patternEditor_selectedMuteRowColor = H2RGBColor( LocalFileMng::readXmlString( this, pPatternEditorNode, "selectedMuteRowColor", m_pDefaultUIStyle->m_patternEditor_selectedMuteRowColor.toString() ) );
		m_pDefaultUIStyle->m_patternEditor_muteRowColor = H2RGBColor( LocalFileMng::readXmlString( this, pPatternEditorNode, "muteRowColor", m_pDefaultUIStyle->m_patternEditor_muteRowColor.toString() ) );
		m_pDefaultUIStyle->m_patternEditor_selectedMuteLockRowColor = H2RGBColor( LocalFileMng::readXmlString( this, pPatternEditorNode, "selectedMuteLockRowColor", m_pDefaultUIStyle->m_patternEditor_selectedMuteLockRowColor.toString() ) );
		m_pDefaultUIStyle->m_patternEditor_selectedLockRowColor = H2RGBColor( LocalFileMng::readXmlString( this, pPatternEditorNode, "selectedLockRowColor", m_pDefaultUIStyle->m_patternEditor_selectedLockRowColor.toString() ) );
		m_pDefaultUIStyle->m_patternEditor_muteLockRowColor = H2RGBColor( LocalFileMng::readXmlString( this, pPatternEditorNode, "muteLockRowColor", m_pDefaultUIStyle->m_patternEditor_muteLockRowColor.toString() ) );
		m_pDefaultUIStyle->m_patternEditor_lockRowColor = H2RGBColor( LocalFileMng::readXmlString( this, pPatternEditorNode, "lockRowColor", m_pDefaultUIStyle->m_patternEditor_lockRowColor.toString() ) );
		m_pDefaultUIStyle->m_patternEditor_textColor = H2RGBColor( LocalFileMng::readXmlString( this, pPatternEditorNode, "textColor", m_pDefaultUIStyle->m_patternEditor_textColor.toString() ) );
		m_pDefaultUIStyle->m_patternEditor_noteColor = H2RGBColor( LocalFileMng::readXmlString( this, pPatternEditorNode, "noteColor", m_pDefaultUIStyle->m_patternEditor_noteColor.toString() ) );
		m_pDefaultUIStyle->m_patternEditor_lineColor = H2RGBColor( LocalFileMng::readXmlString( this, pPatternEditorNode, "lineColor", m_pDefaultUIStyle->m_patternEditor_lineColor.toString() ) );
		m_pDefaultUIStyle->m_patternEditor_line1Color = H2RGBColor( LocalFileMng::readXmlString( this, pPatternEditorNode, "line1Color", m_pDefaultUIStyle->m_patternEditor_line1Color.toString() ) );
		m_pDefaultUIStyle->m_patternEditor_line2Color = H2RGBColor( LocalFileMng::readXmlString( this, pPatternEditorNode, "line2Color", m_pDefaultUIStyle->m_patternEditor_line2Color.toString() ) );
		m_pDefaultUIStyle->m_patternEditor_line3Color = H2RGBColor( LocalFileMng::readXmlString( this, pPatternEditorNode, "line3Color", m_pDefaultUIStyle->m_patternEditor_line3Color.toString() ) );
		m_pDefaultUIStyle->m_patternEditor_line4Color = H2RGBColor( LocalFileMng::readXmlString( this, pPatternEditorNode, "line4Color", m_pDefaultUIStyle->m_patternEditor_line4Color.toString() ) );
		m_pDefaultUIStyle->m_patternEditor_line5Color = H2RGBColor( LocalFileMng::readXmlString( this, pPatternEditorNode, "line5Color", m_pDefaultUIStyle->m_patternEditor_line5Color.toString() ) );
	}
	else {
		warningLog( "[readUIStyle] patternEditor node not found" );
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



WindowProperties::~WindowProperties() {
//	infoLog( "DESTROY" );
}




// :::::::::::::::::::::::::::::::



UIStyle::UIStyle()
 : Object( "UIStyle" )
{
//	infoLog( "INIT" );
}



// ::::::::::::::::::::::::::::::::::::::



H2RGBColor::H2RGBColor(int r, int g, int b)
 : Object( "H2RGBColor" )
 , m_red( r )
 , m_green( g )
 , m_blue( b )
{
//	infoLog( "INIT" );
}



H2RGBColor::~H2RGBColor()
{
//	infoLog( "DESTROY" );
}



H2RGBColor::H2RGBColor( const string& sColor )
 : Object ( "H2RGBColor" )
{
//	infoLog( "INIT " + sColor );
	string temp = sColor;

	int nPos = temp.find(',');
	string sRed = temp.substr(0, nPos);
	temp.erase(0, nPos + 1);

	nPos = temp.find(',');
	string sGreen = temp.substr(0, nPos);
	temp.erase(0, nPos + 1);

	nPos = temp.find(',');
	string sBlue = temp.substr(0, nPos);

	m_red = atoi( sRed.c_str() );
	m_green = atoi( sGreen.c_str() );
	m_blue = atoi( sBlue.c_str() );

}



string H2RGBColor::toString()
{
	string sRes = ::toString( m_red ) + "," + ::toString( m_green ) + "," + ::toString( m_blue );

//	infoLog( "[toString] " + sRes );

	return sRes;
}
