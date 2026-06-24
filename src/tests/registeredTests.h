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

#include <cppunit/extensions/HelperMacros.h>

#include "AdsrTest.h"
#include "AssertionTest.cpp"
#include "AudioBenchmark.h"
#include "AudioDriverTest.h"
#include "AudioEngineTest.h"
#include "AudioExportTest.h"
#include "AutomationPathTest.h"
#include "CliTest.h"
#include "ConfigConcurrencyTest.h"
#include "CoreActionControllerTest.h"
#include "DrumkitExportTest.h"
#include "EngineAccessTest.h"
#include "EventQueueTest.h"
#include "FakePluginHostTest.h"
#include "FilesystemTest.h"
#include "DrumkitTest.h"
#include "EditorMirrorTest.h"
#include "EditorModeTest.h"
#include "H2ProjectTest.h"
#include "IpcProtocolTest.h"
#include "IpcTransportTest.h"
#include "PluginConfigTest.h"
#include "LicenseTest.h"
#include "LoggerInstanceTest.h"
#include "MemoryLeakageTest.h"
#include "MidiActionTest.h"
#include "MidiDriverTest.h"
#include "MidiExportTest.h"
#include "MidiNoteTest.h"
#include "MimeTest.h"
#include "MultiInstanceTest.h"
#include "NetworkTest.h"
#include "NoteTest.h"
#include "ObjectUuidTest.h"
#include "OnlineImporterTest.h"
#include "OscServerTest.h"
#include "OutputBusTest.h"
#include "PatternTest.h"
#include "PluginFeatureGateTest.h"
#include "PluginLifecycleTest.h"
#include "PluginMidiTest.h"
#include "PluginProcessTest.h"
#include "PluginStateTest.h"
#include "PreferencesInstanceTest.h"
#include "PreferencesPersistTest.h"
#include "SampleTest.h"
#include "SoundLibraryTest.h"
#include "TimeTest.h"
#include "Translations.cpp"
#include "TransportTest.h"
#include "XmlTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION( ADSRTest );
CPPUNIT_TEST_SUITE_REGISTRATION( AssertionTest );
CPPUNIT_TEST_SUITE_REGISTRATION( AudioBenchmark );
//CPPUNIT_TEST_SUITE_REGISTRATION( AudioDriverTest );
CPPUNIT_TEST_SUITE_REGISTRATION( AudioEngineTest );
CPPUNIT_TEST_SUITE_REGISTRATION( AudioExportTest );
CPPUNIT_TEST_SUITE_REGISTRATION( AutomationPathTest );
#if not defined(WIN32) and not defined (__APPLE__)
  // For now h2cli is just part of our Linux package.
  CPPUNIT_TEST_SUITE_REGISTRATION( CliTest );
#endif
CPPUNIT_TEST_SUITE_REGISTRATION( ConfigConcurrencyTest );
CPPUNIT_TEST_SUITE_REGISTRATION( CoreActionControllerTest );
CPPUNIT_TEST_SUITE_REGISTRATION( DrumkitExportTest );
CPPUNIT_TEST_SUITE_REGISTRATION( EngineAccessTest );
CPPUNIT_TEST_SUITE_REGISTRATION( EventQueueTest );
CPPUNIT_TEST_SUITE_REGISTRATION( FakePluginHostTest );
CPPUNIT_TEST_SUITE_REGISTRATION( FilesystemTest );
CPPUNIT_TEST_SUITE_REGISTRATION( DrumkitTest );
CPPUNIT_TEST_SUITE_REGISTRATION( EditorMirrorTest );
CPPUNIT_TEST_SUITE_REGISTRATION( EditorModeTest );
CPPUNIT_TEST_SUITE_REGISTRATION( H2ProjectTest );
CPPUNIT_TEST_SUITE_REGISTRATION( IpcProtocolTest );
CPPUNIT_TEST_SUITE_REGISTRATION( IpcTransportTest );
CPPUNIT_TEST_SUITE_REGISTRATION( PluginConfigTest );
CPPUNIT_TEST_SUITE_REGISTRATION( LicenseTest );
CPPUNIT_TEST_SUITE_REGISTRATION( LoggerInstanceTest );
CPPUNIT_TEST_SUITE_REGISTRATION( MemoryLeakageTest );
CPPUNIT_TEST_SUITE_REGISTRATION( MimeTest );
CPPUNIT_TEST_SUITE_REGISTRATION( MidiActionTest );
CPPUNIT_TEST_SUITE_REGISTRATION( MidiDriverTest );
CPPUNIT_TEST_SUITE_REGISTRATION( MidiExportTest );
CPPUNIT_TEST_SUITE_REGISTRATION( MidiNoteTest );
CPPUNIT_TEST_SUITE_REGISTRATION( MultiInstanceTest );
CPPUNIT_TEST_SUITE_REGISTRATION( NetworkTest );
CPPUNIT_TEST_SUITE_REGISTRATION( NoteTest );
CPPUNIT_TEST_SUITE_REGISTRATION( ObjectUuidTest );
CPPUNIT_TEST_SUITE_REGISTRATION( OnlineImporterTest );
CPPUNIT_TEST_SUITE_REGISTRATION( OutputBusTest );
#ifdef H2CORE_HAVE_OSC
CPPUNIT_TEST_SUITE_REGISTRATION( OscServerTest );
#endif
CPPUNIT_TEST_SUITE_REGISTRATION( PatternTest );
CPPUNIT_TEST_SUITE_REGISTRATION( PluginFeatureGateTest );
CPPUNIT_TEST_SUITE_REGISTRATION( PluginLifecycleTest );
CPPUNIT_TEST_SUITE_REGISTRATION( PluginMidiTest );
CPPUNIT_TEST_SUITE_REGISTRATION( PluginProcessTest );
CPPUNIT_TEST_SUITE_REGISTRATION( PluginStateTest );
CPPUNIT_TEST_SUITE_REGISTRATION( PreferencesInstanceTest );
CPPUNIT_TEST_SUITE_REGISTRATION( PreferencesPersistTest );
CPPUNIT_TEST_SUITE_REGISTRATION( SampleTest );
CPPUNIT_TEST_SUITE_REGISTRATION( SoundLibraryTest );
CPPUNIT_TEST_SUITE_REGISTRATION( TimeTest );
CPPUNIT_TEST_SUITE_REGISTRATION( TransportTest );
CPPUNIT_TEST_SUITE_REGISTRATION( UITranslationTest );
CPPUNIT_TEST_SUITE_REGISTRATION( XmlTest );
