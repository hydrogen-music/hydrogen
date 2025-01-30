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

#ifndef H2C_XML_H
#define H2C_XML_H

#include <core/Object.h>
#include <QtCore/QString>
#include <QColor>
#include <QtXml/QDomDocument>

namespace H2Core
{

/**
 * XMLNode is a subclass of QDomNode with read and write values methods
*/
/** \ingroup docCore*/
class XMLNode : public H2Core::Object<XMLNode>, public QDomNode
{
		H2_OBJECT(XMLNode)
	public:
		/** basic constructor */
		XMLNode( );
		/** to wrap a QDomNode */
		XMLNode( const QDomNode& node );
		XMLNode( QDomNode&& node );

		/**
		 *  create a new XMLNode that has to be appended into de XMLDoc
		 * \param name the name of the node to create
		 * \return the newly created node
		 */
		XMLNode createNode( const QString& name );

		/**
		 * reads an integer stored into a child node
		 * \param node the name of the child node to read into
		 * \param default_value the value returned if something goes wrong
		 * \param inexistent_ok if set to false output a DEBUG log line if the node doesn't exists
		 * \param empty_ok if set to false output a DEBUG log line if the child node is empty
		 * \param bSilent Whether debug and info messages should be logged
		 * when anomalies are encountered while reading the XML nodes.
		 */
		int read_int( const QString& node, int default_value,
					  bool inexistent_ok = true, bool empty_ok = true,
					  bool bSilent = false ) const;
		/**
		 * reads a boolean stored into a child node
		 * \param node the name of the child node to read into
		 * \param default_value the value returned if something goes wrong
		 * \param inexistent_ok if set to false output a DEBUG log line if the node doesn't exists
		 * \param empty_ok if set to false output a DEBUG log line if the child node is empty
		 * \param bSilent Whether debug and info messages should be logged
		 * when anomalies are encountered while reading the XML nodes.
		 */
		bool read_bool( const QString& node, bool default_value,
						bool inexistent_ok = true, bool empty_ok = true,
						bool bSilent = false ) const;
		/**
		 * reads a boolean stored into a child node
		 * \param node the name of the child node to read into
		 * \param default_value the value returned if something goes wrong
		 * \param pFound Indicates whether the node was found.
		 * \param inexistent_ok if set to false output a DEBUG log line if the node doesn't exists
		 * \param empty_ok if set to false output a DEBUG log line if the child node is empty
		 * \param bSilent Whether debug and info messages should be logged
		 * when anomalies are encountered while reading the XML nodes.
		 */
	bool read_bool( const QString& node, bool default_value, bool* pFound,
					bool inexistent_ok = true, bool empty_ok = true,
					bool bSilent = false ) const;
		/**
		 * reads a float stored into a child node
		 * \param node the name of the child node to read into
		 * \param default_value the value returned if something goes wrong
		 * \param inexistent_ok if set to false output a DEBUG log line if the node doesn't exists
		 * \param empty_ok if set to false output a DEBUG log line if the child node is empty
		 * \param bSilent Whether debug and info messages should be logged
		 * when anomalies are encountered while reading the XML nodes.
		 */
		float read_float( const QString& node, float default_value,
						  bool inexistent_ok = true, bool empty_ok = true,
						  bool bSilent = false ) const;
		float read_float( const QString& node, float default_value, bool *pFound,
						  bool inexistent_ok = true, bool empty_ok = true,
						  bool bSilent = false ) const ;
		/**
		 * reads a string stored into a child node
		 * \param node the name of the child node to read into
		 * \param default_value the value returned if something goes wrong
		 * \param inexistent_ok if set to false output a DEBUG log line if the node doesn't exists
		 * \param empty_ok if set to false output a DEBUG log line if the child node is empty
		 * \param bSilent Whether debug and info messages should be logged
		 * when anomalies are encountered while reading the XML nodes.
		 */
		QString read_string( const QString& node, const QString& default_value,
							 bool inexistent_ok = true, bool empty_ok = true,
							 bool bSilent = false ) const;
		QColor read_color( const QString& node,
						   const QColor& defaultValue = QColor( 97, 167, 251 ),
						   bool inexistent_ok = true, bool empty_ok = true,
						   bool bSilent = false ) const;

