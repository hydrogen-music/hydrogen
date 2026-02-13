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
#include <core/Midi/Midi.h>
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

		/** Helper to construct ControlChange MIDI messages. */
		struct ControlChange {
			Midi::Parameter parameter;
			Midi::Parameter value;
			Midi::Channel channel;
		};

		/** Helper to construct NoteOff MIDI messages. */
		struct NoteOff {
			Midi::Note note;
			Midi::Parameter velocity;
			Midi::Channel channel;
		};

		MidiMessage();
		MidiMessage(
			Type type,
			Midi::Parameter data1,
			Midi::Parameter data2,
			Midi::Channel channel
		);

		/** Reset message */
		void clear();

		/** Derives the channel associated to the incoming MIDI message (if
		 * applicable #m_nChannel). The particular values are defined by the
		 * MIDI standard and do not dependent on the individual drivers. */
		static Midi::Channel deriveChannel( int nStatusByte );
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

		Midi::Parameter getData1() const;
		void setData1( Midi::Parameter data1 );

		Midi::Parameter getData2() const;
		void setData2( Midi::Parameter data2 );

		Midi::Channel getChannel() const;
		void setChannel( Midi::Channel channel );

		std::vector<unsigned char> getSysexData() const;
		void setSysexData( std::vector<unsigned char> data );
		void appendToSysexData( unsigned char newData );

		int getFrameOffset() const;
		void setFrameOffset( int nFrameOffset );

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
		Midi::Parameter m_data1;
		Midi::Parameter m_data2;
		Midi::Channel m_channel;
		std::vector<unsigned char> m_sysexData;

        /** Specifies the onset of the message relative to m_timePoint.
         *
         * Audio in Hydrogen is processed in separate batches of frames.
         * Especially for large buffer sizes we could end up with a noticable
         * deviation between MIDI and audio output when always sending MIDI
         * notes at a single time per process cycle and allowing the audio to
         * start e.g. at the end of the current batch of frames. This member is
         * meant to compensate for such deviations. */
        int m_nFrameOffset;

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
inline Midi::Parameter MidiMessage::getData1() const {
	return m_data1;
}
inline void MidiMessage::setData1( Midi::Parameter data1 ) {
	m_data1 = data1;
}
inline Midi::Parameter MidiMessage::getData2() const {
	return m_data2;
}
inline void MidiMessage::setData2( Midi::Parameter data2 ) {
	m_data2 = data2;
}
inline Midi::Channel MidiMessage::getChannel() const {
	return m_channel;
}
inline void MidiMessage::setChannel( Midi::Channel channel ) {
	m_channel = channel;
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
inline int MidiMessage::getFrameOffset() const {
	return m_nFrameOffset;
}
inline void MidiMessage::setFrameOffset( int nFrameOffset ) {
	m_nFrameOffset = nFrameOffset;
}

};

#endif

