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

#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include <core/Basics/Drumkit.h>
#include <core/Helpers/Filesystem.h>

namespace H2Core
{

SoundLibraryDatabase::SoundLibraryDatabase()
{
	
	m_patternInfoVector = new soundLibraryInfoVector();
	updateDrumkits();
	updatePatterns();
}

SoundLibraryDatabase::~SoundLibraryDatabase()
{
	//Clean up the patterns data structure
	for ( auto pPatternInfo : *m_patternInfoVector ) {
		delete pPatternInfo;
	}

	for ( auto drumkitEntry : m_drumkitDatabase ) {
		delete drumkitEntry.second;
	}
	
	delete m_patternInfoVector;
}

void SoundLibraryDatabase::printPatterns() const
{
	for ( const auto& pPatternInfo : *m_patternInfoVector ) {
		INFOLOG( QString( "Name: [%1]" ).arg( pPatternInfo->getName() ) );
	}

	for ( const auto& sCategory : m_patternCategories ) {
		INFOLOG( QString( "Category: [%1]" ).arg( sCategory ) );
	}
}

bool SoundLibraryDatabase::isPatternInstalled( const QString& sPatternName ) const
{
	for ( const auto& pPatternInfo : *m_patternInfoVector ) {
		if ( pPatternInfo->getName() == sPatternName ) {
			return true;
		}
	}
	return false;
}

void SoundLibraryDatabase::update()
{
	updatePatterns();
	//updateSongs();
	updateDrumkits();
}

void SoundLibraryDatabase::updateDrumkits() {
	m_drumkitDatabase.clear();

	QStringList drumkitPaths;
	for ( const auto& sDrumkitName : Filesystem::sys_drumkit_list() ) {
		drumkitPaths << 
			Filesystem::absolute_path( Filesystem::sys_drumkits_dir() + sDrumkitName );
	}
	for ( const auto& sDrumkitName : Filesystem::usr_drumkit_list() ) {
		drumkitPaths <<
			Filesystem::absolute_path( Filesystem::usr_drumkits_dir() + sDrumkitName );
	}

	for ( const auto& sDrumkitPath : drumkitPaths ) {
		Drumkit* pDrumkit = Drumkit::load( sDrumkitPath, false );
		if ( pDrumkit != nullptr ) {
			if ( m_drumkitDatabase.find( sDrumkitPath ) !=
				 m_drumkitDatabase.end() ) {
				ERRORLOG( QString( "A drumkit was already loaded from [%1]. Something went wrong." )
						  .arg( sDrumkitPath ) );
				continue;
			}

			m_drumkitDatabase[ sDrumkitPath ] = pDrumkit;
		}
	}
}

Drumkit* SoundLibraryDatabase::getDrumkit( const QString& sDrumkitPath ) const {
	return m_drumkitDatabase.at( Filesystem::absolute_path( sDrumkitPath ) );
}

void SoundLibraryDatabase::updatePatterns()
{
	for ( auto ppPattern : *m_patternInfoVector ) {
		delete ppPattern;
	}
	m_patternInfoVector->clear();
	m_patternCategories = QStringList();

	// search drumkit subdirectories within patterns user directory
	foreach ( const QString& sDrumkit, Filesystem::pattern_drumkits() ) {
		getPatternFromDirectory( Filesystem::patterns_dir( sDrumkit ), m_patternInfoVector);
	}
	// search patterns user directory
	getPatternFromDirectory( Filesystem::patterns_dir(), m_patternInfoVector);
}

void SoundLibraryDatabase::getPatternFromDirectory( const QString& sPatternDir, std::vector<SoundLibraryInfo*>* m_patternInfoVector )
{
	foreach ( const QString& sName, Filesystem::pattern_list( sPatternDir ) ) {
		QString sFile = sPatternDir + sName;
		SoundLibraryInfo* pSoundLibraryInfo = new SoundLibraryInfo( sFile );
		m_patternInfoVector->push_back( pSoundLibraryInfo );
		
		if ( ! m_patternCategories.contains( pSoundLibraryInfo->getCategory() ) ) {
			m_patternCategories << pSoundLibraryInfo->getCategory();
		}
	}
}
};
