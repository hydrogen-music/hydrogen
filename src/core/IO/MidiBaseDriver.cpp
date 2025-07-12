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

bool MidiBaseDriver::sendMessage( const MidiMessage &msg ) {
	if ( ! MidiOutput::sendMessage( msg ) ) {
		return false;
	}

	HandledOutput handledOutput;
	handledOutput.timestamp = QTime::currentTime();
	handledOutput.type = msg.getType();
	handledOutput.nData1 = msg.getData1();
	handledOutput.nData2 = msg.getData2();
	handledOutput.nChannel = msg.getChannel();

	m_handledOutputs.push_back( handledOutput );
	if ( m_handledOutputs.size() > MidiBaseDriver::nBacklogSize ) {
		m_handledOutputs.pop_front();
	}

	EventQueue::get_instance()->pushEvent( Event::Type::MidiOutput, 0 );

	return true;
}
};
