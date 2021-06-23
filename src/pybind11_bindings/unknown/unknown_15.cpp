#include <QtCore/qbytearray.h> // 
#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qbytearray.h> // QByteArrayDataPtr
#include <QtCore/qbytearray.h> // QByteRef
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qiodevice.h> // 
#include <QtCore/qiodevice.h> // QIODevice
#include <QtCore/qlist.h> // QList
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
#include <QtCore/qobject.h> // QObject
#include <QtCore/qobjectdefs.h> // QMetaObject
#include <QtCore/qregexp.h> // QRegExp
#include <QtCore/qregularexpression.h> // QRegularExpression
#include <QtCore/qregularexpression.h> // QRegularExpressionMatch
#include <QtCore/qstring.h> // 
#include <QtCore/qstring.h> // QCharRef
#include <QtCore/qstring.h> // QLatin1String
#include <QtCore/qstring.h> // QString
#include <QtCore/qstring.h> // QStringRef
#include <QtCore/qstringlist.h> // QStringList
#include <QtCore/qstringliteral.h> // QStringDataPtr
#include <QtCore/qstringview.h> // QStringView
#include <QtCore/qtextstream.h> // QTextStream
#include <QtCore/qvector.h> // QVector
#include <iostream> // --trace
#include <iterator> // std::reverse_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <qtreset.h>


#include <pybind11/pybind11.h>
#include <functional>
#include <string>
#include <QtCore/qmetaobject.h>
#include <QtCore/qthread.h>
#include <QtCore/qline.h>
#include <QtCore/qmargins.h>
#include <QtCore/qurl.h>
#include <QtCore/qurlquery.h>
#include <QtCore/qbitarray.h>
#include <QtCore/qrect.h>
#include <QtCore/qtextcodec.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qeasingcurve.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qxmlstream.h>
#include <QtGui/qcolor.h>
#include <QtCore/qabstracteventdispatcher.h>
#include <QtCore/qsocketnotifier.h>
#include <QtCore/qabstractnativeeventfilter.h>
#include <QtCore/qcalendar.h>
#include <QtXml/qdom.h>
#include <QtCore/qmimedata.h>
#include <QtCore/qtimezone.h>
#include <setjmp.h>
#include <core/Logger.h>
#include <custom_qt_casters.h>


#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>)
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*)
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>)
#endif

