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

MidiInput::MidiInput()
		: m_bActive( false )
{
	//

}


MidiInput::~MidiInput()
{
	//INFOLOG( "DESTROY" );
}

void MidiInput::handleMidiMessage( const MidiMessage& msg )
{
		EventQueue::get_instance()->pushEvent( Event::Type::MidiActivity, -1 );

		INFOLOG( QString( "Incoming message:  [%1]" ).arg( msg.toQString() ) );

		// midi channel filter for all messages
		bool bIsChannelValid = true;
		auto pPref = Preferences::get_instance();
		if ( pPref->m_nMidiChannelFilter != -1
		  && pPref->m_nMidiChannelFilter != msg.m_nChannel
		) {
			bIsChannelValid = false;
		}

		// exclude all midi channel filter independent messages
		int type = msg.m_type;
		if (  MidiMessage::SYSEX == type
		   || MidiMessage::START == type
		   || MidiMessage::CONTINUE == type
		   || MidiMessage::STOP == type
		   || MidiMessage::SONG_POS == type
		   || MidiMessage::QUARTER_FRAME == type
		) {
			bIsChannelValid = true;
		}

		if ( !bIsChannelValid) {
			return;
		}

		Hydrogen* pHydrogen = Hydrogen::get_instance();
		auto pAudioEngine = pHydrogen->getAudioEngine();
		if ( ! pHydrogen->getSong() ) {
			ERRORLOG( "No song loaded, skipping note" );
			return;
		}

		switch ( type ) {
		case MidiMessage::SYSEX:
				handleSysexMessage( msg );
				break;

		case MidiMessage::NOTE_ON:
				handleNoteOnMessage( msg );
				break;

		case MidiMessage::NOTE_OFF:
				handleNoteOffMessage( msg, false );
				break;

		case MidiMessage::POLYPHONIC_KEY_PRESSURE:
				handlePolyphonicKeyPressureMessage( msg );
				break;

		case MidiMessage::CONTROL_CHANGE:
				handleControlChangeMessage( msg );
				break;

		case MidiMessage::PROGRAM_CHANGE:
				handleProgramChangeMessage( msg );
				break;

		case MidiMessage::START: /* Start from position 0 */
			if ( pAudioEngine->getState() != AudioEngine::State::Playing ) {
				CoreActionController::locateToColumn( 0 );
				auto pAction = std::make_shared<MidiAction>("PLAY");
				MidiActionManager::get_instance()->handleMidiAction( pAction );
			}
			break;

		case MidiMessage::CONTINUE: /* Just start */ {
			auto pAction = std::make_shared<MidiAction>("PLAY");
			MidiActionManager::get_instance()->handleMidiAction( pAction );
			break;
		}

		case MidiMessage::STOP: /* Stop in current position i.e. Pause */ {
			auto pAction = std::make_shared<MidiAction>("PAUSE");
			MidiActionManager::get_instance()->handleMidiAction( pAction );
			break;
		}

		case MidiMessage::CHANNEL_PRESSURE:
		case MidiMessage::PITCH_WHEEL:
		case MidiMessage::SONG_POS:
		case MidiMessage::QUARTER_FRAME:
		case MidiMessage::SONG_SELECT:
		case MidiMessage::TUNE_REQUEST:
		case MidiMessage::TIMING_CLOCK:
		case MidiMessage::ACTIVE_SENSING:
		case MidiMessage::RESET:
			INFOLOG( QString( "MIDI message of type [%1] is not supported by Hydrogen" )
					  .arg( MidiMessage::TypeToQString( msg.m_type ) ) );
			return;

		case MidiMessage::UNKNOWN:
			WARNINGLOG( "Unknown midi message" );
			return;

		default:
			INFOLOG( QString( "unhandled midi message type: %1 (%2)" )
					  .arg( static_cast<int>( msg.m_type ) )
					  .arg( MidiMessage::TypeToQString( msg.m_type ) ) );
			return;
		}

		// Two spaces after "msg." in a row to align message parameters
		DEBUGLOG( QString( "DONE handling msg: [%1]" ).arg( msg.toQString() ) );
}

void MidiInput::handleControlChangeMessage( const MidiMessage& msg )
{
	//INFOLOG( QString( "[handleMidiMessage] CONTROL_CHANGE Parameter: %1, Value: %2" ).arg( msg.m_nData1 ).arg( msg.m_nData2 ) );
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	MidiActionManager *pMidiActionManager = MidiActionManager::get_instance();
	const auto pMidiMap = Preferences::get_instance()->getMidiMap();

	for ( const auto& ppAction : pMidiMap->getCCActions( msg.m_nData1 ) ) {
		if ( ppAction != nullptr && ! ppAction->isNull() ) {
			auto pNewAction = std::make_shared<MidiAction>( ppAction );
			pNewAction->setValue( QString::number( msg.m_nData2 ) );
			pMidiActionManager->handleMidiAction( pNewAction );
		}
	}

	if ( msg.m_nData1 == 04 ) {
		pHydrogen->setHihatOpenness( msg.m_nData2 );
	}

	pHydrogen->setLastMidiEvent( MidiMessage::Event::CC );
	pHydrogen->setLastMidiEventParameter( msg.m_nData1 );
}

