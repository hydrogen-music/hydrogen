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
	/** Backward compatibility code to load an #InstrumentComponent
	 *	from an #Instrument which itself did not contain one yet.
	 *
	 * This code was used to load a #Song of version <= 0.9.0.
	 */
	static std::shared_ptr<InstrumentComponent> loadInstrumentComponent(
		XMLNode* pNode,
		const QString& sDrumkitPath,
		const QString& sSongPath,
		const License& drumkitLicense,
		bool bSilent = false );
		/**
		 * load playlist from a file
		 * \param pl the playlist to feed
		 * \param pl_path is a path to an xml file
		 * \return a Playlist on success, 0 otherwise
		 */
		static Playlist* load_playlist( Playlist* pl, const QString& pl_path );

	static std::vector<PatternList*>* loadPatternGroupVector( XMLNode* pNode, PatternList* pPatternList, bool bSilent = false );

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
