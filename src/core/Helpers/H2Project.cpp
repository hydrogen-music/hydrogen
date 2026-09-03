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
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include <core/Helpers/H2Project.h>

#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Helpers/Archive.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

#include <QByteArray>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>

#include <map>

#ifdef H2CORE_HAVE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif

namespace H2Core {

// Entry names within the bundle.
static const char* SONG_ENTRY = "song.h2song";
static const char* MANIFEST_ENTRY = "manifest.txt";

// Folder a `.h2project` bundle with the cache key @a sCacheKey is extracted
// to.
static QString projectCacheDir( const QString& sCacheKey ) {
	return Filesystem::cacheDir() + "/projects/" + sCacheKey;
}

// Ensure the key only contains characters which are safe to be used within a
// single folder name - no separators and no traversal fragments.
static QString sanitizeCacheKey( const QString& sKey ) {
	QString sSanitized;
	for ( const auto& c : sKey ) {
		if ( c.isLetterOrNumber() || c == '-' || c == '_' || c == '.' ) {
			sSanitized.append( c );
		}
		else {
			sSanitized.append( '_' );
		}
	}
	return sSanitized;
}

// Cache key of a `.h2project` file on disk: its base name (so extraction
// folders remain identifiable when debugging) plus a hash of the absolute
// path (so files with identical base names do not collide).
static QString fileCacheKey( const QString& sPath ) {
	const QString sHash = QString::fromLatin1(
		QCryptographicHash::hash(
			QFileInfo( sPath ).absoluteFilePath().toUtf8(),
			QCryptographicHash::Sha1 ).toHex() ).left( 10 );
	return sanitizeCacheKey( QFileInfo( sPath ).completeBaseName() ) +
		"-" + sHash;
}

// Cache key of a plugin state: the host process and the Hydrogen instance
// are unique per session, so no stale folder of a previous run can collide.
static QString pluginCacheKey( const Hydrogen* pHydrogen ) {
	return QString( "plugin-%1-%2" )
		.arg( QCoreApplication::applicationPid() )
		.arg( pHydrogen != nullptr ? pHydrogen->getInstanceId() : -1 );
}

#ifdef H2CORE_HAVE_LIBARCHIVE

// ── libarchive in-memory write callbacks ──────────────────────────────────
static la_ssize_t memWriteCb( struct archive*, void* pClient,
							  const void* pBuffer, size_t nLength ) {
	auto* pVec = static_cast<std::vector<unsigned char>*>( pClient );
	const auto* p = static_cast<const unsigned char*>( pBuffer );
	pVec->insert( pVec->end(), p, p + nLength );
	return static_cast<la_ssize_t>( nLength );
}
static int memOpenCloseCb( struct archive*, void* ) {
	return ARCHIVE_OK;
}

static bool writeEntry( struct archive* a, const QString& sName,
						const QByteArray& data ) {
	struct archive_entry* entry = archive_entry_new();
	if ( entry == nullptr ) {
		return false;
	}
	const auto sNameUtf8 = sName.toUtf8();
	archive_entry_set_pathname( entry, sNameUtf8.constData() );
	archive_entry_set_size( entry, data.size() );
	archive_entry_set_filetype( entry, AE_IFREG );
	archive_entry_set_perm( entry, 0644 );

	bool bOk = ( archive_write_header( a, entry ) == ARCHIVE_OK );
	if ( bOk && data.size() > 0 ) {
		const la_ssize_t nWritten =
			archive_write_data( a, data.constData(), data.size() );
		bOk = ( nWritten == static_cast<la_ssize_t>( data.size() ) );
	}
	archive_entry_free( entry );
	return bOk;
}

#endif // H2CORE_HAVE_LIBARCHIVE

// Walk every (instrument, component, layer) holding a sample, in a stable order
// that is identical on write and read, and invoke fn(nInst, nComp, nLayer,
// pSample).
template <typename F>
static void forEachSample( std::shared_ptr<Song> pSong, F fn ) {
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}
	auto pInstruments = pSong->getDrumkit()->getInstruments();
	if ( pInstruments == nullptr ) {
		return;
	}
	for ( int ii = 0; ii < pInstruments->size(); ++ii ) {
		auto pInstrument = pInstruments->get( ii );
		if ( pInstrument == nullptr ) {
			continue;
		}
		auto pComponents = pInstrument->getComponents();
		if ( pComponents == nullptr ) {
			continue;
		}
		for ( int cc = 0; cc < static_cast<int>( pComponents->size() ); ++cc ) {
			auto pComponent = ( *pComponents )[ cc ];
			if ( pComponent == nullptr ) {
				continue;
			}
			const auto layers = pComponent->getLayers();
			for ( int ll = 0; ll < static_cast<int>( layers.size() ); ++ll ) {
				auto pLayer = layers[ ll ];
				if ( pLayer == nullptr || pLayer->getSample() == nullptr ) {
					continue;
				}
				fn( ii, cc, ll, pLayer->getSample() );
			}
		}
	}
	if ( pSong->getPlaybackTrackInstrument() != nullptr ) {
		auto pPlaybackInstrument = pSong->getPlaybackTrackInstrument();
		if ( pPlaybackInstrument->getComponent( 0 ) != nullptr &&
			 pPlaybackInstrument->getComponent( 0 )->getLayer( 0 ) != nullptr ) {
			auto pLayer = pPlaybackInstrument->getComponent( 0 )->getLayer( 0 );
			if ( pLayer->getSample() != nullptr ) {
				fn( -1, 0, 0, pLayer->getSample() );
			}
		}
	}
}

