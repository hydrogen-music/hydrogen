/*
 * Copyright(c) 2010 by Zurcher Jérémy
 *
 * Hydrogen
 * Copyright(c) 2002-200/ Alessandro Cominu
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef H2_XML_HELPER_H
#define H2_XML_HELPER_H

#include <hydrogen/Object.h>
#include <QtCore/QString>
#include <QtXml/QDomDocument>

namespace H2Core
{

class XMLNode : public Object, public QDomNode
{
    H2_OBJECT
    public:
        XMLNode( );
        XMLNode( QDomNode node );

        int read_int( const QString& node, int default_value, bool inexistent_ok=true, bool empty_ok=true );
        bool read_bool( const QString& node, bool default_value, bool inexistent_ok=true, bool empty_ok=true );
        float read_float( const QString& node, float default_value, bool inexistent_ok=true, bool empty_ok=true );
        QString read_string( const QString& node, const QString& default_value, bool inexistent_ok=true, bool empty_ok=true );
        
        void write_int( const QString& node, const int value );
        void write_bool( const QString& node, const bool value );
        void write_float( const QString& node, const float value );
        void write_string( const QString& node, const QString& value );
    private:
        QString read_child_node( const QString& node, bool inexistent_ok, bool empty_ok );
        void write_child_node( const QString& node, const QString& text );
};

class XMLDoc : public Object, public QDomDocument
{
    H2_OBJECT
    public:
        XMLDoc( );
        bool read( const QString& filename );
        bool write( const QString& filename );
};

};

#endif  // H2_XML_HELPER_H

/* vim: set softtabstop=4 expandtab: */
