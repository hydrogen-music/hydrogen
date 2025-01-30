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



enum SMFEventType {
	NOTE_OFF = 128,
	NOTE_ON = 144
};



enum SMFMetaEventType {
	SEQUENCE_NUMBER = 0,
	TEXT_EVENT,
	COPYRIGHT_NOTICE,
	TRACK_NAME,
	INSTRUMENT_NAME,
	LYRIC,
	MARKER,
	CUE_POINT,
	END_OF_TRACK = 0x2f,
	SET_TEMPO = 0x51,
	TIME_SIGNATURE = 0x58,
	KEY_SIGNATURE
};


/** \ingroup docCore docMIDI */
class SMFBase
{
public:
	virtual ~SMFBase() {}
	virtual QByteArray getBuffer() const = 0;
	virtual QString toQString() const = 0;
};



/** \ingroup docCore docMIDI */
class SMFEvent : public SMFBase, public H2Core::Object<SMFEvent>
{
	H2_OBJECT(SMFEvent)
public:
	SMFEvent(unsigned nTicks );
	virtual ~SMFEvent();

	virtual QString toQString() const override;
	int m_nTicks;
	int m_nDeltaTime;
};



/** \ingroup docCore docMIDI */
class SMFTrackNameMetaEvent : public SMFEvent, public H2Core::Object<SMFTrackNameMetaEvent>
{
	H2_OBJECT(SMFTrackNameMetaEvent)
public:
	SMFTrackNameMetaEvent( const QString& sTrackName, unsigned nDeltaTime );
	virtual QByteArray getBuffer() const override;

private:
	QString m_sTrackName;

};



/** \ingroup docCore docMIDI */
class SMFSetTempoMetaEvent : public SMFEvent, public H2Core::Object<SMFSetTempoMetaEvent>
{
	H2_OBJECT(SMFSetTempoMetaEvent)
public:
	SMFSetTempoMetaEvent( float fBPM, unsigned nDeltaTime );
	virtual QByteArray getBuffer() const override;

private:
	unsigned m_fBPM;

};



/** \ingroup docCore docMIDI */
class SMFCopyRightNoticeMetaEvent : public SMFEvent, public H2Core::Object<SMFCopyRightNoticeMetaEvent>
{
	H2_OBJECT(SMFCopyRightNoticeMetaEvent)
public:
	SMFCopyRightNoticeMetaEvent( const QString& sAuthor, unsigned nDeltaTime );
	virtual QByteArray getBuffer() const override;

private:
	QString m_sAuthor;

};



/** \ingroup docCore docMIDI */
class SMFTimeSignatureMetaEvent : public SMFEvent, public H2Core::Object<SMFTimeSignatureMetaEvent>
{
	H2_OBJECT(SMFTimeSignatureMetaEvent)
public:
	SMFTimeSignatureMetaEvent( unsigned nBeats, unsigned nNote , unsigned nMTPMC , unsigned nTSNP24 , unsigned nTicks );
	virtual QByteArray getBuffer() const override;
	// MTPMC = MIDI ticks per metronome click
	// TSNP24 = Thirty Second Notes Per 24 MIDI Ticks.
private:
	unsigned m_nBeats, m_nNote, m_nMTPMC , m_nTSNP24 , m_nTicks;
};



/** \ingroup docCore docMIDI */
class SMFNoteOnEvent : public SMFEvent, public H2Core::Object<SMFNoteOnEvent>
{
	H2_OBJECT(SMFNoteOnEvent)
public:
	SMFNoteOnEvent( unsigned nTicks, int nChannel, int nPitch, int nVelocity );

	virtual QByteArray getBuffer() const override;

protected:
	unsigned m_nChannel;
	unsigned m_nPitch;
	unsigned m_nVelocity;
};



/** \ingroup docCore docMIDI */
class SMFNoteOffEvent : public SMFEvent, public H2Core::Object<SMFNoteOffEvent>
{
	H2_OBJECT(SMFNoteOffEvent)
public:
	SMFNoteOffEvent(  unsigned nTicks, int nChannel, int nPitch, int nVelocity );

	virtual QByteArray getBuffer() const override;

protected:
	unsigned m_nChannel;
	unsigned m_nPitch;
	unsigned m_nVelocity;

};

};

#endif

