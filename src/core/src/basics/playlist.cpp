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
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/helpers/xml.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/LocalFileMng.h>

namespace H2Core
{

Playlist* Playlist::__instance = NULL;

const char* Playlist::__class_name = "Playlist";

Playlist::Playlist()
	: Object( __class_name )
{
	__filename = "";
	m_nSelectedSongNumber = -1;
	m_nActiveSongNumber = -1;
	m_bIsModified = false;
}

Playlist::~Playlist()
{
	clear();
	__instance = 0;
}

void Playlist::create_instance()
{
	if ( __instance == 0 ) {
		__instance = new Playlist();
	}
}

void Playlist::clear()
{
	for ( int i = 0; i < __entries.size(); i++ ) {
		delete __entries[i];
	}
	__entries.clear();
}

Playlist* Playlist::load_file( const QString& pl_path, bool useRelativePaths )
{
	XMLDoc doc;
	if( !doc.read( pl_path ) ) {
		return NULL;
	}
	XMLNode root = doc.firstChildElement( "playlist" );
	if ( root.isNull() ) {
		ERRORLOG( "playlist node not found" );
		return NULL;
	}
	QFileInfo fileInfo = QFileInfo( pl_path );
	return Playlist::load_from( &root, fileInfo, useRelativePaths );
}

Playlist* Playlist::load_from( XMLNode* node, QFileInfo& fileInfo, bool useRelativePaths )
{
	QString filename = node->read_string( "Name", "", false, false );
	if ( filename.isEmpty() ) {
		ERRORLOG( "Playlist has no name, abort" );
		return NULL;
	}

	Playlist* playlist = new Playlist();
	playlist->__filename = filename;

	XMLNode songsNode = node->firstChildElement( "Songs" );
	if ( !songsNode.isNull() ) {
		XMLNode nextNode = songsNode.firstChildElement( "next" );
		while ( !nextNode.isNull() ) {

			QString songPath = nextNode.read_string( "song", "", false, false );
			if ( !songPath.isEmpty() ) {
				Playlist::Entry* entry = new Playlist::Entry();
				QFileInfo songPathInfo( fileInfo.absoluteDir(), songPath );
				entry->m_hFile = songPathInfo.absoluteFilePath();
				entry->m_hFileExists = songPathInfo.isReadable();
				entry->m_hScript = nextNode.read_string( "script", "" );
				entry->m_hScriptEnabled = nextNode.read_bool( "enabled", false );
				playlist->add( entry );
			}

			nextNode = nextNode.nextSiblingElement( "next" );
		}
	} else {
		WARNINGLOG( "Songs node not found" );
	}
	return playlist;
}

bool Playlist::save_file( const QString& pl_path, const QString& name, bool overwrite, bool useRelativePaths )
{
	INFOLOG( QString( "Saving palylist to %1" ).arg( pl_path ) );
	if( !overwrite && Filesystem::file_exists( pl_path, true ) ) {
		ERRORLOG( QString( "palylist %1 already exists" ).arg( pl_path ) );
		return false;
	}

	setFilename( pl_path );

	XMLDoc doc;
	doc.set_root( "playlist", "playlist" );
	XMLNode root = doc.firstChildElement( "playlist" );
	root.write_string( "Name", name);
	XMLNode songs = doc.createElement( "Songs" );
	root.appendChild( songs );
	save_to( &songs, useRelativePaths );
	return doc.write( pl_path );
}

void Playlist::save_to( XMLNode* node, bool useRelativePaths )
{
	for (int i = 0; i < size(); i++ ) {
		Entry* entry = get( i );
		QString path = entry->m_hFile;
		if ( useRelativePaths ) {
			path = QDir( Filesystem::playlists_dir() ).relativeFilePath( path );
		}
		XMLNode song_node = node->ownerDocument().createElement( "next" );
		song_node.write_string( "song", path );
		song_node.write_string( "script", entry->m_hScript );
		song_node.write_string( "enabled", entry->m_hScriptEnabled);
		node->appendChild( song_node );
	}
}

Playlist* Playlist::load( const QString& filename, bool useRelativePaths )
{
	Playlist* playlist = Playlist::load_file( filename, useRelativePaths );

	if ( playlist != NULL ) {
		delete __instance;
		__instance = playlist;
	}

	return playlist;
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
	QString selected = get( songNumber )->m_hFile;
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
	if ( size() == 0 || songNumber >= size() ) {
		return;
	}

	/* NOTE: we are in MIDI thread and can't just call loadSong from here :( */
	EventQueue::get_instance()->push_event( EVENT_PLAYLIST_LOADSONG, songNumber );
}

void Playlist::execScript( int index)
{
	QString file = get( index )->m_hScript;
	bool enabled = ( get( index )->m_hScriptEnabled == "Script not used" );

	if( !enabled || !QFile( file ).exists() ) {
		return;
	}

	int ret = std::system( file.toLocal8Bit() );

	return;
}

};
