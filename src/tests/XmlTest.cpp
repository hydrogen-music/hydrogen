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

#include "XmlTest.h"

#include "TestHelper.h"
#include "assertions/File.h"

#include <unistd.h>

#include <core/Basics/Drumkit.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Sample.h>
#include <core/CoreActionController.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>
#include <core/License.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include <QDir>
#include <QTemporaryDir>
#include <QTextStream>

using namespace H2Core;

static bool check_samples_data( std::shared_ptr<H2Core::Drumkit> dk, bool loaded )
{
	int count = 0;
	H2Core::InstrumentComponent::setMaxLayers( 16 );
	auto instruments = dk->get_instruments();
	for( int i=0; i<instruments->size(); i++ ) {
		count++;
		auto pInstr = ( *instruments )[i];
		for ( const auto& pComponent : *pInstr->get_components() ) {
			for ( int nLayer = 0; nLayer < H2Core::InstrumentComponent::getMaxLayers(); nLayer++ ) {
				auto pLayer = pComponent->get_layer( nLayer );
				if( pLayer ) {
					auto pSample = pLayer->get_sample();
					if( loaded ) {
						if( pSample->get_data_l()==nullptr || pSample->get_data_r()==nullptr ) {
							return false;
						}
					} else {
						if( pSample->get_data_l() != nullptr || pSample->get_data_r() != nullptr ) {
							return false;
						}
					}
				}

			}
		}
	}
	return ( count==4 );
}



