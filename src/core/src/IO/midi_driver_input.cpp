/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <hydrogen/IO/MidiDriverInput.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/midi_action.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/midi_map.h>

namespace H2Core
{

MidiDriverInput::MidiDriverInput( const char* class_name )
		: Object( class_name )
		, m_bActive( false )
		, __hihat_cc_openess ( 127 )
		, __noteOffTick( 0 )
		, __noteOnTick( 0 )
{
	//INFOLOG( "INIT" );

}


MidiDriverInput::~MidiDriverInput()
{
	//INFOLOG( "DESTROY" );
}

void MidiDriverInput::handleMidiMessage( const MidiMessage& msg )
{
		EventQueue::get_instance()->push_event( EVENT_MIDI_ACTIVITY, -1 );

		INFOLOG( "[start of handleMidiMessage]" );
		INFOLOG( QString("[handleMidiMessage] channel: %1").arg(msg.m_nChannel) );
		INFOLOG( QString("[handleMidiMessage] val1: %1").arg( msg.m_nData1 ) );
		INFOLOG( QString("[handleMidiMessage] val2: %1").arg( msg.m_nData2 ) );

		// midi channel filter for all messages
		bool bIsChannelValid = true;
		Preferences* pPref = Preferences::get_instance();
		if ( pPref->m_nMidiChannelFilter != -1
		  && pPref->m_nMidiChannelFilter != msg.m_nChannel
		) {
			bIsChannelValid = false;
		}

		// exclude all midi channel filter independent messages
		int type = msg.m_type;
		if (  MidiMessage::SYSEX == type
		   || MidiMessage::SYSTEM_EXCLUSIVE == type
		   || MidiMessage::START == type
		   || MidiMessage::CONTINUE == type
		   || MidiMessage::STOP == type
		   || MidiMessage::SONG_POS == type
		   || MidiMessage::QUARTER_FRAME == type
		) {
			bIsChannelValid = true;
		}

		if ( !bIsChannelValid) return;

		Hydrogen* pHydrogen = Hydrogen::get_instance();
		if ( ! pHydrogen->getSong() ) {
			ERRORLOG( "No song loaded, skipping note" );
			return;
		}

		switch ( type ) {
		case MidiMessage::SYSEX:
				handleSysexMessage( msg );
				break;

		case MidiMessage::NOTE_ON:
				INFOLOG("This is a NOTE ON message.");
				handleNoteOnMessage( msg );
				break;

		case MidiMessage::NOTE_OFF:
				INFOLOG("This is a NOTE OFF message.");
				handleNoteOffMessage( msg, false );
				break;

		case MidiMessage::POLYPHONIC_KEY_PRESSURE:
				//ERRORLOG( "POLYPHONIC_KEY_PRESSURE event not handled yet" );
				INFOLOG( QString( "[handleMidiMessage] POLYPHONIC_KEY_PRESSURE Parameter: %1, Value: %2")
					.arg( msg.m_nData1 ).arg( msg.m_nData2 ) );
				handlePolyphonicKeyPressureMessage( msg );
				break;

		case MidiMessage::CONTROL_CHANGE:
				INFOLOG( QString( "[handleMidiMessage] CONTROL_CHANGE Parameter: %1, Value: %2")
					.arg( msg.m_nData1 ).arg( msg.m_nData2 ) );
				handleControlChangeMessage( msg );
				break;

		case MidiMessage::PROGRAM_CHANGE:
				INFOLOG( QString( "[handleMidiMessage] PROGRAM_CHANGE Value: %1" )
					.arg( msg.m_nData1 ) );
				handleProgramChangeMessage( msg );
				break;

		case MidiMessage::CHANNEL_PRESSURE:
				ERRORLOG( "CHANNEL_PRESSURE event not handled yet" );
				break;

		case MidiMessage::PITCH_WHEEL:
				ERRORLOG( "PITCH_WHEEL event not handled yet" );
				break;

		case MidiMessage::SYSTEM_EXCLUSIVE:
				ERRORLOG( "SYSTEM_EXCLUSIVE event not handled yet" );
				break;

		case MidiMessage::START: /* Start from position 0 */
				INFOLOG( "START event" );
				if ( pHydrogen->getState() != STATE_PLAYING ) {
					pHydrogen->setPatternPos( 0 );
					pHydrogen->setTimelineBpm();
					pHydrogen->sequencer_play();
				}
				break;

		case MidiMessage::CONTINUE: /* Just start */
				ERRORLOG( "CONTINUE event" );
				if ( pHydrogen->getState() != STATE_PLAYING )
					pHydrogen->sequencer_play();
				break;

		case MidiMessage::STOP: /* Stop in current position i.e. Pause */
				INFOLOG( "STOP event" );
				if ( pHydrogen->getState() == STATE_PLAYING )
					pHydrogen->sequencer_stop();
				break;

		case MidiMessage::SONG_POS:
				ERRORLOG( "SONG_POS event not handled yet" );
				break;

		case MidiMessage::QUARTER_FRAME:
				WARNINGLOG( "QUARTER_FRAME event not handled yet" );
				break;

		case MidiMessage::UNKNOWN:
				ERRORLOG( "Unknown midi message" );
				break;

		default:
				ERRORLOG( QString( "unhandled midi message type: %1" ).arg( msg.m_type ) );
		}
		INFOLOG("[end of handleMidiMessage]");
}

void MidiDriverInput::handleControlChangeMessage( const MidiMessage& msg )
{
	//INFOLOG( QString( "[handleMidiMessage] CONTROL_CHANGE Parameter: %1, Value: %2" ).arg( msg.m_nData1 ).arg( msg.m_nData2 ) );
	Hydrogen *pEngine = Hydrogen::get_instance();
	MidiActionManager *pMidiActionManager = MidiActionManager::get_instance();
	MidiMap *pMidiMap = MidiMap::get_instance();

	Action *pAction = pMidiMap->getCCAction( msg.m_nData1 );
	pAction->setParameter2( QString::number( msg.m_nData2 ) );

	pMidiActionManager->handleAction( pAction );

	if(msg.m_nData1 == 04){
		__hihat_cc_openess = msg.m_nData2;
	}

	pEngine->lastMidiEvent = "CC";
	pEngine->lastMidiEventParameter = msg.m_nData1;
}

void MidiDriverInput::handleProgramChangeMessage( const MidiMessage& msg )
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	MidiActionManager *pMidiActionManager = MidiActionManager::get_instance();
	MidiMap *pMidiMap = MidiMap::get_instance();

