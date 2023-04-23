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
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>

namespace H2Core {
Shortcuts::Shortcuts() :
	m_bRequiresDefaults( false ) {
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

void Shortcuts::saveTo( XMLNode* pNode ) {
	XMLNode shortcutsNode = pNode->createNode( "shortcuts" );
	for ( const auto& it : m_actionsMap ) {
		for ( const auto& aaction : it.second ) {
			XMLNode shortcutNode = shortcutsNode.createNode( "shortcut" );
			shortcutNode.write_string( "keySequence",
									   it.first.toString( QKeySequence::PortableText ) );
			shortcutNode.write_int( "action", static_cast<int>(aaction) );
		}
	}
}

std::shared_ptr<Shortcuts> Shortcuts::loadFrom( XMLNode* pNode, bool bSilent ) {
	auto pShortcuts = std::make_shared<Shortcuts>();
	pShortcuts->createActionInfoMap();
	
	XMLNode shortcutsNode = pNode->firstChildElement( "shortcuts" );
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

		while ( ! shortcutNode.isNull() ) {
			const auto keySequence = QKeySequence::fromString(
				shortcutNode.read_string( "keySequence", "", false, false, bSilent ),
				QKeySequence::PortableText );

			if ( ! keySequence.isEmpty() ) {
				pShortcuts->insertShortcut(
					keySequence,
					static_cast<Action>(shortcutNode.read_int( "action", 0,
															   false, false, bSilent )) );
			}
			shortcutNode = shortcutNode.nextSiblingElement( "shortcut" );
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
	insertShortcut( Qt::Key_S+Qt::ControlModifier, Action::Save );
	insertShortcut( Qt::Key_S+Qt::ControlModifier+Qt::ShiftModifier, Action::SaveAs );
	insertShortcut( QKeySequence::StandardKey::Undo, Action::Undo );
	insertShortcut( QKeySequence::StandardKey::Redo, Action::Redo );
	insertShortcut( Qt::Key_Space, Action::TogglePlayback );
#ifndef Q_OS_MACX
	insertShortcut( Qt::Key_Space+Qt::ControlModifier, Action::TogglePlaybackAtCursor );
#else
	insertShortcut( Qt::Key_Space+Qt::AltModifier, Action::TogglePlaybackAtCursor );
#endif
	insertShortcut( Qt::Key_Comma, Action::BeatCounter );
	insertShortcut( Qt::Key_Backslash, Action::TapTempo );
	insertShortcut( Qt::Key_Plus, Action::BPMIncrease );
	insertShortcut( Qt::Key_Minus, Action::BPMDecrease );
	insertShortcut( Qt::Key_Backspace, Action::JumpToStart );
	insertShortcut( Qt::Key_F10, Action::JumpBarForward );
	insertShortcut( Qt::Key_F9, Action::JumpBarBackward );
	insertShortcut( Qt::Key_F6, Action::PlaylistNextSong );
	insertShortcut( Qt::Key_F5, Action::PlaylistPrevSong );

	// Virtual MIDI keyboard
	switch ( locale ) {
	case locales::de:
		insertShortcut( Qt::Key_Y, Action::VK_C2 );
		break;
	case locales::fr:
		insertShortcut( Qt::Key_W, Action::VK_C2 );
		break;
	default:
		insertShortcut( Qt::Key_Z, Action::VK_C2 );
	}
	insertShortcut( Qt::Key_S, Action::VK_C_sharp2 );
	insertShortcut( Qt::Key_X, Action::VK_D2 );
	insertShortcut( Qt::Key_D, Action::VK_D_sharp2 );
	insertShortcut( Qt::Key_C, Action::VK_E2 );
	insertShortcut( Qt::Key_V, Action::VK_F2 );
	insertShortcut( Qt::Key_G, Action::VK_F_sharp2 );
	insertShortcut( Qt::Key_B, Action::VK_G2 );
	insertShortcut( Qt::Key_H, Action::VK_G_sharp2 );
	insertShortcut( Qt::Key_N, Action::VK_A2 );
	insertShortcut( Qt::Key_J, Action::VK_A_sharp2 );
	switch ( locale ) {
	case locales::fr:
		insertShortcut( Qt::Key_Question, Action::VK_B2 );
		insertShortcut( Qt::Key_A, Action::VK_C3 );
		insertShortcut( Qt::Key_2, Action::VK_C_sharp3 );
		insertShortcut( Qt::Key_Z, Action::VK_D3 );
		break;
	default:
		insertShortcut( Qt::Key_M, Action::VK_B2 );
		insertShortcut( Qt::Key_Q, Action::VK_C3 );
		insertShortcut( Qt::Key_2, Action::VK_C_sharp3 );
		insertShortcut( Qt::Key_W, Action::VK_D3 );
	}
	insertShortcut( Qt::Key_3, Action::VK_D_sharp3 );
	insertShortcut( Qt::Key_E, Action::VK_E3 );
	insertShortcut( Qt::Key_R, Action::VK_F3 );
	insertShortcut( Qt::Key_5, Action::VK_F_sharp3 );
	insertShortcut( Qt::Key_T, Action::VK_G3 );
	insertShortcut( Qt::Key_6, Action::VK_G_sharp3 );
	switch ( locale ) {
	case locales::de:
		insertShortcut( Qt::Key_Z, Action::VK_A3 );
		break;
	default:
		insertShortcut( Qt::Key_Y, Action::VK_A3 );
	}
	insertShortcut( Qt::Key_7, Action::VK_A_sharp3 );
	insertShortcut( Qt::Key_U, Action::VK_B3 );
}

std::vector<Shortcuts::Action> Shortcuts::getActions( QKeySequence keySequence ) const {
	auto it = m_actionsMap.find( keySequence );
	if ( it != m_actionsMap.end() ) {
		// Key found
		return it->second;
	}
	else {
		std::vector<Action> v;
		return std::move( v );
	}
}

void Shortcuts::deleteShortcut( Action action ) {
	for ( const auto& it : m_actionsMap ) {
		std::vector<Shortcuts::Action> v = it.second;
		for ( std::vector<Shortcuts::Action>::iterator itAction = v.begin();
			  itAction != v.end(); ++itAction ) {
			if ( *itAction == action ) {
				if ( v.size() == 1 ) {
					// Just a single action assigned to this key, we
					// remove it from the map.
					m_actionsMap.erase( it.first );
				}
				else {
					v.erase( itAction );
				}
				break;
			}
		}
	}
}

Shortcuts::ActionInfo Shortcuts::getActionInfo( Action action ) const {
	auto it = m_actionInfoMap.find( action );
	if ( it != m_actionInfoMap.end() ) {
		// Action found
		return it->second;
	}
	else {
		return (ActionInfo){ Category::None, "" };
	}
}

QKeySequence Shortcuts::getKeySequence( Action action ) const {
	for ( const auto& it : m_actionsMap ) {
		for ( const auto& aaction : it.second ) {
			if ( aaction == action ) {
				return it.first;
			}
		}
	}
	return QKeySequence( "" );
}

void Shortcuts::createActionInfoMap() {
	m_actionInfoMap.clear();

	// Global shortcuts
	insertActionInfo( Shortcuts::Action::Panic, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Stop transport and all playing notes" ) );
	insertActionInfo( Shortcuts::Action::Save, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Save all modifications to the current song" ) );
	insertActionInfo( Shortcuts::Action::SaveAs, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Save all modifications to a new song" ) );
	insertActionInfo( Shortcuts::Action::Undo, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Undo the last modification" ) );
	insertActionInfo( Shortcuts::Action::Redo, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Redo the last modification" ) );
	insertActionInfo( Shortcuts::Action::TogglePlayback, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Playback toggling" ) );
	insertActionInfo( Shortcuts::Action::TogglePlaybackAtCursor, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Playback toggling at keyboard cursor" ) );
	insertActionInfo( Shortcuts::Action::BeatCounter, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts", "BeatCounter trigger" ) );
	insertActionInfo( Shortcuts::Action::TapTempo, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Tap Tempo trigger" ) );
	insertActionInfo( Shortcuts::Action::BPMIncrease, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts", "BPM increase" ) );
	insertActionInfo( Shortcuts::Action::BPMDecrease, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts", "BPM decrease" ) );
	insertActionInfo( Shortcuts::Action::JumpToStart, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Move playhead to the beginnning of the song" ) );
	insertActionInfo( Shortcuts::Action::JumpBarForward, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Move playhead one bar forward" ) );
	insertActionInfo( Shortcuts::Action::JumpBarBackward, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Move playhead one bar backward" ) );
	insertActionInfo( Shortcuts::Action::PlaylistNextSong, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Playlist: select next song" ) );
	insertActionInfo( Shortcuts::Action::PlaylistPrevSong, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Playlist: select previous song" ) );

	// Virtual MIDI keyboard
	insertActionInfo( Shortcuts::Action::VK_C2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "C2" ) );
	insertActionInfo( Shortcuts::Action::VK_C_sharp2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "C#2" ) );
	insertActionInfo( Shortcuts::Action::VK_D2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "D2" ) );
	insertActionInfo( Shortcuts::Action::VK_D_sharp2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "D#2" ) );
	insertActionInfo( Shortcuts::Action::VK_E2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "E2" ) );
	insertActionInfo( Shortcuts::Action::VK_F2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "F2" ) );
	insertActionInfo( Shortcuts::Action::VK_F_sharp2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "F#2" ) );
	insertActionInfo( Shortcuts::Action::VK_G2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "G2" ) );
	insertActionInfo( Shortcuts::Action::VK_G_sharp2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "G#2" ) );
	insertActionInfo( Shortcuts::Action::VK_A2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "A2" ) );
	insertActionInfo( Shortcuts::Action::VK_A_sharp2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "A#2" ) );
	insertActionInfo( Shortcuts::Action::VK_B2, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "B2" ) );
	insertActionInfo( Shortcuts::Action::VK_C3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "C3" ) );
	insertActionInfo( Shortcuts::Action::VK_C_sharp3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "C#3" ) );
	insertActionInfo( Shortcuts::Action::VK_D3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "D3" ) );
	insertActionInfo( Shortcuts::Action::VK_D_sharp3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "D#3" ) );
	insertActionInfo( Shortcuts::Action::VK_E3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "E3" ) );
	insertActionInfo( Shortcuts::Action::VK_F3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "F3" ) );
	insertActionInfo( Shortcuts::Action::VK_F_sharp3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "F#3" ) );
	insertActionInfo( Shortcuts::Action::VK_G3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "G3" ) );
	insertActionInfo( Shortcuts::Action::VK_G_sharp3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "G#3" ) );
	insertActionInfo( Shortcuts::Action::VK_A3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "A3" ) );
	insertActionInfo( Shortcuts::Action::VK_A_sharp3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "A#3" ) );
	insertActionInfo( Shortcuts::Action::VK_B3, Category::VirtualKeyboard,
					  QT_TRANSLATE_NOOP( "Shortcuts", "B3" ) );
}

