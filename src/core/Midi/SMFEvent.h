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

#ifndef SMF_EVENT_H
#define SMF_EVENT_H

#include <QByteArray>
#include <QString>

#include <core/Midi/Midi.h>
#include <core/Object.h>

namespace H2Core
{

/** \ingroup docCore docMIDI */
class SMFBuffer : public H2Core::Object<SMFBuffer>
{
	H2_OBJECT(SMFBuffer)
public:
	QByteArray getBuffer() {
		return m_buffer;
	}

	void writeByte( char nByte );
	void writeWord( int nVal );
	void writeDWord( long nVal );
	void writeString( const QString& sMsg );
	void writeVarLen( long nVal );

	QByteArray m_buffer;

	SMFBuffer();
};



/** \ingroup docCore docMIDI */
class SMFBase
{
public:
	virtual ~SMFBase() {}
	virtual QByteArray getBuffer() const = 0;
};



/** \ingroup docCore docMIDI */
class SMFEvent : public SMFBase, public H2Core::Object<SMFEvent>
{
	H2_OBJECT(SMFEvent)
public:

	enum class Type {
		CopyrightNotice = 2,
		CuePoint = 7,
		EndOfTrack = 0x2f,
		InstrumentName = 4,
		KeySignature = 0x59,
		Lyric = 5,
		Marker = 6,
		NoteOff = 128,
		NoteOn = 144,
		SequenceNumber = 0,
		SetTempo = 0x51,
		TextEvent = 1,
		TimeSignature = 0x58,
		TrackName = 3,
	};
		static QString TypeToQString( Type type );
		static bool IsMetaEvent( Type type );

	SMFEvent( float fTicks, SMFEvent::Type type );
	virtual ~SMFEvent();

	/** The ticks used in MIDI files are SMF::nTickFactor smaller than the ones
	 * using in Hydrogen itself. To both not require to multiply every tick by
	 * this factor - quite error-prone - and to harness maximum resolution when
	 * humanizing the onset of a note, we will provide its tick in a float. This
	 * allows us to use e.g. `11.25` with a `SMF::nTickFactor` of `4`. */
	float m_fTicks;
	int m_nDeltaTime;

		SMFEvent::Type m_type;
};



/** \ingroup docCore docMIDI */
class SMFTrackNameMetaEvent : public SMFEvent, public H2Core::Object<SMFTrackNameMetaEvent>
{
	H2_OBJECT(SMFTrackNameMetaEvent)
public:
	SMFTrackNameMetaEvent( const QString& sTrackName, float fTicks );
	virtual QByteArray getBuffer() const override;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
	QString m_sTrackName;

};

/** \ingroup docCore docMIDI */
class SMFSetTempoMetaEvent : public SMFEvent, public H2Core::Object<SMFSetTempoMetaEvent>
{
	H2_OBJECT(SMFSetTempoMetaEvent)
public:
	SMFSetTempoMetaEvent( int nBPM, float fTicks );
	virtual QByteArray getBuffer() const override;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
	int m_nBPM;

};

/** \ingroup docCore docMIDI */
class SMFMarkerMetaEvent : public SMFEvent, public H2Core::Object<SMFMarkerMetaEvent>
{
	H2_OBJECT(SMFMarkerMetaEvent)
public:
	SMFMarkerMetaEvent( const QString& sText, float fTicks );
	virtual QByteArray getBuffer() const override;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
	QString m_sText;

};

/** \ingroup docCore docMIDI */
class SMFCopyRightNoticeMetaEvent : public SMFEvent, public H2Core::Object<SMFCopyRightNoticeMetaEvent>
{
	H2_OBJECT(SMFCopyRightNoticeMetaEvent)
public:
	SMFCopyRightNoticeMetaEvent( const QString& sAuthor, float fTicks );
	virtual QByteArray getBuffer() const override;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
	QString m_sAuthor;

};



/** \ingroup docCore docMIDI */
class SMFTimeSignatureMetaEvent : public SMFEvent, public H2Core::Object<SMFTimeSignatureMetaEvent>
{
	H2_OBJECT(SMFTimeSignatureMetaEvent)
public:
	SMFTimeSignatureMetaEvent( int nNumerator, int nDenominator, float fTicks );
	virtual QByteArray getBuffer() const override;
	// MTPMC = MIDI ticks per metronome click
	// TSNP24 = Thirty Second Notes Per 24 MIDI Ticks.

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
	int m_nNumerator, m_nDenominator, m_nMTPMC, m_nTSNP24;
};



/** \ingroup docCore docMIDI */
class SMFNoteOnEvent : public SMFEvent, public H2Core::Object<SMFNoteOnEvent>
{
	H2_OBJECT(SMFNoteOnEvent)
public:
 SMFNoteOnEvent(
	 float fTicks,
	 Midi::Channel Channel,
	 Midi::Note note,
	 int nVelocity
 );

 virtual QByteArray getBuffer() const override;

 QString toQString( const QString& sPrefix = "", bool bShort = true )
	 const override;

protected:
	Midi::Channel m_channel;
	Midi::Note m_note;
	int m_nVelocity;
};



/** \ingroup docCore docMIDI */
class SMFNoteOffEvent : public SMFEvent, public H2Core::Object<SMFNoteOffEvent>
{
	H2_OBJECT(SMFNoteOffEvent)
public:
 SMFNoteOffEvent(
	 float fTicks,
	 Midi::Channel channel,
	 Midi::Note note,
	 int nVelocity
 );

 virtual QByteArray getBuffer() const override;

 QString toQString( const QString& sPrefix = "", bool bShort = true )
	 const override;

protected:
	Midi::Channel m_channel;
	Midi::Note m_note;
	int m_nVelocity;

};

};

#endif

