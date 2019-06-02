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
 * \ingroup docCore
 */
class LocalFileMng : public H2Core::Object
{
public:
	/** \return #m_sClassName*/
	static const char* className() { return m_sClassName; }
	LocalFileMng();
	~LocalFileMng();

	QString getDrumkitNameForPattern( const QString& patternDir );

	static void writeXmlString( QDomNode parent, const QString& name, const QString& text );
	static void writeXmlBool( QDomNode parent, const QString& name, bool value );

	static QString	readXmlString( QDomNode , const QString& nodeName, const QString& defaultValue, bool bCanBeEmpty = false, bool bShouldExists = true , bool tinyXmlCompatMode = false);
	static float	readXmlFloat( QDomNode , const QString& nodeName, float defaultValue, bool bCanBeEmpty = false, bool bShouldExists = true , bool tinyXmlCompatMode = false);
	static int		readXmlInt( QDomNode , const QString& nodeName, int defaultValue, bool bCanBeEmpty = false, bool bShouldExists = true , bool tinyXmlCompatMode = false);
	static bool		readXmlBool( QDomNode , const QString& nodeName, bool defaultValue, bool bShouldExists = true , bool tinyXmlCompatMode = false );
	static void		convertFromTinyXMLString( QByteArray* str );
	static bool		checkTinyXMLCompatMode( const QString& filename );
	static QDomDocument openXmlDocument( const QString& filename );

private:
	/** Contains the name of the class.
	 *
	 * This variable allows from more informative log messages
	 * with the name of the class the message is generated in
	 * being displayed as well. Queried using className().*/
	static const char* m_sClassName;
	static QString processNode( QDomNode node, const QString& nodeName, bool bCanBeEmpty, bool bShouldExists );
};



/**
 * Write XML file of a song
 *
 * \ingroup docCore
 */
class SongWriter : public H2Core::Object
{
public:
	/** \return #m_sClassName*/
	static const char* className() { return m_sClassName; }
	SongWriter();
	~SongWriter();

	// Returns 0 on success.
	int writeSong( Song *song, const QString& filename );
private:
	/** Contains the name of the class.
	 *
	 * This variable allows from more informative log messages
	 * with the name of the class the message is generated in
	 * being displayed as well. Queried using className().*/
	static const char* m_sClassName;
};

};


#endif //LFILEMNG_H