	Action *pAction = pMidiMap->getPCAction();
	pAction->setParameter2( QString::number( msg.m_nData1 ) );

	pMidiActionManager->handleAction( pAction );

	pEngine->lastMidiEvent = "PROGRAM_CHANGE";
	pEngine->lastMidiEventParameter = 0;
}

void MidiDriverInput::handleNoteOnMessage( const MidiMessage& msg )
{
//	INFOLOG( "handleNoteOnMessage" );

	int nNote = msg.m_nData1;
	float fVelocity = msg.m_nData2 / 127.0;

	if ( fVelocity == 0 ) {
		handleNoteOffMessage( msg, false );
		return;
	}

	MidiActionManager * pMidiActionManager = MidiActionManager::get_instance();
	MidiMap * pMidiMap = MidiMap::get_instance();
	Hydrogen *pEngine = Hydrogen::get_instance();

	pEngine->lastMidiEvent = "NOTE";
	pEngine->lastMidiEventParameter = msg.m_nData1;

	bool bActionSuccess = pMidiActionManager->handleAction( pMidiMap->getNoteAction( msg.m_nData1 ) );

	if ( bActionSuccess && Preferences::get_instance()->m_bMidiDiscardNoteAfterAction)
	{
		return;
	}

	bool bPatternSelect = false;

	if ( bPatternSelect ) {
		int patternNumber = nNote - 36;
		//INFOLOG( QString( "next pattern = %1" ).arg( patternNumber ) );
		pEngine->sequencer_setNextPattern( patternNumber );

	} else {
		static const float fPan_L = 0.5f;
		static const float fPan_R = 0.5f;

		int nInstrument = nNote - 36;
		InstrumentList *pInstrList = pEngine->getSong()->get_instrument_list();
		Instrument *pInstr = nullptr;
		
		if ( Preferences::get_instance()->__playselectedinstrument ){
			nInstrument = pEngine->getSelectedInstrumentNumber();
			pInstr= pInstrList->get( pEngine->getSelectedInstrumentNumber());
		}
		else if(Preferences::get_instance()->m_bMidiFixedMapping ){
			pInstr = pInstrList->findMidiNote( nNote );
			
			if(pInstr == nullptr) {
				WARNINGLOG( QString( "Can't find corresponding Instrument for note %1" ).arg( nNote ));
				return;
			}
			
			nInstrument = pInstrList->index( pInstr );
		} else {
			if(nInstrument < 0) {
				//Drop everything < 36
				return;
			}
			
			if( nInstrument >= pInstrList->size()) {
				WARNINGLOG( QString( "Can't find corresponding Instrument for note %1" ).arg( nNote ));
				return;
			}
			
			pInstr = pInstrList->get( nInstrument );
		}

		/*
		Only look to change instrument if the
		current note is actually of hihat and
		hihat openess is outside the instrument selected
		*/
		if ( pInstr != nullptr &&
			 pInstr->get_hihat_grp() >= 0 &&
			 ( __hihat_cc_openess < pInstr->get_lower_cc() || __hihat_cc_openess > pInstr->get_higher_cc() ) )
		{
			for(int i=0 ; i<=pInstrList->size() ; i++)
			{
				Instrument *instr_contestant = pInstrList->get( i );
				if( instr_contestant != nullptr &&
						pInstr->get_hihat_grp() == instr_contestant->get_hihat_grp() &&
						__hihat_cc_openess >= instr_contestant->get_lower_cc() &&
						__hihat_cc_openess <= instr_contestant->get_higher_cc() )
				{
					nInstrument = i;
					break;
				}
			}
		}

		pEngine->addRealtimeNote( nInstrument, fVelocity, fPan_L, fPan_R, 0.0, false, true, nNote );
	}

	__noteOnTick = pEngine->__getMidiRealtimeNoteTickPosition();
}

