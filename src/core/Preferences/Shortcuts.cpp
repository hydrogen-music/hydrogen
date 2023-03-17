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

namespace H2Core {
Shortcuts::Shortcuts() {
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
	
	insertShortcut( Qt::Key_F12, Action::Panic );
	insertShortcut( Qt::Key_S, Action::Save );
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

Shortcuts::ActionInfo Shortcuts::getActionInfo( Action action ) const {
	auto it = m_actionInfoMap.find( action );
	if ( it != m_actionInfoMap.end() ) {
		// Action found
		return it->second;
	}
	else {
		return (ActionInfo){ Category::HardCoded, "Null" };
	}
}

void Shortcuts::createActionInfoMap() {

	insertActionInfo( Shortcuts::Action::Panic, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Stop transport and all playing notes" ) );
	insertActionInfo( Shortcuts::Action::Save, Category::Global,
					  QT_TRANSLATE_NOOP( "Shortcuts", "Store all changes to the current song" ) );
}

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
			.append( QString( " m_actionInfoMap: [" ).arg( sPrefix ).arg( s ) );
		for ( const auto& it : m_actionInfoMap ) {
			sOutput.append( QString( " , Action: %1 : [category: %2, sDescription: %3]" )
							.arg( static_cast<int>(it.first) )
							.arg( static_cast<int>(it.second.category) )
							.arg( it.second.sDescription) );
		}
		sOutput.append( QString( "], m_actionsMap: [" ).arg( sPrefix ).arg( s ) );
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
