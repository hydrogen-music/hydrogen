/*
 * Hydrogen
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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

// First-boot smoke test for the plugin core (ADR 0038).
//
// Instantiates HydrogenPlugin the way a host does - in a fresh process, with
// no CLI options and no QCoreApplication - against an empty $HOME, so the
// plugin's own Logger/Base/Filesystem bootstrap runs exactly as on a user's
// first start. The CTest registration controls the environment:
//
//   with-system-data     $HYDROGEN_SYS_DATA_PATH points at the shipped data.
//   without-system-data  $HYDROGEN_SYS_DATA_PATH points at a missing folder.
//   user-data-only       like without-system-data; after a first boot a user
//                        drumkit is installed and a second instance must
//                        pick it up.
//
// Every mode requires that the plugin runs and that first boot leaves the
// same user-level state the editor creates: the user data folder tree and
// the user-level hydrogen.conf.

#include <plugin/HydrogenPlugin.h>

#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/Midi/Midi.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

using namespace H2Core;

namespace {

constexpr double fSampleRate = 48000;
constexpr unsigned nBlockSize = 512;
constexpr int nBuses = 2;

int nFailures = 0;

void check( bool bCondition, const QString& sWhat )
{
	if ( ! bCondition ) {
		++nFailures;
		std::fprintf( stderr, "FAILED: %s\n", sWhat.toLocal8Bit().constData() );
	}
}

/** Drives @a plugin like a host does for a few blocks with a note and
 * checks the output stays finite. */
void runBlocks( HydrogenPlugin& plugin )
{
	plugin.activate( fSampleRate, nBlockSize );
	std::vector<float> masterL( nBlockSize, 0.0f ), masterR( nBlockSize, 0.0f );
	std::vector<std::vector<float>> busL( nBuses ), busR( nBuses );
	std::vector<float*> busLPtr, busRPtr;
	for ( int ii = 0; ii < nBuses; ++ii ) {
		busL[ ii ].assign( nBlockSize, 0.0f );
		busR[ ii ].assign( nBlockSize, 0.0f );
		busLPtr.push_back( busL[ ii ].data() );
		busRPtr.push_back( busR[ ii ].data() );
	}

	plugin.noteOn( 36, 100, static_cast<int>( Midi::ChannelDefault ) );
	for ( int nBlock = 0; nBlock < 8; ++nBlock ) {
		HostTransportReading transport;
		transport.bValid = true;
		transport.bRolling = true;
		transport.fBpm = 120.0;
		transport.nFrame = static_cast<long long>( nBlock ) * nBlockSize;
		plugin.process( nBlockSize, masterL.data(), masterR.data(), busLPtr,
						busRPtr, transport );
		for ( unsigned nn = 0; nn < nBlockSize; ++nn ) {
			if ( ! std::isfinite( masterL[ nn ] ) ||
				 ! std::isfinite( masterR[ nn ] ) ) {
				check( false, "non-finite output sample" );
				return;
			}
		}
	}
	plugin.deactivate();
}

/** The user-level state the editor creates on its first start: the user
 * data folder tree (Filesystem::checkUserPaths) and the user-level config,
 * which must load back as a Hydrogen config. */
void checkUserLevelState()
{
	const QStringList dirs = { Filesystem::userDataPath(),
							   Filesystem::userDrumkitsDir(),
							   Filesystem::userPatternsDir(),
							   Filesystem::userPlaylistsDir(),
							   Filesystem::userScriptsDir(),
							   Filesystem::userSongsDir(),
							   Filesystem::userThemesDir(),
							   Filesystem::cacheDir(),
							   Filesystem::repositoriesCacheDir() };
	for ( const auto& sDir : dirs ) {
		check( QFileInfo( sDir ).isDir(),
			   QString( "user folder [%1] was not created" ).arg( sDir ) );
	}

	const QString sConfig = Filesystem::userConfigPath();
	check( QFileInfo( sConfig ).isFile(),
		   QString( "user-level config [%1] was not created" ).arg( sConfig ) );
	if ( QFileInfo( sConfig ).isFile() ) {
		check( Preferences::load( sConfig, true, nullptr ) != nullptr,
			   QString( "user-level config [%1] does not load" )
				   .arg( sConfig ) );
	}
}

