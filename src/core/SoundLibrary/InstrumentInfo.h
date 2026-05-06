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

#ifndef INSTRUMENT_INFO_H
#define INSTRUMENT_INFO_H

#include <core/Basics/Instrument.h>
#include <core/Object.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>

namespace H2Core {

class DrumkitInfo;

/**
 * @class InstrumentInfo
 *
 * Holds meta data about a particular instrument within a drumkit. Instances of
 * this class will be contained in #DrumkitInfo instead of living in
 * #SoundLibraryDatabase itself.
 *
 * The path #m_sPath serves as the unique identifier of the drumkit the
 * instrument is part of. Thus, _all_ instruments of a drumkit will share the
 * same sPath.
 *
 * \ingroup docCore docDataStructure */
class InstrumentInfo : public SoundLibraryInfo,
					   public H2Core::Object<InstrumentInfo> {
	H2_OBJECT( InstrumentInfo )
   public:
	InstrumentInfo(
		DrumkitInfo* pInfo,
		const QString& sName,
		Instrument::Id id,
		Instrument::Type sType
	);
	~InstrumentInfo();

	Instrument::Id getId() const;
	Instrument::Type getType() const;

	QString toQString( const QString& sPrefix = "", bool bShort = true )
		const override;

   private:
	Instrument::Id m_id;
	Instrument::Type m_sType;
};
inline Instrument::Id InstrumentInfo::getId() const
{
	return m_id;
}
inline Instrument::Type InstrumentInfo::getType() const
{
	return m_sType;
}
};	// namespace H2Core

#endif	// INSTRUMENT_INFO_H
