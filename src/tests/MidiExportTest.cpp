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

#include <core/Basics/Pattern.h>
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

void MidiExportTest::testTimeSignatureCalculation() {
	___INFOLOG( "" );

	int nNumerator, nDenominator;
	bool bRounded, bScaled;

	// The default pattern should be a 4/4.
	auto pPattern = std::make_shared<Pattern>();
	SMF::PatternToTimeSignature(
		pPattern, &nNumerator, &nDenominator, &bRounded, &bScaled );
	CPPUNIT_ASSERT( nNumerator == 4 );
	CPPUNIT_ASSERT( nDenominator == 4 );
	CPPUNIT_ASSERT( ! bRounded );
	CPPUNIT_ASSERT( ! bScaled );

	// A couple of valid signatures that should pass right through.
	// 1 / 1
	pPattern->setDenominator( 1 );
	pPattern->setLength( 1. / 1. * 4. * static_cast<double>(H2Core::nTicksPerQuarter) );
	SMF::PatternToTimeSignature(
		pPattern, &nNumerator, &nDenominator, &bRounded, &bScaled );
	CPPUNIT_ASSERT( nNumerator == 1 );
	CPPUNIT_ASSERT( nDenominator == 1 );
	CPPUNIT_ASSERT( ! bRounded );
	CPPUNIT_ASSERT( ! bScaled );

	// 2 / 4
	pPattern->setDenominator( 4 );
	pPattern->setLength( 2. / 4. * 4. * static_cast<double>(H2Core::nTicksPerQuarter) );
	SMF::PatternToTimeSignature(
		pPattern, &nNumerator, &nDenominator, &bRounded, &bScaled );
	CPPUNIT_ASSERT( nNumerator == 2 );
	CPPUNIT_ASSERT( nDenominator == 4 );
	CPPUNIT_ASSERT( ! bRounded );
	CPPUNIT_ASSERT( ! bScaled );

	// 6 / 2
	pPattern->setDenominator( 2 );
	pPattern->setLength( 6. / 2. * 4. * static_cast<double>(H2Core::nTicksPerQuarter) );
	SMF::PatternToTimeSignature(
		pPattern, &nNumerator, &nDenominator, &bRounded, &bScaled );
	CPPUNIT_ASSERT( nNumerator == 6 );
	CPPUNIT_ASSERT( nDenominator == 2 );
	CPPUNIT_ASSERT( ! bRounded );
	CPPUNIT_ASSERT( ! bScaled );

	// 8 / 16
	pPattern->setDenominator( 16 );
	pPattern->setLength( 8. / 16. * 4. * static_cast<double>(H2Core::nTicksPerQuarter) );
	SMF::PatternToTimeSignature(
		pPattern, &nNumerator, &nDenominator, &bRounded, &bScaled );
	CPPUNIT_ASSERT( nNumerator == 8 );
	CPPUNIT_ASSERT( nDenominator == 16 );
	CPPUNIT_ASSERT( ! bRounded );
	CPPUNIT_ASSERT( ! bScaled );

	// 7 / 4
	pPattern->setDenominator( 4 );
	pPattern->setLength( 7. / 4. * 4. * static_cast<double>(H2Core::nTicksPerQuarter) );
	SMF::PatternToTimeSignature(
		pPattern, &nNumerator, &nDenominator, &bRounded, &bScaled );
	CPPUNIT_ASSERT( nNumerator == 7 );
	CPPUNIT_ASSERT( nDenominator == 4 );
	CPPUNIT_ASSERT( ! bRounded );
	CPPUNIT_ASSERT( ! bScaled );

	// Some singatures that need rounding or rescaling

	// 2.1 / 4
	pPattern->setDenominator( 4 );
	pPattern->setLength( 2.1 / 4. * 4. * static_cast<double>(H2Core::nTicksPerQuarter) );
	SMF::PatternToTimeSignature(
		pPattern, &nNumerator, &nDenominator, &bRounded, &bScaled );
	CPPUNIT_ASSERT( nNumerator == 2 );
	CPPUNIT_ASSERT( nDenominator == 4 );
	CPPUNIT_ASSERT( bRounded );
	CPPUNIT_ASSERT( ! bScaled );

	// 3.5 / 4
	pPattern->setDenominator( 4 );
	pPattern->setLength( 3.5 / 4. * 4. * static_cast<double>(H2Core::nTicksPerQuarter) );
	SMF::PatternToTimeSignature(
		pPattern, &nNumerator, &nDenominator, &bRounded, &bScaled );
	CPPUNIT_ASSERT( nNumerator == 7 );
	CPPUNIT_ASSERT( nDenominator == 8 );
	CPPUNIT_ASSERT( ! bRounded );
	CPPUNIT_ASSERT( ! bScaled );

	// 4 / 3
	pPattern->setDenominator( 3 );
	pPattern->setLength( 4. / 3. * 4. * static_cast<double>(H2Core::nTicksPerQuarter) );
	SMF::PatternToTimeSignature(
		pPattern, &nNumerator, &nDenominator, &bRounded, &bScaled );
	CPPUNIT_ASSERT( nNumerator == 5 );
	CPPUNIT_ASSERT( nDenominator == 4 );
	CPPUNIT_ASSERT( bRounded );
	CPPUNIT_ASSERT( bScaled );

	// 5 / 6
	pPattern->setDenominator( 6 );
	pPattern->setLength( 5. / 6. * 4. * static_cast<double>(H2Core::nTicksPerQuarter) );
	SMF::PatternToTimeSignature(
		pPattern, &nNumerator, &nDenominator, &bRounded, &bScaled );
	CPPUNIT_ASSERT( nNumerator == 7 );
	CPPUNIT_ASSERT( nDenominator == 8 );
	CPPUNIT_ASSERT( bRounded );
	CPPUNIT_ASSERT( bScaled );

	// 129 / 128
	pPattern->setDenominator( 128 );
	pPattern->setLength( 129. / 128. * 4. * static_cast<double>(H2Core::nTicksPerQuarter) );
	SMF::PatternToTimeSignature(
		pPattern, &nNumerator, &nDenominator, &bRounded, &bScaled );
	CPPUNIT_ASSERT( nNumerator == 129 );
	CPPUNIT_ASSERT( nDenominator == 128 );
	CPPUNIT_ASSERT( bRounded );
	CPPUNIT_ASSERT( ! bScaled );

	___INFOLOG( "passed" );
}
