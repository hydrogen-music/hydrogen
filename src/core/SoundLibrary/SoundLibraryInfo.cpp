/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

SoundLibraryInfo::SoundLibraryInfo( const QString& sPath )
{
	/*
	 *Use the provided file instantiate this object with the corresponding meta
	 *data from either a drumkit, a pattern or a song.
	 */
	
	setPath( sPath );

	XMLDoc doc;
	if ( ! doc.read( sPath, nullptr, true ) ) {
		ERRORLOG( QString( "Unable to load SoundLibraryInfo from [%1]" )
				  .arg( sPath ) );
		return;
	}

	XMLNode rootNode =  doc.firstChildElement( "drumkit_pattern" );
	if ( ! rootNode.isNull() )
	{
		setType( "pattern" );
		setAuthor( rootNode.read_string( "author", "undefined author", false, false ) );
		setLicense( H2Core::License( rootNode.read_string( "license", "", false, false ) ) );

		XMLNode patternNode = rootNode.firstChildElement( "pattern" );
		// Try legacy format fist.
		setName( patternNode.read_string( "pattern_name", "", false, false ) );
		if ( getName().isEmpty() ) {
			// Try current format.
			setName( patternNode.read_string( "name", "", false, false ) );
		}
		setInfo( patternNode.read_string( "info", "No information available.", false, false ) );
		setCategory( patternNode.read_string( "category", "", false, false ) );
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

		//setCategory( rootNode.read_string( "category", "" ) );
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
		//setCategory( rootNode.read_string( "category", "" ) );
	}
}

SoundLibraryInfo::~SoundLibraryInfo()
{
	//default deconstructor
}

}; //namespace H2Core

