/*
 * Hydrogen
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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include "HeadlessEngineLauncherTest.h"

#include <core/Hydrogen.h>
#include <core/IPC/HeadlessEngineLauncher.h>
#include <core/Preferences/Preferences.h>

#include <QtCore/QCoreApplication>

using namespace H2Core;

void HeadlessEngineLauncherTest::testMakeEndpoint()
{
	___INFOLOG( "" );

	const QString sEndpoint = HeadlessEngineLauncher::makeEndpoint();

	CPPUNIT_ASSERT( ! sEndpoint.isEmpty() );
	CPPUNIT_ASSERT( sEndpoint.startsWith( "hydrogen-headless-" ) );

	// The endpoint must contain the current PID.
	const qint64 nPid = QCoreApplication::applicationPid();
	CPPUNIT_ASSERT( sEndpoint.contains( QString::number( nPid ) ) );

	___INFOLOG( "passed" );
}

void HeadlessEngineLauncherTest::testMakeEndpointUniqueness()
{
	___INFOLOG( "" );

	const QString s1 = HeadlessEngineLauncher::makeEndpoint();
	const QString s2 = HeadlessEngineLauncher::makeEndpoint();
	const QString s3 = HeadlessEngineLauncher::makeEndpoint();

	CPPUNIT_ASSERT( ! s1.isEmpty() );
	CPPUNIT_ASSERT( ! s2.isEmpty() );
	CPPUNIT_ASSERT( ! s3.isEmpty() );
	CPPUNIT_ASSERT( s1 != s2 );
	CPPUNIT_ASSERT( s2 != s3 );
	CPPUNIT_ASSERT( s1 != s3 );

	___INFOLOG( "passed" );
}

void HeadlessEngineLauncherTest::testFormatConnectionInfo()
{
	___INFOLOG( "" );

	const QString sEndpoint = "hydrogen-headless-12345-0";
	const QString sInfo = HeadlessEngineLauncher::formatConnectionInfo( sEndpoint );

	CPPUNIT_ASSERT( ! sInfo.isEmpty() );
	CPPUNIT_ASSERT( sInfo.contains( "IPC Server Started" ) );
	CPPUNIT_ASSERT( sInfo.contains( sEndpoint ) );
	CPPUNIT_ASSERT( sInfo.contains( "hydrogen -c" ) );
	CPPUNIT_ASSERT( sInfo.contains( "--connect-via-ipc" ) );
	CPPUNIT_ASSERT( sInfo.contains( "Ctrl+C" ) );

	___INFOLOG( "passed" );
}

void HeadlessEngineLauncherTest::testCreateHeadlessEngine()
{
	___INFOLOG( "" );

	// createHeadlessEngine() creates a fresh Preferences instance (not a
	// singleton — ADR 0015) and a new Hydrogen that owns it. We verify the
	// configuration through the returned Hydrogen's accessor.
	auto pHydrogen = HeadlessEngineLauncher::createHeadlessEngine();
	CPPUNIT_ASSERT( pHydrogen != nullptr );

	// The engine must be in headless GUI state.
	CPPUNIT_ASSERT( pHydrogen->getGUIState() == Hydrogen::GUIState::headless );

	// The engine's Preferences must be configured for headless IPC serving:
	// Null audio, no MIDI, no OSC.
	auto pPref = pHydrogen->getPreferences();
	CPPUNIT_ASSERT( pPref != nullptr );
	CPPUNIT_ASSERT( pPref->m_audioDriver == Preferences::AudioDriver::Null );
	CPPUNIT_ASSERT( pPref->m_midiDriver == Preferences::MidiDriver::None );
	CPPUNIT_ASSERT( ! pPref->getOscServerEnabled() );

	delete pHydrogen;

	___INFOLOG( "passed" );
}