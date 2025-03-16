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

#ifndef SMF_H
#define SMF_H

#include <core/Object.h>

#include <memory>
#include <string>
#include <vector>
#include <QByteArray>

#include <core/SMF/SMFEvent.h>

namespace H2Core
{

class Song;
class Instrument;

/** \ingroup docCore docMIDI */
class SMFHeader : public Object<SMFHeader>, public SMFBase
{
	H2_OBJECT(SMFHeader)
public:
	/** For the MIDI file specs:
	 *
	 * The first word, <format>, specifies the overall organization of the file.
	 * Only three values of <format> are specified:
	 * - 0 the file contains a single multi-channel track
	 * - 1 the file contains one or more simultaneous tracks (or MIDI outputs)
	 *   of a sequence
	 * - 2 the file contains one or more sequentially independent single-track
	 * patterns */
	enum class Format {
		SingleMultiChannelTrack = 0,
		SimultaneousTracks = 1,
		SequentialIndependentTracks = 2
	};
		static QString FormatToQString( Format format );

	SMFHeader( Format format );
	~SMFHeader();
	
	void addTrack();
	virtual QByteArray getBuffer() const override;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
	Format m_format;		///< SMF format
	int m_nTracks;		///< number of tracks
};



/** \ingroup docCore docMIDI */
class SMFTrack : public Object<SMFTrack>, public SMFBase
{
	H2_OBJECT(SMFTrack)
public:

	SMFTrack();
	~SMFTrack();

	void addEvent( std::shared_ptr<SMFEvent> pEvent );

	virtual QByteArray getBuffer() const override;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
	std::vector< std::shared_ptr<SMFEvent> > m_eventList;
};



/** \ingroup docCore docMIDI */
class SMF : public Object<SMF>, public SMFBase
{
	H2_OBJECT(SMF)
public:

	/** Scaling of how much larger in the resolution used for MIDI files
	 * compared to the tick size in the #H2Core::AudioEngine. */
	static constexpr int nTickFactor = 4;
	static constexpr int nTicksPerQuarter =
		H2Core::nTicksPerQuarter * SMF::nTickFactor;

	SMF( SMFHeader::Format format );
	~SMF();

	void addTrack( std::shared_ptr<SMFTrack> pTrack );
	virtual QByteArray getBuffer() const override;

		QString bufferToQString() const;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
	std::vector< std::shared_ptr<SMFTrack> > m_trackList;

	std::shared_ptr<SMFHeader> m_pHeader;
};



typedef std::vector< std::shared_ptr<SMFEvent> > EventList;


/** \ingroup docCore docMIDI */
class SMFWriter : public H2Core::Object<SMFWriter>
{
	H2_OBJECT(SMFWriter)
public:
	SMFWriter();
	virtual ~SMFWriter();
	void save( const QString& sFilename, std::shared_ptr<Song> pSong );

protected:
	std::shared_ptr<SMFTrack> createTrack0( std::shared_ptr<Song> pSong );
	
	virtual std::shared_ptr<SMF> createSMF( std::shared_ptr<Song> pSong ) = 0;
	virtual void prepareEvents( std::shared_ptr<Song> pSong,
								std::shared_ptr<SMF> pSmf )=0;
	virtual std::shared_ptr<EventList> getEvents( std::shared_ptr<Song> pSong,
												  std::shared_ptr<Instrument> pInstr ) = 0;
	virtual void packEvents( std::shared_ptr<Song> pSong,
							  std::shared_ptr<SMF> pSmf ) = 0;
	
private:
	void saveSMF( const QString& sFilename, std::shared_ptr<SMF> pSmf );
};


//-------

/** \ingroup docCore docMIDI */
class SMF1Writer : public Object<SMF1Writer>, public SMFWriter
{
    H2_OBJECT(SMF1Writer)
public:
    SMF1Writer();
	virtual ~SMF1Writer();
protected:
	virtual std::shared_ptr<SMF> createSMF( std::shared_ptr<Song> pSong ) override;
};


/** \ingroup docCore docMIDI */
class SMF1WriterSingle : public Object<SMF1WriterSingle>, public SMF1Writer
{
    H2_OBJECT(SMF1WriterSingle)
public:
    SMF1WriterSingle();
	virtual ~SMF1WriterSingle();
protected:
	virtual void prepareEvents( std::shared_ptr<Song> pSong,
								std::shared_ptr<SMF> pSmf ) override;
	virtual void packEvents( std::shared_ptr<Song> pSong,
							 std::shared_ptr<SMF> pSmf ) override;
	virtual std::shared_ptr<EventList> getEvents( std::shared_ptr<Song> pSong,
												  std::shared_ptr<Instrument> pInstr ) override;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
	std::shared_ptr<EventList> m_pEventList;
};


/** \ingroup docCore docMIDI */
class SMF1WriterMulti : public Object<SMF1WriterMulti>, public SMF1Writer
{
    H2_OBJECT(SMF1WriterMulti)
public:
    SMF1WriterMulti();
	virtual ~SMF1WriterMulti();
protected:
	virtual void prepareEvents( std::shared_ptr<Song> pSong,
								std::shared_ptr<SMF> pSmf ) override;
	virtual void packEvents( std::shared_ptr<Song> pSong,
							  std::shared_ptr<SMF> pSmf ) override;
	virtual std::shared_ptr<EventList> getEvents( std::shared_ptr<Song> pSong,
												  std::shared_ptr<Instrument> pInstr ) override;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
	// contains events for each instrument in separate vector
	std::vector< std::shared_ptr<EventList> > m_eventLists;
};


//-------

/** \ingroup docCore docMIDI */
class SMF0Writer : public Object<SMF0Writer>, public SMFWriter
{
    H2_OBJECT(SMF0Writer)
public:
    SMF0Writer();
	virtual ~SMF0Writer();
protected:
	virtual std::shared_ptr<SMF> createSMF( std::shared_ptr<Song> pSong ) override;
	virtual void prepareEvents( std::shared_ptr<Song> pSong,
								std::shared_ptr<SMF> pSmf ) override;
	virtual void packEvents( std::shared_ptr<Song> pSong,
							  std::shared_ptr<SMF> pSmf ) override;
	virtual std::shared_ptr<EventList> getEvents( std::shared_ptr<Song> pSong,
												  std::shared_ptr<Instrument> pInstr ) override;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
	std::shared_ptr<SMFTrack> m_pTrack;
	std::shared_ptr<EventList> m_pEventList;
};



};

#endif

