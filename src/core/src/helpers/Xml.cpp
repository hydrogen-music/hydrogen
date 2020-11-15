
#include <hydrogen/helpers/xml.h>

#include <QtCore/QFile>
#include <QtCore/QLocale>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtXmlPatterns/QXmlSchema>
#include <QtXmlPatterns/QXmlSchemaValidator>
#include <QAbstractMessageHandler>

#define XMLNS_BASE "http://www.hydrogen-music.org/"
#define XMLNS_XSI "http://www.w3.org/2001/XMLSchema-instance"

namespace H2Core
{

class SilentMessageHandler : public QAbstractMessageHandler
{
public:
	SilentMessageHandler()
		: QAbstractMessageHandler(nullptr)
	{
	}

protected:
	virtual void handleMessage(QtMsgType type, const QString &description,
			const QUrl &identifier, const QSourceLocation &sourceLocation)
	{
		Q_UNUSED(type);
		Q_UNUSED(identifier);
	}

};


const char* XMLNode::__class_name ="XMLNode";

XMLNode::XMLNode() : Object( __class_name ) { }
XMLNode::XMLNode( QDomNode node ) : Object( __class_name ), QDomNode( node ) { }

XMLNode XMLNode::createNode( const QString& name )
{
	XMLNode node = this->ownerDocument().createElement( name );
	appendChild( node );
	return node;
}

QString XMLNode::read_child_node( const QString& node, bool inexistent_ok, bool empty_ok )
{
	if( isNull() ) {
		DEBUGLOG( QString( "try to read %1 XML node from an empty parent %2." ).arg( node ).arg( nodeName() ) );
		return nullptr;
	}
	QDomElement el = firstChildElement( node );
	if( el.isNull() ) {
		if( !inexistent_ok ) DEBUGLOG( QString( "XML node %1->%2 should exists." ).arg( nodeName() ).arg( node ) );
		return nullptr;
	}
	if( el.text().isEmpty() ) {
		if( !empty_ok ) DEBUGLOG( QString( "XML node %1->%2 should not be empty." ).arg( nodeName() ).arg( node ) );
		return nullptr;
	}
	return el.text();
}

QString XMLNode::read_string( const QString& node, const QString& default_value, bool inexistent_ok, bool empty_ok )
{
	QString ret = read_child_node( node, inexistent_ok, empty_ok );
	if( ret.isNull() ) {
		DEBUGLOG( QString( "Using default value %1 for %2" ).arg( default_value ).arg( node ) );
		return default_value;
	}
	return ret;
}

float XMLNode::read_float( const QString& node, float default_value, bool inexistent_ok, bool empty_ok )
{
	QString ret = read_child_node( node, inexistent_ok, empty_ok );
	if( ret.isNull() ) {
		DEBUGLOG( QString( "Using default value %1 for %2" ).arg( default_value ).arg( node ) );
		return default_value;
	}
	QLocale c_locale = QLocale::c();
	return c_locale.toFloat( ret );
}

int XMLNode::read_int( const QString& node, int default_value, bool inexistent_ok, bool empty_ok )
{
	QString ret = read_child_node( node, inexistent_ok, empty_ok );
	if( ret.isNull() ) {
		DEBUGLOG( QString( "Using default value %1 for %2" ).arg( default_value ).arg( node ) );
		return default_value;
	}
	QLocale c_locale = QLocale::c();
	return c_locale.toInt( ret );
}

bool XMLNode::read_bool( const QString& node, bool default_value, bool inexistent_ok, bool empty_ok )
{
	QString ret = read_child_node( node, inexistent_ok, empty_ok );
	if( ret.isNull() ) {
		DEBUGLOG( QString( "Using default value %1 for %2" ).arg( default_value ).arg( node ) );
		return default_value;
	}
	if( ret=="true" ) {
		return true;
	} else {
		return false;
	}
}

QString XMLNode::read_text( bool empty_ok )
{
	QString text = toElement().text();
	if ( !empty_ok && text.isEmpty() ) {
		DEBUGLOG( QString( "XML node %1 should not be empty." ).arg( nodeName() ) );
	}
	return text;
}

QString XMLNode::read_attribute( const QString& attribute, const QString& default_value, bool inexistent_ok, bool empty_ok )
{
	QDomElement el = toElement();
	if ( !inexistent_ok && !el.hasAttribute( attribute ) ) {
		DEBUGLOG( QString( "XML node %1 attribute %2 should exists." ).arg( nodeName() ).arg( attribute ) );
		return default_value;
	}
	QString attr = el.attribute( attribute );
	if ( attr.isEmpty() ) {
		if( !empty_ok ) DEBUGLOG( QString( "XML node %1 attribute %2 should not be empty." ).arg( nodeName() ).arg( attribute ) );
		DEBUGLOG( QString( "Using default value %1 for attribute %2" ).arg( default_value ).arg( attribute ) );
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

const char* XMLDoc::__class_name ="XMLDoc";

XMLDoc::XMLDoc( ) : Object( __class_name ) { }

bool XMLDoc::read( const QString& filepath, const QString& schemapath )
{
	SilentMessageHandler Handler;
	QXmlSchema schema;
	schema.setMessageHandler( &Handler );
	
	bool schema_usable = false;
	
	if( schemapath!=nullptr ) {
		QFile file( schemapath );
		if ( !file.open( QIODevice::ReadOnly ) ) {
			ERRORLOG( QString( "Unable to open XML schema %1 for reading" ).arg( schemapath ) );
		} else {
			schema.load( &file, QUrl::fromLocalFile( file.fileName() ) );
			file.close();
			if ( schema.isValid() ) {
				schema_usable = true;
			} else {
				ERRORLOG( QString( "%2 XML schema is not valid" ).arg( schemapath ) );
			}
		}
	}
	
	QFile file( filepath );
	if ( !file.open( QIODevice::ReadOnly ) ) {
		ERRORLOG( QString( "Unable to open %1 for reading" ).arg( filepath ) );
		return false;
	}
	
	if ( schema_usable ) {
		QXmlSchemaValidator validator( schema );
		if ( !validator.validate( &file, QUrl::fromLocalFile( file.fileName() ) ) ) {
			WARNINGLOG( QString( "XML document %1 is not valid (%2), loading may fail" ).arg( filepath ).arg( schemapath ) );
			file.close();
			return false;
		} else {
			INFOLOG( QString( "XML document %1 is valid (%2)" ).arg( filepath ).arg( schemapath ) );
		}
		file.seek( 0 );
	}
	
	if( !setContent( &file ) ) {
		ERRORLOG( QString( "Unable to read XML document %1" ).arg( filepath ) );
		file.close();
		return false;
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
