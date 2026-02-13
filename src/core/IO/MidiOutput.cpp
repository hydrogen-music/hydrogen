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

#include <core/IO/MidiBaseDriver.h>
#include <core/Midi/MidiMessage.h>
#include <core/Preferences/Preferences.h>

#define LOG_OUTGOING_MESSAGES 0

namespace H2Core
{

MidiOutput::MidiOutput() {
}

MidiOutput::~MidiOutput() {
}

std::shared_ptr<MidiOutput::HandledOutput> MidiOutput::sendMessage(
	const MidiMessage& msg )
{
#if LOG_OUTGOING_MESSAGES
	INFOLOG( QString( "[OUTPUT] sending [%1]" ).arg( msg.toQString() ) );
#endif

	auto pHandledOutput = std::make_shared<HandledOutput>();

	switch( msg.getType() ) {
	case MidiMessage::Type::ControlChange:
		sendControlChangeMessage( msg );
		break;

	case MidiMessage::Type::NoteOn:
		sendNoteOnMessage( msg );
		break;

	case MidiMessage::Type::NoteOff:
		if ( Preferences::get_instance()->getMidiSendNoteOff() !=
			 Preferences::MidiSendNoteOff::Never ) {
			sendNoteOffMessage( msg );
		}
		break;

	case MidiMessage::Type::Start:
	case MidiMessage::Type::Continue:
	case MidiMessage::Type::Stop:
	case MidiMessage::Type::TimingClock:
		sendSystemRealTimeMessage( msg );
		break;

	default:
		// Not handled, we won't send the corresponding event.
		pHandledOutput->type = MidiMessage::Type::Unknown;
#if LOG_OUTGOING_MESSAGES
		WARNINGLOG( "Unknown message type. Dropped" );
#endif
		return pHandledOutput;
	}

	pHandledOutput->timePoint = Clock::now();
	pHandledOutput->type = msg.getType();
	pHandledOutput->data1 = msg.getData1();
	pHandledOutput->data2 = msg.getData2();
	pHandledOutput->channel = msg.getChannel();

	return pHandledOutput;
}

QString MidiOutput::HandledOutput::toQString() const
{
	return QString(
			   "timePoint: %1, msg type: %2, data1: %3, data2: %4, channel: "
			   "%5"
	)
		.arg( H2Core::timePointToQString( timePoint ) )
		.arg( MidiMessage::TypeToQString( type ) )
		.arg( static_cast<int>( data1 ) )
		.arg( static_cast<int>( data2 ) )
		.arg( static_cast<int>( channel ) );
}
};	// namespace H2Core
