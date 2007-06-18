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
 * $Id: SMF.h,v 1.4 2005/05/01 19:51:41 comix Exp $
 *
 */

#ifndef SMF_H
#define SMF_H

#include "../Object.h"
#include "../Song.h"

#include <string>
#include <cstdio>
#include <vector>

#include "SMFEvent.h"

class SMFHeader : public SMFBase, public Object
{
	public:
		SMFHeader( int nFormat, int nTracks, int nTPQN );
		~SMFHeader();
	
		int m_nFormat;		///< SMF format
		int m_nTracks;		///< number of tracks
		int m_nTPQN;		///< ticks per quarter note
		
		virtual vector<char> getBuffer();
};



class SMFTrack : public SMFBase, public Object
{
	public:
		SMFTrack( std::string sTrackName );
		~SMFTrack();
	
		void addEvent( SMFEvent *pEvent );
		
		virtual vector<char> getBuffer();

	private:
		std::vector<SMFEvent*> m_eventList;
};



class SMF : public SMFBase, public Object
{
	public:
		SMF();
		~SMF();

		void addTrack( SMFTrack *pTrack );
		virtual vector<char> getBuffer();
		
	private:
		std::vector<SMFTrack*> m_trackList;

		SMFHeader* m_pHeader;

};



class SMFWriter : Object
{
	public:
		SMFWriter();
		~SMFWriter();
		
		void save( std::string sFilename, Song *pSong );
	
	private:
		FILE *m_file;
		
};

#endif

