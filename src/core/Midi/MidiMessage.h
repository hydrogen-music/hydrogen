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

#ifndef MIDI_MESSAGE_H
#define MIDI_MESSAGE_H

#include <core/config.h>
#include <core/Object.h>
#include <string>
#include <vector>

#include <QString>

namespace H2Core
{

class Note;

/** \ingroup docCore docMIDI */
class MidiMessage
{
	public:
		/** All possible types of incoming MIDI messages.*/
		enum class Type {
			Unknown,
			Sysex,
			NoteOn,
			NoteOff,
			PolyphonicKeyPressure,
			ControlChange,
			ProgramChange,
			ChannelPressure,
			PitchWheel,
			Start,
			Continue,
			Stop,
			SongPos,
			QuarterFrame,
			SongSelect,
			TuneRequest,
			TimingClock,
			ActiveSensing,
			Reset
		};
		static QString TypeToQString( Type type );

		/** Subset of incoming MIDI events that will be handled by
			Hydrogen. */
		enum class Event {
			Null,
			Note,
			CC,
			PC,
			MmcStop,
			MmcPlay,
			MmcPause,
			MmcDeferredPlay,
			MmcFastForward,
			MmcRewind,
			MmcRecordStrobe,
			MmcRecordExit,
			MmcRecordReady
		};
		static QString EventToQString( const Event& event );
		static Event QStringToEvent( const QString& sEvent );
		/** Retrieve the string representation for all available
		 * #Event. */
		static QStringList getEventList();

		/** When recording notes using MIDI NOTE_ON events this offset will be
		 * applied to the provided pitch in order to map it to an instrument
		 * number in the current drmmkit. It corresponds to the electric bass
		 * drum in the General MIDI notation. */
		static constexpr int instrumentOffset = 36;

		Type m_type;
		int m_nData1;
		int m_nData2;
		int m_nChannel;
		std::vector<unsigned char> m_sysexData;

		MidiMessage()
			: m_type( Type::Unknown )
			, m_nData1( -1 )
			, m_nData2( -1 )
			, m_nChannel( -1 ) {}

		/** Reset message */
		void clear();

		static MidiMessage from( std::shared_ptr<Note> pNote );

		/**
		 * Derives and set #m_type (and if applicable #m_nChannel) using the @a
		 * statusByte of an incoming MIDI message. The particular values are
		 * defined by the MIDI standard and do not dependent on the individual
		 * drivers.
		 */
		void setType( int nStatusByte );

		/** Formatted string version for debugging purposes.
		 * \param sPrefix String prefix which will be added in front of
		 *   every new line
		 * \param bShort Instead of the whole content of all classes
		 *   stored as members just a single unique identifier will be
		 *   displayed without line breaks.
		 *
		 * \return String presentation of current object.*/
		QString toQString( const QString& sPrefix = "", bool bShort = true ) const;
};
};

#endif

