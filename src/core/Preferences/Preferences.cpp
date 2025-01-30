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

#include <stdlib.h>
#include <core/Preferences/Preferences.h>

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
#include <core/Helpers/Xml.h>
#include <core/IO/AlsaAudioDriver.h>

#include <QDir>
#include <QProcess>
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
	m_exportFormat = Filesystem::AudioFormat::Flac;
	m_fExportCompressionLevel = 0.0;
	m_bShowExportSongLicenseWarning = true;
	m_bShowExportDrumkitLicenseWarning = true;
	m_bShowExportDrumkitCopyleftWarning = true;
	m_bShowExportDrumkitAttributionWarning = true;

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
	m_nAutosavesPerHour = 60;
	m_patternCategories.push_back( QString("not_categorized") );

	//___ audio engine properties ___
	m_audioDriver = AudioDriver::Auto;
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
	m_sMidiPortName = QString( Preferences::getNullMidiPort() );
	m_sMidiOutputPortName = QString( Preferences::getNullMidiPort() );
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

#ifdef H2CORE_HAVE_ALSA
	// Ensure the device read from the local preferences does
	// exist. If not, we try to replace it with a valid one.
	QStringList alsaDevices = AlsaAudioDriver::getDevices();
	if ( alsaDevices.size() == 0 ||
		 alsaDevices.contains( "hw:0" ) ) {
		m_sAlsaAudioDevice = "hw:0";
	} else {
		// Fall back to a device found on the system (but not the
		// "null" one).
		if ( alsaDevices[ 0 ] != "null" ) {
			m_sAlsaAudioDevice = alsaDevices[ 0 ];
		} else if ( alsaDevices.size() > 1 ) {
			m_sAlsaAudioDevice = alsaDevices[ 1 ];
		} else {
			m_sAlsaAudioDevice = "hw:0";
		}
	}
#else
	m_sAlsaAudioDevice = "hw:0";
#endif

	//___  jack driver properties ___
	m_sJackPortName1 = QString("alsa_pcm:playback_1");
	m_sJackPortName2 = QString("alsa_pcm:playback_2");
	m_bJackTransportMode = true;
	m_bJackConnectDefaults = true;
	m_bJackTrackOuts = false;
	m_bJackTimebaseEnabled = false;
	m_bJackTimebaseMode = NO_JACK_TIMEBASE_CONTROL;
	m_JackTrackOutputMode = JackTrackOutputMode::postFader;

	// OSC configuration
	m_bOscServerEnabled = false;
	m_bOscFeedbackEnabled = true;
	m_nOscServerPort = 9000;
	m_nOscTemporaryPort = -1;

	//___ General properties ___
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
	m_playlistDialogProperties.set(200, 300, 921, 703, false);
	m_directorProperties.set(200, 300, 423, 377, false);
	m_ladspaProperties[0].set(2, 20, 0, 0, false);
	m_ladspaProperties[1].set(2, 20, 0, 0, false);
	m_ladspaProperties[2].set(2, 20, 0, 0, false);
	m_ladspaProperties[3].set(2, 20, 0, 0, false);
	m_nMaxBars = 400;
	m_nMaxLayers = 16;

	/////////////////////////////////////////////////////////////////////////
	//////////////// END OF DEFAULT SETTINGS ////////////////////////////////
	/////////////////////////////////////////////////////////////////////////

	const bool bGlobalPrefLoaded = loadPreferences( true );
	const bool bUserPrefLoaded = loadPreferences( false );
	if ( bGlobalPrefLoaded || bUserPrefLoaded ) {
		m_bLoadingSuccessful = true;
	} else {
		m_bLoadingSuccessful = false;
	}
		
}



Preferences::~Preferences()
{
	INFOLOG( "DESTROY" );
	__instance = nullptr;
}

