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

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/IO/MidiInput.h>
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

MidiInput::HandledInput MidiInput::handleMessage( const MidiMessage& msg ) {
	HandledInput handledInput;
	handledInput.timestamp = QTime::currentTime();
	handledInput.type = msg.getType();
	handledInput.nData1 = msg.getData1();
	handledInput.nData2 = msg.getData2();
	handledInput.nChannel = msg.getChannel();

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
	if (  MidiMessage::Type::Sysex == type
		  || MidiMessage::Type::Start == type
		  || MidiMessage::Type::Continue == type
		  || MidiMessage::Type::Stop == type
		  || MidiMessage::Type::SongPos == type
		  || MidiMessage::Type::QuarterFrame == type
		) {
		bIsChannelValid = true;
	}

	if ( ! bIsChannelValid ) {
		return handledInput;
	}

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	if ( ! pHydrogen->getSong() ) {
		ERRORLOG( "No song loaded, skipping note" );
		return handledInput;
	}

	switch ( type ) {
	case MidiMessage::Type::Sysex:
		handleSysexMessage( msg, handledInput );
		break;

	case MidiMessage::Type::NoteOn:
		handleNoteOnMessage( msg, handledInput );
		break;

	case MidiMessage::Type::NoteOff:
		handleNoteOffMessage( msg, false, handledInput );
		break;

	case MidiMessage::Type::PolyphonicKeyPressure:
		handlePolyphonicKeyPressureMessage( msg, handledInput );
		break;

	case MidiMessage::Type::ControlChange:
		handleControlChangeMessage( msg, handledInput );
		break;

	case MidiMessage::Type::ProgramChange:
		handleProgramChangeMessage( msg, handledInput );
		break;

	case MidiMessage::Type::Start: /* Start from position 0 */
		if ( pAudioEngine->getState() != AudioEngine::State::Playing ) {
			CoreActionController::locateToColumn( 0 );
			MidiActionManager::get_instance()->handleMidiAction(
				std::make_shared<MidiAction>( MidiAction::Type::Play ) );
		}
		break;

	case MidiMessage::Type::Continue: /* Just start */ {
		MidiActionManager::get_instance()->handleMidiAction(
			std::make_shared<MidiAction>( MidiAction::Type::Play ) );
		break;
	}

	case MidiMessage::Type::Stop: /* Stop in current position i.e. Pause */ {
		MidiActionManager::get_instance()->handleMidiAction(
			std::make_shared<MidiAction>( MidiAction::Type::Pause ) );
		break;
	}

	case MidiMessage::Type::ChannelPressure:
	case MidiMessage::Type::PitchWheel:
	case MidiMessage::Type::SongPos:
	case MidiMessage::Type::QuarterFrame:
	case MidiMessage::Type::SongSelect:
	case MidiMessage::Type::TuneRequest:
	case MidiMessage::Type::TimingClock:
	case MidiMessage::Type::ActiveSensing:
	case MidiMessage::Type::Reset:
		INFOLOG( QString( "MIDI message of type [%1] is not supported by Hydrogen" )
				  .arg( MidiMessage::TypeToQString( msg.getType() ) ) );
		return handledInput;

	case MidiMessage::Type::Unknown:
		WARNINGLOG( "Unknown midi message" );
		return handledInput;

	default:
		INFOLOG( QString( "unhandled midi message type: %1 (%2)" )
				 .arg( static_cast<int>( msg.getType() ) )
				 .arg( MidiMessage::TypeToQString( msg.getType() ) ) );
		return handledInput;
	}

	// Two spaces after "msg." in a row to align message parameters
	DEBUGLOG( QString( "DONE handling msg: [%1]" ).arg( msg.toQString() ) );

	return handledInput;
}

void MidiInput::handleControlChangeMessage( const MidiMessage& msg,
											HandledInput& handledInput )
{
	//INFOLOG( QString( "[handleMessage] CONTROL_CHANGE Parameter: %1, Value: %2" ).arg( msg.m_nData1 ).arg( msg.m_nData2 ) );
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	MidiActionManager *pMidiActionManager = MidiActionManager::get_instance();
	const auto pMidiMap = Preferences::get_instance()->getMidiMap();

	for ( const auto& ppAction : pMidiMap->getCCActions( msg.getData1() ) ) {
		if ( ppAction != nullptr && ! ppAction->isNull() ) {
			auto pNewAction = std::make_shared<MidiAction>( ppAction );
			pNewAction->setValue( QString::number( msg.getData2() ) );
			pMidiActionManager->handleMidiAction( pNewAction );
			handledInput.actionTypes.push_back( pNewAction->getType() );
		}
	}

	if ( msg.getData1() == 04 ) {
		pHydrogen->setHihatOpenness( msg.getData2() );
	}

	pHydrogen->setLastMidiEvent( MidiMessage::Event::CC );
	pHydrogen->setLastMidiEventParameter( msg.getData1() );
}

