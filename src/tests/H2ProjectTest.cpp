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
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include "H2ProjectTest.h"

#include "TestHelper.h"
#include "assertions/File.h"

#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/H2Project.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

#include <QtCore/QDir>

#include <map>

using namespace H2Core;

namespace {

// Collect, in stable (inst/comp/layer) order, the loaded frame count of every
// sample of a song's kit. Empty/missing samples report 0.
std::map<QString, long long> sampleFrames( std::shared_ptr<Song> pSong ) {
	std::map<QString, long long> result;
	auto pInstruments = pSong->getDrumkit()->getInstruments();
	for ( int ii = 0; ii < pInstruments->size(); ++ii ) {
		auto pInstrument = pInstruments->get( ii );
		if ( pInstrument == nullptr ) {
			continue;
		}
		auto pComponents = pInstrument->getComponents();
		for ( int cc = 0; cc < static_cast<int>( pComponents->size() ); ++cc ) {
			auto pComponent = ( *pComponents )[ cc ];
			if ( pComponent == nullptr ) {
				continue;
			}
			const auto layers = pComponent->getLayers();
			for ( int ll = 0; ll < static_cast<int>( layers.size() ); ++ll ) {
				auto pLayer = layers[ ll ];
				if ( pLayer == nullptr || pLayer->getSample() == nullptr ) {
					continue;
				}
				result[ QString( "%1/%2/%3" ).arg( ii ).arg( cc ).arg( ll ) ] =
					pLayer->getSample()->isLoaded()
						? pLayer->getSample()->getFrames()
						: 0;
			}
		}
	}

	if ( pSong->getPlaybackTrackInstrument() != nullptr &&
		 pSong->getPlaybackTrackInstrument()->getComponent( 0 ) != nullptr &&
		 pSong->getPlaybackTrackInstrument()->getComponent( 0 )->getLayer( 0
		 ) != nullptr &&
		 pSong->getPlaybackTrackInstrument()
				 ->getComponent( 0 )
				 ->getLayer( 0 )
		 ->getSample() != nullptr ) {
		result["-1/0/0"] = pSong->getPlaybackTrackInstrument()
								   ->getComponent( 0 )
								   ->getLayer( 0 )
								   ->getSample()
								   ->isLoaded()
							   ? pSong->getPlaybackTrackInstrument()
									 ->getComponent( 0 )
									 ->getLayer( 0 )
									 ->getSample()
									 ->getFrames()
							   : 0;
	}

	return result;
}

std::shared_ptr<Song> makeLoadedSong() {
	auto* pHydrogen = pTestHydrogen();
	auto pSong = Song::getEmptySong( pHydrogen );
	if ( pSong != nullptr && pSong->getDrumkit() != nullptr ) {
		pSong->getDrumkit()->loadSamples(
			120, pHydrogen->getPreferences().get() );
	}
	return pSong;
}

std::shared_ptr<Instrument> makePlaybackTrack( const QString& sPath )
{
	auto pPlaybackInstrument =
		std::make_shared<Instrument>( Instrument::PlaybackTrackId );
	pPlaybackInstrument->setName( "PlaybackTrack" );
	auto pSample = std::make_shared<Sample>( sPath );
	auto pLayer = std::make_shared<InstrumentLayer>( pSample );
	pPlaybackInstrument->addLayer(
		pPlaybackInstrument->getComponent( 0 ), pLayer, 0,
		Event::Trigger::Suppress, pTestHydrogen()
	);
	pPlaybackInstrument->loadSamples(
		120, pTestHydrogen()->getPreferences().get()
	);
	return pPlaybackInstrument;
}

// A project is meant to make a song including all its samples portable. In
// order to check this, we create a new drumkit (temporary paths and new file
// names) and discard it after saving the .h2project file and before loading it
// again. The only way to retrieve the samples is now to obtain it from the
// projects. No fallbacks should work.
std::shared_ptr<Drumkit> makeNewDrumkit() {
	auto pDrumkit = std::make_shared<Drumkit>();
	pDrumkit->setName( "Temp-Project-Kit" );
	pDrumkit->setContext( Filesystem::Context::Custom );

	auto pInstrument =
		std::make_shared<Instrument>( Instrument::Id( 0 ), "temp-instr" );
	auto pSample = std::make_shared<Sample>(
		H2TEST_FILE( "drumkits/sampleKit/longSample.flac" )
	);
	auto pLayer = std::make_shared<InstrumentLayer>( pSample );
	pInstrument->addLayer(
		pInstrument->getComponent( 0 ), pLayer, 0, Event::Trigger::Suppress,
		pTestHydrogen()
	);
	pDrumkit->getInstruments()->add( pInstrument );

	CPPUNIT_ASSERT( ! pDrumkit->hasMissingSamples() );

	return pDrumkit;
}

} // namespace

