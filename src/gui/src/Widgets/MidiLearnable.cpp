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

#include "MidiLearnable.h"

#include "../CommonStrings.h"
#include "../HydrogenApp.h"

#include <core/Midi/MidiAction.h>
#include <core/Midi/MidiEventMap.h>
#include <core/Preferences/Preferences.h>

MidiLearnable::MidiLearnable() : m_pMidiAction( nullptr ) {
	HydrogenApp::get_instance()->addEventListener( this );
}

MidiLearnable::~MidiLearnable() {
	auto pHydrogenApp = HydrogenApp::get_instance();
	if ( pHydrogenApp != nullptr ) {
		pHydrogenApp->removeEventListener( this );
	}
}

void MidiLearnable::setMidiAction( std::shared_ptr<MidiAction> pMidiAction ){
	if ( pMidiAction != m_pMidiAction ) {
		m_pMidiAction = pMidiAction;

		midiMapChangedEvent();
	}
}

void MidiLearnable::setBaseToolTip( const QString& sNewTip ) {
	if ( sNewTip != m_sBaseToolTip ) {
		m_sBaseToolTip = sNewTip;
		updateToolTip();
	}
}

void MidiLearnable::midiMapChangedEvent() {
	if ( m_pMidiAction != nullptr ) {
		m_registeredMidiEvents = H2Core::Preferences::get_instance()->
			getMidiEventMap()->getRegisteredMidiEvents( m_pMidiAction );
		updateToolTip();
	}
}

QString MidiLearnable::composeToolTip() const {

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	QString sTip( m_sBaseToolTip );

	// Add the associated MIDI Midiaction.
	if ( m_pMidiAction != nullptr ) {
		sTip.append( QString( "\n\n%1: %2 " )
					 .arg( pCommonStrings->getMidiToolTipHeading() )
					 .arg( MidiAction::typeToQString( m_pMidiAction->getType() ) ) );
		if ( m_registeredMidiEvents.size() > 0 ) {
			for ( const auto& [event, nnParam] : m_registeredMidiEvents ) {
				if ( event == H2Core::MidiMessage::Event::Note ||
					 event == H2Core::MidiMessage::Event::CC ) {
					sTip.append( QString( "\n%1 [%2 : %3]" )
								 .arg( pCommonStrings->getMidiToolTipBound() )
								 .arg( H2Core::MidiMessage::EventToQString( event ) )
								 .arg( nnParam ) );
				}
				else {
					// PC and MMC_x do not have a parameter.
					sTip.append( QString( "\n%1 [%2]" )
								 .arg( pCommonStrings->getMidiToolTipBound() )
								 .arg( H2Core::MidiMessage::EventToQString( event ) ) );
				}
			}
		}
		else {
			sTip.append( QString( "%1" ).arg( pCommonStrings->getMidiToolTipUnbound() ) );
		}
	}

	return sTip;
}
