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

	// Shutdown routine used on error. Therefore, contained commands are not
	// checked for errors themselves.
	auto tearDown = [&]() {
		archive_read_close( pArchive );

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

		QString sNewPath = QString::fromUtf8(
			archive_entry_pathname_utf8( entry ) );
		if ( sNewPath.isEmpty() ) {
			sNewPath = QString::fromUtf8( archive_entry_pathname( entry ) );
		}

		if ( ! bUseUtf8Encoding ) {
			// In case `libarchive` is not able to support UTF-8 on the
			// system, we remove (a lot of) characters. Else they would be
			// represented by wacky ones and the calling routine would have
			// no idea where the extracted files did end up.
			const auto sNewPathTrimmed = Filesystem::removeUtf8Characters(
				sNewPath );
			if ( sNewPathTrimmed != sNewPath ) {
				___ERRORLOG( QString( "Encoding error (no UTF-8 available)! File was renamed [%1] -> [%2]" )
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

		const auto sNewPathUtf8 = sNewPath.toUtf8();
		archive_entry_set_pathname( entry, sNewPathUtf8.data() );
		nRet = archive_read_extract( pArchive, entry, 0 );
		if ( nRet == ARCHIVE_WARN ) {
			___WARNINGLOG( QString( "Entry [%1] extracted with warnings: %2" )
						   .arg( sNewPath )
						   .arg( archive_error_string( pArchive ) ) );
		}
		else if ( nRet != ARCHIVE_OK ) {
			___ERRORLOG( QString( "Unable to extract entry [%1]: %2" )
						 .arg( sNewPath )
						 .arg( archive_error_string( pArchive ) ) );
			tearDown();
			return false;
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
