/*
 * Hydrogen
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

#ifndef AUDIO_BENCHMARK_H
#define AUDIO_BENCHMARK_H


#include <cppunit/extensions/HelperMacros.h>
#include <QTextStream>

class AudioBenchmark : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE(AudioBenchmark);
	CPPUNIT_TEST(audioBenchmark);
	CPPUNIT_TEST_SUITE_END();

	static bool bEnabled;
	QTextStream out;

	void timeADSR();
	double timeExport( int nSampleRate,
					   H2Core::Interpolation::InterpolateMode interpolateMode,
					   double fReference = 0.0,
					   double *pfRMS = nullptr );

 public:
	void audioBenchmark(void);
	static void enable() { bEnabled = true; }
	AudioBenchmark() : out( stdout ) {}
};



#endif
