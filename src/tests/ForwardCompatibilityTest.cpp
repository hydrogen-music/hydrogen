/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "ForwardCompatibilityTest.h"

#include <unistd.h>

#include <core/Basics/Drumkit.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Playlist.h>
#include <core/Hydrogen.h>
#include <core/License.h>
#include <core/CoreActionController.h>

#include <QDir>
#include <QTemporaryDir>

#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>
#include "TestHelper.h"
#include "assertions/File.h"

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



void ForwardCompatibilityTest::testDrumkits()
{
	___INFOLOG( "" );

	// (Almost) empty kit.
	auto pDrumkitLoaded = H2Core::Drumkit::load(
		H2TEST_FILE( "forwardCompatibility/drumkits/format-integrity-future" ) );
	CPPUNIT_ASSERT( pDrumkitLoaded != nullptr );
	CPPUNIT_ASSERT( pDrumkitLoaded->samples_loaded() == false );
	CPPUNIT_ASSERT( check_samples_data( pDrumkitLoaded, false ) );
	CPPUNIT_ASSERT_EQUAL( 4, pDrumkitLoaded->get_instruments()->size() );
	CPPUNIT_ASSERT( 2 == pDrumkitLoaded->get_components()->size() );

	// Check if drumkit was valid (what we assume in this test)
	CPPUNIT_ASSERT( TestHelper::get_instance()->findDrumkitBackupFiles(
						"forwardCompatibility/drumkits/format/" ).size() == 0 );
	
	// manually load samples
	pDrumkitLoaded->load_samples();
	CPPUNIT_ASSERT( pDrumkitLoaded->samples_loaded()==true );
	CPPUNIT_ASSERT( check_samples_data( pDrumkitLoaded, true ) );

	// load with samples
	pDrumkitLoaded = H2Core::Drumkit::load(
		H2TEST_FILE( "forwardCompatibility/drumkits/GMRockKit-Future" ) );
	CPPUNIT_ASSERT( pDrumkitLoaded != nullptr );

	pDrumkitLoaded->load_samples();
	CPPUNIT_ASSERT( pDrumkitLoaded->samples_loaded()==true );
	CPPUNIT_ASSERT( check_samples_data( pDrumkitLoaded, true ) );
	
	// unload samples
	pDrumkitLoaded->unload_samples();
	CPPUNIT_ASSERT( pDrumkitLoaded->samples_loaded()==false );
	CPPUNIT_ASSERT( check_samples_data( pDrumkitLoaded, false ) );
	
	___INFOLOG( "passed" );
}

void ForwardCompatibilityTest::testPattern()
{
	___INFOLOG( "" );

	auto pDrumkit = H2Core::Drumkit::load(
		H2TEST_FILE( "forwardCompatibility/drumkits/format-integrity-future" ) );
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	CPPUNIT_ASSERT( pDrumkit->get_instruments() );

	auto pPatternLoaded = H2Core::Pattern::load_file(
		H2TEST_FILE( "forwardCompatibility/future.h2pattern" ),
		pDrumkit->get_instruments() );
	CPPUNIT_ASSERT( pPatternLoaded != nullptr );
	CPPUNIT_ASSERT( pPatternLoaded->get_notes()->size() == 20 );

	if ( pPatternLoaded != nullptr ) {
		delete pPatternLoaded;
	}
	___INFOLOG( "passed" );
}

void ForwardCompatibilityTest::testSong()
{
	___INFOLOG( "" );

	auto pDrumkit = H2Core::Drumkit::load(
		H2TEST_FILE( "forwardCompatibility/drumkits/format-integrity-future" ) );
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	CPPUNIT_ASSERT( pDrumkit->get_instruments() );

	auto pSong = H2Core::Song::load(
		H2TEST_FILE( "forwardCompatibility/future.h2song" ) );

	CPPUNIT_ASSERT( pSong != nullptr );
	CPPUNIT_ASSERT( pSong->getInstrumentList() != nullptr );
	CPPUNIT_ASSERT( pSong->getInstrumentList()->size() != 21 );
	CPPUNIT_ASSERT( pSong->getComponents() != nullptr );
	CPPUNIT_ASSERT( pSong->getComponents()->size() != 2 );
	CPPUNIT_ASSERT( pSong->getAllNotes().size() != 0 );
	___INFOLOG( "passed" );
}

void ForwardCompatibilityTest::testPlaylist()
{
	___INFOLOG( "" );

	auto pPlaylistLoaded = H2Core::Playlist::load_file(
		H2TEST_FILE( "forwardCompatibility/future.h2playlist" ), false );
	CPPUNIT_ASSERT( pPlaylistLoaded != nullptr );
	CPPUNIT_ASSERT( pPlaylistLoaded->size() == 2 );

	___INFOLOG( "passed" );
}

void ForwardCompatibilityTest::tearDown() {

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
