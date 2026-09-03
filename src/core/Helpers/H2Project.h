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

#ifndef H2C_H2PROJECT_H
#define H2C_H2PROJECT_H

#include <core/Object.h>

#include <QString>

#include <memory>
#include <vector>

namespace H2Core {

class Song;
class Hydrogen;

/**
 * Codec for the self-contained `.h2project` bundle (ADR 0025).
 *
 * A `.h2project` is a libarchive bundle holding the song XML plus the kit's
 * actual sample audio (content-hash deduped) - unlike `.h2song`, which only
 * references samples by path. The very same codec backs:
 *   - the standalone "save as self-contained project" menu action, and
 *   - the plugin's embedded state (ADR 0017/0020) when sample embedding is on.
 *
 * Building a bundle happens entirely in memory. For reconstruction the
 * bundle is extracted into a per-origin folder below Filesystem::cacheDir()
 * - keyed by the originating `.h2project` file or, for plugin states, by
 * host process and Hydrogen instance. The extracted sample files are
 * assigned to the song via Sample::setFilePath() and decoded through the
 * regular file-based code path, so a project opens even when its drumkit
 * is not installed. The extraction folders are tracked by the owning
 * Hydrogen instance (Hydrogen::registerExtractedProjectDir()) and removed
 * again on its teardown.
 *
 * \ingroup docCore
 */
class H2Project : public H2Core::Object<H2Project> {
	H2_OBJECT(H2Project)
public:
	/** Build a `.h2project` bundle (in memory) from @a pSong and its kit's
	 * samples. Returns an empty vector on failure. */
	static std::vector<unsigned char> toBuffer( std::shared_ptr<Song> pSong,
												bool bSilent = false );

	/** Reconstruct a song from an in-memory `.h2project` buffer. The bundle
	 * is extracted to a cache folder derived from @a sCacheKey and its
	 * samples are loaded from the extracted files. Returns nullptr on
	 * failure.
	 *
	 * \param sCacheKey Key identifying the origin of the bundle, e.g. the
	 *   path of the `.h2project` file or a plugin-state identifier. It
	 *   determines the extraction folder below Filesystem::cacheDir() and
	 *   has to be stable across reloads of the same origin. */
	static std::shared_ptr<Song> fromBuffer(
		const std::vector<unsigned char>& data, const QString& sCacheKey,
		Hydrogen* pHydrogen, bool bSilent = false );

	/** Write a `.h2project` bundle for @a pSong to @a sPath. */
	static bool save( std::shared_ptr<Song> pSong, const QString& sPath,
					  bool bSilent = false );

	/** Load a song from a `.h2project` file on disk. */
	static std::shared_ptr<Song> load( const QString& sPath,
										Hydrogen* pHydrogen, bool bSilent = false );

	/** Unified open (T4b.3): load a song from a file that is either a `.h2song`
	 * (XML) or a `.h2project` (archive), detected by container. Callers and
	 * hosts never pick a loader. */
	static std::shared_ptr<Song> openSong( const QString& sPath,
											Hydrogen* pHydrogen,
											bool bSilent = false );

	/** Serialize a song to a plugin-state buffer (ADR 0017/0020/0025/T4b.5).
	 * @param bEmbedSamples ON → a portable `.h2project` bundle (samples
	 *   embedded); OFF → song-only state (the `.h2song` XML; the kit must be
	 *   installed to reload). */
	static std::vector<unsigned char> toState( std::shared_ptr<Song> pSong,
												bool bEmbedSamples,
												bool bSilent = false );

	/** Reconstruct a song from a plugin-state buffer, accepting both embedded
	 * (`.h2project`) and song-only states (detected by container). */
	static std::shared_ptr<Song> fromState(
		const std::vector<unsigned char>& data, Hydrogen* pHydrogen,
		bool bSilent = false );

	/** Heuristic: does this buffer look like a `.h2project` archive (vs a plain
	 * `.h2song` XML document)? Used by the unified open path (T4b.3). */
	static bool looksLikeArchive( const std::vector<unsigned char>& data );
};

};

#endif