void bind_unknown_unknown_15(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B661_[QDomDocumentType] ";
	{ // QDomDocumentType file: line:278
		pybind11::class_<QDomDocumentType, std::shared_ptr<QDomDocumentType>, QDomNode> cl(M(""), "QDomDocumentType", "");
		cl.def( pybind11::init( [](){ return new QDomDocumentType(); } ) );
		cl.def( pybind11::init( [](QDomDocumentType const &o){ return new QDomDocumentType(o); } ) );
		cl.def("assign", (class QDomDocumentType & (QDomDocumentType::*)(const class QDomDocumentType &)) &QDomDocumentType::operator=, "C++: QDomDocumentType::operator=(const class QDomDocumentType &) --> class QDomDocumentType &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("name", (class QString (QDomDocumentType::*)() const) &QDomDocumentType::name, "C++: QDomDocumentType::name() const --> class QString");
		cl.def("entities", (class QDomNamedNodeMap (QDomDocumentType::*)() const) &QDomDocumentType::entities, "C++: QDomDocumentType::entities() const --> class QDomNamedNodeMap");
		cl.def("notations", (class QDomNamedNodeMap (QDomDocumentType::*)() const) &QDomDocumentType::notations, "C++: QDomDocumentType::notations() const --> class QDomNamedNodeMap");
		cl.def("publicId", (class QString (QDomDocumentType::*)() const) &QDomDocumentType::publicId, "C++: QDomDocumentType::publicId() const --> class QString");
		cl.def("systemId", (class QString (QDomDocumentType::*)() const) &QDomDocumentType::systemId, "C++: QDomDocumentType::systemId() const --> class QString");
		cl.def("internalSubset", (class QString (QDomDocumentType::*)() const) &QDomDocumentType::internalSubset, "C++: QDomDocumentType::internalSubset() const --> class QString");
		cl.def("nodeType", (enum QDomNode::NodeType (QDomDocumentType::*)() const) &QDomDocumentType::nodeType, "C++: QDomDocumentType::nodeType() const --> enum QDomNode::NodeType");
	}
	std::cout << "B662_[QDomDocument] ";
	{ // QDomDocument file: line:304
		pybind11::class_<QDomDocument, std::shared_ptr<QDomDocument>, QDomNode> cl(M(""), "QDomDocument", "");
		cl.def( pybind11::init( [](){ return new QDomDocument(); } ) );
		cl.def( pybind11::init<const class QString &>(), pybind11::arg("name") );

		cl.def( pybind11::init<const class QDomDocumentType &>(), pybind11::arg("doctype") );

		cl.def( pybind11::init( [](QDomDocument const &o){ return new QDomDocument(o); } ) );
		cl.def("assign", (class QDomDocument & (QDomDocument::*)(const class QDomDocument &)) &QDomDocument::operator=, "C++: QDomDocument::operator=(const class QDomDocument &) --> class QDomDocument &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("createElement", (class QDomElement (QDomDocument::*)(const class QString &)) &QDomDocument::createElement, "C++: QDomDocument::createElement(const class QString &) --> class QDomElement", pybind11::arg("tagName"));
		cl.def("createDocumentFragment", (class QDomDocumentFragment (QDomDocument::*)()) &QDomDocument::createDocumentFragment, "C++: QDomDocument::createDocumentFragment() --> class QDomDocumentFragment");
		cl.def("createTextNode", (class QDomText (QDomDocument::*)(const class QString &)) &QDomDocument::createTextNode, "C++: QDomDocument::createTextNode(const class QString &) --> class QDomText", pybind11::arg("data"));
		cl.def("createComment", (class QDomComment (QDomDocument::*)(const class QString &)) &QDomDocument::createComment, "C++: QDomDocument::createComment(const class QString &) --> class QDomComment", pybind11::arg("data"));
		cl.def("createCDATASection", (class QDomCDATASection (QDomDocument::*)(const class QString &)) &QDomDocument::createCDATASection, "C++: QDomDocument::createCDATASection(const class QString &) --> class QDomCDATASection", pybind11::arg("data"));
		cl.def("createProcessingInstruction", (class QDomProcessingInstruction (QDomDocument::*)(const class QString &, const class QString &)) &QDomDocument::createProcessingInstruction, "C++: QDomDocument::createProcessingInstruction(const class QString &, const class QString &) --> class QDomProcessingInstruction", pybind11::arg("target"), pybind11::arg("data"));
		cl.def("createAttribute", (class QDomAttr (QDomDocument::*)(const class QString &)) &QDomDocument::createAttribute, "C++: QDomDocument::createAttribute(const class QString &) --> class QDomAttr", pybind11::arg("name"));
		cl.def("createEntityReference", (class QDomEntityReference (QDomDocument::*)(const class QString &)) &QDomDocument::createEntityReference, "C++: QDomDocument::createEntityReference(const class QString &) --> class QDomEntityReference", pybind11::arg("name"));
		cl.def("elementsByTagName", (class QDomNodeList (QDomDocument::*)(const class QString &) const) &QDomDocument::elementsByTagName, "C++: QDomDocument::elementsByTagName(const class QString &) const --> class QDomNodeList", pybind11::arg("tagname"));
		cl.def("importNode", (class QDomNode (QDomDocument::*)(const class QDomNode &, bool)) &QDomDocument::importNode, "C++: QDomDocument::importNode(const class QDomNode &, bool) --> class QDomNode", pybind11::arg("importedNode"), pybind11::arg("deep"));
		cl.def("createElementNS", (class QDomElement (QDomDocument::*)(const class QString &, const class QString &)) &QDomDocument::createElementNS, "C++: QDomDocument::createElementNS(const class QString &, const class QString &) --> class QDomElement", pybind11::arg("nsURI"), pybind11::arg("qName"));
		cl.def("createAttributeNS", (class QDomAttr (QDomDocument::*)(const class QString &, const class QString &)) &QDomDocument::createAttributeNS, "C++: QDomDocument::createAttributeNS(const class QString &, const class QString &) --> class QDomAttr", pybind11::arg("nsURI"), pybind11::arg("qName"));
		cl.def("elementsByTagNameNS", (class QDomNodeList (QDomDocument::*)(const class QString &, const class QString &)) &QDomDocument::elementsByTagNameNS, "C++: QDomDocument::elementsByTagNameNS(const class QString &, const class QString &) --> class QDomNodeList", pybind11::arg("nsURI"), pybind11::arg("localName"));
		cl.def("elementById", (class QDomElement (QDomDocument::*)(const class QString &)) &QDomDocument::elementById, "C++: QDomDocument::elementById(const class QString &) --> class QDomElement", pybind11::arg("elementId"));
		cl.def("doctype", (class QDomDocumentType (QDomDocument::*)() const) &QDomDocument::doctype, "C++: QDomDocument::doctype() const --> class QDomDocumentType");
		cl.def("implementation", (class QDomImplementation (QDomDocument::*)() const) &QDomDocument::implementation, "C++: QDomDocument::implementation() const --> class QDomImplementation");
		cl.def("documentElement", (class QDomElement (QDomDocument::*)() const) &QDomDocument::documentElement, "C++: QDomDocument::documentElement() const --> class QDomElement");
		cl.def("nodeType", (enum QDomNode::NodeType (QDomDocument::*)() const) &QDomDocument::nodeType, "C++: QDomDocument::nodeType() const --> enum QDomNode::NodeType");
		cl.def("setContent", [](QDomDocument &o, const class QString & a0, bool const & a1) -> bool { return o.setContent(a0, a1); }, "", pybind11::arg("text"), pybind11::arg("namespaceProcessing"));
		cl.def("setContent", [](QDomDocument &o, const class QString & a0, bool const & a1, class QString * a2) -> bool { return o.setContent(a0, a1, a2); }, "", pybind11::arg("text"), pybind11::arg("namespaceProcessing"), pybind11::arg("errorMsg"));
		cl.def("setContent", [](QDomDocument &o, const class QString & a0, bool const & a1, class QString * a2, int * a3) -> bool { return o.setContent(a0, a1, a2, a3); }, "", pybind11::arg("text"), pybind11::arg("namespaceProcessing"), pybind11::arg("errorMsg"), pybind11::arg("errorLine"));
		cl.def("setContent", (bool (QDomDocument::*)(const class QString &, bool, class QString *, int *, int *)) &QDomDocument::setContent, "C++: QDomDocument::setContent(const class QString &, bool, class QString *, int *, int *) --> bool", pybind11::arg("text"), pybind11::arg("namespaceProcessing"), pybind11::arg("errorMsg"), pybind11::arg("errorLine"), pybind11::arg("errorColumn"));
		cl.def("setContent", [](QDomDocument &o, class QIODevice * a0, bool const & a1) -> bool { return o.setContent(a0, a1); }, "", pybind11::arg("dev"), pybind11::arg("namespaceProcessing"));
		cl.def("setContent", [](QDomDocument &o, class QIODevice * a0, bool const & a1, class QString * a2) -> bool { return o.setContent(a0, a1, a2); }, "", pybind11::arg("dev"), pybind11::arg("namespaceProcessing"), pybind11::arg("errorMsg"));
		cl.def("setContent", [](QDomDocument &o, class QIODevice * a0, bool const & a1, class QString * a2, int * a3) -> bool { return o.setContent(a0, a1, a2, a3); }, "", pybind11::arg("dev"), pybind11::arg("namespaceProcessing"), pybind11::arg("errorMsg"), pybind11::arg("errorLine"));
		cl.def("setContent", (bool (QDomDocument::*)(class QIODevice *, bool, class QString *, int *, int *)) &QDomDocument::setContent, "C++: QDomDocument::setContent(class QIODevice *, bool, class QString *, int *, int *) --> bool", pybind11::arg("dev"), pybind11::arg("namespaceProcessing"), pybind11::arg("errorMsg"), pybind11::arg("errorLine"), pybind11::arg("errorColumn"));
		cl.def("setContent", [](QDomDocument &o, const class QString & a0) -> bool { return o.setContent(a0); }, "", pybind11::arg("text"));
		cl.def("setContent", [](QDomDocument &o, const class QString & a0, class QString * a1) -> bool { return o.setContent(a0, a1); }, "", pybind11::arg("text"), pybind11::arg("errorMsg"));
		cl.def("setContent", [](QDomDocument &o, const class QString & a0, class QString * a1, int * a2) -> bool { return o.setContent(a0, a1, a2); }, "", pybind11::arg("text"), pybind11::arg("errorMsg"), pybind11::arg("errorLine"));
		cl.def("setContent", (bool (QDomDocument::*)(const class QString &, class QString *, int *, int *)) &QDomDocument::setContent, "C++: QDomDocument::setContent(const class QString &, class QString *, int *, int *) --> bool", pybind11::arg("text"), pybind11::arg("errorMsg"), pybind11::arg("errorLine"), pybind11::arg("errorColumn"));
		cl.def("setContent", [](QDomDocument &o, class QIODevice * a0) -> bool { return o.setContent(a0); }, "", pybind11::arg("dev"));
		cl.def("setContent", [](QDomDocument &o, class QIODevice * a0, class QString * a1) -> bool { return o.setContent(a0, a1); }, "", pybind11::arg("dev"), pybind11::arg("errorMsg"));
		cl.def("setContent", [](QDomDocument &o, class QIODevice * a0, class QString * a1, int * a2) -> bool { return o.setContent(a0, a1, a2); }, "", pybind11::arg("dev"), pybind11::arg("errorMsg"), pybind11::arg("errorLine"));
		cl.def("setContent", (bool (QDomDocument::*)(class QIODevice *, class QString *, int *, int *)) &QDomDocument::setContent, "C++: QDomDocument::setContent(class QIODevice *, class QString *, int *, int *) --> bool", pybind11::arg("dev"), pybind11::arg("errorMsg"), pybind11::arg("errorLine"), pybind11::arg("errorColumn"));
		cl.def("setContent", [](QDomDocument &o, class QXmlStreamReader * a0, bool const & a1) -> bool { return o.setContent(a0, a1); }, "", pybind11::arg("reader"), pybind11::arg("namespaceProcessing"));
		cl.def("setContent", [](QDomDocument &o, class QXmlStreamReader * a0, bool const & a1, class QString * a2) -> bool { return o.setContent(a0, a1, a2); }, "", pybind11::arg("reader"), pybind11::arg("namespaceProcessing"), pybind11::arg("errorMsg"));
		cl.def("setContent", [](QDomDocument &o, class QXmlStreamReader * a0, bool const & a1, class QString * a2, int * a3) -> bool { return o.setContent(a0, a1, a2, a3); }, "", pybind11::arg("reader"), pybind11::arg("namespaceProcessing"), pybind11::arg("errorMsg"), pybind11::arg("errorLine"));
		cl.def("setContent", (bool (QDomDocument::*)(class QXmlStreamReader *, bool, class QString *, int *, int *)) &QDomDocument::setContent, "C++: QDomDocument::setContent(class QXmlStreamReader *, bool, class QString *, int *, int *) --> bool", pybind11::arg("reader"), pybind11::arg("namespaceProcessing"), pybind11::arg("errorMsg"), pybind11::arg("errorLine"), pybind11::arg("errorColumn"));
		cl.def("toString", [](QDomDocument const &o) -> QString { return o.toString(); }, "");
		cl.def("toString", (class QString (QDomDocument::*)(int) const) &QDomDocument::toString, "C++: QDomDocument::toString(int) const --> class QString", pybind11::arg(""));
	}
	std::cout << "B663_[QDomNamedNodeMap] ";
	{ // QDomNamedNodeMap file: line:372
		pybind11::class_<QDomNamedNodeMap, std::shared_ptr<QDomNamedNodeMap>> cl(M(""), "QDomNamedNodeMap", "");
		cl.def( pybind11::init( [](){ return new QDomNamedNodeMap(); } ) );
		cl.def( pybind11::init( [](QDomNamedNodeMap const &o){ return new QDomNamedNodeMap(o); } ) );
		cl.def("assign", (class QDomNamedNodeMap & (QDomNamedNodeMap::*)(const class QDomNamedNodeMap &)) &QDomNamedNodeMap::operator=, "C++: QDomNamedNodeMap::operator=(const class QDomNamedNodeMap &) --> class QDomNamedNodeMap &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("__eq__", (bool (QDomNamedNodeMap::*)(const class QDomNamedNodeMap &) const) &QDomNamedNodeMap::operator==, "C++: QDomNamedNodeMap::operator==(const class QDomNamedNodeMap &) const --> bool", pybind11::arg(""));
		cl.def("__ne__", (bool (QDomNamedNodeMap::*)(const class QDomNamedNodeMap &) const) &QDomNamedNodeMap::operator!=, "C++: QDomNamedNodeMap::operator!=(const class QDomNamedNodeMap &) const --> bool", pybind11::arg(""));
		cl.def("namedItem", (class QDomNode (QDomNamedNodeMap::*)(const class QString &) const) &QDomNamedNodeMap::namedItem, "C++: QDomNamedNodeMap::namedItem(const class QString &) const --> class QDomNode", pybind11::arg("name"));
		cl.def("setNamedItem", (class QDomNode (QDomNamedNodeMap::*)(const class QDomNode &)) &QDomNamedNodeMap::setNamedItem, "C++: QDomNamedNodeMap::setNamedItem(const class QDomNode &) --> class QDomNode", pybind11::arg("newNode"));
		cl.def("removeNamedItem", (class QDomNode (QDomNamedNodeMap::*)(const class QString &)) &QDomNamedNodeMap::removeNamedItem, "C++: QDomNamedNodeMap::removeNamedItem(const class QString &) --> class QDomNode", pybind11::arg("name"));
		cl.def("item", (class QDomNode (QDomNamedNodeMap::*)(int) const) &QDomNamedNodeMap::item, "C++: QDomNamedNodeMap::item(int) const --> class QDomNode", pybind11::arg("index"));
		cl.def("namedItemNS", (class QDomNode (QDomNamedNodeMap::*)(const class QString &, const class QString &) const) &QDomNamedNodeMap::namedItemNS, "C++: QDomNamedNodeMap::namedItemNS(const class QString &, const class QString &) const --> class QDomNode", pybind11::arg("nsURI"), pybind11::arg("localName"));
		cl.def("setNamedItemNS", (class QDomNode (QDomNamedNodeMap::*)(const class QDomNode &)) &QDomNamedNodeMap::setNamedItemNS, "C++: QDomNamedNodeMap::setNamedItemNS(const class QDomNode &) --> class QDomNode", pybind11::arg("newNode"));
		cl.def("removeNamedItemNS", (class QDomNode (QDomNamedNodeMap::*)(const class QString &, const class QString &)) &QDomNamedNodeMap::removeNamedItemNS, "C++: QDomNamedNodeMap::removeNamedItemNS(const class QString &, const class QString &) --> class QDomNode", pybind11::arg("nsURI"), pybind11::arg("localName"));
		cl.def("length", (int (QDomNamedNodeMap::*)() const) &QDomNamedNodeMap::length, "C++: QDomNamedNodeMap::length() const --> int");
		cl.def("count", (int (QDomNamedNodeMap::*)() const) &QDomNamedNodeMap::count, "C++: QDomNamedNodeMap::count() const --> int");
		cl.def("size", (int (QDomNamedNodeMap::*)() const) &QDomNamedNodeMap::size, "C++: QDomNamedNodeMap::size() const --> int");
		cl.def("isEmpty", (bool (QDomNamedNodeMap::*)() const) &QDomNamedNodeMap::isEmpty, "C++: QDomNamedNodeMap::isEmpty() const --> bool");
		cl.def("contains", (bool (QDomNamedNodeMap::*)(const class QString &) const) &QDomNamedNodeMap::contains, "C++: QDomNamedNodeMap::contains(const class QString &) const --> bool", pybind11::arg("name"));
	}
	std::cout << "B664_[QDomDocumentFragment] ";
	{ // QDomDocumentFragment file: line:409
		pybind11::class_<QDomDocumentFragment, std::shared_ptr<QDomDocumentFragment>, QDomNode> cl(M(""), "QDomDocumentFragment", "");
		cl.def( pybind11::init( [](){ return new QDomDocumentFragment(); } ) );
		cl.def( pybind11::init( [](QDomDocumentFragment const &o){ return new QDomDocumentFragment(o); } ) );
		cl.def("assign", (class QDomDocumentFragment & (QDomDocumentFragment::*)(const class QDomDocumentFragment &)) &QDomDocumentFragment::operator=, "C++: QDomDocumentFragment::operator=(const class QDomDocumentFragment &) --> class QDomDocumentFragment &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("nodeType", (enum QDomNode::NodeType (QDomDocumentFragment::*)() const) &QDomDocumentFragment::nodeType, "C++: QDomDocumentFragment::nodeType() const --> enum QDomNode::NodeType");
	}
}