void XmlTest::testDrumkit()
{
	___INFOLOG( "" );
	QString sDrumkitPath = H2Core::Filesystem::tmp_dir()+"dk0";

	std::shared_ptr<H2Core::Drumkit> pDrumkitLoaded = nullptr;
	std::shared_ptr<H2Core::Drumkit> pDrumkitReloaded = nullptr;
	std::shared_ptr<H2Core::Drumkit> pDrumkitCopied = nullptr;
	std::shared_ptr<H2Core::Drumkit> pDrumkitNew = nullptr;
	H2Core::XMLDoc doc;

	// load without samples
	pDrumkitLoaded = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/baseKit") );
	CPPUNIT_ASSERT( pDrumkitLoaded!=nullptr );
	CPPUNIT_ASSERT( pDrumkitLoaded->samples_loaded()==false );
	CPPUNIT_ASSERT( check_samples_data( pDrumkitLoaded, false ) );
	CPPUNIT_ASSERT_EQUAL( 4, pDrumkitLoaded->get_instruments()->size() );

	// Check if drumkit was valid (what we assume in this test)
	CPPUNIT_ASSERT( TestHelper::get_instance()->findDrumkitBackupFiles( "drumkits/baseKit/" )
					.size() == 0 );
	
	// manually load samples
	pDrumkitLoaded->load_samples();
	CPPUNIT_ASSERT( pDrumkitLoaded->samples_loaded()==true );
	CPPUNIT_ASSERT( check_samples_data( pDrumkitLoaded, true ) );

	pDrumkitLoaded = nullptr;
	
	// load with samples
	pDrumkitLoaded = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/baseKit" ) );
	CPPUNIT_ASSERT( pDrumkitLoaded!=nullptr );

	pDrumkitLoaded->load_samples();
	CPPUNIT_ASSERT( pDrumkitLoaded->samples_loaded()==true );
	CPPUNIT_ASSERT( check_samples_data( pDrumkitLoaded, true ) );
	
	// unload samples
	pDrumkitLoaded->unload_samples();
	CPPUNIT_ASSERT( pDrumkitLoaded->samples_loaded()==false );
	CPPUNIT_ASSERT( check_samples_data( pDrumkitLoaded, false ) );
	
	// save drumkit elsewhere
	pDrumkitLoaded->set_name( "pDrumkitLoaded" );
	CPPUNIT_ASSERT( pDrumkitLoaded->save( sDrumkitPath, true ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/drumkit.xml" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/crash.wav" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/hh.wav" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/kick.wav" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/snare.wav" ) );

	// Check whether the generated drumkit is valid.
	CPPUNIT_ASSERT( doc.read( H2Core::Filesystem::drumkit_file( sDrumkitPath ) ) );
	
	// load file
	pDrumkitReloaded = H2Core::Drumkit::load( sDrumkitPath );
	CPPUNIT_ASSERT( pDrumkitReloaded!=nullptr );
	
	// copy constructor
	pDrumkitCopied = std::make_shared<H2Core::Drumkit>( pDrumkitReloaded );
	CPPUNIT_ASSERT( pDrumkitCopied!=nullptr );
	// save file
	pDrumkitCopied->set_name( "COPY" );
	CPPUNIT_ASSERT( pDrumkitCopied->save( sDrumkitPath ) );

	pDrumkitReloaded = nullptr;

	// Check whether blank drumkits are valid.
	pDrumkitNew = std::make_shared<H2Core::Drumkit>();
	CPPUNIT_ASSERT( pDrumkitNew != nullptr );
	CPPUNIT_ASSERT( pDrumkitNew->save( sDrumkitPath ) );
	CPPUNIT_ASSERT( doc.read( H2Core::Filesystem::drumkit_file( sDrumkitPath ) ) );
	pDrumkitReloaded = H2Core::Drumkit::load( sDrumkitPath );
	CPPUNIT_ASSERT( pDrumkitReloaded != nullptr );

	// Cleanup
	H2Core::Filesystem::rm( sDrumkitPath, true );
	___INFOLOG( "passed" );
}

void XmlTest::testDrumkitLegacy()
{
	___INFOLOG( "" );

	QDir legacyDir( H2TEST_FILE( "drumkits/legacyKits" ) );

	// Check whether all contained kits can be loaded.
	for ( const auto& ssDir : legacyDir.entryList( QDir::Dirs |
													QDir::NoDotAndDotDot ) ) {
		___INFOLOG( ssDir );
		const auto pDrumkit = H2Core::Drumkit::load(
			legacyDir.filePath( ssDir ), false, nullptr, false );
		CPPUNIT_ASSERT( pDrumkit != nullptr );
		CPPUNIT_ASSERT( pDrumkit->get_instruments() != nullptr );
		CPPUNIT_ASSERT( pDrumkit->get_instruments() != nullptr );
		for ( const auto& ppInstrument : *pDrumkit->get_instruments() ) {
			CPPUNIT_ASSERT( ppInstrument != nullptr );
			CPPUNIT_ASSERT( ppInstrument->hasSamples() );
			CPPUNIT_ASSERT( ! ppInstrument->has_missing_samples() );
		}
	}

	// Check wether the names stored in the DrumkitComponents in version 0.9.7 -
	// 1.2.X are properly ported to InstrumentComponents.
	const auto pDrumkit = H2Core::Drumkit::load(
		H2TEST_FILE( "drumkits/legacyKits/kit-1.2.3" ), false, nullptr, false );
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	CPPUNIT_ASSERT( pDrumkit->get_instruments()->get( 0 ) != nullptr );
	CPPUNIT_ASSERT( pDrumkit->getComponent( 3 ) != nullptr );
	CPPUNIT_ASSERT( pDrumkit->getComponent( 36 ) != nullptr );
	CPPUNIT_ASSERT( pDrumkit->getComponent( 3 )->get_name() == "First" );
	CPPUNIT_ASSERT( pDrumkit->getComponent( 36 )->get_name() == "Second" );

	___INFOLOG( "passed" );
}

void XmlTest::testShippedDrumkits()
{
	___INFOLOG( "" );
	H2Core::XMLDoc doc;
	for ( auto ii : H2Core::Filesystem::sys_drumkit_list() ) {
		CPPUNIT_ASSERT( doc.read( QString( "%1%2/drumkit.xml" )
								  .arg( H2Core::Filesystem::sys_drumkits_dir() )
								  .arg( ii ) ) );

	}
	___INFOLOG( "passed" );
}

// Load drumkit which includes instrument with invalid ADSR values.
void XmlTest::testDrumkit_invalidADSRValues()
{
	___INFOLOG( "" );
	auto pTestHelper = TestHelper::get_instance();
	std::shared_ptr<H2Core::Drumkit> pDrumkit = nullptr;

	//1. Check, if the drumkit has been loaded
	pDrumkit = H2Core::Drumkit::load( H2TEST_FILE( "drumkits/invAdsrKit") );
	CPPUNIT_ASSERT( pDrumkit != nullptr );

	//2. Make sure that the instruments of the drumkit have been loaded correctly (see GH issue #839)
	auto pInstruments = pDrumkit->get_instruments();
	CPPUNIT_ASSERT( pInstruments != nullptr );

	auto pFirstInstrument = pInstruments->get(0);
	CPPUNIT_ASSERT( pFirstInstrument != nullptr );

	auto pLayer = pFirstInstrument->get_components()->front()->get_layer(0);
	CPPUNIT_ASSERT( pLayer != nullptr );

	auto pSample = pLayer->get_sample();
	CPPUNIT_ASSERT( pSample != nullptr );

	CPPUNIT_ASSERT( pSample->get_filename() == QString("snare.wav"));

	___INFOLOG( "passed" );
}

void XmlTest::testDrumkitUpgrade() {
	___INFOLOG( "" );
	// For all drumkits in the legacy folder, check whether there are
	// invalid. Then, we upgrade them to the most recent version and
	// check whether there are valid and if a second upgrade is yields
	// the same result.
	auto pCoreActionController = H2Core::Hydrogen::get_instance()->getCoreActionController();

	QDir legacyDir( H2TEST_FILE( "drumkits/legacyKits" ) );
	QStringList nameFilters;
	nameFilters << "*" + H2Core::Filesystem::drumkit_ext;

	QString sDrumkitPath;

	for ( const auto& ssFile : legacyDir.entryList( nameFilters, QDir::Files ) ) {

		sDrumkitPath = H2TEST_FILE( "drumkits/legacyKits" ) + "/" + ssFile;

		CPPUNIT_ASSERT( ! pCoreActionController->validateDrumkit( sDrumkitPath, false ) );

		// The number of files within the drumkit has to be constant.
		QTemporaryDir contentOriginal( H2Core::Filesystem::tmp_dir() + "-XXXXXX" );
		contentOriginal.setAutoRemove( false );
		CPPUNIT_ASSERT( pCoreActionController->extractDrumkit( sDrumkitPath,
															   contentOriginal.path() ) );
		QDir contentDirOriginal( contentOriginal.path() );
		int nFilesOriginal =
			contentDirOriginal.entryList( QDir::AllEntries |
										  QDir::NoDotAndDotDot ).size();

		// Upgrade the legacy kit and store the result in a temporary
		// folder (they will be automatically removed by Qt as soon as
		// the variable gets out of scope)
		QTemporaryDir firstUpgrade( H2Core::Filesystem::tmp_dir() + "-XXXXXX" );
		firstUpgrade.setAutoRemove( false );
		CPPUNIT_ASSERT( pCoreActionController->upgradeDrumkit( sDrumkitPath,
															   firstUpgrade.path() ) );
		// The upgrade should have yielded a single .h2drumkit file.
		QDir upgradeFolder( firstUpgrade.path() );
		CPPUNIT_ASSERT( upgradeFolder.entryList( QDir::AllEntries |
												 QDir::NoDotAndDotDot ).size() == 1 );
		
		QString sUpgradedKit( firstUpgrade.path() + "/" +
							  upgradeFolder.entryList( QDir::AllEntries |
													   QDir::NoDotAndDotDot )[ 0 ] );
		CPPUNIT_ASSERT( pCoreActionController->validateDrumkit( sUpgradedKit, false ) );
		
		QTemporaryDir contentUpgraded( H2Core::Filesystem::tmp_dir() + "-XXXXXX" );
		contentUpgraded.setAutoRemove( false );
		CPPUNIT_ASSERT( pCoreActionController->extractDrumkit( sUpgradedKit,
															   contentUpgraded.path() ) );
		QDir contentDirUpgraded( contentUpgraded.path() );
		int nFilesUpgraded =
			contentDirUpgraded.entryList( QDir::AllEntries |
										  QDir::NoDotAndDotDot ).size();
		___INFOLOG( nFilesUpgraded );
		if ( nFilesOriginal != nFilesUpgraded ) {
			___ERRORLOG( "Mismatching content of original and upgraded drumkit." );
			___ERRORLOG( QString( "original [%1]:" ).arg( contentOriginal.path() ) );
			for ( const auto& ssFile : contentDirOriginal.entryList( QDir::AllEntries |
																	 QDir::NoDotAndDotDot ) ) {
				___ERRORLOG( "   " + ssFile );
			}
			___ERRORLOG( QString( "upgraded [%1]:" ).arg( contentUpgraded.path() ) );
			for ( const auto& ssFile : contentDirUpgraded.entryList( QDir::AllEntries |
																	 QDir::NoDotAndDotDot ) ) {
				___ERRORLOG( "   " + ssFile );
			}
		}
		CPPUNIT_ASSERT( nFilesOriginal == nFilesUpgraded );

		// Now we upgrade the upgraded drumkit again and bit-compare
		// the results.
		QTemporaryDir secondUpgrade( H2Core::Filesystem::tmp_dir() + "-XXXXXX" );
		secondUpgrade.setAutoRemove( false );
		CPPUNIT_ASSERT( pCoreActionController->upgradeDrumkit( sUpgradedKit,
															   secondUpgrade.path() ) );
		upgradeFolder = QDir( secondUpgrade.path() );
		CPPUNIT_ASSERT( upgradeFolder.entryList( QDir::AllEntries |
												 QDir::NoDotAndDotDot ).size() == 1 );
		
		QString sValidationKit( secondUpgrade.path() + "/" +
								upgradeFolder.entryList( QDir::AllEntries |
														 QDir::NoDotAndDotDot )[ 0 ] );

		QTemporaryDir contentValidation( H2Core::Filesystem::tmp_dir() + "-XXXXXX" );
		contentValidation.setAutoRemove( false );
		CPPUNIT_ASSERT( pCoreActionController->extractDrumkit( sUpgradedKit,
															   contentValidation.path() ) );

		// Compare the extracted folders. Attention: in the toplevel
		// temporary folder there is a single directory named
		// according to the drumkit. These ones have to be compared.
		H2TEST_ASSERT_DIRS_EQUAL( QDir( contentUpgraded.path() )
								  .entryList( QDir::Dirs |
											  QDir::NoDotAndDotDot )[ 0 ],
								  QDir( contentValidation.path() )
								  .entryList( QDir::Dirs |
											  QDir::NoDotAndDotDot )[ 0 ] );

		// Only clean up if all checks passed.
		H2Core::Filesystem::rm( contentOriginal.path(), true, true );
		H2Core::Filesystem::rm( contentUpgraded.path(), true, true );
		H2Core::Filesystem::rm( contentValidation.path(), true, true );
		H2Core::Filesystem::rm( firstUpgrade.path(), true, true );
		H2Core::Filesystem::rm( secondUpgrade.path(), true, true );
	}
	___INFOLOG( "passed" );
}

void XmlTest::testPattern()
{
	___INFOLOG( "" );
	QString sPatternPath = H2Core::Filesystem::tmp_dir()+"pat.h2pattern";

	H2Core::Pattern* pPatternLoaded = nullptr;
	H2Core::Pattern* pPatternReloaded = nullptr;
	H2Core::Pattern* pPatternCopied = nullptr;
	H2Core::Pattern* pPatternNew = nullptr;
	std::shared_ptr<H2Core::Drumkit> pDrumkit = nullptr;
	std::shared_ptr<H2Core::InstrumentList> pInstrumentList = nullptr;
	H2Core::XMLDoc doc;

	pDrumkit = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/baseKit" ) );
	CPPUNIT_ASSERT( pDrumkit!=nullptr );
	pInstrumentList = pDrumkit->get_instruments();
	CPPUNIT_ASSERT( pInstrumentList->size()==4 );

	pPatternLoaded = H2Core::Pattern::load_file( H2TEST_FILE( "/pattern/pat.h2pattern" ), pInstrumentList );
	CPPUNIT_ASSERT( pPatternLoaded );

	CPPUNIT_ASSERT( pPatternLoaded->save_file( "dk_name", "author", H2Core::License(), sPatternPath, true ) );

	// Check for double freeing when destructing both copy and original.
	pPatternCopied = new H2Core::Pattern( pPatternLoaded );

	// Is stored pattern valid?
	CPPUNIT_ASSERT( doc.read( sPatternPath ) );
	pPatternReloaded = H2Core::Pattern::load_file( sPatternPath, pInstrumentList );
	CPPUNIT_ASSERT( pPatternReloaded != nullptr );

	delete pPatternReloaded;

	// Check whether the constructor produces valid patterns.
	pPatternNew = new H2Core::Pattern( "test", "ladida", "", 1, 1 );
	CPPUNIT_ASSERT( pPatternLoaded->save_file( "dk_name", "author", H2Core::License(), sPatternPath, true ) );
	CPPUNIT_ASSERT( doc.read( sPatternPath ) );
	pPatternReloaded = H2Core::Pattern::load_file( sPatternPath, pInstrumentList );
	CPPUNIT_ASSERT( pPatternReloaded != nullptr );

	delete pPatternReloaded;
	delete pPatternLoaded;
	delete pPatternCopied;
	delete pPatternNew;
	___INFOLOG( "passed" );
}

void XmlTest::checkTestPatterns()
{
	___INFOLOG( "" );
	H2Core::XMLDoc doc;
	CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/pattern/pat.h2pattern" ) ) );
	___INFOLOG( "passed" );
}

void XmlTest::testPlaylist()
{
	___INFOLOG( "" );
	QString sPath = H2Core::Filesystem::tmp_dir()+"playlist.h2playlist";

	H2Core::Playlist::create_instance();
	H2Core::Playlist* pPlaylistCurrent = H2Core::Playlist::get_instance();
	H2Core::Playlist* pPlaylistLoaded = nullptr;
	H2Core::XMLDoc doc;

	CPPUNIT_ASSERT( pPlaylistCurrent->save_file( sPath, "ladida", true, false ) );
	CPPUNIT_ASSERT( doc.read( sPath ) );
	pPlaylistLoaded = H2Core::Playlist::load_file( sPath, false );
	CPPUNIT_ASSERT( pPlaylistLoaded != nullptr );

	delete pPlaylistLoaded;
	delete pPlaylistCurrent;
	___INFOLOG( "passed" );
}

void XmlTest::testSamplePathPortability() {
	___INFOLOG( "" );

	auto pSong = H2Core::Song::load(
		H2TEST_FILE( "/song/sample-path-portability.h2song" ) );
	CPPUNIT_ASSERT( pSong != nullptr );
	CPPUNIT_ASSERT( ! pSong->hasMissingSamples() );

	___INFOLOG( "passed" );
}

void XmlTest::testSamplePathsWritten() {
	___INFOLOG( "" );

	auto pHydrogen = H2Core::Hydrogen::get_instance();
	const auto sBaseKit = QString( "%1/GMRockKit" )
		.arg( H2Core::Filesystem::sys_drumkits_dir() );

	// We use a two-stage approach to check all the sample paths: 1. we parse
	// the XML file into a DOM and select only the part corresponding to the
	// drumkit (containing the samples) and 2. convert this part of the DOM back
	// into a string and perform a line-by-line search for the elements
	// containing sample paths. This way we ensure no other filenames, like the
	// ones for the LADSPA effects, will leak into our check and, at the same
	// time, be resilient to changes in the drumkit structure.
	const QString sSamplePathElement( "<filename>" );
	const QString sSamplePathClosingElement( "</filename>" );

	// Create a new drumkit and save it to disk.
	const auto pBaseKit = pHydrogen->getSoundLibraryDatabase()->getDrumkit(
		sBaseKit );
	CPPUNIT_ASSERT( pBaseKit != nullptr );

	auto pNewKit = std::make_shared<H2Core::Drumkit>( pBaseKit );
	CPPUNIT_ASSERT( pNewKit != nullptr );

	pNewKit->set_name( "testSamplePathsWrittenKit" );
	const auto sNewKitPath = QString( "%1/%2" )
		.arg( H2Core::Filesystem::usr_drumkits_dir() ).arg( pNewKit->get_name() );
	CPPUNIT_ASSERT( ! H2Core::Filesystem::dir_exists( sNewKitPath ) );
	CPPUNIT_ASSERT( pNewKit->save( sNewKitPath ) );

	// Load the new drumkit.xml file and validate that there are no absolute
	// paths.
	H2Core::XMLDoc docNewKit;
	CPPUNIT_ASSERT( docNewKit.read( H2Core::Filesystem::drumkit_file(
										sNewKitPath ) ) );

	auto rootNodeNewKit = docNewKit.firstChildElement( "drumkit_info" );
	CPPUNIT_ASSERT( ! rootNodeNewKit.isNull() );

	QString sRootNodeNewKit;
	QTextStream streamNewKit( &sRootNodeNewKit );
	rootNodeNewKit.save( streamNewKit, 0 );
	QString ssLineDrumkit;
	while ( streamNewKit.readLineInto( &ssLineDrumkit ) ) {
		if ( ssLineDrumkit.contains( sSamplePathElement ) ) {
			const auto sSamplePath = ssLineDrumkit.replace( sSamplePathElement, "" )
				.replace( sSamplePathClosingElement, "" ).trimmed();
			___INFOLOG( QString( "[%1] containing sample path [%2]" )
						.arg( sNewKitPath ).arg( sSamplePath ) );
			CPPUNIT_ASSERT( ! sSamplePath.isEmpty() );
			CPPUNIT_ASSERT( sSamplePath.contains( "." ) );
			CPPUNIT_ASSERT( ! sSamplePath.contains( "/" ) );
			CPPUNIT_ASSERT( ! sSamplePath.contains( "\\" ) );
		}
	}

	CPPUNIT_ASSERT( H2Core::Filesystem::rm( sNewKitPath, true ) );

	////////////////////////////////////////////////////////////////////////////

	// Create a new song and save it to disk.
	auto pSong = H2Core::Song::getEmptySong();
	CPPUNIT_ASSERT( pSong != nullptr );

	pSong->setDrumkit( pNewKit, false );
	const auto sSongPath = H2Core::Filesystem::tmp_file_path(
		"testSamplePathsWritten.h2song");
	CPPUNIT_ASSERT( pSong->save( sSongPath ) );

	// Load the new .h2song file and validate that there are no absolute paths.
	H2Core::XMLDoc docSong;
	CPPUNIT_ASSERT( docSong.read( sSongPath ) );

	auto rootNodeSong = docSong.firstChildElement( "song" );
	CPPUNIT_ASSERT( ! rootNodeSong.isNull() );
	auto drumkitNodeSong = rootNodeSong.firstChildElement( "instrumentList" );
	CPPUNIT_ASSERT( ! drumkitNodeSong.isNull() );

	QString sDrumkitNodeSong;
	QTextStream streamSong( &sDrumkitNodeSong );
	drumkitNodeSong.save( streamSong, 0 );
	QString ssLineSong;
	while ( streamSong.readLineInto( &ssLineSong ) ) {
		if ( ssLineSong.contains( sSamplePathElement ) ) {
			const auto sSamplePath = ssLineSong
				.replace( sSamplePathElement, "" )
				.replace( sSamplePathClosingElement, "" )
				.trimmed();
			___INFOLOG( QString( "[%1] containing sample path [%2]" )
						.arg( sSongPath ).arg( sSamplePath ) );
			CPPUNIT_ASSERT( ! sSamplePath.isEmpty() );
			CPPUNIT_ASSERT( sSamplePath.contains( "." ) );
			CPPUNIT_ASSERT( ! sSamplePath.contains( "/" ) );
			CPPUNIT_ASSERT( ! sSamplePath.contains( "\\" ) );
		}
	}

	CPPUNIT_ASSERT( H2Core::Filesystem::rm( sSongPath ) );

	////////////////////////////////////////////////////////////////////////////

	// Create a new song, manually add a sample, and save it to disk.

	// Create a new song and save it to disk.
	auto pSongCustom = H2Core::Song::getEmptySong();
	CPPUNIT_ASSERT( pSongCustom != nullptr );
	pSongCustom->setDrumkit( pNewKit, false );

	const QString sCustomSamplePath( "/path/to/custom/sample.wav" );
	auto pInstrument = pSongCustom->getInstrumentList()->get( 0 );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	auto pComponent = pInstrument->get_component( 0 );
	CPPUNIT_ASSERT( pComponent != nullptr );
	auto pLayer = pComponent->get_layer( 0 );
	CPPUNIT_ASSERT( pLayer != nullptr );
	auto pSample = pLayer->get_sample();
	CPPUNIT_ASSERT( pSample != nullptr );
	pSample->set_filepath( sCustomSamplePath );

	const auto sSongPathCustom = H2Core::Filesystem::tmp_file_path(
		"testCustomSamplePathsWritten.h2song");
	CPPUNIT_ASSERT( pSongCustom->save( sSongPathCustom ) );

	// Load the new .h2song file and validate that there is a single absolute
	// path.
	H2Core::XMLDoc docSongCustom;
	CPPUNIT_ASSERT( docSongCustom.read( sSongPathCustom ) );

	auto rootNodeSongCustom = docSongCustom.firstChildElement( "song" );
	CPPUNIT_ASSERT( ! rootNodeSongCustom.isNull() );
	auto drumkitNodeSongCustom = rootNodeSongCustom.firstChildElement(
		"instrumentList" );
	CPPUNIT_ASSERT( ! drumkitNodeSongCustom.isNull() );

	const int nExpectedAbsolutePaths = 1;
	int nnAbsolutePaths = 0;

	QString sDrumkitNodeSongCustom;
	QTextStream streamSongCustom( &sDrumkitNodeSongCustom );
	drumkitNodeSongCustom.save( streamSongCustom, 0 );
	QString ssLineSongCustom;
	while ( streamSongCustom.readLineInto( &ssLineSongCustom ) ) {
		if ( ssLineSongCustom.contains( sSamplePathElement ) ) {
			const auto sSamplePath = ssLineSongCustom
				.replace( sSamplePathElement, "" )
				.replace( sSamplePathClosingElement, "" )
				.trimmed();
			___INFOLOG( QString( "[%1] containing sample path [%2]" )
						.arg( sSongPathCustom ).arg( sSamplePath ) );
			CPPUNIT_ASSERT( ! sSamplePath.isEmpty() );
			CPPUNIT_ASSERT( sSamplePath.contains( "." ) );
			if ( sSamplePath.contains( "/" ) || sSamplePath.contains( "\\" ) ) {
				++nnAbsolutePaths;
			}
		}
	}

	CPPUNIT_ASSERT( nnAbsolutePaths == nExpectedAbsolutePaths );

	CPPUNIT_ASSERT( H2Core::Filesystem::rm( sSongPathCustom ) );

	___INFOLOG( "passed" );
}

void XmlTest::testSongLegacy() {
	___INFOLOG( "" );

	// Install the legacy test kit into the current user data dir (a temporary
	// one) to check whether file loading of legacy kits works as expected.
	const QString& sKit = "kit-0.9.3";
	const auto sKitDirTest = H2TEST_FILE(
		QString( "drumkits/legacyKits/%1" ).arg( sKit ) );
	const auto sKitDirUser = QString( "%1/%2" )
		.arg( Filesystem::usr_drumkits_dir() ).arg( sKit );
	___INFOLOG( QString( "sKitDirUser: %1" ).arg( sKitDirUser ) );
	CPPUNIT_ASSERT( Filesystem::mkdir( sKitDirUser ) );
	for ( const auto& ssEntry : QDir( sKitDirTest ).entryList(
			  QDir::Files | QDir::Readable | QDir::NoDotAndDotDot ) ) {
		CPPUNIT_ASSERT( Filesystem::file_copy(
							sKitDirTest + "/" + ssEntry,
							sKitDirUser + "/" + ssEntry,
							false /* overwrite */,
							false /* silent */) );
	}

	QStringList testSongs;
	testSongs << H2TEST_FILE( "song/legacy/test_song_1.2.2.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_1.2.1.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_1.2.0.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_1.2.0-beta1.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_1.1.1.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_1.1.0.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_1.1.0-beta1.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_1.0.2.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_1.0.1.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_1.0.0.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_0.9.7.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_0.9.6.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_0.9.3.h2song" );

	for ( const auto& ssSong : testSongs ) {
		___INFOLOG(ssSong);
		auto pSong = H2Core::Song::load( ssSong, false );
		CPPUNIT_ASSERT( pSong != nullptr );
		CPPUNIT_ASSERT( pSong->getInstrumentList() != nullptr );
		CPPUNIT_ASSERT( pSong->getInstrumentList()->size() > 0 );
		for ( const auto& ppInstrument : *pSong->getInstrumentList() ) {
			CPPUNIT_ASSERT( ppInstrument != nullptr );
			CPPUNIT_ASSERT( ppInstrument->hasSamples() );
		}
		CPPUNIT_ASSERT( ! pSong->hasMissingSamples() );
	}

	// Check that invalid paths and drumkit names could indeed result in missing
	// samples.
	testSongs.clear();
	testSongs << H2TEST_FILE( "song/legacy/test_song_invalid_drumkit_name.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_invalid_sample_path.h2song" );

	for ( const auto& ssSong : testSongs ) {
		___INFOLOG(ssSong);
		auto pSong = H2Core::Song::load( ssSong, false );
		CPPUNIT_ASSERT( pSong != nullptr );
		CPPUNIT_ASSERT( pSong->hasMissingSamples() );
	}

	// cleanup
	CPPUNIT_ASSERT( Filesystem::rm( sKitDirUser, true /* recursive */,
									false /* bSilent */ ) );

	___INFOLOG( "passed" );
}

void XmlTest::tearDown() {

	QDirIterator it( TestHelper::get_instance()->getTestDataDir(),
					 QDirIterator::Subdirectories);
	QStringList filters;
	filters << "*.bak*";
	
	while ( it.hasNext() ) {
		it.next();
		const QDir testFolder( it.next() );
		const QStringList backupFiles = testFolder.entryList( filters, QDir::NoFilter, QDir::NoSort );

		for ( auto& bbackupFile : backupFiles ) {
			
			H2Core::Filesystem::rm( testFolder.absolutePath()
									.append( "/" )
									.append( bbackupFile ), false );
		}
	}
}