///
/// Load the preferences file
///
bool Preferences::loadPreferences( bool bGlobal )
{
	// We do not required the recently used variables to be
	// accumulated throughout various configuration files.
	m_recentFiles.clear();
	m_recentFX.clear();

	bool bRecreate = false;	// configuration file must be recreated?

	QString sPreferencesFilename =
		( bGlobal ? Filesystem::sys_config_path() : Filesystem::usr_config_path() );
	INFOLOG( QString( "Loading preferences file [%1]" )
			 .arg( sPreferencesFilename ) );

	if ( ! Filesystem::file_readable( sPreferencesFilename, true ) ) {
		if ( bGlobal ) {
			ERRORLOG( QString( "Global preferences file [%1] is not readable!" )
					  .arg( sPreferencesFilename ) );
			return false;
		}
		else {
			WARNINGLOG( QString( "User-level preferences file [%1] is not readable! It will be recreated." )
						.arg( sPreferencesFilename ) );
			bRecreate = true;
		}
	}
	else {
		// Preferences is readable.

		XMLDoc doc;
		doc.read( sPreferencesFilename, nullptr, false );
		XMLNode rootNode = doc.firstChildElement( "hydrogen_preferences" );

		if ( !rootNode.isNull() ) {

			// version
			QString version = rootNode.read_string( "version", "", false, false );
			if ( version.isEmpty() ) {
				bRecreate = true;
			}

			//////// GENERAL ///////////
			m_sPreferredLanguage = rootNode.read_string( "preferredLanguage", m_sPreferredLanguage, false, "" );
			__playselectedinstrument = rootNode.read_bool( "instrumentInputMode", __playselectedinstrument, false, false );
			m_bShowDevelWarning = rootNode.read_bool( "showDevelWarning", m_bShowDevelWarning, false, false );
			m_bShowNoteOverwriteWarning = rootNode.read_bool( "showNoteOverwriteWarning", m_bShowNoteOverwriteWarning, false, false );
			m_brestoreLastSong = rootNode.read_bool( "restoreLastSong", m_brestoreLastSong, false, false );
			m_brestoreLastPlaylist = rootNode.read_bool( "restoreLastPlaylist", m_brestoreLastPlaylist, false, false );
			m_bUseLash = rootNode.read_bool( "useLash", false, false, false );
			__useTimelineBpm = rootNode.read_bool( "useTimeLine", __useTimelineBpm, false, false );
			m_nMaxBars = rootNode.read_int( "maxBars", 400, false, false );
			m_nMaxLayers = rootNode.read_int( "maxLayers", 16, false, false );
			setDefaultUILayout( static_cast<InterfaceTheme::Layout>(
				rootNode.read_int( "defaultUILayout",
								   static_cast<int>(InterfaceTheme::Layout::SinglePane), false, false )) );
			setUIScalingPolicy( static_cast<InterfaceTheme::ScalingPolicy>(
				rootNode.read_int( "uiScalingPolicy",
								   static_cast<int>(InterfaceTheme::ScalingPolicy::Smaller), false, false )) );
			m_nLastOpenTab = rootNode.read_int( "lastOpenTab", 0, false, false );
			m_bUseRelativeFilenamesForPlaylists = rootNode.read_bool( "useRelativeFilenamesForPlaylists", false, false, false );
			m_bHideKeyboardCursor = rootNode.read_bool( "hideKeyboardCursorWhenUnused", false, false, false );

			//restore the right m_bsetlash value
			m_bsetLash = m_bUseLash;
			m_useTheRubberbandBpmChangeEvent = rootNode.read_bool( "useTheRubberbandBpmChangeEvent", m_useTheRubberbandBpmChangeEvent, false, false );

			hearNewNotes = rootNode.read_bool( "hearNewNotes", hearNewNotes, false, false );
			quantizeEvents = rootNode.read_bool( "quantizeEvents", quantizeEvents, false, false );

			//rubberband
			if( readPrefFileforotherplaces ){
				//this scond test will check individual user settings
				QString test = rootNode.read_string( "path_to_rubberband", "", false, false );
				if ( QFile( test ).exists() == true ){
					m_rubberBandCLIexecutable = test;
				}else
					{
						m_rubberBandCLIexecutable = "Path to Rubberband-CLI";
					}
			}

			XMLNode recentUsedSongsNode = rootNode.firstChildElement( "recentUsedSongs" );
			if ( ! recentUsedSongsNode.isNull() ) {
				QDomElement songElement = recentUsedSongsNode.firstChildElement( "song" );
				while( !songElement.isNull() && ! songElement.text().isEmpty() ){
					m_recentFiles.push_back( songElement.text() );
					songElement = songElement.nextSiblingElement( "song" );
				}

			} else {
				WARNINGLOG( "recentUsedSongs node not found" );
			}

			XMLNode recentFXNode = rootNode.firstChildElement( "recentlyUsedEffects" );
			if ( ! recentFXNode.isNull() ) {
				QDomElement fxElement = recentFXNode.firstChildElement( "FX" );
				while ( !fxElement.isNull()  && ! fxElement.text().isEmpty()) {
					m_recentFX.push_back( fxElement.text() );
					fxElement = fxElement.nextSiblingElement( "FX" );
				}
			} else {
				WARNINGLOG( "recentlyUsedEffects node not found" );
			}

			sServerList.clear();
			XMLNode serverListNode = rootNode.firstChildElement( "serverList" );
			if ( ! serverListNode.isNull() ) {
				QDomElement serverElement = serverListNode.firstChildElement( "server" );
				while ( !serverElement.isNull() && !serverElement.text().isEmpty() ) {
					sServerList.push_back( serverElement.text() );
					serverElement = serverElement.nextSiblingElement( "server" );
				}
			} else {
				WARNINGLOG( "serverList node not found" );
			}

			m_patternCategories.clear();
			XMLNode patternCategoriesNode = rootNode.firstChildElement( "patternCategories" );
			if ( ! patternCategoriesNode.isNull() ) {
				QDomElement patternCategoriesElement = patternCategoriesNode.firstChildElement( "categories" );
				while ( !patternCategoriesElement.isNull() && !patternCategoriesElement.text().isEmpty() ) {
					m_patternCategories.push_back( patternCategoriesElement.text() );
					patternCategoriesElement = patternCategoriesElement.nextSiblingElement( "categories" );
				}
			} else {
				WARNINGLOG( "patternCategories node not found" );
			}


			/////////////// AUDIO ENGINE //////////////
			XMLNode audioEngineNode = rootNode.firstChildElement( "audio_engine" );
			if ( audioEngineNode.isNull() ) {
				WARNINGLOG( "audio_engine node not found" );
				bRecreate = true;
			} else {
				const QString sAudioDriver = audioEngineNode.read_string(
					"audio_driver", Preferences::audioDriverToQString(
						Preferences::AudioDriver::Auto ),
					false, false );
				m_audioDriver = parseAudioDriver( sAudioDriver );
				if ( m_audioDriver == AudioDriver::None ) {
					WARNINGLOG( "Falling back to 'Auto' audio driver" );
					m_audioDriver = AudioDriver::Auto;
				}
				m_bUseMetronome = audioEngineNode.read_bool( "use_metronome", m_bUseMetronome, false, false );
				m_fMetronomeVolume = audioEngineNode.read_float( "metronome_volume", 0.5f, false, false );
				m_nMaxNotes = audioEngineNode.read_int( "maxNotes", m_nMaxNotes, false, false );
				m_nBufferSize = audioEngineNode.read_int( "buffer_size", m_nBufferSize, false, false );
				m_nSampleRate = audioEngineNode.read_int( "samplerate", m_nSampleRate, false, false );

				//// OSS DRIVER ////
				XMLNode ossDriverNode = audioEngineNode.firstChildElement( "oss_driver" );
				if ( ossDriverNode.isNull()  ) {
					WARNINGLOG( "oss_driver node not found" );
					bRecreate = true;
				} else {
					m_sOSSDevice = ossDriverNode.read_string( "ossDevice", m_sOSSDevice, false, false );
				}

				//// PORTAUDIO DRIVER ////
				XMLNode portAudioDriverNode = audioEngineNode.firstChildElement( "portaudio_driver" );
				if ( portAudioDriverNode.isNull()  ) {
					WARNINGLOG( "portaudio_driver node not found" );
					bRecreate = true;
				} else {
					m_sPortAudioDevice = portAudioDriverNode.read_string( "portAudioDevice", m_sPortAudioDevice, false, true );
					m_sPortAudioHostAPI = portAudioDriverNode.read_string( "portAudioHostAPI", m_sPortAudioHostAPI, false, true );
					m_nLatencyTarget = portAudioDriverNode.read_int( "latencyTarget", m_nLatencyTarget, false, false );
				}

				//// COREAUDIO DRIVER ////
				XMLNode coreAudioDriverNode = audioEngineNode.firstChildElement( "coreaudio_driver" );
				if ( coreAudioDriverNode.isNull()  ) {
					WARNINGLOG( "coreaudio_driver node not found" );
					bRecreate = true;
				} else {
					m_sCoreAudioDevice = coreAudioDriverNode.read_string( "coreAudioDevice", m_sCoreAudioDevice, false, true );
				}

				//// JACK DRIVER ////
				XMLNode jackDriverNode = audioEngineNode.firstChildElement( "jack_driver" );
				if ( jackDriverNode.isNull() ) {
					WARNINGLOG( "jack_driver node not found" );
					bRecreate = true;
				} else {
					m_sJackPortName1 = jackDriverNode.read_string( "jack_port_name_1", m_sJackPortName1, false, false );
					m_sJackPortName2 = jackDriverNode.read_string( "jack_port_name_2", m_sJackPortName2, false, false );
					QString sMode = jackDriverNode.read_string( "jack_transport_mode", "NO_JACK_TRANSPORT", false, false );
					if ( sMode == "NO_JACK_TRANSPORT" ) {
						m_bJackTransportMode = NO_JACK_TRANSPORT;
					} else if ( sMode == "USE_JACK_TRANSPORT" ) {
						m_bJackTransportMode = USE_JACK_TRANSPORT;
					}

					// We stick to the old Timebase strings (? why strings for a
					// boolean option?) for backward and forward compatibility
					// of old versions still in use.
					m_bJackTimebaseEnabled = jackDriverNode.read_bool( "jack_timebase_enabled", false, false, false );
					QString tmMode = jackDriverNode.read_string( "jack_transport_mode_master", "NO_JACK_TIME_MASTER", false, false );
					if ( tmMode == "NO_JACK_TIME_MASTER" ) {
						m_bJackTimebaseMode = NO_JACK_TIMEBASE_CONTROL;
					} else if ( tmMode == "USE_JACK_TIME_MASTER" ) {
						m_bJackTimebaseMode = USE_JACK_TIMEBASE_CONTROL;
					}

					m_bJackTrackOuts = jackDriverNode.read_bool( "jack_track_outs", m_bJackTrackOuts, false, false );
					m_bJackConnectDefaults = jackDriverNode.read_bool( "jack_connect_defaults", m_bJackConnectDefaults, false, false );

					int nJackTrackOutputMode = jackDriverNode.read_int( "jack_track_output_mode", 0, false, false );
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
				XMLNode alsaAudioDriverNode = audioEngineNode.firstChildElement( "alsa_audio_driver" );
				if ( alsaAudioDriverNode.isNull() ) {
					WARNINGLOG( "alsa_audio_driver node not found" );
					bRecreate = true;
				} else {
					m_sAlsaAudioDevice = alsaAudioDriverNode.read_string( "alsa_audio_device", m_sAlsaAudioDevice, false, false );
				}

				/// MIDI DRIVER ///
				XMLNode midiDriverNode = audioEngineNode.firstChildElement( "midi_driver" );
				if ( midiDriverNode.isNull() ) {
					WARNINGLOG( "midi_driver node not found" );
					bRecreate = true;
				} else {
					m_sMidiDriver = midiDriverNode.read_string(
						"driverName", m_sMidiDriver, false, false );
					// Ensure compatibility will older versions of the
					// files after capitalization in the GUI
					// (2021-02-05). This can be dropped in releases
					// >= 1.2
					if ( m_sMidiDriver == "JackMidi" ) {
						m_sMidiDriver = "JACK-MIDI";
					} else if ( m_sMidiDriver == "CoreMidi" ) {
						m_sMidiDriver = "CoreMIDI";
					}
					m_sMidiPortName = midiDriverNode.read_string(
						"port_name", Preferences::getNullMidiPort(), false, false );
					m_sMidiOutputPortName = midiDriverNode.read_string(
						"output_port_name", Preferences::getNullMidiPort(), false, false );
					m_nMidiChannelFilter = midiDriverNode.read_int( "channel_filter", -1, false, false );
					m_bMidiNoteOffIgnore = midiDriverNode.read_bool( "ignore_note_off", true, false, false );
					m_bMidiDiscardNoteAfterAction = midiDriverNode.read_bool( "discard_note_after_action", true, false, false );
					m_bMidiFixedMapping = midiDriverNode.read_bool( "fixed_mapping", false, false, true );
					m_bEnableMidiFeedback = midiDriverNode.read_bool( "enable_midi_feedback", false, false, true );
				}

				/// OSC ///
				XMLNode oscServerNode = audioEngineNode.firstChildElement( "osc_configuration" );
				if ( oscServerNode.isNull() ) {
					WARNINGLOG( "osc_configuration node not found" );
					bRecreate = true;
				} else {
					m_bOscServerEnabled = oscServerNode.read_bool( "oscEnabled", false, false, false );
					m_bOscFeedbackEnabled = oscServerNode.read_bool( "oscFeedbackEnabled", true, false, false );
					m_nOscServerPort = oscServerNode.read_int( "oscServerPort", 9000, false, false );
				}
			}

			/////////////// GUI //////////////
			XMLNode guiNode = rootNode.firstChildElement( "gui" );
			if ( guiNode.isNull() ) {
				WARNINGLOG( "gui node not found" );
				bRecreate = true;
			} else {
				// QT Style
				setQTStyle( guiNode.read_string( "QTStyle", "Fusion", false, true ) );

				if ( getQTStyle() == "Plastique" ){
					setQTStyle( "Fusion" );
				}

				// Font fun
				setApplicationFontFamily( guiNode.read_string( "application_font_family", getApplicationFontFamily(), false, false ) );
				// The value defaults to m_sApplicationFontFamily on
				// purpose to provide backward compatibility.
				setLevel2FontFamily( guiNode.read_string( "level2_font_family", getLevel2FontFamily(), false, false ) );
				setLevel3FontFamily( guiNode.read_string( "level3_font_family", getLevel3FontFamily(), false, false ) );
				setFontSize( static_cast<FontTheme::FontSize>(
					guiNode.read_int( "font_size",
									  static_cast<int>(FontTheme::FontSize::Medium), false, false ) ) );

				// Mixer falloff speed
				setMixerFalloffSpeed( guiNode.read_float( "mixer_falloff_speed",
														 InterfaceTheme::FALLOFF_NORMAL, false, false ) );

				// pattern editor grid resolution
				m_nPatternEditorGridResolution = guiNode.read_int( "patternEditorGridResolution", m_nPatternEditorGridResolution, false, false );
				m_bPatternEditorUsingTriplets = guiNode.read_bool( "patternEditorUsingTriplets", m_bPatternEditorUsingTriplets, false, false );
				
				m_bShowInstrumentPeaks = guiNode.read_bool( "showInstrumentPeaks", m_bShowInstrumentPeaks, false, false );
				m_bIsFXTabVisible = guiNode.read_bool( "isFXTabVisible", m_bIsFXTabVisible, false, false );
				m_bShowAutomationArea = guiNode.read_bool( "showAutomationArea", m_bShowAutomationArea, false, false );
				m_bShowPlaybackTrack = guiNode.read_bool( "showPlaybackTrack", m_bShowPlaybackTrack, false, false );


				// pattern editor grid geometry
				m_nPatternEditorGridHeight = guiNode.read_int( "patternEditorGridHeight", m_nPatternEditorGridHeight, false, false );
				m_nPatternEditorGridWidth = guiNode.read_int( "patternEditorGridWidth", m_nPatternEditorGridWidth, false, false );

				// song editor grid geometry
				m_nSongEditorGridHeight = guiNode.read_int( "songEditorGridHeight", m_nSongEditorGridHeight, false, false );
				m_nSongEditorGridWidth = guiNode.read_int( "songEditorGridWidth", m_nSongEditorGridWidth, false, false );

				// mainForm window properties
				setMainFormProperties( readWindowProperties( guiNode, "mainForm_properties", mainFormProperties ) );
				setMixerProperties( readWindowProperties( guiNode, "mixer_properties", mixerProperties ) );
				setPatternEditorProperties( readWindowProperties( guiNode, "patternEditor_properties", patternEditorProperties ) );
				setSongEditorProperties( readWindowProperties( guiNode, "songEditor_properties", songEditorProperties ) );
				setInstrumentRackProperties( readWindowProperties( guiNode, "instrumentRack_properties", instrumentRackProperties ) );
				setAudioEngineInfoProperties( readWindowProperties( guiNode, "audioEngineInfo_properties", audioEngineInfoProperties ) );
				// In order to be backward compatible we still call the XML node
				// "playlistDialog". For some time we had playlistEditor and
				// playlistDialog coexisting.
				setPlaylistDialogProperties( readWindowProperties( guiNode, "playlistDialog_properties", m_playlistDialogProperties ) );
				setDirectorProperties( readWindowProperties( guiNode, "director_properties", m_directorProperties ) );

				// last used file dialog folders
				m_sLastExportPatternAsDirectory = guiNode.read_string( "lastExportPatternAsDirectory", QDir::homePath(), true, false, true );
				m_sLastExportSongDirectory = guiNode.read_string( "lastExportSongDirectory", QDir::homePath(), true, false, true );
				m_sLastSaveSongAsDirectory = guiNode.read_string( "lastSaveSongAsDirectory", QDir::homePath(), true, false, true );
				m_sLastOpenSongDirectory = guiNode.read_string( "lastOpenSongDirectory", Filesystem::songs_dir(), true, false, true );
				m_sLastOpenPatternDirectory = guiNode.read_string( "lastOpenPatternDirectory", Filesystem::patterns_dir(), true, false, true );
				m_sLastExportLilypondDirectory = guiNode.read_string( "lastExportLilypondDirectory", QDir::homePath(), true, false, true );
				m_sLastExportMidiDirectory = guiNode.read_string( "lastExportMidiDirectory", QDir::homePath(), true, false, true );
				m_sLastImportDrumkitDirectory = guiNode.read_string( "lastImportDrumkitDirectory", QDir::homePath(), true, false, true );
				m_sLastExportDrumkitDirectory = guiNode.read_string( "lastExportDrumkitDirectory", QDir::homePath(), true, false, true );
				m_sLastOpenLayerDirectory = guiNode.read_string( "lastOpenLayerDirectory", QDir::homePath(), true, false, true );
				m_sLastOpenPlaybackTrackDirectory = guiNode.read_string( "lastOpenPlaybackTrackDirectory", QDir::homePath(), true, false, true );
				m_sLastAddSongToPlaylistDirectory = guiNode.read_string( "lastAddSongToPlaylistDirectory", Filesystem::songs_dir(), true, false, true );
				m_sLastPlaylistDirectory = guiNode.read_string( "lastPlaylistDirectory", Filesystem::playlists_dir(), true, false, true );
				m_sLastPlaylistScriptDirectory = guiNode.read_string( "lastPlaylistScriptDirectory", Filesystem::scripts_dir(), true, false, true );
				m_sLastImportThemeDirectory = guiNode.read_string( "lastImportThemeDirectory", QDir::homePath(), true, false, true );
				m_sLastExportThemeDirectory = guiNode.read_string( "lastExportThemeDirectory", QDir::homePath(), true, false, true );

				//export dialog properties
				m_exportFormat = Filesystem::AudioFormatFromSuffix(
					guiNode.read_string( "exportDialogFormat", "flac", true, true ) );
				m_fExportCompressionLevel = guiNode.read_float(
					"exportDialogCompressionLevel", 0.0, true, true );
				m_nExportModeIdx = guiNode.read_int( "exportDialogMode", 0, false, false );
				m_nExportSampleRateIdx = guiNode.read_int( "exportDialogSampleRate", 0, false, false );
				m_nExportSampleDepthIdx = guiNode.read_int( "exportDialogSampleDepth", 0, false, false );
				m_bShowExportSongLicenseWarning =
					guiNode.read_bool( "showExportSongLicenseWarning", true,
									   true, false );
				
				m_bShowExportDrumkitLicenseWarning =
					guiNode.read_bool( "showExportDrumkitLicenseWarning", true,
									   true, false );
				m_bShowExportDrumkitCopyleftWarning =
					guiNode.read_bool( "showExportDrumkitCopyleftWarning", true,
									   true, false );
				m_bShowExportDrumkitAttributionWarning =
					guiNode.read_bool( "showExportDrumkitAttributionWarning", true,
									   true, false );
				
				m_bFollowPlayhead = guiNode.read_bool( "followPlayhead", true, false, false );

				// midi export dialog properties
				m_nMidiExportMode = guiNode.read_int( "midiExportDialogMode", 0, false, false );
				
				//beatcounter
				QString bcMode = guiNode.read_string( "bc", "BC_OFF", false, false );
				if ( bcMode == "BC_OFF" ) {
					m_bbc = BC_OFF;
				} else if ( bcMode == "BC_ON" ) {
					m_bbc = BC_ON;
				}


				QString setPlay = guiNode.read_string( "setplay", "SET_PLAY_OFF", false, false );
				if ( setPlay == "SET_PLAY_OFF" ) {
					m_mmcsetplay = SET_PLAY_OFF;
				} else if ( setPlay == "SET_PLAY_ON" ) {
					m_mmcsetplay = SET_PLAY_ON;
				}

				m_countOffset = guiNode.read_int( "countoffset", 0, false, false );
				m_startOffset = guiNode.read_int( "playoffset", 0, false, false );

				// ~ beatcounter

				m_nAutosavesPerHour = guiNode.read_int( "autosavesPerHour", 60, false, false );
				
				//SoundLibraryPanel expand items
				__expandSongItem = guiNode.read_bool( "expandSongItem", __expandSongItem, false, false );
				__expandPatternItem = guiNode.read_bool( "expandPatternItem", __expandPatternItem, false, false );

				for ( unsigned nFX = 0; nFX < MAX_FX; nFX++ ) {
					QString sNodeName = QString("ladspaFX_properties%1").arg( nFX );
					setLadspaProperties( nFX, readWindowProperties( guiNode, sNodeName, m_ladspaProperties[nFX] ) );
				}

				XMLNode pColorThemeNode = guiNode.firstChildElement( "colorTheme" );
				if ( !pColorThemeNode.isNull() ) {
					Theme::readColorTheme( pColorThemeNode, m_pTheme );
				} else {
					WARNINGLOG( "colorTheme node not found" );
					bRecreate = true;
				}

				//SongEditor coloring
				setColoringMethod( static_cast<InterfaceTheme::ColoringMethod>(
					guiNode.read_int("SongEditor_ColoringMethod",
									 static_cast<int>(InterfaceTheme::ColoringMethod::Custom), false, false )) );
				std::vector<QColor> colors( getMaxPatternColors() );
				for ( int ii = 0; ii < getMaxPatternColors(); ii++ ) {
					colors[ ii ] = guiNode.read_color( QString( "SongEditor_pattern_color_%1" ).arg( ii ),
													   m_pTheme->getColorTheme()->m_accentColor, false, false );
				}
				setPatternColors( colors );
				setVisiblePatternColors( guiNode.read_int( "SongEditor_visible_pattern_colors", 1, false, false ) );
				if ( getVisiblePatternColors() > 50 ) {
					setVisiblePatternColors( 50 );
				} else if ( getVisiblePatternColors() < 0 ) {
					setVisiblePatternColors( 0 );
				}
			}

			/////////////// FILES //////////////
			XMLNode filesNode = rootNode.firstChildElement( "files" );
			if ( filesNode.isNull() ) {
				WARNINGLOG( "files node not found" );
				bRecreate = true;
			} else {
				// last used song
				m_lastSongFilename = filesNode.read_string( "lastSongFilename", m_lastSongFilename, false, true );
				m_lastPlaylistFilename = filesNode.read_string( "lastPlaylistFilename", m_lastPlaylistFilename, false, true );
				m_sDefaultEditor = filesNode.read_string( "defaulteditor", m_sDefaultEditor, false, true );
			}

			MidiMap::reset_instance();
			MidiMap* mM = MidiMap::get_instance();


			XMLNode pMidiEventMapNode = rootNode.firstChildElement( "midiEventMap" );
			if ( !pMidiEventMapNode.isNull() ) {
				XMLNode pMidiEventNode = pMidiEventMapNode.firstChildElement( "midiEvent" );
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
			bRecreate = true;
		}
	}

	if ( m_nMaxLayers < 16 ) {
		m_nMaxLayers = 16;
	}

	// The preferences file should be recreated?
	if ( bRecreate && ! bGlobal ) {
		WARNINGLOG( "Recreating configuration file." );
		savePreferences();
		return false;
	}

	return true;
}



///
/// Save the preferences file
///
bool Preferences::savePreferences()
{
	QString sPreferencesFilename;
	const QString sPreferencesOverwritePath = Filesystem::getPreferencesOverwritePath();
	if ( sPreferencesOverwritePath.isEmpty() ) {
		sPreferencesFilename = Filesystem::usr_config_path();
	} else {
		sPreferencesFilename = sPreferencesOverwritePath;
	}

	INFOLOG( QString( "Saving preferences file %1" ).arg( sPreferencesFilename ) );

	XMLDoc doc;
	XMLNode rootNode = doc.set_root( "hydrogen_preferences" );

	// hydrogen version
	rootNode.write_string( "version", QString( get_version().c_str() ) );

	////// GENERAL ///////
	rootNode.write_string( "preferredLanguage", m_sPreferredLanguage );
	rootNode.write_bool( "restoreLastSong", m_brestoreLastSong );
	rootNode.write_bool( "restoreLastPlaylist", m_brestoreLastPlaylist );

	rootNode.write_bool( "useLash", m_bsetLash );
	rootNode.write_bool( "useTimeLine", __useTimelineBpm );

	rootNode.write_int( "maxBars", m_nMaxBars );
	rootNode.write_int( "maxLayers", m_nMaxLayers );

	rootNode.write_int( "defaultUILayout", static_cast<int>(getDefaultUILayout()) );
	rootNode.write_int( "uiScalingPolicy", static_cast<int>(getUIScalingPolicy()) );
	rootNode.write_int( "lastOpenTab", m_nLastOpenTab );

	rootNode.write_bool( "useTheRubberbandBpmChangeEvent", m_useTheRubberbandBpmChangeEvent );

	rootNode.write_bool( "useRelativeFilenamesForPlaylists", m_bUseRelativeFilenamesForPlaylists );
	rootNode.write_bool( "hideKeyboardCursorWhenUnused", m_bHideKeyboardCursor );
	
	// instrument input mode
	rootNode.write_bool( "instrumentInputMode", __playselectedinstrument );
	
	//show development version warning
	rootNode.write_bool( "showDevelWarning", m_bShowDevelWarning );

	// Warn about overwriting notes
	rootNode.write_bool( "showNoteOverwriteWarning", m_bShowNoteOverwriteWarning );

	// hear new notes in the pattern editor
	rootNode.write_bool( "hearNewNotes", hearNewNotes );

	// key/midi event prefs
	rootNode.write_bool( "quantizeEvents", quantizeEvents );

	//extern executables
	if ( !Filesystem::file_executable( m_rubberBandCLIexecutable , true /* silent */) ) {
		m_rubberBandCLIexecutable = "Path to Rubberband-CLI";
	}
	rootNode.write_string( "path_to_rubberband", m_rubberBandCLIexecutable );

	// Recent used songs
	XMLNode recentUsedSongsNode = rootNode.createNode( "recentUsedSongs" );
	{
		unsigned nSongs = 5;
		if ( m_recentFiles.size() < 5 ) {
			nSongs = m_recentFiles.size();
		}
		for ( unsigned i = 0; i < nSongs; i++ ) {
			recentUsedSongsNode.write_string( "song", m_recentFiles[ i ] );
		}
	}

	XMLNode recentFXNode = rootNode.createNode( "recentlyUsedEffects" );
	{
		int nFX = 0;
		QString FXname;
		foreach( FXname, m_recentFX ) {
			recentFXNode.write_string( "FX", FXname );
			if ( ++nFX > 10 ) break;
		}
	}


	std::list<QString>::const_iterator cur_Server;

	XMLNode serverListNode = rootNode.createNode( "serverList" );
	for( cur_Server = sServerList.begin(); cur_Server != sServerList.end(); ++cur_Server ){
		serverListNode.write_string( QString("server") , QString( *cur_Server ) );
	}


	std::list<QString>::const_iterator cur_patternCategories;

	XMLNode patternCategoriesNode = rootNode.createNode( "patternCategories" );
	for( cur_patternCategories = m_patternCategories.begin(); cur_patternCategories != m_patternCategories.end(); ++cur_patternCategories ){
		patternCategoriesNode.write_string( QString("categories") , QString( *cur_patternCategories ) );
	}

	//---- AUDIO ENGINE ----
	XMLNode audioEngineNode = rootNode.createNode( "audio_engine" );
	{
		// audio driver
		audioEngineNode.write_string( "audio_driver",
									  audioDriverToQString( m_audioDriver ) );

		// use metronome
		audioEngineNode.write_bool( "use_metronome", m_bUseMetronome );
		audioEngineNode.write_float( "metronome_volume", m_fMetronomeVolume );
		audioEngineNode.write_int( "maxNotes", m_nMaxNotes );
		audioEngineNode.write_int( "buffer_size", m_nBufferSize );
		audioEngineNode.write_int( "samplerate", m_nSampleRate );

		//// OSS DRIVER ////
		XMLNode ossDriverNode = audioEngineNode.createNode( "oss_driver" );
		{
			ossDriverNode.write_string( "ossDevice", m_sOSSDevice );
		}

		//// PORTAUDIO DRIVER ////
		XMLNode portAudioDriverNode = audioEngineNode.createNode( "portaudio_driver" );
		{
			portAudioDriverNode.write_string( "portAudioDevice", m_sPortAudioDevice );
			portAudioDriverNode.write_string( "portAudioHostAPI", m_sPortAudioHostAPI );
			portAudioDriverNode.write_int( "latencyTarget", m_nLatencyTarget );
		}

		//// COREAUDIO DRIVER ////
		XMLNode coreAudioDriverNode = audioEngineNode.createNode( "coreaudio_driver" );
		{
			coreAudioDriverNode.write_string( "coreAudioDevice", m_sCoreAudioDevice );
		}

		//// JACK DRIVER ////
		XMLNode jackDriverNode = audioEngineNode.createNode( "jack_driver" );
		{
			jackDriverNode.write_string( "jack_port_name_1", m_sJackPortName1 );	// jack port name 1
			jackDriverNode.write_string( "jack_port_name_2", m_sJackPortName2 );	// jack port name 2

			// jack transport client
			QString sMode;
			if ( m_bJackTransportMode == NO_JACK_TRANSPORT ) {
				sMode = "NO_JACK_TRANSPORT";
			} else if ( m_bJackTransportMode == USE_JACK_TRANSPORT ) {
				sMode = "USE_JACK_TRANSPORT";
			}
			jackDriverNode.write_string( "jack_transport_mode", sMode );

			jackDriverNode.write_bool( "jack_timebase_enabled", m_bJackTimebaseEnabled );
			// We stick to the old Timebase strings (? why strings for a boolean
			// option?) for backward and forward compatibility of old versions
			// still in use.
			QString tmMode;
			if ( m_bJackTimebaseMode == NO_JACK_TIMEBASE_CONTROL ) {
				tmMode = "NO_JACK_TIME_MASTER";
			} else if (  m_bJackTimebaseMode == USE_JACK_TIMEBASE_CONTROL ) {
				tmMode = "USE_JACK_TIME_MASTER";
			}
			jackDriverNode.write_string( "jack_transport_mode_master", tmMode );

			// jack default connection
			jackDriverNode.write_bool( "jack_connect_defaults", m_bJackConnectDefaults );

			int nJackTrackOutputMode;
			if ( m_JackTrackOutputMode == JackTrackOutputMode::postFader ) {
				nJackTrackOutputMode = 0;
			} else if ( m_JackTrackOutputMode == JackTrackOutputMode::preFader ) {
				nJackTrackOutputMode = 1;
			}
			jackDriverNode.write_int( "jack_track_output_mode", nJackTrackOutputMode );

			// jack track outs
			jackDriverNode.write_bool( "jack_track_outs", m_bJackTrackOuts );
		}

		//// ALSA AUDIO DRIVER ////
		XMLNode alsaAudioDriverNode = audioEngineNode.createNode( "alsa_audio_driver" );
		{
			alsaAudioDriverNode.write_string( "alsa_audio_device", m_sAlsaAudioDevice );
		}

		/// MIDI DRIVER ///
		XMLNode midiDriverNode = audioEngineNode.createNode( "midi_driver" );
		{
			midiDriverNode.write_string( "driverName", m_sMidiDriver );
			midiDriverNode.write_string( "port_name", m_sMidiPortName );
			midiDriverNode.write_string( "output_port_name", m_sMidiOutputPortName );
			midiDriverNode.write_int( "channel_filter", m_nMidiChannelFilter );
			midiDriverNode.write_bool( "ignore_note_off", m_bMidiNoteOffIgnore );
			midiDriverNode.write_bool( "enable_midi_feedback", m_bEnableMidiFeedback );
			midiDriverNode.write_bool( "discard_note_after_action", m_bMidiDiscardNoteAfterAction );
			midiDriverNode.write_bool( "fixed_mapping", m_bMidiFixedMapping );
		}
		
		/// OSC ///
		XMLNode oscNode = audioEngineNode.createNode( "osc_configuration" );
		{
			oscNode.write_int( "oscServerPort", m_nOscServerPort );
			oscNode.write_bool( "oscEnabled", m_bOscServerEnabled );
			oscNode.write_bool( "oscFeedbackEnabled", m_bOscFeedbackEnabled );
		}
		
	}

	//---- GUI ----
	XMLNode guiNode = rootNode.createNode( "gui" );
	{
		guiNode.write_string( "QTStyle", getQTStyle() );
		guiNode.write_string( "application_font_family", getApplicationFontFamily() );
		guiNode.write_string( "level2_font_family", getLevel2FontFamily() );
		guiNode.write_string( "level3_font_family", getLevel3FontFamily() );
		guiNode.write_int( "font_size", static_cast<int>(getFontSize()) );
		guiNode.write_float( "mixer_falloff_speed", getMixerFalloffSpeed() );
		guiNode.write_int( "patternEditorGridResolution", m_nPatternEditorGridResolution );
		guiNode.write_int( "patternEditorGridHeight", m_nPatternEditorGridHeight );
		guiNode.write_int( "patternEditorGridWidth", m_nPatternEditorGridWidth );
		guiNode.write_bool( "patternEditorUsingTriplets", m_bPatternEditorUsingTriplets );
		guiNode.write_int( "songEditorGridHeight", m_nSongEditorGridHeight );
		guiNode.write_int( "songEditorGridWidth", m_nSongEditorGridWidth );
		guiNode.write_bool( "showInstrumentPeaks", m_bShowInstrumentPeaks );
		guiNode.write_bool( "isFXTabVisible", m_bIsFXTabVisible );
		guiNode.write_bool( "showAutomationArea", m_bShowAutomationArea );
		guiNode.write_bool( "showPlaybackTrack", m_bShowPlaybackTrack );

		// MainForm window properties
		writeWindowProperties( guiNode, "mainForm_properties", mainFormProperties );
		writeWindowProperties( guiNode, "mixer_properties", mixerProperties );
		writeWindowProperties( guiNode, "patternEditor_properties", patternEditorProperties );
		writeWindowProperties( guiNode, "songEditor_properties", songEditorProperties );
		writeWindowProperties( guiNode, "instrumentRack_properties", instrumentRackProperties );
		writeWindowProperties( guiNode, "audioEngineInfo_properties", audioEngineInfoProperties );
		writeWindowProperties( guiNode, "playlistDialog_properties", m_playlistDialogProperties );
		writeWindowProperties( guiNode, "director_properties", m_directorProperties );
		for ( unsigned nFX = 0; nFX < MAX_FX; nFX++ ) {
			QString sNode = QString("ladspaFX_properties%1").arg( nFX );
			writeWindowProperties( guiNode, sNode, m_ladspaProperties[nFX] );
		}
		
		// last used file dialog folders
		guiNode.write_string( "lastExportPatternAsDirectory", m_sLastExportPatternAsDirectory );
		guiNode.write_string( "lastExportSongDirectory", m_sLastExportSongDirectory );
		guiNode.write_string( "lastSaveSongAsDirectory", m_sLastSaveSongAsDirectory );
		guiNode.write_string( "lastOpenSongDirectory", m_sLastOpenSongDirectory );
		guiNode.write_string( "lastOpenPatternDirectory", m_sLastOpenPatternDirectory );
		guiNode.write_string( "lastExportLilypondDirectory", m_sLastExportLilypondDirectory );
		guiNode.write_string( "lastExportMidiDirectory", m_sLastExportMidiDirectory );
		guiNode.write_string( "lastImportDrumkitDirectory", m_sLastImportDrumkitDirectory );
		guiNode.write_string( "lastExportDrumkitDirectory", m_sLastExportDrumkitDirectory );
		guiNode.write_string( "lastOpenLayerDirectory", m_sLastOpenLayerDirectory );
		guiNode.write_string( "lastOpenPlaybackTrackDirectory", m_sLastOpenPlaybackTrackDirectory );
		guiNode.write_string( "lastAddSongToPlaylistDirectory", m_sLastAddSongToPlaylistDirectory );
		guiNode.write_string( "lastPlaylistDirectory", m_sLastPlaylistDirectory );
		guiNode.write_string( "lastPlaylistScriptDirectory", m_sLastPlaylistScriptDirectory );
		guiNode.write_string( "lastImportThemeDirectory", m_sLastImportThemeDirectory );
		guiNode.write_string( "lastExportThemeDirectory", m_sLastExportThemeDirectory );
				
		//ExportSongDialog
		guiNode.write_int( "exportDialogMode", m_nExportModeIdx );
		guiNode.write_string( "exportDialogFormat",
							  Filesystem::AudioFormatToSuffix( m_exportFormat ) );
		guiNode.write_float( "exportDialogCompressionLevel",
							 m_fExportCompressionLevel );
		guiNode.write_int( "exportDialogSampleRate",  m_nExportSampleRateIdx );
		guiNode.write_int( "exportDialogSampleDepth", m_nExportSampleDepthIdx );
		guiNode.write_bool( "showExportSongLicenseWarning", m_bShowExportSongLicenseWarning );
		guiNode.write_bool( "showExportDrumkitLicenseWarning", m_bShowExportDrumkitLicenseWarning );
		guiNode.write_bool( "showExportDrumkitCopyleftWarning", m_bShowExportDrumkitCopyleftWarning );
		guiNode.write_bool( "showExportDrumkitAttributionWarning", m_bShowExportDrumkitAttributionWarning );

		guiNode.write_bool( "followPlayhead", m_bFollowPlayhead );

		//ExportMidiDialog
		guiNode.write_int( "midiExportDialogMode", m_nMidiExportMode );

		//beatcounter
		QString bcMode;

		if ( m_bbc == BC_OFF ) {
			bcMode = "BC_OFF";
		} else if ( m_bbc  == BC_ON ) {
			bcMode = "BC_ON";
		}
		guiNode.write_string( "bc", bcMode );

		QString setPlay;
		if ( m_mmcsetplay == SET_PLAY_OFF ) {
			setPlay = "SET_PLAY_OFF";
		} else if ( m_mmcsetplay == SET_PLAY_ON ) {
			setPlay = "SET_PLAY_ON";
		}
		guiNode.write_string( "setplay", setPlay );

		guiNode.write_int( "countoffset", m_countOffset );
		guiNode.write_int( "playoffset", m_startOffset );
		// ~ beatcounter

		guiNode.write_int( "autosavesPerHour", m_nAutosavesPerHour );

		//SoundLibraryPanel expand items
		guiNode.write_bool( "expandSongItem", __expandSongItem );
		guiNode.write_bool( "expandPatternItem", __expandPatternItem );

		// User interface style
		Theme::writeColorTheme( &guiNode, m_pTheme );

		//SongEditor coloring method
		guiNode.write_int( "SongEditor_ColoringMethod", static_cast<int>(getColoringMethod()) );
		for ( int ii = 0; ii < getMaxPatternColors(); ii++ ) {
			guiNode.write_color( QString( "SongEditor_pattern_color_%1" ).arg( ii ), getPatternColors()[ ii ] );
		}
		guiNode.write_int( "SongEditor_visible_pattern_colors", getVisiblePatternColors() );
	}

	//---- FILES ----
	XMLNode filesNode = rootNode.createNode( "files" );
	{
		// last used song
		filesNode.write_string( "lastSongFilename", m_lastSongFilename );
		filesNode.write_string( "lastPlaylistFilename", m_lastPlaylistFilename );
		filesNode.write_string( "defaulteditor", m_sDefaultEditor );
	}

	// In case the Preferences in both system and user space are
	// bricked Hydrogen attempts to recreate the Preferences before
	// the core is properly initialized. That's why we assure for the
	// MidiMap to be initialized.
	MidiMap::create_instance();
	MidiMap * mM = MidiMap::get_instance();

	//---- MidiMap ----
	XMLNode midiEventMapNode = rootNode.createNode( "midiEventMap" );

	for( const auto& [ssType, ppAction] : mM->getMMCActionMap() ){
		if ( ppAction != nullptr && ! ppAction->isNull() ){
			XMLNode midiEventNode = midiEventMapNode.createNode( "midiEvent" );

			midiEventNode.write_string( "mmcEvent" , ssType );
			midiEventNode.write_string( "action" , ppAction->getType());
			midiEventNode.write_string( "parameter" , ppAction->getParameter1() );
			midiEventNode.write_string( "parameter2" , ppAction->getParameter2() );
			midiEventNode.write_string( "parameter3" , ppAction->getParameter3() );
		}
	}

	for ( const auto& [nnPitch, ppAction] : mM->getNoteActionMap() ){
		if ( ppAction != nullptr && ! ppAction->isNull() ){
			XMLNode midiEventNode = midiEventMapNode.createNode( "midiEvent" );

			midiEventNode.write_string(
				"noteEvent", MidiMessage::EventToQString( MidiMessage::Event::Note ) );
			midiEventNode.write_int( "eventParameter" , nnPitch );
			midiEventNode.write_string( "action" , ppAction->getType() );
			midiEventNode.write_string( "parameter" , ppAction->getParameter1() );
			midiEventNode.write_string( "parameter2" , ppAction->getParameter2() );
			midiEventNode.write_string( "parameter3" , ppAction->getParameter3() );
		}
	}

	for ( const auto& [nnParam, ppAction] : mM->getCCActionMap() ){
		if ( ppAction != nullptr && ! ppAction->isNull() ){
			XMLNode midiEventNode = midiEventMapNode.createNode( "midiEvent" );

			midiEventNode.write_string(
				"ccEvent", MidiMessage::EventToQString( MidiMessage::Event::CC ) );
			midiEventNode.write_int( "eventParameter" , nnParam );
			midiEventNode.write_string( "action" , ppAction->getType() );
			midiEventNode.write_string( "parameter" , ppAction->getParameter1() );
			midiEventNode.write_string( "parameter2" , ppAction->getParameter2() );
			midiEventNode.write_string( "parameter3" , ppAction->getParameter3() );
		}
	}

	for ( const auto& ppAction : mM->getPCActions() ) {
		if ( ppAction != nullptr && ! ppAction->isNull() ){
			XMLNode midiEventNode = midiEventMapNode.createNode( "midiEvent" );

			midiEventNode.write_string(
				"pcEvent", MidiMessage::EventToQString( MidiMessage::Event::PC ) );
			midiEventNode.write_string( "action" , ppAction->getType() );
			midiEventNode.write_string( "parameter" , ppAction->getParameter1() );
			midiEventNode.write_string( "parameter2" , ppAction->getParameter2() );
			midiEventNode.write_string( "parameter3" , ppAction->getParameter3() );
		}
	}

	return doc.write( sPreferencesFilename );
}

Preferences::AudioDriver Preferences::parseAudioDriver( const QString& sDriver ) {
	const QString s = QString( sDriver ).toLower();
	if ( s == "auto" ) {
		return AudioDriver::Auto;
	}
	else if ( s == "jack" || s == "jackaudio") {
		return AudioDriver::Jack;
	}
	else if ( s == "oss" ) {
		return AudioDriver::Oss;
	}
	else if ( s == "alsa" ) {
		return AudioDriver::Alsa;
	}
	else if ( s == "pulseaudio" || s == "pulse" ) {
		return AudioDriver::PulseAudio;
	}
	else if ( s == "coreaudio" || s == "core" ) {
		return AudioDriver::CoreAudio;
	}
	else if ( s == "portaudio" || s == "port" ) {
		return AudioDriver::PortAudio;
	}
	else {
		if ( Logger::isAvailable() ) {
			ERRORLOG( QString( "Unable to parse driver [%1]" ). arg( sDriver ) );
		}
		return AudioDriver::None;
	}
}

QString Preferences::audioDriverToQString( const Preferences::AudioDriver& driver ) {
	switch ( driver ) {
	case AudioDriver::Auto:
		return "Auto";
	case AudioDriver::Jack:
		return "JACK";
	case AudioDriver::Oss:
		return "OSS";
	case AudioDriver::Alsa:
		return "ALSA";
	case AudioDriver::PulseAudio:
		return "PulseAudio";
	case AudioDriver::CoreAudio:
		return "CoreAudio";
	case AudioDriver::PortAudio:
		return "PortAudio";
	case AudioDriver::Disk:
		return "Disk";
	case AudioDriver::Fake:
		return "Fake";
	case AudioDriver::Null:
		return "Null";
	case AudioDriver::None:
		return "nullptr";
	default:
		return "Unhandled driver type";
	}
}

bool Preferences::checkJackSupport() {
	// Check whether the Logger is already available.
	const bool bUseLogger = Logger::isAvailable();

#ifndef H2CORE_HAVE_JACK
	if ( bUseLogger ) {
		INFOLOG( "Hydrogen was compiled without JACK support." );
	}
	return false;
#else
  #ifndef H2CORE_HAVE_DYNAMIC_JACK_CHECK
	if ( bUseLogger ) {
		INFOLOG( "JACK support enabled." );
	}
	return true;
  #else
	/**
	 * Calls @a sExecutable in a subprocess using the @a sOption CLI
	 * option and reports the results.
	 *
	 * @return An empty string indicates, that the call exited with a
	 *   code other than zero.
	 */
	auto checkExecutable = [&]( const QString& sExecutable,
								const QString& sOption ) {
		QProcess process;
		process.start( sExecutable, QStringList( sOption ) );
		process.waitForFinished( -1 );

		if ( process.exitCode() != 0 ) {
			return QString( "" );
		}

		QString sStdout = process.readAllStandardOutput();
		if ( sStdout.isEmpty() ) {
			return QString( "No output" );
		}

		return QString( sStdout.trimmed() );
	};


	bool bJackSupport = false;

	// Classic JACK
	QString sCapture = checkExecutable( "jackd", "--version" );
	if ( ! sCapture.isEmpty() ) {
		bJackSupport = true;
		if ( bUseLogger ) {
			INFOLOG( QString( "'jackd' of version [%1] found." )
					 .arg( sCapture ) );
		}
	}

	// JACK compiled with DBus support (maybe this one is packaged but
	// the classical one isn't).
	//
	// `jackdbus` is supposed to be run by the DBus message daemon and
	// does not have proper CLI options. But it does not fail by
	// passing a `-h` either and this will serve for checking its
	// presence.
	sCapture = checkExecutable( "jackdbus", "-h" );
	if ( ! sCapture.isEmpty() ) {
		bJackSupport = true;
		if ( bUseLogger ) {
			INFOLOG( "'jackdbus' found." );
		}
	}

	// Pipewire JACK interface
	//
	// `pw-jack` has no version query CLI option (yet). But showing
	// the help will serve for checking its presence.
	sCapture = checkExecutable( "pw-jack", "-h" );
	if ( ! sCapture.isEmpty() ) {
		bJackSupport = true;
		if ( bUseLogger ) {
			INFOLOG( "'pw-jack' found." );
		}
	}

	if ( bUseLogger ) {
		if ( bJackSupport ) {
			INFOLOG( "Dynamic JACK discovery succeeded. JACK support enabled." );
		}
		else {
			WARNINGLOG( "Dynamic JACK discovery failed. JACK support disabled." );
		}
	}

	return bJackSupport;
  #endif
#endif
}

std::vector<Preferences::AudioDriver> Preferences::getSupportedAudioDrivers() {

	std::vector<AudioDriver> drivers;

	// We always do a fresh check. Maybe dynamical discovery will yield a
	// different result this time.
	bool bJackSupported = checkJackSupport();

	// The order of the assigned drivers is important as Hydrogen uses
	// it when trying different drivers in case "Auto" was selected.
#if defined(WIN32)
  #ifdef H2CORE_HAVE_PORTAUDIO
	drivers.push_back( AudioDriver::PortAudio );
  #endif
	if ( bJackSupported ) {
		drivers.push_back( AudioDriver::Jack );
	}
#elif defined(__APPLE__)
  #ifdef H2CORE_HAVE_COREAUDIO
	drivers.push_back( AudioDriver::CoreAudio );
  #endif
	if ( bJackSupported ) {
		drivers.push_back( AudioDriver::Jack );
	}
  #ifdef H2CORE_HAVE_PULSEAUDIO
	drivers.push_back( AudioDriver::PulseAudio );
  #endif
  #ifdef H2CORE_HAVE_PORTAUDIO
	drivers.push_back( AudioDriver::PortAudio );
  #endif
#else /* Linux */
	if ( bJackSupported ) {
		drivers.push_back( AudioDriver::Jack );
	}
  #ifdef H2CORE_HAVE_PULSEAUDIO
	drivers.push_back( AudioDriver::PulseAudio );
  #endif
  #ifdef H2CORE_HAVE_ALSA
	drivers.push_back( AudioDriver::Alsa );
  #endif
  #ifdef H2CORE_HAVE_OSS
	drivers.push_back( AudioDriver::Oss );
  #endif
  #ifdef H2CORE_HAVE_PORTAUDIO
	drivers.push_back( AudioDriver::PortAudio );
  #endif
#endif

	return drivers;
}


void Preferences::setMostRecentFX( QString FX_name )
{
	int pos = m_recentFX.indexOf( FX_name );

	if ( pos != -1 ) {
		m_recentFX.removeAt( pos );
	}

	m_recentFX.push_front( FX_name );
}

/// Read the xml nodes related to window properties
WindowProperties Preferences::readWindowProperties( XMLNode parent, const QString& windowName, WindowProperties defaultProp )
{
	WindowProperties prop = defaultProp;

	XMLNode windowPropNode  = parent.firstChildElement( windowName );
	if ( windowPropNode.isNull() ) {
		WARNINGLOG( "Error reading configuration file: " + windowName + " node not found" );
	} else {
		prop.visible = windowPropNode.read_bool( "visible", true, false, false );
		prop.x = windowPropNode.read_int( "x", prop.x, false, false );
		prop.y = windowPropNode.read_int( "y", prop.y, false, false );
		prop.width = windowPropNode.read_int( "width", prop.width, false, false );
		prop.height = windowPropNode.read_int( "height", prop.height, false, false );
		prop.m_geometry = QByteArray::fromBase64( windowPropNode.read_string( "geometry",
																			  prop.m_geometry.toBase64(), false, true )
												  .toUtf8() );
	}

	return prop;
}



/// Write the xml nodes related to window properties
void Preferences::writeWindowProperties( XMLNode parent, const QString& windowName, const WindowProperties& prop )
{
	XMLNode windowPropNode = parent.createNode( windowName );
	
	windowPropNode.write_bool( "visible", prop.visible );
	windowPropNode.write_int( "x", prop.x );
	windowPropNode.write_int( "y", prop.y );
	windowPropNode.write_int( "width", prop.width );
	windowPropNode.write_int( "height", prop.height );
	windowPropNode.write_string( "geometry", QString( prop.m_geometry.toBase64() ) );
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
