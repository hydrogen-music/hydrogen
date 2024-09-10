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

	const std::vector<Interpolation::InterpolateMode> interpolations{
		Interpolation::InterpolateMode::Linear,
		Interpolation::InterpolateMode::Cosine,
		Interpolation::InterpolateMode::Third,
		Interpolation::InterpolateMode::Cubic,
		Interpolation::InterpolateMode::Hermite};

	const std::vector<int> sampleRates{ 22050, 44100, 48000, 88200, 96000, 192000 };
	const std::vector<int> sampleDepths{ 8, 16, 24, 32 };

	const QStringList fileExtensions = Filesystem::supportedSampleFormats();

	for ( const auto& ssExtension : fileExtensions ) {
		for ( const auto& iinterpolationMode : interpolations ) {
			pSampler->setInterpolateMode( iinterpolationMode );
			if ( ssExtension == "ogg" ) {
				// Just a single sample rate and depth supported for Opus encoded
				// files.
				// TODO why?
				const QString sFilename = QString( "%1/song-%2.%3" )
					.arg( exportDir.path() )
					.arg( Interpolation::ModeToQString( iinterpolationMode ) )
					.arg( ssExtension );
				TestHelper::exportSong( sSong, sFilename );
			}
			else {
				// All other file types
				for ( const auto& nnSampleRate : sampleRates ) {
					for ( const auto& nnSampleDepth : sampleDepths ) {
						const QString sFilename = QString( "%1/song-%2-%3-%4.%5" )
							.arg( exportDir.path() )
							.arg( Interpolation::ModeToQString( iinterpolationMode ) )
							.arg( nnSampleRate )
							.arg( nnSampleDepth )
							.arg( ssExtension );
						TestHelper::exportSong(
							sSong, sFilename, nnSampleRate, nnSampleDepth );

					}
				}
			}
		}
	}

	// Cleanup
	H2Core::Filesystem::rm( exportDir.path(), true, true );

	___INFOLOG( "passed" );
}
