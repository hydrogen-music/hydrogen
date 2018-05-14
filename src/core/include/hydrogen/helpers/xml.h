
#ifndef H2C_XML_H
#define H2C_XML_H

#include <hydrogen/object.h>
#include <QtCore/QString>
#include <QtXml/QDomDocument>

namespace H2Core
{

/**
 * XMLNode is a subclass of QDomNode with read and write values methods
*/
class XMLNode : public H2Core::Object, public QDomNode
{
		H2_OBJECT
	public:
		/** basic constructor */
		XMLNode( );
		/** to wrap a QDomNode */
		XMLNode( QDomNode node );

		/**
		 * reads an integer stored into a child node
		 * \param node the name of the child node to read into
		 * \param default_value the value returned if something goes wrong
		 * \param inexistent_ok if set to false output a DEBUG log line if the node doesn't exists
		 * \param empty_ok if set to false output a DEBUG log line if the child node is empty
		 */
		int read_int( const QString& node, int default_value, bool inexistent_ok=true, bool empty_ok=true );
		/**
		 * reads a boolean stored into a child node
		 * \param node the name of the child node to read into
		 * \param default_value the value returned if something goes wrong
		 * \param inexistent_ok if set to false output a DEBUG log line if the node doesn't exists
		 * \param empty_ok if set to false output a DEBUG log line if the child node is empty
		 */
		bool read_bool( const QString& node, bool default_value, bool inexistent_ok=true, bool empty_ok=true );
		/**
		 * reads a float stored into a child node
		 * \param node the name of the child node to read into
		 * \param default_value the value returned if something goes wrong
		 * \param inexistent_ok if set to false output a DEBUG log line if the node doesn't exists
		 * \param empty_ok if set to false output a DEBUG log line if the child node is empty
		 */
		float read_float( const QString& node, float default_value, bool inexistent_ok=true, bool empty_ok=true );
		/**
		 * reads a string stored into a child node
		 * \param node the name of the child node to read into
		 * \param default_value the value returned if something goes wrong
		 * \param inexistent_ok if set to false output a DEBUG log line if the node doesn't exists
		 * \param empty_ok if set to false output a DEBUG log line if the child node is empty
		 */
		QString read_string( const QString& node, const QString& default_value, bool inexistent_ok=true, bool empty_ok=true );

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
	private:
		/**
		 * reads a string stored into a child node
		 * \param node the name of the child node to read into
		 * \param inexistent_ok if set to false output a DEBUG log line if the node doesn't exists
		 * \param empty_ok if set to false output a DEBUG log line if the child node is empty
		 */
		QString read_child_node( const QString& node, bool inexistent_ok, bool empty_ok );
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
class XMLDoc : public H2Core::Object, public QDomDocument
{
		H2_OBJECT
	public:
		/** basic constructor */
		XMLDoc( );
		/**
		 * read the content of an xml file
		 * \param filepath the path to the file to read from
		 * \param schema_path the path to the XML Schema file
		 */
		bool read( const QString& filepath, const QString& schemapath=0 );
		/**
		 * write itself into a file
		 * \param filepath the path to the file to write to
		 */
		bool write( const QString& filepath );
		/**
		 * create the xml header and root node
		 * \param node_name, the name of the rootnode to build
		 * \param xmlns, the xml namespace prefix to add after XMLNS_BASE
		 */
		void set_root( const QString& node_name, const QString& xmlns );
};

};

#endif  // H2C_XML_H

/* vim: set softtabstop=4 noexpandtab: */
