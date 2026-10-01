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

#include <QDir>

#include <map>
#include <set>

#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include <core/Basics/Drumkit.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>
#include <core/SoundLibrary/DrumkitInfo.h>
#include <core/SoundLibrary/PatternInfo.h>
#include <core/SoundLibrary/SongInfo.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>

namespace H2Core {

SoundLibraryDatabase::SoundLibraryDatabase( Hydrogen* pHydrogen )
	: m_pHydrogen( pHydrogen ),
	  m_pSnapshot( std::make_shared<Snapshot>() )
{
	update();
}

SoundLibraryDatabase::~SoundLibraryDatabase()
{
}

std::shared_ptr<const SoundLibraryDatabase::Snapshot>
SoundLibraryDatabase::getSnapshot() const
{
	std::lock_guard<std::mutex> lock( m_writerMutex );
	return m_pSnapshot;
}

std::vector<std::shared_ptr<SoundLibraryInfo>>
SoundLibraryDatabase::getDrumkitInfos() const
{
	return getSnapshot()->drumkitInfos;
}

std::vector<std::shared_ptr<SoundLibraryInfo>>
SoundLibraryDatabase::getPatternInfos() const
{
	return getSnapshot()->patternInfos;
}

std::vector<std::shared_ptr<SoundLibraryInfo>>
SoundLibraryDatabase::getSongInfos() const
{
	return getSnapshot()->songInfos;
}

std::map<QString, std::shared_ptr<Drumkit>>
SoundLibraryDatabase::getDrumkitDatabase() const
{
	return getSnapshot()->drumkitDatabase;
}

void SoundLibraryDatabase::publish( std::shared_ptr<Snapshot> pNewSnapshot )
{
	std::lock_guard<std::mutex> lock( m_writerMutex );

	// Merge the custom drumkit registrations: entries added since the
	// new snapshot was built must not be lost to the rebuild.
	for ( const auto& ssPath : m_pSnapshot->customDrumkitPaths ) {
		if ( ! pNewSnapshot->customDrumkitPaths.contains( ssPath ) ) {
			pNewSnapshot->customDrumkitPaths << ssPath;
		}
	}
	for ( const auto& ssFolder : m_pSnapshot->customDrumkitFolders ) {
		if ( ! pNewSnapshot->customDrumkitFolders.contains( ssFolder ) ) {
			pNewSnapshot->customDrumkitFolders << ssFolder;
		}
	}

	m_pSnapshot = std::move( pNewSnapshot );
}

QString SoundLibraryDatabase::findArtifact(
	Filesystem::Artifact artifact,
	Filesystem::Context context,
	const QString& sName,
	bool bStacked
) const
{
	if ( artifact == Filesystem::Artifact::DrumkitBundled ||
		 artifact == Filesystem::Artifact::Playlist ) {
		ERRORLOG( QString( "%1 can not be installed in the Sound Library" )
					  .arg( Filesystem::ArtifactToQString( artifact ) ) );
		return "";
	}

	// Hold one snapshot for all passes: the lookup must not mix
	// content of different publications.
	const auto pSnapshot = getSnapshot();

	std::vector<Filesystem::Context> contexts;
	if ( bStacked ) {
		contexts.push_back( Filesystem::Context::Custom );
		contexts.push_back( Filesystem::Context::SessionReadOnly );
		contexts.push_back( Filesystem::Context::User );
		contexts.push_back( Filesystem::Context::System );
	}
	else {
		contexts.push_back( context );
	}

	// In stacked lookup, we first have to check all artifacts for one context
	// before preceeding to the next. But we can be clever and cache the
	// artifacts in the first pass which match by name but not by context.
	std::vector<std::pair<QString, Filesystem::Context>> cachedArtifacts;
	for ( const auto& ccontext : contexts ) {
		if ( cachedArtifacts.size() > 0 ) {
			// Starting from the second pass we can take a shortcut.
			for ( const auto& [ssPath, ccachedContext] : cachedArtifacts ) {
				if ( ccachedContext == ccontext ) {
					return ssPath;
				}
			}
		}
		else {
			// First pass
			switch ( artifact ) {
			case Filesystem::Artifact::DrumkitExtracted:
				for ( const auto& [_, ppDrumkit] : pSnapshot->drumkitDatabase ) {
						if ( ppDrumkit != nullptr &&
							 ppDrumkit->getName() == sName ) {
							if ( ppDrumkit->getContext() == ccontext ) {
								return ppDrumkit->getPath();
							}
							else if ( bStacked ) {
								cachedArtifacts.push_back( std::make_pair(
									ppDrumkit->getPath(),
									ppDrumkit->getContext()
								) );
							}
						}
					}
					break;

			case Filesystem::Artifact::Pattern:
				for ( const auto& ppPatternInfo : pSnapshot->patternInfos ) {
					if ( ppPatternInfo != nullptr &&
						 ppPatternInfo->getName() == sName ) {
						if ( ppPatternInfo->getContext() == ccontext ) {
							return ppPatternInfo->getPath();
						}
						else if ( bStacked ) {
							cachedArtifacts.push_back( std::make_pair(
								ppPatternInfo->getPath(),
								ppPatternInfo->getContext()
							) );
						}
					}
				}
				break;

			case Filesystem::Artifact::Song:
				for ( const auto& ppSongInfo : pSnapshot->songInfos ) {
					if ( ppSongInfo != nullptr &&
						 ppSongInfo->getName() == sName ) {
						if ( ppSongInfo->getContext() == ccontext ) {
							return ppSongInfo->getPath();
						}
						else if ( bStacked ) {
							cachedArtifacts.push_back( std::make_pair(
								ppSongInfo->getPath(), ppSongInfo->getContext()
							) );
						}
					}
				}
				break;

			default:
					ERRORLOG( QString( "Unsupported artifact: [%1]" )
								  .arg( Filesystem::ArtifactToQString( artifact
								  ) ) );
					return "";
			}
		}
	}
	return "";
}

void SoundLibraryDatabase::update()
{
	updatePatterns( Event::Trigger::Suppress );
	updateSongs( Event::Trigger::Suppress );
	updateDrumkits( Event::Trigger::Suppress );

	m_pHydrogen->getEventQueue()->pushEvent(
		Event::Type::SoundLibraryChanged, 0
	);
}

void SoundLibraryDatabase::updateDrumkits( Event::Trigger trigger )
{
	// Build the new content outside the publication lock: the listing
	// reaches back into the database (custom drumkit folders) and the
	// drumkit loads are expensive disk I/O which must not block
	// readers.
	auto pNewSnapshot = std::make_shared<Snapshot>( *getSnapshot() );
	pNewSnapshot->drumkitDatabase.clear();
	pNewSnapshot->drumkitInfos.clear();

	QStringList drumkitPaths;
	drumkitPaths << Filesystem::listContent(
		Filesystem::Artifact::DrumkitExtracted, Filesystem::Context::System,
		"", m_pHydrogen
	);
	drumkitPaths << Filesystem::listContent(
		Filesystem::Artifact::DrumkitExtracted, Filesystem::Context::User,
		"", m_pHydrogen
	);

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
				const auto sFilePath = QString( "%1/%2/%3" )
										   .arg( ssDir )
										   .arg( ssEntry )
										   .arg( Filesystem::drumkitXml() );
				if ( Filesystem::fileExists( sFilePath ) &&
					 ! pNewSnapshot->customDrumkitPaths.contains(
						 sFilePath ) ) {
					pNewSnapshot->customDrumkitPaths << sFilePath;
				}
			}
		}
	}
