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

#include "PluginInstanceIdentityTest.h"

#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/Logger.h>

#include "utils/FakePluginHost.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <cppunit/extensions/HelperMacros.h>

using namespace H2Core;

namespace {

// The instance log path Hydrogen's plugin branch builds for suffix n: the
// process default log's base name plus the suffix, in the tmp dir.
QString instanceLogPath( int n ) {
	const QFileInfo logInfo( Filesystem::logFilePath() );
	return Filesystem::tmpDir() + "/" + logInfo.completeBaseName() +
		QString( "_%1." ).arg( n ) + logInfo.suffix();
}

// Remove every <base>_<n>.<suffix> instance log of this suite so the suffix
// allocation below is hermetic: instance logs of earlier tests in this binary
// must not shift the suffixes this test asserts. The suite's own log file
// carries no _<n> suffix and is never touched.
void cleanInstanceLogNamespace() {
	for ( int n = 0; n < 10000; ++n ) {
		QFile file( instanceLogPath( n ) );
		if ( ! file.exists() ) {
			break;
		}
		file.remove();
	}
}

} // namespace

void PluginInstanceIdentityTest::testInstanceLogTakesFirstFreeSuffix() {
	___INFOLOG( "" );

	cleanInstanceLogNamespace();

	// Simulate another instance already holding the first suffix - e.g. one
	// in another format module of the same host process, whose per-module
	// copy of the core cannot share a counter with this one.
	QFile taken( instanceLogPath( 0 ) );
	CPPUNIT_ASSERT( taken.open( QIODevice::WriteOnly ) );
	taken.close();

	FakePluginHost host;
	CPPUNIT_ASSERT( host.getHydrogen()->getLogger()->getLogFile() ==
					instanceLogPath( 1 ) );
	// The exclusively created log file is the process-wide instance id
	// source: the editor endpoint and the plugin state cache key are keyed
	// on it and must not collide with the instance holding suffix 0.
	CPPUNIT_ASSERT( host.getHydrogen()->getInstanceId() == 1 );

	___INFOLOG( "passed" );
}

void PluginInstanceIdentityTest::testDistinctInstancesGetDistinctLogsAndIds() {
	___INFOLOG( "" );

	cleanInstanceLogNamespace();

	FakePluginHost first;
	FakePluginHost second;

	CPPUNIT_ASSERT( first.getHydrogen()->getLogger()->getLogFile() ==
					instanceLogPath( 0 ) );
	CPPUNIT_ASSERT( second.getHydrogen()->getLogger()->getLogFile() ==
					instanceLogPath( 1 ) );
	CPPUNIT_ASSERT( first.getHydrogen()->getInstanceId() == 0 );
	CPPUNIT_ASSERT( second.getHydrogen()->getInstanceId() == 1 );

	___INFOLOG( "passed" );
}

CPPUNIT_TEST_SUITE_REGISTRATION( PluginInstanceIdentityTest );
