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

#include <core/Helpers/Xml.h>
#include <core/Helpers/Legacy.h>

#include <QtCore/QFile>
#include <QtCore/QLocale>
#include <QtCore/QString>
#include <QtCore/QTextStream>

#define XMLNS_BASE "http://www.hydrogen-music.org/"
#define XMLNS_XSI "http://www.w3.org/2001/XMLSchema-instance"

namespace H2Core
{

XMLNode::XMLNode() { }
XMLNode::XMLNode( const QDomNode& node ) : QDomNode( node ) { }
XMLNode::XMLNode( QDomNode&& node ) : QDomNode( node ) { }

XMLNode XMLNode::createNode( const QString& name )
{
	XMLNode node = this->ownerDocument().createElement( name );
	appendChild( node );
	return node;
}

QString XMLNode::read_child_node( const QString& node, bool inexistent_ok,
								  bool empty_ok, bool bSilent ) const
{
	if( isNull() ) {
		ERRORLOG( QString( "try to read %1 XML node from an empty parent %2." )
				  .arg( node ).arg( nodeName() ) );
		return nullptr;
	}
	QDomElement el = firstChildElement( node );
	if( el.isNull() ) {
		if ( !inexistent_ok && ! bSilent ) {
			WARNINGLOG( QString( "XML node %1->%2 should exists." )
						.arg( nodeName() ).arg( node ) );
		}
		return nullptr;
	}
	if( el.text().isEmpty() ) {
		if( !empty_ok && ! bSilent ) {
			WARNINGLOG( QString( "XML node %1->%2 should not be empty." )
						.arg( nodeName() ).arg( node ) );
		}
		return nullptr;
	}
	return el.text();
}

QString XMLNode::read_string( const QString& node, const QString& sDefaultValue,
							  bool inexistent_ok, bool empty_ok, bool bSilent ) const
{
	QString sText = read_child_node( node, inexistent_ok, empty_ok, bSilent );
	if ( sText.isNull() && ! sDefaultValue.isEmpty() ) {
		if ( ! bSilent ) {
			WARNINGLOG( QString( "Using default value %1 for %2" )
						.arg( sDefaultValue ).arg( node ) );
		}
		return sDefaultValue;
	}
	return sText;
}
QColor XMLNode::read_color( const QString& node, const QColor& default_value,
							bool inexistent_ok, bool empty_ok, bool bSilent ) const
{
	QString text = read_child_node( node, inexistent_ok, empty_ok, bSilent );
	
	if ( ! text.isEmpty() ) {
		QStringList textList = text.split( QLatin1Char( ',' ) );
		if ( textList.size() != 3 ) {
			if ( ! bSilent ) {
				WARNINGLOG( QString( "Invalid color format [%1] for node [%2]" )
							.arg( default_value.name() ).arg( node ) );
			}
			return default_value;
		}
		
		QColor color( textList[ 0 ].toInt(), textList[ 1 ].toInt(), textList[ 2 ].toInt() );
		if ( ! color.isValid() ) {
			if ( ! bSilent ) {
				WARNINGLOG( QString( "Invalid color values [%1] for node [%2]" )
							.arg( default_value.name() ).arg( node ) );
			}
			return default_value;
		}
		return color;
	}

	if ( ! bSilent ) {
		WARNINGLOG( QString( "Using default value [%1] for node [%2]" )
					.arg( default_value.name() ).arg( node ) );
	}
	return default_value;
}

float XMLNode::read_float( const QString& node, float default_value,
						   bool inexistent_ok, bool empty_ok, bool bSilent ) const
{
	QString ret = read_child_node( node, inexistent_ok, empty_ok, bSilent );
	if( ret.isNull() ) {
		if ( ! bSilent ) {
			WARNINGLOG( QString( "Using default value %1 for %2" )
						.arg( default_value ).arg( node ) );
		}
		return default_value;
	}
	QLocale c_locale = QLocale::c();
	return c_locale.toFloat( ret );
}

float XMLNode::read_float( const QString& node, float default_value,
						   bool *pFound, bool inexistent_ok, bool empty_ok,
						   bool bSilent ) const
{
	QString ret = read_child_node( node, inexistent_ok, empty_ok, bSilent );
	if( ret.isNull() ) {
		if ( ! bSilent ) {
			WARNINGLOG( QString( "Using default value %1 for %2" )
						.arg( default_value ).arg( node ) );
		}
		*pFound = false;
		return default_value;
	} else {
		*pFound = true;
		QLocale c_locale = QLocale::c();
		return c_locale.toFloat( ret );
	}
}

int XMLNode::read_int( const QString& node, int default_value, bool inexistent_ok,
					   bool empty_ok, bool bSilent ) const
{
	QString ret = read_child_node( node, inexistent_ok, empty_ok, bSilent );
	if( ret.isNull() ) {
		if ( ! bSilent ) {
			WARNINGLOG( QString( "Using default value %1 for %2" )
						.arg( default_value ).arg( node ) );
		}
		return default_value;
	}
	QLocale c_locale = QLocale::c();
	return c_locale.toInt( ret );
}

bool XMLNode::read_bool( const QString& node, bool default_value,
						 bool inexistent_ok, bool empty_ok, bool bSilent ) const
{
	QString ret = read_child_node( node, inexistent_ok, empty_ok, bSilent );
	if( ret.isNull() ) {
		if ( ! bSilent ) {
			WARNINGLOG( QString( "Using default value %1 for %2" )
						.arg( default_value ).arg( node ) );
		}
		return default_value;
	}
	if( ret=="true" ) {
		return true;
	} else {
		return false;
	}
}

bool XMLNode::read_bool( const QString& node, bool default_value,
						 bool* pFound, bool inexistent_ok, bool empty_ok,
						 bool bSilent ) const
{
	QString ret = read_child_node( node, inexistent_ok, empty_ok, bSilent );
	if( ret.isNull() ) {
		*pFound = false;
		if ( ! bSilent ) {
			WARNINGLOG( QString( "Using default value %1 for %2" )
						.arg( default_value ).arg( node ) );
		}
		return default_value;
	}

	*pFound = true;
	if( ret=="true" ) {
		return true;
	} else {
		return false;
	}
}

QString XMLNode::read_text( bool empty_ok, bool bSilent ) const
{
	QString text = toElement().text();
	if ( !empty_ok && text.isEmpty() && ! bSilent ) {
		WARNINGLOG( QString( "XML node %1 should not be empty." ).arg( nodeName() ) );
	}
	return text;
}

QString XMLNode::read_attribute( const QString& attribute,
								 const QString& default_value, bool inexistent_ok,
								 bool empty_ok, bool bSilent ) const
{
	QDomElement el = toElement();
	if ( !inexistent_ok && !el.hasAttribute( attribute ) ) {
		if ( ! bSilent ) {
			WARNINGLOG( QString( "XML node %1 attribute %2 should exists." )
						.arg( nodeName() ).arg( attribute ) );
		}
		return default_value;
	}
	QString attr = el.attribute( attribute );
	if ( attr.isEmpty() ) {
		if( !empty_ok && ! bSilent ) {
			WARNINGLOG( QString( "XML node %1 attribute %2 should not be empty." )
						.arg( nodeName() ).arg( attribute ) );
		}

		if ( ! bSilent ) {
			WARNINGLOG( QString( "Using default value %1 for attribute %2" )
						.arg( default_value ).arg( attribute ) );
		}
		return default_value;
	}
	return attr;
}

void XMLNode::write_attribute( const QString& attribute, const QString& value )
{
	toElement().setAttribute( attribute, value );
}

void XMLNode::write_child_node( const QString& node, const QString& text )
{
	QDomDocument doc = this->ownerDocument();
	QDomElement el = doc.createElement( node );
	QDomText txt = doc.createTextNode( text );
	el.appendChild( txt );
	this->appendChild( el );
}
void XMLNode::write_string( const QString& node, const QString& value )
{
	write_child_node( node, value );
}
void XMLNode::write_color( const QString& node, const QColor& color )
{
	write_child_node( node, QString( "%1,%2,%3" )
					  .arg( color.red() )
					  .arg( color.green() )
					  .arg( color.blue() ) );
}
void XMLNode::write_float( const QString& node, const float value )
{
	write_child_node( node, QString::number( value ) );
}
void XMLNode::write_int( const QString& node, const int value )
{
	write_child_node( node, QString::number( value ) );
}
void XMLNode::write_bool( const QString& name, const bool value )
{
	write_child_node( name, QString( ( value ? "true" : "false" ) ) );
}


XMLDoc::XMLDoc( ) { }

XMLDoc::XMLDoc( const QString& sSerialized ) {
	setContent( sSerialized );
}

bool XMLDoc::read( const QString& sFilePath, bool bSilent ) {
	
	QFile file( sFilePath );
	if ( !file.open( QIODevice::ReadOnly ) ) {
		ERRORLOG( QString( "Unable to open [%1] for reading" )
				  .arg( sFilePath ) );
		return false;
	}
	
	if ( Legacy::checkTinyXMLCompatMode( &file ) ) {
		// Document was created using TinyXML and not using QtXML. We
		// need to convert it first.
		if ( ! setContent( Legacy::convertFromTinyXML( &file ) ) ) {
			ERRORLOG( QString( "Unable to read conversion result document [%1]" )
					  .arg( sFilePath ) );
			file.close();
			return false;
		}
	}
	else  {
		// File was written using current format.
		if ( ! setContent( &file ) ) {
			ERRORLOG( QString( "Unable to read XML document [%1]" )
					  .arg( sFilePath ) );
			file.close();
			return false;
		}
	}
	file.close();
	
	return true;
}

bool XMLDoc::write( const QString& filepath )
{
	QFile file( filepath );
	if ( !file.open( QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate ) ) {
		ERRORLOG( QString( "Unable to open %1 for writing" ).arg( filepath ) );
		return false;
	}
	QTextStream out( &file );
	out.setCodec( "UTF-8" );
	out << toString().toUtf8();
	out.flush();

	bool rv = true;
	if ( !toString().isEmpty() && file.size() == 0 ) {
		rv = false;
	}

	file.close();
	return rv;
}

XMLNode XMLDoc::set_root( const QString& node_name, const QString& xmlns )
{
	QDomProcessingInstruction header = createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" );
	appendChild( header );
	XMLNode root = createElement( node_name );
	if ( !xmlns.isEmpty() ) {
		QDomElement el = root.toElement();
		el.setAttribute( "xmlns",XMLNS_BASE+xmlns );
		el.setAttribute( "xmlns:xsi",XMLNS_XSI );
	}
	appendChild( root );
	return root;
}

};