#endif

	// Either of the two session contexts does the job.
	drumkitPaths << Filesystem::listContent(
		Filesystem::Artifact::DrumkitExtracted,
		Filesystem::Context::SessionReadOnly,
		"", m_pHydrogen
	);
	drumkitPaths << Filesystem::listContent(
		Filesystem::Artifact::DrumkitExtracted, Filesystem::Context::Custom,
		"", m_pHydrogen
	);

	// custom drumkits added by the user
	for ( const auto& sDrumkitPath : pNewSnapshot->customDrumkitPaths ) {
		if ( ! drumkitPaths.contains( sDrumkitPath ) ) {
			drumkitPaths << sDrumkitPath;
		}
	}

	for ( const auto& sDrumkitPath : drumkitPaths ) {
		auto pDrumkit = Drumkit::load( sDrumkitPath, true, nullptr, false,
									   m_pHydrogen );
		if ( pDrumkit != nullptr ) {
			if ( pNewSnapshot->drumkitDatabase.find( sDrumkitPath ) !=
				 pNewSnapshot->drumkitDatabase.end() ) {
				ERRORLOG( QString( "A drumkit was already loaded from [%1]. "
								   "Something went wrong." )
							  .arg( sDrumkitPath ) );
				continue;
			}

			INFOLOG( QString( "Drumkit [%1] loaded from [%2]" )
						 .arg( pDrumkit->getName() )
						 .arg( sDrumkitPath ) );

			pNewSnapshot->drumkitDatabase[sDrumkitPath] = pDrumkit;

			auto pInfo = DrumkitInfo::from( pDrumkit );
			if ( pInfo != nullptr ) {
				pNewSnapshot->drumkitInfos.push_back( pInfo );
				registerUniqueLabel( pInfo, pNewSnapshot.get() );
			}
		}
		else {
			ERRORLOG(
				QString( "Unable to load drumkit at [%1]" ).arg( sDrumkitPath )
			);
		}
	}

	publish( std::move( pNewSnapshot ) );

	if ( trigger != Event::Trigger::Suppress ) {
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::SoundLibraryChanged, 0
		);
	}
}

