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

class Instrument;
class Pattern;
class Song;

typedef std::vector< std::shared_ptr<SMFEvent> > EventList;

struct SMFTimeSignatureFailure {
	int nColumn;
	/** The numerator within Hydrogen is a floating point number */
	float fOldNumerator;
	/** The numerator within the SMF is an integer. */
	int nNewNumerator;
	int nOldDenominator;
	int nNewDenominator;
	bool bRounded;
	bool bScaled;
};

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

	/** @param pOtherEventList can contain events which will be shallow copied
	 *    into the track. */
	SMFTrack( std::shared_ptr<EventList> pOtherEventList = nullptr );
	~SMFTrack();

	void addEvent( std::shared_ptr<SMFEvent> pEvent );

	virtual QByteArray getBuffer() const override;

		static void sortEvents( std::shared_ptr<EventList> pEventList );
		void sortAndTimeEvents();

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
	std::shared_ptr<EventList> m_pEventList;
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

		/** When deriving a new time signature is required, large positiv values
		 * will favor little change in the denominator and rounding in the
		 * numerator and negative ones the other way around. */
		static constexpr double fPenaltyTimeSignature = 1.1;

		/** The nominator and denominator used within Hydrogen and those used
		 * within the TimeSignature of a Standard MIDI File have different
		 * constraints. While in the former we use arbitrary float values for
		 * the numerator and arbitrary integers for the denominator in order to
		 * support all sorts of pattern sizes, the latter only supports integers
		 * for both and only powers of two for the denominator.
		 *
		 * This function takes a pattern and uses a heuristic to obtain the
		 * closest possible SMF-compatible numerator/denominator pair. */
		static void PatternToTimeSignature( std::shared_ptr<Pattern> pPattern,
											int* pNumerator, int* pDenominator,
											bool* pRounded, bool* pScaled );

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


/** \ingroup docCore docMIDI */
class SMFWriter : public H2Core::Object<SMFWriter>
{
	H2_OBJECT(SMFWriter)
public:
	SMFWriter( SMFHeader::Format format );
	virtual ~SMFWriter();
	void save( const QString& sFilename, std::shared_ptr<Song> pSong );
		const std::vector<SMFTimeSignatureFailure>& getTimeSignatureFailures() const;

protected:
	std::shared_ptr<SMFTrack> createTrack0( std::shared_ptr<Song> pSong );
	
	virtual void prepareEvents( std::shared_ptr<Song> pSong )=0;
	virtual void addEvent( std::shared_ptr<SMFEvent> pEvent,
						   std::shared_ptr<Instrument> pInstr ) = 0;
	virtual void packEvents( std::shared_ptr<Song> pSong,
							  std::shared_ptr<SMF> pSmf ) = 0;

		SMFHeader::Format m_format;
		std::vector<SMFTimeSignatureFailure> m_timeSignatureFailures;
	
private:
	void saveSMF( const QString& sFilename, std::shared_ptr<SMF> pSmf );
};

inline const std::vector<SMFTimeSignatureFailure>& SMFWriter::getTimeSignatureFailures() const {
	return m_timeSignatureFailures;
}


//-------

/** \ingroup docCore docMIDI */
class SMF1Writer : public Object<SMF1Writer>, public SMFWriter
{
    H2_OBJECT(SMF1Writer)
public:
    SMF1Writer();
	virtual ~SMF1Writer();
protected:
	/** Track containing tempo, time signature, and text (Timeline tags)
	 * events. */
	std::shared_ptr<SMFTrack> m_pTrack0;
};


/** \ingroup docCore docMIDI */
class SMF1WriterSingle : public Object<SMF1WriterSingle>, public SMF1Writer
{
    H2_OBJECT(SMF1WriterSingle)
public:
    SMF1WriterSingle();
	virtual ~SMF1WriterSingle();
protected:
	virtual void prepareEvents( std::shared_ptr<Song> pSong ) override;
	virtual void packEvents( std::shared_ptr<Song> pSong,
							 std::shared_ptr<SMF> pSmf ) override;
	virtual void addEvent( std::shared_ptr<SMFEvent> pEvent,
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
	virtual void prepareEvents( std::shared_ptr<Song> pSong ) override;
	virtual void packEvents( std::shared_ptr<Song> pSong,
							  std::shared_ptr<SMF> pSmf ) override;
	virtual void addEvent( std::shared_ptr<SMFEvent> pEvent,
						   std::shared_ptr<Instrument> pInstr ) override;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
	/** Contains events for each instrument in separate list using the
	 * corresponding instrument ID as index. */
	std::map<int, std::shared_ptr<EventList> > m_eventLists;
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
	virtual void prepareEvents( std::shared_ptr<Song> pSong ) override;
	virtual void packEvents( std::shared_ptr<Song> pSong,
							  std::shared_ptr<SMF> pSmf ) override;
	virtual void addEvent( std::shared_ptr<SMFEvent> pEvent,
						   std::shared_ptr<Instrument> pInstr ) override;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
		/** Track containing all events. */
	std::shared_ptr<SMFTrack> m_pTrack;
	std::shared_ptr<EventList> m_pEventList;
};



};

#endif

