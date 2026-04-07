/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef SOUND_LIBRARY_INFO_H
#define SOUND_LIBRARY_INFO_H

#include <vector>

#include <core/Helpers/Filesystem.h>
#include <core/License.h>
#include <core/Object.h>

namespace H2Core
{

/**
 * @class SoundLibraryInfo
 *
 * Holds meta data about a particular artifact in the Sound Library.
 *
 * The path #m_sPath serves as the unique identifier for each info item.
 *
 * @author Sebastian Moors
 *
 * \ingroup docCore docDataStructure */
class SoundLibraryInfo : public H2Core::Object<SoundLibraryInfo> {
	H2_OBJECT( SoundLibraryInfo )
   public:
	SoundLibraryInfo();
	SoundLibraryInfo(
		const QString& sName,
		const QString& sURL,
		const QString& sInfo,
		const QString& sAuthor,
		Filesystem::Artifact artifact,
		const License& license,
		const QString& sPath
	);
	~SoundLibraryInfo();

	/**
	 * Reads the content found in @a sPath.
	 *
	 * @param sPath Path to .h2pattern XML file
	 * @return `true` on success
	 */
	bool load( const QString& sPath );

	const QString& getName() const { return m_sName; }
	const QString& getUrl() const { return m_sURL; }
	const QString& getInfo() const { return m_sInfo; }
	const QString& getAuthor() const { return m_sAuthor; }
	Filesystem::Artifact getArtifact() const { return m_artifact; }
	const H2Core::License& getLicense() const { return m_license; }
	const QString& getPath() const { return m_sPath; }
	Filesystem::Context getContext() const { return m_context; }
	const QString& getLabel() const { return m_sLabel; }
	void setLabel( const QString& sLabel ) { m_sLabel = sLabel; }

	/** Formatted string version for debugging purposes.
	 * \param sPrefix String prefix which will be added in front of
	 * every new line
	 * \param bShort Instead of the whole content of all classes
	 * stored as members just a single unique identifier will be
	 * displayed without line breaks.
	 *
	 * \return String presentation of current object.*/
	QString toQString( const QString& sPrefix = "", bool bShort = true )
		const override;

   protected:
	QString m_sName;
	QString m_sURL;
	QString m_sInfo;
	QString m_sAuthor;
	Filesystem::Artifact m_artifact;
	H2Core::License m_license;
	/** Absolute path to locate the resource. This will also be used as
	 * unique identifier for the artifact. */
	QString m_sPath;
	/** Unique label of an artifact within a given context within the Sound
	 * Library. In case there are multiple items bearing the same name, the
	 * first one registered will keep it while all further ones be suffixed by a
	 * number in braces. */
	QString m_sLabel;

	Filesystem::Context m_context;
};
};	// namespace H2Core

#endif	// SOUND_LIBRARY_INFO_H
