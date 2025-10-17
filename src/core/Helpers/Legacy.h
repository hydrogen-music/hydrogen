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

#ifndef H2C_LEGACY_H
#define H2C_LEGACY_H

#include <core/Object.h>
#include <memory>
#include <core/License.h>

namespace H2Core {

class Drumkit;
class Playlist;
class Pattern;
class PatternList;
class InstrumentComponent;
class InstrumentList;
class License;
class XMLNode;

/**
 * Legacy is a container for legacy code which should be once removed
 */
/** \ingroup docCore*/
class Legacy : public H2Core::Object<Legacy> {
		H2_OBJECT(Legacy)
	public:

		/** Backward compatibility for kits created between 0.9.7 and 1.2.X.
		 * These included an element called `DrumkitComponent` holding some
		 * parameters now included in `InstrumentComponent`. This function
		 * searches for those parameters and assignes them to the components in
		 * @a pInstrumentList. */
		static void loadComponentNames( std::shared_ptr<InstrumentList> pInstrumentList,
										const XMLNode& rootNode,
										bool bSilent = false );

	/** backward compatibility for loading the current drumkit from songs saved
	 * prior to version 1.3.0.
	 *
	 * These did not store proper drumkits but raw instrument lists and
	 * components as well as some meta data. this is already about 90% of make a
	 * drumkit a drumkit but the pieces missing let to various inconsistencies
	 * and bugs.
	 *
	 * @param sSongPath If not empty, absolute path to the .h2song file the
	 *   drumkit is contained in. It is used to resolve sample paths relative to
	 *   the .h2song file.
	 * */
	static std::shared_ptr<Drumkit> loadEmbeddedSongDrumkit( const XMLNode& pRootNode,
															 const QString& sSongPath = "",
															 bool bSilent = false );

	/** Backward compatibility code to load an #InstrumentComponent
	 *	from an #Instrument which itself did not contain one yet.
	 *
	 * This code was used to load a #Song of version <= 0.9.0.
	 *
	 * \param pNode the XMLDode to read from
	 * \param sDrumkitPath the directory holding the drumkit data
	 * @param sSongPath If not empty, absolute path to the .h2song file the
	 *   instrument component is contained in. It is used to resolve sample
	 *   paths relative to the .h2song file.
	 * \param drumkitLicense License assigned to all #Sample
	 *   contain in the loaded #InstrumentLayer.
	 * \param bSilent if set to true, all log messages except of
	 *   errors and warnings are suppressed
	 */
	static std::shared_ptr<InstrumentComponent> loadInstrumentComponent( const XMLNode& pNode,
																		 const QString& sDrumkitPath,
																		 const QString& sSongPath = "",
																		 const License& drumkitLicense = License(),
																		 bool bSilent = false );
		/**
		 * load playlist from a file
		 * \param pl_path is a path to an xml file
		 * \return a Playlist on success, 0 otherwise
		 */
		static std::shared_ptr<Playlist> load_playlist( const QString& pl_path );

	static std::shared_ptr< std::vector< std::shared_ptr<PatternList> > > loadPatternGroupVector(
		const XMLNode& pNode,
		std::shared_ptr<PatternList> pPatternList,
		bool bSilent = false );

	/**
	 *	Check if filename was created with TinyXml or QtXml
	 *
	 * \return TinyXML: true, QtXml: false
	 */
	static bool checkTinyXMLCompatMode( QFile* pFile, bool bSilent = false );
	static QByteArray convertFromTinyXML( QFile* pFile, bool bSilent = false );

private:
	/** Convert (in-place) an XML escape sequence into a literal byte,
	 * rather than the character it actually refers to.
	 */
	static void convertStringFromTinyXML( QByteArray* pString );
};

};

#endif  // H2C_LEGACY_H

/* vim: set softtabstop=4 noexpandtab: */