// song + kit -> .h2project bundle (in memory) -> reconstructs identically,
// including the per-instrument order that drives the bus mapping (ADR 0019/0025).
void H2ProjectTest::testBufferRoundTrip() {
	___INFOLOG( "" );

	const QString sTmpDir =
		Filesystem::tmpDir() + "/h2project-buffer-round-trip/drumkit.xml";
	const QString sTmpPlaybackTrack =
		Filesystem::tmpFilePath( "playback.flac" );
	CPPUNIT_ASSERT( Filesystem::fileCopy(
		H2TEST_FILE( "song/res/playbackTrack.flac" ), sTmpPlaybackTrack, true
	) );

	auto pHydrogen = pTestHydrogen();
	auto pSong = Song::getEmptySong( pHydrogen );
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pDrumkit = makeNewDrumkit();
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	pDrumkit->save( sTmpDir, false );
	pSong->setDrumkit( pDrumkit );
	pSong->getDrumkit()->loadSamples(
		120, pHydrogen->getPreferences().get() );

	auto pPlaybackTrack = makePlaybackTrack( sTmpPlaybackTrack );
	CPPUNIT_ASSERT( pPlaybackTrack != nullptr );
	pSong->setPlaybackTrackInstrument( pPlaybackTrack );

	const int nInstruments = pSong->getDrumkit()->getInstruments()->size();
	CPPUNIT_ASSERT( nInstruments > 0 );

	const auto framesBefore = sampleFrames( pSong );
	CPPUNIT_ASSERT( ! framesBefore.empty() );

	const auto bundle = H2Project::toBuffer( pSong, true );
	CPPUNIT_ASSERT( ! bundle.empty() );
	CPPUNIT_ASSERT( H2Project::looksLikeArchive( bundle ) );

	// Remove the backing drumkit from disk. All samples must be retrieved from
	// the project.
	Filesystem::rm( sTmpDir, true );
	Filesystem::rm( sTmpPlaybackTrack );

	auto pReconstructed = H2Project::fromBuffer( bundle, pTestHydrogen(), true );
	CPPUNIT_ASSERT( pReconstructed != nullptr );
	CPPUNIT_ASSERT( pReconstructed->getDrumkit() != nullptr );

	// Same instrument count and order (the 1-to-1 bus mapping rides on this).
	auto pInstr1 = pSong->getDrumkit()->getInstruments();
	auto pInstr2 = pReconstructed->getDrumkit()->getInstruments();
	CPPUNIT_ASSERT_EQUAL( pInstr1->size(), pInstr2->size() );
	for ( int ii = 0; ii < pInstr1->size(); ++ii ) {
		CPPUNIT_ASSERT_EQUAL( pInstr1->get( ii )->getName().toStdString(),
							  pInstr2->get( ii )->getName().toStdString() );
	}

	// Every sample decoded from the bundle and matches the original frame count.
	const auto framesAfter = sampleFrames( pReconstructed );
	CPPUNIT_ASSERT_EQUAL( framesBefore.size(), framesAfter.size() );
	for ( const auto& [ sKey, nFrames ] : framesBefore ) {
		CPPUNIT_ASSERT( framesAfter.find( sKey ) != framesAfter.end() );
		CPPUNIT_ASSERT( framesAfter.at( sKey ) > 0 ); // decoded from memory
		CPPUNIT_ASSERT_EQUAL( nFrames, framesAfter.at( sKey ) );
	}

	___INFOLOG( "passed" );
}

