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
#include <core/SoundLibrary/PatternInfo.h>
#include <core/SoundLibrary/SongInfo.h>

namespace H2Core {

SoundLibraryDatabase::SoundLibraryDatabase()
{
	update();
}

SoundLibraryDatabase::~SoundLibraryDatabase()
{
}

QString SoundLibraryDatabase::findArtifact(
	Filesystem::Artifact artifact,
	Filesystem::Context context,
	const QString& sName
) const
{
	switch ( artifact ) {
		case Filesystem::Artifact::DrumkitBundled:
			ERRORLOG( "Bundled drumkits aren't installed in the Sound Library"
			);
			return "";

		case Filesystem::Artifact::DrumkitExtracted:
			for ( const auto& [_, ppDrumkit] : m_drumkitDatabase ) {
				if ( ppDrumkit != nullptr && ppDrumkit->getName() == sName &&
					 ppDrumkit->getContext() == context ) {
					return ppDrumkit->getPath();
				}
			}
			return "";
			break;

		case Filesystem::Artifact::Pattern:
			for ( const auto& ppPatternInfo : m_patternInfos ) {
				if ( ppPatternInfo != nullptr &&
					 ppPatternInfo->getName() == sName &&
					 ppPatternInfo->getContext() == context ) {
					return ppPatternInfo->getPath();
				}
			}
			return "";

		case Filesystem::Artifact::Playlist:
			ERRORLOG( "Bundled playlists aren't installed in the Sound Library"
			);
			return "";

		case Filesystem::Artifact::Song:
			for ( const auto& ppSongInfo : m_songInfos ) {
				if ( ppSongInfo != nullptr && ppSongInfo->getName() == sName &&
					 ppSongInfo->getContext() == context ) {
					return ppSongInfo->getPath();
				}
			}
			return "";

		default:
			ERRORLOG( QString( "Unsupported artifact: [%1]" )
						  .arg( Filesystem::ArtifactToQString( artifact ) ) );
			return "";
	}
}

void SoundLibraryDatabase::update()
{
	updatePatterns( Event::Trigger::Suppress );
	updateSongs( Event::Trigger::Suppress );
	updateDrumkits( Event::Trigger::Suppress );

	EventQueue::get_instance()->pushEvent(
		Event::Type::SoundLibraryChanged, 0
	);
}

void SoundLibraryDatabase::updateDrumkits( Event::Trigger trigger )
{
	m_drumkitDatabase.clear();

	QStringList drumkitPaths;
	// system drumkits
	for ( const auto& sDrumkitName : Filesystem::systemDrumkitList() ) {
		drumkitPaths << Filesystem::absolutePath(
			Filesystem::systemDrumkitsDir() + sDrumkitName
		);
	}
	// user drumkits
	for ( const auto& sDrumkitName : Filesystem::userDrumkitList() ) {
		drumkitPaths << Filesystem::absolutePath(
			Filesystem::userDrumkitsDir() + sDrumkitName
		);
	}

#ifdef H2CORE_HAVE_APPIMAGE
	// When starting Hydrogen as an AppImage, all drumkits installed via the
	// package manager are not part of the system drumkit folder of this
	// instance. Instead, we treat them as custom drumkit.
	const auto additionalDirs = QStringList()
								<< "/usr/share/hydrogen/data/drumkits"
								<< "/usr/local/share/hydrogen/data/drumkits";
	for ( const auto& ssDir : additionalDirs ) {
		if ( Filesystem::dirExists( ssDir, true ) ) {
			for ( const auto& ssEntry : QDir( ssDir ).entryList(
					  QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot
				  ) ) {
				const auto sFilePath =
					QString( "%1/%2" ).arg( ssDir ).arg( ssEntry );
				if ( Filesystem::drumkitValid( sFilePath ) &&
					 !m_customDrumkitPaths.contains( sFilePath ) ) {
					m_customDrumkitPaths << sFilePath;
				}
			}
		}
	}
#endif

	// custom drumkits added by the user
	for ( const auto& sDrumkitPath : m_customDrumkitPaths ) {
		if ( !drumkitPaths.contains( sDrumkitPath ) ) {
			drumkitPaths << sDrumkitPath;
		}
	}

	// search custom drumkit folders for valid kits. Be careful not to add
	// directories, which do not correspond to drumkits. This would lead to a
	// lot of false positive error messages.
	for ( const auto& sDrumkitFolder : m_customDrumkitFolders ) {
		for ( const auto& sDrumkitName :
			  Filesystem::drumkitList( sDrumkitFolder ) ) {
			drumkitPaths
				<< QDir( sDrumkitFolder ).absoluteFilePath( sDrumkitName );
		}
	}

	for ( const auto& sDrumkitPath : drumkitPaths ) {
		auto pDrumkit = Drumkit::load( sDrumkitPath );
		if ( pDrumkit != nullptr ) {
			if ( m_drumkitDatabase.find( sDrumkitPath ) !=
				 m_drumkitDatabase.end() ) {
				ERRORLOG( QString( "A drumkit was already loaded from [%1]. "
								   "Something went wrong." )
							  .arg( sDrumkitPath ) );
				continue;
			}

			INFOLOG( QString( "Drumkit [%1] loaded from [%2]" )
						 .arg( pDrumkit->getName() )
						 .arg( sDrumkitPath ) );

			m_drumkitDatabase[sDrumkitPath] = pDrumkit;
			registerUniqueLabel( sDrumkitPath, pDrumkit );
		}
		else {
			ERRORLOG(
				QString( "Unable to load drumkit at [%1]" ).arg( sDrumkitPath )
			);
		}
	}

	if ( trigger != Event::Trigger::Suppress ) {
		EventQueue::get_instance()->pushEvent(
			Event::Type::SoundLibraryChanged, 0
		);
	}
}

std::shared_ptr<Drumkit>
SoundLibraryDatabase::getDrumkit( const QString& sDrumkit, bool bUpgrade )
{
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
		sDrumkitPath = Filesystem::drumkitPathSearch(
			sDrumkit, Filesystem::Lookup::stacked, false
		);
	}
	sDrumkitPath = Filesystem::absolutePath( sDrumkitPath );

