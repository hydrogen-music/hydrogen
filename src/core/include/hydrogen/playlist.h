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

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QDialog>
#include <hydrogen/object.h>
#include <hydrogen/globals.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <vector>
#include <cassert>

///
/// handle playlist
///

class Playlist :  public H2Core::Object

{
	H2_OBJECT

	public:
		static void create_instance();
		static Playlist* get_instance() { assert(__instance); return __instance; }

		~Playlist();

//		std::vector<HPlayListNode> m_PlayList;
		void setNextSongByNumber(int SongNumber); /* Call from MIDI thread */
		bool loadSong (int SongNumber); /* Call from main (e.g. GUI) thread */
		void setSelectedSongNr( int songNumber);

		int selectedSongNumber;
		int activeSongNumber;

		int getSelectedSongNr();
		void setActiveSongNumber( int ActiveSongNumber);
		int getActiveSongNumber();

//		QString __playlistName;

		const QString& get_filename() {
			return __filename;
		}

		void set_filename( const QString& filename ) {
			__filename = filename;
		}
		
		static Playlist* load( const QString& filename );
		bool save( const QString& filename );

	private:
		static Playlist* __instance;
		QString __filename;

		/// Constructor
		Playlist();

		void execScript( int index);
};

#endif