std::vector<unsigned char> H2Project::toBuffer( std::shared_ptr<Song> pSong,
												bool bSilent ) {
#ifndef H2CORE_HAVE_LIBARCHIVE
	___ERRORLOG( "libarchive not available; cannot build .h2project" );
	return {};
#else
	if ( pSong == nullptr ) {
		___ERRORLOG( "Invalid song" );
		return {};
	}

	// Collect the unique sample blobs (deduped by content hash), the manifest
	// mapping each (inst,comp,layer) slot to its blob entry, and a lookup
	// from sample file path to entry name. The latter is handed to the XML
	// serialization below so that `<filename>` elements reference bundle
	// entries instead of local paths - keeping the bundle portable and its
	// XML reproducible across machines.
	std::map<QString, QByteArray> sampleEntries; // entry name -> bytes
	std::map<QString, QString> hashToEntry;      // content hash -> entry name
	Xml::ProjectSampleEntries pathToEntry;       // sample path -> entry name
	QStringList manifestLines;

	forEachSample( pSong, [&]( int ii, int cc, int ll,
							   std::shared_ptr<Sample> pSample ) {
		const QString sPath = pSample->getFilePath();
		QFile file( sPath );
		if ( ! file.open( QIODevice::ReadOnly ) ) {
			___WARNINGLOG( QString( "Unable to read sample [%1]; skipping" )
						   .arg( sPath ) );
			return;
		}
		const QByteArray bytes = file.readAll();
		file.close();

		const QString sHash = QString::fromLatin1(
			QCryptographicHash::hash( bytes, QCryptographicHash::Sha1 ).toHex() );

		QString sEntry;
		auto it = hashToEntry.find( sHash );
		if ( it != hashToEntry.end() ) {
			sEntry = it->second;
		}
		else {
			const QString sSuffix = QFileInfo( sPath ).suffix();
			sEntry = QString( "samples/%1%2%3" )
						 .arg( sHash )
						 .arg( sSuffix.isEmpty() ? "" : "." )
						 .arg( sSuffix );
			hashToEntry[ sHash ] = sEntry;
			sampleEntries[ sEntry ] = bytes;
		}
		pathToEntry[ sPath ] = sEntry;

		manifestLines << QString( "%1 %2 %3 %4" )
							 .arg( ii ).arg( cc ).arg( ll ).arg( sEntry );
	} );

	const QByteArray songXml = pSong->toXmlBuffer(
		Xml::Flag::KeepMissingSamples | Xml::Flag::Project, bSilent,
		&pathToEntry
	);

	const QByteArray manifest = manifestLines.join( "\n" ).toUtf8();

	// Assemble the archive in memory.
	std::vector<unsigned char> out;

	struct archive* a = archive_write_new();
	if ( a == nullptr ) {
		___ERRORLOG( "Unable to create archive" );
		return {};
	}
	archive_write_add_filter_gzip( a );
	archive_write_set_format_pax_restricted( a );
	if ( archive_write_open( a, &out, memOpenCloseCb, memWriteCb,
							 memOpenCloseCb ) != ARCHIVE_OK ) {
		___ERRORLOG( QString( "Unable to open archive for writing: %1" )
					 .arg( archive_error_string( a ) ) );
		archive_write_free( a );
		return {};
	}

	bool bOk = writeEntry( a, SONG_ENTRY, songXml );
	bOk = bOk && writeEntry( a, MANIFEST_ENTRY, manifest );
	for ( const auto& [ sEntry, bytes ] : sampleEntries ) {
		bOk = bOk && writeEntry( a, sEntry, bytes );
	}

	archive_write_close( a );
	archive_write_free( a );

	if ( ! bOk ) {
		___ERRORLOG( "Error while writing .h2project archive" );
		return {};
	}

	if ( ! bSilent ) {
		___INFOLOG( QString( "Built .h2project: %1 bytes, %2 unique samples" )
					.arg( out.size() ).arg( sampleEntries.size() ) );
	}

	return out;
#endif
}

