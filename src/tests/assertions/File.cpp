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
#include "File.h"

#include <cmath>

#include <core/Helpers/Xml.h>

#include <QDir>
#include <QFileInfo>

static constexpr qint64 BUFFER_SIZE = 4096;


void H2Test::checkFilesEqual( const QString& sExpected, const QString& sActual,
							  bool bEquality, CppUnit::SourceLine sourceLine ) {
	QFile f1( sExpected );
	QFile f2( sActual );
	checkFileArgs( sExpected, f1, sActual, f2, bEquality, sourceLine );


	auto remaining = f1.size();
	qint64 offset = 0;
	while ( remaining > 0 ) {
		char buf1[BUFFER_SIZE];
		char buf2[BUFFER_SIZE];

		qint64 toRead = qMin( remaining, (qint64)BUFFER_SIZE );
		auto r1 = f1.read( buf1, toRead );
		if ( r1 != toRead ) {
			if ( bEquality ) {
				throw CppUnit::Exception(
					CppUnit::Message( "Short read or read error" ), sourceLine );
			} else {
				return;
			}
		}

		auto r2 = f2.read( buf2, toRead );
		if ( r2 != toRead ) {
			if ( bEquality ) {
				throw CppUnit::Exception(
					CppUnit::Message( "Short read or read error" ), sourceLine );
			} else {
				return;
			}
		}

		for ( int i = 0; i < r1; i++ ) {
			if ( buf1[i] != buf2[i] ) {
				if ( bEquality ) {
					auto diffLocation = offset + i + 1;
					CppUnit::Message msg(
						std::string( "Files differ at byte " ) +
						std::to_string( diffLocation ),
						std::string( "Expected: " ) + sExpected.toStdString(),
						std::string( "Actual  : " ) + sActual.toStdString() );
					throw CppUnit::Exception( msg, sourceLine );
				}
				else {
					return;
				}
			}
		}

		offset += r1;
		remaining -= r1;
	}

	if ( ! bEquality ) {
		CppUnit::Message msg(
			std::string( "Files are idential" ),
			std::string( "Expected: " ) + sExpected.toStdString(),
			std::string( "Actual  : " ) + sActual.toStdString() );
		throw CppUnit::Exception( msg, sourceLine );
	}
}

void H2Test::checkXmlFilesEqual( const QString& sExpected, const QString& sActual,
								 bool bEquality, CppUnit::SourceLine sourceLine ) {
	QFile f1( sExpected );
	QFile f2( sActual );
	checkFileArgs( sExpected, f1, sActual, f2, bEquality, sourceLine );

	H2Core::XMLDoc docExpected, docActual;
	if ( ! docExpected.read( sExpected ) ) {
		CppUnit::Message msg( QString( "Unable to parse expected document [%1]" )
							  .arg( sExpected ).toStdString() );
		throw CppUnit::Exception( msg, sourceLine );
	}
	if ( ! docActual.read( sActual ) ) {
		CppUnit::Message msg( QString( "Unable to parse actual document [%1]" )
							  .arg( sActual ).toStdString() );
		throw CppUnit::Exception( msg, sourceLine );
	}

	const QString sDocExpected = docExpected.toString();
	const QString sDocActual = docActual.toString();

	if ( sDocExpected != sDocActual ) {
		if ( bEquality ) {
			// Does not match. Let's compare it line by line to produce a more
			// helpful assert message.
			const QStringList expectedLines = sDocExpected.split( "\n" );
			const QStringList actualLines = sDocActual.split( "\n" );
			const int nMaxLines =
				std::max( expectedLines.size(), actualLines.size() );

			QString sMsgPart;
			for ( int ii = 0; ii < nMaxLines; ++ii ) {
				if ( ii >= expectedLines.size() || ii >= actualLines.size() ) {
					sMsgPart = QString( "in number of lines: expected [%1] - actual [%2]" )
						.arg( expectedLines.size() ).arg( actualLines.size() );
					break;
				}
				if ( expectedLines.at( ii ) != actualLines.at( ii ) ) {
					sMsgPart = QString( "at line [%1]:\n\texpected: %2\n\tactual  : %3" )
						.arg( ii ).arg( expectedLines.at( ii ) )
						.arg( actualLines.at( ii ) );
					break;
				}
			}

			CppUnit::Message msg(
				std::string( "XML files differ " ) + sMsgPart.toStdString(),
				std::string( "Expected: " ) + sExpected.toStdString(),
				std::string( "Actual  : " ) + sActual.toStdString() );
			throw CppUnit::Exception( msg, sourceLine );
		}
		else {
			return;
		}
	}

	if ( ! bEquality ) {
		CppUnit::Message msg(
			std::string( "Files are idential" ),
			std::string( "Expected: " ) + sExpected.toStdString(),
			std::string( "Actual  : " ) + sActual.toStdString() );
		throw CppUnit::Exception( msg, sourceLine );
	}
}