	if ( sDrumkitPath.isEmpty() ) {
		ERRORLOG(
			QString(
				"Unable determine drumkit path based on supplied string [%1]"
			)
				.arg( sDrumkit )
		);
		return nullptr;
	}

	if ( m_drumkitDatabase.find( sDrumkitPath ) == m_drumkitDatabase.end() ) {
		// Drumkit is not present in database yet. We attempt to load
		// and add it.
		auto pDrumkit = Drumkit::load(
			sDrumkitPath,
			true,	  // upgrade
			nullptr,  // do not check for legacy format
			false	  // bSilent
		);
		if ( pDrumkit == nullptr ) {
			return nullptr;
		}

		m_customDrumkitPaths << sDrumkitPath;

		m_drumkitDatabase[sDrumkitPath] = pDrumkit;
		registerUniqueLabel( sDrumkitPath, pDrumkit );

		INFOLOG( QString( "Session Drumkit [%1] loaded from [%2]" )
					 .arg( pDrumkit->getName() )
					 .arg( sDrumkitPath ) );

		EventQueue::get_instance()->pushEvent(
			Event::Type::SoundLibraryChanged, 0
		);

		return pDrumkit;
	}

	return m_drumkitDatabase.at( sDrumkitPath );
}

std::shared_ptr<Drumkit> SoundLibraryDatabase::getPreviousDrumkit() const
{
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return nullptr;
	}

	const auto sLastLoadedDrumkitPath = pSong->getLastLoadedDrumkitPath();
	const auto search = m_drumkitDatabase.find( sLastLoadedDrumkitPath );

	if ( sLastLoadedDrumkitPath.isEmpty() ||
		 search == m_drumkitDatabase.end() ) {
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

std::shared_ptr<Drumkit> SoundLibraryDatabase::getNextDrumkit() const
{
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return nullptr;
	}

	const auto sLastLoadedDrumkitPath = pSong->getLastLoadedDrumkitPath();
	const auto search = m_drumkitDatabase.find( sLastLoadedDrumkitPath );

	if ( sLastLoadedDrumkitPath.isEmpty() ||
		 search == m_drumkitDatabase.end() ||
		 std::next( m_drumkitDatabase.find( sLastLoadedDrumkitPath ), 1 ) ==
			 m_drumkitDatabase.end() ) {
		// In case we do not find the last loaded kit or it is located at the
		// very bottom, we start at the top.
		return m_drumkitDatabase.begin()->second;
	}

	return std::next( search, 1 )->second;
}

