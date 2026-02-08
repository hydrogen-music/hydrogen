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

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/EventQueue.h>
#include <core/Helpers/TimeHelper.h>
#include <core/Hydrogen.h>
#include <core/Midi/Midi.h>
#include <core/Preferences/Preferences.h>

#include <QMutexLocker>

namespace H2Core {

MidiBaseDriver::MidiBaseDriver()
	: MidiInput(),
	  MidiOutput(),
	  m_bSendClockTick( false ),
	  m_bNotifyOnNextTick( false ),
	  m_interval( 20.0 ),
	  m_intervalCompensation( std::chrono::microseconds::zero() ),
	  m_nAverageIntervalNs( 0 ),
	  m_lastTick( TimePoint() ),
	  m_nTickCount( 0 ),
	  m_pClockThread( nullptr ),
      m_bInputActive( false ),
      m_bOutputActive( false )
{
	if ( Preferences::get_instance()->getMidiClockOutputSend() ) {
		startMidiClockStream( Hydrogen::get_instance()
								  ->getAudioEngine()
								  ->getTransportPosition()
								  ->getBpm() );
	}

	{
		std::unique_lock lock{ m_inputMessageHandlerMutex };
		m_pInputMessageHandler = std::make_shared<std::thread>(
			MidiBaseDriver::inputMessageHandler, (void*) this
		);
		// Wait till the thread as created successfully.
		m_inputMessageHandlerCV.wait( lock, [&] { return m_bInputActive; } );
	}
	{
		std::unique_lock lock{ m_outputMessageHandlerMutex };
		m_pOutputMessageHandler = std::make_shared<std::thread>(
			MidiBaseDriver::outputMessageHandler, (void*) this
		);
		// Wait till the thread as created successfully.
		m_outputMessageHandlerCV.wait( lock, [&] { return m_bOutputActive; } );
	}
}

MidiBaseDriver::~MidiBaseDriver()
{
	if ( m_pClockThread != nullptr ) {
		stopMidiClockStream();
	}

	if ( m_pInputMessageHandler != nullptr ) {
		m_bInputActive = false;
		{
			std::scoped_lock lock{ m_inputMessageHandlerMutex };
			m_inputMessageHandlerCV.notify_all();
		}
		m_pInputMessageHandler->join();
		m_pInputMessageHandler = nullptr;
	}

	if ( m_pOutputMessageHandler != nullptr ) {
		m_bOutputActive = false;
		{
			std::scoped_lock lock{ m_outputMessageHandlerMutex };
			m_outputMessageHandlerCV.notify_all();
		}
		m_pOutputMessageHandler->join();
		m_pOutputMessageHandler = nullptr;
	}
}

QString MidiBaseDriver::portTypeToQString( const PortType& portType )
{
	switch ( portType ) {
		case PortType::Input:
			return "Input";
		case PortType::Output:
			return "Output";
		default:
			return "Unhandled port type";
	}
}

void MidiBaseDriver::enqueueInputMessage( const MidiMessage& msg )
{
	if ( m_pInputMessageHandler == nullptr || !m_bInputActive ) {
		ERRORLOG(
			QString(
				"Midi driver not functional. Can't handle incoming message [%1]"
			)
				.arg( msg.toQString() )
		);
		return;
	}

	std::scoped_lock lock{ m_inputMessageHandlerMutex };

	m_inputMessageQueue.push( msg );
	m_inputMessageHandlerCV.notify_all();
}

void MidiBaseDriver::enqueueOutputMessage( const MidiMessage& msg )
{
	if ( m_pOutputMessageHandler == nullptr || !m_bOutputActive ) {
		ERRORLOG(
			QString(
				"Midi driver not functional. Can't handle incoming message [%1]"
			)
				.arg( msg.toQString() )
		);
		return;
	}

	std::scoped_lock lock{ m_outputMessageHandlerMutex };

	m_outputMessageQueue.push( msg );
	m_outputMessageHandlerCV.notify_all();
}

std::vector<std::shared_ptr<MidiInput::HandledInput> >
MidiBaseDriver::getHandledInputs()
{
	std::vector<std::shared_ptr<MidiInput::HandledInput> > inputs;

	QMutexLocker mx( &m_handledInputsMutex );
	inputs.reserve( m_handledInputs.size() );

	for ( const auto& ppHandledInput : m_handledInputs ) {
		inputs.push_back( ppHandledInput );
	}

	return std::move( inputs );
}

std::vector<std::shared_ptr<MidiOutput::HandledOutput> >
MidiBaseDriver::getHandledOutputs()
{
	std::vector<std::shared_ptr<MidiOutput::HandledOutput> > outputs;

	QMutexLocker mx( &m_handledOutputMutex );
	outputs.reserve( m_handledOutputs.size() );

	for ( const auto& ppHandledOutput : m_handledOutputs ) {
		outputs.push_back( ppHandledOutput );
	}

	return std::move( outputs );
}

void MidiBaseDriver::sendAllNotesOff()
{
	const auto pPref = Preferences::get_instance();
	if ( pPref->getMidiSendNoteOff() == Preferences::MidiSendNoteOff::Never ) {
		return;
	}

	const auto threshold =
		Clock::now() -
		std::chrono::seconds( MidiBaseDriver::nAllNotesOffThresholdInSeconds );

	std::set<std::pair<Midi::Note, Midi::Channel> > noteOnMessages;

	// Note-Offs for all recent Note-On messages
	{
		QMutexLocker mx( &m_handledOutputMutex );
		for ( const auto hhandledOutput : m_handledOutputs ) {
			if ( hhandledOutput == nullptr ||
				 hhandledOutput->timePoint < threshold ) {
				continue;
			}

			if ( hhandledOutput->type == MidiMessage::Type::NoteOn ) {
				// Enqueue
				noteOnMessages.insert( std::make_pair(
					Midi::noteFromIntClamp(
						static_cast<int>( hhandledOutput->data1 )
					),
					hhandledOutput->channel
				) );
			}
			else if ( hhandledOutput->type == MidiMessage::Type::NoteOff ) {
				// Remove the corresponding Note-On message. It does not require
				// a Note-Off anymore.
				const auto signature = std::make_pair(
					Midi::noteFromIntClamp(
						static_cast<int>( hhandledOutput->data1 )
					),
					hhandledOutput->channel
				);
				const auto it = noteOnMessages.find( signature );
				if ( it != noteOnMessages.end() ) {
					noteOnMessages.erase( it );
				}
			}
		}
	}

	for ( const auto [nnote, cchannel] : noteOnMessages ) {
		MidiMessage::NoteOff noteOff = {
			nnote, Midi::ParameterMinimum, cchannel
		};
		enqueueOutputMessage( MidiMessage::from( noteOff ) );
	}

	// "All Notes Off" Channel Mode Message
	MidiMessage::ControlChange allNotesOff = {
		Midi::parameterFromIntClamp( 123 ), Midi::ParameterMinimum,
		Preferences::get_instance()->getMidiFeedbackChannel()
	};
	enqueueOutputMessage( MidiMessage::from( allNotesOff ) );
}

void MidiBaseDriver::startMidiClockStream( float fBpm )
{
	// Ensure there is just a single thread.
	m_bSendClockTick = false;
	if ( m_pClockThread != nullptr ) {
		m_pClockThread->join();
		m_pClockThread = nullptr;
	}

	// 24 MIDI Clock messages should make up a quarter.
	const double fInterval = 60000 / fBpm / 24.0;
	m_interval = std::chrono::duration<double, std::milli>( fInterval );

	m_bSendClockTick = true;
	m_nTickCount = 0;
	m_pClockThread = std::make_shared<std::thread>(
		MidiBaseDriver::midiClockStream, (void*) this
	);
}

void MidiBaseDriver::stopMidiClockStream()
{
	m_bSendClockTick = false;

	if ( m_pClockThread != nullptr ) {
		m_pClockThread->join();
		m_pClockThread = nullptr;
	}

	// By resetting the time point of the last MIDI clock message sent the next
	// call to startMidiClockStream() will send a message right away instead of
	// waiting for the provided interval or logging failure to do so.
	m_lastTick = TimePoint();
	m_intervalCompensation = std::chrono::microseconds::zero();
	m_nAverageIntervalNs = 0;
}

void MidiBaseDriver::waitForNextMidiClockTick()
{
	if ( !m_bSendClockTick || m_pClockThread == nullptr ) {
		return;
	}

	std::unique_lock lock{ m_midiClockMutex };

	m_bNotifyOnNextTick = true;

	m_midiClockCV.wait( lock, [&] { return !m_bNotifyOnNextTick; } );
}

void MidiBaseDriver::midiClockStream( void* pInstance )
{
	auto pMidiDriver = static_cast<MidiBaseDriver*>( pInstance );
	if ( pMidiDriver == nullptr ) {
		ERRORLOG( "Invalid instance provided. Shutting down." );
		return;
	}

	auto pHydrogen = Hydrogen::get_instance();
	const auto pPref = Preferences::get_instance();

	while ( pMidiDriver->m_bSendClockTick ) {
		auto start = Clock::now();

		long nSleepBiasNs = 0;
		if ( pMidiDriver->m_lastTick != TimePoint() ) {
			if ( start - pMidiDriver->m_lastTick >=
				 pMidiDriver->m_interval +
					 pMidiDriver->m_intervalCompensation ) {
				WARNINGLOG(
					QString(
						"MIDI Clock message could not be sent in time. "
						"Duration: [%1], Interval: [%2], compensation: [%3]"
					)
						.arg( ( pMidiDriver->m_lastTick - start ).count() )
						.arg( pMidiDriver->m_interval.count() )
						.arg( pMidiDriver->m_intervalCompensation.count() )
				);
			}
			else {
				// Compensate for the processing time.
				const auto sleepDuration = pMidiDriver->m_interval +
										   pMidiDriver->m_intervalCompensation -
										   ( start - pMidiDriver->m_lastTick );

				const auto preSleep = Clock::now();
				pHydrogen->getTimeHelper()->highResolutionSleep( sleepDuration
				);
				const auto postSleep = Clock::now();

				// Sleep routines only guarantee to sleep for _at least_ the
				// provided amount of time. The additional sleep time we have to
				// compensate on the next tick.
				nSleepBiasNs =
					std::chrono::duration_cast<std::chrono::nanoseconds>(
						postSleep - preSleep - sleepDuration
					)
						.count();
			}
		}

		// Sending the message itself does increase the distance between
		// consecutive ticks. We have to compensate for this delay or the
		// resulting tempo will always be too low.
		const auto preSend = Clock::now();

		// Send event
		if ( pPref->getMidiFeedbackChannel() != Midi::ChannelOff &&
			 pPref->getMidiFeedbackChannel() != Midi::ChannelInvalid ) {
			pMidiDriver->sendMessage( MidiMessage(
				MidiMessage::Type::TimingClock, Midi::ParameterMinimum,
				Midi::ParameterMinimum, pPref->getMidiFeedbackChannel()
			) );
		}
		++pMidiDriver->m_nTickCount;

		const auto end = Clock::now();

		// Due to systematic errors, like biases in processing routines, the
		// interval using which MIDI clock signals are being sent could be off
		// and resulting tempo would drift away from our internal one. In order
		// to prevent this from happening, we keep track of the average interval
		// on which MIDI clock messages have been sent and compensate in such a
		// way that the next average would be our target interval.
		//
		// Skip the first round in order to have a valid m_lastTick.
		if ( pMidiDriver->m_nTickCount > 1 ) {
			// Cumulative average.
			pMidiDriver->m_nAverageIntervalNs =
				( std::chrono::duration_cast<std::chrono::nanoseconds>(
					  end - pMidiDriver->m_lastTick
				  )
					  .count() +
				  pMidiDriver->m_nAverageIntervalNs *
					  ( pMidiDriver->m_nTickCount - 2 ) ) /
				( pMidiDriver->m_nTickCount - 1 );

			pMidiDriver->m_intervalCompensation =
				( pMidiDriver->m_nTickCount - 1 ) *
				( pMidiDriver->m_interval -
				  std::chrono::duration<long, std::nano>(
					  pMidiDriver->m_nAverageIntervalNs
				  ) );
		}

		// Componesate for the surplus time consumed during sleep.
		pMidiDriver->m_intervalCompensation -=
			std::chrono::duration<long, std::nano>( nSleepBiasNs );

		// Compensate for the processing time of sending the MIDI message
		pMidiDriver->m_intervalCompensation -= ( end - preSend );

		pMidiDriver->m_lastTick = end;

		// Send notification - if required.
		//
		// A notification on a tick event is only required for starting
		// transport. This occurs very seldomly compared to sending a tick
		// without. It is not 100% clean but we omit locking the mutex when
		// accessing the atomic shared data pMidiDriver->m_bNotifyOnNextTick
		// because of its average cost.
		if ( pMidiDriver->m_bNotifyOnNextTick ) {
			std::scoped_lock lock{ pMidiDriver->m_midiClockMutex };

			pMidiDriver->m_bNotifyOnNextTick = false;

			pMidiDriver->m_midiClockCV.notify_all();
		}
	}
}

std::shared_ptr<MidiInput::HandledInput> MidiBaseDriver::handleMessage(
	const MidiMessage& msg
)
{
	const auto pHandledInput = MidiInput::handleMessage( msg );

	if ( pHandledInput != nullptr &&
		 pHandledInput->type != MidiMessage::Type::Unknown ) {
		QMutexLocker mx( &m_handledInputsMutex );
		m_handledInputs.push_back( pHandledInput );
		if ( m_handledInputs.size() > MidiBaseDriver::nBacklogSize ) {
			m_handledInputs.pop_front();
		}

		EventQueue::get_instance()->pushEvent( Event::Type::MidiInput, 0 );
	}

	return pHandledInput;
}

void MidiBaseDriver::inputMessageHandler( void* pInstance )
{
	auto pDriver = static_cast<MidiBaseDriver*>( pInstance );
	if ( pDriver == nullptr ) {
		ERRORLOG( "Invalid instance provided. Shutting down." );
		return;
	}

	// Signal the instance that we are ready.
	pDriver->m_bInputActive = true;
	pDriver->m_inputMessageHandlerCV.notify_all();

	std::vector<MidiMessage> messages;
	while ( pDriver->m_bInputActive ) {
		messages.clear();
		{
			std::unique_lock lock{ pDriver->m_inputMessageHandlerMutex };
			pDriver->m_inputMessageHandlerCV.wait( lock, [&] {
				return pDriver->m_inputMessageQueue.size() > 0 ||
					   !pDriver->m_bInputActive;
			} );

			if ( !pDriver->m_bInputActive ) {
				return;
			}

			// We pop all events and store it in a vector to block the minimum
			// time possible and the have our MIDI driver to be as responsive as
			// possible.
			messages.reserve( pDriver->m_inputMessageQueue.size() );
			while ( pDriver->m_inputMessageQueue.size() > 0 ) {
				messages.push_back(
					std::move( pDriver->m_inputMessageQueue.front() )
				);
				pDriver->m_inputMessageQueue.pop();
			}
		}

		for ( const auto& mmessage : messages ) {
			pDriver->handleMessage( mmessage );
		}
	}
}

std::shared_ptr<MidiOutput::HandledOutput> MidiBaseDriver::sendMessage(
	const MidiMessage& msg
)
{
	const auto pHandledOutput = MidiOutput::sendMessage( msg );

	if ( pHandledOutput != nullptr &&
		 pHandledOutput->type != MidiMessage::Type::Unknown ) {
		QMutexLocker mx( &m_handledOutputMutex );
		m_handledOutputs.push_back( pHandledOutput );
		if ( m_handledOutputs.size() > MidiBaseDriver::nBacklogSize ) {
			m_handledOutputs.pop_front();
		}

		EventQueue::get_instance()->pushEvent( Event::Type::MidiOutput, 0 );
	}

	return pHandledOutput;
}

void MidiBaseDriver::outputMessageHandler( void* pInstance )
{
	auto pDriver = static_cast<MidiBaseDriver*>( pInstance );
	if ( pDriver == nullptr ) {
		ERRORLOG( "Invalid instance provided. Shutting down." );
		return;
	}

	// Signal the instance that we are ready.
	pDriver->m_bOutputActive = true;
	pDriver->m_outputMessageHandlerCV.notify_all();

	std::vector<MidiMessage> messages;
	while ( pDriver->m_bOutputActive ) {
		messages.clear();
		{
			std::unique_lock lock{ pDriver->m_outputMessageHandlerMutex };
			pDriver->m_outputMessageHandlerCV.wait( lock, [&] {
				return pDriver->m_outputMessageQueue.size() > 0 ||
					   !pDriver->m_bOutputActive;
			} );

			if ( !pDriver->m_bOutputActive ) {
				return;
			}

			// We pop all events and store it in a vector to block the minimum
			// time possible and the have our MIDI driver to be as responsive as
			// possible.
			messages.reserve( pDriver->m_outputMessageQueue.size() );
			while ( pDriver->m_outputMessageQueue.size() > 0 ) {
				messages.push_back(
					std::move( pDriver->m_outputMessageQueue.front() )
				);
				pDriver->m_outputMessageQueue.pop();
			}
		}

		for ( const auto& mmessage : messages ) {
			pDriver->sendMessage( mmessage );
		}
	}
}
};	// namespace H2Core