void MidiInput::handleProgramChangeMessage( const MidiMessage& msg,
											HandledInput& handledInput )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	MidiActionManager *pMidiActionManager = MidiActionManager::get_instance();
	const auto pMidiMap = Preferences::get_instance()->getMidiMap();

	for ( const auto& ppAction : pMidiMap->getPCActions() ) {
		if ( ppAction != nullptr && ! ppAction->isNull() ) {
			auto pNewAction = std::make_shared<MidiAction>( ppAction );
			pNewAction->setValue( QString::number( msg.getData1() ) );
			pMidiActionManager->handleMidiAction( pNewAction );
			handledInput.actionTypes.push_back( pNewAction->getType() );
		}
	}

	pHydrogen->setLastMidiEvent( MidiMessage::Event::PC );
	pHydrogen->setLastMidiEventParameter( 0 );
}

void MidiInput::handleNoteOnMessage( const MidiMessage& msg,
									 HandledInput& handledInput )
{
	const int nNote = msg.getData1();
	const float fVelocity = msg.getData2() / 127.0;

	if ( fVelocity == 0 ) {
		handleNoteOffMessage( msg, false, handledInput );
		return;
	}

	const auto pPref = Preferences::get_instance();
	auto pMidiActionManager = MidiActionManager::get_instance();
	const auto pMidiMap = pPref->getMidiMap();
	auto pHydrogen = Hydrogen::get_instance();

	pHydrogen->setLastMidiEvent( MidiMessage::Event::Note );
	pHydrogen->setLastMidiEventParameter( msg.getData1() );

	bool bActionSuccess = false;
	for ( const auto& ppAction : pMidiMap->getNoteActions( msg.getData1() ) ) {
		if ( ppAction != nullptr && ! ppAction->isNull() ) {
			auto pNewAction = std::make_shared<MidiAction>( ppAction );
			pNewAction->setValue( QString::number( msg.getData2() ) );
			if ( pMidiActionManager->handleMidiAction( pNewAction ) ) {
				bActionSuccess = true;
				handledInput.actionTypes.push_back( pNewAction->getType() );
			}
		}
	}

	if ( bActionSuccess && pPref->m_bMidiDiscardNoteAfterAction ) {
		return;
	}

	QStringList mappedInstruments;
	CoreActionController::handleNote( nNote, fVelocity, false, &mappedInstruments );

	handledInput.mappedInstruments = mappedInstruments;
}

/*
	EDrums (at least Roland TD-6V) uses PolyphonicKeyPressure
	for cymbal choke.
	If the message is 127 (choked) we send a NoteOff
*/
void MidiInput::handlePolyphonicKeyPressureMessage( const MidiMessage& msg,
													HandledInput& handledInput )
{
	if( msg.getData2() == 127 ) {
		handleNoteOffMessage( msg, true, handledInput );
	}
}

void MidiInput::handleNoteOffMessage( const MidiMessage& msg, bool CymbalChoke,
									  HandledInput& handledInput )
{
//	INFOLOG( "handleNoteOffMessage" );
	if ( !CymbalChoke && Preferences::get_instance()->m_bMidiNoteOffIgnore ) {
		return;
	}

	QStringList mappedInstruments;
	CoreActionController::handleNote(
		msg.getData1(), 0.0, true, &mappedInstruments );

	handledInput.mappedInstruments = mappedInstruments;
}

void MidiInput::handleSysexMessage( const MidiMessage& msg,
									HandledInput& handledInput )
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


	MidiActionManager * pMidiActionManager = MidiActionManager::get_instance();
	const auto pMidiMap = Preferences::get_instance()->getMidiMap();
	Hydrogen *pHydrogen = Hydrogen::get_instance();

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
			pMidiActionManager->handleMidiActions( actions );
			for ( const auto& ppAction : actions ) {
				if ( ppAction != nullptr ) {
					handledInput.actionTypes.push_back( ppAction->getType() );
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
