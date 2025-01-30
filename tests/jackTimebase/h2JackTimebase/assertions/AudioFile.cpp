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

#include "AudioFile.h"

#include <sndfile.h>
#include <memory>

static constexpr qint64 BUFFER_SIZE = 4096;

void H2Test::checkAudioFilesEqual(const QString& sExpected, const QString& sActual, CppUnit::SourceLine sourceLine)
{
	SF_INFO info1 = {0};
	std::unique_ptr<SNDFILE, decltype(&sf_close)>
		f1{ sf_open( sExpected.toLocal8Bit().data(), SFM_READ, &info1), sf_close };
	if ( f1 == nullptr ) {
		CppUnit::Message msg(
			QString( "Can't open reference file [%1]" ).arg( sExpected )
			.toLocal8Bit().data(),
			sf_strerror( nullptr )
		);
		throw CppUnit::Exception(msg, sourceLine);
	}

	SF_INFO info2 = {0};
	std::unique_ptr<SNDFILE, decltype(&sf_close)>
		f2{ sf_open( sActual.toLocal8Bit().data(), SFM_READ, &info2), sf_close };
	if ( f2 == nullptr ) {
		CppUnit::Message msg(
			QString( "Can't open results file [%1]" ).arg( sActual )
			.toLocal8Bit().data(),
			sf_strerror( nullptr )
		);
		throw CppUnit::Exception(msg, sourceLine);
	}

	if ( info1.frames != info2.frames ) {
		CppUnit::Message msg(
			"Number of samples different",
			std::string("Expected: ") + sExpected.toStdString(),
			std::string("Actual  : ") + sActual.toStdString() );
		throw CppUnit::Exception(msg, sourceLine);
	}

	auto remainingSamples = info1.frames * info1.channels;
	auto offset = 0LL;
	while ( remainingSamples > 0 ) {
		short buf1[ BUFFER_SIZE ];
		short buf2[ BUFFER_SIZE ];
		auto toRead = qMin( remainingSamples, (sf_count_t)BUFFER_SIZE );

		auto read1 = sf_read_short( f1.get(), buf1, toRead);
		if ( read1 != toRead ) throw CppUnit::Exception( CppUnit::Message( "Short read or read error" ), sourceLine );

		auto read2= sf_read_short( f2.get(), buf2, toRead);
		if ( read2 != toRead ) throw CppUnit::Exception( CppUnit::Message( "Short read or read error" ), sourceLine );

		for ( sf_count_t i = 0; i < toRead; ++i ) {
			// Bit-precise floating point on all platforms is unneeded, and arbitrarily small differences can
			// create rounding differences. Allow results to differ by 1 either way to account for this.
			int delta = (int)buf1[i] - (int)buf2[i];
			if ( delta < -1 || delta > 1 ) {
				auto diffLocation = offset + i + 1;
				CppUnit::Message msg(
					std::string("Files differ at sample ") + std::to_string(diffLocation),
					std::string("Expected: ") + sExpected.toStdString(),
					std::string("Actual  : ") + sActual.toStdString() );
				throw CppUnit::Exception(msg, sourceLine);

			}
		}

		offset += read1;
		remainingSamples -= read1;
	}
}

void H2Test::checkAudioFilesDataEqual(const QString& sExpected, const QString& sActual, CppUnit::SourceLine sourceLine)
{
	SF_INFO expectedInfo = {0};
	std::unique_ptr<SNDFILE, decltype(&sf_close)>
		f1{ sf_open( sExpected.toLocal8Bit().data(), SFM_READ, &expectedInfo), sf_close };
	if ( f1 == nullptr ) {
		CppUnit::Message msg(
			QString( "Can't open reference file [%1]" ).arg( sExpected )
			.toLocal8Bit().data(),
			sf_strerror( nullptr )
		);
		throw CppUnit::Exception(msg, sourceLine);
	}

	SF_INFO actualInfo = {0};
	std::unique_ptr<SNDFILE, decltype(&sf_close)>
		f2{ sf_open( sActual.toLocal8Bit().data(), SFM_READ, &actualInfo), sf_close };
	if ( f2 == nullptr ) {
		CppUnit::Message msg(
			QString( "Can't open results file [%1]" ).arg( sActual )
			.toLocal8Bit().data(),
			sf_strerror( nullptr )
		);
		throw CppUnit::Exception(msg, sourceLine);
	}

	if ( expectedInfo.frames > actualInfo.frames ) {
		CppUnit::Message msg(
			QString( "Number of samples retrieved from expected file larger than in actual one.\nExpected [%1]: %2\nActual [%3]: %4" )
			.arg( sExpected ).arg( expectedInfo.frames )
			.arg( sActual ).arg( actualInfo.frames )
			.toLocal8Bit().data() );
		throw CppUnit::Exception(msg, sourceLine);
	}

	auto totalSamples = actualInfo.frames * actualInfo.channels;
	auto expectedSamples = expectedInfo.frames * expectedInfo.channels;
	auto samplesRead = 0;
	auto offset = 0LL;
	sf_count_t toRead;
	while ( samplesRead < totalSamples ) {
		short buf1[ BUFFER_SIZE ];
		short buf2[ BUFFER_SIZE ];

		toRead = qMin( totalSamples - samplesRead, (sf_count_t)BUFFER_SIZE );
		if ( samplesRead < expectedSamples &&
			 samplesRead + toRead >= expectedSamples ) {
			// Read the remainder of the expected sample before
			// skipping it.
			toRead = qMin( expectedSamples - samplesRead, (sf_count_t)BUFFER_SIZE );
		}

		if ( samplesRead < expectedSamples ) {
			auto read1 = sf_read_short( f1.get(), buf1, toRead);
			if ( read1 != toRead ) {
				throw CppUnit::Exception( CppUnit::Message( "Short read or read error 1" ),
										  sourceLine );
			}
		}

		auto read2= sf_read_short( f2.get(), buf2, toRead);
		if ( read2 != toRead ) {
			throw CppUnit::Exception( CppUnit::Message( "Short read or read error 2" ),
									  sourceLine );
		}

		for ( sf_count_t i = 0; i < toRead; ++i ) {
			// Bit-precise floating point on all platforms is
			// unneeded, and arbitrarily small differences can create
			// rounding differences. Allow results to differ by 1
			// either way to account for this.
			if ( samplesRead < expectedSamples ) {
				// There were enough samples in the expected audio
				// file left to read them
				int delta = (int)buf1[i] - (int)buf2[i];
				if ( delta < -1 || delta > 1 ) {
					auto diffLocation = offset + i + 1;
					CppUnit::Message msg(
						std::string("Files differ at sample ") + std::to_string(diffLocation),
						std::string("Expected: ") + sExpected.toStdString(),
						std::string("Actual  : ") + sActual.toStdString() );
					throw CppUnit::Exception(msg, sourceLine);

				}
			}
			else {
				// The samples in the expected file are already
				// exhausted. In order for still treat it equivalent
				// to the actual one, the require the latter to only
				// contain zeros from now on.
				if ( buf2[0] != 0 ) {
					auto diffLocation = offset + i + 1;
					CppUnit::Message msg(
						std::string("Got [") + std::to_string(diffLocation) +
						std::string( "] instead of 0 in" ),
						std::string("Actual  : ") + sActual.toStdString() );
					throw CppUnit::Exception(msg, sourceLine);
				}
			}
					
		}

		offset += toRead;
		samplesRead += toRead;
	}
}