void MidiInput::handleProgramChangeMessage( const MidiMessage& msg )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	MidiActionManager *pMidiActionManager = MidiActionManager::get_instance();
	const auto pMidiMap = Preferences::get_instance()->getMidiMap();

	for ( const auto& ppAction : pMidiMap->getPCActions() ) {
		if ( ppAction != nullptr && ! ppAction->isNull() ) {
			auto pNewAction = std::make_shared<MidiAction>( ppAction );
			pNewAction->setValue( QString::number( msg.m_nData1 ) );
			pMidiActionManager->handleMidiAction( pNewAction );
		}
	}

	pHydrogen->setLastMidiEvent( MidiMessage::Event::PC );
	pHydrogen->setLastMidiEventParameter( 0 );
}

void MidiInput::handleNoteOnMessage( const MidiMessage& msg ) {

	const int nNote = msg.m_nData1;
	const float fVelocity = msg.m_nData2 / 127.0;

	if ( fVelocity == 0 ) {
		handleNoteOffMessage( msg, false );
		return;
	}

	const auto pPref = Preferences::get_instance();
	auto pMidiActionManager = MidiActionManager::get_instance();
	const auto pMidiMap = pPref->getMidiMap();
	auto pHydrogen = Hydrogen::get_instance();

	pHydrogen->setLastMidiEvent( MidiMessage::Event::Note );
	pHydrogen->setLastMidiEventParameter( msg.m_nData1 );

	bool bActionSuccess = false;
	for ( const auto& ppAction : pMidiMap->getNoteActions( msg.m_nData1 ) ) {
		if ( ppAction != nullptr && ! ppAction->isNull() ) {
			auto pNewAction = std::make_shared<MidiAction>( ppAction );
			pNewAction->setValue( QString::number( msg.m_nData2 ) );
			if ( pMidiActionManager->handleMidiAction( pNewAction ) ) {
				bActionSuccess = true;
			}
		}
	}

	if ( bActionSuccess && pPref->m_bMidiDiscardNoteAfterAction ) {
		return;
	}

	CoreActionController::handleNote( nNote, fVelocity, false );
}

/*
	EDrums (at least Roland TD-6V) uses PolyphonicKeyPressure
	for cymbal choke.
	If the message is 127 (choked) we send a NoteOff
*/
void MidiInput::handlePolyphonicKeyPressureMessage( const MidiMessage& msg )
{
	if( msg.m_nData2 == 127 ) {
		handleNoteOffMessage( msg, true );
	}
}

void MidiInput::handleNoteOffMessage( const MidiMessage& msg, bool CymbalChoke )
{
//	INFOLOG( "handleNoteOffMessage" );
	if ( !CymbalChoke && Preferences::get_instance()->m_bMidiNoteOffIgnore ) {
		return;
	}

	CoreActionController::handleNote( msg.m_nData1, 0.0, true );
}

void MidiInput::handleSysexMessage( const MidiMessage& msg )
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


	if ( msg.m_sysexData.size() == 6 && 
		 msg.m_sysexData[ 1 ] == 127 && msg.m_sysexData[ 3 ] == 6 ) {
		// MIDI Machine Control (MMC) message

		MidiMessage::Event event = MidiMessage::Event::Null;
		QString sMMCtype;
		switch ( msg.m_sysexData[4] ) {
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
			pHydrogen->setLastMidiEventParameter( msg.m_nData1 );
			
			pMidiActionManager->handleMidiActions( pMidiMap->getMMCActions( sMMCtype ) );
		}
		else {
			WARNINGLOG( "Unknown MIDI Machine Control (MMC) Command" );
		}
	}
	else if ( msg.m_sysexData.size() == 13 && 
			  msg.m_sysexData[ 1 ] == 127 && msg.m_sysexData[ 3 ] == 68 ) {
		WARNINGLOG( "MMC GOTO Message not implemented yet" );
		// int hr = msg.m_sysexData[7];
		// int mn = msg.m_sysexData[8];
		// int sc = msg.m_sysexData[9];
		// int fr = msg.m_sysexData[10];
		// int ff = msg.m_sysexData[11];
		// char tmp[200];
		// sprintf( tmp, "[handleSysexMessage] GOTO %d:%d:%d:%d:%d", hr, mn, sc, fr, ff );
		// INFOLOG( tmp );
	}
	else {
		WARNINGLOG( QString( "Unsupported SysEx message: [%1]" )
					.arg( msg.toQString() ) );
	}
}

};
