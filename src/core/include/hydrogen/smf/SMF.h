/*
 * Hydrogen
 * Copyright(c) 2002-2004 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#ifndef SMF_H
#define SMF_H

#include <hydrogen/object.h>
#include <hydrogen/basics/song.h>

#include <string>
#include <cstdio>
#include <vector>

#include <hydrogen/smf/SMFEvent.h>

namespace H2Core
{

class SMFHeader : public SMFBase, public H2Core::Object
{
public:
	/** \return #m_sClassName*/
	static const char* className() { return m_sClassName; }
	SMFHeader( int nFormat, int nTracks, int nTPQN );
	~SMFHeader();

	int m_nFormat;		///< SMF format
	int m_nTracks;		///< number of tracks
	int m_nTPQN;		///< ticks per quarter note

	virtual std::vector<char> getBuffer();
private:
	/** Contains the name of the class.
	 *
	 * This variable allows from more informative log messages
	 * with the name of the class the message is generated in
	 * being displayed as well. Queried using className().*/
	static const char* m_sClassName;
};



class SMFTrack : public SMFBase, public H2Core::Object
{
public:
	/** \return #m_sClassName*/
	static const char* className() { return m_sClassName; }

	SMFTrack();
	~SMFTrack();

	void addEvent( SMFEvent *pEvent );

	virtual std::vector<char> getBuffer();

private:
	/** Contains the name of the class.
	 *
	 * This variable allows from more informative log messages
	 * with the name of the class the message is generated in
	 * being displayed as well. Queried using className().*/
	static const char* m_sClassName;
	std::vector<SMFEvent*> m_eventList;
};



class SMF : public SMFBase, public H2Core::Object
{
public:
	/** \return #m_sClassName*/
	static const char* className() { return m_sClassName; }
	SMF();
	~SMF();

	void addTrack( SMFTrack *pTrack );
	virtual std::vector<char> getBuffer();

private:
	/** Contains the name of the class.
	 *
	 * This variable allows from more informative log messages
	 * with the name of the class the message is generated in
	 * being displayed as well. Queried using className().*/
	static const char* m_sClassName;
	std::vector<SMFTrack*> m_trackList;

	SMFHeader* m_pHeader;

};



class SMFWriter : Object
{
public:
	/** \return #m_sClassName*/
	static const char* className() { return m_sClassName; }
	SMFWriter();
	~SMFWriter();

	void save( const QString& sFilename, Song *pSong );

private:
	/** Contains the name of the class.
	 *
	 * This variable allows from more informative log messages
	 * with the name of the class the message is generated in
	 * being displayed as well. Queried using className().*/
	static const char* m_sClassName;
	FILE *m_file;

};

};

#endif