bool copyDirectory( const QString& sSource, const QString& sTarget )
{
	if ( ! QDir().mkpath( sTarget ) ) {
		return false;
	}
	QDirIterator it( sSource, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
					 QDirIterator::Subdirectories );
	while ( it.hasNext() ) {
		const QString sPath = it.next();
		const QString sRelative = QDir( sSource ).relativeFilePath( sPath );
		const QString sDestination = QDir( sTarget ).filePath( sRelative );
		if ( it.fileInfo().isDir() ) {
			if ( ! QDir().mkpath( sDestination ) ) {
				return false;
			}
		}
		else if ( ! QFile::copy( sPath, sDestination ) ) {
			return false;
		}
	}
	return true;
}

/** Whether the instance's sound library lists a drumkit located inside the
 * user drumkit folder at @a sKitDir. */
bool soundLibraryHasKit( HydrogenPlugin& plugin, const QString& sKitDir )
{
	auto pDatabase = plugin.getHydrogen()->getSoundLibraryDatabase();
	pDatabase->waitForInitialScan();
	const QString sExpected = QFileInfo( sKitDir ).canonicalFilePath();
	for ( const auto& pInfo : pDatabase->getDrumkitInfos() ) {
		if ( pInfo == nullptr ) {
			continue;
		}
		// The info may name the kit folder or its drumkit.xml.
		const QFileInfo info( pInfo->getPath() );
		if ( info.canonicalFilePath() == sExpected ||
			 QFileInfo( info.absolutePath() ).canonicalFilePath() ==
				 sExpected ) {
			return true;
		}
	}
	return false;
}

} // namespace

int main( int argc, char** argv )
{
	if ( argc < 2 ) {
		std::fprintf( stderr,
					  "usage: %s with-system-data|without-system-data|"
					  "user-data-only [<user drumkit folder>]\n",
					  argv[ 0 ] );
		return 2;
	}
	const QString sMode = QString::fromLocal8Bit( argv[ 1 ] );
	const bool bUserDataOnly = sMode == "user-data-only";
	if ( sMode != "with-system-data" && sMode != "without-system-data" &&
		 ! bUserDataOnly ) {
		std::fprintf( stderr, "unknown mode [%s]\n", argv[ 1 ] );
		return 2;
	}
	if ( bUserDataOnly && argc < 3 ) {
		std::fprintf( stderr, "user-data-only requires a drumkit folder\n" );
		return 2;
	}

	// A leftover from a previous run would let the checks pass vacuously.
	const QString sHome = qEnvironmentVariable( "HOME" );
	const QDir home( sHome );
	if ( sHome.isEmpty() || ! home.exists() ||
		 ! home.entryList( QDir::AllEntries | QDir::NoDotAndDotDot |
						   QDir::Hidden ).isEmpty() ) {
		std::fprintf( stderr, "HOME [%s] must be an existing, empty folder\n",
					  sHome.toLocal8Bit().constData() );
		return 2;
	}

	// First boot.
	{
		HydrogenPlugin plugin( fSampleRate, nBlockSize, nBuses );
		runBlocks( plugin );
	}
	checkUserLevelState();

	if ( bUserDataOnly ) {
		// The user downloaded a kit; the shipped data is still missing. A
		// new instance must run on and offer the user-level kit.
		const QString sSource = QString::fromLocal8Bit( argv[ 2 ] );
		const QString sKitDir = QDir( Filesystem::userDrumkitsDir() )
									.filePath( QFileInfo( sSource ).fileName() );
		check( copyDirectory( sSource, sKitDir ),
			   QString( "unable to install [%1] into [%2]" )
				   .arg( sSource )
				   .arg( sKitDir ) );

		HydrogenPlugin plugin( fSampleRate, nBlockSize, nBuses );
		check( soundLibraryHasKit( plugin, sKitDir ),
			   QString( "user drumkit [%1] is not offered" ).arg( sKitDir ) );
		runBlocks( plugin );
	}

	if ( nFailures > 0 ) {
		std::printf( "PLUGIN FIRST BOOT (%s): FAILED (%d)\n",
					 sMode.toLocal8Bit().constData(), nFailures );
		return 1;
	}
	std::printf( "PLUGIN FIRST BOOT (%s): PASSED\n",
				 sMode.toLocal8Bit().constData() );
	return 0;
}
