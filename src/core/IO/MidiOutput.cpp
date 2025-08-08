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

std::shared_ptr<MidiOutput::HandledOutput> MidiOutput::sendMessage(
	const MidiMessage& msg )
{
	auto pHandledOutput = std::make_shared<HandledOutput>();

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
		pHandledOutput->type = MidiMessage::Type::Unknown;
		return pHandledOutput;
	}

	pHandledOutput->timestamp = QTime::currentTime();
	pHandledOutput->type = msg.getType();
	pHandledOutput->nData1 = msg.getData1();
	pHandledOutput->nData2 = msg.getData2();
	pHandledOutput->nChannel = msg.getChannel();

	return pHandledOutput;
}

QString MidiOutput::HandledOutput::toQString() const {
	return QString( "timestamp: %1, msg type: %2, nData1: %3, nData2: %4, nChannel: %5" )
		.arg( timestamp.toString( "HH:mm:ss.zzz" ) )
		.arg( MidiMessage::TypeToQString( type ) ).arg( nData1 ).arg( nData2 )
		.arg( nChannel );
}
};
