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

#include <core/Helpers/Archive.h>

#include <core/config.h>
#include <core/Helpers/Filesystem.h>

#include <QDir>
#include <QFileInfo>
#include <QSaveFile>

#ifdef H2CORE_HAVE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>

#include <clocale>
#endif

namespace H2Core {

#ifdef H2CORE_HAVE_LIBARCHIVE

// Shared extraction loop used by both public entry points of this class. It
// takes ownership of @a pArchive and frees it on every exit.
static bool extractArchive( struct archive* pArchive, const QString& sTargetDir,
							bool bSilent, QStringList* pExtractedPaths,
							bool* pEncodingIssuesDetected ) {
	if ( pEncodingIssuesDetected != nullptr ) {
		*pEncodingIssuesDetected = false;
	}

	if ( ! QDir().mkpath( sTargetDir ) ) {
		___ERRORLOG( QString( "Unable to create target dir [%1]" )
					 .arg( sTargetDir ) );
		archive_read_free( pArchive );
		return false;
	}

	// Drumkits shipped by Hydrogen may contain UTF-8 encoded file and folder
	// names. On systems whose locale does not support UTF-8 those have to be
	// stripped of the problematic characters (flagged via
	// pEncodingIssuesDetected) or the resulting paths would be unusable.
	bool bUseUtf8Encoding = true;
	if ( nullptr == setlocale( LC_ALL, "en_US.UTF-8" ) ) {
		___INFOLOG( "No en_US.UTF-8 locale available on this system" );
		bUseUtf8Encoding = false;
	}

#ifdef WIN32
	// On Windows the `libarchive` build used in the Windows CI converts
	// the entry names read from the archive headers using the console
	// codepage - and not the raw bytes stored in the archive - whenever a
	// UTF-8 locale is active. Names of files and folders containing
	// non-ASCII characters would be mangled this way and, since the
	// conversion happens while reading the headers, there is no way to
	// recover the original ones afterwards. As we handle all encoding
	// ourselves using Qt, we switch the codeset-relevant part of the
	// locale off for the duration of the reading. In a "C" locale
	// `libarchive` passes the raw bytes through and
	// `archive_entry_pathname()` yields the unmodified (UTF-8) names.
	const QByteArray sLocaleCtype = setlocale( LC_CTYPE, nullptr );
	if ( setlocale( LC_CTYPE, "C" ) == nullptr ) {
		___WARNINGLOG( "Unable to switch to the C locale for reading the archive" );
	}
#endif

	// Shutdown routine used on error. Therefore, contained commands are not
	// checked for errors themselves.
	auto tearDown = [&]() {
		archive_read_close( pArchive );

#ifdef WIN32
		setlocale( LC_CTYPE, sLocaleCtype.constData() );
#endif

#if ARCHIVE_VERSION_NUMBER < 3000000
		archive_read_finish( pArchive );
#else
		archive_read_free( pArchive );
#endif
	};

	int nRet;
	struct archive_entry* entry;
	while ( ( nRet = archive_read_next_header( pArchive, &entry ) )
			!= ARCHIVE_EOF ) {
		if ( nRet != ARCHIVE_OK ) {
			___ERRORLOG( QString( "Unable to read next archive header: %1" )
						 .arg( archive_error_string( pArchive ) ) );
			tearDown();
			return false;
		}
		if ( entry == nullptr ) {
			___ERRORLOG( "Couldn't read in next archive entry" );
			tearDown();
			return false;
		}

		// Do NOT use `archive_entry_pathname_utf8()`! On some systems -
		// e.g. Windows - it lazily converts the stored multibyte name
		// using the current locale and yields mangled entry names. The
		// plain multibyte version contains the raw bytes as stored within
		// the archive (which are UTF-8 for all archives created by
		// Hydrogen itself). In case they are not valid UTF-8, the
		// check below will handle them.
		QString sNewPath = QString::fromUtf8( archive_entry_pathname( entry ) );
		if ( sNewPath.isEmpty() ) {
			___ERRORLOG( "Couldn't read pathname of archive entry" );
			tearDown();
			return false;
		}

		if ( ! bUseUtf8Encoding || sNewPath.contains( QChar( 0xFFFD ) ) ) {
			// In case `libarchive` is not able to support UTF-8 on the
			// system, we remove (a lot of) characters. Else they would be
			// represented by wacky ones and the calling routine would have
			// no idea where the extracted files did end up.
			//
			// The second condition covers archives with entry names which
			// are not valid UTF-8 - e.g. encoded using a legacy codepage.
			// `QString::fromUtf8()` does not fail for those but replaces
			// all problematic characters with the Unicode replacement
			// character.
			const auto sNewPathTrimmed = Filesystem::removeUtf8Characters(
				sNewPath );
			if ( sNewPathTrimmed != sNewPath ) {
				___ERRORLOG( QString( "Encoding error! File was renamed [%1] -> [%2]" )
							 .arg( sNewPath ).arg( sNewPathTrimmed ) );
				if ( pEncodingIssuesDetected != nullptr ) {
					*pEncodingIssuesDetected = true;
				}
				sNewPath = sNewPathTrimmed;
			}
		}

		// The archives are created by Hydrogen itself. But a malformed or
		// malicious one must never cause us to write outside sTargetDir.
		if ( sNewPath.startsWith( "/" ) || sNewPath == ".." ||
			 sNewPath.contains( "../" ) ) {
			___ERRORLOG( QString( "Unsafe entry path [%1] in archive. Aborting" )
						 .arg( sNewPath ) );
			tearDown();
			return false;
		}

		sNewPath.prepend( sTargetDir + "/" );

		// The archive may reference folders in entry paths without shipping
		// explicit folder entries (e.g. the `samples/` folder of a
		// .h2project).
		if ( ! QDir().mkpath( QFileInfo( sNewPath ).absolutePath() ) ) {
			___ERRORLOG( QString( "Unable to create parent dir of [%1]" )
						 .arg( sNewPath ) );
			tearDown();
			return false;
		}

		// We do not use `archive_read_extract()` but write the files
		// ourselves using the Qt classes. The former relies on
		// `libarchive` doing the multibyte to wide character conversion
		// when creating the files - which is done using the system
		// codepage on Windows and results in unusable paths for entry
		// names containing non-ASCII characters. By using the QString we
		// just assembled, extracted files and folders will be written
		// using proper UTF-16 paths (Qt handles the conversion) and match
		// exactly the ones reported via pExtractedPaths.
		if ( archive_entry_filetype( entry ) == AE_IFDIR ) {
			if ( ! QDir().mkpath( sNewPath ) ) {
				___ERRORLOG( QString( "Unable to create directory [%1]" )
							 .arg( sNewPath ) );
				tearDown();
				return false;
			}
		}
		else if ( archive_entry_hardlink( entry ) != nullptr ||
				  archive_entry_symlink( entry ) != nullptr ||
				  archive_entry_filetype( entry ) != AE_IFREG ) {
			___WARNINGLOG( QString( "Skipping unsupported entry [%1] of type [%2]" )
						   .arg( sNewPath )
						   .arg( archive_entry_filetype( entry ) ) );
		}
		else {
			QSaveFile file( sNewPath );
			if ( ! file.open( QIODevice::WriteOnly ) ) {
				___ERRORLOG( QString( "Unable to open file [%1] for writing: %2" )
							 .arg( sNewPath )
							 .arg( file.errorString() ) );
				tearDown();
				return false;
			}

			char buffer[ 65536 ];
			la_ssize_t nBytesRead;
			while ( ( nBytesRead = archive_read_data( pArchive, buffer,
													  sizeof( buffer ) ) )
					> 0 ) {
				if ( file.write( buffer, nBytesRead ) != nBytesRead ) {
					___ERRORLOG( QString( "Unable to write [%1] bytes to file [%2]" )
								 .arg( nBytesRead )
								 .arg( sNewPath ) );
					tearDown();
					return false;
				}
			}
			if ( nBytesRead < 0 ) {
				___ERRORLOG( QString( "Unable to read data of entry [%1]: %2" )
							 .arg( sNewPath )
							 .arg( archive_error_string( pArchive ) ) );
				tearDown();
				return false;
			}
			if ( ! file.commit() ) {
				___ERRORLOG( QString( "Unable to save file [%1]: %2" )
							 .arg( sNewPath )
							 .arg( file.errorString() ) );
				tearDown();
				return false;
			}
		}

		if ( pExtractedPaths != nullptr ) {
			pExtractedPaths->append( sNewPath );
		}
	}

	nRet = archive_read_close( pArchive );
	if ( nRet != ARCHIVE_OK ) {
		___ERRORLOG( QString( "Unable to close archive: %1" )
					 .arg( archive_error_string( pArchive ) ) );
		archive_read_free( pArchive );
#ifdef WIN32
		setlocale( LC_CTYPE, sLocaleCtype.constData() );
#endif
		return false;
	}

#if ARCHIVE_VERSION_NUMBER < 3000000
	archive_read_finish( pArchive );
#else
	nRet = archive_read_free( pArchive );
	if ( nRet != ARCHIVE_OK ) {
		___WARNINGLOG( QString( "Unable to free archive: %1" )
					   .arg( archive_error_string( pArchive ) ) );
	}
#endif

#ifdef WIN32
	setlocale( LC_CTYPE, sLocaleCtype.constData() );
#endif

	if ( ! bSilent ) {
		___INFOLOG( QString( "Extracted [%1] entries to [%2]" )
					.arg( pExtractedPaths != nullptr ?
						  pExtractedPaths->size() : 0 )
					.arg( sTargetDir ) );
	}

	return true;
}

static struct archive* createArchiveReader() {
	struct archive* pArchive = archive_read_new();
	if ( pArchive == nullptr ) {
		___ERRORLOG( "Unable to create new archive" );
		return nullptr;
	}

