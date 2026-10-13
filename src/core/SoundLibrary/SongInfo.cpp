/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/SoundLibrary/SongInfo.h>

#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>

namespace H2Core {

SongInfo::SongInfo()
{
}

SongInfo::~SongInfo()
{
}

bool SongInfo::load( const QString& sPath, Hydrogen* pHydrogen )
{
	XMLDoc doc;
	if ( !doc.read( sPath, true ) ) {
		ERRORLOG( QString( "Unable to load SongInfo from [%1]" ).arg( sPath ) );
		return false;
	}

	const XMLNode rootNode = doc.firstChildElement( "song" );
	if ( !rootNode.isNull() ) {
		m_sPath = sPath;
		m_context = Filesystem::DetermineContext( sPath, pHydrogen );
		m_type = SoundLibraryInfo::Type::Song;
		m_sAuthor =
			rootNode.read_string( "author", "undefined author", false, false );
		m_license =
			H2Core::License( rootNode.read_string( "license", "", false, false )
			);
		m_sName = rootNode.read_string( "name", "", false, false );
		m_sInfo = rootNode.read_string( "notes", "", false, false );
		const XMLNode tagsNode = rootNode.firstChildElement( "tags" );
		if ( !tagsNode.isNull() ) {
			QStringList tags;
			auto tagNode = tagsNode.firstChildElement( "tag" );
			while ( !tagNode.isNull() && !tagNode.text().isEmpty() ) {
				tags << tagNode.text();
				tagNode = tagNode.nextSiblingElement( "tag" );
			}
			m_tags = tags;
		}
		m_nVersion = rootNode.read_int( "userVersion", 0, false, false, true );
	}
	else {
		ERRORLOG(
			QString( "Couldn't load song meta data from [%1]" ).arg( sPath )
		);
		return false;
	}

	return true;
}

QString SongInfo::toQString( const QString& sPrefix, bool bShort ) const
{
	return SoundLibraryInfo::toQString( sPrefix, bShort );
}
};	// namespace H2Core
