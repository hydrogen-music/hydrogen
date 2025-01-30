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

#include <string>
#include <cstdio>
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
	SMFHeader( int nFormat, int nTracks, int nTPQN );
	~SMFHeader();
	
	void addTrack();
	virtual QByteArray getBuffer() const override;
	virtual QString toQString() const override;
	
private:
	int m_nFormat;		///< SMF format
	int m_nTracks;		///< number of tracks
	int m_nTPQN;		///< ticks per quarter note
};



/** \ingroup docCore docMIDI */
class SMFTrack : public Object<SMFTrack>, public SMFBase
{
	H2_OBJECT(SMFTrack)
public:

	SMFTrack();
	~SMFTrack();

	void addEvent( SMFEvent *pEvent );

	virtual QByteArray getBuffer() const override;
	virtual QString toQString() const override;

private:
	std::vector<SMFEvent*> m_eventList;
};



/** \ingroup docCore docMIDI */
class SMF : public Object<SMF>, public SMFBase
{
	H2_OBJECT(SMF)
public:
	SMF( int nFormat, int nTPQN );
	~SMF();

	void addTrack( SMFTrack *pTrack );
	virtual QByteArray getBuffer() const override;
	virtual QString toQString() const override;

private:
	std::vector<SMFTrack*> m_trackList;

	SMFHeader* m_pHeader;
};



typedef std::vector<SMFEvent*> EventList;


/** \ingroup docCore docMIDI */
class SMFWriter : public H2Core::Object<SMFWriter>
{
	H2_OBJECT(SMFWriter)
public:
	SMFWriter();
	virtual ~SMFWriter();
	void save( const QString& sFilename, std::shared_ptr<Song> pSong );

protected:
	void sortEvents( EventList* pEventList );
	SMFTrack* createTrack0( std::shared_ptr<Song> pSong );
	
	virtual SMF* createSMF( std::shared_ptr<Song> pSong ) = 0;
	virtual void prepareEvents( std::shared_ptr<Song> pSong, SMF* pSmf )=0;
	virtual EventList* getEvents( std::shared_ptr<Song> pSong, std::shared_ptr<Instrument> pInstr ) = 0;
	virtual void  packEvents( std::shared_ptr<Song> pSong, SMF* pSmf ) = 0;
	
private:
	void saveSMF( const QString& sFilename, SMF* pSmf );
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
	virtual SMF* createSMF( std::shared_ptr<Song> pSong ) override;
};


/** \ingroup docCore docMIDI */
class SMF1WriterSingle : public Object<SMF1WriterSingle>, public SMF1Writer
{
    H2_OBJECT(SMF1WriterSingle)
public:
    SMF1WriterSingle();
	virtual ~SMF1WriterSingle();
protected:
	virtual void prepareEvents( std::shared_ptr<Song> pSong, SMF* pSmf ) override;
	virtual void packEvents( std::shared_ptr<Song> pSong, SMF* pSmf ) override;
	virtual EventList* getEvents( std::shared_ptr<Song> pSong, std::shared_ptr<Instrument> pInstr ) override;
private:
	EventList m_eventList;
};


/** \ingroup docCore docMIDI */
class SMF1WriterMulti : public Object<SMF1WriterMulti>, public SMF1Writer
{
    H2_OBJECT(SMF1WriterMulti)
public:
    SMF1WriterMulti();
	virtual ~SMF1WriterMulti();
protected:
	virtual void prepareEvents( std::shared_ptr<Song> pSong, SMF* pSmf ) override;
	virtual void  packEvents( std::shared_ptr<Song> pSong, SMF* pSmf ) override;
	virtual EventList* getEvents( std::shared_ptr<Song> pSong, std::shared_ptr<Instrument> pInstr ) override;
private:
	// contains events for each instrument in separate vector
	std::vector<EventList*> m_eventLists;
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
	virtual SMF* createSMF( std::shared_ptr<Song> pSong ) override;
	virtual void prepareEvents( std::shared_ptr<Song> pSong, SMF* pSmf ) override;
	virtual void  packEvents( std::shared_ptr<Song> pSong, SMF* pSmf ) override;
	virtual EventList* getEvents( std::shared_ptr<Song> pSong, std::shared_ptr<Instrument> pInstr ) override;
private:
	SMFTrack* m_pTrack;
	EventList m_eventList;
};



};

#endif

