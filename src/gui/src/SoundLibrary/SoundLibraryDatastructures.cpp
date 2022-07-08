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

#include "SoundLibraryImportDialog.h"
#include "SoundLibraryRepositoryDialog.h"
#include "SoundLibraryPanel.h"

#include "../Widgets/DownloadWidget.h"
#include "../HydrogenApp.h"
#include "../InstrumentRack.h"

#include <core/H2Exception.h>
#include <core/Preferences/Preferences.h>
#include <core/Basics/Drumkit.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>

#include "SoundLibraryDatastructures.h"

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Basics/Drumkit.h>

using namespace H2Core;

SoundLibraryDatabase* SoundLibraryDatabase::__instance = nullptr;

SoundLibraryDatabase::SoundLibraryDatabase()
{
	
	patternVector = new soundLibraryInfoVector();
	updatePatterns();
}

SoundLibraryDatabase::~SoundLibraryDatabase()
{
	//Clean up the patterns data structure
	soundLibraryInfoVector::iterator mapIterator;
	for( mapIterator=patternVector->begin(); mapIterator != patternVector->end(); mapIterator++ )
	{
		delete *mapIterator;
	}

	delete patternVector;
}

void SoundLibraryDatabase::create_instance()
{
	if ( __instance == nullptr ) {
		__instance = new SoundLibraryDatabase;
	}
}

void SoundLibraryDatabase::printPatterns()
{
	soundLibraryInfoVector::iterator mapIterator;
	for( mapIterator=patternVector->begin(); mapIterator != patternVector->end(); mapIterator++ )
	{
		INFOLOG(  QString( "Name: " + (*mapIterator)->getName() ) );
	}

	for (int i = 0; i < patternCategories.size(); ++i) 
	{
		INFOLOG( patternCategories.at(i) )
	}
}

bool SoundLibraryDatabase::isPatternInstalled( const QString& patternName)
{

	soundLibraryInfoVector::iterator mapIterator;
	for( mapIterator=patternVector->begin(); mapIterator != patternVector->end(); mapIterator++ )
	{
		if( (*mapIterator)->getName() == patternName )
		{
			return true;
		}
	}
	return false;
}

void SoundLibraryDatabase::update()
{
	updatePatterns();
	//updateSongs();
	//updateDrumkits();
}

void SoundLibraryDatabase::updatePatterns()
{
	for ( auto ppPattern : *patternVector ) {
		delete ppPattern;
	}
	patternVector->clear();
	patternCategories = QStringList();

	// search drumkit subdirectories within patterns user directory
	foreach ( const QString& drumkit, Filesystem::pattern_drumkits() ) {
		getPatternFromDirectory( Filesystem::patterns_dir( drumkit ), patternVector);
	}
	// search patterns user directory
	getPatternFromDirectory( Filesystem::patterns_dir(), patternVector);
}

void SoundLibraryDatabase::getPatternFromDirectory( const QString& sPatternDir, std::vector<SoundLibraryInfo*>* patternVector )
{
	foreach ( const QString& fName, Filesystem::pattern_list( sPatternDir ) ) {
		QString sFile = sPatternDir + fName;
		SoundLibraryInfo* slInfo = new SoundLibraryInfo( sFile );
		patternVector->push_back( slInfo );
		if ( !patternCategories.contains( slInfo->getCategory() ) ) {
			patternCategories << slInfo->getCategory();
		}
	}
}


soundLibraryInfoVector* SoundLibraryDatabase::getAllPatterns() const
{
	return patternVector;
}




SoundLibraryInfo::SoundLibraryInfo()
{
	//default constructor
}

SoundLibraryInfo::SoundLibraryInfo( const QString& sPath)
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
	if ( !rootNode.isNull() )
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
	if ( !rootNode.isNull() )
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
	if ( !rootNode.isNull() )
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




