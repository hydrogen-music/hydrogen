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
#include <hydrogen/LocalFileMng.h>

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
	__filename = "";
//	__playlistName = "";
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

bool Playlist::save( const QString& filename )
{
	set_filename( filename );

	LocalFileMng fileMng;
	if ( fileMng.savePlayList( filename.toLocal8Bit().constData() ) == 0 )
		return true;

	return false;
}

Playlist* Playlist::load( const QString& filename )
{
	LocalFileMng fileMng;
	int ret = fileMng.loadPlayList( filename.toLocal8Bit().constData() );

	if ( ret == 0 ) {
		Playlist* P = get_instance();
		P->set_filename( filename );
		return P;
	}
	return NULL;
}

void Playlist::setNextSongByNumber(int songNumber)
{
	Hydrogen* H = Hydrogen::get_instance();
	Preferences *pPref = Preferences::get_instance();

	int playlist_size = H->m_PlayList.size();
	if ( songNumber > playlist_size - 1 || playlist_size == 0 )
		return;

	if ( H->getState() == STATE_PLAYING )
		H->sequencer_stop();

	setSelectedSongNr( songNumber );
	setActiveSongNumber( songNumber );

	/* Load Song from file */
	QString selected = H->m_PlayList[ songNumber ].m_hFile;
	Song *pSong = Song::load( selected );
	if ( ! pSong ) return;

	/* Remove current song from memory not from hdd OFC ;-)
		i.e unload, this must be done after load new song */
	Song* oldSong = H->getSong();
	if ( oldSong ) {
		H->removeSong();
		delete oldSong;
	}

	H->setSelectedPatternNumber( 0 );
	H->setSong( pSong );

	pPref->setLastSongFilename( pSong->get_filename() );
	vector<QString> recentFiles = pPref->getRecentFiles();
	recentFiles.insert( recentFiles.begin(), selected );
	pPref->setRecentFiles( recentFiles );

	EventQueue::get_instance()->push_event( EVENT_PLAYLIST_LOADSONG, songNumber);

	execScript( songNumber );
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

void Playlist::execScript( int index)
{
	QString file;
	QString script;

	file = Hydrogen::get_instance()->m_PlayList[ index ].m_hScript;
	script = Hydrogen::get_instance()->m_PlayList[ index ].m_hScriptEnabled;

	if( !QFile( file ).exists()  || script == "Script not used")
		return;

	int ret = std::system( file.toLocal8Bit() );

	return;
}
