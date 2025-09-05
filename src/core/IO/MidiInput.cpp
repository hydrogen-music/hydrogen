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

#include <core/IO/MidiInput.h>

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Playlist.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/Midi/MidiAction.h>
#include <core/Midi/MidiActionManager.h>
#include <core/Midi/MidiMap.h>
#include <core/Preferences/Preferences.h>

namespace H2Core
{

MidiInput::MidiInput() {
}

MidiInput::~MidiInput() {
}

std::shared_ptr<MidiInput::HandledInput> MidiInput::handleMessage(
	const MidiMessage& msg )
{
	auto pHandledInput = std::make_shared<HandledInput>();
	pHandledInput->timestamp = QTime::currentTime();
	pHandledInput->type = msg.getType();
	pHandledInput->nData1 = msg.getData1();
	pHandledInput->nData2 = msg.getData2();
	pHandledInput->nChannel = msg.getChannel();

	INFOLOG( QString( "Incoming message:  [%1]" ).arg( msg.toQString() ) );

	// midi channel filter for all messages
	bool bIsChannelValid = true;
	auto pPref = Preferences::get_instance();
	if ( pPref->m_nMidiChannelFilter != -1 &&
		 pPref->m_nMidiChannelFilter != msg.getChannel() ) {
		bIsChannelValid = false;
	}

	// exclude all midi channel filter independent messages
	auto type = msg.getType();
	if ( MidiMessage::Type::Continue == type ||
		 MidiMessage::Type::QuarterFrame == type ||
		 MidiMessage::Type::SongPos == type ||
		 MidiMessage::Type::Start == type ||
		 MidiMessage::Type::Stop == type ||
		 MidiMessage::Type::Sysex == type ||
		 MidiMessage::Type::TimingClock == type ) {
		bIsChannelValid = true;
	}

	if ( ! bIsChannelValid ) {
		INFOLOG( QString( "Dropping message due to invalid channel: [%1]" )
				 .arg( msg.toQString() ) );
		return pHandledInput;
	}

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pMidiActionManager = pHydrogen->getMidiActionManager();
	if ( ! pHydrogen->getSong() ) {
		ERRORLOG( "No song loaded, skipping note" );
		return pHandledInput;
	}

	switch ( type ) {
	case MidiMessage::Type::Sysex:
		handleSysexMessage( msg, pHandledInput );
		break;

	case MidiMessage::Type::NoteOn:
		handleNoteOnMessage( msg, pHandledInput );
		break;

	case MidiMessage::Type::NoteOff:
		handleNoteOffMessage( msg, false, pHandledInput );
		break;

	case MidiMessage::Type::PolyphonicKeyPressure:
		handlePolyphonicKeyPressureMessage( msg, pHandledInput );
		break;

	case MidiMessage::Type::ControlChange:
		handleControlChangeMessage( msg, pHandledInput );
		break;

	case MidiMessage::Type::ProgramChange:
		handleProgramChangeMessage( msg, pHandledInput );
		break;

	case MidiMessage::Type::Start:
		// Start from position 0
		if ( pPref->getMidiTransportInputHandling() ) {
			CoreActionController::locateToColumn( 0 );
			// According to the MIDI Spec 1.0 v4.2.1 Start and Continue indicate
			// that transport is about to start. But the actual start is done on
			// the next MIDI clock tick.
			if ( ! pPref->getMidiClockInputHandling() ) {
				// Start right away
				pMidiActionManager->handleMidiActionAsync(
					std::make_shared<MidiAction>( MidiAction::Type::Play ) );
			}
			else {
				pMidiActionManager->setPendingStart( true );
			}
		}
		break;

	case MidiMessage::Type::Continue: {
		// Start transport at the current position.
		if ( pPref->getMidiTransportInputHandling() ) {
			// According to the MIDI Spec 1.0 v4.2.1 Start and Continue indicate
			// that transport is about to start. But the actual start is done on
			// the next MIDI clock tick.
			if ( ! pPref->getMidiClockInputHandling() ) {
				// Start right away
				pMidiActionManager->handleMidiActionAsync(
					std::make_shared<MidiAction>( MidiAction::Type::Play ) );
			}
			else {
				pMidiActionManager->setPendingStart( true );
			}
		}
		break;
	}

	case MidiMessage::Type::Stop: {
		// Stop in current position i.e. Pause. According to the MIDI Spec 1.0
		// v4.2.1 stopping should always be done immediately.
		if ( pPref->getMidiTransportInputHandling() ) {
			pMidiActionManager->handleMidiActionAsync(
				std::make_shared<MidiAction>( MidiAction::Type::Pause ) );
		}
		break;
	}

	case MidiMessage::Type::SongPos:
		if ( pPref->getMidiTransportInputHandling() ) {
			// A song position provided via MIDI has the lowest resolution of a
			// 1/16 note / 6 MIDI clocks. 24 MIDI clocks make a quarter.
			CoreActionController::locateToTick(
				msg.getData1() * 6 * H2Core::nTicksPerQuarter / 24, true );
		}
		break;

	case MidiMessage::Type::SongSelect:
		if ( pPref->getMidiTransportInputHandling() ) {
			// According to the MIDI 1.0 spec Version 4.2.1 this message
 			// indicates which "song or sequence is to be played". Since
 			// Hydrogen has both concepts of songs and sequences, we let the
 			// user choose via the song mode.
 			if ( pHydrogen->getMode() == Song::Mode::Song ) {
				if ( pHydrogen->getPlaylist() == nullptr ||
					 pHydrogen->getPlaylist()->size() == 0 ) {
					WARNINGLOG( "In Song Mode the SONG_SELECT MIDI message is used to select songs from the current playlist. But you do not have a playlist yet." );
				}
				else {
					auto pAction = std::make_shared<MidiAction>(
						MidiAction::Type::PlaylistSong );
					pAction->setParameter1( QString::number( msg.getData1() ) );
					pMidiActionManager->handleMidiActionAsync( pAction );
				}
			}
			else {
				auto pAction = std::make_shared<MidiAction>(
					MidiAction::Type::SelectNextPattern );
				pAction->setParameter1( QString::number( msg.getData1() ) );
				pMidiActionManager->handleMidiActionAsync( pAction );
			}
		}
		break;

	case MidiMessage::Type::TimingClock:
		if ( pPref->getMidiClockInputHandling() ) {
			pMidiActionManager->handleMidiActionAsync(
				std::make_shared<MidiAction>( MidiAction::Type::TimingClockTick ));
		}
		break;

	case MidiMessage::Type::ChannelPressure:
	case MidiMessage::Type::PitchWheel:
	case MidiMessage::Type::QuarterFrame:
	case MidiMessage::Type::TuneRequest:
	case MidiMessage::Type::ActiveSensing:
	case MidiMessage::Type::Reset:
		INFOLOG( QString( "MIDI message of type [%1] is not supported by Hydrogen" )
				  .arg( MidiMessage::TypeToQString( msg.getType() ) ) );
		return pHandledInput;

	case MidiMessage::Type::Unknown:
		WARNINGLOG( "Unknown midi message" );
		return pHandledInput;

	default:
		INFOLOG( QString( "unhandled midi message type: %1 (%2)" )
				 .arg( static_cast<int>( msg.getType() ) )
				 .arg( MidiMessage::TypeToQString( msg.getType() ) ) );
		return pHandledInput;
	}

	// Two spaces after "msg." in a row to align message parameters
	DEBUGLOG( QString( "DONE handling msg: [%1]" ).arg( msg.toQString() ) );

	return pHandledInput;
}

void MidiInput::handleControlChangeMessage(
	const MidiMessage& msg, std::shared_ptr<HandledInput> pHandledInput )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiActionManager = pHydrogen->getMidiActionManager();
	const auto pMidiMap = Preferences::get_instance()->getMidiMap();