QString SoundLibraryDatabase::canonicalDrumkitPath(
	const QString& sDrumkitPath )
{
	// Callers may address a kit by its folder or with redundant
	// separators. Canonicalizing keeps the database keyed by a single
	// form per kit, so such spellings hit the cached kit instead of
	// registering duplicates of it.
	return QDir::cleanPath(
		Filesystem::sanitizeDrumkitPath( sDrumkitPath ) );
}

std::shared_ptr<Drumkit>
SoundLibraryDatabase::getDrumkit( const QString& sDrumkitPath, bool bUpgrade )
{
	if ( sDrumkitPath.isEmpty() ) {
		ERRORLOG( "No drumkit path provided" );
		return nullptr;
	}

	const QString sCanonicalPath = canonicalDrumkitPath( sDrumkitPath );
	if ( sCanonicalPath.isEmpty() ) {
		ERRORLOG( QString( "[%1] is not a valid drumkit path" )
					  .arg( sDrumkitPath ) );
		return nullptr;
	}

	const auto pInitialSnapshot = getSnapshot();
	const auto foundInitial =
		pInitialSnapshot->drumkitDatabase.find( sCanonicalPath );
	if ( foundInitial != pInitialSnapshot->drumkitDatabase.end() ) {
		return foundInitial->second;
	}

	INFOLOG( QString( "Drumkit [%1] not found in DB. Loading from file" )
				 .arg( sDrumkitPath ) );
	// Drumkit is not present in database yet. We attempt to load and
	// add it. The load happens outside the publication lock: it is
	// expensive disk I/O and must not block readers or other writers.
	auto pDrumkit = Drumkit::load(
		sCanonicalPath,
		bUpgrade,
		nullptr,  // do not check for legacy format
		false,	  // bSilent
		m_pHydrogen
	);
	if ( pDrumkit == nullptr ) {
		return nullptr;
	}

	auto pInfo = DrumkitInfo::from( pDrumkit );

	std::lock_guard<std::mutex> lock( m_writerMutex );
	// Another writer may have published this kit (or a full rescan)
	// while we were loading. Re-check under the lock.
	const auto foundCurrent =
		m_pSnapshot->drumkitDatabase.find( sCanonicalPath );
	if ( foundCurrent != m_pSnapshot->drumkitDatabase.end() ) {
		return foundCurrent->second;
	}

	auto pNewSnapshot = std::make_shared<Snapshot>( *m_pSnapshot );
	if ( ! pNewSnapshot->customDrumkitPaths.contains( sCanonicalPath ) ) {
		pNewSnapshot->customDrumkitPaths << sCanonicalPath;
	}
	pNewSnapshot->drumkitDatabase[sCanonicalPath] = pDrumkit;
	if ( pInfo != nullptr ) {
		pNewSnapshot->drumkitInfos.push_back( pInfo );
		registerUniqueLabel( pInfo, pNewSnapshot.get() );
	}
	m_pSnapshot = std::move( pNewSnapshot );

	INFOLOG( QString( "Session Drumkit [%1] loaded from [%2]" )
				 .arg( pDrumkit->getName() )
				 .arg( sCanonicalPath ) );

	m_pHydrogen->getEventQueue()->pushEvent(
		Event::Type::SoundLibraryChanged, 0
	);

	return pDrumkit;
}

