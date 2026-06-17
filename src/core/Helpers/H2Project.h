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
 * Both the build and the reconstruction work entirely in memory: samples are
 * decoded straight from the archive via Sample::loadFromMemory() (libsndfile
 * virtual I/O), so a project opens even when its drumkit is not installed.
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

	/** Reconstruct a song (with its samples decoded from the bundle) from an
	 * in-memory `.h2project` buffer. Returns nullptr on failure. */
	static std::shared_ptr<Song> fromBuffer(
		const std::vector<unsigned char>& data, Hydrogen* pHydrogen,
		bool bSilent = false );

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