std::shared_ptr<Song> H2Project::fromBuffer(
	const std::vector<unsigned char>& data, const QString& sCacheKey,
	Hydrogen* pHydrogen, bool bSilent ) {
#ifndef H2CORE_HAVE_LIBARCHIVE
	___ERRORLOG( "libarchive not available; cannot read .h2project" );
	return nullptr;
#else
	if ( data.empty() ) {
		___ERRORLOG( "Empty .h2project buffer" );
		return nullptr;
	}
	if ( pHydrogen == nullptr ) {
		___ERRORLOG( "Invalid Hydrogen instance" );
		return nullptr;
	}
	if ( sCacheKey.isEmpty() ) {
		___ERRORLOG( "Empty cache key" );
		return nullptr;
	}

	// Extract the bundle into its per-key cache folder.
	const QString sExtractDir = projectCacheDir( sCacheKey );
	if ( QFileInfo( sExtractDir ).exists() ) {
		Filesystem::rm( sExtractDir, true, true );
	}

	QStringList extractedPaths;
	if ( ! Archive::extractFromBuffer( data, sExtractDir, bSilent,
									   &extractedPaths ) ) {
		if ( QFileInfo( sExtractDir ).exists() ) {
			Filesystem::rm( sExtractDir, true, true );
		}
		return nullptr;
	}
	pHydrogen->registerExtractedProjectDir( sExtractDir );

	const QString sSongPath = sExtractDir + "/" + SONG_ENTRY;
	QFile songFile( sSongPath );
	if ( ! songFile.open( QIODevice::ReadOnly ) ) {
		___ERRORLOG( QString( "No song entry in .h2project archive [%1]" )
					 .arg( sSongPath ) );
		return nullptr;
	}
	const QByteArray songXml = songFile.readAll();
	songFile.close();

	auto pSong = Song::fromXmlBuffer(
		songXml, Xml::Flag::Project, bSilent, pHydrogen
	);
	if ( pSong == nullptr ) {
		___ERRORLOG( "Unable to reconstruct song from .h2project" );
		return nullptr;
	}

	// Parse the manifest: "inst comp layer entry" per line.
	std::map<QString, QString> slotToEntry; // "i/c/l" -> entry name
	const QString sManifestPath = sExtractDir + "/" + MANIFEST_ENTRY;
	QFile manifestFile( sManifestPath );
	if ( manifestFile.open( QIODevice::ReadOnly ) ) {
		const QStringList lines =
			QString::fromUtf8( manifestFile.readAll() ).split(
				'\n', Qt::SkipEmptyParts );
		manifestFile.close();
		for ( const auto& sLine : lines ) {
			const QStringList parts = sLine.split( ' ' );
			if ( parts.size() >= 4 ) {
				const QString sKey = QString( "%1/%2/%3" )
										  .arg( parts[0] ).arg( parts[1] )
										  .arg( parts[2] );
				slotToEntry[ sKey ] = parts[3];
			}
		}
	}

	// Assign the extracted sample files to their slots. The actual decoding
	// happens through the regular file-based loading below.
	forEachSample( pSong, [&]( int ii, int cc, int ll,
							   std::shared_ptr<Sample> pSample ) {
		const QString sKey = QString( "%1/%2/%3" ).arg( ii ).arg( cc ).arg( ll );
		auto itSlot = slotToEntry.find( sKey );
		if ( itSlot == slotToEntry.end() ) {
			ERRORLOG( QString( "Couldn't find slot [%1]" ).arg( sKey ) );
			return;
		}
		const QString sSamplePath = sExtractDir + "/" + itSlot->second;
		if ( ! QFileInfo( sSamplePath ).exists() ) {
			ERRORLOG( QString( "Couldn't find extracted sample [%1]" )
					  .arg( sSamplePath ) );
			return;
		}
		pSample->setFilePath( sSamplePath );
	} );

	// Decode the samples via the standard, synchronous file-based code path.
	pSong->getDrumkit()->loadSamples( 120, pHydrogen->getPreferences().get() );
	if ( pSong->getPlaybackTrackInstrument() != nullptr ) {
		pSong->getPlaybackTrackInstrument()->loadSamples(
			120, pHydrogen->getPreferences().get() );
	}

	for ( auto ppInstrument : *pSong->getDrumkit()->getInstruments() ) {
		if ( ppInstrument != nullptr ) {
			ppInstrument->checkForMissingSamples(
				Event::Trigger::Suppress, pHydrogen
			);
		}
	}

	return pSong;
#endif
}

