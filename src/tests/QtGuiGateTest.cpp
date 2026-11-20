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

#include "QtGuiGateTest.h"

#include "TestHelper.h"

#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTextStream>

namespace {

QStringList coreSources() {
	QStringList files;
	QDirIterator it( QString( CMAKE_SOURCE_DIR ) + "/src/core",
					{ "*.h", "*.cpp" }, QDir::Files,
					QDirIterator::Subdirectories );
	while ( it.hasNext() ) {
		files << it.next();
	}
	return files;
}

// Root of the Qt installation the tests are compiled against (e.g.
// .../qt6), derived from the QtCore header the build configured.
// Its per-module subdirectories are the ground truth for deciding
// which module a bare #include <QFoo> pulls in - no hardcoded class
// list to drift out of sync with Qt.
QString qtIncludeRoot() {
	const QFileInfo coreHeader( QString( QT_QNAMESPACE_H ) );
	return QFileInfo( coreHeader.absolutePath() ).absolutePath();
}

} // namespace

void QtGuiGateTest::testCoreSourcesFreeOfGuiIncludes() {
	___INFOLOG( "" );

	const QString sQtRoot = qtIncludeRoot();
	QStringList offenders;

	const QRegularExpression includePattern(
		"^\\s*#\\s*include\\s*<([^>]+)>" );
	const QRegularExpression bareClassPattern( "^Q[A-Z]\\w*$" );
	for ( const QString& sFile : coreSources() ) {
		QFile file( sFile );
		if ( ! file.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
			CPPUNIT_FAIL( ( "unable to open " + sFile ).toStdString() );
		}

		QTextStream stream( &file );
		int nLine = 0;
		while ( ! stream.atEnd() ) {
			const QString sLine = stream.readLine();
			++nLine;
			const auto match = includePattern.match( sLine );
			if ( ! match.hasMatch() ) {
				continue;
			}
			const QString sInclude = match.captured( 1 );

			if ( sInclude.startsWith( "QtGui" ) ||
				 sInclude.startsWith( "QtWidgets" ) ) {
				offenders << QString( "%1:%2: <%3>" )
					.arg( sFile ).arg( nLine ).arg( sInclude );
				continue;
			}

			if ( ! bareClassPattern.match( sInclude ).hasMatch() ) {
				continue;
			}
			for ( const QString& sModule : { "QtGui", "QtWidgets" } ) {
				if ( QFileInfo::exists( sQtRoot + "/" + sModule + "/" +
										sInclude + ".h" ) ) {
					offenders << QString( "%1:%2: <%3> (from %4)" )
						.arg( sFile ).arg( nLine )
						.arg( sInclude ).arg( sModule );
					break;
				}
			}
		}
	}

	if ( ! offenders.isEmpty() ) {
		CPPUNIT_FAIL( ( QString( "src/core must stay free of QtGui/QtWidgets "
								 "includes (headless boundary - convert at "
								 "the Skin.h boundary instead; first 10):\n%1" )
							.arg( offenders.mid( 0, 10 ).join( "\n" ) ) )
						  .toStdString() );
	}

	___INFOLOG( "passed" );
}

void QtGuiGateTest::testCoreCMakeFreeOfGuiLinkage() {
	___INFOLOG( "" );

	QFile file( QString( CMAKE_SOURCE_DIR ) + "/src/core/CMakeLists.txt" );
	if ( ! file.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
		CPPUNIT_FAIL( "unable to open src/core/CMakeLists.txt" );
	}

	const QString sContent = QTextStream( &file ).readAll();
	const QRegularExpression linkagePattern( "Qt[^\\s]*::(Gui|Widgets)" );
	const auto match = linkagePattern.match( sContent );
	if ( match.hasMatch() ) {
		CPPUNIT_FAIL( ( QString( "src/core/CMakeLists.txt links %1 - the "
								 "core library must stay headless" )
							.arg( match.captured( 0 ) ) )
						  .toStdString() );
	}

	___INFOLOG( "passed" );
}