std::shared_ptr<Drumkit> SoundLibraryDatabase::getPreviousDrumkit() const
{
	auto pHydrogen = m_pHydrogen;
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return nullptr;
	}

	const auto pSnapshot = getSnapshot();
	const auto& drumkitDatabase = pSnapshot->drumkitDatabase;
	if ( drumkitDatabase.empty() ) {
		ERRORLOG( "No drumkits available" );
		return nullptr;
	}

	const auto sLastLoadedDrumkitPath = pSong->getLastLoadedDrumkitPath();
	const auto search = drumkitDatabase.find( sLastLoadedDrumkitPath );

	if ( sLastLoadedDrumkitPath.isEmpty() ||
		 search == drumkitDatabase.end() ) {
		// In case we do not find the last loaded kit, we start at the top.
		return drumkitDatabase.begin()->second;
	}
	else if ( search == drumkitDatabase.begin() ) {
		// Periodic boundary conditions. The previous with respect to the first
		// one is the last.
		return std::prev( drumkitDatabase.end(), 1 )->second;
	}

	return std::prev( search, 1 )->second;
}

std::shared_ptr<Drumkit> SoundLibraryDatabase::getNextDrumkit() const
{
	auto pHydrogen = m_pHydrogen;
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return nullptr;
	}

	const auto pSnapshot = getSnapshot();
	const auto& drumkitDatabase = pSnapshot->drumkitDatabase;
	if ( drumkitDatabase.empty() ) {
		ERRORLOG( "No drumkits available" );
		return nullptr;
	}

	const auto sLastLoadedDrumkitPath = pSong->getLastLoadedDrumkitPath();
	const auto search = drumkitDatabase.find( sLastLoadedDrumkitPath );

	if ( sLastLoadedDrumkitPath.isEmpty() ||
		 search == drumkitDatabase.end() ||
		 std::next( drumkitDatabase.find( sLastLoadedDrumkitPath ), 1 ) ==
			 drumkitDatabase.end() ) {
		// In case we do not find the last loaded kit or it is located at the
		// very bottom, we start at the top.
		return drumkitDatabase.begin()->second;
	}

	return std::next( search, 1 )->second;
}

void SoundLibraryDatabase::registerUniqueLabel(
	std::shared_ptr<SoundLibraryInfo> pInfo,
	Snapshot* pSnapshot
)
{
    if ( pInfo == nullptr || pSnapshot == nullptr ) {
        return;
    }

	// Ensure uniqueness of the label.
	int nCount = 1;
	QString sUniqueItemLabel = pInfo->getName();

	// For we display both read-write and read-only session kits within same
	// node in the sound library.
	auto contextMatch = [&]( Filesystem::Context context1,
							 Filesystem::Context context2 ) {
		if ( ( context1 == Filesystem::Context::SessionReadOnly ||
			   context1 == Filesystem::Context::SessionReadWrite ) &&
			 ( context2 == Filesystem::Context::SessionReadOnly ||
			   context2 == Filesystem::Context::SessionReadWrite ) ) {
			return true;
		}
		else {
			return context1 == context2;
		}
	};
	auto labelContained = [&]( const QString& sLabel,
							   Filesystem::Context context ) {
		switch ( pInfo->getType() ) {
			case SoundLibraryInfo::Type::Drumkit: {
				for ( const auto& ppInfo : pSnapshot->drumkitInfos ) {
					// Ensure we do not pick up the label for this kit.
					if ( ppInfo != nullptr && ppInfo->getLabel() == sLabel &&
						 contextMatch(
							 ppInfo->getContext(), pInfo->getContext()
						 ) &&
						 ppInfo->getPath() != pInfo->getPath() ) {
						return true;
					}
				}
				return false;
			}
			case SoundLibraryInfo::Type::Pattern: {
				for ( const auto& ppInfo : pSnapshot->patternInfos ) {
					// Ensure we do not pick up the label for this kit.
					if ( ppInfo != nullptr && ppInfo->getLabel() == sLabel &&
						 contextMatch(
							 ppInfo->getContext(), pInfo->getContext()
						 ) &&
						 ppInfo->getPath() != pInfo->getPath() ) {
						return true;
					}
				}
				return false;
			}
			case SoundLibraryInfo::Type::Song: {
				for ( const auto& ppInfo : pSnapshot->songInfos ) {
					// Ensure we do not pick up the label for this kit.
					if ( ppInfo != nullptr && ppInfo->getLabel() == sLabel &&
						 contextMatch(
							 ppInfo->getContext(), pInfo->getContext()
						 ) &&
						 ppInfo->getPath() != pInfo->getPath() ) {
						return true;
					}
				}
				return false;
			}
			default:
				ERRORLOG( QString( "Unsupported type [%1]" )
							  .arg( SoundLibraryInfo::TypeToQString(
								  pInfo->getType()
							  ) ) )
				return false;
		}
	};

	while ( labelContained( sUniqueItemLabel, pInfo->getContext() ) ) {
		sUniqueItemLabel =
			QString( "%1 (%2)" ).arg( pInfo->getName() ).arg( nCount );
		nCount++;

		if ( nCount > 1000 ) {
			// That's a bit much.
			ERRORLOG( "Something went wrong in determining an unique label" );
		}
	}

	pInfo->setLabel( sUniqueItemLabel );
}

