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

#include <core/IO/MidiOutput.h>

#include <core/Midi/MidiMessage.h>

namespace H2Core
{

MidiOutput::MidiOutput() {
}

MidiOutput::~MidiOutput() {
}

MidiOutput::HandledOutput MidiOutput::sendMessage( const MidiMessage& msg ) {
	HandledOutput handledOutput;

	switch( msg.getType() ) {
	case MidiMessage::Type::ControlChange:
		sendControlChangeMessage( msg );
		break;

	case MidiMessage::Type::NoteOn:
		sendNoteOffMessage( msg );
		sendNoteOnMessage( msg );
		break;

	case MidiMessage::Type::NoteOff:
		sendNoteOffMessage( msg );
		break;

	default:
		// Not handled, we won't send the corresponding event.
		handledOutput.type = MidiMessage::Type::Unknown;
		return handledOutput;
	}

	handledOutput.timestamp = QTime::currentTime();
	handledOutput.type = msg.getType();
	handledOutput.nData1 = msg.getData1();
	handledOutput.nData2 = msg.getData2();
	handledOutput.nChannel = msg.getChannel();

	return handledOutput;
}

QString MidiOutput::HandledOutput::toQString() const {
	return QString( "timestamp: %1, msg type: %2, nData1: %3, nData2: %4, nChannel: %5" )
		.arg( timestamp.toString( "HH:mm:ss.zzz" ) )
		.arg( MidiMessage::TypeToQString( type ) ).arg( nData1 ).arg( nData2 )
		.arg( nChannel );
}
};
