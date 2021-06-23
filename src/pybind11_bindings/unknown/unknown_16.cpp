#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
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

void bind_unknown_unknown_16(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B665_[QDomCharacterData] ";
	{ // QDomCharacterData file: line:426
		pybind11::class_<QDomCharacterData, std::shared_ptr<QDomCharacterData>, QDomNode> cl(M(""), "QDomCharacterData", "");
		cl.def( pybind11::init( [](){ return new QDomCharacterData(); } ) );
		cl.def( pybind11::init( [](QDomCharacterData const &o){ return new QDomCharacterData(o); } ) );
		cl.def("assign", (class QDomCharacterData & (QDomCharacterData::*)(const class QDomCharacterData &)) &QDomCharacterData::operator=, "C++: QDomCharacterData::operator=(const class QDomCharacterData &) --> class QDomCharacterData &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("substringData", (class QString (QDomCharacterData::*)(unsigned long, unsigned long)) &QDomCharacterData::substringData, "C++: QDomCharacterData::substringData(unsigned long, unsigned long) --> class QString", pybind11::arg("offset"), pybind11::arg("count"));
		cl.def("appendData", (void (QDomCharacterData::*)(const class QString &)) &QDomCharacterData::appendData, "C++: QDomCharacterData::appendData(const class QString &) --> void", pybind11::arg("arg"));
		cl.def("insertData", (void (QDomCharacterData::*)(unsigned long, const class QString &)) &QDomCharacterData::insertData, "C++: QDomCharacterData::insertData(unsigned long, const class QString &) --> void", pybind11::arg("offset"), pybind11::arg("arg"));
		cl.def("deleteData", (void (QDomCharacterData::*)(unsigned long, unsigned long)) &QDomCharacterData::deleteData, "C++: QDomCharacterData::deleteData(unsigned long, unsigned long) --> void", pybind11::arg("offset"), pybind11::arg("count"));
		cl.def("replaceData", (void (QDomCharacterData::*)(unsigned long, unsigned long, const class QString &)) &QDomCharacterData::replaceData, "C++: QDomCharacterData::replaceData(unsigned long, unsigned long, const class QString &) --> void", pybind11::arg("offset"), pybind11::arg("count"), pybind11::arg("arg"));
		cl.def("length", (int (QDomCharacterData::*)() const) &QDomCharacterData::length, "C++: QDomCharacterData::length() const --> int");
		cl.def("data", (class QString (QDomCharacterData::*)() const) &QDomCharacterData::data, "C++: QDomCharacterData::data() const --> class QString");
		cl.def("setData", (void (QDomCharacterData::*)(const class QString &)) &QDomCharacterData::setData, "C++: QDomCharacterData::setData(const class QString &) --> void", pybind11::arg(""));
		cl.def("nodeType", (enum QDomNode::NodeType (QDomCharacterData::*)() const) &QDomCharacterData::nodeType, "C++: QDomCharacterData::nodeType() const --> enum QDomNode::NodeType");
	}
	std::cout << "B666_[QDomAttr] ";
	{ // QDomAttr file: line:459
		pybind11::class_<QDomAttr, std::shared_ptr<QDomAttr>, QDomNode> cl(M(""), "QDomAttr", "");
		cl.def( pybind11::init( [](){ return new QDomAttr(); } ) );
		cl.def( pybind11::init( [](QDomAttr const &o){ return new QDomAttr(o); } ) );
		cl.def("assign", (class QDomAttr & (QDomAttr::*)(const class QDomAttr &)) &QDomAttr::operator=, "C++: QDomAttr::operator=(const class QDomAttr &) --> class QDomAttr &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("name", (class QString (QDomAttr::*)() const) &QDomAttr::name, "C++: QDomAttr::name() const --> class QString");
		cl.def("specified", (bool (QDomAttr::*)() const) &QDomAttr::specified, "C++: QDomAttr::specified() const --> bool");
		cl.def("ownerElement", (class QDomElement (QDomAttr::*)() const) &QDomAttr::ownerElement, "C++: QDomAttr::ownerElement() const --> class QDomElement");
		cl.def("value", (class QString (QDomAttr::*)() const) &QDomAttr::value, "C++: QDomAttr::value() const --> class QString");
		cl.def("setValue", (void (QDomAttr::*)(const class QString &)) &QDomAttr::setValue, "C++: QDomAttr::setValue(const class QString &) --> void", pybind11::arg(""));
		cl.def("nodeType", (enum QDomNode::NodeType (QDomAttr::*)() const) &QDomAttr::nodeType, "C++: QDomAttr::nodeType() const --> enum QDomNode::NodeType");
	}
	std::cout << "B667_[QDomElement] ";
	{ // QDomElement file: line:486
		pybind11::class_<QDomElement, std::shared_ptr<QDomElement>, QDomNode> cl(M(""), "QDomElement", "");
		cl.def( pybind11::init( [](){ return new QDomElement(); } ) );
		cl.def( pybind11::init( [](QDomElement const &o){ return new QDomElement(o); } ) );
		cl.def("assign", (class QDomElement & (QDomElement::*)(const class QDomElement &)) &QDomElement::operator=, "C++: QDomElement::operator=(const class QDomElement &) --> class QDomElement &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("attribute", [](QDomElement const &o, const class QString & a0) -> QString { return o.attribute(a0); }, "", pybind11::arg("name"));
		cl.def("attribute", (class QString (QDomElement::*)(const class QString &, const class QString &) const) &QDomElement::attribute, "C++: QDomElement::attribute(const class QString &, const class QString &) const --> class QString", pybind11::arg("name"), pybind11::arg("defValue"));
		cl.def("setAttribute", (void (QDomElement::*)(const class QString &, const class QString &)) &QDomElement::setAttribute, "C++: QDomElement::setAttribute(const class QString &, const class QString &) --> void", pybind11::arg("name"), pybind11::arg("value"));
		cl.def("setAttribute", (void (QDomElement::*)(const class QString &, long long)) &QDomElement::setAttribute, "C++: QDomElement::setAttribute(const class QString &, long long) --> void", pybind11::arg("name"), pybind11::arg("value"));
		cl.def("setAttribute", (void (QDomElement::*)(const class QString &, unsigned long long)) &QDomElement::setAttribute, "C++: QDomElement::setAttribute(const class QString &, unsigned long long) --> void", pybind11::arg("name"), pybind11::arg("value"));
		cl.def("setAttribute", (void (QDomElement::*)(const class QString &, int)) &QDomElement::setAttribute, "C++: QDomElement::setAttribute(const class QString &, int) --> void", pybind11::arg("name"), pybind11::arg("value"));
		cl.def("setAttribute", (void (QDomElement::*)(const class QString &, unsigned int)) &QDomElement::setAttribute, "C++: QDomElement::setAttribute(const class QString &, unsigned int) --> void", pybind11::arg("name"), pybind11::arg("value"));
		cl.def("setAttribute", (void (QDomElement::*)(const class QString &, float)) &QDomElement::setAttribute, "C++: QDomElement::setAttribute(const class QString &, float) --> void", pybind11::arg("name"), pybind11::arg("value"));
		cl.def("setAttribute", (void (QDomElement::*)(const class QString &, double)) &QDomElement::setAttribute, "C++: QDomElement::setAttribute(const class QString &, double) --> void", pybind11::arg("name"), pybind11::arg("value"));
		cl.def("removeAttribute", (void (QDomElement::*)(const class QString &)) &QDomElement::removeAttribute, "C++: QDomElement::removeAttribute(const class QString &) --> void", pybind11::arg("name"));
		cl.def("attributeNode", (class QDomAttr (QDomElement::*)(const class QString &)) &QDomElement::attributeNode, "C++: QDomElement::attributeNode(const class QString &) --> class QDomAttr", pybind11::arg("name"));
		cl.def("setAttributeNode", (class QDomAttr (QDomElement::*)(const class QDomAttr &)) &QDomElement::setAttributeNode, "C++: QDomElement::setAttributeNode(const class QDomAttr &) --> class QDomAttr", pybind11::arg("newAttr"));
		cl.def("removeAttributeNode", (class QDomAttr (QDomElement::*)(const class QDomAttr &)) &QDomElement::removeAttributeNode, "C++: QDomElement::removeAttributeNode(const class QDomAttr &) --> class QDomAttr", pybind11::arg("oldAttr"));
		cl.def("elementsByTagName", (class QDomNodeList (QDomElement::*)(const class QString &) const) &QDomElement::elementsByTagName, "C++: QDomElement::elementsByTagName(const class QString &) const --> class QDomNodeList", pybind11::arg("tagname"));
		cl.def("hasAttribute", (bool (QDomElement::*)(const class QString &) const) &QDomElement::hasAttribute, "C++: QDomElement::hasAttribute(const class QString &) const --> bool", pybind11::arg("name"));
		cl.def("attributeNS", [](QDomElement const &o, const class QString & a0, const class QString & a1) -> QString { return o.attributeNS(a0, a1); }, "", pybind11::arg("nsURI"), pybind11::arg("localName"));
		cl.def("attributeNS", (class QString (QDomElement::*)(const class QString, const class QString &, const class QString &) const) &QDomElement::attributeNS, "C++: QDomElement::attributeNS(const class QString, const class QString &, const class QString &) const --> class QString", pybind11::arg("nsURI"), pybind11::arg("localName"), pybind11::arg("defValue"));
		cl.def("setAttributeNS", (void (QDomElement::*)(const class QString, const class QString &, const class QString &)) &QDomElement::setAttributeNS, "C++: QDomElement::setAttributeNS(const class QString, const class QString &, const class QString &) --> void", pybind11::arg("nsURI"), pybind11::arg("qName"), pybind11::arg("value"));
		cl.def("setAttributeNS", (void (QDomElement::*)(const class QString, const class QString &, int)) &QDomElement::setAttributeNS, "C++: QDomElement::setAttributeNS(const class QString, const class QString &, int) --> void", pybind11::arg("nsURI"), pybind11::arg("qName"), pybind11::arg("value"));
		cl.def("setAttributeNS", (void (QDomElement::*)(const class QString, const class QString &, unsigned int)) &QDomElement::setAttributeNS, "C++: QDomElement::setAttributeNS(const class QString, const class QString &, unsigned int) --> void", pybind11::arg("nsURI"), pybind11::arg("qName"), pybind11::arg("value"));
		cl.def("setAttributeNS", (void (QDomElement::*)(const class QString, const class QString &, long long)) &QDomElement::setAttributeNS, "C++: QDomElement::setAttributeNS(const class QString, const class QString &, long long) --> void", pybind11::arg("nsURI"), pybind11::arg("qName"), pybind11::arg("value"));
		cl.def("setAttributeNS", (void (QDomElement::*)(const class QString, const class QString &, unsigned long long)) &QDomElement::setAttributeNS, "C++: QDomElement::setAttributeNS(const class QString, const class QString &, unsigned long long) --> void", pybind11::arg("nsURI"), pybind11::arg("qName"), pybind11::arg("value"));
		cl.def("setAttributeNS", (void (QDomElement::*)(const class QString, const class QString &, double)) &QDomElement::setAttributeNS, "C++: QDomElement::setAttributeNS(const class QString, const class QString &, double) --> void", pybind11::arg("nsURI"), pybind11::arg("qName"), pybind11::arg("value"));
		cl.def("removeAttributeNS", (void (QDomElement::*)(const class QString &, const class QString &)) &QDomElement::removeAttributeNS, "C++: QDomElement::removeAttributeNS(const class QString &, const class QString &) --> void", pybind11::arg("nsURI"), pybind11::arg("localName"));
		cl.def("attributeNodeNS", (class QDomAttr (QDomElement::*)(const class QString &, const class QString &)) &QDomElement::attributeNodeNS, "C++: QDomElement::attributeNodeNS(const class QString &, const class QString &) --> class QDomAttr", pybind11::arg("nsURI"), pybind11::arg("localName"));
		cl.def("setAttributeNodeNS", (class QDomAttr (QDomElement::*)(const class QDomAttr &)) &QDomElement::setAttributeNodeNS, "C++: QDomElement::setAttributeNodeNS(const class QDomAttr &) --> class QDomAttr", pybind11::arg("newAttr"));
		cl.def("elementsByTagNameNS", (class QDomNodeList (QDomElement::*)(const class QString &, const class QString &) const) &QDomElement::elementsByTagNameNS, "C++: QDomElement::elementsByTagNameNS(const class QString &, const class QString &) const --> class QDomNodeList", pybind11::arg("nsURI"), pybind11::arg("localName"));
		cl.def("hasAttributeNS", (bool (QDomElement::*)(const class QString &, const class QString &) const) &QDomElement::hasAttributeNS, "C++: QDomElement::hasAttributeNS(const class QString &, const class QString &) const --> bool", pybind11::arg("nsURI"), pybind11::arg("localName"));
		cl.def("tagName", (class QString (QDomElement::*)() const) &QDomElement::tagName, "C++: QDomElement::tagName() const --> class QString");
		cl.def("setTagName", (void (QDomElement::*)(const class QString &)) &QDomElement::setTagName, "C++: QDomElement::setTagName(const class QString &) --> void", pybind11::arg("name"));
		cl.def("attributes", (class QDomNamedNodeMap (QDomElement::*)() const) &QDomElement::attributes, "C++: QDomElement::attributes() const --> class QDomNamedNodeMap");
		cl.def("nodeType", (enum QDomNode::NodeType (QDomElement::*)() const) &QDomElement::nodeType, "C++: QDomElement::nodeType() const --> enum QDomNode::NodeType");
		cl.def("text", (class QString (QDomElement::*)() const) &QDomElement::text, "C++: QDomElement::text() const --> class QString");
	}
	std::cout << "B668_[QDomText] ";
	{ // QDomText file: line:544
		pybind11::class_<QDomText, std::shared_ptr<QDomText>, QDomCharacterData> cl(M(""), "QDomText", "");
		cl.def( pybind11::init( [](){ return new QDomText(); } ) );
		cl.def( pybind11::init( [](QDomText const &o){ return new QDomText(o); } ) );
		cl.def("assign", (class QDomText & (QDomText::*)(const class QDomText &)) &QDomText::operator=, "C++: QDomText::operator=(const class QDomText &) --> class QDomText &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("splitText", (class QDomText (QDomText::*)(int)) &QDomText::splitText, "C++: QDomText::splitText(int) --> class QDomText", pybind11::arg("offset"));
		cl.def("nodeType", (enum QDomNode::NodeType (QDomText::*)() const) &QDomText::nodeType, "C++: QDomText::nodeType() const --> enum QDomNode::NodeType");
	}
	std::cout << "B669_[QDomComment] ";
	{ // QDomComment file: line:565
		pybind11::class_<QDomComment, std::shared_ptr<QDomComment>, QDomCharacterData> cl(M(""), "QDomComment", "");
		cl.def( pybind11::init( [](){ return new QDomComment(); } ) );
		cl.def( pybind11::init( [](QDomComment const &o){ return new QDomComment(o); } ) );
		cl.def("assign", (class QDomComment & (QDomComment::*)(const class QDomComment &)) &QDomComment::operator=, "C++: QDomComment::operator=(const class QDomComment &) --> class QDomComment &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("nodeType", (enum QDomNode::NodeType (QDomComment::*)() const) &QDomComment::nodeType, "C++: QDomComment::nodeType() const --> enum QDomNode::NodeType");
	}
	std::cout << "B670_[QDomCDATASection] ";
	{ // QDomCDATASection file: line:582
		pybind11::class_<QDomCDATASection, std::shared_ptr<QDomCDATASection>, QDomText> cl(M(""), "QDomCDATASection", "");
		cl.def( pybind11::init( [](){ return new QDomCDATASection(); } ) );
		cl.def( pybind11::init( [](QDomCDATASection const &o){ return new QDomCDATASection(o); } ) );
		cl.def("assign", (class QDomCDATASection & (QDomCDATASection::*)(const class QDomCDATASection &)) &QDomCDATASection::operator=, "C++: QDomCDATASection::operator=(const class QDomCDATASection &) --> class QDomCDATASection &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("nodeType", (enum QDomNode::NodeType (QDomCDATASection::*)() const) &QDomCDATASection::nodeType, "C++: QDomCDATASection::nodeType() const --> enum QDomNode::NodeType");
	}
	std::cout << "B671_[QDomNotation] ";
	{ // QDomNotation file: line:599
		pybind11::class_<QDomNotation, std::shared_ptr<QDomNotation>, QDomNode> cl(M(""), "QDomNotation", "");
		cl.def( pybind11::init( [](){ return new QDomNotation(); } ) );
		cl.def( pybind11::init( [](QDomNotation const &o){ return new QDomNotation(o); } ) );
		cl.def("assign", (class QDomNotation & (QDomNotation::*)(const class QDomNotation &)) &QDomNotation::operator=, "C++: QDomNotation::operator=(const class QDomNotation &) --> class QDomNotation &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("publicId", (class QString (QDomNotation::*)() const) &QDomNotation::publicId, "C++: QDomNotation::publicId() const --> class QString");
		cl.def("systemId", (class QString (QDomNotation::*)() const) &QDomNotation::systemId, "C++: QDomNotation::systemId() const --> class QString");
		cl.def("nodeType", (enum QDomNode::NodeType (QDomNotation::*)() const) &QDomNotation::nodeType, "C++: QDomNotation::nodeType() const --> enum QDomNode::NodeType");
	}
	std::cout << "B672_[QDomEntity] ";
	{ // QDomEntity file: line:620
		pybind11::class_<QDomEntity, std::shared_ptr<QDomEntity>, QDomNode> cl(M(""), "QDomEntity", "");
		cl.def( pybind11::init( [](){ return new QDomEntity(); } ) );
		cl.def( pybind11::init( [](QDomEntity const &o){ return new QDomEntity(o); } ) );
		cl.def("assign", (class QDomEntity & (QDomEntity::*)(const class QDomEntity &)) &QDomEntity::operator=, "C++: QDomEntity::operator=(const class QDomEntity &) --> class QDomEntity &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("publicId", (class QString (QDomEntity::*)() const) &QDomEntity::publicId, "C++: QDomEntity::publicId() const --> class QString");
		cl.def("systemId", (class QString (QDomEntity::*)() const) &QDomEntity::systemId, "C++: QDomEntity::systemId() const --> class QString");
		cl.def("notationName", (class QString (QDomEntity::*)() const) &QDomEntity::notationName, "C++: QDomEntity::notationName() const --> class QString");
		cl.def("nodeType", (enum QDomNode::NodeType (QDomEntity::*)() const) &QDomEntity::nodeType, "C++: QDomEntity::nodeType() const --> enum QDomNode::NodeType");
	}
}