void SoundLibraryDatabase::registerDrumkitFolder( const QString& sDrumkitFolder
)
{
	// On Windows the provided system dir needs cleaning and looks like this
	// [C:\\projects\\hydrogen/data/\\drumkits/]. For all other OSs this is
	// not necessary. But it does no harm either and might be a live saver
	// in some edge cases.
	const QString sCleanedFolder = QString( sDrumkitFolder )
										 .replace( "\\", "/" )
										 .replace( "//", "/" );

	std::lock_guard<std::mutex> lock( m_writerMutex );
	if ( m_pSnapshot->customDrumkitFolders.contains( sCleanedFolder ) ) {
		return;
	}

	auto pNewSnapshot = std::make_shared<Snapshot>( *m_pSnapshot );
	pNewSnapshot->customDrumkitFolders << sCleanedFolder;
	m_pSnapshot = std::move( pNewSnapshot );
}

void SoundLibraryDatabase::registerCustomDrumkitPath( const QString& sPath )
{
	if ( sPath.isEmpty() ) {
		return;
	}

	const QString sCanonicalPath = canonicalDrumkitPath( sPath );
	if ( sCanonicalPath.isEmpty() ) {
		ERRORLOG( QString( "[%1] is not a valid drumkit path" )
					  .arg( sPath ) );
		return;
	}

	std::lock_guard<std::mutex> lock( m_writerMutex );
	if ( m_pSnapshot->customDrumkitPaths.contains( sCanonicalPath ) ) {
		return;
	}

	auto pNewSnapshot = std::make_shared<Snapshot>( *m_pSnapshot );
	pNewSnapshot->customDrumkitPaths << sCanonicalPath;
	m_pSnapshot = std::move( pNewSnapshot );
}

QStringList SoundLibraryDatabase::getDrumkitFolders() const
{
	QStringList drumkitFolders( getSnapshot()->customDrumkitFolders );

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
	const auto pSnapshot = getSnapshot();
	for ( const auto& [_, ppDrumkit] : pSnapshot->drumkitDatabase ) {
		if ( ppDrumkit != nullptr ) {
			allTypes.merge( ppDrumkit->getAllTypes() );
		}
	}

	return allTypes;
}

void SoundLibraryDatabase::updatePatterns( Event::Trigger trigger )
{
	// Build the new content outside the publication lock (see
	// updateDrumkits()).
	auto pNewSnapshot = std::make_shared<Snapshot>( *getSnapshot() );
	pNewSnapshot->patternInfos.clear();

	QStringList patternPaths;
	patternPaths << Filesystem::listContent(
		Filesystem::Artifact::Pattern, Filesystem::Context::System,
		"", m_pHydrogen
	);
	patternPaths << Filesystem::listContent(
		Filesystem::Artifact::Pattern, Filesystem::Context::User,
		"", m_pHydrogen
	);
	patternPaths << Filesystem::listContent(
		Filesystem::Artifact::Pattern, Filesystem::Context::Custom,
		"", m_pHydrogen
	);

	for ( const auto& ssPath : patternPaths ) {
		auto pInfo = std::make_shared<PatternInfo>();
		if ( pInfo->load( ssPath, m_pHydrogen ) ) {
			INFOLOG( QString( "Pattern [%1] registered from [%2]" )
						 .arg( pInfo->getName() )
						 .arg( ssPath ) );
			pNewSnapshot->patternInfos.push_back( pInfo );
			registerUniqueLabel( pInfo, pNewSnapshot.get() );
		}
		else {
			WARNINGLOG(
				QString( "Unable to register pattern [%1]" ).arg( ssPath )
			);
		}
	}

	publish( std::move( pNewSnapshot ) );

	if ( trigger != Event::Trigger::Suppress ) {
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::SoundLibraryChanged, 0
		);
	}
}

