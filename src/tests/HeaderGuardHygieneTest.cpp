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

#include "HeaderGuardHygieneTest.h"

#include <core/config.h>

#include <QDirIterator>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <map>

namespace {

// Same detection as the one-shot rename: the first #ifndef/#define
// pair within the first 40 lines (behind the license header) is the
// include guard; #pragma once files carry none.
QString guardOf( const QString& sPath ) {
	QFile file( sPath );
	if ( ! file.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
		return QString();
	}

	QTextStream stream( &file );
	QStringList lines;
	for ( int i = 0; i < 45; ++i ) {
		const QString sLine = stream.readLine();
		if ( sLine.isNull() ) {
			break;
		}
		lines << sLine;
	}

	for ( int i = 0; i < 30 && i < lines.size(); ++i ) {
		if ( lines[ i ].contains( "#pragma once" ) ) {
			return QString();
		}
	}

	for ( int i = 0; i < 40 && i < lines.size(); ++i ) {
		const QString sTrimmed = lines[ i ].trimmed();
		if ( ! sTrimmed.startsWith( "#ifndef " ) ) {
			continue;
		}
		const QString sGuard = sTrimmed.mid( 8 ).trimmed();
		if ( sGuard.isEmpty() || sGuard.contains( ' ' ) ) {
			continue;
		}
		for ( int j = i + 1; j <= i + 5 && j < lines.size(); ++j ) {
			if ( lines[ j ].trimmed() == QString( "#define %1" ).arg( sGuard ) ) {
				return sGuard;
			}
		}
	}

	return QString();
}

QStringList headerFiles() {
	QStringList files;
	QDirIterator it( QString( CMAKE_SOURCE_DIR ) + "/src",
					{ "*.h", "*.hpp" }, QDir::Files,
					QDirIterator::Subdirectories );
	while ( it.hasNext() ) {
		files << it.next();
	}
	return files;
}

} // namespace

void HeaderGuardHygieneTest::testGuardPrefix() {
	QStringList offenders;
	for ( const QString& sFile : headerFiles() ) {
		const QString sGuard = guardOf( sFile );
		if ( ! sGuard.isEmpty() && ! sGuard.startsWith( "H2C_" ) ) {
			offenders << sGuard + "  " + sFile;
		}
	}

	if ( ! offenders.isEmpty() ) {
		CPPUNIT_FAIL( ( QString( "%1 headers carry a non-H2C_ include guard "
								 "(an SDK interlock can silently skip them; "
								 "first 10):\n%2" )
							.arg( offenders.size() )
							.arg( offenders.mid( 0, 10 ).join( "\n" ) ) )
						  .toStdString() );
	}
}

void HeaderGuardHygieneTest::testGuardUniqueness() {
	std::map<QString, QStringList> guards;
	for ( const QString& sFile : headerFiles() ) {
		const QString sGuard = guardOf( sFile );
		if ( ! sGuard.isEmpty() ) {
			guards[ sGuard ] << sFile;
		}
	}

	QStringList duplicates;
	for ( const auto& [ sGuard, files ] : guards ) {
		if ( files.size() > 1 ) {
			duplicates << sGuard + ": " + files.join( ", " );
		}
	}

	if ( ! duplicates.isEmpty() ) {
		CPPUNIT_FAIL( ( "duplicate include guards (the second include "
						"would be silently skipped):\n"
						+ duplicates.join( "\n" ) )
						  .toStdString() );
	}
}

CPPUNIT_TEST_SUITE_REGISTRATION( HeaderGuardHygieneTest );
