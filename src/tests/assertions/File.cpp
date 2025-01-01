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
	checkFileArgs( sExpected, f1, sActual, f2, bEquality, FileType::Xml,
				   sourceLine );

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
								 const bool bEquality,
								 const H2Test::FileType& fileType,
								 CppUnit::SourceLine sourceLine ) {
	QFile f1( sExpected );
	QFile f2( sActual );
	checkFileArgs( sExpected, f1, sActual, f2, bEquality, fileType, sourceLine );

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
			QStringList expectedLines = sDocExpected.split( "\n" );
			QStringList actualLines = sDocActual.split( "\n" );

			if ( fileType == FileType::Preferences ) {
				// In the preferences config files there are various elements
				// `lastXXDirectory` used to cache last folders selected in
				// various file browsers. These elements must not exist (to
				// allow them to fallback to e.g. the users home directory).
				QStringList linesToRemove;
				for ( const auto& ssLine : actualLines ) {
					if ( ssLine.contains( "<lastExportPatternAsDirectory>" ) ||
						 ssLine.contains( "<lastExportSongDirectory>" ) ||
						 ssLine.contains( "<lastSaveSongAsDirectory>" ) ||
						 ssLine.contains( "<lastOpenSongDirectory>" ) ||
						 ssLine.contains( "<lastOpenPatternDirectory>" ) ||
						 ssLine.contains( "<lastExportLilypondDirectory>" ) ||
						 ssLine.contains( "<lastExportMidiDirectory>" ) ||
						 ssLine.contains( "<lastImportDrumkitDirectory>" ) ||
						 ssLine.contains( "<lastExportDrumkitDirectory>" ) ||
						 ssLine.contains( "<lastOpenLayerDirectory>" ) ||
						 ssLine.contains( "<lastOpenPlaybackTrackDirectory>" ) ||
						 ssLine.contains( "<lastAddSongToPlaylistDirectory>" ) ||
						 ssLine.contains( "<lastPlaylistDirectory>" ) ||
						 ssLine.contains( "<lastPlaylistScriptDirectory>" ) ||
						 ssLine.contains( "<lastImportThemeDirectory>" ) ||
						 ssLine.contains( "<lastExportThemeDirectory>" ) ) {
						linesToRemove << ssLine;
					}
				}
				for ( const auto& ssRemoveLine : linesToRemove ) {
					CPPUNIT_ASSERT( actualLines.removeAll( ssRemoveLine ) == 1 );
				}
			}
			else if ( fileType == FileType::Drumkit ) {
				QStringList actualLinesToRemove;
				QStringList expectedLinesToRemove;
#ifdef WIN32
				// Drop all comment lines (Drumkit GPL license notice) as their
				// line break mess up comparison within the AppVeyor Windows
				// pipeline (but not locally which is why I use this more
				// sluggish way of dealing with it).
				for ( const auto& ssLine : actualLines ) {
					// Ignore both comments and lines which do not contain a
					// valid XML element.
					if ( ssLine.contains( "<!--" ) ||
						 ssLine.contains( "-->" ) ||
						 ! ( ssLine.contains( "<" ) && ssLine.contains( ">" ) ||
							 ssLine.contains( "</" ) && ssLine.contains( ">" ) ) ) {
						actualLinesToRemove << ssLine;
					}
				}
				for ( const auto& ssLine : expectedLines ) {
					// Ignore both comments and lines which do not contain a
					// valid XML element.
					if ( ssLine.contains( "<!--" ) ||
						 ssLine.contains( "-->" ) ||
						 ! ( ssLine.contains( "<" ) && ssLine.contains( ">" ) ||
							 ssLine.contains( "</" ) && ssLine.contains( ">" ) ) ) {
						expectedLinesToRemove << ssLine;
					}
				}
#else
				// Ignore the copyright since it contains a timestamp and would
				// require us to update the copyright date of our drumkits every
				// year.
				for ( const auto& ssLine : actualLines ) {
					// Ignore both comments and lines which do not contain a
					// valid XML element.
					if ( ssLine.contains( "<!--Copyright" ) ) {
						actualLinesToRemove << ssLine;
					}
				}
				for ( const auto& ssLine : expectedLines ) {
					if ( ssLine.contains( "<!--Copyright" ) ) {
						expectedLinesToRemove << ssLine;
					}
				}
#endif
				for ( const auto& ssRemoveLine : actualLinesToRemove ) {
					actualLines.removeAll( ssRemoveLine );
				}
				for ( const auto& ssRemoveLine : expectedLinesToRemove ) {
					expectedLines.removeAll( ssRemoveLine );
				}
			}

			const int nMaxLines =
				std::max( expectedLines.size(), actualLines.size() );

			QString sMsgPart;
			for ( int ii = 0; ii < nMaxLines; ++ii ) {
				if ( ii >= expectedLines.size() || ii >= actualLines.size() ) {
					sMsgPart = QString( "in number of lines: expected [%1] - actual [%2]" )
						.arg( expectedLines.size() ).arg( actualLines.size() );
					break;
				}

				// Sometimes the attributes in the root element are written in a
				// different order. This is totally ok for with respect to the
				// XML standard but messes up file comparison.
				if ( expectedLines.at( ii ).contains( "xmlns:xsi" ) &&
					 actualLines.at( ii ).contains( "xmlns:xsi" ) ) {
					continue;
				}

				// In song files we ignore both the current Hydrogen versions
				// (changes with every commit) as well as sample and drumkit
				// paths (so, we do not have to teach Hydrogen to store song
				// with relative paths just to pass some unit tests).
				//
				// Preferences do only change the version when saving them
				// straight away.
				if ( ( fileType == FileType::Song ||
					   fileType == FileType::Preferences ) && (
						( expectedLines.at( ii ).contains( "<version>" ) &&
						  actualLines.at( ii ).contains( "<version>" ) )
						) ) {
					continue;
				}

				if ( fileType == FileType::Song  && (
						( expectedLines.at( ii ).contains( "<filename>" ) &&
						  actualLines.at( ii ).contains( "<filename>" ) ) ||
						( expectedLines.at( ii ).contains( "<lastLoadedDrumkitPath>" ) &&
						  actualLines.at( ii ).contains( "<lastLoadedDrumkitPath>" ) ) ||
						( expectedLines.at( ii ).contains( "<drumkitPath>" ) &&
						  actualLines.at( ii ).contains( "<drumkitPath>" ) )
						) ) {
					continue;
				}

#ifdef WIN32
				// Windows uses its own set of carriage returns and line feeds.
				// These might be introduced into the <info> elements and mess
				// up the unit test.
				if ( expectedLines.at( ii ).contains( "&#xd;" ) ||
					 actualLines.at( ii ).contains( "&#xd;" ) ||
					 expectedLines.at( ii ).contains( "&#xa;" ) ||
					 actualLines.at( ii ).contains( "&#xa;" ) ) {
					continue;
				}
#endif

				if ( expectedLines.at( ii ) != actualLines.at( ii ) ) {
					sMsgPart = QString( "at line [%1]:\n\texpected: %2\n\tactual  : %3" )
						.arg( ii ).arg( expectedLines.at( ii ) )
						.arg( actualLines.at( ii ) );
					break;
				}
			}

			if ( ! sMsgPart.isEmpty() ) {
				CppUnit::Message msg(
					std::string( "XML files differ " ) + sMsgPart.toStdString(),
					std::string( "Expected: " ) + sExpected.toStdString(),
					std::string( "Actual  : " ) + sActual.toStdString() );
				throw CppUnit::Exception( msg, sourceLine );
			}
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
							const bool bEquality,
							const H2Test::FileType& fileType,
							CppUnit::SourceLine sourceLine ) {

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

#ifndef WIN32
	// We omit the test for file size on Windows since it can/does introduce
	// additional carriage return and line feed symbols in <info> elements
	// breaking the unit tests.
	if ( f1.size() != f2.size() && bEquality && fileType == FileType::Drumkit ) {
		CppUnit::Message msg(
			"File size differ",
			std::string( "Expected: " ) + sExpected.toStdString(),
			std::string( "Actual  : " ) + sActual.toStdString() );
		throw CppUnit::Exception( msg, sourceLine );
	}
#endif
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
			CppUnit::Message msg(
				QString( "Mismatching number of file in directories\n")
				.append( QString( " * Expected : %1 [%2]\n" )
						 .arg( contentExpected.size() ).arg( sDirExpected ) )
				.append( QString( " * Actual   : %1 [%2]\n" )
						 .arg( contentActual.size() ).arg( sDirActual ) )
				.toStdString() );
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
