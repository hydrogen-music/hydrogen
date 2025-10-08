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

#include <cppunit/extensions/HelperMacros.h>

#include "AdsrTest.h"
#include "AudioDriverTest.h"
#include "AutomationPathSerializerTest.cpp"
#include "AutomationPathTest.cpp"
#include "CoreActionControllerTest.h"
#include "DrumkitExportTest.h"
#include "EventQueueTest.h"
#include "FilesystemTest.h"
#include "ForwardCompatibilityTest.h"
#include "FunctionalTests.cpp"
#include "InstrumentListTest.cpp"
#include "LicenseTest.h"
#include "MemoryLeakageTest.h"
#include "MidiNoteTest.cpp"
#include "NoteTest.cpp"
#include "OscServerTest.h"
#include "PatternTest.h"
#include "SampleTest.cpp"
#include "SongExportTest.h"
#include "TimeTest.h"
#include "Translations.cpp"
#include "TransportTest.h"
#include "XmlTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION( ADSRTest );
CPPUNIT_TEST_SUITE_REGISTRATION( AudioDriverTest );
CPPUNIT_TEST_SUITE_REGISTRATION( AutomationPathSerializerTest );
CPPUNIT_TEST_SUITE_REGISTRATION( AutomationPathTest );
CPPUNIT_TEST_SUITE_REGISTRATION( CoreActionControllerTest );
CPPUNIT_TEST_SUITE_REGISTRATION( DrumkitExportTest );
CPPUNIT_TEST_SUITE_REGISTRATION( EventQueueTest );
CPPUNIT_TEST_SUITE_REGISTRATION( FilesystemTest );
CPPUNIT_TEST_SUITE_REGISTRATION( ForwardCompatibilityTest );
CPPUNIT_TEST_SUITE_REGISTRATION( FunctionalTest );
CPPUNIT_TEST_SUITE_REGISTRATION( InstrumentListTest );
CPPUNIT_TEST_SUITE_REGISTRATION( LicenseTest );
CPPUNIT_TEST_SUITE_REGISTRATION( MemoryLeakageTest );
CPPUNIT_TEST_SUITE_REGISTRATION( MidiNoteTest );
CPPUNIT_TEST_SUITE_REGISTRATION( NoteTest );
#ifdef H2CORE_HAVE_OSC
CPPUNIT_TEST_SUITE_REGISTRATION( OscServerTest );
#endif
CPPUNIT_TEST_SUITE_REGISTRATION( PatternTest );
CPPUNIT_TEST_SUITE_REGISTRATION( SampleTest );
CPPUNIT_TEST_SUITE_REGISTRATION( SongExportTest );
CPPUNIT_TEST_SUITE_REGISTRATION( TimeTest );
CPPUNIT_TEST_SUITE_REGISTRATION( TransportTest );
CPPUNIT_TEST_SUITE_REGISTRATION( UITranslationTest );
CPPUNIT_TEST_SUITE_REGISTRATION( XmlTest );
