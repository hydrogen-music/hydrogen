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


#ifndef PATCH_H
#define PATCH_H

#include <vector>
#include <memory>

#include <QString>

#include <core/Object.h>
#include <core/Basics/Note.h>

/** Custom container encapsulating all changes done in the #PatchBay.
 *
 * \ingroup docGUI*/
class Patch : public H2Core::Object<Patch> {
		H2_OBJECT(Patch)

public:
		struct Mapping {
			QString sOldPatternType;
			int nNewInstrumentId;
			std::vector< std::shared_ptr<H2Core::Note> > affectedNotes;

			/** Formatted string version for debugging purposes.
			 * \param sPrefix String prefix which will be added in front of
			 * every new line
			 * \param bShort Instead of the whole content of all classes
			 * stored as members just a single unique identifier will be
			 * displayed without line breaks.
			 *
			 * \return String presentation of current object.*/
			QString toQString( const QString& sPrefix = "",
							   bool bShort = true ) const;
		};

		Patch();
		~Patch();

		void addMapping( const QString& sOldPatternType, int nNewInstrumentId,
						 std::vector< std::shared_ptr<H2Core::Note> > affectedNotes );
		const std::vector<Mapping> getMappings() const {
			return m_mappings;
		}

		/** Formatted string version for debugging purposes.
		 * \param sPrefix String prefix which will be added in front of
		 * every new line
		 * \param bShort Instead of the whole content of all classes
		 * stored as members just a single unique identifier will be
		 * displayed without line breaks.
		 *
		 * \return String presentation of current object.*/
		QString toQString( const QString& sPrefix = "",
						   bool bShort = true ) const override;

private:
		std::vector<Mapping> m_mappings;
};

#endif