/*
	EDrums (at least Roland TD-6V) uses PolyphonicKeyPressure
	for cymbal choke.
	If the message is 127 (choked) we send a NoteOff
*/
void MidiDriverInput::handlePolyphonicKeyPressureMessage( const MidiMessage& msg )
{
	if( msg.m_nData2 == 127 ) {
		handleNoteOffMessage( msg, true );
	}
}

void MidiDriverInput::handleNoteOffMessage( const MidiMessage& msg, bool CymbalChoke )
{
//	INFOLOG( "handleNoteOffMessage" );
	if ( !CymbalChoke && Preferences::get_instance()->m_bMidiNoteOffIgnore ) {
		return;
	}

	Hydrogen *pEngine = Hydrogen::get_instance();
	InstrumentList* pInstrList = pEngine->getSong()->get_instrument_list();

	__noteOffTick = pEngine->getTickPosition();
	unsigned long notelength = computeDeltaNoteOnOfftime();

	int nNote = msg.m_nData1;
	//float fVelocity = msg.m_nData2 / 127.0; //we need this in future to controll release velocity
	int nInstrument = nNote - 36;
	Instrument *pInstr = nullptr;

	if ( Preferences::get_instance()->__playselectedinstrument ){
		nInstrument = pEngine->getSelectedInstrumentNumber();
		pInstr = pInstrList->get( pEngine->getSelectedInstrumentNumber());
	} else if( Preferences::get_instance()->m_bMidiFixedMapping ) {
		pInstr = pInstrList->findMidiNote( nNote );

		if( pInstr == nullptr ) {
			WARNINGLOG( QString( "Can't find corresponding Instrument for note %1" ).arg( nNote ));
			return;
		}
		nInstrument = pInstrList->index(pInstr);
	}
	else {
		if( nInstrument < 0 ) {
			//Drop everything < 36
			return;
		}
		
		if( nInstrument >= pInstrList->size()) {
			WARNINGLOG( QString( "Can't find corresponding Instrument for note %1" ).arg( nNote ));
			return;
		}
		
		pInstr =  pInstrList->get(nInstrument);
	}

	float fStep = pow( 1.0594630943593, (nNote) );
	if ( !Preferences::get_instance()->__playselectedinstrument ) {
		fStep = 1;
	}

	bool use_note_off = AudioEngine::get_instance()->get_sampler()->is_instrument_playing( pInstr );
	if(use_note_off){
		if ( Preferences::get_instance()->__playselectedinstrument ){
			AudioEngine::get_instance()->get_sampler()->midi_keyboard_note_off( msg.m_nData1 );
		}
		else
		{
			if ( pInstrList->size() < nInstrument +1 ) {
				return;
			}
			
			Note *pOffNote = new Note( pInstr,
										0.0,
										0.0,
										0.0,
										0.0,
										-1,
										0 );
			pOffNote->set_note_off( true );
			AudioEngine::get_instance()->get_sampler()->note_on( pOffNote );
			delete pOffNote;
		}
		
		if(Preferences::get_instance()->getRecordEvents()) {
			AudioEngine::get_instance()->get_sampler()->setPlayingNotelength( pInstr, notelength * fStep, __noteOnTick );
		}
	}
}