void SoundLibraryDatabase::registerUniqueLabel(
	const QString& sDrumkitPath,
	std::shared_ptr<Drumkit> pDrumkit
)
{
	QString sLabel = pDrumkit->getName();
	const auto drumkitContext = pDrumkit->getContext();

	if ( drumkitContext == Filesystem::Context::System ) {
		/*: suffix appended to a drumkit name in order to make in unique.*/
		QString sSuffix = QT_TRANSLATE_NOOP( "SoundLibraryDatabase", "system" );
		sLabel.append( QString( " (%1)" ).arg( sSuffix ) );
	}
	else if ( drumkitContext == Filesystem::Context::SessionReadOnly || drumkitContext == Filesystem::Context::SessionReadWrite ) {
		/*: suffix appended to a drumkit name in order to make in unique.*/
		QString sSuffix =
			QT_TRANSLATE_NOOP( "SoundLibraryDatabase", "session" );
		sLabel.append( QString( " (%1)" ).arg( sSuffix ) );
	}

	// Ensure uniqueness of the label.
	int nCount = 1;
	QString sUniqueItemLabel = sLabel;

	auto labelContained = [&]( const QString& sLabel ) {
		for ( const auto& [_, ssLabel] : m_drumkitUniqueLabels ) {
			if ( ssLabel == sLabel ) {
				return true;
			}
		}

		return false;
	};

	// Ensure we do not pick up the label for this kit.
	m_drumkitUniqueLabels[sDrumkitPath] = "";

	while ( labelContained( sUniqueItemLabel ) ) {
		sUniqueItemLabel = QString( "%1 (%2)" ).arg( sLabel ).arg( nCount );
		nCount++;

		if ( nCount > 1000 ) {
			// That's a bit much.
			ERRORLOG( "Something went wrong in determining an unique label" );
		}
	}

	m_drumkitUniqueLabels[sDrumkitPath] = sUniqueItemLabel;
}

QString SoundLibraryDatabase::getUniqueLabel( const QString& sDrumkitPath
) const
{
	if ( m_drumkitUniqueLabels.find( sDrumkitPath ) ==
		 m_drumkitUniqueLabels.end() ) {
		return "";
	}

	return m_drumkitUniqueLabels.at( Filesystem::absolutePath( sDrumkitPath )
	);
}

void SoundLibraryDatabase::registerDrumkitFolder( const QString& sDrumkitFolder
)
{
	if ( !m_customDrumkitFolders.contains( sDrumkitFolder ) ) {
		// On Windows the provided system dir needs cleaning and looks like this
		// [C:\\projects\\hydrogen/data/\\drumkits/]. For all other OSs this is
		// not necessary. But it does no harm either and might be a live safer
		// in some edge cases.
		m_customDrumkitFolders << QString( sDrumkitFolder )
									  .replace( "\\", "/" )
									  .replace( "//", "/" );
	}
}

QStringList SoundLibraryDatabase::getDrumkitFolders() const
{
	QStringList drumkitFolders( m_customDrumkitFolders );

	// On Windows the provided system dir needs cleaning and looks like this
	// [C:\\projects\\hydrogen/data/\\drumkits/]. For all other OSs this is not
	// necessary. But it does no harm either and might be a live safer in some
	// edge cases.
	drumkitFolders << Filesystem::systemDrumkitsDir()
						  .replace( "\\", "/" )
						  .replace( "//", "/" )
				   << Filesystem::userDrumkitsDir()
						  .replace( "\\", "/" )
						  .replace( "//", "/" );

	return drumkitFolders;
}

std::set<Instrument::Type> SoundLibraryDatabase::getAllTypes() const
{
	std::set<Instrument::Type> allTypes;
	for ( const auto& [_, ppDrumkit] : m_drumkitDatabase ) {
		if ( ppDrumkit != nullptr ) {
			allTypes.merge( ppDrumkit->getAllTypes() );
		}
	}

	return allTypes;
}

void SoundLibraryDatabase::updatePatterns( Event::Trigger trigger )
{
	m_patternInfos.clear();

	QStringList patternPaths;
	patternPaths << Filesystem::listContent(
		Filesystem::Artifact::Pattern, Filesystem::Context::System
	);
	patternPaths << Filesystem::listContent(
		Filesystem::Artifact::Pattern, Filesystem::Context::User
	);

	for ( const auto& ssPath : patternPaths ) {
		auto pInfo = std::make_shared<PatternInfo>();
		if ( pInfo->load( ssPath ) ) {
			INFOLOG( QString( "Pattern [%1] registered from [%2]" )
						 .arg( pInfo->getName() )
						 .arg( ssPath ) );
			m_patternInfos.push_back( pInfo );
		}
		else {
			WARNINGLOG(
				QString( "Unable to register pattern [%1]" ).arg( ssPath )
			);
		}
	}

	if ( trigger != Event::Trigger::Suppress ) {
		EventQueue::get_instance()->pushEvent(
			Event::Type::SoundLibraryChanged, 0
		);
	}
}