void H2Test::checkFileArgs( const QString& sExpected, QFile& f1,
							const QString& sActual, QFile& f2,
							bool bEquality, CppUnit::SourceLine sourceLine ) {

	if ( ! f1.open( QIODevice::ReadOnly ) ) {
		CppUnit::Message msg(
			std::string( "Can't open reference file: " ) +
			f1.errorString().toStdString(),
			std::string( "Expected: ") + sExpected.toStdString() );
		throw CppUnit::Exception( msg, sourceLine );
	}
	if ( ! f2.open( QIODevice::ReadOnly ) ) {
		CppUnit::Message msg(
			std::string( "Can't open result file: " ) +
			f2.errorString().toStdString(),
			std::string( "Actual  : " ) + sActual.toStdString() );
		throw CppUnit::Exception( msg, sourceLine );
	}
	if ( f1.size() != f2.size() && bEquality ) {
		CppUnit::Message msg(
			"File size differ",
			std::string( "Expected: " ) + sExpected.toStdString(),
			std::string( "Actual  : " ) + sActual.toStdString() );
		throw CppUnit::Exception( msg, sourceLine );
	}
}

void H2Test::checkDirsEqual( const QString& sDirExpected,
							 const QString& sDirActual, bool bEquality,
							 CppUnit::SourceLine sourceLine ) {

	QDir dirExpected( sDirExpected );
	QDir dirActual( sDirActual );

	QStringList contentExpected =
		dirExpected.entryList( QDir::Files | QDir::NoDotAndDotDot );
	QStringList contentActual =
		dirActual.entryList( QDir::Files | QDir::NoDotAndDotDot );

	if ( contentExpected.size() != contentActual.size() ) {
		if ( bEquality ) {
			CppUnit::Message msg( std::string("Mismatching number of file in directories "),
								  std::string("Expected: ") + std::to_string( contentExpected.size() ),
								  std::string("Actual  : ") + std::to_string( contentActual.size() ) );
			throw CppUnit::Exception(msg, sourceLine);
		} else {
			return;
		}
	}

	bool bFileDiffers = false;
	for ( const auto& ssFile : contentExpected ) {
		QString sFileActual( dirActual.filePath( ssFile ) );
		QString sFileExpected( dirExpected.filePath( ssFile ) );

		if ( ! QFileInfo( sFileActual ).exists() ) {
			if ( bEquality ) {
				CppUnit::Message msg( std::string("File [") + ssFile.toStdString() +
									  std::string("] does exist in the expected but not in the actual folder.") );
				throw CppUnit::Exception(msg, sourceLine);
			} else {
				return;
			}
		}

		try {
			checkFilesEqual( sFileExpected, sFileActual, bEquality, sourceLine );
			if ( ! bEquality ) {
				// If we want to folders to differ, it is enough to have just a
				// single difference in a single file.
				bFileDiffers = true;
			}
		} catch ( CppUnit::Exception exception) {
			if ( bEquality ) {
				// If we want two folders to be identical, every difference is
				// fatal.
				throw exception;
			}
		}
	}

	if ( ! bEquality && ! bFileDiffers ) {
		CppUnit::Message msg(
			"Folders match",
			std::string( "Expected: " ) + sDirExpected.toStdString(),
			std::string( "Actual  : " ) + sDirActual.toStdString() );
		throw CppUnit::Exception( msg, sourceLine );
	}
}
