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

#include <map>
#include <set>

#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include <core/Basics/Drumkit.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>

namespace H2Core
{

QString SoundLibraryDatabase::m_sPatternBaseCategory = "not_categorized";

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

	// search custom drumkit folders for valid kits. Be careful not to add
	// directories, which do not correspond to drumkits. This would lead to a
	// lot of false positive error messages.
	for ( const auto& sDrumkitFolder : m_customDrumkitFolders ) {
		for ( const auto& sDrumkitName : Filesystem::drumkit_list( sDrumkitFolder ) ) {
			drumkitPaths << QDir( sDrumkitFolder ).absoluteFilePath( sDrumkitName );
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
					 .arg( pDrumkit->getName() ).arg( sDrumkitPath ) );

			m_drumkitDatabase[ sDrumkitPath ] = pDrumkit;
			registerUniqueLabel( sDrumkitPath, pDrumkit );
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
		m_drumkitDatabase[ sDrumkitPath ] = pDrumkit;
		registerUniqueLabel( sDrumkitPath, pDrumkit );
	}
	else {
		ERRORLOG( QString( "Unable to load drumkit at [%1]" ).arg( sDrumkitPath ) );
	}

	if ( bTriggerEvent ) {
		EventQueue::get_instance()->push_event( EVENT_SOUND_LIBRARY_CHANGED, 0 );
	}
}

std::shared_ptr<Drumkit> SoundLibraryDatabase::getDrumkit( const QString& sDrumkit ) {

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

		m_drumkitDatabase[ sDrumkitPath ] = pDrumkit;
		registerUniqueLabel( sDrumkitPath, pDrumkit );
		
		INFOLOG( QString( "Session Drumkit [%1] loaded from [%2]" )
				  .arg( pDrumkit->getName() )
				  .arg( sDrumkitPath ) );

		EventQueue::get_instance()->push_event( EVENT_SOUND_LIBRARY_CHANGED, 0 );
		
		return pDrumkit;
	}
	
	return m_drumkitDatabase.at( sDrumkitPath );
}

std::shared_ptr<Drumkit> SoundLibraryDatabase::getPreviousDrumkit() const {

	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return nullptr;
	}

	const auto sLastLoadedDrumkitPath = pSong->getLastLoadedDrumkitPath();
	const auto search = m_drumkitDatabase.find( sLastLoadedDrumkitPath );

	if ( sLastLoadedDrumkitPath.isEmpty() || search == m_drumkitDatabase.end() ) {
		// In case we do not find the last loaded kit, we start at the top.
		return m_drumkitDatabase.begin()->second;
	}
	else if ( search == m_drumkitDatabase.begin() ) {
		// Periodic boundary conditions. The previous with respect to the first
		// one is the last.
		return std::prev( m_drumkitDatabase.end(), 1 )->second;
	}

	return std::prev( search, 1 )->second;
}

std::shared_ptr<Drumkit> SoundLibraryDatabase::getNextDrumkit() const {

	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return nullptr;
	}

	const auto sLastLoadedDrumkitPath = pSong->getLastLoadedDrumkitPath();
	const auto search = m_drumkitDatabase.find( sLastLoadedDrumkitPath );

	if ( sLastLoadedDrumkitPath.isEmpty() || search == m_drumkitDatabase.end() ||
		 std::next( m_drumkitDatabase.find( sLastLoadedDrumkitPath ), 1 ) ==
		 m_drumkitDatabase.end() ) {
		// In case we do not find the last loaded kit or it is located at the
		// very bottom, we start at the top.
		return m_drumkitDatabase.begin()->second;
	}

	return std::next( search, 1 )->second;
}

void SoundLibraryDatabase::registerUniqueLabel( const QString& sDrumkitPath,
												std::shared_ptr<Drumkit> pDrumkit ) {

	QString sLabel = pDrumkit->getName();
	const auto drumkitContext = pDrumkit->getContext();

	if ( drumkitContext == Drumkit::Context::System ) {
		/*: suffix appended to a drumkit name in order to make in unique.*/
		QString sSuffix = QT_TRANSLATE_NOOP( "SoundLibraryDatabase", "system" );
		sLabel.append( QString( " (%1)" ).arg( sSuffix ) );
	}
	else if ( drumkitContext == Drumkit::Context::SessionReadOnly ||
			  drumkitContext == Drumkit::Context::SessionReadWrite ) {
		/*: suffix appended to a drumkit name in order to make in unique.*/
		QString sSuffix = QT_TRANSLATE_NOOP( "SoundLibraryDatabase", "session" );
		sLabel.append( QString( " (%1)" ).arg( sSuffix ) );
	}

	// Ensure uniqueness of the label.
	int nCount = 1;
	QString sUniqueItemLabel = sLabel;

	auto labelContained = [&]( const QString& sLabel ){
		for ( const auto& [ _, ssLabel ] : m_drumkitUniqueLabels ) {
			if ( ssLabel == sLabel ) {
				return true;
			}
		}

		return false;
	};

	// Ensure we do not pick up the label for this kit.
	m_drumkitUniqueLabels[ sDrumkitPath ] = "";

	while ( labelContained( sUniqueItemLabel ) ) {
		sUniqueItemLabel = QString( "%1 (%2)" ).arg( sLabel ).arg( nCount );
		nCount++;

		if ( nCount > 1000 ) {
			// That's a bit much.
			ERRORLOG( "Something went wrong in determining an unique label" );
		}
	}

	m_drumkitUniqueLabels[ sDrumkitPath ] = sUniqueItemLabel;
}

