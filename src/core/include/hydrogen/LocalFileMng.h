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

#ifndef LFILEMNG_H
#define LFILEMNG_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <hydrogen/object.h>

#include <QDomDocument>


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
class LocalFileMng : public H2Core::Object
{
	H2_OBJECT
public:
	LocalFileMng();
	~LocalFileMng();

	static std::vector<QString> getDrumkitsFromDirectory( QString );
	std::vector<QString> getPatternDirList();
	std::vector<QString> getSongList();
	std::vector<QString> getPatternsForDrumkit( const QString&  );
	std::vector<QString> getAllPatternNames();
	int getPatternList( const QString& );
	int mergeAllPatternList( std::vector<QString> );

	std::vector<QString> getallPatternList(){
		return m_allPatternList;
	}
	std::vector<QString> getAllCategoriesFromPattern();

	QString getDrumkitNameForPattern( const QString& patternDir );

	static void writeXmlString( QDomNode parent, const QString& name, const QString& text );
	static void writeXmlBool( QDomNode parent, const QString& name, bool value );

	Pattern* loadPattern( const QString& directory );
	int savePattern( Song *song , const QString& drumkit_name, int selectedpattern , const QString& patternname, const QString& realpatternname, int mode);

	int savePlayList( const std::string& patternname );
	int loadPlayList( const std::string& patternname);
	
	static QString copyInstrumentLineToString(Song *song, int selectedPattern, int selectedInstrument);
	static bool pasteInstrumentLineFromString(Song *song, const QString & serialized, int selectedPattern, int selectedInstrument, std::list< Pattern* > & patterns);

	int writeTempPatternList( Song *song, const QString& filename);//used for undo/redo

	static QString readXmlString( QDomNode , const QString& nodeName, const QString& defaultValue, bool bCanBeEmpty = false, bool bShouldExists = true , bool tinyXmlCompatMode = false);
	static float readXmlFloat( QDomNode , const QString& nodeName, float defaultValue, bool bCanBeEmpty = false, bool bShouldExists = true , bool tinyXmlCompatMode = false);
	static int readXmlInt( QDomNode , const QString& nodeName, int defaultValue, bool bCanBeEmpty = false, bool bShouldExists = true , bool tinyXmlCompatMode = false);
	static bool readXmlBool( QDomNode , const QString& nodeName, bool defaultValue, bool bShouldExists = true , bool tinyXmlCompatMode = false );
	static void convertFromTinyXMLString( QByteArray* str );
	static bool checkTinyXMLCompatMode( const QString& filename );
	static QDomDocument openXmlDocument( const QString& filename );

private:
	void fileCopy( const QString& sOrigFilename, const QString& sDestFilename );
	std::vector<QString> m_allPatternList;
};



/**
 * Write XML file of a song
 */
class SongWriter : public H2Core::Object
{
	H2_OBJECT
public:
	SongWriter();
	~SongWriter();

	// Returns 0 on success.
	int writeSong( Song *song, const QString& filename );
};

};


#endif //LFILEMNG_H