QString Shortcuts::categoryToQString( Category category ) {
	QString s;

	switch ( category ) {
	case Category::HardCoded:
		s = QT_TRANSLATE_NOOP( "Shortcuts", "Hard Coded" );
		break;
	case Category::None:
		s = "";
		break;
	case Category::Global:
		s = QT_TRANSLATE_NOOP( "Shortcuts", "Global" );
		break;
	case Category::MainWindow:
		s = QT_TRANSLATE_NOOP( "Shortcuts", "Main Window" );
		break;
	case Category::Editors:
		s = QT_TRANSLATE_NOOP( "Shortcuts", "Editors" );
		break;
	case Category::VirtualKeyboard:
		s = QT_TRANSLATE_NOOP( "Shortcuts", "Virtual Keyboard" );
		break;
	case Category::Mixer:
		s = QT_TRANSLATE_NOOP( "Shortcuts", "Mixer" );
		break;
	case Category::PlaylistEditor:
		s = QT_TRANSLATE_NOOP( "Shortcuts", "PlaylistEditor" );
		break;
	case Category::SampleEditor:
		s = QT_TRANSLATE_NOOP( "Shortcuts", "Sampler Editor" );
		break;
	case Category::Director:
		s = QT_TRANSLATE_NOOP( "Shortcuts", "Director" );
		break;
	case Category::All:
		s = QT_TRANSLATE_NOOP( "Shortcuts", "All Categories" );
		break;
	};

	return std::move( s );
};

void Shortcuts::insertShortcut( QKeySequence keySequence, Action action ) {
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

void Shortcuts::insertActionInfo( Action action, Category category,
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
			sOutput.append( "]" );
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
