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
#include <core/SoundLibrary/DrumkitInfo.h>
#include <core/SoundLibrary/PatternInfo.h>
#include <core/SoundLibrary/SongInfo.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>

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

	std::vector<Filesystem::Context> contexts;
	if ( bStacked ) {
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
					for ( const auto& [_, ppDrumkit] : m_drumkitDatabase ) {
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
					for ( const auto& ppPatternInfo : m_patternInfos ) {
						if ( ppPatternInfo != nullptr &&
							 ppPatternInfo->getName() == sName ) {
							if ( ppPatternInfo->getContext() == ccontext ) {
								return ppPatternInfo->getPath();
							}
						}
						else if ( bStacked ) {
							cachedArtifacts.push_back( std::make_pair(
								ppPatternInfo->getPath(),
								ppPatternInfo->getContext()
							) );
						}
					}

				case Filesystem::Artifact::Song:
					for ( const auto& ppSongInfo : m_songInfos ) {
						if ( ppSongInfo != nullptr &&
							 ppSongInfo->getName() == sName ) {
							if ( ppSongInfo->getContext() == ccontext ) {
								return ppSongInfo->getPath();
							}
						}
						else if ( bStacked ) {
							cachedArtifacts.push_back( std::make_pair(
								ppSongInfo->getPath(), ppSongInfo->getContext()
							) );
						}
					}

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

	EventQueue::get_instance()->pushEvent(
		Event::Type::SoundLibraryChanged, 0
	);
}

void SoundLibraryDatabase::updateDrumkits( Event::Trigger trigger )
{
	m_drumkitDatabase.clear();
    m_drumkitInfos.clear();

	QStringList drumkitPaths;
	drumkitPaths << Filesystem::listContent(
		Filesystem::Artifact::DrumkitExtracted, Filesystem::Context::System
	);
	drumkitPaths << Filesystem::listContent(
		Filesystem::Artifact::DrumkitExtracted, Filesystem::Context::User
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
					 !m_customDrumkitPaths.contains( sFilePath ) ) {
					m_customDrumkitPaths << sFilePath;
				}
			}
		}
	}
#endif

	// Either of the two session contexts does the job.
	drumkitPaths << Filesystem::listContent(
		Filesystem::Artifact::DrumkitExtracted,
		Filesystem::Context::SessionReadOnly
	);

	// custom drumkits added by the user
	for ( const auto& sDrumkitPath : m_customDrumkitPaths ) {
		if ( !drumkitPaths.contains( sDrumkitPath ) ) {
			drumkitPaths << sDrumkitPath;
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

			auto pInfo = DrumkitInfo::from( pDrumkit );
			if ( pInfo != nullptr ) {
				m_drumkitInfos.push_back( pInfo );
				registerUniqueLabel( pInfo );
			}
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
SoundLibraryDatabase::getDrumkit( const QString& sDrumkitPath, bool bUpgrade )
{
	if ( sDrumkitPath.isEmpty() ) {
		ERRORLOG( "No drumkit path provided" );
		return nullptr;
	}

	if ( m_drumkitDatabase.find( sDrumkitPath ) == m_drumkitDatabase.end() ) {
		INFOLOG( QString( "Drumkit [%1] not found in DB. Loading from file" )
					 .arg( sDrumkitPath ) );
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

		auto pInfo = DrumkitInfo::from( pDrumkit );
		if ( pInfo != nullptr ) {
			m_drumkitInfos.push_back( pInfo );
			registerUniqueLabel( pInfo );
		}

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
	std::shared_ptr<SoundLibraryInfo> pInfo
)
{
    if ( pInfo == nullptr ) {
        return;
    }

	// Ensure uniqueness of the label.
	int nCount = 1;
	QString sUniqueItemLabel = pInfo->getName();

	auto labelContained = [&]( const QString& sLabel ) {
		switch ( pInfo->getArtifact() ) {
			case Filesystem::Artifact::DrumkitExtracted: {
				for ( const auto& ppInfo : m_drumkitInfos ) {
					// Ensure we do not pick up the label for this kit.
					if ( ppInfo != nullptr && ppInfo->getLabel() == sLabel &&
						 ppInfo->getPath() != pInfo->getPath() ) {
						return true;
					}
				}
				return false;
			}
			case Filesystem::Artifact::Pattern: {
				for ( const auto& ppInfo : m_patternInfos ) {
					// Ensure we do not pick up the label for this kit.
					if ( ppInfo != nullptr && ppInfo->getLabel() == sLabel &&
						 ppInfo->getPath() != pInfo->getPath() ) {
						return true;
					}
				}
				return false;
			}
			case Filesystem::Artifact::Song: {
				for ( const auto& ppInfo : m_songInfos ) {
					// Ensure we do not pick up the label for this kit.
					if ( ppInfo != nullptr && ppInfo->getLabel() == sLabel &&
						 ppInfo->getPath() != pInfo->getPath() ) {
						return true;
					}
				}
				return false;
			}
			default:
				ERRORLOG( QString( "Unsupported artifact [%1]" )
							  .arg( Filesystem::ArtifactToQString(
								  pInfo->getArtifact()
							  ) ) )
				return false;
		}
	};

	while ( labelContained( sUniqueItemLabel ) ) {
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
			registerUniqueLabel( pInfo );
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
			registerUniqueLabel( pInfo );
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
