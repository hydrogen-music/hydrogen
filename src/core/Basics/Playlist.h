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

#ifndef H2C_PLAYLIST_H
#define H2C_PLAYLIST_H

#include <core/Object.h>

namespace H2Core
{

/**
 * Drumkit info
*/
/** \ingroup docCore docDataStructure */
class Playlist : public H2Core::Object<Playlist>

{
		H2_OBJECT(Playlist)

	public:
		struct Entry
		{
			QString filePath;
			bool fileExists;
			QString scriptPath;
			bool scriptEnabled;
		};
		
		/**
		 * If #__instance equals 0, a new Playlist singleton
		 * will be created and stored in it.
		 *
		 * It is called in Hydrogen::audioEngine_init().
		 */
		static void create_instance();
		/**
		 * Returns a pointer to the current Playlist singleton
		 * stored in #__instance.
		 */
		static Playlist* get_instance() { assert(__instance); return __instance; }

		~Playlist();

		void	activateSong (int SongNumber );

		int		size() const;
		Entry*	get( int idx );

		void	clear();
		void	add( Entry* entry );

		void	setNextSongByNumber( int SongNumber );
		int		getSelectedSongNr();
		void	setSelectedSongNr( int songNumber );

		int		getActiveSongNumber();
		void	setActiveSongNumber( int ActiveSongNumber );
		
		bool	getSongFilenameByNumber( int songNumber, QString& fileName);

		const QString& getFilename();
		void setFilename( const QString& filename );
		bool getIsModified();
		void setIsModified( bool IsModified );

		static Playlist* load( const QString& filename, bool useRelativePaths );
		static Playlist* load_file( const QString& pl_path, bool useRelativePaths );
		bool save_file( const QString& pl_path, const QString& name, bool overwrite, bool useRelativePaths );
		/** Formatted string version for debugging purposes.
		 * \param sPrefix String prefix which will be added in front of
		 * every new line
		 * \param bShort Instead of the whole content of all classes
		 * stored as members just a single unique identifier will be
		 * displayed without line breaks.
		 *
		 * \return String presentation of current object.*/
		QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

	private:
		/**
		 * Object holding the current Playlist singleton. It is
		 * initialized with NULL, set with create_instance(), and
		 * accessed with get_instance().
		 */
		static Playlist* __instance;
		QString __filename;

		std::vector<Entry*> __entries;

		int m_nSelectedSongNumber;
		int m_nActiveSongNumber;

		bool m_bIsModified;

		Playlist();

		void execScript( int index );

		void save_to( XMLNode* node, bool useRelativePaths );
};

inline int Playlist::size() const
{
	return __entries.size();
}

inline Playlist::Entry* Playlist::get( int idx )
{
	assert( idx >= 0 && idx < size() );
	return __entries[ idx ];
}

inline void Playlist::add( Entry* entry )
{
	__entries.push_back( entry );
}

inline int Playlist::getSelectedSongNr()
{
	return m_nSelectedSongNumber;
}

inline void Playlist::setSelectedSongNr( int songNumber )
{
	m_nSelectedSongNumber = songNumber;
}

inline int Playlist::getActiveSongNumber()
{
	return m_nActiveSongNumber;
}

inline void Playlist::setActiveSongNumber( int ActiveSongNumber )
{
	m_nActiveSongNumber = ActiveSongNumber ;
}

inline const QString& Playlist::getFilename()
{
	return __filename;
}

inline void Playlist::setFilename( const QString& filename )
{
	__filename = filename;
}

inline bool Playlist::getIsModified()
{
	return m_bIsModified;
}

inline void Playlist::setIsModified( bool IsModified )
{
	m_bIsModified = IsModified;
}

};

#endif // H2C_PLAYLIST_H
