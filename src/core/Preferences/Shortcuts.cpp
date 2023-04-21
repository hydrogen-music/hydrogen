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
#include <QKeySequence>

#include <core/Preferences/Shortcuts.h>
#include <core/Helpers/Xml.h>

namespace H2Core {
Shortcuts::Shortcuts() {
}

Shortcuts::Shortcuts( const std::shared_ptr<Shortcuts> pOther ) {
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
			shortcutNode.write_int( "key", it.first );
			shortcutNode.write_int( "action", static_cast<int>(aaction) );
		}
	}
}

std::shared_ptr<Shortcuts> Shortcuts::loadFrom( XMLNode* pNode, bool bSilent ) {
	auto pShortcuts = std::make_shared<Shortcuts>();
	pShortcuts->createActionInfoMap();
	
	XMLNode shortcutsNode = pNode->firstChildElement( "shortcuts" );
	if ( shortcutsNode.isNull() ) {
		WARNINGLOG( "shortcut node not found" );
		pShortcuts->createDefaultShortcuts();
	}
	else {
		XMLNode shortcutNode = shortcutsNode.firstChildElement( "shortcut" );
		while ( ! shortcutNode.isNull() ) {
			pShortcuts->insertShortcut( 
				shortcutNode.read_int( "key", 0, false, false, bSilent ),
				static_cast<Action>(shortcutNode.read_int( "action", 0,
														   false, false, bSilent )) );
			shortcutNode = shortcutNode.nextSiblingElement( "shortcut" );
		}
	}

	return pShortcuts;
}

void Shortcuts::createDefaultShortcuts() {
	m_actionsMap.clear();
	
	// Global shortcuts
	insertShortcut( Qt::Key_F12, Action::Panic );
	insertShortcut( Qt::Key_S, Action::Save );
	insertShortcut( Qt::Key_S+Qt::ControlModifier, Action::SaveAs );
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
}

std::vector<Shortcuts::Action> Shortcuts::getActions( int nKey ) const {
	auto it = m_actionsMap.find( nKey );
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

int Shortcuts::getKey( Action action ) const {
	for ( const auto& it : m_actionsMap ) {
		for ( const auto& aaction : it.second ) {
			if ( aaction == action ) {
				return it.first;
			}
		}
	}
	return -1;
}

void Shortcuts::createActionInfoMap() {
	m_actionInfoMap.clear();
	insertActionInfo( Shortcuts::Action::Panic, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Stop transport and all playing notes" ) );
	insertActionInfo( Shortcuts::Action::Save, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Store all modifications to the current song" ) );
	insertActionInfo( Shortcuts::Action::SaveAs, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Store all modifications to a new song" ) );
	insertActionInfo( Shortcuts::Action::Undo, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Undo the last modification" ) );
	insertActionInfo( Shortcuts::Action::Redo, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Redo the last modification" ) );
	insertActionInfo( Shortcuts::Action::TogglePlayback, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Toggle playback" ) );
	insertActionInfo( Shortcuts::Action::TogglePlaybackAtCursor, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Toggle playback at keyboard cursor" ) );
	insertActionInfo( Shortcuts::Action::BeatCounter, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Trigger BeatCounter" ) );
	insertActionInfo( Shortcuts::Action::TapTempo, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Trigger Tap Tempo" ) );
	insertActionInfo( Shortcuts::Action::BPMIncrease, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Increase tempo" ) );
	insertActionInfo( Shortcuts::Action::BPMDecrease, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Decrease tempo" ) );
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
										 "Select next song in playlist" ) );
	insertActionInfo( Shortcuts::Action::PlaylistPrevSong, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts",
										 "Select previous song in playlist" ) );
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

void Shortcuts::insertShortcut( int nKey, Action action ) {
	auto it = m_actionsMap.find( nKey );
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
		m_actionsMap[ nKey ] = v;
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
			sOutput.append( QString( "%1%2%2Key: %3 : [" )
							.arg( sPrefix ).arg( s ).arg( it.first ) );
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
			sOutput.append( QString( ", Key: %1 : [" ).arg( it.first ) );
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