	for ( const auto& ppAction : pMidiMap->getCCActions( msg.getData1() ) ) {
		if ( ppAction != nullptr && ! ppAction->isNull() ) {
			auto pNewAction = MidiAction::from( ppAction );
			pNewAction->setValue( QString::number( msg.getData2() ) );
			pMidiActionManager->handleMidiActionAsync( pNewAction );
			pHandledInput->actionTypes.push_back( pNewAction->getType() );
		}
	}

	if ( msg.getData1() == 04 ) {
		pHydrogen->setHihatOpenness( msg.getData2() );
	}

	pHydrogen->setLastMidiEvent( MidiMessage::Event::CC );
	pHydrogen->setLastMidiEventParameter( msg.getData1() );
}

void MidiInput::handleProgramChangeMessage(
	const MidiMessage& msg, std::shared_ptr<HandledInput> pHandledInput )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiActionManager = pHydrogen->getMidiActionManager();
	const auto pMidiMap = Preferences::get_instance()->getMidiMap();

	for ( const auto& ppAction : pMidiMap->getPCActions() ) {
		if ( ppAction != nullptr && ! ppAction->isNull() ) {
			auto pNewAction = MidiAction::from( ppAction );
			pNewAction->setValue( QString::number( msg.getData1() ) );
			pMidiActionManager->handleMidiActionAsync( pNewAction );
			pHandledInput->actionTypes.push_back( pNewAction->getType() );
		}
	}

	pHydrogen->setLastMidiEvent( MidiMessage::Event::PC );
	pHydrogen->setLastMidiEventParameter( 0 );
}

