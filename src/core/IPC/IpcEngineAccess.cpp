/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/IPC/IpcEngineAccess.h>

#include <core/IO/DiskWriterDriver.h>
#include <core/IO/MidiBaseDriver.h>
#include <core/IPC/IpcChannel.h>
#include <core/IPC/IpcMessage.h>
#include <core/Midi/MidiMessage.h>

namespace H2Core {

namespace {

// Issues a blocking request against the authoritative engine. Returns
// false on timeout; callers fall back to an empty result (the GUI treats
// "no answer" like "no data" rather than showing stale mirror state).
bool ipcRequest( IpcChannel* pChannel, const IpcMessage& request,
				 IpcMessage& reply )
{
	return pChannel != nullptr &&
		pChannel->request( request, reply, 3000 );
}

} // namespace

void IpcEngineAccess::sequencerPlay() {
	// Transport start is engine-authoritative: forward it to the real engine and
	// reflect it locally so the GUI's transport widgets update immediately.
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::Play ) );
	}
	m_pMirror->sequencerPlay();
}

void IpcEngineAccess::sequencerStop() {
	// Transport stop is engine-authoritative: forward it to the real engine and
	// reflect it locally so the GUI's transport widgets update immediately.
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::Stop ) );
	}
	m_pMirror->sequencerStop();
}

void IpcEngineAccess::setSelectedInstrumentNumber(
	int nInstrument, Event::Trigger trigger )
{
	// Instrument selection is engine-relevant: the headless engine's
	// MIDI-to-selected-instrument routing follows it. Forward the change and
	// apply it locally so the GUI updates immediately; the engine's echo
	// event re-applies the same value on the mirror (idempotent).
	if ( m_pChannel != nullptr ) {
		m_pChannel->send(
			IpcMessage( IpcOpcode::SetSelectedInstrument ).arg( nInstrument ) );
	}
	m_pMirror->setSelectedInstrumentNumber( nInstrument, trigger );
}

QStringList IpcEngineAccess::getAudioDevices(
		Preferences::AudioDriver kind, const QString& sHostAPI
	) const {
	IpcMessage reply;
	if ( ! ipcRequest( m_pChannel,
					   IpcMessage( IpcOpcode::GetAudioDevices )
						   .arg( static_cast<int>( kind ) )
						   .arg( sHostAPI ),
					   reply ) ) {
		WARNINGLOG( QString( "Engine did not answer the audio device query "
							 "for driver [%1], host API [%2]" )
						.arg( Preferences::audioDriverToQString( kind ) )
						.arg( sHostAPI ) );
		return QStringList();
	}
	const auto& args = reply.getArgs();
	if ( args.isEmpty() ) {
		return QStringList();
	}
	return args[0].toStringList();
}

QStringList IpcEngineAccess::getAudioHostAPIs() const {
	IpcMessage reply;
	if ( ! ipcRequest( m_pChannel, IpcMessage( IpcOpcode::GetAudioHostAPIs ),
					   reply ) ) {
		WARNINGLOG( "Engine did not answer the host API query" );
		return QStringList();
	}
	const auto& args = reply.getArgs();
	if ( args.isEmpty() ) {
		return QStringList();
	}
	return args[0].toStringList();
}

int IpcEngineAccess::getOscTemporaryPort() const {
	IpcMessage reply;
	if ( ! ipcRequest( m_pChannel,
					   IpcMessage( IpcOpcode::GetOscTemporaryPort ),
					   reply ) ) {
		WARNINGLOG( "Engine did not answer the OSC temporary port query" );
		return -1;
	}
	const auto& args = reply.getArgs();
	if ( args.isEmpty() ) {
		return -1;
	}
	return args[0].toInt();
}

std::vector<QString> IpcEngineAccess::getMidiPorts(
		MidiBaseDriver::PortType portType ) const {
	std::vector<QString> ports;
	IpcMessage reply;
	if ( ! ipcRequest( m_pChannel,
					   IpcMessage( IpcOpcode::GetMidiPorts )
						   .arg( static_cast<int>( portType ) ),
					   reply ) ) {
		WARNINGLOG( QString( "Engine did not answer the MIDI port query for "
							 "port type [%1]" )
						.arg( MidiBaseDriver::portTypeToQString( portType ) ) );
		return ports;
	}
	const auto& args = reply.getArgs();
	if ( ! args.isEmpty() ) {
		for ( const auto& ssPort : args[0].toStringList() ) {
			ports.push_back( ssPort );
		}
	}
	return ports;
}

