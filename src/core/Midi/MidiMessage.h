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
#include <core/Helpers/Time.h>
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

	static constexpr int nChannelMinimum = 0;
	static constexpr int nChannelDefault = 9;
	static constexpr int nChannelMaximum = 15;
	/** Used to indicate "off" - that no channel and MIDI events will be
         associated to an instrument - in the channel spin box. Must be set to a
         value outside the channel min-max range. */
	static constexpr int nChannelOff = -1;

	static constexpr int nNoteMinimum = 0;
	static constexpr int nNoteDefault = 36;
	static constexpr int nNoteMaximum = 127;

		/** All possible types of incoming MIDI messages.*/
		enum class Type {
			ActiveSensing,
			ChannelPressure,
			Continue,
			ControlChange,
			NoteOff,
			NoteOn,
			PitchWheel,
			PolyphonicKeyPressure,
			ProgramChange,
			Reset,
			QuarterFrame,
			SongPos,
			SongSelect,
			Start,
			Stop,
			Sysex,
			TimingClock,
			TuneRequest,
			Unknown
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

		/** Helper to construct ControlChange MIDI messages. */
		struct ControlChange {
			int nParameter;
			int nValue;
			int nChannel;
		};

		/** Helper to construct NoteOff MIDI messages. */
		struct NoteOff {
			int nKey;
			int nVelocity;
			int nChannel;
		};

		MidiMessage();
		MidiMessage( Type type, int nData1, int Data2, int nChannel );

		/** Reset message */
		void clear();

		/** Derives the channel associated to the incoming MIDI message (if
		 * applicable #m_nChannel). The particular values are defined by the
		 * MIDI standard and do not dependent on the individual drivers. */
		static int deriveChannel( int nStatusByte );
		/** Derives the type of the incoming MIDI message. The particular values
		 * are defined by the MIDI standard and do not dependent on the
		 * individual drivers. */
		static Type deriveType( int nStatusByte );

		static MidiMessage from( const MidiMessage& msg );
		static MidiMessage from( const ControlChange& controlChange );
		static MidiMessage from( std::shared_ptr<Note> pNote );
		static MidiMessage from( const NoteOff& noteOff );

		const TimePoint& getTimePoint() const;

		Type getType() const;
		void setType( Type type );

		int getData1() const;
		void setData1( int nData1 );

		int getData2() const;
		void setData2( int nData2 );

		int getChannel() const;
		void setChannel( int Channel );

		std::vector<unsigned char> getSysexData() const;
		void setSysexData( std::vector<unsigned char> data );
		void appendToSysexData( unsigned char newData );

		bool operator==( const MidiMessage& other ) const;
		bool operator!=( const MidiMessage& other ) const;

		/** Formatted string version for debugging purposes.
		 * \param sPrefix String prefix which will be added in front of
		 *   every new line
		 * \param bShort Instead of the whole content of all classes
		 *   stored as members just a single unique identifier will be
		 *   displayed without line breaks.
		 *
		 * \return String presentation of current object.*/
		QString toQString( const QString& sPrefix = "", bool bShort = true ) const;

	private:
		TimePoint m_timePoint;
		Type m_type;
		int m_nData1;
		int m_nData2;
		int m_nChannel;
		std::vector<unsigned char> m_sysexData;

};

inline const TimePoint& MidiMessage::getTimePoint() const {
	return m_timePoint;
}
inline MidiMessage::Type MidiMessage::getType() const {
	return m_type;
}
inline void MidiMessage::setType( MidiMessage::Type type ) {
	m_type = type;
}
inline int MidiMessage::getData1() const {
	return m_nData1;
}
inline void MidiMessage::setData1( int nData1 ) {
	m_nData1 = nData1;
}
inline int MidiMessage::getData2() const {
	return m_nData2;
}
inline void MidiMessage::setData2( int nData2 ) {
	m_nData2 = nData2;
}
inline int MidiMessage::getChannel() const {
	return m_nChannel;
}
inline void MidiMessage::setChannel( int nChannel ) {
	m_nChannel = nChannel;
}
inline std::vector<unsigned char> MidiMessage::getSysexData() const {
	return m_sysexData;
}
inline void MidiMessage::setSysexData( std::vector<unsigned char> sysexData ) {
	m_sysexData = sysexData;
}
inline void MidiMessage::appendToSysexData( unsigned char newData ) {
	m_sysexData.push_back( newData );
}

};

#endif

