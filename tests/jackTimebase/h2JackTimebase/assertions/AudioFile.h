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

#ifndef ASSERTIONS_AUDIOFILE_H
#define ASSERTIONS_AUDIOFILE_H

#include <cppunit/extensions/HelperMacros.h>
#include <QString>

namespace H2Test {
	
	void checkAudioFilesEqual(const QString &expected, const QString &actual, CppUnit::SourceLine sourceLine);
	void checkAudioFilesDataEqual(const QString &expected, const QString &actual, CppUnit::SourceLine sourceLine);

}

/**
 * \brief Assert that two files' contents are the same
 **/
#define H2TEST_ASSERT_AUDIO_FILES_EQUAL(expected, actual) \
	H2Test::checkAudioFilesEqual(expected, actual, CPPUNIT_SOURCELINE())

/**
 * \brief Assert that two files' contents are the same expect for
 * tailing 0s in @a actual
 **/
#define H2TEST_ASSERT_AUDIO_FILES_DATA_EQUAL(expected, actual) \
	H2Test::checkAudioFilesDataEqual(expected, actual, CPPUNIT_SOURCELINE())

#endif