	int nRet;
#if ARCHIVE_VERSION_NUMBER < 3000000
	archive_read_support_compression_all( pArchive );
#else
	nRet = archive_read_support_filter_all( pArchive );
	if ( nRet != ARCHIVE_OK ) {
		___WARNINGLOG( QString( "Couldn't add support for all filters: %1" )
					   .arg( archive_error_string( pArchive ) ) );
	}
#endif

	nRet = archive_read_support_format_all( pArchive );
	if ( nRet != ARCHIVE_OK ) {
		___WARNINGLOG( QString( "Couldn't add support for all formats: %1" )
					   .arg( archive_error_string( pArchive ) ) );
	}

	return pArchive;
}

#endif // H2CORE_HAVE_LIBARCHIVE

bool Archive::extract( const QString& sArchivePath, const QString& sTargetDir,
					   bool bSilent, QStringList* pExtractedPaths,
					   bool* pEncodingIssuesDetected ) {
#ifndef H2CORE_HAVE_LIBARCHIVE
	___ERRORLOG( "libarchive is not available on this system. Unable to extract archive" );
	if ( pEncodingIssuesDetected != nullptr ) {
		*pEncodingIssuesDetected = false;
	}
	return false;
#else
	if ( ! bSilent ) {
		___INFOLOG( QString( "Extracting archive [%1] to [%2] using `libarchive` version [%3]" )
					.arg( sArchivePath ).arg( sTargetDir )
					.arg( ARCHIVE_VERSION_STRING ) );
	}

	struct archive* pArchive = createArchiveReader();
	if ( pArchive == nullptr ) {
		return false;
	}

	int nRet;
#if ARCHIVE_VERSION_NUMBER < 3000000
	const auto sArchivePathUtf8 = sArchivePath.toUtf8();
	nRet = archive_read_open_file( pArchive, sArchivePathUtf8.constData(),
								   10240 );
#else
  #ifdef WIN32
	QString sArchivePathPadded = sArchivePath;
	sArchivePathPadded.append( '\0' );
	auto archivePathW = sArchivePathPadded.toStdWString();
	nRet = archive_read_open_filename_w( pArchive, archivePathW.c_str(),
										 10240 );
  #else
	const auto sArchivePathUtf8 = sArchivePath.toUtf8();
	nRet = archive_read_open_filename( pArchive, sArchivePathUtf8.constData(),
									   10240 );
  #endif
#endif
	if ( nRet != ARCHIVE_OK ) {
		___ERRORLOG( QString( "Unable to open archive [%1] for reading: %2" )
					 .arg( sArchivePath )
					 .arg( archive_error_string( pArchive ) ) );
		archive_read_free( pArchive );
		return false;
	}

	return extractArchive( pArchive, sTargetDir, bSilent, pExtractedPaths,
						   pEncodingIssuesDetected );
#endif
}

bool Archive::extractFromBuffer(
	const std::vector<unsigned char>& data,
	const QString& sTargetDir,
	bool bSilent,
	QStringList* pExtractedPaths,
	bool* pEncodingIssuesDetected )
{
#ifndef H2CORE_HAVE_LIBARCHIVE
	___ERRORLOG( "libarchive is not available on this system. Unable to extract archive" );
	if ( pEncodingIssuesDetected != nullptr ) {
		*pEncodingIssuesDetected = false;
	}
	return false;
#else
	if ( data.empty() ) {
		___ERRORLOG( "Empty archive buffer" );
		return false;
	}

	if ( ! bSilent ) {
		___INFOLOG( QString( "Extracting in-memory archive of size [%1] to [%2] using `libarchive` version [%3]" )
					.arg( data.size() ).arg( sTargetDir )
					.arg( ARCHIVE_VERSION_STRING ) );
	}

	struct archive* pArchive = createArchiveReader();
	if ( pArchive == nullptr ) {
		return false;
	}

	if ( archive_read_open_memory( pArchive, data.data(), data.size() )
		 != ARCHIVE_OK ) {
		___ERRORLOG( QString( "Unable to open in-memory archive for reading: %1" )
					 .arg( archive_error_string( pArchive ) ) );
		archive_read_free( pArchive );
		return false;
	}

	return extractArchive( pArchive, sTargetDir, bSilent, pExtractedPaths,
						   pEncodingIssuesDetected );
#endif
}

};
