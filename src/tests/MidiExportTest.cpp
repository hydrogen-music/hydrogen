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

#include <MidiExportTest.h>

#include <QString>

#include <core/Helpers/Filesystem.h>
#include <core/SMF/SMF.h>

#include "TestHelper.h"
#include "assertions/File.h"

using namespace H2Core;

void MidiExportTest::testExportMIDISMF1Single() {
	___INFOLOG( "" );
	const auto sSongFile = H2TEST_FILE("functional/test.h2song");
	const auto sOutFile = Filesystem::tmp_file_path("smf1single.test.mid");
	const auto sRefFile = H2TEST_FILE("functional/smf1single.test.ref.mid");

	auto pWriter = std::make_shared<SMF1WriterSingle>();
	TestHelper::exportMIDI( sSongFile, sOutFile, pWriter );
	H2TEST_ASSERT_FILES_EQUAL( sRefFile, sOutFile );
	Filesystem::rm( sOutFile );
	___INFOLOG( "passed" );
}
	
void MidiExportTest::testExportMIDISMF1Multi() {
	___INFOLOG( "" );
	const auto sSongFile = H2TEST_FILE("functional/test.h2song");
	const auto sOutFile = Filesystem::tmp_file_path("smf1multi.test.mid");
	const auto sRefFile = H2TEST_FILE("functional/smf1multi.test.ref.mid");

	auto pWriter = std::make_shared<SMF1WriterMulti>();
	TestHelper::exportMIDI( sSongFile, sOutFile, pWriter );
	H2TEST_ASSERT_FILES_EQUAL( sRefFile, sOutFile );
	Filesystem::rm( sOutFile );
	___INFOLOG( "passed" );
}
	
void MidiExportTest::testExportMIDISMF0() {
	___INFOLOG( "" );
	const auto sSongFile = H2TEST_FILE("functional/test.h2song");
	const auto sOutFile = Filesystem::tmp_file_path("smf0.test.mid");
	const auto sRefFile = H2TEST_FILE("functional/smf0.test.ref.mid");

	auto pWriter = std::make_shared<SMF0Writer>();
	TestHelper::exportMIDI( sSongFile, sOutFile, pWriter );
	H2TEST_ASSERT_FILES_EQUAL( sRefFile, sOutFile );
	Filesystem::rm( sOutFile );
	___INFOLOG( "passed" );
}

void MidiExportTest::testExportVelocityAutomationMIDISMF1() {
	___INFOLOG( "" );
	const auto sSongFile = H2TEST_FILE("functional/velocityautomation.h2song");
	const auto sOutFile = Filesystem::tmp_file_path("smf1.velocityautomation.mid");
	const auto sRefFile = H2TEST_FILE("functional/smf1.velocityautomation.ref.mid");

	auto pWriter = std::make_shared<SMF1WriterSingle>();
	TestHelper::exportMIDI( sSongFile, sOutFile, pWriter );
	H2TEST_ASSERT_FILES_EQUAL( sRefFile, sOutFile );
		
	Filesystem::rm( sOutFile );
	___INFOLOG( "passed" );
}
	
void MidiExportTest::testExportVelocityAutomationMIDISMF0() {
	___INFOLOG( "" );
	const auto sSongFile = H2TEST_FILE("functional/velocityautomation.h2song");
	const auto sOutFile = Filesystem::tmp_file_path("smf0.velocityautomation.mid");
	const auto sRefFile = H2TEST_FILE("functional/smf0.velocityautomation.ref.mid");

	auto pWriter = std::make_shared<SMF0Writer>();
	TestHelper::exportMIDI( sSongFile, sOutFile, pWriter );
	H2TEST_ASSERT_FILES_EQUAL( sRefFile, sOutFile );

	Filesystem::rm( sOutFile );
	___INFOLOG( "passed" );
}