// The XML-buffer serialisers that carry individual Drumkit / Instrument /
// Pattern objects across the editor↔engine IPC split (ADR 0030) round-trip
// in memory without touching disk.
void H2ProjectTest::testBasicsBufferRoundTrip() {
	___INFOLOG( "" );

	auto* pHydrogen = pTestHydrogen();
	auto pSong = makeLoadedSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pDrumkit = pSong->getDrumkit();
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	CPPUNIT_ASSERT( pDrumkit->getInstruments()->size() > 0 );

	// --- Drumkit ---
	const auto kitBuffer = pDrumkit->toXmlBuffer(
		Xml::Flag::SongKit | Xml::Flag::KeepMissingSamples, true );
	CPPUNIT_ASSERT( ! kitBuffer.isEmpty() );
	auto pKit2 = Drumkit::fromXmlBuffer(
		kitBuffer, "", Xml::Flag::SongKit, true, pHydrogen );
	CPPUNIT_ASSERT( pKit2 != nullptr );

	// --- Instrument ---
	auto pInstr = pDrumkit->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstr != nullptr );
	const auto instrBuffer = pInstr->toXmlBuffer(
		Xml::Flag::SongKit | Xml::Flag::KeepMissingSamples, true );
	CPPUNIT_ASSERT( ! instrBuffer.isEmpty() );
	auto pInstr2 = Instrument::fromXmlBuffer(
		instrBuffer, Xml::Flag::SongKit, true, pHydrogen );
	CPPUNIT_ASSERT( pInstr2 != nullptr );

	// --- Pattern ---
	auto pPatternList = pSong->getPatternList();
	CPPUNIT_ASSERT( pPatternList != nullptr );
	CPPUNIT_ASSERT( pPatternList->size() > 0 );
	auto pPattern = pPatternList->get( 0 );
	CPPUNIT_ASSERT( pPattern != nullptr );
	const auto patBuffer = pPattern->toXmlBuffer( pDrumkit, true );
	CPPUNIT_ASSERT( ! patBuffer.isEmpty() );
	auto pPattern2 = Pattern::fromXmlBuffer(
		patBuffer, pDrumkit, true, pHydrogen->getSoundLibraryDatabase() );
	CPPUNIT_ASSERT( pPattern2 != nullptr );

	// --- Playlist ---
	auto pPlaylist = std::make_shared<Playlist>();
	// Use an OS-absolute base so the path round-trips verbatim on every platform.
	// "/tmp/..." is absolute on Unix but NOT on Windows (no drive letter): there
	// load_from() treats it as relative and resolves it against the CWD, yielding
	// "C:/tmp/...". QDir::rootPath() is "/" on Unix and "C:/" (current drive) on
	// Windows, so the entry is genuinely absolute — and thus preserved as-is —
	// everywhere.
	const QString sEntrySongPath = QDir::rootPath() + "tmp/ipc-playlist-song.h2song";
	const QString sEntryScriptPath = QDir::rootPath() + "tmp/ipc-playlist-script.sh";
	auto pEntry = std::make_shared<PlaylistEntry>(
		sEntrySongPath, sEntryScriptPath, true );
	CPPUNIT_ASSERT( pPlaylist->add( pEntry ) );
	const auto plBuffer = pPlaylist->toXmlBuffer();
	CPPUNIT_ASSERT( ! plBuffer.isEmpty() );
	auto pPlaylist2 = Playlist::fromXmlBuffer( plBuffer );
	CPPUNIT_ASSERT( pPlaylist2 != nullptr );
	CPPUNIT_ASSERT( pPlaylist2->get( 0 ) != nullptr );
	// Absolute song path round-trips verbatim (no relative resolution).
	CPPUNIT_ASSERT_EQUAL( pEntry->getSongPath().toStdString(),
						  pPlaylist2->get( 0 )->getSongPath().toStdString() );
	CPPUNIT_ASSERT_EQUAL( pEntry->getScriptEnabled(),
						  pPlaylist2->get( 0 )->getScriptEnabled() );

	___INFOLOG( "passed" );
}

