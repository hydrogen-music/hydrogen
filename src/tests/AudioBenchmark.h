#ifndef AUDIO_BENCHMARK_H
#define AUDIO_BENCHMARK_H


#include <cppunit/extensions/HelperMacros.h>

class AudioBenchmark : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE(AudioBenchmark);
	CPPUNIT_TEST(audioBenchmark);
	CPPUNIT_TEST_SUITE_END();
	static bool bEnabled;
 public:
	void audioBenchmark(void);
	static void enable() { bEnabled = true; }
};



#endif