void MidiInput::handleNoteOnMessage(
	const MidiMessage& msg, std::shared_ptr<HandledInput> pHandledInput )
{
	const int nNote = msg.getData1();
	const float fVelocity = msg.getData2() / 127.0;

	if ( fVelocity == 0 ) {
		handleNoteOffMessage( msg, false, pHandledInput );
		return;
	}

	const auto pPref = Preferences::get_instance();
	const auto pMidiMap = pPref->getMidiMap();
	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiActionManager = pHydrogen->getMidiActionManager();

	pHydrogen->setLastMidiEvent( MidiMessage::Event::Note );
	pHydrogen->setLastMidiEventParameter( msg.getData1() );

	bool bActionSuccess = false;
	for ( const auto& ppAction : pMidiMap->getNoteActions( msg.getData1() ) ) {
		if ( ppAction != nullptr && ! ppAction->isNull() ) {
			auto pNewAction = MidiAction::from( ppAction );
			pNewAction->setValue( QString::number( msg.getData2() ) );
			if ( pMidiActionManager->handleMidiActionAsync( pNewAction ) ) {
				bActionSuccess = true;
				pHandledInput->actionTypes.push_back( pNewAction->getType() );
			}
		}
	}

	if ( bActionSuccess && pPref->m_bMidiDiscardNoteAfterAction ) {
		return;
	}

	QStringList mappedInstruments;
	CoreActionController::handleNote( nNote, fVelocity, false, &mappedInstruments );

	pHandledInput->mappedInstruments = mappedInstruments;
}

/*
	EDrums (at least Roland TD-6V) uses PolyphonicKeyPressure
	for cymbal choke.
	If the message is 127 (choked) we send a NoteOff
*/
void MidiInput::handlePolyphonicKeyPressureMessage(
	const MidiMessage& msg, std::shared_ptr<HandledInput> pHandledInput )
{
	if( msg.getData2() == 127 ) {
		handleNoteOffMessage( msg, true, pHandledInput );
	}
}

void MidiInput::handleNoteOffMessage( const MidiMessage& msg, bool CymbalChoke,
									  std::shared_ptr<HandledInput> pHandledInput )
{
	if ( !CymbalChoke && Preferences::get_instance()->m_bMidiNoteOffIgnore ) {
		return;
	}

	QStringList mappedInstruments;
	CoreActionController::handleNote(
		msg.getData1(), 0.0, true, &mappedInstruments );

	pHandledInput->mappedInstruments = mappedInstruments;
}

