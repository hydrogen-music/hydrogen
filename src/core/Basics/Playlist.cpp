/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
	__entries.clear();
}

Playlist* Playlist::load( const QString& sPath )
{
	Playlist* prev = __instance;

	Playlist* pPlaylist;
	XMLDoc doc;
	if ( !doc.read( sPath, Filesystem::playlist_xsd_path() ) ) {
		auto pPlaylist = Legacy::load_playlist( sPath );
		if ( pPlaylist == nullptr ) {
			ERRORLOG( QString( "Unable to load playlist [%1]" )
					  .arg( sPath ) );
			return nullptr;
		}
		WARNINGLOG( QString( "update playlist %1" ).arg( sPath ) );
		pPlaylist->saveAs( sPath, true );
	}
	else {
		XMLNode root = doc.firstChildElement( "playlist" );
		if ( ! root.isNull() ) {
			if ( root.read_string( "name", "", false, false ).isEmpty() ) {
				WARNINGLOG( "Playlist does not contain name" );
			}
			pPlaylist = Playlist::load_from( root, sPath );
		}
		else {
			ERRORLOG( "playlist node not found" );
			pPlaylist = nullptr;
		}

	}

	if ( pPlaylist != nullptr ) {
		delete prev;
		__instance = pPlaylist;
	}
	else {
		ERRORLOG( QString( "Unable to load Playlist [%1]" ).arg( sPath ) );
		__instance = prev;
	}

	return pPlaylist;

}

Playlist* Playlist::load_from( const XMLNode& node, const QString& sPath )
{
	QFileInfo fileInfo( sPath );

	Playlist* pPlaylist = new Playlist();
	pPlaylist->setFilename( fileInfo.absoluteFilePath() );

	XMLNode songsNode = node.firstChildElement( "songs" );
	if ( !songsNode.isNull() ) {
		XMLNode nextNode = songsNode.firstChildElement( "song" );
		while ( !nextNode.isNull() ) {

			QString songPath = nextNode.read_string( "path", "", false, false );
			if ( !songPath.isEmpty() ) {
				auto pEntry = std::make_shared<Playlist::Entry>();
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

bool Playlist::saveAs( const QString& sTargetPath, bool bSilent ) {
	if ( ! bSilent  ) {
		INFOLOG( QString( "Saving playlist [%1] as [%2]" )
				 .arg( __filename ).arg( sTargetPath ) );
	}

	setFilename( sTargetPath );

	return save( true );
}

bool Playlist::save( bool bSilent ) const {
	if ( __filename.isEmpty() ) {
		ERRORLOG( "No filepath provided!" );
		return false;
	}

	if ( ! bSilent ) {
		INFOLOG( QString( "Saving playlist to [%1]" ).arg( __filename ) );
	}

	XMLDoc doc;
	XMLNode root = doc.set_root( "playlist", "playlist" );

	QFileInfo info( __filename );
	root.write_string( "name", info.fileName() );

	saveTo( root );
	return doc.write( __filename );
}

void Playlist::saveTo( XMLNode& node ) const
{
	XMLNode songs = node.createNode( "songs" );

	for ( const auto& pEntry : __entries ) {
		QString sPath = pEntry->filePath;
		if ( Preferences::get_instance()->isPlaylistUsingRelativeFilenames() ) {
			sPath = QDir( Filesystem::playlists_dir() ).relativeFilePath( sPath );
		}
		XMLNode song_node = songs.createNode( "song" );
		song_node.write_string( "path", sPath );
		song_node.write_string( "scriptPath", pEntry->scriptPath );
		song_node.write_bool( "scriptEnabled", pEntry->scriptEnabled);
	}
}

/* This method is called by Event dispatcher thread ( GUI ) */
void Playlist::activateSong( int songNumber )
{
	setSelectedSongNr( songNumber );
	setActiveSongNumber( songNumber );

	execScript( songNumber );
}

bool Playlist::getSongFilenameByNumber( int songNumber, QString& filename) const
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

void Playlist::execScript( int nIndex ) const
{
	QString sFile = get( nIndex )->scriptPath;

	if ( !get( nIndex )->scriptEnabled ) {
		return;
	}
	if ( !QFile( sFile ).exists() ) {
		ERRORLOG( QString( "Script [%1] for playlist [%2] does not exist!" )
				  .arg( sFile ).arg( nIndex ) );
		return;
	}

	int nRes = std::system( sFile.toLocal8Bit() );
	if ( nRes != 0 ) {
		WARNINGLOG( QString( "Script [%1] for playlist [%2] exited with status code [%3]" )
					.arg( sFile ).arg( nIndex ).arg( nRes ) );
	}

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
			for ( const auto& ii : __entries ) {
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
			for ( const auto& ii : __entries ) {
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
