/*
 * Hydrogen
 * Copyright(c) 2002-2018 by the Hydrogen Team
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "audiofile.h"

#include <sndfile.h>
#include <memory>

static constexpr qint64 BUFFER_SIZE = 4096;

void H2Test::checkAudioFilesEqual(const QString &expected, const QString &actual, CppUnit::SourceLine sourceLine)
{
	SF_INFO info1 = {0};
	std::unique_ptr<SNDFILE, decltype(&sf_close)>
		f1{ sf_open( expected.toLocal8Bit().data(), SFM_READ, &info1), sf_close };
	if ( f1 == nullptr ) {
		CppUnit::Message msg(
			"Can't open reference file",
			sf_strerror( NULL )
		);
		throw CppUnit::Exception(msg, sourceLine);
	}

	SF_INFO info2 = {0};
	std::unique_ptr<SNDFILE, decltype(&sf_close)>
		f2{ sf_open( actual.toLocal8Bit().data(), SFM_READ, &info2), sf_close };
	if ( f2 == nullptr ) {
		CppUnit::Message msg(
			"Can't open results file",
			sf_strerror( NULL )
		);
		throw CppUnit::Exception(msg, sourceLine);
	}

	if ( info1.frames != info2.frames ) {
		CppUnit::Message msg(
			"Number of samples different",
			std::string("Expected: ") + expected.toStdString(),
			std::string("Actual  : ") + actual.toStdString() );
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
			if ( buf1[i] != buf2[i] ) {
				auto diffLocation = offset + i + 1;
				CppUnit::Message msg(
					std::string("Files differ at sample ") + std::to_string(diffLocation),
					std::string("Expected: ") + expected.toStdString(),
					std::string("Actual  : ") + actual.toStdString() );
				throw CppUnit::Exception(msg, sourceLine);

			}
		}

		offset += read1;
		remainingSamples -= read1;
	}
}