		/**
		 * reads an attribute from the node
		 * \param attribute the name of the attribute to read
		 * \param default_value the value returned if something goes wrong
		 * \param inexistent_ok if set to false output a DEBUG log line if the attribute doesn't exists
		 * \param empty_ok if set to false output a DEBUG log line if the attribute is empty
		 * \param bSilent Whether debug and info messages should be logged
		 * when anomalies are encountered while reading the XML nodes.
		 */
		QString read_attribute( const QString& attribute,
								const QString& default_value, bool inexistent_ok,
								bool empty_ok, bool bSilent = false ) const;

		/**
		 * reads the text (content) from the node
		 * \param empty_ok if set to false output a DEBUG log line if the node is empty
		 * \param bSilent Whether debug and info messages should be logged
		 * when anomalies are encountered while reading the XML nodes.
		 */
		QString read_text( bool empty_ok, bool bSilent = false ) const;

		/**
		 * write an integer into a child node
		 * \param node the name of the child node to create
		 * \param value the value to write
		 */
		void write_int( const QString& node, const int value );
		/**
		 * write a boolean into a child node
		 * \param node the name of the child node to create
		 * \param value the value to write
		 */
		void write_bool( const QString& node, const bool value );
		/**
		 * write a float into a child node
		 * \param node the name of the child node to create
		 * \param value the value to write
		 */
		void write_float( const QString& node, const float value );
		/**
		 * write a string into a child node
		 * \param node the name of the child node to create
		 * \param value the value to write
		 */
		void write_string( const QString& node, const QString& value );
	void write_color( const QString& node, const QColor& color );

		/**
		 * write a string as an attribute of the node
		 * \param attribute the name of the attribute to create
		 * \param value the value to write in the attribute
		 */
		void write_attribute( const QString& attribute, const QString& value );
	private:
		/**
		 * reads a string stored into a child node
		 * \param node the name of the child node to read into
		 * \param inexistent_ok if set to false output a DEBUG log line if the node doesn't exists
		 * \param empty_ok if set to false output a DEBUG log line if the child node is empty
		 * \param bSilent Whether debug and info messages should be logged
		 * when anomalies are encountered while reading the XML nodes.
		 */
	QString read_child_node( const QString& node, bool inexistent_ok,
							 bool empty_ok, bool bSilent = false ) const;
		/**
		 * write a string into a child node
		 * \param node the name of the child node to create
		 * \param text the text to write
		 */
		void write_child_node( const QString& node, const QString& text );
};

/**
 * XMLDoc is a subclass of QDomDocument with read and write methods
*/
/** \ingroup docCore*/
class XMLDoc : public H2Core::Object<XMLDoc>, public QDomDocument
{
		H2_OBJECT(XMLDoc)
	public:
		/** basic constructor */
		XMLDoc( );
		XMLDoc( const QString& sSerialized );
		/**
		 * read the content of an xml file
		 * \param filepath the path to the file to read from
		 * \param schemapath the path to the XML Schema file
		 * \param bSilent Whether debug and info messages should be logged
		 * when anomalies are encountered while reading the XML nodes.
		 */
	bool read( const QString& filepath, const QString& schemapath = nullptr,
			   bool bSilent = false );
		/**
		 * write itself into a file
		 * \param filepath the path to the file to write to
		 */
		bool write( const QString& filepath );
		/**
		 * create the xml header and root node
		 * \param node_name the name of the rootnode to build
		 * \param xmlns the xml namespace prefix to add after XMLNS_BASE
		 */
		XMLNode set_root( const QString& node_name, const QString& xmlns = nullptr );
};

};

#endif  // H2C_XML_H

/* vim: set softtabstop=4 noexpandtab: */
