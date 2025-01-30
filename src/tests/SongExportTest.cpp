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

#include "SongExportTest.h"

#include "TestHelper.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/Sampler/Interpolation.h>
#include <core/Sampler/Sampler.h>

#include <memory>
#include <vector>
#include <QTemporaryDir>
#include <QString>

using namespace H2Core;

void SongExportTest::testSongExport() {
	___INFOLOG( "" );
	auto pHydrogen = Hydrogen::get_instance();
	auto pSampler = pHydrogen->getAudioEngine()->getSampler();

	const QString sSong = H2TEST_FILE( "song/AE_sampleConsistency.h2song" );

	// Will contain all exported audio files and is only cleaned up on success.
	QTemporaryDir exportDir( H2Core::Filesystem::tmp_dir() + "songExport-XXXXXX" );
	exportDir.setAutoRemove( false );

	// Full test
	//
	// For reasons not clear to me yet, the full test fails on Windows with a
	// "bad alloc" exception. Checking the memory required by the test process
	// one can see that virtual memory is constantly increasing and physical one
	// looks like an irregular saw pattern. The process is quite hungry for
	// memory and at times the OS frees it. Though, at some point memory
	// consumption get's too big and something in the stack get's killed.
	//
	// But the behavior above does not seem to be related to the audio export
	// code. At least, virtual memory consumption also increases linear when
	// running the whole XmlTest suite instead of the audio export below and
	// exporting a single set of audio parameters over and over again yields the
	// same results.
	//
	// const std::vector<Interpolation::InterpolateMode> interpolations{
	// 	Interpolation::InterpolateMode::Linear,
	// 	Interpolation::InterpolateMode::Cosine,
	// 	Interpolation::InterpolateMode::Third,
	// 	Interpolation::InterpolateMode::Cubic,
	// 	Interpolation::InterpolateMode::Hermite};
	// const std::vector<int> sampleRates{ 22050, 44100, 48000, 88200, 96000, 192000 };
	// const std::vector<int> sampleDepths{ 8, 16, 24, 32 };
	// const std::vector<double> compressionLevels{ 0.0, 0.1, 0.25, 0.5, 0.75, 0.8263534124364, 1.0 };

	// Superficial one. We do not want our pipeline tests to take too long.
	const std::vector<Interpolation::InterpolateMode> interpolations{
		Interpolation::InterpolateMode::Linear};
	const std::vector<int> sampleRates{ 48000 };
	const std::vector<int> sampleDepths{ 16, 32 };
	const std::vector<double> compressionLevels{ 0.0 };

	for ( const auto& fformat : Filesystem::supportedAudioFormats() ) {
		for ( const auto& iinterpolationMode : interpolations ) {
			for ( const auto& nnSampleRate : sampleRates ) {
				for ( const auto& nnSampleDepth : sampleDepths ) {
					for ( const auto& ffCompressionLevel : compressionLevels ) {
						pSampler->setInterpolateMode( iinterpolationMode );
						const QString sFilename = QString( "%1/song-%2-%3-%4-%5.%6" )
							.arg( exportDir.path() )
							.arg( Interpolation::ModeToQString( iinterpolationMode ) )
							.arg( nnSampleRate )
							.arg( nnSampleDepth )
							.arg( ffCompressionLevel )
							.arg( Filesystem::AudioFormatToSuffix( fformat ) );
						if ( fformat != Filesystem::AudioFormat::Ogg &&
							 fformat != Filesystem::AudioFormat::Flac &&
							 fformat != Filesystem::AudioFormat::Opus &&
							 fformat != Filesystem::AudioFormat::Mp3 &&
							 ffCompressionLevel != compressionLevels[ 0 ] ) {
							// Only for these ones compression/quality
							// trade-offs are supported.
							continue;
						}

						// See
						// https://libsndfile.github.io/libsndfile/formats.html
						// for which parameters are suppored for the particular
						// formats.
						//
						// Instead of making audio export fail on non-supported
						// parameter combinations, we tailor this test and UI to
						// only allow valid ones. It would be bad UX to provide
						// an invalid option.
						if ( ( fformat == Filesystem::AudioFormat::Ogg ||
							   fformat == Filesystem::AudioFormat::Opus ) &&
							 ( nnSampleRate != 48000 || nnSampleDepth != 32 ) ) {
							continue;
						}
						if ( fformat == Filesystem::AudioFormat::Flac &&
							 nnSampleDepth == 32 ) {
							continue;
						}
						if ( fformat == Filesystem::AudioFormat::Voc &&
							 nnSampleDepth > 16 ) {
							continue;
						}
						if ( fformat == Filesystem::AudioFormat::Mp3 &&
							 ( nnSampleDepth != 16 || nnSampleRate > 48000 ) ) {
							continue;
						}

						TestHelper::exportSong(
							sSong, sFilename, nnSampleRate, nnSampleDepth,
							ffCompressionLevel );
					}
				}
			}
		}
	}

	// Cleanup
	H2Core::Filesystem::rm( exportDir.path(), true, true );

	___INFOLOG( "passed" );
}
