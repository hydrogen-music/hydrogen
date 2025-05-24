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
#ifdef H2CORE_HAVE_JACK
	auto pHydrogen = Hydrogen::get_instance();
	auto pPref = Preferences::get_instance();

	perform( &AudioEngineTests::testTransportProcessingJack );
#else
	throw CppUnit::Exception(
		CppUnit::Message( "Unapplicable. Compiled without JACK support!" ) );
#endif

	___INFOLOG( "\npassed\n" );
}

void TransportTestsTimebase::testTransportProcessingOffsetsJack() {
	___INFOLOG( "\n\n" );
#ifdef H2CORE_HAVE_JACK
	auto pHydrogen = Hydrogen::get_instance();
	auto pPref = Preferences::get_instance();

	perform( &AudioEngineTests::testTransportProcessingOffsetsJack );
#else
	throw CppUnit::Exception(
		CppUnit::Message( "Unapplicable. Compiled without JACK support!" ) );
#endif

	___INFOLOG( "\npassed\n" );
}

void TransportTestsTimebase::testTransportRelocationJack() {
	___INFOLOG( "\n\n" );
#ifdef H2CORE_HAVE_JACK
	auto pHydrogen = Hydrogen::get_instance();
	auto pPref = Preferences::get_instance();

	perform( &AudioEngineTests::testTransportRelocationJack );
#else
	throw CppUnit::Exception(
		CppUnit::Message( "Unapplicable. Compiled without JACK support!" ) );
#endif

	___INFOLOG( "\npassed\n" );
}

void TransportTestsTimebase::testTransportRelocationOffsetsJack() {
	___INFOLOG( "\n\n" );
#ifdef H2CORE_HAVE_JACK
	auto pHydrogen = Hydrogen::get_instance();
	auto pPref = Preferences::get_instance();

	perform( &AudioEngineTests::testTransportRelocationOffsetsJack );
#else
	throw CppUnit::Exception(
		CppUnit::Message( "Unapplicable. Compiled without JACK support!" ) );
#endif

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
