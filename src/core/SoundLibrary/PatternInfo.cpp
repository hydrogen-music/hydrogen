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

#include <core/SoundLibrary/PatternInfo.h>

#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>
#include "SoundLibrary/SoundLibraryInfo.h"

namespace H2Core {

PatternInfo::PatternInfo()
{
}

PatternInfo::~PatternInfo()
{
}

bool PatternInfo::load( const QString& sPath )
{
	setPath( sPath );
	setContext( Filesystem::DetermineContext( sPath ) );

	XMLDoc doc;
	if ( !doc.read( sPath, true ) ) {
		ERRORLOG(
			QString( "Unable to load PatternInfo from [%1]" ).arg( sPath )
		);
		return false;
	}

	XMLNode rootNode = doc.firstChildElement( "drumkit_pattern" );
	if ( !rootNode.isNull() ) {
		setType( "pattern" );

		setAuthor( rootNode.read_string(
			"author", "undefined author", true, false, true
		) );
		setLicense( H2Core::License(
			rootNode.read_string( "license", "", true, false, true )
		) );
		XMLNode patternNode = rootNode.firstChildElement( "pattern" );
		// Try legacy format fist.
		setName( patternNode.read_string( "pattern_name", "", true, true ) );
		if ( getName().isEmpty() ) {
			// Try current format.
			setName( patternNode.read_string( "name", "", false, false ) );
		}
		if ( getAuthor() == "undefined author" ) {
			// current format
			setAuthor( patternNode.read_string(
				"author", "undefined author", true, false, true
			) );
		}
		if ( getLicense().isEmpty() ) {
			// current format
			setLicense( H2Core::License(
				patternNode.read_string( "license", "", true, false, true )
			) );
		}
		setInfo( patternNode.read_string(
			"info", "No information available.", false, true, true
		) );
	}
    else {
		ERRORLOG(
			QString( "Couldn't load pattern meta data from [%1]" ).arg( sPath )
		);
		return false;
	}

	return true;
}

QString PatternInfo::toQString( const QString& sPrefix, bool bShort ) const
{
	return SoundLibraryInfo::toQString( sPrefix, bShort );
}
};	// namespace H2Core
