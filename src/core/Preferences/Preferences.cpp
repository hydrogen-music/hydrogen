/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <stdlib.h>
#include <core/Preferences/Preferences.h>

#include <core/LocalFileMng.h>

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
#include <algorithm>
#include <memory>

#include <core/MidiMap.h>
#include <core/Version.h>
#include <core/Helpers/Filesystem.h>

#include <QDir>
//#include <QApplication>

namespace H2Core
{

Preferences* Preferences::__instance = nullptr;

void Preferences::create_instance()
{
	if ( __instance == nullptr ) {
		__instance = new Preferences;
	}
}

Preferences::Preferences()
{
	__instance = this;
	m_pTheme = std::make_shared<Theme>();
	
	// switch to enable / disable lash, only on h2 startup
	m_brestartLash = false;
	m_bsetLash = false;

	//rubberband bpm change queue
	m_useTheRubberbandBpmChangeEvent = false;

	QString rubberBandCLIPath = getenv( "PATH" );
	QStringList rubberBandCLIPathList = rubberBandCLIPath.split(":");//linux use ":" as separator. maybe windows and osx use other separators

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

	m_sPreferredLanguage = QString();
	__playsamplesonclicking = false; // audio file browser
	__playselectedinstrument = false; // midi keyboard and keyboard play only selected instrument

	recordEvents = false; // not recording by default
	punchInPos = 0;
	punchOutPos = -1;

	__expandSongItem = true; //SoundLibraryPanel
	__expandPatternItem = true; //SoundLibraryPanel
	__useTimelineBpm = false;		// use timeline
	
	m_sLastExportPatternAsDirectory = QDir::homePath();
	m_sLastExportSongDirectory = QDir::homePath();
	m_sLastSaveSongAsDirectory = QDir::homePath();
	m_sLastOpenSongDirectory = Filesystem::songs_dir();
	m_sLastOpenPatternDirectory = Filesystem::patterns_dir();
	m_sLastExportLilypondDirectory = QDir::homePath();
	m_sLastExportMidiDirectory = QDir::homePath();
	m_sLastImportDrumkitDirectory = QDir::homePath();
	m_sLastExportDrumkitDirectory = QDir::homePath();
	m_sLastOpenLayerDirectory = QDir::homePath();
	m_sLastOpenPlaybackTrackDirectory = QDir::homePath();
	m_sLastAddSongToPlaylistDirectory = Filesystem::songs_dir();
	m_sLastPlaylistDirectory = Filesystem::playlists_dir();
	m_sLastPlaylistScriptDirectory = Filesystem::scripts_dir();
	
	//export dialog
	m_nExportModeIdx = 0;
	m_nExportSampleRateIdx = 0;
	m_nExportSampleDepthIdx = 0;

	//export midi dialog
	m_nMidiExportMode = 0;
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

	sServerList.push_back( QString("http://hydrogen-music.org/feeds/drumkit_list.php") );
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
#if defined(H2CORE_HAVE_ALSA)
	m_sMidiDriver = QString("ALSA");
#elif defined(H2CORE_HAVE_PORTMIDI)
	m_sMidiDriver = QString("PortMidi");
#elif defined(H2CORE_HAVE_COREMIDI)
	m_sMidiDriver = QString("CoreMIDI");
#elif defined(H2CORE_HAVE_JACK)
	m_sMidiDriver = QString("JACK-MIDI");
#else
	// Set ALSA as fallback if none of the above options are available
	// (although MIDI won't work in this case).
	m_sMidiDriver = QString( "ALSA" );
#endif
	m_sMidiPortName = QString("None");
	m_sMidiOutputPortName = QString("None");
	m_nMidiChannelFilter = -1;
	m_bMidiNoteOffIgnore = false;
	m_bMidiFixedMapping = false;
	m_bMidiDiscardNoteAfterAction = false;

	// PortAudio properties
	m_sPortAudioDevice = QString();
	m_sPortAudioHostAPI = QString();
	m_nLatencyTarget = 0;

	// CoreAudio
	m_sCoreAudioDevice = QString();

	//___  alsa audio driver properties ___
	m_sAlsaAudioDevice = QString("hw:0");

	//___  jack driver properties ___
	m_sJackPortName1 = QString("alsa_pcm:playback_1");
	m_sJackPortName2 = QString("alsa_pcm:playback_2");
	m_bJackTransportMode = true;
	m_bJackConnectDefaults = true;
	m_bJackTrackOuts = false;
	m_bJackTimebaseEnabled = true;
	m_bJackMasterMode = NO_JACK_TIME_MASTER;
	m_JackTrackOutputMode = JackTrackOutputMode::postFader;

	// OSC configuration
	m_bOscServerEnabled = false;
	m_bOscFeedbackEnabled = true;
	m_nOscServerPort = 9000;
	m_nOscTemporaryPort = -1;

	//___ General properties ___
	m_bPatternModePlaysSelected = true;
	m_brestoreLastSong = true;
	m_brestoreLastPlaylist = false;
	m_bUseLash = false;
	m_bShowDevelWarning = false;
	m_bShowNoteOverwriteWarning = true;
	// NONE: lastSongFilename;
	hearNewNotes = true;
	// NONE: m_recentFiles;
	// NONE: m_recentFX;
	quantizeEvents = true;
	recordEvents = false;
	m_bUseRelativeFilenamesForPlaylists = false;
	m_bHideKeyboardCursor = false;
	m_bPatternFollowsSong = false;

	//___ GUI properties ___
	m_nPatternEditorGridResolution = 8;
	m_bPatternEditorUsingTriplets = false;
	m_bShowInstrumentPeaks = true;
	m_bIsFXTabVisible = true;
	m_bShowAutomationArea = false;
	m_bShowPlaybackTrack = false;
	m_nPatternEditorGridHeight = 21;
	m_nPatternEditorGridWidth = 3;
	m_nSongEditorGridHeight = 18;
	m_nSongEditorGridWidth = 16;
	mainFormProperties.set(0, 0, 1000, 700, true);
	mixerProperties.set(10, 350, 829, 276, true);
	patternEditorProperties.set(280, 100, 706, 439, true);
	songEditorProperties.set(10, 10, 600, 250, true);
	instrumentRackProperties.set(500, 20, 526, 437, true);
	audioEngineInfoProperties.set(720, 120, 0, 0, false);
	m_ladspaProperties[0].set(2, 20, 0, 0, false);
	m_ladspaProperties[1].set(2, 20, 0, 0, false);
	m_ladspaProperties[2].set(2, 20, 0, 0, false);
	m_ladspaProperties[3].set(2, 20, 0, 0, false);
	m_nMaxBars = 400;
	m_nMaxLayers = 16;

	/////////////////////////////////////////////////////////////////////////
	//////////////// END OF DEFAULT SETTINGS ////////////////////////////////
	/////////////////////////////////////////////////////////////////////////

	loadPreferences( true );	// Global settings
	loadPreferences( false );	// User settings
}



Preferences::~Preferences()
{
	INFOLOG( "DESTROY" );
	__instance = nullptr;
}






///
/// Load the preferences file
///
void Preferences::loadPreferences( bool bGlobal )
{
	// We do not required the recently used variables to be
	// accumulated throughout various configuration files.
	m_recentFiles.clear();
	m_recentFX.clear();

	bool recreate = false;	// configuration file must be recreated?

	QString sPreferencesFilename;
	const QString sPreferencesOverwritePath = Filesystem::getPreferencesOverwritePath();
	if ( sPreferencesOverwritePath.isEmpty() ) {
			sPreferencesFilename = ( bGlobal ? Filesystem::sys_config_path() : Filesystem::usr_config_path() );
			INFOLOG( QString( "Loading preferences file (%1) [%2]" ).arg( bGlobal ? "SYS" : "USER" ).arg( sPreferencesFilename ) );
	} else {
		sPreferencesFilename = sPreferencesOverwritePath;
		INFOLOG( QString( "Loading preferences file %1" ).arg( sPreferencesFilename ) );
	}
	
	Filesystem::file_readable( sPreferencesFilename );

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
			m_sPreferredLanguage = LocalFileMng::readXmlString( rootNode, "preferredLanguage", QString() );
			__playselectedinstrument = LocalFileMng::readXmlBool( rootNode, "instrumentInputMode", __playselectedinstrument );
			m_bShowDevelWarning = LocalFileMng::readXmlBool( rootNode, "showDevelWarning", m_bShowDevelWarning );
			m_bShowNoteOverwriteWarning = LocalFileMng::readXmlBool( rootNode, "showNoteOverwriteWarning", m_bShowNoteOverwriteWarning );
			m_brestoreLastSong = LocalFileMng::readXmlBool( rootNode, "restoreLastSong", m_brestoreLastSong );
			m_brestoreLastPlaylist = LocalFileMng::readXmlBool( rootNode, "restoreLastPlaylist", m_brestoreLastPlaylist );
			m_bPatternModePlaysSelected = LocalFileMng::readXmlBool( rootNode, "patternModePlaysSelected", true );
			m_bUseLash = LocalFileMng::readXmlBool( rootNode, "useLash", false );
			__useTimelineBpm = LocalFileMng::readXmlBool( rootNode, "useTimeLine", __useTimelineBpm );
			m_nMaxBars = LocalFileMng::readXmlInt( rootNode, "maxBars", 400 );
			m_nMaxLayers = LocalFileMng::readXmlInt( rootNode, "maxLayers", 16 );
			setDefaultUILayout( static_cast<InterfaceTheme::Layout>(LocalFileMng::readXmlInt( rootNode, "defaultUILayout",
																							  static_cast<int>(InterfaceTheme::Layout::SinglePane) )) );
			setUIScalingPolicy( static_cast<InterfaceTheme::ScalingPolicy>(LocalFileMng::readXmlInt( rootNode, "uiScalingPolicy", static_cast<int>(InterfaceTheme::ScalingPolicy::Smaller) )) );
			m_nLastOpenTab =  LocalFileMng::readXmlInt( rootNode, "lastOpenTab", 0 );
			m_bUseRelativeFilenamesForPlaylists = LocalFileMng::readXmlBool( rootNode, "useRelativeFilenamesForPlaylists", false );
			m_bHideKeyboardCursor = LocalFileMng::readXmlBool( rootNode, "hideKeyboardCursorWhenUnused", false );
			m_bPatternFollowsSong = LocalFileMng::readXmlBool( rootNode, "patternFollowsSong", false );

			//restore the right m_bsetlash value
			m_bsetLash = m_bUseLash;
			m_useTheRubberbandBpmChangeEvent = LocalFileMng::readXmlBool( rootNode, "useTheRubberbandBpmChangeEvent", m_useTheRubberbandBpmChangeEvent );

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
				// Ensure compatibility will older versions of the
				// files after capitalization in the GUI
				// (2021-02-05). This can be dropped in releases >=
				// 1.2
				if ( m_sAudioDriver == "Jack" ) {
					m_sAudioDriver = "JACK";
				} else if ( m_sAudioDriver == "Oss" ) {
					m_sAudioDriver = "OSS";
				} else if ( m_sAudioDriver == "Alsa" ) {
					m_sAudioDriver = "ALSA";
				}
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

				//// PORTAUDIO DRIVER ////
				QDomNode portAudioDriverNode = audioEngineNode.firstChildElement( "portaudio_driver" );
				if ( portAudioDriverNode.isNull()  ) {
					WARNINGLOG( "portaudio_driver node not found" );
					recreate = true;
				} else {
					m_sPortAudioDevice = LocalFileMng::readXmlString( portAudioDriverNode, "portAudioDevice", m_sPortAudioDevice );
					m_sPortAudioHostAPI = LocalFileMng::readXmlString( portAudioDriverNode, "portAudioHostAPI", m_sPortAudioHostAPI );
					m_nLatencyTarget = LocalFileMng::readXmlInt( portAudioDriverNode, "latencyTarget", m_nLatencyTarget );
				}

				//// COREAUDIO DRIVER ////
				QDomNode coreAudioDriverNode = audioEngineNode.firstChildElement( "coreaudio_driver" );
				if ( coreAudioDriverNode.isNull()  ) {
					WARNINGLOG( "coreaudio_driver node not found" );
					recreate = true;
				} else {
					m_sCoreAudioDevice = LocalFileMng::readXmlString( coreAudioDriverNode, "coreAudioDevice", m_sCoreAudioDevice );
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
					m_bJackTimebaseEnabled = LocalFileMng::readXmlBool( jackDriverNode, "jack_timebase_enabled", true );
					QString tmMode = LocalFileMng::readXmlString( jackDriverNode, "jack_transport_mode_master", "NO_JACK_TIME_MASTER" );
					if ( tmMode == "NO_JACK_TIME_MASTER" ) {
						m_bJackMasterMode = NO_JACK_TIME_MASTER;
					} else if ( tmMode == "USE_JACK_TIME_MASTER" ) {
						m_bJackMasterMode = USE_JACK_TIME_MASTER;
					}

					//~ jack time master

					m_bJackTrackOuts = LocalFileMng::readXmlBool( jackDriverNode, "jack_track_outs", m_bJackTrackOuts );
					m_bJackConnectDefaults = LocalFileMng::readXmlBool( jackDriverNode, "jack_connect_defaults", m_bJackConnectDefaults );

					int nJackTrackOutputMode = LocalFileMng::readXmlInt( jackDriverNode, "jack_track_output_mode", 0 );
					switch ( nJackTrackOutputMode ) {
					case 0:
						m_JackTrackOutputMode = JackTrackOutputMode::postFader;
						break;
					case 1:
						m_JackTrackOutputMode = JackTrackOutputMode::preFader;
						break;
					default:
						WARNINGLOG( QString( "Unknown jack_track_output_mode value [%1]. Using JackTrackOutputMode::postFader instead." )
								  .arg( nJackTrackOutputMode ) );
						m_JackTrackOutputMode = JackTrackOutputMode::postFader;
					}
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
					// Ensure compatibility will older versions of the
					// files after capitalization in the GUI
					// (2021-02-05). This can be dropped in releases
					// >= 1.2
					if ( m_sAudioDriver == "JackMidi" ) {
						m_sAudioDriver = "JACK-MIDI";
					} else if ( m_sAudioDriver == "CoreMidi" ) {
						m_sAudioDriver = "CoreMIDI";
					}
					m_sMidiPortName = LocalFileMng::readXmlString( midiDriverNode, "port_name", "None" );
					m_sMidiOutputPortName = LocalFileMng::readXmlString( midiDriverNode, "output_port_name", "None" );
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
					m_bOscFeedbackEnabled = LocalFileMng::readXmlBool( oscServerNode, "oscFeedbackEnabled", true );
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
				setQTStyle( LocalFileMng::readXmlString( guiNode, "QTStyle", "Fusion", true ) );

				if ( getQTStyle() == "Plastique" ){
					setQTStyle( "Fusion" );
				}

				// Font fun
				setApplicationFontFamily( LocalFileMng::readXmlString( guiNode, "application_font_family", getApplicationFontFamily() ) );
				// The value defaults to m_sApplicationFontFamily on
				// purpose to provide backward compatibility.
				setLevel2FontFamily( LocalFileMng::readXmlString( guiNode, "level2_font_family", getLevel2FontFamily() ) );
				setLevel3FontFamily( LocalFileMng::readXmlString( guiNode, "level3_font_family", getLevel3FontFamily() ) );
				setFontSize( static_cast<FontTheme::FontSize>( LocalFileMng::readXmlInt( guiNode, "font_size",
																			  static_cast<int>(FontTheme::FontSize::Normal) ) ) );

				// Mixer falloff speed
				setMixerFalloffSpeed( LocalFileMng::readXmlFloat( guiNode, "mixer_falloff_speed",
																  InterfaceTheme::FALLOFF_NORMAL ) );

				// pattern editor grid resolution
				m_nPatternEditorGridResolution = LocalFileMng::readXmlInt( guiNode, "patternEditorGridResolution", m_nPatternEditorGridResolution );
				m_bPatternEditorUsingTriplets = LocalFileMng::readXmlBool( guiNode, "patternEditorUsingTriplets", m_bPatternEditorUsingTriplets );
				
				m_bShowInstrumentPeaks = LocalFileMng::readXmlBool( guiNode, "showInstrumentPeaks", m_bShowInstrumentPeaks );
				m_bIsFXTabVisible = LocalFileMng::readXmlBool( guiNode, "isFXTabVisible", m_bIsFXTabVisible );
				m_bShowAutomationArea = LocalFileMng::readXmlBool( guiNode, "showAutomationArea", m_bShowAutomationArea );
				m_bShowPlaybackTrack = LocalFileMng::readXmlBool( guiNode, "showPlaybackTrack", m_bShowPlaybackTrack );


				// pattern editor grid geometry
				m_nPatternEditorGridHeight = LocalFileMng::readXmlInt( guiNode, "patternEditorGridHeight", m_nPatternEditorGridHeight );
				m_nPatternEditorGridWidth = LocalFileMng::readXmlInt( guiNode, "patternEditorGridWidth", m_nPatternEditorGridWidth );

				// song editor grid geometry
				m_nSongEditorGridHeight = LocalFileMng::readXmlInt( guiNode, "songEditorGridHeight", m_nSongEditorGridHeight );
				m_nSongEditorGridWidth = LocalFileMng::readXmlInt( guiNode, "songEditorGridWidth", m_nSongEditorGridWidth );

				// mainForm window properties
				setMainFormProperties( readWindowProperties( guiNode, "mainForm_properties", mainFormProperties ) );
				setMixerProperties( readWindowProperties( guiNode, "mixer_properties", mixerProperties ) );
				setPatternEditorProperties( readWindowProperties( guiNode, "patternEditor_properties", patternEditorProperties ) );
				setSongEditorProperties( readWindowProperties( guiNode, "songEditor_properties", songEditorProperties ) );
				setInstrumentRackProperties( readWindowProperties( guiNode, "instrumentRack_properties", instrumentRackProperties ) );
				setAudioEngineInfoProperties( readWindowProperties( guiNode, "audioEngineInfo_properties", audioEngineInfoProperties ) );

				// last used file dialog folders
				m_sLastExportPatternAsDirectory = LocalFileMng::readXmlString( guiNode, "lastExportPatternAsDirectory", QDir::homePath() );
				m_sLastExportSongDirectory = LocalFileMng::readXmlString( guiNode, "lastExportSongDirectory", QDir::homePath() );
				m_sLastSaveSongAsDirectory = LocalFileMng::readXmlString( guiNode, "lastSaveSongAsDirectory", QDir::homePath() );
				m_sLastOpenSongDirectory = LocalFileMng::readXmlString( guiNode, "lastOpenSongDirectory", Filesystem::songs_dir() );
				m_sLastOpenPatternDirectory = LocalFileMng::readXmlString( guiNode, "lastOpenPatternDirectory", Filesystem::patterns_dir() );
				m_sLastExportLilypondDirectory = LocalFileMng::readXmlString( guiNode, "lastExportLilypondDirectory", QDir::homePath() );
				m_sLastExportMidiDirectory = LocalFileMng::readXmlString( guiNode, "lastExportMidiDirectory", QDir::homePath() );
				m_sLastImportDrumkitDirectory = LocalFileMng::readXmlString( guiNode, "lastImportDrumkitDirectory", QDir::homePath() );
				m_sLastExportDrumkitDirectory = LocalFileMng::readXmlString( guiNode, "lastExportDrumkitDirectory", QDir::homePath() );
				m_sLastOpenLayerDirectory = LocalFileMng::readXmlString( guiNode, "lastOpenLayerDirectory", QDir::homePath() );
				m_sLastOpenPlaybackTrackDirectory = LocalFileMng::readXmlString( guiNode, "lastOpenPlaybackTrackDirectory", QDir::homePath() );
				m_sLastAddSongToPlaylistDirectory = LocalFileMng::readXmlString( guiNode, "lastAddSongToPlaylistDirectory", Filesystem::songs_dir() );
				m_sLastPlaylistDirectory = LocalFileMng::readXmlString( guiNode, "lastPlaylistDirectory", Filesystem::playlists_dir() );
				m_sLastPlaylistScriptDirectory = LocalFileMng::readXmlString( guiNode, "lastPlaylistScriptDirectory", Filesystem::scripts_dir() );
				m_sLastImportThemeDirectory = LocalFileMng::readXmlString( guiNode, "lastImportThemeDirectory", QDir::homePath() );
				m_sLastExportThemeDirectory = LocalFileMng::readXmlString( guiNode, "lastExportThemeDirectory", QDir::homePath() );

				//export dialog properties
				m_nExportTemplateIdx = LocalFileMng::readXmlInt( guiNode, "exportDialogTemplate", 0 );
				m_nExportModeIdx = LocalFileMng::readXmlInt( guiNode, "exportDialogMode", 0 );
				m_nExportSampleRateIdx = LocalFileMng::readXmlInt( guiNode, "exportDialogSampleRate", 0 );
				m_nExportSampleDepthIdx = LocalFileMng::readXmlInt( guiNode, "exportDialogSampleDepth", 0 );
					
				m_bFollowPlayhead = LocalFileMng::readXmlBool( guiNode, "followPlayhead", true );

				// midi export dialog properties
				m_nMidiExportMode = LocalFileMng::readXmlInt( guiNode, "midiExportDialogMode", 0 );
				
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

				QDomNode pColorThemeNode = guiNode.firstChildElement( "colorTheme" );
				if ( !pColorThemeNode.isNull() ) {
					Theme::readColorTheme( pColorThemeNode, m_pTheme );
				} else {
					WARNINGLOG( "colorTheme node not found" );
					recreate = true;
				}

				//SongEditor coloring
				setColoringMethod( static_cast<InterfaceTheme::ColoringMethod>(LocalFileMng::readXmlInt( guiNode,
																										 "SongEditor_ColoringMethod",
																										 static_cast<int>(InterfaceTheme::ColoringMethod::Custom) )) );
				std::vector<QColor> colors( getMaxPatternColors() );
				for ( int ii = 0; ii < getMaxPatternColors(); ii++ ) {
					colors[ ii ] = LocalFileMng::readXmlColor( guiNode, QString( "SongEditor_pattern_color_%1" ).arg( ii ),
															   m_pTheme->getColorTheme()->m_accentColor );
				}
				setPatternColors( colors );
				setVisiblePatternColors( LocalFileMng::readXmlInt( guiNode, "SongEditor_visible_pattern_colors", 1 ) );
				if ( getVisiblePatternColors() > 50 ) {
					setVisiblePatternColors( 50 );
				} else if ( getVisiblePatternColors() < 0 ) {
					setVisiblePatternColors( 0 );
				}
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
						QString sAction = pMidiEventNode.firstChildElement("action").text();
						QString sParam = pMidiEventNode.firstChildElement("parameter").text();
						QString sParam2 = pMidiEventNode.firstChildElement("parameter2").text();
						QString sParam3 = pMidiEventNode.firstChildElement("parameter3").text();

						std::shared_ptr<Action> pAction = std::make_shared<Action>( sAction );
						pAction->setParameter1( sParam );
						pAction->setParameter2( sParam2 );
						pAction->setParameter3( sParam3 );
						mM->registerMMCEvent(event, pAction);
					}

					if( pMidiEventNode.firstChildElement().nodeName() == QString("noteEvent")){
						QString event = pMidiEventNode.firstChildElement("noteEvent").text();
						QString sAction = pMidiEventNode.firstChildElement("action").text();
						QString sParam = pMidiEventNode.firstChildElement("parameter").text();
						QString sParam2 = pMidiEventNode.firstChildElement("parameter2").text();
						QString sParam3 = pMidiEventNode.firstChildElement("parameter3").text();
						QString s_eventParameter = pMidiEventNode.firstChildElement("eventParameter").text();
						std::shared_ptr<Action> pAction = std::make_shared<Action>( sAction );
						pAction->setParameter1( sParam );
						pAction->setParameter2( sParam2 );
						pAction->setParameter3( sParam3 );
						mM->registerNoteEvent(s_eventParameter.toInt(), pAction);
					}

					if( pMidiEventNode.firstChildElement().nodeName() == QString("ccEvent") ){
						QString event = pMidiEventNode.firstChildElement("ccEvent").text();
						QString sAction = pMidiEventNode.firstChildElement("action").text();
						QString sParam = pMidiEventNode.firstChildElement("parameter").text();
						QString sParam2 = pMidiEventNode.firstChildElement("parameter2").text();
						QString sParam3 = pMidiEventNode.firstChildElement("parameter3").text();
						QString s_eventParameter = pMidiEventNode.firstChildElement("eventParameter").text();
						std::shared_ptr<Action> pAction = std::make_shared<Action>( sAction );
						pAction->setParameter1( sParam );
						pAction->setParameter2( sParam2 );
						pAction->setParameter3( sParam3 );
						mM->registerCCEvent( s_eventParameter.toInt(), pAction );
					}

					if( pMidiEventNode.firstChildElement().nodeName() == QString("pcEvent") ){
						QString event = pMidiEventNode.firstChildElement("pcEvent").text();
						QString sAction = pMidiEventNode.firstChildElement("action").text();
						QString sParam = pMidiEventNode.firstChildElement("parameter").text();
						QString sParam2 = pMidiEventNode.firstChildElement("parameter2").text();
						QString sParam3 = pMidiEventNode.firstChildElement("parameter3").text();
						std::shared_ptr<Action> pAction = std::make_shared<Action>( sAction );
						pAction->setParameter1( sParam );
						pAction->setParameter2( sParam2 );
						pAction->setParameter3( sParam3 );
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

	if ( m_nMaxLayers < 16 ) {
		m_nMaxLayers = 16;
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
	QString sPreferencesFilename;
	const QString sPreferencesOverwritePath = Filesystem::getPreferencesOverwritePath();
	if ( sPreferencesOverwritePath.isEmpty() ) {
		sPreferencesFilename = Filesystem::usr_config_path();
	} else {
		sPreferencesFilename = sPreferencesOverwritePath;
	}

	INFOLOG( QString( "Saving preferences file %1" ).arg( sPreferencesFilename ) );

	QDomDocument doc;
	QDomProcessingInstruction header = doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	doc.appendChild( header );

	QDomNode rootNode = doc.createElement( "hydrogen_preferences" );

	// hydrogen version
	LocalFileMng::writeXmlString( rootNode, "version", QString( get_version().c_str() ) );

	////// GENERAL ///////
	LocalFileMng::writeXmlString( rootNode, "preferredLanguage", m_sPreferredLanguage );
	LocalFileMng::writeXmlString( rootNode, "restoreLastSong", m_brestoreLastSong ? "true": "false" );
	LocalFileMng::writeXmlString( rootNode, "restoreLastPlaylist", m_brestoreLastPlaylist ? "true": "false" );

	LocalFileMng::writeXmlString( rootNode, "patternModePlaysSelected", m_bPatternModePlaysSelected ? "true": "false" );

	LocalFileMng::writeXmlString( rootNode, "useLash", m_bsetLash ? "true": "false" );
	LocalFileMng::writeXmlString( rootNode, "useTimeLine", __useTimelineBpm ? "true": "false" );

	LocalFileMng::writeXmlString( rootNode, "maxBars", QString::number( m_nMaxBars ) );
	LocalFileMng::writeXmlString( rootNode, "maxLayers", QString::number( m_nMaxLayers ) );

	LocalFileMng::writeXmlString( rootNode, "defaultUILayout", QString::number( static_cast<int>(getDefaultUILayout()) ) );
	LocalFileMng::writeXmlString( rootNode, "uiScalingPolicy", QString::number( static_cast<int>(getUIScalingPolicy()) ) );
	LocalFileMng::writeXmlString( rootNode, "lastOpenTab", QString::number( m_nLastOpenTab ) );

	LocalFileMng::writeXmlString( rootNode, "useTheRubberbandBpmChangeEvent", m_useTheRubberbandBpmChangeEvent ? "true": "false" );

	LocalFileMng::writeXmlString( rootNode, "useRelativeFilenamesForPlaylists", m_bUseRelativeFilenamesForPlaylists ? "true": "false" );
	LocalFileMng::writeXmlBool( rootNode, "hideKeyboardCursorWhenUnused", m_bHideKeyboardCursor );
	LocalFileMng::writeXmlBool( rootNode, "patternFollowsSong", m_bPatternFollowsSong );
	
	// instrument input mode
	LocalFileMng::writeXmlString( rootNode, "instrumentInputMode", __playselectedinstrument ? "true": "false" );
	
	//show development version warning
	LocalFileMng::writeXmlString( rootNode, "showDevelWarning", m_bShowDevelWarning ? "true": "false" );

	// Warn about overwriting notes
	LocalFileMng::writeXmlString( rootNode, "showNoteOverwriteWarning", m_bShowNoteOverwriteWarning ? "true" : "false" );

	// hear new notes in the pattern editor
	LocalFileMng::writeXmlString( rootNode, "hearNewNotes", hearNewNotes ? "true": "false" );

	// key/midi event prefs
	//LocalFileMng::writeXmlString( rootNode, "recordEvents", recordEvents ? "true": "false" );
	LocalFileMng::writeXmlString( rootNode, "quantizeEvents", quantizeEvents ? "true": "false" );

	//extern executables
	if ( !Filesystem::file_executable( m_rubberBandCLIexecutable , true /* silent */) ) {
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

		//// PORTAUDIO DRIVER ////
		QDomNode portAudioDriverNode = doc.createElement( "portaudio_driver" );
		{
			LocalFileMng::writeXmlString( portAudioDriverNode, "portAudioDevice", m_sPortAudioDevice );
			LocalFileMng::writeXmlString( portAudioDriverNode, "portAudioHostAPI", m_sPortAudioHostAPI );
			LocalFileMng::writeXmlString( portAudioDriverNode, "latencyTarget", QString("%1").arg( m_nLatencyTarget ) );
		}
		audioEngineNode.appendChild( portAudioDriverNode );

		//// COREAUDIO DRIVER ////
		QDomNode coreAudioDriverNode = doc.createElement( "coreaudio_driver" );
		{
			LocalFileMng::writeXmlString( coreAudioDriverNode, "coreAudioDevice", m_sCoreAudioDevice );
		}
		audioEngineNode.appendChild( coreAudioDriverNode );

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
			LocalFileMng::writeXmlBool( jackDriverNode, "jack_timebase_enabled", m_bJackTimebaseEnabled );
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

			int nJackTrackOutputMode;
			if ( m_JackTrackOutputMode == JackTrackOutputMode::postFader ) {
				nJackTrackOutputMode = 0;
			} else if ( m_JackTrackOutputMode == JackTrackOutputMode::preFader ) {
				nJackTrackOutputMode = 1;
			}
			LocalFileMng::writeXmlString( jackDriverNode, "jack_track_output_mode", QString("%1").arg( nJackTrackOutputMode ));

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
			LocalFileMng::writeXmlString( midiDriverNode, "output_port_name", m_sMidiOutputPortName );
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
			
			if ( m_bOscFeedbackEnabled ) {
				LocalFileMng::writeXmlString( oscNode, "oscFeedbackEnabled", "true" );
			} else {
				LocalFileMng::writeXmlString( oscNode, "oscFeedbackEnabled", "false" );
			}
		}
		audioEngineNode.appendChild( oscNode );
		
	}
	rootNode.appendChild( audioEngineNode );

	//---- GUI ----
	QDomNode guiNode = doc.createElement( "gui" );
	{
		LocalFileMng::writeXmlString( guiNode, "QTStyle", getQTStyle() );
		LocalFileMng::writeXmlString( guiNode, "application_font_family", getApplicationFontFamily() );
		LocalFileMng::writeXmlString( guiNode, "level2_font_family", getLevel2FontFamily() );
		LocalFileMng::writeXmlString( guiNode, "level3_font_family", getLevel3FontFamily() );
		LocalFileMng::writeXmlString( guiNode, "font_size", QString::number( static_cast<int>(getFontSize()) ) );
		LocalFileMng::writeXmlString( guiNode, "mixer_falloff_speed", QString("%1").arg( getMixerFalloffSpeed() ) );
		LocalFileMng::writeXmlString( guiNode, "patternEditorGridResolution", QString("%1").arg( m_nPatternEditorGridResolution ) );
		LocalFileMng::writeXmlString( guiNode, "patternEditorGridHeight", QString("%1").arg( m_nPatternEditorGridHeight ) );
		LocalFileMng::writeXmlString( guiNode, "patternEditorGridWidth", QString("%1").arg( m_nPatternEditorGridWidth ) );
		LocalFileMng::writeXmlBool( guiNode, "patternEditorUsingTriplets", m_bPatternEditorUsingTriplets );
		LocalFileMng::writeXmlString( guiNode, "songEditorGridHeight", QString("%1").arg( m_nSongEditorGridHeight ) );
		LocalFileMng::writeXmlString( guiNode, "songEditorGridWidth", QString("%1").arg( m_nSongEditorGridWidth ) );
		LocalFileMng::writeXmlBool( guiNode, "showInstrumentPeaks", m_bShowInstrumentPeaks );
		LocalFileMng::writeXmlBool( guiNode, "isFXTabVisible", m_bIsFXTabVisible );
		LocalFileMng::writeXmlBool( guiNode, "showAutomationArea", m_bShowAutomationArea );
		LocalFileMng::writeXmlBool( guiNode, "showPlaybackTrack", m_bShowPlaybackTrack );

		// MainForm window properties
		writeWindowProperties( guiNode, "mainForm_properties", mainFormProperties );
		writeWindowProperties( guiNode, "mixer_properties", mixerProperties );
		writeWindowProperties( guiNode, "patternEditor_properties", patternEditorProperties );
		writeWindowProperties( guiNode, "songEditor_properties", songEditorProperties );
		writeWindowProperties( guiNode, "instrumentRack_properties", instrumentRackProperties );
		writeWindowProperties( guiNode, "audioEngineInfo_properties", audioEngineInfoProperties );
		for ( unsigned nFX = 0; nFX < MAX_FX; nFX++ ) {
			QString sNode = QString("ladspaFX_properties%1").arg( nFX );
			writeWindowProperties( guiNode, sNode, m_ladspaProperties[nFX] );
		}
		
		// last used file dialog folders
		LocalFileMng::writeXmlString( guiNode, "lastExportPatternAsDirectory", m_sLastExportPatternAsDirectory );
		LocalFileMng::writeXmlString( guiNode, "lastExportSongDirectory", m_sLastExportSongDirectory );
		LocalFileMng::writeXmlString( guiNode, "lastSaveSongAsDirectory", m_sLastSaveSongAsDirectory );
		LocalFileMng::writeXmlString( guiNode, "lastOpenSongDirectory", m_sLastOpenSongDirectory );
		LocalFileMng::writeXmlString( guiNode, "lastOpenPatternDirectory", m_sLastOpenPatternDirectory );
		LocalFileMng::writeXmlString( guiNode, "lastExportLilypondDirectory", m_sLastExportLilypondDirectory );
		LocalFileMng::writeXmlString( guiNode, "lastExportMidiDirectory", m_sLastExportMidiDirectory );
		LocalFileMng::writeXmlString( guiNode, "lastImportDrumkitDirectory", m_sLastImportDrumkitDirectory );
		LocalFileMng::writeXmlString( guiNode, "lastExportDrumkitDirectory", m_sLastExportDrumkitDirectory );
		LocalFileMng::writeXmlString( guiNode, "lastOpenLayerDirectory", m_sLastOpenLayerDirectory );
		LocalFileMng::writeXmlString( guiNode, "lastOpenPlaybackTrackDirectory", m_sLastOpenPlaybackTrackDirectory );
		LocalFileMng::writeXmlString( guiNode, "lastAddSongToPlaylistDirectory", m_sLastAddSongToPlaylistDirectory );
		LocalFileMng::writeXmlString( guiNode, "lastPlaylistDirectory", m_sLastPlaylistDirectory );
		LocalFileMng::writeXmlString( guiNode, "lastPlaylistScriptDirectory", m_sLastPlaylistScriptDirectory );
		LocalFileMng::writeXmlString( guiNode, "lastImportThemeDirectory", m_sLastImportThemeDirectory );
		LocalFileMng::writeXmlString( guiNode, "lastExportThemeDirectory", m_sLastExportThemeDirectory );
				
		//ExportSongDialog
		LocalFileMng::writeXmlString( guiNode, "exportDialogMode", QString("%1").arg( m_nExportModeIdx ) );
		LocalFileMng::writeXmlString( guiNode, "exportDialogTemplate", QString("%1").arg( m_nExportTemplateIdx ) );
		LocalFileMng::writeXmlString( guiNode, "exportDialogSampleRate",  QString("%1").arg( m_nExportSampleRateIdx ) );
		LocalFileMng::writeXmlString( guiNode, "exportDialogSampleDepth", QString("%1").arg( m_nExportSampleDepthIdx ) );

		LocalFileMng::writeXmlBool( guiNode, "followPlayhead", m_bFollowPlayhead );

		//ExportMidiDialog
		LocalFileMng::writeXmlString( guiNode, "midiExportDialogMode", QString("%1").arg( m_nMidiExportMode ) );

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
		Theme::writeColorTheme( &guiNode, m_pTheme );

		//SongEditor coloring method
		LocalFileMng::writeXmlString( guiNode, "SongEditor_ColoringMethod", QString::number( static_cast<int>(getColoringMethod()) ) );
		for ( int ii = 0; ii < getMaxPatternColors(); ii++ ) {
			LocalFileMng::writeXmlColor( guiNode, QString( "SongEditor_pattern_color_%1" ).arg( ii ), getPatternColors()[ ii ] );
		}
		LocalFileMng::writeXmlString( guiNode, "SongEditor_visible_pattern_colors", QString::number( getVisiblePatternColors() ) );
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
	auto mmcMap = mM->getMMCMap();

	//---- MidiMap ----
	QDomNode midiEventMapNode = doc.createElement( "midiEventMap" );

	for( const auto& it : mmcMap ){
		QString event = it.first;
		auto pAction = it.second;
		if ( pAction->getType() != "NOTHING" ){
			QDomNode midiEventNode = doc.createElement( "midiEvent" );

			LocalFileMng::writeXmlString( midiEventNode, "mmcEvent" , event );
			LocalFileMng::writeXmlString( midiEventNode, "action" , pAction->getType());
			LocalFileMng::writeXmlString( midiEventNode, "parameter" , pAction->getParameter1() );
			LocalFileMng::writeXmlString( midiEventNode, "parameter2" , pAction->getParameter2() );
			LocalFileMng::writeXmlString( midiEventNode, "parameter3" , pAction->getParameter3() );

			midiEventMapNode.appendChild( midiEventNode );
		}
	}

	for( int note=0; note < 128; note++ ){
		auto pAction = mM->getNoteAction( note );
		if( pAction != nullptr && pAction->getType() != "NOTHING") {
			QDomNode midiEventNode = doc.createElement( "midiEvent" );

			LocalFileMng::writeXmlString( midiEventNode, "noteEvent" , QString("NOTE") );
			LocalFileMng::writeXmlString( midiEventNode, "eventParameter" , QString::number( note ) );
			LocalFileMng::writeXmlString( midiEventNode, "action" , pAction->getType() );
			LocalFileMng::writeXmlString( midiEventNode, "parameter" , pAction->getParameter1() );
			LocalFileMng::writeXmlString( midiEventNode, "parameter2" , pAction->getParameter2() );
			LocalFileMng::writeXmlString( midiEventNode, "parameter3" , pAction->getParameter3() );
			midiEventMapNode.appendChild( midiEventNode );
		}
	}

	for( int parameter=0; parameter < 128; parameter++ ){
		auto pAction = mM->getCCAction( parameter );
		if( pAction != nullptr && pAction->getType() != "NOTHING") {
			QDomNode midiEventNode = doc.createElement( "midiEvent" );

			LocalFileMng::writeXmlString( midiEventNode, "ccEvent" , QString("CC") );
			LocalFileMng::writeXmlString( midiEventNode, "eventParameter" , QString::number( parameter ) );
			LocalFileMng::writeXmlString( midiEventNode, "action" , pAction->getType() );
			LocalFileMng::writeXmlString( midiEventNode, "parameter" , pAction->getParameter1() );
			LocalFileMng::writeXmlString( midiEventNode, "parameter2" , pAction->getParameter2() );
			LocalFileMng::writeXmlString( midiEventNode, "parameter3" , pAction->getParameter3() );
			midiEventMapNode.appendChild( midiEventNode );
		}
	}

	{
		auto pAction = mM->getPCAction();
		if( pAction != nullptr && pAction->getType() != "NOTHING") {
			QDomNode midiEventNode = doc.createElement( "midiEvent" );

			LocalFileMng::writeXmlString( midiEventNode, "pcEvent" , QString("PROGRAM_CHANGE") );
			LocalFileMng::writeXmlString( midiEventNode, "action" , pAction->getType() );
			LocalFileMng::writeXmlString( midiEventNode, "parameter" , pAction->getParameter1() );
			LocalFileMng::writeXmlString( midiEventNode, "parameter2" , pAction->getParameter2() );
			LocalFileMng::writeXmlString( midiEventNode, "parameter3" , pAction->getParameter3() );
			midiEventMapNode.appendChild( midiEventNode );
		}
	}

	rootNode.appendChild( midiEventMapNode );

	doc.appendChild( rootNode );

	QFile file( sPreferencesFilename );
	if ( !file.open(QIODevice::WriteOnly) ) {
		return;
	}

	QTextStream TextStream( &file );
	doc.save( TextStream, 1 );

	file.close();
}

void Preferences::setMostRecentFX( QString FX_name )
{
	int pos = m_recentFX.indexOf( FX_name );

	if ( pos != -1 ) {
		m_recentFX.removeAt( pos );
	}

	m_recentFX.push_front( FX_name );
}

void Preferences::insertRecentFile( const QString sFilename ){

	bool bAlreadyContained =
		std::find( m_recentFiles.begin(), m_recentFiles.end(),
				   sFilename ) != m_recentFiles.end();
	
	m_recentFiles.insert( m_recentFiles.begin(), sFilename );

	if ( bAlreadyContained ) {
		// Eliminate all duplicates in the list while keeping the one
		// inserted at the beginning.
		setRecentFiles( m_recentFiles );
	}
}

void Preferences::setRecentFiles( const std::vector<QString> recentFiles )
{
	// find single filenames. (skip duplicates)
	std::vector<QString> sTmpVec;
	for ( const auto& ssFilename : recentFiles ) {
		if ( std::find( sTmpVec.begin(), sTmpVec.end(), ssFilename) ==
			 sTmpVec.end() ) {
			// Particular file is not contained yet.
			sTmpVec.push_back( ssFilename );
		}
	}

	m_recentFiles = sTmpVec;
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
		prop.m_geometry = QByteArray::fromBase64( LocalFileMng::readXmlString( windowPropNode, "geometry",
																			   prop.m_geometry.toBase64() )
												  .toUtf8() );
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
	LocalFileMng::writeXmlString( windowPropNode, "geometry", QString( prop.m_geometry.toBase64() ) );

	parent.appendChild( windowPropNode );
}

// -----------------------



WindowProperties::WindowProperties()
{
//	infoLog( "INIT" );
	x = 0;
	y = 0;
	width = 0;
	height = 0;
	visible = true;
}


WindowProperties::WindowProperties(const WindowProperties & other)
		: x(other.x),
		y(other.y),
		width(other.width),
		height(other.height),
		visible(other.visible)
{
//	infoLog( "INIT" );
}



WindowProperties::~WindowProperties()
{
//	infoLog( "DESTROY" );
}

};
