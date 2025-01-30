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

#include <core/Preferences/Preferences.h>
#include <core/Hydrogen.h>
#include <core/Basics/Playlist.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Legacy.h>
#include <core/Helpers/Xml.h>
#include <core/EventQueue.h>

namespace H2Core
{

Playlist* Playlist::__instance = nullptr;

Playlist::Playlist()
{
	__filename = "";
	m_nSelectedSongNumber = -1;
	m_nActiveSongNumber = -1;
	m_bIsModified = false;
}

Playlist::~Playlist()
{
	clear();
	__instance = nullptr;
}

void Playlist::create_instance()
{
	if ( __instance == nullptr ) {
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
	if ( !doc.read( pl_path, Filesystem::playlist_xsd_path() ) ) {
		Playlist* pl = new Playlist();
		Playlist* ret = Legacy::load_playlist( pl, pl_path );
		if ( ret == nullptr ) {
			delete pl;	// __instance = 0;
			return nullptr;
		}
		WARNINGLOG( QString( "update playlist %1" ).arg( pl_path ) );
		pl->save_file( pl_path, pl->getFilename(), true, useRelativePaths );
		return pl;
	}
	XMLNode root = doc.firstChildElement( "playlist" );
	if ( root.isNull() ) {
		ERRORLOG( "playlist node not found" );
		return nullptr;
	}
	QFileInfo fileInfo = QFileInfo( pl_path );
	return Playlist::load_from( &root, fileInfo, useRelativePaths );
}

Playlist* Playlist::load_from( XMLNode* node, QFileInfo& fileInfo, bool useRelativePaths )
{
	// Check whether the file was created using a newer version of Hydrogen.
	auto formatVersionNode = node->firstChildElement( "formatVersion" );
	if ( ! formatVersionNode.isNull() ) {
		WARNINGLOG( QString( "Playlist file [%1] was created with a more recent version of Hydrogen than the current one!" )
					.arg( fileInfo.absoluteFilePath() ) );
	}

	Playlist* pPlaylist = new Playlist();
	pPlaylist->setFilename( fileInfo.absoluteFilePath() );

	XMLNode songsNode = node->firstChildElement( "songs" );
	if ( !songsNode.isNull() ) {
		XMLNode nextNode = songsNode.firstChildElement( "song" );
		while ( !nextNode.isNull() ) {

			QString songPath = nextNode.read_string( "path", "", false, false );
			if ( !songPath.isEmpty() ) {
				Playlist::Entry* pEntry = new Playlist::Entry();
				QFileInfo songPathInfo( fileInfo.absoluteDir(), songPath );
				pEntry->filePath = songPathInfo.absoluteFilePath();
				pEntry->fileExists = songPathInfo.isReadable();
				pEntry->scriptPath = nextNode.read_string( "scriptPath", "" );
				pEntry->scriptEnabled = nextNode.read_bool( "scriptEnabled", false );
				pPlaylist->add( pEntry );
			}

			nextNode = nextNode.nextSiblingElement( "song" );
		}
	} else {
		WARNINGLOG( "songs node not found" );
	}
	return pPlaylist;
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
	XMLNode root = doc.set_root( "playlist", "playlist" );
	root.write_string( "name", name);
	XMLNode songs = root.createNode( "songs" );
	save_to( &songs, useRelativePaths );
	return doc.write( pl_path );
}

void Playlist::save_to( XMLNode* node, bool useRelativePaths )
{
	QFileInfo fileInfo( __filename );

	for (int i = 0; i < size(); i++ ) {
		Entry* entry = get( i );
		QString path = entry->filePath;
		if ( useRelativePaths ) {
			path = fileInfo.absoluteDir().relativeFilePath( path );
		}
		XMLNode song_node = node->createNode( "song" );
		song_node.write_string( "path", path );
		song_node.write_string( "scriptPath", entry->scriptPath );
		song_node.write_bool( "scriptEnabled", entry->scriptEnabled);
	}
}

Playlist* Playlist::load( const QString& filename, bool useRelativePaths )
{
	// load_file might set __instance = 0;
	Playlist* prev = __instance;
	Playlist* playlist = Playlist::load_file( filename, useRelativePaths );

	if ( playlist != nullptr ) {
		delete prev;
		__instance = playlist;
	} else {
		__instance = prev;
	}

	return playlist;
}

/* This method is called by Event dispatcher thread ( GUI ) */
void Playlist::activateSong( int songNumber )
{
	setSelectedSongNr( songNumber );
	setActiveSongNumber( songNumber );

	execScript( songNumber );
}

bool Playlist::getSongFilenameByNumber( int songNumber, QString& filename)
{
	bool Success = true;
	
	if ( size() == 0 || songNumber >= size() ) {
		Success = false;
	}
	
	if( Success)  {
		filename = get( songNumber )->filePath;
	}

	return Success;
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
	QString file = get( index )->scriptPath;

	if ( !get( index )->scriptEnabled || !QFile( file ).exists() ) {
		return;
	}

	std::system( file.toLocal8Bit() );

	return;
}

QString Playlist::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Playlist]\n" ).arg( sPrefix )
			.append( QString( "%1%2filename: %3\n" ).arg( sPrefix ).arg( s ).arg( __filename ) )
			.append( QString( "%1%2m_nSelectedSongNumber: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nSelectedSongNumber ) )
			.append( QString( "%1%2m_nActiveSongNumber: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nActiveSongNumber ) )
			.append( QString( "%1%2entries:\n" ).arg( sPrefix ).arg( s ) );
		if ( size() > 0 ) {
			for ( auto ii : __entries ) {
				sOutput.append( QString( "%1%2Entry:\n" ).arg( sPrefix ).arg( s + s ) )
					.append( QString( "%1%2filePath: %3\n" ).arg( sPrefix ).arg( s + s + s ).arg( ii->filePath ) )
					.append( QString( "%1%2fileExists: %3\n" ).arg( sPrefix ).arg( s + s + s ).arg( ii->fileExists ) )
					.append( QString( "%1%2scriptPath: %3\n" ).arg( sPrefix ).arg( s + s + s ).arg( ii->scriptPath ) )
					.append( QString( "%1%2scriptEnabled: %3\n" ).arg( sPrefix ).arg( s + s + s ).arg( ii->scriptEnabled ) );
			}
		}
		sOutput.append( QString( "%1%2m_bIsModified: %3\n" ).arg( sPrefix ).arg( s ).arg( m_bIsModified ) );
	} else {
		sOutput = QString( "[Playlist]" )
			.append( QString( " filename: %1" ).arg( __filename ) )
			.append( QString( ", m_nSelectedSongNumber: %1" ).arg( m_nSelectedSongNumber ) )
			.append( QString( ", m_nActiveSongNumber: %1" ).arg( m_nActiveSongNumber ) )
			.append( ", entries: {" );
		if ( size() > 0 ) {
			for ( auto ii : __entries ) {
				sOutput.append( QString( "[filePath: %1" ).arg( ii->filePath ) )
					.append( QString( ", fileExists: %1" ).arg( ii->fileExists ) )
					.append( QString( ", scriptPath: %1" ).arg( ii->scriptPath ) )
					.append( QString( ", scriptEnabled: %1] " ).arg( ii->scriptEnabled ) );
										
										
			}
		}
		sOutput.append( QString( "}, m_bIsModified: %1\n" ).arg( m_bIsModified ) );
	}
	
	return sOutput;
}
};