unsigned long MidiDriverInput::computeDeltaNoteOnOfftime()
{
	unsigned long  __notelengthTicks = __noteOffTick - __noteOnTick;
	return __notelengthTicks;

}

void MidiDriverInput::handleSysexMessage( const MidiMessage& msg )
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
	MidiMap * pMidiMap = MidiMap::get_instance();
	Hydrogen *pEngine = Hydrogen::get_instance();

	pEngine->lastMidiEventParameter = msg.m_nData1;


	if ( msg.m_sysexData.size() == 6 ) {
		if (
			( msg.m_sysexData[0] == 240 ) &&
			( msg.m_sysexData[1] == 127 ) &&
					//( msg.m_sysexData[2] == 0 ) &&
			( msg.m_sysexData[3] == 6 ) ) {


			switch ( msg.m_sysexData[4] ) {

			case 1:	// STOP
			{
				pEngine->lastMidiEvent = "MMC_STOP";
				pMidiActionManager->handleAction(pMidiMap->getMMCAction("MMC_STOP"));
				break;
			}

			case 2:	// PLAY
			{
				pEngine->lastMidiEvent = "MMC_PLAY";
				pMidiActionManager->handleAction(pMidiMap->getMMCAction("MMC_PLAY"));
				break;
			}

			case 3:	//DEFERRED PLAY
			{
				pEngine->lastMidiEvent = "MMC_PLAY";
				pMidiActionManager->handleAction(pMidiMap->getMMCAction("MMC_PLAY"));
				break;
			}

			case 4:	// FAST FWD
				pEngine->lastMidiEvent = "MMC_FAST_FORWARD";
				pMidiActionManager->handleAction(pMidiMap->getMMCAction("MMC_FAST_FORWARD"));
				break;

			case 5:	// REWIND
				pEngine->lastMidiEvent = "MMC_REWIND";
				pMidiActionManager->handleAction(pMidiMap->getMMCAction("MMC_REWIND"));
				break;

			case 6:	// RECORD STROBE (PUNCH IN)
				pEngine->lastMidiEvent = "MMC_RECORD_STROBE";
				pMidiActionManager->handleAction(pMidiMap->getMMCAction("MMC_RECORD_STROBE"));
				break;

			case 7:	// RECORD EXIT (PUNCH OUT)
				pEngine->lastMidiEvent = "MMC_RECORD_EXIT";
				pMidiActionManager->handleAction(pMidiMap->getMMCAction("MMC_RECORD_EXIT"));
				break;

			case 8:	// RECORD READY
				pEngine->lastMidiEvent = "MMC_RECORD_READY";
				pMidiActionManager->handleAction(pMidiMap->getMMCAction("MMC_RECORD_READY"));
				break;

			case 9:	//PAUSE
				pEngine->lastMidiEvent = "MMC_PAUSE";
				pMidiActionManager->handleAction(pMidiMap->getMMCAction("MMC_PAUSE"));
				break;

			default:
				WARNINGLOG( "Unknown MMC Command" );
//					midiDump( buf, nBytes );
			}
		}
	} else if ( msg.m_sysexData.size() == 13 ) {
		ERRORLOG( "MMC GOTO Message not implemented yet" );
//		midiDump( buf, nBytes );
		//int id = buf[2];
		int hr = msg.m_sysexData[7];
		int mn = msg.m_sysexData[8];
		int sc = msg.m_sysexData[9];
		int fr = msg.m_sysexData[10];
		int ff = msg.m_sysexData[11];
		char tmp[200];
		sprintf( tmp, "[handleSysexMessage] GOTO %d:%d:%d:%d:%d", hr, mn, sc, fr, ff );
		INFOLOG( tmp );

	} else {
		// sysex dump
		QString sDump;
		char tmpChar[64];
		for ( int i = 0; i < ( int )msg.m_sysexData.size(); ++i ) {
			sprintf( tmpChar, "%X ", ( int )msg.m_sysexData[ i ] );
			sDump += tmpChar;
		}
		WARNINGLOG( QString( "Unknown SysEx message: (%1) [%2]" ).arg( msg.m_sysexData.size() ).arg( sDump ) );
	}
}

};
