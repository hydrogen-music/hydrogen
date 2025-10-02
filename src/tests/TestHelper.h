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

#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <QString>
#include <cassert>

#include <core/Midi/SMF.h>

class TestHelper {
	static TestHelper*	m_pInstance;
	QString m_sDataDir;
	QString m_sTestDataDir;
	bool m_bAppveyor;
	
	public:
		TestHelper();

	bool isAppveyor() const;
		QString getDataDir() const;
		QString getTestDataDir() const;
		QString getTestFile(const QString& file) const;
	QStringList findDrumkitBackupFiles( const QString& sDir ) const;

		static QString sRootDir;

	/**
	 * Picks different combinations of sample rate and buffer size of
	 * the FakeAudioDriver, stores them in the Preferences instance, and
	 * restarts the FakeAudioDriver.
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
	 */
	static void exportSong( const QString& sFileName );

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

#define H2TEST_FILE(name) TestHelper::get_instance()->getTestFile(name)
#define ASSERT_SONG(pSong) {                                                    \
	CPPUNIT_ASSERT( pSong != nullptr );				                            \
	CPPUNIT_ASSERT( pSong->getDrumkit() != nullptr );                           \
	CPPUNIT_ASSERT( ! pSong->getDrumkit()->hasMissingSamples() );               \
}

#endif
