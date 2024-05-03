/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/CoreActionController.h>
#include <core/AudioEngine/AudioEngineTests.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/Helpers/Filesystem.h>

#include <iostream>

#include "TransportTestsTimebase.h"
#include "TestHelper.h"

#include "assertions/AudioFile.h"

using namespace H2Core;

void TransportTestsTimebase::testTransportProcessingJack() {
	___INFOLOG( "\n\n" );
	auto pHydrogen = Hydrogen::get_instance();
	auto pPref = Preferences::get_instance();

	// In case we are testing Timebase support we have to checkout both provided
	// types of synchronization.
	if ( pHydrogen->getJackTimebaseState() !=
		 JackAudioDriver::Timebase::None ) {

		pPref->m_JackBBTSync = Preferences::JackBBTSyncMethod::constMeasure;
		___INFOLOG( Preferences::JackBBTSyncMethodToQString(
						pPref->m_JackBBTSync ) );

		perform( &AudioEngineTests::testTransportProcessingJack );

		pPref->m_JackBBTSync = Preferences::JackBBTSyncMethod::identicalBars;
		___INFOLOG( Preferences::JackBBTSyncMethodToQString(
						pPref->m_JackBBTSync ) );
	}

	perform( &AudioEngineTests::testTransportProcessingJack );

	___INFOLOG( "\npassed\n" );
}

void TransportTestsTimebase::testTransportRelocationJack() {
	___INFOLOG( "\n\n" );
	auto pHydrogen = Hydrogen::get_instance();
	auto pPref = Preferences::get_instance();

	// In case we are testing Timebase support we have to checkout both provided
	// types of synchronization.
	if ( pHydrogen->getJackTimebaseState() !=
		 JackAudioDriver::Timebase::None ) {

		pPref->m_JackBBTSync = Preferences::JackBBTSyncMethod::constMeasure;
		___INFOLOG( Preferences::JackBBTSyncMethodToQString(
						pPref->m_JackBBTSync ) );

		perform( &AudioEngineTests::testTransportRelocationJack );

		pPref->m_JackBBTSync = Preferences::JackBBTSyncMethod::identicalBars;
		___INFOLOG( Preferences::JackBBTSyncMethodToQString(
						pPref->m_JackBBTSync ) );
	}

	perform( &AudioEngineTests::testTransportRelocationJack );

	___INFOLOG( "\npassed\n" );
}

void TransportTestsTimebase::perform( std::function<void()> func ) {
	try {
		func();
	} catch ( std::exception& err ) {
		CppUnit::Message msg( err.what() );
		throw CppUnit::Exception( msg );
	}
}
