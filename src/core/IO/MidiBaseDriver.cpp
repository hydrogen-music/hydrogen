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
#include <core/Preferences/Preferences.h>

#include <QMutexLocker>

namespace H2Core {

MidiBaseDriver::MidiBaseDriver()
 : MidiInput()
   , MidiOutput()
   , m_bSendClockTick( false )
   , m_bNotifyOnNextTick( false )
   , m_interval( 20.0 )
   , m_intervalCompensation( std::chrono::microseconds::zero() )
   , m_nAverageIntervalNs( 0 )
   , m_lastTick( TimePoint() )
   , m_nTickCount( 0 )
   , m_pClockThread( nullptr )
{
	if ( Preferences::get_instance()->getMidiClockOutputSend() ) {
		startMidiClockStream(
							 Hydrogen::get_instance()->getAudioEngine()
							 ->getTransportPosition()->getBpm() );
	}
}

MidiBaseDriver::~MidiBaseDriver() {
	if ( m_pClockThread != nullptr ) {
		stopMidiClockStream();
	}
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
	// Ensure there is just a single thread.
	m_bSendClockTick = false;
	if ( m_pClockThread != nullptr ) {
		m_pClockThread->join();
		m_pClockThread = nullptr;
	}

	// 24 MIDI Clock messages should make up a quarter.
	const double fInterval = 60000 / fBpm / 24.0;
	m_interval = std::chrono::duration<double, std::milli>(fInterval);

	m_bSendClockTick = true;
	m_nTickCount = 0;
	m_pClockThread = std::make_shared<std::thread>(
		MidiBaseDriver::midiClockStream, ( void* ) this );
}

void MidiBaseDriver::stopMidiClockStream() {
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

void MidiBaseDriver::waitForNextMidiClockTick() {
	if ( ! m_bSendClockTick || m_pClockThread == nullptr ) {
		return;
	}

    std::unique_lock lock{ m_midiClockMutex };

	m_bNotifyOnNextTick = true;

	m_midiClockCV.wait( lock, [&]{ return ! m_bNotifyOnNextTick; });
}

void MidiBaseDriver::midiClockStream( void* pInstance ) {
	auto pMidiDriver = static_cast<MidiBaseDriver*>( pInstance );
	if ( pMidiDriver == nullptr ) {
		ERRORLOG( "Invalid instance provided. Shutting down." );
		return;
	}

	auto pHydrogen = Hydrogen::get_instance();

	while ( pMidiDriver->m_bSendClockTick ) {
		auto start = Clock::now();

		long nSleepBiasNs = 0;
		if ( pMidiDriver->m_lastTick != TimePoint() ) {
			if ( start - pMidiDriver->m_lastTick >=
				 pMidiDriver->m_interval + pMidiDriver->m_intervalCompensation ) {
				WARNINGLOG( QString( "MIDI Clock message could not be sent in time. Duration: [%1], Interval: [%2], compensation: [%3]" )
							.arg( ( pMidiDriver->m_lastTick - start ).count() )
							.arg( pMidiDriver->m_interval.count() )
							.arg( pMidiDriver->m_intervalCompensation.count() ) );
			}
			else {
				// Compensate for the processing time.
				const auto sleepDuration = pMidiDriver->m_interval +
					pMidiDriver->m_intervalCompensation -
					( start - pMidiDriver->m_lastTick );

				const auto preSleep = Clock::now();
				pHydrogen->getTimeHelper()->highResolutionSleep( sleepDuration );
				const auto postSleep = Clock::now();

				// Sleep routines only guarantee to sleep for _at least_ the
				// provided amount of time. The additional sleep time we have to
				// compensate on the next tick.
				nSleepBiasNs = std::chrono::duration_cast<
					std::chrono::nanoseconds>(
						postSleep - preSleep - sleepDuration ).count();
			}
		}

		// Sending the message itself does increase the distance between
		// consecutive ticks. We have to compensate for this delay or the
		// resulting tempo will always be too low.
		const auto preSend = Clock::now();

		// Send event
		pMidiDriver->sendMessage(
			MidiMessage( MidiMessage::Type::TimingClock, 0, 0, 0 ) );
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
					end - pMidiDriver->m_lastTick ).count() +
				  pMidiDriver->m_nAverageIntervalNs *
				  ( pMidiDriver->m_nTickCount - 2 ) ) /
				( pMidiDriver->m_nTickCount - 1 );

			pMidiDriver->m_intervalCompensation =
				( pMidiDriver->m_nTickCount - 1 ) * ( pMidiDriver->m_interval -
				std::chrono::duration<long, std::nano>(
					pMidiDriver->m_nAverageIntervalNs ) );
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
		// accessing the atomic shared data pMidiDriver->m_bNotifyOnNextTick because of its
		// average cost.
		if ( pMidiDriver->m_bNotifyOnNextTick ) {
			std::scoped_lock lock{ pMidiDriver->m_midiClockMutex };

			pMidiDriver->m_bNotifyOnNextTick = false;

			pMidiDriver->m_midiClockCV.notify_all();
		}

	}
}
};