void SoundLibraryDatabase::updateSongs( Event::Trigger trigger )
{
	// Build the new content outside the publication lock (see
	// updateDrumkits()).
	auto pNewSnapshot = std::make_shared<Snapshot>( *getSnapshot() );
	pNewSnapshot->songInfos.clear();

	QStringList songPaths;
	songPaths << Filesystem::listContent(
		Filesystem::Artifact::Song, Filesystem::Context::System,
		"", m_pHydrogen
	);
	songPaths << Filesystem::listContent(
		Filesystem::Artifact::Song, Filesystem::Context::User,
		"", m_pHydrogen
	);
	songPaths << Filesystem::listContent(
		Filesystem::Artifact::Song, Filesystem::Context::Custom,
		"", m_pHydrogen
	);

	for ( const auto& ssPath : songPaths ) {
		auto pInfo = std::make_shared<SongInfo>();
		if ( pInfo->load( ssPath, m_pHydrogen ) ) {
			INFOLOG( QString( "Song [%1] registered from [%2]" )
						 .arg( pInfo->getName() )
						 .arg( ssPath ) );
			pNewSnapshot->songInfos.push_back( pInfo );
			registerUniqueLabel( pInfo, pNewSnapshot.get() );
		}
		else {
			WARNINGLOG(
				QString( "Unable to register song [%1]" ).arg( ssPath )
			);
		}
	}

	publish( std::move( pNewSnapshot ) );

	if ( trigger != Event::Trigger::Suppress ) {
		m_pHydrogen->getEventQueue()->pushEvent(
			Event::Type::SoundLibraryChanged, 0
		);
	}
}

QString SoundLibraryDatabase::toQString( const QString& sPrefix, bool bShort )
	const
{
	QString s = Base::sPrintIndention;
	QString sOutput;
	// Hold one snapshot so the stringification is a coherent view
	// instead of a mix of different publications.
	const auto pSnapshot = getSnapshot();
	if ( !bShort ) {
		sOutput = QString( "%1[SoundLibraryDatabase]\n" )
					  .arg( sPrefix )
					  .append( QString( "%1%2m_drumkitDatabase:\n" )
								   .arg( sPrefix )
								   .arg( s ) );
		for ( const auto& [ssPath, ddrumkit] : pSnapshot->drumkitDatabase ) {
			sOutput.append( QString( "%1%2%2%3: %4\n" )
								.arg( sPrefix )
								.arg( s )
								.arg( ssPath )
								.arg( ddrumkit->toQString( "", true ) ) );
		}
		sOutput.append(
			QString( "%1%2m_patternInfoVector:\n" ).arg( sPrefix ).arg( s )
		);
		for ( const auto& ppatternInfo : pSnapshot->patternInfos ) {
			sOutput.append( QString( "%3\n" ).arg(
				ppatternInfo->toQString( sPrefix + s + s, bShort )
			) );
		}
		sOutput.append(
			QString( "%1%2m_songInfoVector:\n" ).arg( sPrefix ).arg( s )
		);
		for ( const auto& pSongInfo : pSnapshot->songInfos ) {
			sOutput.append( QString( "%3\n" ).arg(
				pSongInfo->toQString( sPrefix + s + s, bShort )
			) );
		}
		sOutput
			.append( QString( "%1%2m_customDrumkitPaths: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( pSnapshot->customDrumkitPaths.join( ", " ) ) )
			.append( QString( "%1%2m_customDrumkitFolders: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( pSnapshot->customDrumkitFolders.join( ", " ) ) );
	}
	else {
		sOutput = QString( "[SoundLibraryDatabase] " )
					  .append( "m_drumkitDatabase: " );
		for ( const auto& [ssPath, ppDrumkit] : pSnapshot->drumkitDatabase ) {
			sOutput.append(
				QString( "[%1: %2] " ).arg( ssPath ).arg( ppDrumkit->getName() )
			);
		}
		sOutput.append( ", m_patternInfos: " );
		for ( const auto& ppatternInfo : pSnapshot->patternInfos ) {
			sOutput.append( QString( "%1, " ).arg( ppatternInfo->getPath() ) );
		}
		sOutput.append( ", m_songInfos: " );
		for ( const auto& pSongInfo : pSnapshot->songInfos ) {
			sOutput.append( QString( "%1, " ).arg( pSongInfo->getPath() ) );
		}
		sOutput
			.append( QString( ", m_customDrumkitPaths: %1" )
						 .arg( pSnapshot->customDrumkitPaths.join( ", " ) ) )
			.append( QString( ", m_customDrumkitFolders: %1" )
						 .arg( pSnapshot->customDrumkitFolders.join( ", " ) ) );
	}

	return sOutput;
}
};	// namespace H2Core
