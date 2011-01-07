/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <hydrogen/h2_exception.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/playlist.h>
#include <hydrogen/event_queue.h>

#include <vector>
#include <cstdlib>



using namespace H2Core;


Playlist* Playlist::__instance = NULL;

const char* Playlist::__class_name = "Playlist";

Playlist::Playlist()
		: Object( __class_name )
{
	if ( __instance ) {class HydrogenApp;

		_ERRORLOG( "Playlist in use" );
	}class HydrogenApp;

	//_INFOLOG( "[Playlist]" );
	__instance = this;
	__playlistName = "";
	selectedSongNumber = -1;
	activeSongNumber = -1;
}



Playlist::~Playlist()
{
	//_INFOLOG( "[~Playlist]" );
	__instance = NULL;
}



void Playlist::create_instance()
{
	if ( __instance == 0 ) {
		__instance = new Playlist;
	}
}



Song* Playlist::setNextSongByNumber(int songNumber)
{
	
	
	if ( songNumber > (int)Hydrogen::get_instance()->m_PlayList.size() -1 || (int)Hydrogen::get_instance()->m_PlayList.size() == 0 )
		return 0;

	setSelectedSongNr( songNumber );
	setActiveSongNumber( songNumber );

	QString selected;
	selected = Hydrogen::get_instance()->m_PlayList[ songNumber ].m_hFile;

	Song* song = loadSong( selected );
	execScript( songNumber );

	EventQueue::get_instance()->push_event( EVENT_METRONOME, 3 );
    return song;
}



Song* Playlist::setNextSongPlaylist()
{
	int index = getSelectedSongNr();
	if (index == -1 ){
		if ( getActiveSongNumber() != -1){
			index = getActiveSongNumber();
        } else {
            return 0;
        }
	}

	index = index +1;
	if ( (int) index > (int)Hydrogen::get_instance()->m_PlayList.size() -1 || index < 0) 
		return 0;
	setSelectedSongNr( index );
	setActiveSongNumber( index );

	Song* song = loadSong( Hydrogen::get_instance()->m_PlayList[ index ].m_hFile );
	execScript( index );

	EventQueue::get_instance()->push_event( EVENT_METRONOME, 3 );	
    return song;
}



Song* Playlist::setPrevSongPlaylist()
{
	int index = getSelectedSongNr();

	if (index == -1 ){
		if ( getActiveSongNumber() != -1 ){
			index = getActiveSongNumber();
		}else
		{
			return 0;
		}
	}

	index = index -1;

	if (index < 0 ) 
		return 0;

	setSelectedSongNr( index );
	setActiveSongNumber( index );

	Song* song = loadSong( Hydrogen::get_instance()->m_PlayList[ index ].m_hFile );
	execScript( index );

	EventQueue::get_instance()->push_event( EVENT_METRONOME, 3 );
    return song;
}



void Playlist::setSelectedSongNr( int songNumber )
{
	selectedSongNumber = songNumber;
}



int Playlist::getSelectedSongNr()
{
	return selectedSongNumber;
}



void Playlist::setActiveSongNumber( int ActiveSongNumber)
{
	 activeSongNumber = ActiveSongNumber ;
}



int Playlist::getActiveSongNumber()
{
	return activeSongNumber;
}



Song* Playlist::loadSong( QString songName )
{
	Hydrogen *engine = Hydrogen::get_instance();
	

	if ( engine->getState() == STATE_PLAYING ){
		engine->sequencer_stop();
	}

	engine->m_timelinetagvector.clear();
    engine->setSelectedPatternNumber ( 0 );

    return Song::load( songName );
}



void Playlist::execScript( int index)
{
	QString file;
	QString script;

	file = Hydrogen::get_instance()->m_PlayList[ index ].m_hScript;
	script = Hydrogen::get_instance()->m_PlayList[ index ].m_hScriptEnabled;

	if( file == "no Script" || script == "Script not used")
		return;

	int ret = std::system( file.toLocal8Bit() );

	return;
}
