/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef SOUNDLIBRARY_H
#define SOUNDLIBRARY_H

#include <string>
#include <hydrogen/Object.h>

namespace H2Core
{

class InstrumentList;


/**
\ingroup H2CORE
\brief	SoundLibrary class.
*/
class SoundLibrary : public Object
{
public:
	SoundLibrary();
	~SoundLibrary();

private:
};



/**
\ingroup H2CORE
\brief	Drumkit info
*/
class Drumkit : public Object
{
public:
	Drumkit();
	~Drumkit();

	/// Loads a single Drumkit
	static Drumkit* load( const QString& sFilename );

	/// Lists the User drumkit list
	static std::vector<QString> getUserDrumkitList();

	/// Lists the System drumkit list
	static std::vector<QString> getSystemDrumkitList();

	/// Installs a drumkit
	static void install( const QString& filename );

	// Save a drumkit
	static void save( const QString& sName, const QString& sAuthor, const QString& sInfo, const QString& sLicense );


	/// Remove a Drumkit from the disk
	static void removeDrumkit( const QString& sDrumkitName );

	InstrumentList *getInstrumentList() {
		return m_pInstrumentList;
	}
	void setInstrumentList( InstrumentList* instr ) {
		this->m_pInstrumentList = instr;
	}

	void setName( const QString& name ) {
		this->m_sName = name;
	}
	const QString& getName() {
		return m_sName;
	}

	void setAuthor( const QString& author ) {
		this->m_sAuthor = author;
	}
	const QString& getAuthor() {
		return m_sAuthor;
	}

	void setInfo( const QString& info ) {
		this->m_sInfo = info;
	}
	const QString& getInfo() {
		return m_sInfo;
	}

	void setLicense( const QString& license ) {
		this->m_sLicense = license;
	}
	const QString& getLicense() {
		return m_sLicense;
	}

	void dump();

private:
	InstrumentList *m_pInstrumentList;
	QString m_sName;
	QString m_sAuthor;
	QString m_sInfo;
	QString m_sLicense;
};

};

#endif