// The same round-trip via a file on disk.
void H2ProjectTest::testFileRoundTrip() {
	___INFOLOG( "" );

	const QString sTmpDir =
		Filesystem::tmpDir() + "/h2project-file-round-trip/drumkit.xml";
	const QString sTmpPlaybackTrack =
		Filesystem::tmpFilePath( "playback.flac" );
	CPPUNIT_ASSERT( Filesystem::fileCopy(
		H2TEST_FILE( "song/res/playbackTrack.flac" ), sTmpPlaybackTrack, true
	) );

	auto* pHydrogen = pTestHydrogen();
	auto pSong = Song::getEmptySong( pHydrogen );
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pDrumkit = makeNewDrumkit();
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	pDrumkit->save( sTmpDir, false );
	pSong->setDrumkit( pDrumkit );
	pSong->getDrumkit()->loadSamples(
		120, pHydrogen->getPreferences().get() );

	auto pPlaybackTrack = makePlaybackTrack( sTmpPlaybackTrack );
	CPPUNIT_ASSERT( pPlaybackTrack != nullptr );
	pSong->setPlaybackTrackInstrument( pPlaybackTrack );

	const auto framesBefore = sampleFrames( pSong );

	const QString sPath = Filesystem::tmpFilePath( "roundtrip.h2project" );
	CPPUNIT_ASSERT( H2Project::save( pSong, sPath, true ) );
	CPPUNIT_ASSERT( Filesystem::fileExists( sPath, true ) );

	// Remove the backing drumkit from disk. All samples must be retrieved from
	// the project.
	Filesystem::rm( sTmpDir, true );
	Filesystem::rm( sTmpPlaybackTrack );

	auto pReconstructed = H2Project::load( sPath, pTestHydrogen(), true );
	CPPUNIT_ASSERT( pReconstructed != nullptr );
	CPPUNIT_ASSERT_EQUAL( sPath.toStdString(),
						  pReconstructed->getPath().toStdString() );

	const auto framesAfter = sampleFrames( pReconstructed );
	CPPUNIT_ASSERT_EQUAL( framesBefore.size(), framesAfter.size() );
	for ( const auto& [ sKey, nFrames ] : framesBefore ) {
		CPPUNIT_ASSERT( framesAfter.find( sKey ) != framesAfter.end() );
		CPPUNIT_ASSERT_EQUAL( nFrames, framesAfter.at( sKey ) );
	}

	// Storing the project again must yield the same content. The archives
	// are not compared byte by byte - tar/gzip headers are not reproducible -
	// but their extracted content.
	const QString sPath2 = Filesystem::tmpFilePath( "roundtrip-2.h2project" );
	CPPUNIT_ASSERT( H2Project::save( pReconstructed, sPath2, true ) );
	H2TEST_ASSERT_TAR_ARCHIVES_EQUAL( sPath, sPath2 );

	Filesystem::rm( sPath, false, true );
	Filesystem::rm( sPath2, false, true );

	___INFOLOG( "passed" );
}

// The container detector tells `.h2project` archives from `.h2song` XML so the
// unified open path can route correctly (T4b.3).
void H2ProjectTest::testContainerDetection() {
	___INFOLOG( "" );

	auto pSong = makeLoadedSong();
	const auto bundle = H2Project::toBuffer( pSong, true );
	CPPUNIT_ASSERT( ! bundle.empty() );
	CPPUNIT_ASSERT( H2Project::looksLikeArchive( bundle ) );

	// An .h2song XML document must NOT look like an archive.
	const QByteArray songXml =
		pSong->toXmlBuffer( Xml::Flag::KeepMissingSamples | Xml::Flag::Ipc );
	std::vector<unsigned char> xml(
		reinterpret_cast<const unsigned char*>( songXml.constData() ),
		reinterpret_cast<const unsigned char*>( songXml.constData() ) +
			songXml.size() );
	CPPUNIT_ASSERT( ! H2Project::looksLikeArchive( xml ) );

	___INFOLOG( "passed" );
}

// One open endpoint loads both a `.h2song` and a `.h2project` (T4b.3).
void H2ProjectTest::testUnifiedOpen() {
	___INFOLOG( "" );

	auto pSong = makeLoadedSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	const int nInstruments = pSong->getDrumkit()->getInstruments()->size();

	const QString sSongPath = Filesystem::tmpFilePath( "unified.h2song" );
	const QString sProjPath = Filesystem::tmpFilePath( "unified.h2project" );
	CPPUNIT_ASSERT( pSong->save( sSongPath, true, true ) );
	CPPUNIT_ASSERT( H2Project::save( pSong, sProjPath, true ) );

	// The same endpoint loads either container.
	auto pFromSong = H2Project::openSong( sSongPath, pTestHydrogen(), true );
	CPPUNIT_ASSERT( pFromSong != nullptr );
	CPPUNIT_ASSERT_EQUAL( nInstruments,
						  pFromSong->getDrumkit()->getInstruments()->size() );

	auto pFromProject = H2Project::openSong( sProjPath, pTestHydrogen(), true );
	CPPUNIT_ASSERT( pFromProject != nullptr );
	CPPUNIT_ASSERT_EQUAL( nInstruments,
						  pFromProject->getDrumkit()->getInstruments()->size() );

	Filesystem::rm( sSongPath, false, true );
	Filesystem::rm( sProjPath, false, true );

	___INFOLOG( "passed" );
}