void SoundLibraryDatabase::updateSongs( Event::Trigger trigger )
{
	m_songInfos.clear();

	QStringList songPaths;
	songPaths << Filesystem::listContent(
		Filesystem::Artifact::Song, Filesystem::Context::System
	);
	songPaths << Filesystem::listContent(
		Filesystem::Artifact::Song, Filesystem::Context::User
	);

	for ( const auto& ssPath : songPaths ) {
		auto pInfo = std::make_shared<SongInfo>();
		if ( pInfo->load( ssPath ) ) {
			INFOLOG( QString( "Song [%1] registered from [%2]" )
						 .arg( pInfo->getName() )
						 .arg( ssPath ) );
			m_songInfos.push_back( pInfo );
		}
		else {
			WARNINGLOG(
				QString( "Unable to register song [%1]" ).arg( ssPath )
			);
		}
	}

	if ( trigger != Event::Trigger::Suppress ) {
		EventQueue::get_instance()->pushEvent(
			Event::Type::SoundLibraryChanged, 0
		);
	}
}

QString SoundLibraryDatabase::toQString( const QString& sPrefix, bool bShort )
	const
{
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( !bShort ) {
		sOutput = QString( "%1[SoundLibraryDatabase]\n" )
					  .arg( sPrefix )
					  .append( QString( "%1%2m_drumkitDatabase:\n" )
								   .arg( sPrefix )
								   .arg( s ) );
		for ( const auto& [ssPath, ddrumkit] : m_drumkitDatabase ) {
			sOutput.append( QString( "%1%2%2%3: %4\n" )
								.arg( sPrefix )
								.arg( s )
								.arg( ssPath )
								.arg( ddrumkit->toQString( "", true ) ) );
		}
		sOutput.append(
			QString( "%1%2m_drumkitUniqueLabels:\n" ).arg( sPrefix ).arg( s )
		);
		for ( const auto& [ssPath, ssLabel] : m_drumkitUniqueLabels ) {
			sOutput.append( QString( "%1%2%2%3: %4\n" )
								.arg( sPrefix )
								.arg( s )
								.arg( ssPath )
								.arg( ssLabel ) );
		}
		sOutput.append(
			QString( "%1%2m_patternInfoVector:\n" ).arg( sPrefix ).arg( s )
		);
		for ( const auto& ppatternInfo : m_patternInfos ) {
			sOutput.append( QString( "%3\n" ).arg(
				ppatternInfo->toQString( sPrefix + s + s, bShort )
			) );
		}
		sOutput.append(
			QString( "%1%2m_songInfoVector:\n" ).arg( sPrefix ).arg( s )
		);
		for ( const auto& pSongInfo : m_songInfos ) {
			sOutput.append( QString( "%3\n" ).arg(
				pSongInfo->toQString( sPrefix + s + s, bShort )
			) );
		}
		sOutput
			.append( QString( "%1%2m_customDrumkitPaths: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_customDrumkitPaths.join( ", " ) ) )
			.append( QString( "%1%2m_customDrumkitFolders: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_customDrumkitFolders.join( ", " ) ) );
	}
	else {
		sOutput = QString( "[SoundLibraryDatabase] " )
					  .append( "m_drumkitDatabase: " );
		for ( const auto& [ssPath, ppDrumkit] : m_drumkitDatabase ) {
			sOutput.append(
				QString( "[%1: %2] " ).arg( ssPath ).arg( ppDrumkit->getName() )
			);
		}
		sOutput.append( ", m_drumkitUniqueLabels: " );
		for ( const auto& [ssPath, ssLabel] : m_drumkitUniqueLabels ) {
			sOutput.append( QString( "[%1: %2] " ).arg( ssPath ).arg( ssLabel )
			);
		}
		sOutput.append( ", m_patternInfos: " );
		for ( const auto& ppatternInfo : m_patternInfos ) {
			sOutput.append( QString( "%1, " ).arg( ppatternInfo->getPath() ) );
		}
		sOutput.append( ", m_songInfos: " );
		for ( const auto& pSongInfo : m_songInfos ) {
			sOutput.append( QString( "%1, " ).arg( pSongInfo->getPath() ) );
		}
		sOutput
			.append( QString( ", m_customDrumkitPaths: %1" )
						 .arg( m_customDrumkitPaths.join( ", " ) ) )
			.append( QString( ", m_customDrumkitFolders: %1" )
						 .arg( m_customDrumkitFolders.join( ", " ) ) );
	}

	return sOutput;
}
};	// namespace H2Core
