/*
 * Hydrogen
 * Copyright(c) 2002-2007 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <hydrogen/IO/MidiInput.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/instrument.h>
#include <hydrogen/note.h>

namespace H2Core {

MidiInput::MidiInput( const std::string sClassName )
 : Object( sClassName )
 , m_bActive( false )
{
	//INFOLOG( "INIT" );
}


MidiInput::~MidiInput()
{
	//INFOLOG( "DESTROY" );
}

void MidiInput::handleMidiMessage(const MidiMessage& msg)
{
	EventQueue::getInstance()->pushEvent( EVENT_MIDI_ACTIVITY, -1 );

//	infoLog( "[handleMidiMessage]" );
//	infoLog( "[handleMidiMessage] channel: " + toString( msg.m_nChannel ) );
//	infoLog( "[handleMidiMessage] val1: " + toString( msg.m_nData1 ) );
//	infoLog( "[handleMidiMessage] val2: " + toString( msg.m_nData2 ) );

	switch ( msg.m_type ) {
		case MidiMessage::SYSEX:
			handleSysexMessage( msg );
			break;

		case MidiMessage::NOTE_ON:
			handleNoteOnMessage( msg );
			break;

		case MidiMessage::NOTE_OFF:
			handleNoteOffMessage( msg );
			break;

		case MidiMessage::POLYPHONIC_KEY_PRESSURE:
			ERRORLOG( "POLYPHONIC_KEY_PRESSURE event not handled yet" );
			break;

		case MidiMessage::CONTROL_CHANGE:
			ERRORLOG( "CONTROL_CHANGE event not handled yet" );
			break;

		case MidiMessage::PROGRAM_CHANGE:
			ERRORLOG( "PROGRAM_CHANGE event not handled yet" );
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

		case MidiMessage::START:
			INFOLOG( "START event" );
			if ( Hydrogen::getInstance()->getState() != STATE_PLAYING ) {
				Hydrogen::getInstance()->sequencer_play();
			}
			break;

		case MidiMessage::CONTINUE:
			ERRORLOG( "CONTINUE event not handled yet" );
			break;

		case MidiMessage::STOP:
			INFOLOG( "STOP event" );
			if ( Hydrogen::getInstance()->getState() == STATE_PLAYING ) {
				Hydrogen::getInstance()->sequencer_stop();
			}
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
			ERRORLOG( "unhandled midi message type: " + toString( msg.m_type ) );
	}
}



void MidiInput::handleNoteOnMessage( const MidiMessage& msg )
{
	INFOLOG( "handleNoteOnMessage" );

	int nMidiChannelFilter = Preferences::getInstance()->m_nMidiChannelFilter;
	int nChannel = msg.m_nChannel;
	int nNote = msg.m_nData1;
	float fVelocity = msg.m_nData2 / 127.0;

	if ( fVelocity == 0 ) {
		handleNoteOffMessage( msg );
		return;
	}

	bool bIsChannelValid = true;
	if ( nMidiChannelFilter != -1 ) {
		bIsChannelValid = ( nChannel == nMidiChannelFilter );
	}

	Hydrogen *pEngine = Hydrogen::getInstance();

	bool bPatternSelect = false;

	if ( bIsChannelValid ) {
		if ( bPatternSelect ) {
			int patternNumber = nNote - 36;
			INFOLOG( "next pattern = " + toString(patternNumber) );

			pEngine->sequencer_setNextPattern( patternNumber, false, false );
		}
		else {
			static const float fPan_L = 1.0f;
			static const float fPan_R = 1.0f;

			int nInstrument = nNote - 36;
			if ( nInstrument < 0 ) {
				nInstrument = 0;
			}
			if ( nInstrument > (MAX_INSTRUMENTS -1 ) ) {
				nInstrument = MAX_INSTRUMENTS - 1;
			}

			pEngine->addRealtimeNote( nInstrument, fVelocity, fPan_L, fPan_R, 0.0, true);
		}
	}
}



void MidiInput::handleNoteOffMessage( const MidiMessage& msg )
{
	INFOLOG( "handleNoteOffMessage" );
	if ( Preferences::getInstance()->m_bMidiNoteOffIgnore ) {
		return;
	}

	Hydrogen *pEngine = Hydrogen::getInstance();
	Song *pSong = pEngine->getSong();

	int nNote = msg.m_nData1;
	int nInstrument = nNote - 36;
	if ( nInstrument < 0 ) {
		nInstrument = 0;
	}
	if ( nInstrument > (MAX_INSTRUMENTS -1 ) ) {
		nInstrument = MAX_INSTRUMENTS - 1;
	}
	Instrument *pInstr = ( pSong->getInstrumentList() )->get( nInstrument );
	const unsigned nPosition = 0;
	const float fVelocity = 0.0f;
	const float fPan_L = 0.5f;
	const float fPan_R = 0.5f;
	const int nLenght = -1;
	const float fPitch = 0.0f;
	Note *pNewNote = new Note( pInstr, nPosition, fVelocity, fPan_L, fPan_R, nLenght, fPitch );

	pEngine->midi_noteOff( pNewNote );
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
		9	Pause


		Goto MMC message
		0	1	2	3	4	5	6	7	8	9	10	11	12
		240	127	id	6	68	6	1	hr	mn	sc	fr	ff	247
	*/


	if ( msg.m_sysexData.size() == 6 ) {
		if (
				( msg.m_sysexData[0] == 0xF0 ) &&
				( msg.m_sysexData[1] == 127 ) &&
				( msg.m_sysexData[2] == 127 ) &&
				( msg.m_sysexData[3] == 6 ) )
		{
			switch (msg.m_sysexData[4] ) {
				case 1:	// STOP
					WARNINGLOG( "MMC STOP not implemented yet" );
					break;

				case 2:	// PLAY
					WARNINGLOG( "MMC PLAY not implemented yet." );
					break;

				case 3:	//DEFERRED PLAY
					WARNINGLOG( "MMC DEFERRED PLAY not implemented yet." );
					break;

				case 4:	// FAST FWD
					WARNINGLOG( "MMC FAST FWD not implemented yet." );
					break;

				case 5:	// REWIND
					WARNINGLOG( "MMC REWIND not implemented yet." );
					break;

				case 6:	// RECORD STROBE (PUNCH IN)
					WARNINGLOG( "MMC PUNCH IN not implemented yet." );
					break;

				case 7:	// RECORD EXIT (PUNCH OUT)
					WARNINGLOG( "MMC PUNCH OUT not implemented yet." );
					break;

				case 9:	//PAUSE
					WARNINGLOG( "MMC PAUSE not implemented yet." );
					break;

				default:
					WARNINGLOG( "Unknown MMC Command" );
//					midiDump( buf, nBytes );
			}
		}
	}
	else if ( msg.m_sysexData.size() == 13 ) {
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

	}
	else {
		// sysex dump
		string sDump = "";
		char tmpChar[64];
		for ( int i = 0; i < (int)msg.m_sysexData.size(); ++i) {
			sprintf( tmpChar, "%X ", (int)msg.m_sysexData[ i ] );
			sDump += tmpChar;
		}
		WARNINGLOG( "Unknown SysEx message: " + string( "(" ) + toString( msg.m_sysexData.size() ) + string( ") [ " ) + sDump + string( " ]" ));
	}

}

};


