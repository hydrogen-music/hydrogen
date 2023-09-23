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

#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include <core/Basics/Drumkit.h>
#include <core/EventQueue.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>

namespace H2Core
{

SoundLibraryDatabase::SoundLibraryDatabase()
{
	update();
}

SoundLibraryDatabase::~SoundLibraryDatabase()
{
}

void SoundLibraryDatabase::printPatterns() const
{
	for ( const auto& pPatternInfo : m_patternInfoVector ) {
		INFOLOG( QString( "Name: [%1]" ).arg( pPatternInfo->getName() ) );
	}

	for ( const auto& sCategory : m_patternCategories ) {
		INFOLOG( QString( "Category: [%1]" ).arg( sCategory ) );
	}
}

bool SoundLibraryDatabase::isPatternInstalled( const QString& sPatternName ) const
{
	for ( const auto& pPatternInfo : m_patternInfoVector ) {
		if ( pPatternInfo->getName() == sPatternName ) {
			return true;
		}
	}
	return false;
}

void SoundLibraryDatabase::update()
{
	updatePatterns( false );
	//updateSongs();
	updateDrumkits( false );
	
	EventQueue::get_instance()->push_event( EVENT_SOUND_LIBRARY_CHANGED, 0 );
}

void SoundLibraryDatabase::updateDrumkits( bool bTriggerEvent ) {

	m_drumkitDatabase.clear();

	QStringList drumkitPaths;
	// system drumkits
	for ( const auto& sDrumkitName : Filesystem::sys_drumkit_list() ) {
		drumkitPaths << 
			Filesystem::absolute_path( Filesystem::sys_drumkits_dir() + sDrumkitName );
	}
	// user drumkits
	for ( const auto& sDrumkitName : Filesystem::usr_drumkit_list() ) {
		drumkitPaths <<
			Filesystem::absolute_path( Filesystem::usr_drumkits_dir() + sDrumkitName );
	}
	// custom drumkits added by the user
	for ( const auto& sDrumkitPath : m_customDrumkitPaths ) {
		if ( ! drumkitPaths.contains( sDrumkitPath ) ) {
			drumkitPaths << sDrumkitPath;
		}
	}

	for ( const auto& sDrumkitPath : drumkitPaths ) {
		auto pDrumkit = Drumkit::load( sDrumkitPath );
		if ( pDrumkit != nullptr ) {
			if ( m_drumkitDatabase.find( sDrumkitPath ) !=
				 m_drumkitDatabase.end() ) {
				ERRORLOG( QString( "A drumkit was already loaded from [%1]. Something went wrong." )
						  .arg( sDrumkitPath ) );
				continue;
			}

			INFOLOG( QString( "Drumkit [%1] loaded from [%2]" )
					 .arg( pDrumkit->get_name() ).arg( sDrumkitPath ) );

			m_drumkitDatabase.emplace(
				std::make_pair( sDrumkitPath,
								(DrumkitElement){ pDrumkit, loadDrumkitMap( sDrumkitPath ) } ) );
		}
		else {
			ERRORLOG( QString( "Unable to load drumkit at [%1]" ).arg( sDrumkitPath ) );
		}
	}

	if ( bTriggerEvent ) {
		EventQueue::get_instance()->push_event( EVENT_SOUND_LIBRARY_CHANGED, 0 );
	}
}

void SoundLibraryDatabase::updateDrumkit( const QString& sDrumkitPath, bool bTriggerEvent ) {

	auto pDrumkit = Drumkit::load( sDrumkitPath );
	if ( pDrumkit != nullptr ) {
		m_drumkitDatabase[ sDrumkitPath ] =
			(DrumkitElement){ pDrumkit, loadDrumkitMap( sDrumkitPath ) };
	}
	else {
		ERRORLOG( QString( "Unable to load drumkit at [%1]" ).arg( sDrumkitPath ) );
	}

	if ( bTriggerEvent ) {
		EventQueue::get_instance()->push_event( EVENT_SOUND_LIBRARY_CHANGED, 0 );
	}
}

std::shared_ptr<Drumkit> SoundLibraryDatabase::getDrumkit( const QString& sDrumkit, bool bLoad ) {

	// Convert supplied path or drumkit name into absolute path used
	// either as ID to retrieve the drumkit from cache or for loading
	// it from disk in case it is not present yet.

	QString sDrumkitPath;
	if ( sDrumkit.contains( "/" ) || sDrumkit.contains( "\\" ) ) {
		// Supplied string is a path to a drumkit
		sDrumkitPath = sDrumkit;
	}
	else {
		// Supplied string it the name of a drumkit
		sDrumkitPath = Filesystem::drumkit_path_search( sDrumkit,
														Filesystem::Lookup::stacked,
														false );
	}
	sDrumkitPath = Filesystem::absolute_path( sDrumkitPath );

	if ( sDrumkitPath.isEmpty() ) {
		ERRORLOG( QString( "Unable determine drumkit path based on supplied string [%1]" )
				  .arg( sDrumkit ) );
		return nullptr;
	}

	if ( m_drumkitDatabase.find( sDrumkitPath ) ==
		 m_drumkitDatabase.end() ) {
		if ( ! bLoad ) {
			return nullptr;
		}

		// Drumkit is not present in database yet. We attempt to load
		// and add it.
		auto pDrumkit = Drumkit::load( sDrumkitPath,
									   true, // upgrade
									   false // bSilent
									   );
		if ( pDrumkit == nullptr ) {
			return nullptr;
		}

		m_customDrumkitPaths << sDrumkitPath;

		m_drumkitDatabase[ sDrumkitPath ] =
			(DrumkitElement){ pDrumkit, loadDrumkitMap( sDrumkitPath ) };
		
		INFOLOG( QString( "Session Drumkit [%1] loaded from [%2]" )
				  .arg( pDrumkit->get_name() )
				  .arg( sDrumkitPath ) );

		EventQueue::get_instance()->push_event( EVENT_SOUND_LIBRARY_CHANGED, 0 );
		
		return pDrumkit;
	}
	
	return m_drumkitDatabase.at( sDrumkitPath ).drumkit;
}

std::multimap<int, QString> SoundLibraryDatabase::loadDrumkitMap( const QString& sDrumkitPath ) {

	std::multimap<int, QString> map;

	const QString sMapFile = Filesystem::getDrumkitMapFile( sDrumkitPath );

	DEBUGLOG( QString( "map file [%1] found for drumkit [%2]" )
			  .arg( sMapFile ).arg( sDrumkitPath ) );

	if ( ! sMapFile.isEmpty() ) {
		// A map file was found.

		XMLDoc doc;
		if ( ! doc.read( sMapFile, Filesystem::drumkit_map_xsd_path(), true ) ) {
			WARNINGLOG( QString( "Mapping file [%1] found for [%2] is not valid. It's loading might fail." )
						.arg( sMapFile ).arg( sDrumkitPath ) );
		}

		XMLNode rootNode = doc.firstChildElement( "drumkit_map" );
		if ( ! rootNode.isNull() ) {
			XMLNode mappingNode = rootNode.firstChildElement( "mapping" );

			while ( ! mappingNode.isNull() ) {
				const QString sGID =
					mappingNode.read_string( "gid", "", false, false, false );
				const int nInstrumentID =
					mappingNode.read_int( "instrumentID", -1, false, false, false );

				if ( ! sGID.isEmpty() && nInstrumentID != -1 ) {
					map.emplace( std::pair( nInstrumentID, sGID ) );
				}

				// Move on to the next entry until there is none left.
				mappingNode = rootNode.nextSiblingElement( "mapping" );
			}
		}
	}

	return map;
}

void SoundLibraryDatabase::updatePatterns( bool bTriggerEvent )
{
	m_patternInfoVector.clear();
	m_patternCategories = QStringList();

	// search drumkit subdirectories within patterns user directory
	foreach ( const QString& sDrumkit, Filesystem::pattern_drumkits() ) {
		loadPatternFromDirectory( Filesystem::patterns_dir( sDrumkit ) );
	}
	// search patterns user directory
	loadPatternFromDirectory( Filesystem::patterns_dir() );

	if ( bTriggerEvent ) {
		EventQueue::get_instance()->push_event( EVENT_SOUND_LIBRARY_CHANGED, 0 );
	}
}

void SoundLibraryDatabase::loadPatternFromDirectory( const QString& sPatternDir )
{
	foreach ( const QString& sName, Filesystem::pattern_list( sPatternDir ) ) {
		QString sFile = sPatternDir + sName;
		std::shared_ptr<SoundLibraryInfo> pInfo =
			std::make_shared<SoundLibraryInfo>();

		if ( pInfo->load( sFile ) ) {
			INFOLOG( QString( "Pattern [%1] of category [%2] loaded from [%3]" )
					 .arg( pInfo->getName() ).arg( pInfo->getCategory() )
					 .arg( sFile ) );
			
			m_patternInfoVector.push_back( pInfo );
		
			if ( ! m_patternCategories.contains( pInfo->getCategory() ) ) {
				m_patternCategories << pInfo->getCategory();
			}
		}
	}
}

QString SoundLibraryDatabase::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[SoundLibraryDatabase]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_drumkitDatabase:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& [ ssPath, ddrumkitElement ] : m_drumkitDatabase ) {
			sOutput.append( QString( "%1%2%2%3: %4\n" ).arg( sPrefix ).arg( s )
							.arg( ssPath ).arg( ddrumkitElement.drumkit->toQString( "", true ) ) )
				.append( QString( "%1%2%2%2mapping:\n" ).arg( sPrefix ).arg( s ) );
			for ( const auto& [ nnInstrumentId, ssGID ] : ddrumkitElement.map ) {
				sOutput.append( QString( "%1%2%2%2%2%3: %4\n" ).arg( sPrefix ).arg( s )
								.arg( nnInstrumentId ).arg( ssGID ) );
			}
		}
		sOutput.append( QString( "%1%2m_patternInfoVector:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& ppatternInfo : m_patternInfoVector ) {
			sOutput.append( QString( "%3\n" )
							.arg( ppatternInfo->toQString( sPrefix + s + s, bShort ) ) );
		}
		sOutput.append( QString( "%1%2m_patternCategories: %3\n" ).arg( sPrefix ).arg( s )
						.arg( m_patternCategories.join( ", " ) ) );
		sOutput.append( QString( "%1%2m_customDrumkitPaths:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& ssCustomPath : m_customDrumkitPaths ) {
			sOutput.append( QString( "%1%2%2%3\n" ).arg( sPrefix ).arg( s )
							.arg( ssCustomPath ) );
		}
	}
	else {

		sOutput = QString( "%1[SoundLibraryDatabase]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_drumkitDatabase:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& eentry : m_drumkitDatabase ) {
			sOutput.append( QString( "%1%2%2%3: mapping: [" ).arg( sPrefix ).arg( s )
							.arg( eentry.first ) );
			for ( const auto& [ nnInstrumentId, ssGID ] : eentry.second.map ) {
				sOutput.append( QString( " %1: %2," ).arg( nnInstrumentId ).arg( ssGID ) );
			}
			sOutput.append( "]\n" );

		}
		sOutput.append( QString( "%1%2m_patternInfoVector:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& ppatternInfo : m_patternInfoVector ) {
			sOutput.append( QString( "%1%2%2%3\n" ).arg( sPrefix ).arg( s )
							.arg( ppatternInfo->getPath() ) );
		}
		sOutput.append( QString( "%1%2m_patternCategories: %3\n" ).arg( sPrefix ).arg( s )
						.arg( m_patternCategories.join( ", " ) ) );
		sOutput.append( QString( "%1%2m_customDrumkitPaths:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& ssCustomPath : m_customDrumkitPaths ) {
			sOutput.append( QString( "%1%2%2%3\n" ).arg( sPrefix ).arg( s )
							.arg( ssCustomPath ) );
		}
	}

	return sOutput;
}
};
