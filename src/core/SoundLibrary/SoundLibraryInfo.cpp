/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2023 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/SoundLibrary/SoundLibraryInfo.h>
#include <core/Helpers/Xml.h>
#include <core/License.h>

namespace H2Core
{


SoundLibraryInfo::SoundLibraryInfo()
{
	//default constructor
}

bool SoundLibraryInfo::load( const QString& sPath ) {
	setPath( sPath );

	XMLDoc doc;
	if ( ! doc.read( sPath, nullptr, true ) ) {
		ERRORLOG( QString( "Unable to load SoundLibraryInfo from [%1]" )
				  .arg( sPath ) );
		return false;
	}

	bool bLoadingWorked = false;

	XMLNode rootNode =  doc.firstChildElement( "drumkit_pattern" );
	if ( ! rootNode.isNull() )
	{
		setType( "pattern" );
		setAuthor( rootNode.read_string( "author", "undefined author", false, false ) );
		setLicense( H2Core::License( rootNode.read_string( "license", "", false, false ) ) );

		XMLNode patternNode = rootNode.firstChildElement( "pattern" );
		// Try legacy format fist.
		setName( patternNode.read_string( "pattern_name", "", true, true ) );
		if ( getName().isEmpty() ) {
			// Try current format.
			setName( patternNode.read_string( "name", "", false, false ) );
		}
		setInfo( patternNode.read_string( "info", "No information available.", false, true, true ) );
		setCategory( patternNode.read_string( "category", "", false, true ) );

		QString sDrumkitName = rootNode.read_string( "drumkit_name", "", false, false );
		if ( sDrumkitName.isEmpty() ) {
			sDrumkitName = rootNode.read_string( "pattern_for_drumkit", "" );
		}
		setDrumkitName( sDrumkitName );
		
		bLoadingWorked = true;
	}


	//New drumkits
	rootNode = doc.firstChildElement( "drumkit_info" );
	if ( ! rootNode.isNull() )
	{
		setType( "drumkit" );
		setAuthor( rootNode.read_string( "author", "undefined author", false, false ) );
		setLicense( H2Core::License( rootNode.read_string( "license", "", false, false ) ) );
		setName( rootNode.read_string( "name", "", false, false ) );
		setInfo( rootNode.read_string( "info", "No information available.", false, false ) );
		setImage( rootNode.read_string( "image", "", false, false ) );
		setImageLicense( H2Core::License( rootNode.read_string( "imageLicense", "", false, false ) ) );
		
		bLoadingWorked = true;
	}

	//Songs
	rootNode = doc.firstChildElement( "song" );
	if ( ! rootNode.isNull() )
	{
		setType( "song" );
		setAuthor( rootNode.read_string( "author", "undefined author", false, false ) );
		setLicense( H2Core::License( rootNode.read_string( "license", "", false, false ) ) );
		setName( rootNode.read_string( "name", "", false, false ) );
		setInfo( rootNode.read_string( "info", "No information available.", false, false ) );

		bLoadingWorked = true;
	}

	if ( ! bLoadingWorked ) {
		ERRORLOG( QString( "[%1] could not be loaded as pattern, song, or drumkit" )
				  .arg( sPath ) );
		return false;
	}

	return true;
}

SoundLibraryInfo::~SoundLibraryInfo()
{
	//default deconstructor
}

}; //namespace H2Core

