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

#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/playlist.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/LocalFileMng.h>

namespace H2Core
{

Playlist* Playlist::__instance = NULL;

const char* Playlist::__class_name = "Playlist";

Playlist::Playlist()
	: Object( __class_name )
{
	if ( __instance ) {
		_ERRORLOG( "Playlist in use" );
	}

	__instance = this;
	__filename = "";
	m_nSelectedSongNumber = -1;
	m_nActiveSongNumber = -1;
	m_bIsModified = false;
}

Playlist::~Playlist()
{
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
	setFilename( filename );

	LocalFileMng fileMng;
	if ( fileMng.savePlayList( filename.toLocal8Bit().constData() ) == 0 ) {
		return true;
	}

	return false;
}

Playlist* Playlist::load( const QString& filename )
{
	LocalFileMng fileMng;
	int ret = fileMng.loadPlayList( filename.toLocal8Bit().constData() );

	if ( ret == 0 ) {
		Playlist* pPlaylist = get_instance();
		pPlaylist->setFilename( filename );
		return pPlaylist;
	}
	return NULL;
}

/* This method is called by Event dispacher thread ( GUI ) */
bool Playlist::loadSong( int songNumber )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Preferences *pPref = Preferences::get_instance();

	if ( pHydrogen->getState() == STATE_PLAYING ) {
		pHydrogen->sequencer_stop();
	}

	/* Load Song from file */
	QString selected = pHydrogen->m_PlayList[ songNumber ].m_hFile;
	Song *pSong = Song::load( selected );
	if ( ! pSong ) {
		return false;
	}

	setSelectedSongNr( songNumber );
	setActiveSongNumber( songNumber );

	pHydrogen->setSong( pSong );

	pPref->setLastSongFilename( pSong->get_filename() );
	vector<QString> recentFiles = pPref->getRecentFiles();
	recentFiles.insert( recentFiles.begin(), selected );
	pPref->setRecentFiles( recentFiles );

	execScript( songNumber );

	return true;
}

/* This method is called by MIDI thread */
void Playlist::setNextSongByNumber( int songNumber )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();

	int playlist_size = pHydrogen->m_PlayList.size();
	if ( songNumber > playlist_size - 1 || playlist_size == 0 ) {
		return;
	}

	/* NOTE: we are in MIDI thread and can't just call loadSong from here :( */
	EventQueue::get_instance()->push_event( EVENT_PLAYLIST_LOADSONG, songNumber );
}

void Playlist::execScript( int index)
{
	QString file;
	QString script;

	file = Hydrogen::get_instance()->m_PlayList[ index ].m_hScript;
	script = Hydrogen::get_instance()->m_PlayList[ index ].m_hScriptEnabled;

	if( !QFile( file ).exists()  || script == "Script not used") {
		return;
	}

	int ret = std::system( file.toLocal8Bit() );

	return;
}

};
