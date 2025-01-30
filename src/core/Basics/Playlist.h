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
#include <core/Helpers/Xml.h>

#include <memory>
#include <vector>

namespace H2Core
{

class PlaylistEntry : public H2Core::Object<PlaylistEntry> {
	H2_OBJECT(PlaylistEntry)

	PlaylistEntry( const QString& sFilePath = "", const QString& sScriptPath = "",
				   bool bScriptEnabled = false );
	PlaylistEntry( std::shared_ptr<PlaylistEntry> pOther );

	static const QString sLegacyEmptyScriptPath;

		const QString& getSongPath() const;
		void setSongPath( const QString& sSongPath );
		const QString& getScriptPath() const;
		void setScriptPath( const QString& sScriptPath );
		bool getScriptEnabled() const;
		void setScriptEnabled( bool bEnabled );
		bool getSongExists() const;
		bool getScriptExists() const;

	static std::shared_ptr<PlaylistEntry> fromMimeText( const QString& sText );
	QString toMimeText() const;

	friend bool operator==( std::shared_ptr<PlaylistEntry> pLeft,
							std::shared_ptr<PlaylistEntry> pRight );
	friend bool operator!=( std::shared_ptr<PlaylistEntry> pLeft,
							std::shared_ptr<PlaylistEntry> pRight );

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:

	QString m_sSongPath;
	QString m_sScriptPath;
	bool m_bScriptEnabled;

	/** Runtime member variable. Not written to disk and set whenever
	 * #m_sSongPath is updated. */
	bool m_bSongExists;
	/** Runtime member variable. Not written to disk and set whenever
	 * #m_sScriptPath is updated. */
	bool m_bScriptExists;
};

/** \ingroup docCore docDataStructure */
class Playlist : public H2Core::Object<Playlist>

{
		H2_OBJECT(Playlist)

	public:
		
		Playlist();

		bool	activateSong (int SongNumber );

		int		size() const;
		std::shared_ptr<PlaylistEntry>	get( int idx ) const;

		std::vector<std::shared_ptr<PlaylistEntry>>::iterator begin() {
			return m_entries.begin();
		}
		std::vector<std::shared_ptr<PlaylistEntry>>::iterator end() {
			return m_entries.end();
		}

		void	clear();
		/** Adds a new song/ entry to the current playlist.
		 *
		 * If @a nIndex is set to a value of -1, @a pEntry will be appended at
		 * the end of the playlist. */
		bool	add( std::shared_ptr<PlaylistEntry> entry, int nIndex = -1 );
		/** Removes a song from the current playlist.
		 *
		 * If @a nIndex is set to a value of -1, the first occurrance of @a
		 * pEntry will be deleted. */
		bool	remove( std::shared_ptr<PlaylistEntry> entry, int nIndex = -1 );

		int		getActiveSongNumber() const;

		QString	getSongFilenameByNumber( int nSongNumber ) const;

		const QString& getFilename() const;
		void setFilename( const QString& filename );
		bool getIsModified() const;
		void setIsModified( bool IsModified );

		static std::shared_ptr<Playlist> load( const QString& sPath );
		bool saveAs( const QString& sTargetPath, bool bSilent = false );
		bool save( bool bSilent = false ) const;
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

		static std::shared_ptr<Playlist> load_from( const XMLNode& root, const QString& sPath );
		void saveTo( XMLNode& node ) const;

		void execScript( int index ) const;

		void setActiveSongNumber( int ActiveSongNumber );

		QString m_sFilename;
		std::vector<std::shared_ptr<PlaylistEntry>> m_entries;
		int m_nActiveSongNumber;
		bool m_bIsModified;

		/** Used to indicate changes in the underlying XSD file. */
		static constexpr int nCurrentFormatVersion = 2;
};

inline const QString& PlaylistEntry::getSongPath() const {
	return m_sSongPath;
}
inline const QString& PlaylistEntry::getScriptPath() const {
	return m_sScriptPath;
}
inline bool PlaylistEntry::getScriptEnabled() const {
	return m_bScriptEnabled;
}
inline void PlaylistEntry::setScriptEnabled( bool bEnabled ) {
	m_bScriptEnabled = bEnabled;
}
inline bool PlaylistEntry::getSongExists() const {
	return m_bSongExists;
}
inline bool PlaylistEntry::getScriptExists() const {
	return m_bScriptExists;
}

inline int Playlist::size() const
{
	return m_entries.size();
}

inline int Playlist::getActiveSongNumber() const
{
	return m_nActiveSongNumber;
}

inline void Playlist::setActiveSongNumber( int ActiveSongNumber )
{
	m_nActiveSongNumber = ActiveSongNumber ;
}

inline const QString& Playlist::getFilename() const
{
	return m_sFilename;
}

inline void Playlist::setFilename( const QString& filename )
{
	m_sFilename = filename;
}

inline bool Playlist::getIsModified() const
{
	return m_bIsModified;
}

inline void Playlist::setIsModified( bool IsModified )
{
	m_bIsModified = IsModified;
}

};

#endif // H2C_PLAYLIST_H