bool H2Project::save( std::shared_ptr<Song> pSong, const QString& sPath,
					  bool bSilent ) {
	const auto out = toBuffer( pSong, bSilent );
	if ( out.empty() ) {
		return false;
	}
	QFile file( sPath );
	if ( ! file.open( QIODevice::WriteOnly ) ) {
		___ERRORLOG( QString( "Unable to open [%1] for writing" ).arg( sPath ) );
		return false;
	}
	const qint64 nWritten = file.write(
		reinterpret_cast<const char*>( out.data() ),
		static_cast<qint64>( out.size() ) );
	file.close();
	return nWritten == static_cast<qint64>( out.size() );
}

std::shared_ptr<Song> H2Project::load( const QString& sPath,
									   Hydrogen* pHydrogen, bool bSilent ) {
	QFile file( sPath );
	if ( ! file.open( QIODevice::ReadOnly ) ) {
		___ERRORLOG( QString( "Unable to open [%1] for reading" ).arg( sPath ) );
		return nullptr;
	}
	const QByteArray raw = file.readAll();
	file.close();
	std::vector<unsigned char> data(
		reinterpret_cast<const unsigned char*>( raw.constData() ),
		reinterpret_cast<const unsigned char*>( raw.constData() ) + raw.size() );
	auto pSong = fromBuffer( data, fileCacheKey( sPath ), pHydrogen, bSilent );
	if ( pSong != nullptr ) {
		pSong->setPath( sPath );
	}
	return pSong;
}

std::shared_ptr<Song> H2Project::openSong( const QString& sPath,
											Hydrogen* pHydrogen, bool bSilent ) {
	// Peek the first bytes to detect the container, then defer to the proper
	// full loader for that format (Song::load handles legacy .h2song quirks).
	QFile file( sPath );
	if ( ! file.open( QIODevice::ReadOnly ) ) {
		___ERRORLOG( QString( "Unable to open [%1]" ).arg( sPath ) );
		return nullptr;
	}
	const QByteArray head = file.read( 16 );
	file.close();

	const std::vector<unsigned char> headVec(
		reinterpret_cast<const unsigned char*>( head.constData() ),
		reinterpret_cast<const unsigned char*>( head.constData() ) +
			head.size() );

	if ( looksLikeArchive( headVec ) ) {
		return H2Project::load( sPath, pHydrogen, bSilent );
	}
	return Song::load( sPath, bSilent, pHydrogen );
}

std::vector<unsigned char> H2Project::toState( std::shared_ptr<Song> pSong,
											   bool bEmbedSamples, bool bSilent ) {
	if ( pSong == nullptr ) {
		___ERRORLOG( "Invalid song" );
		return {};
	}

	if ( bEmbedSamples ) {
		// Portable: full bundle with embedded sample audio.
		return toBuffer( pSong, bSilent );
	}

	// Song-only: the .h2song XML; the kit must be installed to reload.
	const QByteArray xml = pSong->toXmlBuffer(
		Xml::Flag::KeepMissingSamples | Xml::Flag::Ipc, bSilent
	);
	return std::vector<unsigned char>(
		reinterpret_cast<const unsigned char*>( xml.constData() ),
		reinterpret_cast<const unsigned char*>( xml.constData() ) + xml.size()
	);
}

std::shared_ptr<Song> H2Project::fromState(
	const std::vector<unsigned char>& data, Hydrogen* pHydrogen, bool bSilent ) {
	if ( data.empty() ) {
		___ERRORLOG( "Empty plugin-state buffer" );
		return nullptr;
	}

	if ( looksLikeArchive( data ) ) {
		return fromBuffer( data, pluginCacheKey( pHydrogen ), pHydrogen,
						   bSilent );
	}

	// Song-only state: a plain .h2song XML document.
	const QByteArray xml(
		reinterpret_cast<const char*>( data.data() ),
		static_cast<int>( data.size() ) );
	return Song::fromXmlBuffer( xml, Xml::Flag::Ipc, bSilent, pHydrogen );
}

bool H2Project::looksLikeArchive( const std::vector<unsigned char>& data ) {
	// A `.h2song` is an XML document (starts with optional BOM/whitespace then
	// '<'). A `.h2project` is a gzip-compressed archive (magic 0x1f 0x8b) or, if
	// stored uncompressed, a tar/pax stream - never starts with '<'. Detect the
	// gzip magic, falling back to "not XML" for safety.
	if ( data.size() >= 2 && data[0] == 0x1f && data[1] == 0x8b ) {
		return true;
	}
	// Skip leading whitespace and check for an XML opening.
	for ( size_t i = 0; i < data.size(); ++i ) {
		const unsigned char c = data[i];
		if ( c == ' ' || c == '\t' || c == '\r' || c == '\n' ) {
			continue;
		}
		return c != '<';
	}
	return false;
}

};
