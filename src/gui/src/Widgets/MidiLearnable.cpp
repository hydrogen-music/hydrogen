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
#include "../HydrogenApp.h"

#include <core/MidiMap.h>

MidiLearnable::MidiLearnable() : m_pAction( nullptr ) {
	HydrogenApp::get_instance()->addEventListener( this );
}

MidiLearnable::~MidiLearnable() {
	auto pHydrogenApp = HydrogenApp::get_instance();
	if ( pHydrogenApp != nullptr ) {
		pHydrogenApp->removeEventListener( this );
	}
}

void MidiLearnable::setAction( std::shared_ptr<Action> pAction ){
	m_pAction = pAction;

	midiMapChangedEvent();
}

void MidiLearnable::midiMapChangedEvent() {
	if ( m_pAction != nullptr ) {
		m_registeredMidiEvents =
			MidiMap::get_instance()->getRegisteredMidiEvents( m_pAction );
		updateTooltip();
	}
}
