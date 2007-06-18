/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: LocalFileMng.h,v 1.8 2005/05/09 18:12:24 comix Exp $
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

#include "Object.h"

class TiXmlNode;
class Note;
class Instrument;
class InstrumentList;
class Sequence;
class Pattern;
class Song;
class DrumkitInfo;

//----------------------------------------------------------------------------
/**
 * Class for load and save local files.
 */
class LocalFileMng : public Object{
	public:
		/** Costruttore. */
		LocalFileMng();

		/** Distruttore. */
		~LocalFileMng();

		vector<string> listUserDrumkits();
		vector<string> listSystemDrumkits();
		string getDrumkitDirectory( string drumkitName );

		DrumkitInfo* loadDrumkit( string directory );
		int saveDrumkit( DrumkitInfo *info );
		void installDrumkit( string filename );
		int uninstallDrumkit( string drumkitName );

		static void writeXmlString( ::TiXmlNode *parent, string name, string text );
		static void writeXmlBool( ::TiXmlNode *parent, string name, bool value );

		static string readXmlString( Object* obj, ::TiXmlNode* parent, string nodeName, string defaultValue, bool bCanBeEmpty = false );
		static int readXmlInt( Object* obj, ::TiXmlNode* parent, string nodeName, int defaultValue, bool bCanBeEmpty = false, bool bShouldExists = true );
		static float readXmlFloat( Object* obj, ::TiXmlNode* parent, string nodeName, float defaultValue, bool bCanBeEmpty = false, bool bShouldExists = true );
		static bool readXmlBool(  Object* obj, ::TiXmlNode* parent, string nodeName, bool defaultValue, bool bShouldExists = true );

		/** Loads a song from disk */
		Song* loadSong(string filename);

		/** Save the song */
		void saveSong(Song *song, string filename);

	private:
		void fileCopy( const string& sOrigFilename, const string& sDestFilename );
};





//-----------------------------------------------------------------------------
/**
 * Read XML file of a song
 */
class SongReader : public Object {
	public:
		SongReader();
		~SongReader();
		Song* readSong(string filename);

	private:
		string m_sSongVersion;

		/// Dato un XmlNode restituisce un oggetto Pattern
		Pattern* getPattern(::TiXmlNode* pattern, InstrumentList* instrList);

		/// Dato un XmlNode restituisce un oggetto sequence
		Sequence* getSequence(::TiXmlNode* sequence, InstrumentList* instrList, bool bConvertFrom080);

		/// Dato un XmlNode restituisce un oggetto note
		Note* getNote(::TiXmlNode* node, InstrumentList *instrList);
};




//-----------------------------------------------------------------------------
/**
 * Write XML file of a song
 */
class SongWriter : public Object {
	public:
		/** Constructor */
		SongWriter();

		/** Destructor */
		~SongWriter();

		void writeSong(Song *song, string filename);
};




#endif //LFILEMNG_H

