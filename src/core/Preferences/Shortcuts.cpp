/*
 * Hydrogen
 * Copyright(c) 2023-2023 The hydrogen development team
 * [hydrogen-devel@lists.sourceforge.net]
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
#include <QtGlobal>

#include <core/Preferences/Shortcuts.h>

#include <core/config.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>

// Unfortunately, there seems to be no combination of Qt6 QKeySequence
// constructor working without compiler warnings but at the same time still
// being valid Qt 5 code. Therefore, we have to circumvent this issue with some
// custom code.
#ifdef H2CORE_HAVE_QT6
  #define SEPARATOR |
#else
  #define SEPARATOR +
#endif

namespace H2Core {
Shortcuts::Shortcuts() :
	m_bRequiresDefaults( true ) {
}

Shortcuts::Shortcuts( const std::shared_ptr<Shortcuts> pOther ) {
	m_bRequiresDefaults = pOther->m_bRequiresDefaults;
	
	for ( const auto& it : pOther->m_actionInfoMap ) {
		m_actionInfoMap[ it.first ] = it.second;
	}
	for ( const auto& it : pOther->m_actionsMap ) {
		m_actionsMap[ it.first ] = it.second;
	}
}
Shortcuts::~Shortcuts() {
}

void Shortcuts::saveTo( XMLNode& node ) const {
	XMLNode shortcutsNode = node.createNode( "shortcuts" );
	for ( const auto& it : m_actionsMap ) {
		for ( const auto& aaction : it.second ) {
			XMLNode shortcutNode = shortcutsNode.createNode( "shortcut" );
			shortcutNode.write_string( "keySequence",
									   it.first.toString( QKeySequence::PortableText ) );
			shortcutNode.write_int( "action", static_cast<int>(aaction) );
		}
	}
}

std::shared_ptr<Shortcuts> Shortcuts::loadFrom( const XMLNode& node, bool bSilent ) {
	auto pShortcuts = std::make_shared<Shortcuts>();
	pShortcuts->createActionInfoMap();
	
	XMLNode shortcutsNode = node.firstChildElement( "shortcuts" );
	if ( shortcutsNode.isNull() ) {
		if ( Hydrogen::get_instance() == nullptr ||
			 Hydrogen::get_instance()->getGUIState() != H2Core::Hydrogen::GUIState::ready ) {
			// No shortcuts found. We need to create the default ones. But
			// it is essential that we do not do this right away. If no
			// QApplication is present, Qt will segfault when attempting
			// to access standard keys. Instead, this step will be delayed
			// till after the bootstrap.
			WARNINGLOG( "shortcut node not found." );
			pShortcuts->m_bRequiresDefaults = true;
		}
		else {
			pShortcuts->createDefaultShortcuts();
		}
	}
	else {
		XMLNode shortcutNode = shortcutsNode.firstChildElement( "shortcut" );

		bool bShortcutsFound = false;
		while ( ! shortcutNode.isNull() ) {
			const auto keySequence = QKeySequence::fromString(
				shortcutNode.read_string( "keySequence", "", false, false, bSilent ),
				QKeySequence::PortableText );

			if ( ! keySequence.isEmpty() ) {
				pShortcuts->insertShortcut(
					keySequence,
					static_cast<Action>(shortcutNode.read_int( "action", 0,
															   false, false, bSilent )) );
				bShortcutsFound = true;
			}
			shortcutNode = shortcutNode.nextSiblingElement( "shortcut" );
		}

		if ( bShortcutsFound ) {
			// There was at least one valid shortcut. Make sure we do not
			// replace it with teh default ones.
			pShortcuts->m_bRequiresDefaults = false;
		}
	}

	return pShortcuts;
}

void Shortcuts::createDefaultShortcuts() {
	m_actionsMap.clear();

	// POSIX Locale for keyboardlayout QWERTZ: de_DE, de_AT, de_LU, de_CH, de
	//   locale for keyboardlayout AZERTY: fr_BE, fr_CA, fr_FR, fr_LU, fr_CH
	//   locale for keyboardlayout QWERTY: en_GB, en_US, en_ZA, usw.
	const QString sLocaleName = QLocale::system().name();

	enum class locales {
		de,
		fr,
		en
	};
	
	locales locale;
	if ( sLocaleName.contains( "de" ) || sLocaleName.contains( "DE" ) ) {
		locale = locales::de;
	}
	else if ( sLocaleName.contains( "fr" ) || sLocaleName.contains( "FR" ) ) {
		locale = locales::fr;
	}
	else {
		locale = locales::en;
	}
	
	// Global shortcuts
	insertShortcut( Qt::Key_F12, Action::Panic );
	insertShortcut( Qt::ControlModifier SEPARATOR Qt::Key_S, Action::SaveSong );
	insertShortcut( Qt::ControlModifier SEPARATOR Qt::ShiftModifier SEPARATOR Qt::Key_S,
					Action::SaveAsSong );
	insertShortcut( QKeySequence::StandardKey::Undo, Action::Undo );
	insertShortcut( QKeySequence::StandardKey::Redo, Action::Redo );
	insertShortcut( Qt::Key_Space, Action::PlayPauseToggle );
#ifndef Q_OS_MACX
	insertShortcut( Qt::ControlModifier SEPARATOR Qt::Key_Space , Action::PlayPauseToggleAtCursor );
#else
	insertShortcut( Qt::AltModifier SEPARATOR Qt::Key_Space, Action::PlayPauseToggleAtCursor );
#endif
	insertShortcut( Qt::Key_Comma, Action::BeatCounter );
	insertShortcut( Qt::Key_Backslash, Action::TapTempo );
	insertShortcut( Qt::Key_Plus, Action::BPMIncreaseCoarse );
	insertShortcut( Qt::Key_Minus, Action::BPMDecreaseCoarse );
	insertShortcut( Qt::Key_Backspace, Action::JumpToStart );
	insertShortcut( Qt::Key_F10, Action::JumpBarForward );
	insertShortcut( Qt::Key_F9, Action::JumpBarBackward );
	insertShortcut( Qt::Key_F6, Action::PlaylistNextSong );
	insertShortcut( Qt::Key_F5, Action::PlaylistPrevSong );

	// MainForm actions
	insertShortcut( Qt::ControlModifier SEPARATOR Qt::Key_N, Action::NewSong );
	insertShortcut( Qt::ControlModifier SEPARATOR Qt::Key_O, Action::OpenSong );
	insertShortcut( Qt::ControlModifier SEPARATOR Qt::Key_D, Action::OpenDemoSong );
	insertShortcut( Qt::ControlModifier SEPARATOR Qt::ShiftModifier SEPARATOR Qt::Key_P,
					Action::OpenPattern );
	insertShortcut( Qt::ControlModifier SEPARATOR Qt::Key_P, Action::ExportPattern );
	insertShortcut( Qt::ControlModifier SEPARATOR Qt::Key_E, Action::ExportSong );
	insertShortcut( Qt::ControlModifier SEPARATOR Qt::Key_M, Action::ExportMIDI );
	insertShortcut( Qt::ControlModifier SEPARATOR Qt::Key_L, Action::ExportLilyPond );
#ifndef Q_OS_MACX
	insertShortcut( Qt::ControlModifier SEPARATOR Qt::Key_Q, Action::Quit );
#endif
	insertShortcut( Qt::AltModifier SEPARATOR Qt::Key_D, Action::ShowDirector );
	insertShortcut( Qt::AltModifier SEPARATOR Qt::Key_M, Action::ShowMixer );
	insertShortcut( Qt::AltModifier SEPARATOR Qt::Key_I, Action::ShowRack );
	insertShortcut( Qt::AltModifier SEPARATOR Qt::Key_A, Action::ShowAutomation );
	insertShortcut( Qt::AltModifier SEPARATOR Qt::Key_F, Action::ShowFullscreen );
	insertShortcut( Qt::ControlModifier SEPARATOR Qt::AltModifier SEPARATOR Qt::Key_I,
					Action::InputInstrument );
	insertShortcut( Qt::ControlModifier SEPARATOR Qt::AltModifier SEPARATOR Qt::Key_D,
					Action::InputDrumkit );
	insertShortcut( Qt::AltModifier SEPARATOR Qt::Key_P, Action::ShowPreferencesDialog );
	insertShortcut( Qt::ControlModifier SEPARATOR Qt::Key_Question, Action::OpenManual );

	// Virtual MIDI keyboard
	switch ( locale ) {
	case locales::de:
		insertShortcut( Qt::Key_Y, Action::VK_36_C2 );
		break;
	case locales::fr:
		insertShortcut( Qt::Key_W, Action::VK_36_C2 );
		break;
	default:
		insertShortcut( Qt::Key_Z, Action::VK_36_C2 );
	}
	insertShortcut( Qt::Key_S, Action::VK_37_C_sharp2 );
	insertShortcut( Qt::Key_X, Action::VK_38_D2 );
	insertShortcut( Qt::Key_D, Action::VK_39_D_sharp2 );
	insertShortcut( Qt::Key_C, Action::VK_40_E2 );
	insertShortcut( Qt::Key_V, Action::VK_41_F2 );
	insertShortcut( Qt::Key_G, Action::VK_42_F_sharp2 );
	insertShortcut( Qt::Key_B, Action::VK_43_G2 );
	insertShortcut( Qt::Key_H, Action::VK_44_G_sharp2 );
	insertShortcut( Qt::Key_N, Action::VK_45_A2 );
	insertShortcut( Qt::Key_J, Action::VK_46_A_sharp2 );
	switch ( locale ) {
	case locales::fr:
		insertShortcut( Qt::Key_Question, Action::VK_47_B2 );
		insertShortcut( Qt::Key_A, Action::VK_48_C3 );
		insertShortcut( Qt::Key_2, Action::VK_49_C_sharp3 );
		insertShortcut( Qt::Key_Z, Action::VK_50_D3 );
		break;
	default:
		insertShortcut( Qt::Key_M, Action::VK_47_B2 );
		insertShortcut( Qt::Key_Q, Action::VK_48_C3 );
		insertShortcut( Qt::Key_2, Action::VK_49_C_sharp3 );
		insertShortcut( Qt::Key_W, Action::VK_50_D3 );
	}
	insertShortcut( Qt::Key_3, Action::VK_51_D_sharp3 );
	insertShortcut( Qt::Key_E, Action::VK_52_E3 );
	insertShortcut( Qt::Key_R, Action::VK_53_F3 );
	insertShortcut( Qt::Key_5, Action::VK_54_F_sharp3 );
	insertShortcut( Qt::Key_T, Action::VK_55_G3 );
	insertShortcut( Qt::Key_6, Action::VK_56_G_sharp3 );
	switch ( locale ) {
	case locales::de:
		insertShortcut( Qt::Key_Z, Action::VK_57_A3 );
		break;
	default:
		insertShortcut( Qt::Key_Y, Action::VK_57_A3 );
	}
	insertShortcut( Qt::Key_7, Action::VK_58_A_sharp3 );
	insertShortcut( Qt::Key_U, Action::VK_59_B3 );

	// Playlist Editor
	insertShortcut( Qt::AltModifier SEPARATOR Qt::ControlModifier SEPARATOR Qt::Key_A, Action::PlaylistAddSong );
	insertShortcut( Qt::AltModifier SEPARATOR Qt::ControlModifier SEPARATOR Qt::ShiftModifier SEPARATOR Qt::Key_A,
					Action::PlaylistAddCurrentSong );
	insertShortcut( Qt::AltModifier SEPARATOR Qt::ControlModifier SEPARATOR Qt::Key_D, Action::PlaylistRemoveSong );
	insertShortcut( Qt::AltModifier SEPARATOR Qt::ControlModifier SEPARATOR Qt::Key_N, Action::NewPlaylist );
	insertShortcut( Qt::AltModifier SEPARATOR Qt::ControlModifier SEPARATOR Qt::Key_O, Action::OpenPlaylist );
	insertShortcut( Qt::AltModifier SEPARATOR Qt::ControlModifier SEPARATOR Qt::Key_S, Action::SavePlaylist );
	insertShortcut( Qt::AltModifier SEPARATOR Qt::ControlModifier SEPARATOR Qt::ShiftModifier SEPARATOR Qt::Key_S,
					Action::SaveAsPlaylist );

}

std::vector<Shortcuts::Action> Shortcuts::getActions( const QKeySequence& keySequence ) const {
	auto it = m_actionsMap.find( keySequence );
	if ( it != m_actionsMap.end() ) {
		// Key found
		return it->second;
	}
	else {
		return std::vector<Action>();
	}
}

void Shortcuts::deleteShortcut( const QKeySequence& keySequence,
								const Action& action ) {
	for ( auto it = m_actionsMap.begin(); it != m_actionsMap.end(); ) {
		if ( it->first == keySequence ) {
			for ( std::vector<Shortcuts::Action>::iterator itAction = it->second.begin();
				  itAction != it->second.end(); ) {
				if ( *itAction == action ) {
					itAction = it->second.erase( itAction );
				}
				else {
					++itAction;
				}
			}
			
			if ( it->second.size() == 0 ) {
				// No more actions assigned to this shortcut. Delete
				// it altogether.
				it = m_actionsMap.erase( it );
			}
			else {
				++it;
			}
		}
		else {
			++it;
		}
	}
}


Shortcuts::ActionInfo Shortcuts::getActionInfo( const Action& action ) const {
	auto it = m_actionInfoMap.find( action );
	if ( it != m_actionInfoMap.end() ) {
		// Action found
		return it->second;
	}
	else {
		return (ActionInfo){ Category::None, "" };
	}
}

QKeySequence Shortcuts::getKeySequence( const Action& action ) const {
	for ( const auto& it : m_actionsMap ) {
		for ( const auto& aaction : it.second ) {
			if ( aaction == action ) {
				return it.first;
			}
		}
	}
	return QKeySequence( "" );
}

std::vector<QKeySequence> Shortcuts::getKeySequences( const Action& action ) const {
	std::vector<QKeySequence> keySequences;
	
	for ( const auto& [kkeySequence, aactions] : m_actionsMap ) {
		for ( const auto& aaction : aactions ) {
			if ( aaction == action ) {
				keySequences.push_back( kkeySequence );
			}
		}
	}

	if ( keySequences.size() == 0 ) {
		keySequences.push_back( QKeySequence( "" ) );
	}
	
	return keySequences;
}

void Shortcuts::createActionInfoMap() {
	m_actionInfoMap.clear();

	// Core commands
	insertActionInfo( Shortcuts::Action::Panic, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Pause transport and stop all playing notes" ) );
	insertActionInfo( Shortcuts::Action::Play, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Start playback" ) );
	insertActionInfo( Shortcuts::Action::Pause, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Pause playback" ) );
	insertActionInfo( Shortcuts::Action::Stop, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Stop playback" ) );
	insertActionInfo( Shortcuts::Action::PlayPauseToggle, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Start/Pause playback" ) );
	insertActionInfo( Shortcuts::Action::PlayStopToggle, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Start/Stop playback" ) );
	insertActionInfo( Shortcuts::Action::PlayPauseToggleAtCursor, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Start playback at keyboard cursor" ) );
	
	insertActionInfo( Shortcuts::Action::RecordReady, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Record toggling (if playback isn't running)" ) );
	insertActionInfo( Shortcuts::Action::RecordStrobe, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Record activation" ) );
	insertActionInfo( Shortcuts::Action::RecordStrobeToggle, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Record toggling" ) );
	insertActionInfo( Shortcuts::Action::RecordExit, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Record deactivation" ) );
	
	insertActionInfo( Shortcuts::Action::MasterMute, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Mute master output" ) );
	insertActionInfo( Shortcuts::Action::MasterUnmute, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Unmute master output" ) );
	insertActionInfo( Shortcuts::Action::MasterMuteToggle, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Mute toggling of master output" ) );
	insertActionInfo( Shortcuts::Action::MasterVolumeIncrease, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Increase volume of master output" ) );
	insertActionInfo( Shortcuts::Action::MasterVolumeDecrease, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Decrease volume of master output" ) );
	
	insertActionInfo( Shortcuts::Action::JumpToStart, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Move playhead to the beginnning of the song" ) );
	insertActionInfo( Shortcuts::Action::JumpBarForward, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Move playhead one bar forward" ) );
	insertActionInfo( Shortcuts::Action::JumpBarBackward, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Move playhead one bar backward" ) );
	
	insertActionInfo( Shortcuts::Action::BPMIncreaseCoarse, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "BPM increase (coarse)" ) );
	insertActionInfo( Shortcuts::Action::BPMDecreaseCoarse, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "BPM decrease (coarse)" ) );
	insertActionInfo( Shortcuts::Action::BPMIncreaseFine, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "BPM increase (fine)" ) );
	insertActionInfo( Shortcuts::Action::BPMDecreaseFine, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "BPM decrease (fine)" ) );
	
	insertActionInfo( Shortcuts::Action::BeatCounter, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "BeatCounter trigger" ) );
	insertActionInfo( Shortcuts::Action::TapTempo, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Tap Tempo trigger" ) );
	
	insertActionInfo( Shortcuts::Action::PlaylistNextSong, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Playlist: select next song" ) );
	insertActionInfo( Shortcuts::Action::PlaylistPrevSong, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Playlist: select previous song" ) );
	
	insertActionInfo( Shortcuts::Action::TimelineToggle, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Toggle Timeline" ) );
	insertActionInfo( Shortcuts::Action::MetronomeToggle, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Toggle Metronome" ) );
	insertActionInfo( Shortcuts::Action::JackTransportToggle, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Toggle JACK Transport" ) );
	insertActionInfo( Shortcuts::Action::JackTimebaseToggle, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Toggle JACK Timebase support" ) );
	insertActionInfo( Shortcuts::Action::SongModeToggle, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Toggle song/pattern mode" ) );
	insertActionInfo( Shortcuts::Action::LoopModeToggle, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Toggle loop mode" ) );

	insertActionInfo( Shortcuts::Action::LoadNextDrumkit, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Switch to next drumkit of soundlibrary" ) );
	insertActionInfo( Shortcuts::Action::LoadPrevDrumkit, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Switch to previous drumkit of soundlibrary" ) );

	insertActionInfo( Shortcuts::Action::CountIn, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Count in and start playback" ) );
	insertActionInfo( Shortcuts::Action::CountInPauseToggle, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Count in and start/pause playback" ) );
	insertActionInfo( Shortcuts::Action::CountInStopToggle, Category::CommandNoArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Count in and start/stop playback" ) );

	// commands with 1 argument
	insertActionInfo( Shortcuts::Action::BPM, Category::Command1Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Set BPM" ) );
	insertActionInfo( Shortcuts::Action::MasterVolume, Category::Command1Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Set volume of master output" ) );
	insertActionInfo( Shortcuts::Action::JumpToBar, Category::Command1Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Set playhead position" ) );
	
	insertActionInfo( Shortcuts::Action::SelectNextPattern, Category::Command1Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Select next pattern" ) );
	insertActionInfo( Shortcuts::Action::SelectOnlyNextPattern, Category::Command1Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Select only next pattern" ) );
	insertActionInfo( Shortcuts::Action::SelectAndPlayPattern, Category::Command1Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Select next pattern and start playback" ) );

	insertActionInfo( Shortcuts::Action::PlaylistSong, Category::Command1Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Select Playlist song" ) );
	
	insertActionInfo( Shortcuts::Action::TimelineDeleteMarker, Category::Command1Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Timeline: delete tempo marker" ) );
	insertActionInfo( Shortcuts::Action::TimelineDeleteTag, Category::Command1Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Timeline: delete tag" ) );

	insertActionInfo( Shortcuts::Action::SelectInstrument, Category::Command1Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Set current instrument" ) );
	insertActionInfo( Shortcuts::Action::StripVolumeIncrease, Category::Command1Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Increase volume of instrument" ) );
	insertActionInfo( Shortcuts::Action::StripVolumeDecrease, Category::Command1Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Decrease volume of instrument" ) );
	insertActionInfo( Shortcuts::Action::StripMuteToggle, Category::Command1Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Toggle instrument mute" ) );
	insertActionInfo( Shortcuts::Action::StripSoloToggle, Category::Command1Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Toggle instrument solo" ) );
	
	// commands with 2 argument
	insertActionInfo( Shortcuts::Action::StripVolume, Category::Command2Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Set instrument volume" ) );
	insertActionInfo( Shortcuts::Action::StripPan, Category::Command2Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Set instrument pan" ) );
	insertActionInfo( Shortcuts::Action::StripFilterCutoff, Category::Command2Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Set instrument filter cutoff" ) );

	insertActionInfo( Shortcuts::Action::TimelineAddMarker, Category::Command2Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Timeline: add tempo marker" ) );
	insertActionInfo( Shortcuts::Action::TimelineAddTag, Category::Command2Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Timeline: add tag" ) );

	insertActionInfo( Shortcuts::Action::ToggleGridCell, Category::Command2Args,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Toggle cell in song editor grid" ) );

	// commands with many argument
	insertActionInfo( Shortcuts::Action::LayerPitch, Category::CommandManyArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Set instrument layer pitch" ) );
	insertActionInfo( Shortcuts::Action::LayerGain, Category::CommandManyArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Set instrument layer gain" ) );
	
	insertActionInfo( Shortcuts::Action::StripEffectLevel, Category::CommandManyArgs,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Set instrument FX aux level" ) );
	
	// MainMenu
	insertActionInfo( Shortcuts::Action::NewSong, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Create empty song" ) );
	insertActionInfo( Shortcuts::Action::OpenSong, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Open song from disk" ) );
	insertActionInfo( Shortcuts::Action::EditSongProperties, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Edit song properties" ) );
	insertActionInfo( Shortcuts::Action::OpenDemoSong, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Open demo song" ) );
	insertActionInfo( Shortcuts::Action::SaveSong, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Save all modifications to the current song" ) );
	insertActionInfo( Shortcuts::Action::SaveAsSong, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Save all modifications to a new song" ) );
	insertActionInfo( Shortcuts::Action::OpenPattern, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Open pattern from disk" ) );
	insertActionInfo( Shortcuts::Action::ExportPattern, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Write pattern to disk" ) );
	insertActionInfo( Shortcuts::Action::ExportSong, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Export song to audio file" ) );
	insertActionInfo( Shortcuts::Action::ExportMIDI, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Export song to MIDI file" ) );
	insertActionInfo( Shortcuts::Action::ExportLilyPond, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Export song to LilyPond file" ) );
	insertActionInfo( Shortcuts::Action::Quit, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Quit Hydrogen" ) );
	
	insertActionInfo( Shortcuts::Action::Undo, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Undo the last modification" ) );
	insertActionInfo( Shortcuts::Action::Redo, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Redo the last modification" ) );
	insertActionInfo( Shortcuts::Action::ShowUndoHistory, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Show modification history" ) );

	insertActionInfo( Shortcuts::Action::NewDrumkit, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Create empty drumkit" ) );
	insertActionInfo( Shortcuts::Action::OpenDrumkit, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Open drumkit from soundlibrary" ) );
	insertActionInfo( Shortcuts::Action::EditDrumkitProperties, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Edit drumkit properties" ) );
	insertActionInfo( Shortcuts::Action::SaveDrumkitToSoundLibrary, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Save current drumkit to Sound Library" ) );
	insertActionInfo( Shortcuts::Action::SaveDrumkitToSession, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Save current drumkit to NSM session folder" ) );
	insertActionInfo( Shortcuts::Action::ExportDrumkit, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Export drumkit to disk" ) );
	insertActionInfo( Shortcuts::Action::ImportDrumkit, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Import drumkit from disk" ) );
	insertActionInfo( Shortcuts::Action::ImportOnlineDrumkit, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Import drumkit from server" ) );

	insertActionInfo( Shortcuts::Action::AddInstrument, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Add instrument to current drumkit" ) );
	insertActionInfo( Shortcuts::Action::ClearAllInstruments, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Clear all instruments in current drumkit" ) );
	insertActionInfo( Shortcuts::Action::AddComponent, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Add component to current instrument" ) );

	insertActionInfo( Shortcuts::Action::ShowPlaylist, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Show playlist editor" ) );
	insertActionInfo( Shortcuts::Action::ShowDirector, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Show director" ) );
	insertActionInfo( Shortcuts::Action::ShowMixer, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Show mixer" ) );
	insertActionInfo( Shortcuts::Action::ShowRack, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Show instrument rack" ) );
	insertActionInfo( Shortcuts::Action::ShowAutomation, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Show automation path" ) );
	insertActionInfo( Shortcuts::Action::ShowTimeline, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Show timeline" ) );
	insertActionInfo( Shortcuts::Action::ShowPlaybackTrack, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Show playback track" ) );
	insertActionInfo( Shortcuts::Action::ShowFullscreen, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Toggle fullscreen mode" ) );
	
	insertActionInfo( Shortcuts::Action::InputInstrument, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Use instrument mode for MIDI input" ) );
	insertActionInfo( Shortcuts::Action::InputDrumkit, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Use drumkit mode for MIDI input" ) );
	insertActionInfo( Shortcuts::Action::ShowPreferencesDialog, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Show preferences dialog" ) );

	insertActionInfo( Shortcuts::Action::ShowAudioEngineInfo, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Show audio engine info dialog" ) );
	insertActionInfo( Shortcuts::Action::ShowFilesystemInfo, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Show filesystem info dialog" ) );
	insertActionInfo( Shortcuts::Action::LogLevelNone, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Log Level = None" ) );
	insertActionInfo( Shortcuts::Action::LogLevelError, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Log Level = Error" ) );
	insertActionInfo( Shortcuts::Action::LogLevelWarning, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Log Level = Warning" ) );
	insertActionInfo( Shortcuts::Action::LogLevelInfo, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Log Level = Info" ) );
	insertActionInfo( Shortcuts::Action::LogLevelDebug, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Log Level = Debug" ) );
	insertActionInfo( Shortcuts::Action::OpenLogFile, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Open log file" ) );
	insertActionInfo( Shortcuts::Action::DebugPrintObjects, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Print object debug count to log" ) );

	insertActionInfo( Shortcuts::Action::OpenManual, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Open user manual" ) );
	insertActionInfo( Shortcuts::Action::ShowAbout, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Show about dialog" ) );
	insertActionInfo( Shortcuts::Action::ShowReportBug, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Report bug in web browser" ) );
	insertActionInfo( Shortcuts::Action::ShowDonate, Category::MainMenu,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Show donate dialog" ) );

	// Virtual VK keyboard
	insertActionInfo( Shortcuts::Action::VK_36_C2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 36 (C2)" ) );
	insertActionInfo( Shortcuts::Action::VK_37_C_sharp2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 37 (C#2)" ) );
	insertActionInfo( Shortcuts::Action::VK_38_D2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 38 (D2)" ) );
	insertActionInfo( Shortcuts::Action::VK_39_D_sharp2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 39 (D#2)" ) );
	insertActionInfo( Shortcuts::Action::VK_40_E2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 40 (E2)" ) );
	insertActionInfo( Shortcuts::Action::VK_41_F2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 41 (F2)" ) );
	insertActionInfo( Shortcuts::Action::VK_42_F_sharp2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 42 (F#2)" ) );
	insertActionInfo( Shortcuts::Action::VK_43_G2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 43 (G2)" ) );
	insertActionInfo( Shortcuts::Action::VK_44_G_sharp2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 44 (G#2)" ) );
	insertActionInfo( Shortcuts::Action::VK_45_A2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 45 (A2)" ) );
	insertActionInfo( Shortcuts::Action::VK_46_A_sharp2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 46 (A#2)" ) );
	insertActionInfo( Shortcuts::Action::VK_47_B2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 47 (B2)" ) );
	insertActionInfo( Shortcuts::Action::VK_48_C3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 48 (C3)" ) );
	insertActionInfo( Shortcuts::Action::VK_49_C_sharp3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 49 (C#3)" ) );
	insertActionInfo( Shortcuts::Action::VK_50_D3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 50 (D3)" ) );
	insertActionInfo( Shortcuts::Action::VK_51_D_sharp3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 51 (D#3)" ) );
	insertActionInfo( Shortcuts::Action::VK_52_E3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 52 (E3)" ) );
	insertActionInfo( Shortcuts::Action::VK_53_F3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 53 (F3)" ) );
	insertActionInfo( Shortcuts::Action::VK_54_F_sharp3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 54 (F#3)" ) );
	insertActionInfo( Shortcuts::Action::VK_55_G3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 55 (G3)" ) );
	insertActionInfo( Shortcuts::Action::VK_56_G_sharp3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 56 (G#3)" ) );
	insertActionInfo( Shortcuts::Action::VK_57_A3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 57 (A3)" ) );
	insertActionInfo( Shortcuts::Action::VK_58_A_sharp3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 58 (A#3)" ) );
	insertActionInfo( Shortcuts::Action::VK_59_B3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "VK Note-on Pitch 59 (B3)" ) );

	// Playlist Editor
	insertActionInfo( Shortcuts::Action::PlaylistAddSong, Category::PlaylistEditor,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Add song to Playlist" ) );
	insertActionInfo( Shortcuts::Action::PlaylistAddCurrentSong, Category::PlaylistEditor,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Add current song to Playlist" ) );
	insertActionInfo( Shortcuts::Action::PlaylistRemoveSong, Category::PlaylistEditor,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Remove song from Playlist" ) );
	insertActionInfo( Shortcuts::Action::NewPlaylist, Category::PlaylistEditor,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Create new Playlist" ) );
	insertActionInfo( Shortcuts::Action::OpenPlaylist, Category::PlaylistEditor,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Open Playlist from disk" ) );
	insertActionInfo( Shortcuts::Action::SavePlaylist, Category::PlaylistEditor,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Save modifications to Playlist" ) );
	insertActionInfo( Shortcuts::Action::SaveAsPlaylist, Category::PlaylistEditor,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Save modifications to new Playlist" ) );

#ifndef WIN32
	insertActionInfo( Shortcuts::Action::PlaylistAddScript, Category::PlaylistEditor,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Add script to Playlist" ) );
	insertActionInfo( Shortcuts::Action::PlaylistEditScript, Category::PlaylistEditor,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Edit script" ) );
	insertActionInfo( Shortcuts::Action::PlaylistRemoveScript, Category::PlaylistEditor,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Remove script from Playlist" ) );
	insertActionInfo( Shortcuts::Action::PlaylistCreateScript, Category::PlaylistEditor,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Create script for Playlist" ) );
#endif
}

QString Shortcuts::categoryToQString( const Category& category ) {
	QString s;

	switch ( category ) {
	case Category::None:
		s = "";
		break;
	case Category::CommandNoArgs:
		s = QT_TRANSLATE_NOOP( "Shortcuts", "Commands (0 args)" );
		break;
	case Category::Command1Args:
		s = QT_TRANSLATE_NOOP( "Shortcuts", "Commands (1 arg)" );
		break;
	case Category::Command2Args:
		s = QT_TRANSLATE_NOOP( "Shortcuts", "Commands (2 args)" );
		break;
	case Category::CommandManyArgs:
		s = QT_TRANSLATE_NOOP( "Shortcuts", "Commands (many args)" );
		break;
	case Category::MainMenu:
		s = QT_TRANSLATE_NOOP( "Shortcuts", "Main Window" );
		break;
	case Category::VirtualKeyboard:
		s = QT_TRANSLATE_NOOP( "Shortcuts", "Virtual Keyboard" );
		break;
	case Category::PlaylistEditor:
		s = QT_TRANSLATE_NOOP( "Shortcuts", "PlaylistEditor" );
		break;
	case Category::All:
		s = QT_TRANSLATE_NOOP( "Shortcuts", "All Categories" );
		break;
	};

	return std::move( s );
};

void Shortcuts::insertShortcut( const QKeySequence& keySequence,
								const Action& action ) {
	auto it = m_actionsMap.find( keySequence );
	if ( it != m_actionsMap.end() ) {
		// Key found

		bool bAlreadyContained = false;
		for ( const auto& aactionContained : it->second ) {
			if ( aactionContained == action ) {
				bAlreadyContained = true;
				break;
			}
		}

		if ( ! bAlreadyContained ) {
			it->second.push_back( action );
		}
	}
	else {
		std::vector<Action> v;
		v.push_back( action );
		m_actionsMap[ keySequence ] = v;
	}
}

void Shortcuts::insertActionInfo( const Action& action,
								  const Category& category,
								  const QString& sDescription ) {
	m_actionInfoMap[ action ] = { category, sDescription };
}

QString Shortcuts::toQString( const QString& sPrefix, bool bShort ) const {

	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Shortcuts]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_actionInfoMap: \n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& it : m_actionInfoMap ) {
			sOutput.append( QString( "%1%2%2Action: %3 : [category: %4, sDescription: %5]\n" )
							.arg( sPrefix ).arg( s )
							.arg( static_cast<int>(it.first) )
							.arg( static_cast<int>(it.second.category) )
							.arg( it.second.sDescription) );
		}
		sOutput.append( QString( "%1%2m_actionsMap: \n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& it : m_actionsMap ) {
			sOutput.append( QString( "%1%2%2KeySequence: %3 : [" )
							.arg( sPrefix ).arg( s )
							.arg( it.first.toString( QKeySequence::PortableText ) ) );
			for ( const auto& aaction : it.second ) {
				sOutput.append( QString( ", Action: %1" )
								.arg( static_cast<int>(aaction ) ) );
			}
			sOutput.append( "]\n" );
		}
	}
	else {
		sOutput = QString( "[Shortcuts]" )
			.append( QString( " m_actionInfoMap: [" ) );
		for ( const auto& it : m_actionInfoMap ) {
			sOutput.append( QString( " , Action: %1 : [category: %2, sDescription: %3]" )
							.arg( static_cast<int>(it.first) )
							.arg( static_cast<int>(it.second.category) )
							.arg( it.second.sDescription) );
		}
		sOutput.append( QString( "], m_actionsMap: [" ) );
		for ( const auto& it : m_actionsMap ) {
			sOutput.append( QString( ", KeySequence: %1 : [" )
							.arg( it.first.toString( QKeySequence::PortableText ) ) );
			for ( const auto& aaction : it.second ) {
				sOutput.append( QString( ", Action: %1" )
								.arg( static_cast<int>(aaction ) ) );
			}
			sOutput.append( "]" );
		}
		sOutput.append( "]" );
	}

	return std::move( sOutput );
}

};