void MidiInput::handleSysexMessage( const MidiMessage& msg,
									std::shared_ptr<HandledInput> pHandledInput )
{

	/*
		General MMC message
		0	1	2	3	4	5
		F0	7F	id	6	cmd	247

		cmd:
		1	stop
		2	play
		3	Deferred play
		4	Fast Forward
		5	Rewind
		6	Record strobe (punch in)
		7	Record exit (punch out)
		8	Record ready
		9	Pause


		Goto MMC message
		0	1	2	3	4	5	6	7	8	9	10	11	12
		240	127	id	6	68	6	1	hr	mn	sc	fr	ff	247
	*/

	auto pHydrogen = Hydrogen::get_instance();
	auto pMidiActionManager = pHydrogen->getMidiActionManager();
	const auto pMidiMap = Preferences::get_instance()->getMidiMap();

	const auto sysexData = msg.getSysexData();

	if ( sysexData.size() == 6 &&
		 sysexData[ 1 ] == 127 && sysexData[ 3 ] == 6 ) {
		// MIDI Machine Control (MMC) message

		MidiMessage::Event event = MidiMessage::Event::Null;
		QString sMMCtype;
		switch ( sysexData[4] ) {
		case 1:	// STOP
			event = MidiMessage::Event::MmcStop;
			break;

		case 2:	// PLAY
			event = MidiMessage::Event::MmcPlay;
			break;

		case 3:	//DEFERRED PLAY
			event = MidiMessage::Event::MmcDeferredPlay;
			break;

		case 4:	// FAST FWD
			event = MidiMessage::Event::MmcFastForward;
			break;

		case 5:	// REWIND
			event = MidiMessage::Event::MmcRewind;
			break;

		case 6:	// RECORD STROBE (PUNCH IN)
			event = MidiMessage::Event::MmcRecordStrobe;
			break;

		case 7:	// RECORD EXIT (PUNCH OUT)
			event = MidiMessage::Event::MmcRecordExit;
			break;

		case 8:	// RECORD READY
			event = MidiMessage::Event::MmcRecordReady;
			break;

		case 9:	//PAUSE
			event = MidiMessage::Event::MmcPause;
			break;
		}

		if ( event != MidiMessage::Event::Null ) {
			const QString sMMCtype = MidiMessage::EventToQString( event );
			INFOLOG( QString( "MIDI Machine Control command: [%1]" )
					 .arg( sMMCtype ) );
			
			pHydrogen->setLastMidiEvent( event );
			pHydrogen->setLastMidiEventParameter( msg.getData1() );

			auto actions = pMidiMap->getMMCActions( sMMCtype );
			pMidiActionManager->handleMidiActionsAsync( actions );
			for ( const auto& ppAction : actions ) {
				if ( ppAction != nullptr ) {
					pHandledInput->actionTypes.push_back( ppAction->getType() );
				}
			}
		}
		else {
			WARNINGLOG( "Unknown MIDI Machine Control (MMC) Command" );
		}
	}
	else if ( sysexData.size() == 13 &&
			  sysexData[ 1 ] == 127 && sysexData[ 3 ] == 68 ) {
		WARNINGLOG( "MMC GOTO Message not implemented yet" );
		// int hr = sysexData[7];
		// int mn = sysexData[8];
		// int sc = sysexData[9];
		// int fr = sysexData[10];
		// int ff = sysexData[11];
		// char tmp[200];
		// sprintf( tmp, "[handleSysexMessage] GOTO %d:%d:%d:%d:%d", hr, mn, sc, fr, ff );
		// INFOLOG( tmp );
	}
	else {
		WARNINGLOG( QString( "Unsupported SysEx message: [%1]" )
					.arg( msg.toQString() ) );
	}
}

QString MidiInput::HandledInput::toQString() const {
	QStringList types;
	for ( const auto& ttype : actionTypes ) {
		types << MidiAction::typeToQString( ttype );
	}
	return QString( "timestamp: %1, msg type: %2, nData1: %3, nData2: %4, nChannel: %5, actionTypes: [%6], mappedInstrument: [%7]" )
		.arg( timestamp.toString( "HH:mm:ss.zzz" ) )
		.arg( MidiMessage::TypeToQString( type ) ).arg( nData1 ).arg( nData2 )
		.arg( nChannel ).arg( types.join( "," ) )
		.arg( mappedInstruments.join( ", " ) );
}

};