QString SoundLibraryDatabase::getUniqueLabel( const QString& sDrumkitPath ) const {
	if ( m_drumkitUniqueLabels.find( sDrumkitPath ) ==
		 m_drumkitUniqueLabels.end() ) {
		return "";
	}

	return m_drumkitUniqueLabels.at( Filesystem::absolute_path( sDrumkitPath ) );
}

void SoundLibraryDatabase::registerDrumkitFolder( const QString& sDrumkitFolder ) {
	if ( ! m_customDrumkitFolders.contains( sDrumkitFolder ) ) {
		m_customDrumkitFolders << sDrumkitFolder;
	}
}

QStringList SoundLibraryDatabase::getDrumkitFolders() const {
	QStringList drumkitFolders( m_customDrumkitFolders );

	drumkitFolders << Filesystem::sys_drumkits_dir()
		<< Filesystem::usr_drumkits_dir();

	return drumkitFolders;
}

std::set<DrumkitMap::Type> SoundLibraryDatabase::getAllTypes() const {
	std::set<DrumkitMap::Type> allTypes;
	for ( const auto& [ _, ppDrumkit ] : m_drumkitDatabase ) {
		if ( ppDrumkit != nullptr ) {
			allTypes.merge( ppDrumkit->getAllTypes() );
		}
	}

	return allTypes;
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
		for ( const auto& [ ssPath, ddrumkit ] : m_drumkitDatabase ) {
			sOutput.append( QString( "%1%2%2%3: %4\n" ).arg( sPrefix ).arg( s )
							.arg( ssPath ).arg( ddrumkit->toQString( "", true ) ) );
		}
		sOutput.append( QString( "%1%2m_drumkitUniqueLabels:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& [ ssPath, ssLabel ] : m_drumkitUniqueLabels ) {
			sOutput.append( QString( "%1%2%2%3: %4\n" ).arg( sPrefix ).arg( s )
							.arg( ssPath ).arg( ssLabel ) );
		}
		sOutput.append( QString( "%1%2m_patternInfoVector:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& ppatternInfo : m_patternInfoVector ) {
			sOutput.append( QString( "%3\n" )
							.arg( ppatternInfo->toQString( sPrefix + s + s, bShort ) ) );
		}
		sOutput.append( QString( "%1%2m_patternCategories: %3\n" ).arg( sPrefix ).arg( s )
						.arg( m_patternCategories.join( ", " ) ) )
			.append( QString( "%1%2m_customDrumkitPaths: %3\n" ).arg( sPrefix ).arg( s )
						.arg( m_customDrumkitPaths.join( ", " ) ) )
			.append( QString( "%1%2m_customDrumkitFolders: %3\n" ).arg( sPrefix ).arg( s )
						.arg( m_customDrumkitFolders.join( ", " ) ) );
	}
	else {
		sOutput = QString( "[SoundLibraryDatabase] " )
			.append( "m_drumkitDatabase: " );
		for ( const auto& [ ssPath, ppDrumkit ] : m_drumkitDatabase ) {
			sOutput.append( QString( "[%1: %2] " )
							.arg( ssPath ).arg( ppDrumkit->getName() ) );
		}
		sOutput.append( ", m_drumkitUniqueLabels: " );
		for ( const auto& [ ssPath, ssLabel ] : m_drumkitUniqueLabels ) {
			sOutput.append( QString( "[%1: %2] " )
							.arg( ssPath ).arg( ssLabel ) );
		}
		sOutput.append( ", m_patternInfoVector: " );
		for ( const auto& ppatternInfo : m_patternInfoVector ) {
			sOutput.append( QString( "%1, " ).arg( ppatternInfo->getPath() ) );
		}
		sOutput.append( QString( ", m_patternCategories: %1" )
						.arg( m_patternCategories.join( ", " ) ) )
			.append( QString( ", m_customDrumkitPaths: %1" )
						.arg( m_customDrumkitPaths.join( ", " ) ) )
			.append( QString( ", m_customDrumkitFolders: %1" )
						.arg( m_customDrumkitFolders.join( ", " ) ) );
	}

	return sOutput;
}
};
