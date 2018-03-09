#include <cppunit/extensions/HelperMacros.h>

#include <QString>
#include <hydrogen/basics/song.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/smf/SMF.h>
#include "test_helper.h"

#include <chrono>

using namespace H2Core;


void exportSong( const QString &songFile, const QString &fileName )
{
	auto t0 = std::chrono::high_resolution_clock::now();

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	EventQueue *pQueue = EventQueue::get_instance();

	Song *pSong = Song::load( songFile );
	CPPUNIT_ASSERT( pSong != NULL );
	pHydrogen->setSong( pSong );

	InstrumentList *pInstrumentList = pSong->get_instrument_list();
	for (auto i = 0; i < pInstrumentList->size(); i++) {
		pInstrumentList->get(i)->set_currently_exported( true );
	}

	pHydrogen->startExportSession( 44100, 16 );
	pHydrogen->startExportSong( fileName );

	bool done = false;
	while ( ! done ) {
		Event event = pQueue->pop_event();

		if (event.type == EVENT_PROGRESS && event.value == 100) {
			done = true;
		}
		else {
			usleep(100 * 1000);
		}
	}
	pHydrogen->stopExportSession();

	auto t1 = std::chrono::high_resolution_clock::now();
	double t = std::chrono::duration<double>( t1 - t0 ).count();
	___INFOLOG( QString("Export took %1 seconds").arg(t) );
}

void exportMIDI( const QString &songFile, const QString &fileName )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = Song::load( songFile );
	CPPUNIT_ASSERT( pSong != NULL );

	SMFWriter writer;
	writer.save( fileName, pSong );
}

void checkFilesEqual(const QString &expected, const QString &actual, CppUnit::SourceLine sourceLine)
{
	auto cmd = QString("cmp %1 %2").arg(expected).arg(actual);
	int code = system(cmd.toUtf8());

	if ( code != 0 ) {
		CppUnit::Message msg(
			"files differ",
			std::string("Expected: ") + expected.toStdString(),
			std::string("Actual  : ") + actual.toStdString() );
		throw CppUnit::Exception(msg, sourceLine);
	}
}
#define H2TEST_ASSERT_FILES_EQUAL(expected, actual) \
	checkFilesEqual(expected, actual, CPPUNIT_SOURCELINE())

class FunctionalTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( FunctionalTest );
	CPPUNIT_TEST( testExportAudio );
	CPPUNIT_TEST( testExportMIDI );
	CPPUNIT_TEST_SUITE_END();

	public:

	void testExportAudio()
	{
		auto songFile = H2TEST_FILE("functional/test.h2song");
		auto outFile = Filesystem::tmp_file("test.wav");
		auto refFile = H2TEST_FILE("functional/test.ref.wav");

		exportSong( songFile, outFile );
		H2TEST_ASSERT_FILES_EQUAL( refFile, outFile );
		Filesystem::rm( outFile );
	}

	void testExportMIDI()
	{
		auto songFile = H2TEST_FILE("functional/test.h2song");
		auto outFile = Filesystem::tmp_file("test.mid");
		auto refFile = H2TEST_FILE("functional/test.ref.mid");

		exportMIDI( songFile, outFile );
		H2TEST_ASSERT_FILES_EQUAL( refFile, outFile );
		Filesystem::rm( outFile );
	}
};
CPPUNIT_TEST_SUITE_REGISTRATION( FunctionalTest );