std::vector<std::shared_ptr<MidiInput::HandledInput>>
IpcEngineAccess::getHandledMidiInputs() const {
	std::vector<std::shared_ptr<MidiInput::HandledInput>> inputs;
	IpcMessage reply;
	if ( ! ipcRequest( m_pChannel,
					   IpcMessage( IpcOpcode::GetHandledMidiInputs ),
					   reply ) ) {
		WARNINGLOG( "Engine did not answer the handled MIDI input query" );
		return inputs;
	}
	const auto& args = reply.getArgs();
	if ( args.isEmpty() ) {
		return inputs;
	}
	bool bOk = false;
	const int nCount = args[0].toInt( &bOk );
	// 1 count arg + 7 args per entry.
	if ( ! bOk || nCount < 0 || args.size() < 1 + nCount * 7 ) {
		WARNINGLOG( QString( "Malformed handled MIDI input reply: count "
							 "[%1], args [%2]" )
						.arg( args[0].toString() )
						.arg( args.size() ) );
		return inputs;
	}
	for ( int ii = 0; ii < nCount; ++ii ) {
		const int nBase = 1 + ii * 7;
		auto pHandled = std::make_shared<MidiInput::HandledInput>();
		// Engine and editor share the host clock, so the epoch count
		// reconstructs the original TimePoint losslessly.
		pHandled->timePoint = TimePoint(
			TimePoint::duration( args[ nBase ].toLongLong() ) );
		pHandled->type = static_cast<MidiMessage::Type>(
			args[ nBase + 1 ].toInt() );
		pHandled->data1 = static_cast<Midi::Parameter>(
			args[ nBase + 2 ].toInt() );
		pHandled->data2 = static_cast<Midi::Parameter>(
			args[ nBase + 3 ].toInt() );
		pHandled->channel = static_cast<Midi::Channel>(
			args[ nBase + 4 ].toInt() );
		for ( const auto& actionType : args[ nBase + 5 ].toList() ) {
			pHandled->actionTypes.push_back(
				static_cast<MidiAction::Type>( actionType.toInt() ) );
		}
		pHandled->mappedInstruments = args[ nBase + 6 ].toStringList();
		inputs.push_back( std::move( pHandled ) );
	}
	return inputs;
}

std::vector<std::shared_ptr<MidiOutput::HandledOutput>>
IpcEngineAccess::getHandledMidiOutputs() const {
	std::vector<std::shared_ptr<MidiOutput::HandledOutput>> outputs;
	IpcMessage reply;
	if ( ! ipcRequest( m_pChannel,
					   IpcMessage( IpcOpcode::GetHandledMidiOutputs ),
					   reply ) ) {
		WARNINGLOG( "Engine did not answer the handled MIDI output query" );
		return outputs;
	}
	const auto& args = reply.getArgs();
	if ( args.isEmpty() ) {
		return outputs;
	}
	bool bOk = false;
	const int nCount = args[0].toInt( &bOk );
	// 1 count arg + 5 args per entry.
	if ( ! bOk || nCount < 0 || args.size() < 1 + nCount * 5 ) {
		WARNINGLOG( QString( "Malformed handled MIDI output reply: count "
							 "[%1], args [%2]" )
						.arg( args[0].toString() )
						.arg( args.size() ) );
		return outputs;
	}
	for ( int ii = 0; ii < nCount; ++ii ) {
		const int nBase = 1 + ii * 5;
		auto pHandled = std::make_shared<MidiOutput::HandledOutput>();
		pHandled->timePoint = TimePoint(
			TimePoint::duration( args[ nBase ].toLongLong() ) );
		pHandled->type = static_cast<MidiMessage::Type>(
			args[ nBase + 1 ].toInt() );
		pHandled->data1 = static_cast<Midi::Parameter>(
			args[ nBase + 2 ].toInt() );
		pHandled->data2 = static_cast<Midi::Parameter>(
			args[ nBase + 3 ].toInt() );
		pHandled->channel = static_cast<Midi::Channel>(
			args[ nBase + 4 ].toInt() );
		outputs.push_back( std::move( pHandled ) );
	}
	return outputs;
}

bool IpcEngineAccess::isExportWritingFailed() const
{
	const auto pDriver =
		std::dynamic_pointer_cast<DiskWriterDriver>( m_pMirror->getAudioDriver()
		);
	return pDriver != nullptr && pDriver->writingFailed();
}
};
