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
	static Drumkit* load( const std::string& sFilename );

	/// Lists the User drumkit list
	static std::vector<std::string> getUserDrumkitList();

	/// Lists the System drumkit list
	static std::vector<std::string> getSystemDrumkitList();

	/// Installs a drumkit
	static void install( const std::string& filename );

	// Save a drumkit
	static void save( const std::string& sName, const std::string& sAuthor, const std::string& sInfo );


	/// Remove a Drumkit from the disk
	static void removeDrumkit( const std::string& sDrumkitName );

	InstrumentList *getInstrumentList()
	{
		return m_pInstrumentList;
	}
	void setInstrumentList( InstrumentList* instr )
	{
		this->m_pInstrumentList = instr;
	}

	void setName( const std::string& name )
	{
		this->m_sName = name;
	}
	const std::string& getName()
	{
		return m_sName;
	}

	void setAuthor( const std::string& author )
	{
		this->m_sAuthor = author;
	}
	const std::string& getAuthor()
	{
		return m_sAuthor;
	}

	void setInfo( const std::string& info )
	{
		this->m_sInfo = info;
	}
	const std::string& getInfo()
	{
		return m_sInfo;
	}

	void dump();

private:
	InstrumentList *m_pInstrumentList;
	std::string m_sName;
	std::string m_sAuthor;
	std::string m_sInfo;
};

};

#endif
