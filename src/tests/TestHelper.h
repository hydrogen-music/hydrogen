/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <QString>
#include <cassert>
#include <memory>
#include <vector>

#include <core/Basics/Drumkit.h>
#include <core/Basics/Song.h>
#include <core/Midi/SMF.h>

namespace H2Core {
	class Hydrogen;
	class Preferences;
	class EventQueue;
}

class TestHelper {
	public:
		TestHelper();

		/** Set/get the suite's Hydrogen instance (set once by the harness). */
		void setHydrogen( H2Core::Hydrogen* pHydrogen ) { m_pHydrogen = pHydrogen; }
		H2Core::Hydrogen* getHydrogen() const { return m_pHydrogen; }

	bool isAppveyor() const;
		QString getDataDir() const;
		QString getTestDataDir() const;
		QString getTestFile(const QString& file) const;
	QStringList findDrumkitBackupFiles( const QString& sPath ) const;

		static QString sRootDir;

	/**
	 * Picks different combinations of sample rate and buffer size of
	 * the SoftwareDriver, stores them in the Preferences instance, and
	 * restarts the SoftwareDriver.
	 *
	 * \param nIndex Numbers 0 till 10 correspond to hard-coded
	 * parameter combinations. For all others random values will be
	 * used.
	 * 
	 * \return true on success
	 */
	static void varyAudioDriverConfig( int nIndex );

	/**
	 * Export Hydrogon song @a sSongFile to audio file @a sFileName;
	 *
	 * \param sSongFile Path to Hydrogen file
	 * \param sFileName Output file name
	 * @param nSampleRate sample rate using which to export
	 * @param nSampleDepth sample depth using which to export
	 * @param fCompressionLevel Trades off audio quality against compression
	 *   rate defined between 0.0 (maximum quality) and 1.0 (maximum
	 *   compression).
	 */
	static void exportSong( const QString& sSongFile,
							const QString& sFileName,
							int nSampleRate = 44100,
							int nSampleDepth = 16,
							double fCompressionLevel = 0.0 );
	/**
	 * Export the current song within Hydrogen to audio file @a sFileName;
	 *
	 * \param sFileName Output file name
	 * \param excludedInstruments Identities of instruments to leave out of the
	 *   export (empty = export all). Forwarded to
	 *   #H2Core::Hydrogen::startExportSong().
	 */
	static void exportSong(
		const QString& sFileName,
		const std::vector<H2Core::Uuid>& excludedInstruments = {} );

	/**
	 * Export Hydrogon song @a sSongFile to MIDI file @a sFileName
	 * using writer @a writer.
	 * \param sSongFile Path to Hydrogen file
	 * \param sFileName Output file name
	 * \param writer Writer.
	 **/
	static void exportMIDI( const QString& sSongFile, const QString& sFileName,
							std::shared_ptr<H2Core::SMFWriter> pWriter,
							bool bUseHumanization );

		/** Blocks till the audio processing callback was called again (the
		 * current realtime frames got incremented).
		 *
		 * The realtime frames are update in each loop of the process cycle. By
		 * checking for a new value, we ensure a whole process cycle - including
		 * the adoption of a new tempo or state - has passed. */
		static void waitForAudioDriver();
		/** Wait till the LoopBackMidiDriver did send, receive, and handle the
		 * message. */
		static void waitForMidiDriver();
		/** Since incoming MIDI events are handled asynchronously, we pause
		 * execution till all are handled. */
		static void waitForMidiActionManagerWorkerThread();


	static void			createInstance( bool bAppveyor );
	static TestHelper*	get_instance();

	static H2Core::Hydrogen* makeEngine();
	/** A standalone headless engine standing in for the editor-side mirror that
	 * the GUI would read from. Caller owns it. */
	static H2Core::Hydrogen* makeMirror();
	static QString uniqueEndpoint();

	/** Pump the (main-thread) editor channel via the event loop and yield to
	 * the engine's bridge thread, until @a cond holds or @a nTimeoutMs
	 * elapses. */
	static bool pumpUntil( std::function<bool()> cond, int nTimeoutMs = 4000 );

	/** Pump as above, draining the mirror's EventQueue and reporting whether
	 * the given (type,value) event arrived (forwarded from the engine). */
	static bool pumpUntilEvent(
		H2Core::Hydrogen* pMirror,
		H2Core::Event::Type type,
		int nValue,
		int nTimeoutMs = 4000
	);

private:
	static TestHelper*	m_pInstance;
	QString m_sDataDir;
	QString m_sTestDataDir;
	bool m_bAppveyor;
	/** The process-current Hydrogen instance the suite runs against (ADR 0015,
	 * T1.5 test-instance fixture). Held here so tests reach it without the
	 * Hydrogen::get_instance() shim. */
	H2Core::Hydrogen* m_pHydrogen = nullptr;

	static int nEndpointCounter;
};

inline TestHelper*	TestHelper::get_instance() 
{ 
	assert(m_pInstance); return m_pInstance; 
}

inline bool TestHelper::isAppveyor() const {
	return m_bAppveyor;
}

inline QString TestHelper::getDataDir() const 
{ 
	return m_sDataDir; 
}

inline QString TestHelper::getTestDataDir() const 
{ 
	return m_sTestDataDir;
}

inline QString TestHelper::getTestFile(const QString& file) const
{
	return m_sTestDataDir + file; 
}

/** @name Test-instance accessors (ADR 0015, T1.5 fixture)
 * Resolve the suite's engine instance held by TestHelper, replacing the
 * transitional Hydrogen/Preferences/EventQueue::get_instance() shims in tests.
 * Use these only in tests that run against the harness singleton — tests that
 * construct their own Hydrogen must keep their explicit instance.
 * @{ */
H2Core::Hydrogen* pTestHydrogen();
std::shared_ptr<H2Core::Preferences> pTestPreferences();
H2Core::EventQueue* pTestEventQueue();
/** @} */

#define H2TEST_FILE(name) TestHelper::get_instance()->getTestFile(name)
#define ASSERT_SONG(pSong) {                                                    \
	CPPUNIT_ASSERT( pSong != nullptr );				                            \
	CPPUNIT_ASSERT( pSong->getDrumkit() != nullptr );                           \
	CPPUNIT_ASSERT( ! pSong->getDrumkit()->hasMissingSamples() );               \
}
// Ensure consistency between POSIX systems and Windows.
#define ASSERT_PATH(sPath1, sPath2) {			\
		const auto sCleanedPath1 = QString( sPath1 ).replace( '\\', '/' ); \
		const auto sCleanedPath2 = QString( sPath2 ).replace( '\\', '/' ); \
		CPPUNIT_ASSERT( sCleanedPath1 == sCleanedPath2 );				\
}
#define ASSERT_PATH_UNEQUAL(sPath1, sPath2) {			\
		const auto sCleanedPath1 = QString( sPath1 ).replace( '\\', '/' ); \
		const auto sCleanedPath2 = QString( sPath2 ).replace( '\\', '/' ); \
		CPPUNIT_ASSERT( sCleanedPath1 != sCleanedPath2 );				\
}

#endif
