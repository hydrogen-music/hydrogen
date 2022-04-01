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

#include <core/LocalFileMng.h>
#include <core/H2Exception.h>
#include <core/Preferences/Preferences.h>
#include <core/Basics/Drumkit.h>
#include <core/Helpers/Filesystem.h>

#include "SoundLibraryDatastructures.h"

#include <core/Object.h>
#include <core/LocalFileMng.h>
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

SoundLibraryInfo::SoundLibraryInfo(const QString &path)
{
	/*
	 *Use the provided file instantiate this object with the corresponding meta
	 *data from either a drumkit, a pattern or a song.
	 */

	QDomDocument doc  = LocalFileMng::openXmlDocument( path );
	setPath( path );

	QDomNode rootNode =  doc.firstChildElement( "drumkit_pattern" );
	if ( !rootNode.isNull() )
	{
		setType( "pattern" );
		setAuthor( LocalFileMng::readXmlString( rootNode,"author", "undefined author" ) );
		setLicense( LocalFileMng::readXmlString( rootNode,"license", "undefined license" ) );

		QDomNode patternNode = rootNode.firstChildElement( "pattern" );
		setName( LocalFileMng::readXmlString( patternNode,"pattern_name", "" ) );
		if ( getName().isEmpty() ) {
			setName( LocalFileMng::readXmlString( patternNode,"name", "" ) );
		}
		setInfo( LocalFileMng::readXmlString( patternNode,"info", "No information available." ) );
		setCategory( LocalFileMng::readXmlString( patternNode,"category", "" ) );
	}


	//New drumkits
	rootNode = doc.firstChildElement( "drumkit_info" );
	if ( !rootNode.isNull() )
	{
		setType( "drumkit" );
		setAuthor( LocalFileMng::readXmlString( rootNode,"author", "undefined author" ) );
		setLicense( LocalFileMng::readXmlString( rootNode,"license", "undefined license" ) );
		setName( LocalFileMng::readXmlString( rootNode,"name", "" ) );
		setInfo( LocalFileMng::readXmlString( rootNode,"info", "No information available." ) );
		setImage( LocalFileMng::readXmlString( rootNode,"image", "" ) );
		setImageLicense( LocalFileMng::readXmlString( rootNode,"imageLicense", "undefined license" ) );

		//setCategory( LocalFileMng::readXmlString( rootNode,"category", "" ) );
	}

	//Songs
	rootNode = doc.firstChildElement( "song" );
	if ( !rootNode.isNull() )
	{
		setType( "song" );
		setAuthor( LocalFileMng::readXmlString( rootNode,"author", "undefined author" ) );
		setLicense( LocalFileMng::readXmlString( rootNode,"license", "undefined license" ) );
		setName( LocalFileMng::readXmlString( rootNode,"name", "" ) );
		setInfo( LocalFileMng::readXmlString( rootNode,"info", "No information available." ) );
		//setCategory( LocalFileMng::readXmlString( rootNode,"category", "" ) );
	}
}

SoundLibraryInfo::~SoundLibraryInfo()
{
	//default deconstructor
}




