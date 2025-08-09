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

#include <QMutexLocker>

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

std::shared_ptr<MidiInput::HandledInput> MidiBaseDriver::handleMessage(
	const MidiMessage &msg )
{
	const auto pHandledInput = MidiInput::handleMessage( msg );

	if ( pHandledInput != nullptr &&
		 pHandledInput->type != MidiMessage::Type::Unknown ) {
		QMutexLocker mx( &m_inputMutex );
		m_handledInputs.push_back( pHandledInput );
		if ( m_handledInputs.size() > MidiBaseDriver::nBacklogSize ) {
			m_handledInputs.pop_front();
		}

		EventQueue::get_instance()->pushEvent( Event::Type::MidiInput, 0 );
	}

	return pHandledInput;
}

std::shared_ptr<MidiOutput::HandledOutput> MidiBaseDriver::sendMessage(
	const MidiMessage &msg )
{
	const auto pHandledOutput = MidiOutput::sendMessage( msg );

	if ( pHandledOutput != nullptr &&
		 pHandledOutput->type != MidiMessage::Type::Unknown ) {
		QMutexLocker mx( &m_outputMutex );
		m_handledOutputs.push_back( pHandledOutput );
		if ( m_handledOutputs.size() > MidiBaseDriver::nBacklogSize ) {
			m_handledOutputs.pop_front();
		}

		EventQueue::get_instance()->pushEvent( Event::Type::MidiOutput, 0 );
	}

	return pHandledOutput;
}

std::vector< std::shared_ptr<MidiInput::HandledInput> > MidiBaseDriver::getHandledInputs() {
	std::vector< std::shared_ptr<MidiInput::HandledInput> > inputs;

	QMutexLocker mx( &m_inputMutex );
	inputs.reserve( m_handledInputs.size() );

	for ( const auto& ppHandledInput : m_handledInputs ) {
		inputs.push_back( ppHandledInput );
	}

	return std::move( inputs );
}

std::vector< std::shared_ptr<MidiOutput::HandledOutput> > MidiBaseDriver::getHandledOutputs() {
	std::vector< std::shared_ptr<MidiOutput::HandledOutput> > outputs;

	QMutexLocker mx( &m_outputMutex );
	outputs.reserve( m_handledOutputs.size() );

	for ( const auto& ppHandledOutput : m_handledOutputs ) {
		outputs.push_back( ppHandledOutput );
	}

	return std::move( outputs );
}

void MidiBaseDriver::startMidiClockStream( float fBpm ) {
	// 24 MIDI Clock messages should make up a quarter.
	const float nInterval = 60000 / fBpm / 24.0;

	DEBUGLOG( "not implemented yet" );
}

void MidiBaseDriver::stopMidiClockStream() {
	DEBUGLOG( "not implemented yet" );
}
};
