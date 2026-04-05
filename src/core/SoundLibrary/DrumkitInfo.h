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

#ifndef DRUMKIT_INFO_H
#define DRUMKIT_INFO_H

#include <memory>
#include <vector>

#include <core/Basics/Instrument.h>
#include <core/Object.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>

namespace H2Core
{

class Drumkit;

/**
 * @class DrumkitInfo
 *
 * Holds meta data about a particular song in the Sound Library.
 *
 * Drumkits are a special case in #SoundLibraryDatabase. We load the whole kit
 * into it for full insights, smooth switching, and other things not required
 * for either songs or patterns. This class is not the single source of truth
 * for the drumkit in the database but an abstraction used by #SoundLibraryPanel
 * to display the content in the GUI.
 *
 * \ingroup docCore docDataStructure */
class DrumkitInfo : public SoundLibraryInfo,
					public H2Core::Object<DrumkitInfo> {
	H2_OBJECT( DrumkitInfo )

   public:
	struct InstrumentInfo {
		const QString& sName;
		Instrument::Id id;
		Instrument::Type sType;
	};

	DrumkitInfo();
	~DrumkitInfo();

	static std::shared_ptr<DrumkitInfo> from( std::shared_ptr<Drumkit> pDrumkit
	);

	/** Retrieves the #Drumkit of @a sPath from #SoundLibraryDatabase and reads
	 * in its metadata.
	 *
	 * @return `true` on success
	 */
	bool load( const QString& sPath );

	const std::vector<InstrumentInfo>& getInstrumentInfos() const;

	QString toQString( const QString& sPrefix = "", bool bShort = true )
		const override;

   private:
	void assignFrom( std::shared_ptr<Drumkit> pDrumkit );
	std::vector<InstrumentInfo> m_instrumentInfos;
};
inline const std::vector<DrumkitInfo::InstrumentInfo>&
DrumkitInfo::getInstrumentInfos() const
{
	return m_instrumentInfos;
}
};	// namespace H2Core

#endif	// DRUMKIT_INFO_H
