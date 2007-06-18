/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: MidiDriver.cpp,v 1.7 2005/06/14 13:54:06 comix Exp $
 *
 */

#include "MidiDriver.h"
#include "lib/EventQueue.h"
#include "lib/Preferences.h"
#include "lib/Hydrogen.h"

MidiDriver::MidiDriver( std::string sDriverName )
 : Object( sDriverName )
 , m_bActive( false )
{
	infoLog( "INIT" );
}


MidiDriver::~MidiDriver()
{
	infoLog( "DESTROY" );
}

void MidiDriver::handleMidiMessage(const MidiMessage& msg)
{
	EventQueue::getInstance()->pushEvent( EVENT_MIDI_ACTIVITY, -1 );

//	infoLog( "[handleMidiMessage]" );
//	infoLog( "[handleMidiMessage] channel: " + toString( msg.m_nChannel ) );
//	infoLog( "[handleMidiMessage] val1: " + toString( msg.m_nData1 ) );
//	infoLog( "[handleMidiMessage] val2: " + toString( msg.m_nData2 ) );

	switch ( msg.m_type ) {
		case MidiMessage::NOTE_ON:
			handleNoteOnMessage( msg );
			break;

		case MidiMessage::NOTE_OFF:
			handleNoteOffMessage( msg );
			break;

		case MidiMessage::POLYPHONIC_KEY_PRESSURE:
			errorLog( "[handleMidiMessage] POLYPHONIC_KEY_PRESSURE event not handled yet" );
			break;

		case MidiMessage::CONTROL_CHANGE:
			errorLog( "[handleMidiMessage] CONTROL_CHANGE event not handled yet" );
			break;

		case MidiMessage::PROGRAM_CHANGE:
			errorLog( "[handleMidiMessage] PROGRAM_CHANGE event not handled yet" );
			break;

		case MidiMessage::CHANNEL_PRESSURE:
			errorLog( "[handleMidiMessage] CHANNEL_PRESSURE event not handled yet" );
			break;

		case MidiMessage::PITCH_WHEEL:
			errorLog( "[handleMidiMessage] PITCH_WHEEL event not handled yet" );
			break;

		case MidiMessage::SYSTEM_EXCLUSIVE:
			errorLog( "[handleMidiMessage] SYSTEM_EXCLUSIVE event not handled yet" );
			break;

		case MidiMessage::START:
			infoLog( "[handleMidiMessage] START event" );
			if ( Hydrogen::getInstance()->getState() != STATE_PLAYING ) {
				Hydrogen::getInstance()->start();
			}
			break;

		case MidiMessage::CONTINUE:
			errorLog( "[handleMidiMessage] CONTINUE event not handled yet" );
			break;

		case MidiMessage::STOP:
			infoLog( "[handleMidiMessage] STOP event" );
			if ( Hydrogen::getInstance()->getState() == STATE_PLAYING ) {
				Hydrogen::getInstance()->stop();
			}
			break;

		case MidiMessage::SONG_POS:
			errorLog( "[handleMidiMessage] SONG_POS event not handled yet" );
			break;

		default:
			errorLog( "[handleMidiMessage] unhandled midi message type" );
	}
}



void MidiDriver::handleNoteOnMessage( const MidiMessage& msg )
{
	infoLog( "[handleNoteOnMessage]" );

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
			infoLog( "[handleNoteOnMessage] next pattern = " + toString(patternNumber) );

			pEngine->setNextPattern( patternNumber );
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



void MidiDriver::handleNoteOffMessage( const MidiMessage& msg )
{
	infoLog( "[handleNoteOffMessage]" );
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
	const float fPan_L = 1.0f;
	const float fPan_R = 1.0f;
	const int nLenght = -1;
	const float fPitch = 0.0f;
	Note *pNewNote = new Note( pInstr, nPosition, fVelocity, fPan_L, fPan_R, nLenght, fPitch );

	pEngine->noteOff( pNewNote );
}



