/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef LFILEMNG_H
#define LFILEMNG_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <core/Object.h>

#include <QColor>
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

/** \ingroup docCore*/
class LocalFileMng : public H2Core::Object<LocalFileMng>
{
	H2_OBJECT(LocalFileMng)
public:
	LocalFileMng();
	~LocalFileMng();

	QString getDrumkitNameForPattern( const QString& patternDir );

	static void writeXmlString( QDomNode parent, const QString& name, const QString& text );
	static void writeXmlColor( QDomNode parent, const QString& name, const QColor& color );
	static void writeXmlBool( QDomNode parent, const QString& name, bool value );

	static QString	readXmlString( QDomNode , const QString& nodeName, const QString& defaultValue, bool bCanBeEmpty = false, bool bShouldExists = true , bool tinyXmlCompatMode = false);
	static QColor	readXmlColor( QDomNode , const QString& nodeName, const QColor& defaultValue = QColor( 97, 167, 251 ), bool bCanBeEmpty = false, bool bShouldExists = true , bool tinyXmlCompatMode = false);
	static float	readXmlFloat( QDomNode , const QString& nodeName, float defaultValue, bool bCanBeEmpty = false, bool bShouldExists = true , bool tinyXmlCompatMode = false);
	static float	readXmlFloat( QDomNode, const QString& nodeName, float defaultValue, bool *pFound,
								 bool bCanBeEmpty = false, bool bShouldExists = true, bool tinyXmlCompatMode = false );	
	static int		readXmlInt( QDomNode , const QString& nodeName, int defaultValue, bool bCanBeEmpty = false, bool bShouldExists = true , bool tinyXmlCompatMode = false);
	static bool		readXmlBool( QDomNode,
								 const QString& nodeName,
								 bool defaultValue,
								 bool *pFound,
								 bool bShouldExists = true,
								 bool tinyXmlCompatMode = false );
	static bool		readXmlBool( QDomNode , const QString& nodeName, bool defaultValue, bool bShouldExists = true , bool tinyXmlCompatMode = false );
	static void		convertFromTinyXMLString( QByteArray* str );
	static bool		checkTinyXMLCompatMode( const QString& filename );
	static QDomDocument openXmlDocument( const QString& filename );
	
private:
	static QString processNode( QDomNode node, const QString& nodeName, bool bCanBeEmpty, bool bShouldExists );
};

/** \ingroup docCore*/
class SongWriter : public H2Core::Object<SongWriter>
{
	H2_OBJECT(SongWriter)
public:
	SongWriter();
	~SongWriter();

	// Returns 0 on success.
	int writeSong( std::shared_ptr<Song> song, const QString& filename );
};

};


#endif //LFILEMNG_H

