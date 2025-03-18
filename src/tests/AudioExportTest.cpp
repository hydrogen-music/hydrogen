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

#include "AudioExportTest.h"

#include <QString>

#include <core/Helpers/Filesystem.h>

#include "TestHelper.h"
#include "assertions/AudioFile.h"

#include <vector>

using namespace H2Core;

void AudioExportTest::testExportAudio() {
	___INFOLOG( "" );
	const auto sSongFile = H2TEST_FILE("functional/test_adsr.h2song");

	// Test exporting using a couple of different sample rate (to test both
	// resampling and no resampling) and sample depths.
	struct Setup {
		QString sTempFile;
		QString sReferenceFile;
		int nSampleRate;
		int nSampleDepth;
	};

	std::vector<Setup> setups;
	setups.push_back( { "test-44100-16.wav",
			"functional/test-44100-16.ref.flac", 44100, 16} );
	setups.push_back( { "test-48000-16.wav",
			"functional/test-48000-16.ref.flac", 48000, 16} );
	setups.push_back( { "test-48000-32.wav",
			"functional/test-48000-32.ref.flac", 48000, 32} );

	for ( int ii = 0; ii < setups.size(); ++ii ) {
		___INFOLOG( QString( "Testing sample rate: [%1] and depth: [%2]" )
					.arg( setups[ii].nSampleRate )
					.arg( setups[ii].nSampleDepth ) );
		const auto sOutFile = Filesystem::tmp_file_path( setups[ii].sTempFile );
		const auto sRefFile = H2TEST_FILE( setups[ii].sReferenceFile );

		TestHelper::exportSong( sSongFile, sOutFile, setups[ii].nSampleRate,
								setups[ii].nSampleDepth );
		H2TEST_ASSERT_AUDIO_FILES_EQUAL( sRefFile, sOutFile );
		Filesystem::rm( sOutFile );
	}
	___INFOLOG( "passed" );
}

void AudioExportTest::testExportVelocityAutomationAudio() {
	___INFOLOG( "" );
	const auto sSongFile = H2TEST_FILE("functional/velocityautomation.h2song");
	const auto sOutFile = Filesystem::tmp_file_path("velocityautomation.wav");
	const auto sRefFile = H2TEST_FILE("functional/velocityautomation.ref.flac");

	TestHelper::exportSong( sSongFile, sOutFile );
	H2TEST_ASSERT_AUDIO_FILES_EQUAL( sRefFile, sOutFile );
	Filesystem::rm( sOutFile );
	___INFOLOG( "passed" );
}
