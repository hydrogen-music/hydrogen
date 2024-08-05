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

#include <core/Basics/InstrumentComponent.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>
#include <core/IO/AlsaAudioDriver.h>
#include <core/MidiMap.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <core/Version.h>

#include <QDir>
#include <QProcess>

namespace H2Core
{

std::shared_ptr<Preferences> Preferences::__instance = nullptr;

void Preferences::create_instance()
{
	if ( __instance == nullptr ) {
		// User-level configs
		auto pPrefUser = load( Filesystem::usr_config_path() );
		if ( pPrefUser != nullptr ) {
			__instance = pPrefUser;
			__instance->m_bLoadingSuccessful = true;
		}
		else {
			// Fallback to system-level configs (the one we ship)
			auto pPrefSystem = load( Filesystem::sys_config_path() );
			if ( pPrefSystem != nullptr ) {
				INFOLOG( QString( "Couldn't load user-level configuration from [%1]. Falling back to system-level one in [%2]" )
						 .arg( Filesystem::usr_config_path() )
						 .arg( Filesystem::sys_config_path() ) );
				__instance = pPrefSystem;
				__instance->m_bLoadingSuccessful = true;
			}
			else {
				ERRORLOG( QString( "Couldn't load config file from neither [%1] nor [%2]." )
						 .arg( Filesystem::usr_config_path() )
						 .arg( Filesystem::sys_config_path() ) );
				__instance = std::shared_ptr<Preferences>(new Preferences());
				__instance->m_bLoadingSuccessful = false;
			}

		}

		// Propagate loaded settings
		InstrumentComponent::setMaxLayers( __instance->getMaxLayers() );
	}
}

void Preferences::replaceInstance( std::shared_ptr<Preferences> pOther ) {
	__instance = pOther;
}

Preferences::Preferences()
{
	m_pShortcuts = std::make_shared<Shortcuts>();

	// switch to enable / disable lash, only on h2 startup
	m_bRestartLash = false;
	m_bSetLash = false;

	//rubberband bpm change queue
	m_bUseTheRubberbandBpmChangeEvent = false;

	QString rubberBandCLIPath = getenv( "PATH" );
	QStringList rubberBandCLIPathList = rubberBandCLIPath.split(":");//linux use ":" as separator. maybe windows and osx use other separators

	//find the Rubberband-CLI in system env
	//if this fails a second test will check individual user settings
	for(int i = 0; i < rubberBandCLIPathList.size(); ++i){
		m_sRubberBandCLIexecutable = rubberBandCLIPathList[i] + "/rubberband";
		if ( QFile( m_sRubberBandCLIexecutable ).exists() == true ){
			m_bSearchForRubberbandOnLoad = false;
			break;
		}
		else {
			m_sRubberBandCLIexecutable = "Path to Rubberband-CLI";
			m_bSearchForRubberbandOnLoad = true;
		}
	}

	m_sPreferredLanguage = QString();
	m_bPlaySamplesOnClicking = false; // audio file browser
	m_bPlaySelectedInstrument = false; // midi keyboard and keyboard play only selected instrument

	m_bRecordEvents = false; // not recording by default
	m_nPunchInPos = 0;
	m_nPunchOutPos = -1;

	m_bExpandSongItem = true; //SoundLibraryPanel
	m_bExpandPatternItem = true; //SoundLibraryPanel
	m_bUseTimelineBpm = false;		// use timeline
	
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
	m_sLastImportThemeDirectory = QDir::homePath();
	m_sLastExportThemeDirectory = QDir::homePath();
	
	// export dialog
	m_nExportTemplateIdx = 0;
	m_nExportModeIdx = 0;
	m_nExportSampleRateIdx = 0;
	m_nExportSampleDepthIdx = 0;
	m_bShowExportSongLicenseWarning = true;
	m_bShowExportDrumkitLicenseWarning = true;
	m_bShowExportDrumkitCopyleftWarning = true;
	m_bShowExportDrumkitAttributionWarning = true;

	//export midi dialog
	m_nMidiExportMode = 0;

	m_bFollowPlayhead = true;

	m_bBbc = BC_OFF;
	m_bMmcSetPlay = SET_PLAY_OFF;

	m_nCountOffset = 0;  // beatcounter
	m_nStartOffset = 0;  // beatcounter

	m_serverList.push_back( QString("http://hydrogen-music.org/feeds/drumkit_list.php") );
	m_nAutosavesPerHour = 60;
	m_patternCategories.push_back( SoundLibraryDatabase::m_sPatternBaseCategory );

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
	m_bMidiNoteOffIgnore = true;
	m_bMidiFixedMapping = false;
	m_bMidiDiscardNoteAfterAction = true;
	m_bEnableMidiFeedback = false;

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
	m_bJackMasterMode = NO_JACK_TIME_MASTER;
	m_JackTrackOutputMode = JackTrackOutputMode::postFader;

	// OSC configuration
	m_bOscServerEnabled = false;
	m_bOscFeedbackEnabled = true;
	m_nOscServerPort = 9000;
	m_nOscTemporaryPort = -1;

	//___ General properties ___
	m_bUseLash = false;
	m_bShowDevelWarning = false;
	m_bShowNoteOverwriteWarning = true;
	// NONE: lastSongFilename;
	m_bHearNewNotes = true;
	// NONE: m_recentFiles;
	// NONE: m_recentFX;
	m_bQuantizeEvents = true;
	m_bRecordEvents = false;
	m_bUseRelativeFilenamesForPlaylists = false;
	m_bHideKeyboardCursor = false;

	//___ GUI properties ___
	m_nLastOpenTab = 0;
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
	m_mainFormProperties.set(0, 0, 1000, 700, true);
	m_mixerProperties.set(10, 350, 829, 276, true);
	m_patternEditorProperties.set(280, 100, 706, 439, true);
	m_songEditorProperties.set(10, 10, 600, 250, true);
	m_instrumentRackProperties.set(500, 20, 526, 437, true);
	m_audioEngineInfoProperties.set(720, 120, 0, 0, false);
	m_playlistEditorProperties.set(200, 300, 921, 703, false);
	m_directorProperties.set(200, 300, 423, 377, false);
	m_ladspaProperties[0].set(2, 20, 0, 0, false);
	m_ladspaProperties[1].set(2, 20, 0, 0, false);
	m_ladspaProperties[2].set(2, 20, 0, 0, false);
	m_ladspaProperties[3].set(2, 20, 0, 0, false);
	m_nMaxBars = 400;
	m_nMaxLayers = 16;
}

Preferences::~Preferences()
{
	INFOLOG( "DESTROY" );
	__instance = nullptr;
}

std::shared_ptr<Preferences> Preferences::load( const QString& sPath, const bool bSilent ) {
	if ( ! Filesystem::file_readable( sPath, bSilent ) ) {
		return nullptr;
	}

	XMLDoc doc;
	doc.read( sPath, nullptr, false );
	const XMLNode rootNode = doc.firstChildElement( "hydrogen_preferences" );
	if ( rootNode.isNull() ) {
		ERRORLOG( QString( "Preferences file [%1] ill-formatted. <hydrogen_preferences> node not found." )
				  .arg( sPath ) );
		return nullptr;
	}

	if ( ! bSilent ) {
		INFOLOG( QString( "Loading preferences from [%1]" ).arg( sPath ) );
	}

	auto pPref = std::shared_ptr<Preferences>(new Preferences());

	//////// GENERAL ///////////
	auto interfaceTheme = InterfaceTheme();
	auto fontTheme = FontTheme();
	auto colorTheme = ColorTheme();

	pPref->m_sPreferredLanguage = rootNode.read_string(
		"preferredLanguage", pPref->m_sPreferredLanguage, false, "", bSilent );
	pPref->m_bPlaySelectedInstrument = rootNode.read_bool(
		"instrumentInputMode", pPref->m_bPlaySelectedInstrument, false, false,
		bSilent );
	pPref->m_bShowDevelWarning = rootNode.read_bool(
		"showDevelWarning", pPref->m_bShowDevelWarning, false, false, bSilent );
	pPref->m_bShowNoteOverwriteWarning = rootNode.read_bool(
		"showNoteOverwriteWarning",
		pPref->m_bShowNoteOverwriteWarning, false, false, bSilent );
	pPref->m_bUseLash = rootNode.read_bool(
		"useLash", pPref->m_bUseLash, false, false, bSilent );
	pPref->m_bSetLash = pPref->m_bUseLash;
	pPref->m_bUseTimelineBpm = rootNode.read_bool(
		"useTimeLine", pPref->m_bUseTimelineBpm, false, false, bSilent );
	pPref->m_nMaxBars = rootNode.read_int(
		"maxBars", pPref->m_nMaxBars, false, false, bSilent );
	pPref->m_nMaxLayers = rootNode.read_int(
		"maxLayers", pPref->m_nMaxLayers, false, false, bSilent );
	if ( pPref->m_nMaxLayers < 16 ) {
		WARNINGLOG( QString( "[maxLayers: %1] is smaller than the minimum number of layers [16]" )
					.arg( pPref->m_nMaxLayers ) );
		pPref->m_nMaxLayers = 16;
	}
	interfaceTheme.m_layout = static_cast<InterfaceTheme::Layout>(
		rootNode.read_int( "defaultUILayout",
						   static_cast<int>(interfaceTheme.m_layout),
						   false, false, bSilent ));
	interfaceTheme.m_uiScalingPolicy = static_cast<InterfaceTheme::ScalingPolicy>(
		rootNode.read_int( "uiScalingPolicy",
						   static_cast<int>(interfaceTheme.m_uiScalingPolicy),
						   false, false, bSilent ));
	pPref->m_nLastOpenTab = rootNode.read_int(
		"lastOpenTab", pPref->m_nLastOpenTab, false, false, bSilent );
	pPref->m_bUseRelativeFilenamesForPlaylists = rootNode.read_bool(
		"useRelativeFilenamesForPlaylists",
		pPref->m_bUseRelativeFilenamesForPlaylists, false, false, bSilent );
	pPref->m_bHideKeyboardCursor = rootNode.read_bool(
		"hideKeyboardCursorWhenUnused",
		pPref->m_bHideKeyboardCursor, false, false, bSilent );
	pPref->m_bUseTheRubberbandBpmChangeEvent = rootNode.read_bool(
		"useTheRubberbandBpmChangeEvent",
		pPref->m_bUseTheRubberbandBpmChangeEvent, false, false, bSilent );

	pPref->m_bHearNewNotes = rootNode.read_bool(
		"hearNewNotes", pPref->m_bHearNewNotes, false, false, bSilent );
	pPref->m_bQuantizeEvents = rootNode.read_bool(
		"quantizeEvents", pPref->m_bQuantizeEvents, false, false, bSilent );

	if ( pPref->m_bSearchForRubberbandOnLoad ){
		// In case Rubberband CLI executable was not found yet, we check the
		// additional path provided in the config (Preferences constructor
		// already checked common places).
		const QString sRubberbandPath = rootNode.read_string(
			"path_to_rubberband", "", false, false, bSilent );
		if ( ! sRubberbandPath.isEmpty() && QFile( sRubberbandPath ).exists() ){
			pPref->m_sRubberBandCLIexecutable = sRubberbandPath;
		}
		else {
			pPref->m_sRubberBandCLIexecutable = "Path to Rubberband-CLI";
		}
	}

	const XMLNode recentUsedSongsNode =
		rootNode.firstChildElement( "recentUsedSongs" );
	if ( ! recentUsedSongsNode.isNull() ) {
		QDomElement songElement = recentUsedSongsNode.firstChildElement( "song" );
		while( ! songElement.isNull() && ! songElement.text().isEmpty() ){
			pPref->m_recentFiles.push_back( songElement.text() );
			songElement = songElement.nextSiblingElement( "song" );
		}
	}
	else {
		WARNINGLOG( "<recentUsedSongs> node not found" );
	}

	const XMLNode recentFXNode =
		rootNode.firstChildElement( "recentlyUsedEffects" );
	if ( ! recentFXNode.isNull() ) {
		QDomElement fxElement = recentFXNode.firstChildElement( "FX" );
		while ( ! fxElement.isNull()  && ! fxElement.text().isEmpty() ) {
			pPref->m_recentFX.push_back( fxElement.text() );
			fxElement = fxElement.nextSiblingElement( "FX" );
		}
	}
	else {
		WARNINGLOG( "<recentlyUsedEffects> node not found" );
	}

	// Use the default server defined in the constructor and add additional
	// ones.
	const XMLNode serverListNode = rootNode.firstChildElement( "serverList" );
	if ( ! serverListNode.isNull() ) {
		QDomElement serverElement = serverListNode.firstChildElement( "server" );
		while ( ! serverElement.isNull() && !serverElement.text().isEmpty() ) {
			if ( ! pPref->m_serverList.contains( serverElement.text() ) ) {
				pPref->m_serverList.push_back( serverElement.text() );
			}

			serverElement = serverElement.nextSiblingElement( "server" );
		}
	}
	else {
		WARNINGLOG( "<serverList> node not found" );
	}

	// Use the default categories defined in the constructor and add additional
	// ones.
	const XMLNode patternCategoriesNode =
		rootNode.firstChildElement( "patternCategories" );
	if ( ! patternCategoriesNode.isNull() ) {
		QDomElement patternCategoriesElement =
			patternCategoriesNode.firstChildElement( "categories" );
		while ( ! patternCategoriesElement.isNull() &&
				! patternCategoriesElement.text().isEmpty() ) {
			if ( ! pPref->m_patternCategories.contains(
					 patternCategoriesElement.text() ) ) {
				pPref->m_patternCategories.push_back(
					patternCategoriesElement.text() );
			}

			patternCategoriesElement = patternCategoriesElement.nextSiblingElement( "categories" );
		}
	} else {
		WARNINGLOG( "<patternCategories> node not found" );
	}

	/////////////// AUDIO ENGINE //////////////
	const XMLNode audioEngineNode = rootNode.firstChildElement( "audio_engine" );
	if ( ! audioEngineNode.isNull() ) {
		const QString sAudioDriver = audioEngineNode.read_string(
			"audio_driver",
			Preferences::audioDriverToQString( pPref->m_audioDriver ),
			false, false, bSilent );
		pPref->m_audioDriver = parseAudioDriver( sAudioDriver );
		if ( pPref->m_audioDriver == AudioDriver::None ) {
			WARNINGLOG( QString( "Parsing of audio driver [%1] failed. Falling back to 'Auto'" )
						.arg( sAudioDriver ) );
			pPref->m_audioDriver = AudioDriver::Auto;
		}
		pPref->m_bUseMetronome = audioEngineNode.read_bool(
			"use_metronome", pPref->m_bUseMetronome, false, false, bSilent );
		pPref->m_fMetronomeVolume = audioEngineNode.read_float(
			"metronome_volume", pPref->m_fMetronomeVolume, false, false, bSilent );
		pPref->m_nMaxNotes = audioEngineNode.read_int(
			"maxNotes", pPref->m_nMaxNotes, false, false, bSilent );
		pPref->m_nBufferSize = audioEngineNode.read_int(
			"buffer_size", pPref->m_nBufferSize, false, false, bSilent );
		pPref->m_nSampleRate = audioEngineNode.read_int(
			"samplerate", pPref->m_nSampleRate, false, false, bSilent );

		//// OSS DRIVER ////
		const XMLNode ossDriverNode =
			audioEngineNode.firstChildElement( "oss_driver" );
		if ( ! ossDriverNode.isNull()  ) {
			pPref->m_sOSSDevice = ossDriverNode.read_string(
				"ossDevice", pPref->m_sOSSDevice, false, false, bSilent );
		}
		else {
			WARNINGLOG( "<portaudio_driver> node not found" );
		}

		//// PORTAUDIO DRIVER ////
		const XMLNode portAudioDriverNode =
			audioEngineNode.firstChildElement( "portaudio_driver" );
		if ( ! portAudioDriverNode.isNull()  ) {
			pPref->m_sPortAudioDevice = portAudioDriverNode.read_string(
				"portAudioDevice", pPref->m_sPortAudioDevice, false, true, bSilent );
			pPref->m_sPortAudioHostAPI = portAudioDriverNode.read_string(
				"portAudioHostAPI", pPref->m_sPortAudioHostAPI, false, true,
				bSilent );
			pPref->m_nLatencyTarget = portAudioDriverNode.read_int(
				"latencyTarget", pPref->m_nLatencyTarget, false, false, bSilent );
		}
		else {
			WARNINGLOG( "<portaudio_driver> node not found" );
		}

		//// COREAUDIO DRIVER ////
		const XMLNode coreAudioDriverNode =
			audioEngineNode.firstChildElement( "coreaudio_driver" );
		if ( ! coreAudioDriverNode.isNull()  ) {
			pPref->m_sCoreAudioDevice = coreAudioDriverNode.read_string(
				"coreAudioDevice", pPref->m_sCoreAudioDevice, false, true,
				bSilent );
		}
		else {
			WARNINGLOG( "<coreaudio_driver> node not found" );
		}

		//// JACK DRIVER ////
		const XMLNode jackDriverNode =
			audioEngineNode.firstChildElement( "jack_driver" );
		if ( ! jackDriverNode.isNull() ) {
			pPref->m_sJackPortName1 = jackDriverNode.read_string(
				"jack_port_name_1",
				pPref->m_sJackPortName1, false, false, bSilent );
			pPref->m_sJackPortName2 = jackDriverNode.read_string(
				"jack_port_name_2",
				pPref->m_sJackPortName2, false, false, bSilent );
			const QString sMode = jackDriverNode.read_string(
				"jack_transport_mode", "", false, false, bSilent );
			if ( sMode == "NO_JACK_TRANSPORT" ) {
				pPref->m_bJackTransportMode = NO_JACK_TRANSPORT;
			} else if ( sMode == "USE_JACK_TRANSPORT" ) {
				pPref->m_bJackTransportMode = USE_JACK_TRANSPORT;
			}

			pPref->m_bJackTimebaseEnabled = jackDriverNode.read_bool(
				"jack_timebase_enabled",
				pPref->m_bJackTimebaseEnabled, false, false, bSilent );

			// Constructor's default value will only be overwritten in case the
			// parameter is present and well formatted.
			const QString sJackMasterMode = jackDriverNode.read_string(
				"jack_transport_mode_master", "", false, false, bSilent );
			if ( sJackMasterMode == "NO_JACK_TIME_MASTER" ) {
				pPref->m_bJackMasterMode = NO_JACK_TIME_MASTER;
			}
			else if ( sJackMasterMode == "USE_JACK_TIME_MASTER" ) {
				pPref->m_bJackMasterMode = USE_JACK_TIME_MASTER;
			}
			else if ( ! sJackMasterMode.isEmpty() ){
				WARNINGLOG( QString( "Unable to parse <jack_transport_mode_master>: [%1]" )
							.arg( sJackMasterMode ) );
			}

			pPref->m_bJackTrackOuts = jackDriverNode.read_bool(
				"jack_track_outs", pPref->m_bJackTrackOuts, false, false, bSilent );
			pPref->m_bJackConnectDefaults = jackDriverNode.read_bool(
				"jack_connect_defaults",
				pPref->m_bJackConnectDefaults, false, false, bSilent );

			const int nJackTrackOutputMode = jackDriverNode.read_int(
				"jack_track_output_mode", -255, false, false, bSilent );
			if ( nJackTrackOutputMode == 0 ) {
				pPref->m_JackTrackOutputMode = JackTrackOutputMode::postFader;
			}
			else if ( nJackTrackOutputMode == 1 ) {
				pPref->m_JackTrackOutputMode = JackTrackOutputMode::preFader;
			}
			else if ( nJackTrackOutputMode != -255 ) {
				WARNINGLOG( QString( "Unable to parse <jack_track_output_mode>: [%1]" )
							.arg( nJackTrackOutputMode ) );
			}
		}
		else {
			WARNINGLOG( "<jack_driver> node not found" );
		}

		/// ALSA AUDIO DRIVER ///
		const XMLNode alsaAudioDriverNode =
			audioEngineNode.firstChildElement( "alsa_audio_driver" );
		if ( ! alsaAudioDriverNode.isNull() ) {
			pPref->m_sAlsaAudioDevice = alsaAudioDriverNode.read_string(
				"alsa_audio_device",
				pPref->m_sAlsaAudioDevice, false, false, bSilent );
		} else {
			WARNINGLOG( "<alsa_audio_driver> node not found" );
		}

		/// MIDI DRIVER ///
		const XMLNode midiDriverNode =
			audioEngineNode.firstChildElement( "midi_driver" );
		if ( ! midiDriverNode.isNull() ) {
			pPref->m_sMidiDriver = midiDriverNode.read_string(
				"driverName", pPref->m_sMidiDriver, false, false, bSilent );
			// Ensure compatibility with older versions of the
			// files after capitalization in the GUI
			// (2021-02-05). This can be dropped in releases
			// >= 1.2
			if ( pPref->m_sMidiDriver == "JackMidi" ) {
				pPref->m_sMidiDriver = "JACK-MIDI";
			} else if ( pPref->m_sMidiDriver == "CoreMidi" ) {
				pPref->m_sMidiDriver = "CoreMIDI";
			}
			pPref->m_sMidiPortName = midiDriverNode.read_string(
				"port_name", pPref->m_sMidiPortName, false, false, bSilent );
			pPref->m_sMidiOutputPortName = midiDriverNode.read_string(
				"output_port_name",
				pPref->m_sMidiOutputPortName, false, false, bSilent );
			pPref->m_nMidiChannelFilter = midiDriverNode.read_int(
				"channel_filter",
				pPref->m_nMidiChannelFilter, false, false, bSilent );
			pPref->m_bMidiNoteOffIgnore = midiDriverNode.read_bool(
				"ignore_note_off",
				pPref->m_bMidiNoteOffIgnore, false, false, bSilent );
			pPref->m_bMidiDiscardNoteAfterAction = midiDriverNode.read_bool(
				"discard_note_after_action",
				pPref->m_bMidiDiscardNoteAfterAction, false, false, bSilent );
			pPref->m_bMidiFixedMapping = midiDriverNode.read_bool(
				"fixed_mapping",
				pPref->m_bMidiFixedMapping, false, true, bSilent );
			pPref->m_bEnableMidiFeedback = midiDriverNode.read_bool(
				"enable_midi_feedback",
				pPref->m_bEnableMidiFeedback, false, true, bSilent );
		}
		else {
			WARNINGLOG( "<midi_driver> node not found" );
		}

		/// OSC ///
		const XMLNode oscServerNode =
			audioEngineNode.firstChildElement( "osc_configuration" );
		if ( ! oscServerNode.isNull() ) {
			pPref->m_bOscServerEnabled = oscServerNode.read_bool(
				"oscEnabled", pPref->m_bOscServerEnabled, false, false, bSilent );
			pPref->m_bOscFeedbackEnabled = oscServerNode.read_bool(
				"oscFeedbackEnabled",
				pPref->m_bOscFeedbackEnabled, false, false, bSilent );
			pPref->m_nOscServerPort = oscServerNode.read_int(
				"oscServerPort", pPref->m_nOscServerPort, false, false, bSilent );
		}
		else {
			WARNINGLOG( "<osc_configuration> node not found" );
		}
	}

	/////////////// GUI //////////////
	XMLNode guiNode = rootNode.firstChildElement( "gui" );
	if ( ! guiNode.isNull() ) {
		QString sQTStyle = guiNode.read_string(
			"QTStyle", interfaceTheme.m_sQTStyle, false, true, bSilent );

		if ( sQTStyle == "Plastique" ){
			sQTStyle = "Fusion";
		}
		interfaceTheme.m_sQTStyle = sQTStyle;

		fontTheme.m_sApplicationFontFamily = guiNode.read_string(
			"application_font_family",
			fontTheme.m_sApplicationFontFamily, false, false, bSilent );
		fontTheme.m_sLevel2FontFamily = guiNode.read_string(
			"level2_font_family",
			fontTheme.m_sLevel2FontFamily, false, false, bSilent );
		fontTheme.m_sLevel3FontFamily = guiNode.read_string(
			"level3_font_family",
			fontTheme.m_sLevel3FontFamily, false, false, bSilent );
		fontTheme.m_fontSize = static_cast<FontTheme::FontSize>(
			guiNode.read_int(
				"font_size",
				static_cast<int>( fontTheme.m_fontSize ), false, false, bSilent ) );

		interfaceTheme.m_fMixerFalloffSpeed = guiNode.read_float(
			"mixer_falloff_speed",
			interfaceTheme.m_fMixerFalloffSpeed, false, false, bSilent );

		pPref->m_nPatternEditorGridResolution = guiNode.read_int(
			"patternEditorGridResolution",
			pPref->m_nPatternEditorGridResolution, false, false, bSilent );
		pPref->m_bPatternEditorUsingTriplets = guiNode.read_bool(
			"patternEditorUsingTriplets",
			pPref->m_bPatternEditorUsingTriplets, false, false, bSilent );
				
		pPref->m_bShowInstrumentPeaks = guiNode.read_bool(
			"showInstrumentPeaks",
			pPref->m_bShowInstrumentPeaks, false, false, bSilent );
		pPref->m_bIsFXTabVisible = guiNode.read_bool(
			"isFXTabVisible", pPref->m_bIsFXTabVisible, false, false, bSilent );
		pPref->m_bShowAutomationArea = guiNode.read_bool(
			"showAutomationArea",
			pPref->m_bShowAutomationArea, false, false, bSilent );
		pPref->m_bShowPlaybackTrack = guiNode.read_bool(
			"showPlaybackTrack",
			pPref->m_bShowPlaybackTrack, false, false, bSilent );


		// pattern editor grid geometry
		pPref->m_nPatternEditorGridHeight = guiNode.read_int(
			"patternEditorGridHeight",
			pPref->m_nPatternEditorGridHeight, false, false, bSilent );
		pPref->m_nPatternEditorGridWidth = guiNode.read_int(
			"patternEditorGridWidth",
			pPref->m_nPatternEditorGridWidth, false, false, bSilent );

		// song editor grid geometry
		pPref->m_nSongEditorGridHeight = guiNode.read_int(
			"songEditorGridHeight",
			pPref->m_nSongEditorGridHeight, false, false, bSilent );
		pPref->m_nSongEditorGridWidth = guiNode.read_int(
			"songEditorGridWidth",
			pPref->m_nSongEditorGridWidth, false, false, bSilent );

		// mainForm window properties
		pPref->setMainFormProperties(
			loadWindowPropertiesFrom( guiNode, "mainForm_properties",
									  pPref->m_mainFormProperties, bSilent ) );
		pPref->setMixerProperties(
			loadWindowPropertiesFrom( guiNode, "mixer_properties",
									  pPref->m_mixerProperties, bSilent ) );
		pPref->setPatternEditorProperties(
			loadWindowPropertiesFrom( guiNode, "patternEditor_properties",
									  pPref->m_patternEditorProperties, bSilent ) );
		pPref->setSongEditorProperties(
			loadWindowPropertiesFrom( guiNode, "songEditor_properties",
									  pPref->m_songEditorProperties, bSilent ) );
		pPref->setInstrumentRackProperties(
			loadWindowPropertiesFrom( guiNode, "instrumentRack_properties",
									  pPref->m_instrumentRackProperties, bSilent ) );
		pPref->setAudioEngineInfoProperties(
			loadWindowPropertiesFrom( guiNode, "audioEngineInfo_properties",
									  pPref->m_audioEngineInfoProperties, bSilent ) );
		// In order to be backward compatible we still call the XML node
		// "playlistDialog". For some time we had playlistEditor and
		// playlistDialog coexisting.
		pPref->setPlaylistEditorProperties(
			loadWindowPropertiesFrom( guiNode, "playlistDialog_properties",
									  pPref->m_playlistEditorProperties, bSilent ) );
		pPref->setDirectorProperties(
			loadWindowPropertiesFrom( guiNode, "director_properties",
									  pPref->m_directorProperties, bSilent ) );

		// last used file dialog folders
		pPref->m_sLastExportPatternAsDirectory = guiNode.read_string(
			"lastExportPatternAsDirectory",
			pPref->m_sLastExportPatternAsDirectory, true, false, bSilent );
		pPref->m_sLastExportSongDirectory = guiNode.read_string(
			"lastExportSongDirectory",
			pPref->m_sLastExportSongDirectory, true, false, bSilent );
		pPref->m_sLastSaveSongAsDirectory = guiNode.read_string(
			"lastSaveSongAsDirectory",
			pPref->m_sLastSaveSongAsDirectory, true, false, bSilent );
		pPref->m_sLastOpenSongDirectory = guiNode.read_string(
			"lastOpenSongDirectory",
			pPref->m_sLastOpenSongDirectory, true, false, bSilent );
		pPref->m_sLastOpenPatternDirectory = guiNode.read_string(
			"lastOpenPatternDirectory",
			pPref->m_sLastOpenPatternDirectory, true, false, bSilent );
		pPref->m_sLastExportLilypondDirectory = guiNode.read_string(
			"lastExportLilypondDirectory",
			pPref->m_sLastExportLilypondDirectory, true, false, bSilent );
		pPref->m_sLastExportMidiDirectory = guiNode.read_string(
			"lastExportMidiDirectory",
			pPref->m_sLastExportMidiDirectory, true, false, bSilent );
		pPref->m_sLastImportDrumkitDirectory = guiNode.read_string(
			"lastImportDrumkitDirectory",
			pPref->m_sLastImportDrumkitDirectory, true, false, bSilent );
		pPref->m_sLastExportDrumkitDirectory = guiNode.read_string(
			"lastExportDrumkitDirectory",
			pPref->m_sLastExportDrumkitDirectory, true, false, bSilent );
		pPref->m_sLastOpenLayerDirectory = guiNode.read_string(
			"lastOpenLayerDirectory",
			pPref->m_sLastOpenLayerDirectory, true, false, bSilent );
		pPref->m_sLastOpenPlaybackTrackDirectory = guiNode.read_string(
			"lastOpenPlaybackTrackDirectory",
			pPref->m_sLastOpenPlaybackTrackDirectory, true, false, bSilent );
		pPref->m_sLastAddSongToPlaylistDirectory = guiNode.read_string(
			"lastAddSongToPlaylistDirectory",
			pPref->m_sLastAddSongToPlaylistDirectory, true, false, bSilent );
		pPref->m_sLastPlaylistDirectory = guiNode.read_string(
			"lastPlaylistDirectory",
			pPref->m_sLastPlaylistDirectory, true, false, bSilent );
		pPref->m_sLastPlaylistScriptDirectory = guiNode.read_string(
			"lastPlaylistScriptDirectory",
			pPref->m_sLastPlaylistScriptDirectory, true, false, bSilent );
		pPref->m_sLastImportThemeDirectory = guiNode.read_string(
			"lastImportThemeDirectory",
			pPref->m_sLastImportThemeDirectory, true, false, bSilent );
		pPref->m_sLastExportThemeDirectory = guiNode.read_string(
			"lastExportThemeDirectory",
			pPref->m_sLastExportThemeDirectory, true, false, bSilent );

		// export dialog properties
		pPref->m_nExportTemplateIdx = guiNode.read_int(
			"exportDialogTemplate",
			pPref->m_nExportTemplateIdx, false, false, bSilent );
		pPref->m_nExportModeIdx = guiNode.read_int(
			"exportDialogMode", pPref->m_nExportModeIdx, false, false, bSilent );
		pPref->m_nExportSampleRateIdx = guiNode.read_int(
			"exportDialogSampleRate",
			pPref->m_nExportSampleRateIdx, false, false, bSilent );
		pPref->m_nExportSampleDepthIdx = guiNode.read_int(
			"exportDialogSampleDepth",
			pPref->m_nExportSampleDepthIdx, false, false, bSilent );
		pPref->m_bShowExportSongLicenseWarning = guiNode.read_bool(
			"showExportSongLicenseWarning",
			pPref->m_bShowExportSongLicenseWarning, true, false, bSilent );
		pPref->m_bShowExportDrumkitLicenseWarning = guiNode.read_bool(
			"showExportDrumkitLicenseWarning",
			pPref->m_bShowExportDrumkitLicenseWarning, true, false, bSilent );
		pPref->m_bShowExportDrumkitCopyleftWarning = guiNode.read_bool(
			"showExportDrumkitCopyleftWarning",
			pPref->m_bShowExportDrumkitCopyleftWarning, true, false, bSilent );
		pPref->m_bShowExportDrumkitAttributionWarning = guiNode.read_bool(
			"showExportDrumkitAttributionWarning",
			pPref->m_bShowExportDrumkitAttributionWarning, true, false, bSilent );
				
		pPref->m_bFollowPlayhead = guiNode.read_bool(
			"followPlayhead", pPref->m_bFollowPlayhead, false, false, bSilent );

		// midi export dialog properties
		pPref->m_nMidiExportMode = guiNode.read_int(
			"midiExportDialogMode", pPref->m_nMidiExportMode, false, false, bSilent );
				
		// beatcounter
		const QString sUseBeatCounter =
			guiNode.read_string( "bc", "", false, false, bSilent );
		if ( sUseBeatCounter == "BC_OFF" ) {
			pPref->m_bBbc = BC_OFF;
		}
		else if ( sUseBeatCounter == "BC_ON" ) {
			pPref->m_bBbc = BC_ON;
		}
		else if ( ! sUseBeatCounter.isEmpty() ) {
			WARNINGLOG( QString( "Unable to parse <bc>: [%1]" )
						.arg( sUseBeatCounter ) );
		}

		const QString sBeatCounterSetPlay =
			guiNode.read_string( "setplay", "", false, false, bSilent );
		if ( sBeatCounterSetPlay == "SET_PLAY_OFF" ) {
			pPref->m_bMmcSetPlay = SET_PLAY_OFF;
		}
		else if ( sBeatCounterSetPlay == "SET_PLAY_ON" ) {
			pPref->m_bMmcSetPlay = SET_PLAY_ON;
		}
		else if ( ! sBeatCounterSetPlay.isEmpty() ) {
			WARNINGLOG( QString( "Unable to parse <setplay>: [%1]" )
						.arg( sBeatCounterSetPlay ) );
		}

		pPref->m_nCountOffset = guiNode.read_int(
			"countoffset", pPref->m_nCountOffset, false, false, bSilent );
		pPref->m_nStartOffset = guiNode.read_int(
			"playoffset", pPref->m_nStartOffset, false, false, bSilent );

		// ~ beatcounter

		pPref->m_nAutosavesPerHour = guiNode.read_int(
			"autosavesPerHour", pPref->m_nAutosavesPerHour, false, false, bSilent );
				
		// SoundLibraryPanel expand items
		pPref->m_bExpandSongItem = guiNode.read_bool(
			"expandSongItem", pPref->m_bExpandSongItem, false, false, bSilent );
		pPref->m_bExpandPatternItem = guiNode.read_bool(
			"expandPatternItem", pPref->m_bExpandPatternItem, false, false, bSilent );

		for ( unsigned nFX = 0; nFX < MAX_FX; nFX++ ) {
			const QString sNodeName = QString( "ladspaFX_properties%1" ).arg( nFX );
			pPref->setLadspaProperties(
				nFX, loadWindowPropertiesFrom(
					guiNode, sNodeName, pPref->m_ladspaProperties[ nFX ],
					bSilent ) );
		}

		const XMLNode colorThemeNode = guiNode.firstChildElement( "colorTheme" );
		if ( ! colorThemeNode.isNull() ) {
			colorTheme = ColorTheme::loadFrom( colorThemeNode, bSilent );
		}
		else {
			WARNINGLOG( "<colorTheme> node not found" );
		}

		// SongEditor coloring
		interfaceTheme.m_coloringMethod = static_cast<InterfaceTheme::ColoringMethod>(
				guiNode.read_int( "SongEditor_ColoringMethod",
								  static_cast<int>(interfaceTheme.m_coloringMethod),
								  false, false, bSilent ) );
		std::vector<QColor> patternColors( InterfaceTheme::nMaxPatternColors );
		for ( int ii = 0; ii < InterfaceTheme::nMaxPatternColors; ii++ ) {
			patternColors[ ii ] = guiNode.read_color(
				QString( "SongEditor_pattern_color_%1" ).arg( ii ),
				colorTheme.m_accentColor, false, false, bSilent );
		}
		interfaceTheme.m_patternColors = patternColors;
		interfaceTheme.m_nVisiblePatternColors = std::clamp(
			guiNode.read_int(
				"SongEditor_visible_pattern_colors",
				interfaceTheme.m_nVisiblePatternColors, false, false, bSilent ),
			0, 50 );
	}
	else {
		WARNINGLOG( "<gui> node not found" );
	}

	/////////////// FILES //////////////
	const XMLNode filesNode = rootNode.firstChildElement( "files" );
	if ( ! filesNode.isNull() ) {
		pPref->m_sLastSongFilename = filesNode.read_string(
			"lastSongFilename", pPref->m_sLastSongFilename, false, true, bSilent );
		pPref->m_sLastPlaylistFilename = filesNode.read_string(
			"lastPlaylistFilename",
			pPref->m_sLastPlaylistFilename, false, true, bSilent );
		pPref->m_sDefaultEditor = filesNode.read_string(
			"defaulteditor", pPref->m_sDefaultEditor, false, true, bSilent );
	}
	else {
		WARNINGLOG( "<files> node not found" );
	}

	// Midi map
	MidiMap::reset_instance();
	MidiMap* mM = MidiMap::get_instance();

	const XMLNode midiEventMapNode = rootNode.firstChildElement( "midiEventMap" );
	if ( ! midiEventMapNode.isNull() ) {
		XMLNode eventNode = midiEventMapNode.firstChildElement( "midiEvent" );
		while ( ! eventNode.isNull() ) {
			const QString sNodeName = eventNode.firstChildElement().nodeName();
			if ( sNodeName == "mmcEvent" ) {
				std::shared_ptr<Action> pAction = std::make_shared<Action>(
					eventNode.firstChildElement( "action" ).text() );
				pAction->setParameter1(
					eventNode.firstChildElement( "parameter" ).text() );
				pAction->setParameter2(
					eventNode.firstChildElement( "parameter2" ).text() );
				pAction->setParameter3(
					eventNode.firstChildElement( "parameter3" ).text() );

				mM->registerMMCEvent(
					eventNode.firstChildElement( "mmcEvent" ).text(),
					pAction );
			}
			else if ( sNodeName == "noteEvent" ) {
				std::shared_ptr<Action> pAction = std::make_shared<Action>(
					eventNode.firstChildElement( "action" ).text() );
				pAction->setParameter1(
					eventNode.firstChildElement( "parameter" ).text() );
				pAction->setParameter2(
					eventNode.firstChildElement( "parameter2" ).text() );
				pAction->setParameter3(
					eventNode.firstChildElement( "parameter3" ).text() );

				mM->registerNoteEvent(
					eventNode.firstChildElement( "eventParameter").text().toInt(),
					pAction );
			}
			else if ( sNodeName == "ccEvent" ){
				std::shared_ptr<Action> pAction = std::make_shared<Action>(
					eventNode.firstChildElement( "action" ).text() );
				pAction->setParameter1(
					eventNode.firstChildElement( "parameter" ).text() );
				pAction->setParameter2(
					eventNode.firstChildElement( "parameter2" ).text() );
				pAction->setParameter3(
					eventNode.firstChildElement( "parameter3" ).text() );
				mM->registerCCEvent(
					eventNode.firstChildElement( "eventParameter" ).text().toInt(),
					pAction );
			}
			else if ( sNodeName == "pcEvent" ){
				std::shared_ptr<Action> pAction = std::make_shared<Action>(
					eventNode.firstChildElement( "action" ).text() );
				pAction->setParameter1(
					eventNode.firstChildElement( "parameter" ).text() );
				pAction->setParameter2(
					eventNode.firstChildElement( "parameter2" ).text() );
				pAction->setParameter3(
					eventNode.firstChildElement( "parameter3" ).text() );
				mM->registerPCEvent( pAction );
			}
			else {
				WARNINGLOG( QString( "Unknown MIDI map node [%1]" )
							.arg( sNodeName ) );
			}

			eventNode = eventNode.nextSiblingElement( "midiEvent" );
		}
	}
	else {
		WARNINGLOG( "<midiMap> node not found" );
	}

	pPref->m_theme = Theme( colorTheme, interfaceTheme, fontTheme );

	// Shortcuts
	pPref->m_pShortcuts = Shortcuts::loadFrom( rootNode, bSilent );

	return pPref;
}

bool Preferences::saveCopyAs( const QString& sPath, const bool bSilent ) const {
	return saveTo( sPath, bSilent );
}

bool Preferences::save( const bool bSilent ) const {
	return saveTo( Filesystem::usr_config_path(), bSilent );
}

bool Preferences::saveTo( const QString& sPath, const bool bSilent ) const {
	if ( ! bSilent ) {
		INFOLOG( QString( "Saving preferences file into [%1]" ).arg( sPath ) );
	}

	const auto interfaceTheme = m_theme.m_interface;
	const auto fontTheme = m_theme.m_font;

	XMLDoc doc;
	XMLNode rootNode = doc.set_root( "hydrogen_preferences" );

	// hydrogen version
	rootNode.write_int( "formatVersion", nCurrentFormatVersion );
	rootNode.write_string( "version", QString( get_version().c_str() ) );

	////// GENERAL ///////
	rootNode.write_string( "preferredLanguage", m_sPreferredLanguage );

	rootNode.write_bool( "useLash", m_bSetLash );
	rootNode.write_bool( "useTimeLine", m_bUseTimelineBpm );

	rootNode.write_int( "maxBars", m_nMaxBars );
	rootNode.write_int( "maxLayers", m_nMaxLayers );

	rootNode.write_int( "defaultUILayout", static_cast<int>(
							interfaceTheme.m_layout) );
	rootNode.write_int( "uiScalingPolicy", static_cast<int>(
							interfaceTheme.m_uiScalingPolicy) );
	rootNode.write_int( "lastOpenTab", m_nLastOpenTab );

	rootNode.write_bool( "useTheRubberbandBpmChangeEvent", m_bUseTheRubberbandBpmChangeEvent );

	rootNode.write_bool( "useRelativeFilenamesForPlaylists", m_bUseRelativeFilenamesForPlaylists );
	rootNode.write_bool( "hideKeyboardCursorWhenUnused", m_bHideKeyboardCursor );
	
	// instrument input mode
	rootNode.write_bool( "instrumentInputMode", m_bPlaySelectedInstrument );
	
	//show development version warning
	rootNode.write_bool( "showDevelWarning", m_bShowDevelWarning );

	// Warn about overwriting notes
	rootNode.write_bool( "showNoteOverwriteWarning", m_bShowNoteOverwriteWarning );

	// hear new notes in the pattern editor
	rootNode.write_bool( "hearNewNotes", m_bHearNewNotes );

	// key/midi event prefs
	rootNode.write_bool( "quantizeEvents", m_bQuantizeEvents );

	//extern executables
	QString rubberBandCLIexecutable( m_sRubberBandCLIexecutable );
	if ( !Filesystem::file_executable( rubberBandCLIexecutable, true /* silent */) ) {
		rubberBandCLIexecutable = "Path to Rubberband-CLI";
	}
	rootNode.write_string( "path_to_rubberband", rubberBandCLIexecutable );

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

	XMLNode serverListNode = rootNode.createNode( "serverList" );
	for ( const auto& ssServer : m_serverList ){
		serverListNode.write_string( "server", ssServer );
	}

	XMLNode patternCategoriesNode = rootNode.createNode( "patternCategories" );
	for ( const auto& ssCategory : m_patternCategories ){
		patternCategoriesNode.write_string( "categories", ssCategory );
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

			//jack time master
			jackDriverNode.write_bool( "jack_timebase_enabled", m_bJackTimebaseEnabled );
			QString tmMode;
			if ( m_bJackMasterMode == NO_JACK_TIME_MASTER ) {
				tmMode = "NO_JACK_TIME_MASTER";
			} else if (  m_bJackMasterMode == USE_JACK_TIME_MASTER ) {
				tmMode = "USE_JACK_TIME_MASTER";
			}
			jackDriverNode.write_string( "jack_transport_mode_master", tmMode );
			// ~ jack time master

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
		guiNode.write_string( "QTStyle", interfaceTheme.m_sQTStyle );
		guiNode.write_string( "application_font_family",
							  fontTheme.m_sApplicationFontFamily );
		guiNode.write_string( "level2_font_family",
							  fontTheme.m_sLevel2FontFamily );
		guiNode.write_string( "level3_font_family",
							  fontTheme.m_sLevel3FontFamily );
		guiNode.write_int( "font_size",
						   static_cast<int>(fontTheme.m_fontSize) );
		guiNode.write_float( "mixer_falloff_speed",
							 interfaceTheme.m_fMixerFalloffSpeed );
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
		saveWindowPropertiesTo( guiNode, "mainForm_properties", m_mainFormProperties );
		saveWindowPropertiesTo( guiNode, "mixer_properties", m_mixerProperties );
		saveWindowPropertiesTo( guiNode, "patternEditor_properties", m_patternEditorProperties );
		saveWindowPropertiesTo( guiNode, "songEditor_properties", m_songEditorProperties );
		saveWindowPropertiesTo( guiNode, "instrumentRack_properties", m_instrumentRackProperties );
		saveWindowPropertiesTo( guiNode, "audioEngineInfo_properties", m_audioEngineInfoProperties );
		saveWindowPropertiesTo( guiNode, "playlistDialog_properties", m_playlistEditorProperties );
		saveWindowPropertiesTo( guiNode, "director_properties", m_directorProperties );
		for ( unsigned nFX = 0; nFX < MAX_FX; nFX++ ) {
			QString sNode = QString("ladspaFX_properties%1").arg( nFX );
			saveWindowPropertiesTo( guiNode, sNode, m_ladspaProperties[nFX] );
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
		guiNode.write_int( "exportDialogTemplate", m_nExportTemplateIdx );
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

		if ( m_bBbc == BC_OFF ) {
			bcMode = "BC_OFF";
		} else if ( m_bBbc  == BC_ON ) {
			bcMode = "BC_ON";
		}
		guiNode.write_string( "bc", bcMode );

		QString setPlay;
		if ( m_bMmcSetPlay == SET_PLAY_OFF ) {
			setPlay = "SET_PLAY_OFF";
		} else if ( m_bMmcSetPlay == SET_PLAY_ON ) {
			setPlay = "SET_PLAY_ON";
		}
		guiNode.write_string( "setplay", setPlay );

		guiNode.write_int( "countoffset", m_nCountOffset );
		guiNode.write_int( "playoffset", m_nStartOffset );
		// ~ beatcounter

		guiNode.write_int( "autosavesPerHour", m_nAutosavesPerHour );

		//SoundLibraryPanel expand items
		guiNode.write_bool( "expandSongItem", m_bExpandSongItem );
		guiNode.write_bool( "expandPatternItem", m_bExpandPatternItem );

		// User interface style
		m_theme.m_color.saveTo( guiNode );

		//SongEditor coloring method
		guiNode.write_int( "SongEditor_ColoringMethod",
						   static_cast<int>(m_theme.m_interface.m_coloringMethod ) );
		for ( int ii = 0; ii < InterfaceTheme::nMaxPatternColors; ii++ ) {
			guiNode.write_color( QString( "SongEditor_pattern_color_%1" ).arg( ii ),
								 m_theme.m_interface.m_patternColors[ ii ] );
		}
		guiNode.write_int( "SongEditor_visible_pattern_colors",
						   m_theme.m_interface.m_nVisiblePatternColors );
	}

	//---- FILES ----
	XMLNode filesNode = rootNode.createNode( "files" );
	{
		// last used song
		filesNode.write_string( "lastSongFilename", m_sLastSongFilename );
		filesNode.write_string( "lastPlaylistFilename", m_sLastPlaylistFilename );
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

	// Shortcuts
	m_pShortcuts->saveTo( rootNode );

	return doc.write( sPath );
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

void Preferences::setMostRecentFX( const QString& FX_name )
{
	int pos = m_recentFX.indexOf( FX_name );

	if ( pos != -1 ) {
		m_recentFX.removeAt( pos );
	}

	m_recentFX.push_front( FX_name );
}

/// Read the xml nodes related to window properties
WindowProperties Preferences::loadWindowPropertiesFrom( const XMLNode& parent,
														const QString& sWindowName,
														const WindowProperties& defaultProp,
														const bool bSilent )
{
	WindowProperties prop { defaultProp };

	const XMLNode windowPropNode  = parent.firstChildElement( sWindowName );
	if ( ! windowPropNode.isNull() ) {
		prop.visible = windowPropNode.read_bool(
			"visible", true, false, false, bSilent );
		prop.x = windowPropNode.read_int(
			"x", prop.x, false, false, bSilent );
		prop.y = windowPropNode.read_int(
			"y", prop.y, false, false, bSilent );
		prop.width = windowPropNode.read_int(
			"width", prop.width, false, false, bSilent );
		prop.height = windowPropNode.read_int(
			"height", prop.height, false, false, bSilent );
		prop.m_geometry = QByteArray::fromBase64(
			windowPropNode.read_string(
				"geometry",
				prop.m_geometry.toBase64(), false, true, bSilent ).toUtf8() );
	}
	else {
		WARNINGLOG( QString( "Error reading configuration file <%1> node not found" )
					.arg( sWindowName ) );
	}

	return prop;
}



/// Write the xml nodes related to window properties
void Preferences::saveWindowPropertiesTo( XMLNode& parent, const QString& windowName, const WindowProperties& prop )
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
