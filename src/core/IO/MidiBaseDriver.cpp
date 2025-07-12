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

#include <core/IO/MidiBaseDriver.h>

#include <core/EventQueue.h>

namespace H2Core {

MidiBaseDriver::MidiBaseDriver() : MidiInput(), MidiOutput() {
}
MidiBaseDriver::~MidiBaseDriver() {
}

QString MidiBaseDriver::portTypeToQString( const PortType& portType ) {
	switch ( portType ) {
	case PortType::Input:
		return "Input";
	case PortType::Output:
		return "Output";
	default:
		return "Unhandled port type";
	}
}

MidiInput::HandledInput MidiBaseDriver::handleMessage( const MidiMessage &msg ) {
	const auto handledInput = MidiInput::handleMessage( msg );

	if ( handledInput.type != MidiMessage::Type::Unknown ) {
		m_handledInputs.push_back( handledInput );
		if ( m_handledInputs.size() > MidiBaseDriver::nBacklogSize ) {
			m_handledInputs.pop_front();
		}

		EventQueue::get_instance()->pushEvent( Event::Type::MidiInput, 0 );
	}

	return handledInput;
}

MidiOutput::HandledOutput MidiBaseDriver::sendMessage( const MidiMessage &msg ) {
	const auto handledOutput = MidiOutput::sendMessage( msg );

	if ( handledOutput.type != MidiMessage::Type::Unknown ) {
		m_handledOutputs.push_back( handledOutput );
		if ( m_handledOutputs.size() > MidiBaseDriver::nBacklogSize ) {
			m_handledOutputs.pop_front();
		}

		EventQueue::get_instance()->pushEvent( Event::Type::MidiOutput, 0 );
	}

	return handledOutput;
}
};
