/*
 * Hydrogen
 * Copyright(c) 2002-2007 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#ifndef LFILEMNG_H
#define LFILEMNG_H

#include <iostream>
#include <fstream>
#include <vector>
using std::vector;
#include <string>
using std::string;

#include <hydrogen/Object.h>

class TiXmlNode;

namespace H2Core
{

class Note;
class Instrument;
class InstrumentList;
class Sequence;
class Pattern;
class Song;
class Drumkit;

/**
 *
 */
class LocalFileMng : public Object
{
public:
	LocalFileMng();
	~LocalFileMng();

	vector<string> getUserDrumkitList();
	vector<string> getSystemDrumkitList();
	string getDrumkitDirectory( const std::string& drumkitName );

	Drumkit* loadDrumkit( const std::string& directory );
	int saveDrumkit( Drumkit *pDrumkit );
	//void installDrumkit( const std::string& filename );
	//int uninstallDrumkit( const std::string& drumkitName );

	static void writeXmlString( ::TiXmlNode *parent, const std::string& name, const std::string& text );
	static void writeXmlBool( ::TiXmlNode *parent, const std::string& name, bool value );

	static string readXmlString( ::TiXmlNode* parent, const std::string& nodeName, const std::string& defaultValue, bool bCanBeEmpty = false, bool bShouldExists = true );
	static int readXmlInt( ::TiXmlNode* parent, const std::string& nodeName, int defaultValue, bool bCanBeEmpty = false, bool bShouldExists = true );
	static float readXmlFloat( ::TiXmlNode* parent, const std::string& nodeName, float defaultValue, bool bCanBeEmpty = false, bool bShouldExists = true );
	static bool readXmlBool(  ::TiXmlNode* parent, const std::string& nodeName, bool defaultValue, bool bShouldExists = true );


private:
	void fileCopy( const std::string& sOrigFilename, const std::string& sDestFilename );
};





//-----------------------------------------------------------------------------




//-----------------------------------------------------------------------------
/**
 * Write XML file of a song
 */
class SongWriter : public Object
{
public:
	SongWriter();
	~SongWriter();

	void writeSong( Song *song, const std::string& filename );
};

};


#endif //LFILEMNG_H

