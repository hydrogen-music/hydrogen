/*
 * Hydrogen
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

#ifndef H2C_DRUMKIT_MAPPING_H
#define H2C_DRUMKIT_MAPPING_H

#include <map>
#include <memory>
#include <set>
#include <QString>

#include <core/Basics/Instrument.h>
#include <core/Object.h>

namespace H2Core
{

class XMLDoc;
class XMLNode;

/**
 * DrumkitMap defines a 1:1 mapping of all #H2Core::Instrument of a
 * #H2Core::Drumkit to general type strings.
 *
 * By relating two maps using the general type as keys we can load another
 * #H2Core::Drumkit or import a #H2Core::Pattern without distorting the
 * pattern's content or any other loss of information.
*/
/** \ingroup docCore docDataStructure */
class DrumkitMap : public H2Core::Object<DrumkitMap>
{
	H2_OBJECT( DrumkitMap )
  public:
	DrumkitMap();
	DrumkitMap( std::shared_ptr<DrumkitMap> pOther );
	~DrumkitMap();

	/**
	 * Load #H2Core::DrumkitMap from an absolute path.
	 *
	 * \param sPath An absolute path pointing to a .h2map file.
	 * \param bSilent if set to true, all log messages except of
	 *   errors and warnings are suppressed.
	 *
	 * \return A #H2Core::DrumkitMap on success, an empty map otherwise.
	 */
	static std::shared_ptr<DrumkitMap> load( const QString& sPath,
											 bool bSilent = false );

	/**
	 * Load #H2Core::DrumkitMap from a XML node.
	 *
	 * \param node An XML node to load the mapping from, e.g. as part of a
	 * .h2song file
	 * \param bSilent if set to true, all log messages except of
	 *   errors and warnings are suppressed.
	 *
	 * \return A #H2Core::DrumkitMap on success, an empty map otherwise.
	 */
	static std::shared_ptr<DrumkitMap> loadFrom( const XMLNode& node,
												 bool bSilent = false );

		XMLDoc toXml( bool bSilent = false ) const;
	/**
	 * Save a #H2Core::DrumkitMap to disk (as a .h2map).
	 *
	 * \param sPath Absolute path. If the suffix .h2map was ommitted, it
	 * will be appended automatically.
	 * \param bSilent if set to true, all log messages except of
	 * errors and warnings are suppressed.
	 *
	 * \return true on success
	 */
	bool save( const QString& sPath, bool bSilent = false ) const;
	/**
	 * Save a #H2Core::DrumkitMap to an XML node.
	 *
	 * \param node XML node to write the mapping to.
	 * \param bSilent if set to true, all log messages except of
	 * errors and warnings are suppressed.
	 *
	 * \return true on success
	 */
	void saveTo( XMLNode& node, bool bSilent = false ) const;

	/**
	 * \param sType for which to get the corresponding id
	 * \param pOk set to `true` in case @a sType was found.
	 */
	Instrument::Id getId( const Instrument::Type& sType, bool* pOk ) const;
	/** Get type for @a id */
	Instrument::Type getType( Instrument::Id id ) const;
	/** Returns all unique types found #m_mapping */
	std::set<Instrument::Type> getAllTypes() const;

	bool addMapping( Instrument::Id id, const Instrument::Type& sType );

	/** Whether there are mappings present in the map */
	bool isEmpty() const;

	/** Remove all elements from map. */
	void clear();

	/** Number of elements in map */
	int size() const;

	/** Custom iterators */
	std::map<Instrument::Id, Instrument::Type>::iterator begin();
	std::map<Instrument::Id, Instrument::Type>::iterator end();

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
	/**
	 * Map instrument IDs to instrument Instrument::type strings (1:1).
	 *
	 * Instrument IDs are defined in each individual drumkit while the type
	 * strings are arbitrary strings using which instruments of different
	 * kits can be mapped onto each other.
	 */
	std::map<Instrument::Id, Instrument::Type> m_mapping;

		/** Used to indicate changes in the underlying XSD file. */
		static constexpr int nCurrentFormatVersion = 2;
};

inline void DrumkitMap::clear() {
	m_mapping.clear();
}
inline bool DrumkitMap::isEmpty() const {
	return m_mapping.empty();
}
inline int DrumkitMap::size() const {
	return m_mapping.size();
}
inline std::map<Instrument::Id, Instrument::Type>::iterator DrumkitMap::begin() {
	return m_mapping.begin();
}
inline std::map<Instrument::Id, Instrument::Type>::iterator DrumkitMap::end() {
	return m_mapping.end();
}
}; // namespace H2Core

#endif // H2C_DRUMKITMAPPING_H

/* vim: set softtabstop=4 noexpandtab: */
