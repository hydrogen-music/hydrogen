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

#ifndef H2C_ARCHIVE_H
#define H2C_ARCHIVE_H

#include <core/Object.h>

#include <QString>
#include <QStringList>

#include <vector>

namespace H2Core {

/**
 * Single extraction back end for all tar-based artifacts of Hydrogen -
 * `.h2drumkit` files as well as `.h2project` bundles - built on
 * `libarchive`.
 *
 * The public interface is free of `libarchive` types (only the implementation
 * depends on the library) so consumers require neither its headers nor its link
 * flags.
 *
 * \ingroup docCore
 */
class Archive : public H2Core::Object<Archive> {
	H2_OBJECT(Archive)

public:
	/**
	 * Extract all entries of the tar archive at @a sArchivePath into the
	 * folder @a sTargetDir.
	 *
	 * \param sArchivePath Absolute path to an existing tar archive.
	 * \param sTargetDir Absolute path of the folder to extract the archive
	 * into. It will be created if not present. Already existing content
	 * will be overwritten.
	 * \param bSilent Whether info messages should be logged.
	 * \param pExtractedPaths If not null, it will contain the absolute
	 * paths of all extracted entries (in archive order). This allows
	 * callers to locate specific artifacts without scanning @a sTargetDir -
	 * which might hold unrelated content, e.g. other drumkits.
	 * \param pEncodingIssuesDetected Will be set to true in case the system
	 * does not support UTF-8 and at least one entry path had to be stripped
	 * of problematic characters in order to be writable.
	 *
	 * \return true on success
	 */
	static bool extract( const QString& sArchivePath, const QString& sTargetDir,
						 bool bSilent = false,
						 QStringList* pExtractedPaths = nullptr,
						 bool* pEncodingIssuesDetected = nullptr );

	/**
	 * Same as extract() but for an archive held in memory, e.g. a plugin
	 * state or a `.h2project` buffer.
	 *
	 * \param data In-memory archive. It only has to remain valid for the
	 * duration of the call.
	 * \param sTargetDir Absolute path of the folder to extract the archive
	 * into. It will be created if not present. Already existing content
	 * will be overwritten.
	 * \param bSilent Whether info messages should be logged.
	 * \param pExtractedPaths If not null, it will contain the absolute
	 * paths of all extracted entries (in archive order).
	 * \param pEncodingIssuesDetected Will be set to true in case the system
	 * does not support UTF-8 and at least one entry path had to be stripped
	 * of problematic characters in order to be writable.
	 *
	 * \return true on success
	 */
	static bool extractFromBuffer(
		const std::vector<unsigned char>& data,
		const QString& sTargetDir,
		bool bSilent = false,
		QStringList* pExtractedPaths = nullptr,
		bool* pEncodingIssuesDetected = nullptr
	);
};

};

#endif
