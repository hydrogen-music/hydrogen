#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qiodevice.h> // 
#include <QtCore/qiodevice.h> // QIODevice
#include <QtCore/qlocale.h> // QLocale
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
#include <QtCore/qtextstream.h> // 
#include <QtCore/qtextstream.h> // QTextStream
#include <QtCore/qvector.h> // QVector
#include <cstdio> // _IO_FILE
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

void bind_unknown_unknown_14(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B658_[QDomImplementation] ";
	{ // QDomImplementation file: line:96
		pybind11::class_<QDomImplementation, std::shared_ptr<QDomImplementation>> cl(M(""), "QDomImplementation", "");
		cl.def( pybind11::init( [](){ return new QDomImplementation(); } ) );
		cl.def( pybind11::init( [](QDomImplementation const &o){ return new QDomImplementation(o); } ) );

		pybind11::enum_<QDomImplementation::InvalidDataPolicy>(cl, "InvalidDataPolicy", pybind11::arithmetic(), "")
			.value("AcceptInvalidChars", QDomImplementation::AcceptInvalidChars)
			.value("DropInvalidChars", QDomImplementation::DropInvalidChars)
			.value("ReturnNullNode", QDomImplementation::ReturnNullNode)
			.export_values();

		cl.def("assign", (class QDomImplementation & (QDomImplementation::*)(const class QDomImplementation &)) &QDomImplementation::operator=, "C++: QDomImplementation::operator=(const class QDomImplementation &) --> class QDomImplementation &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("__eq__", (bool (QDomImplementation::*)(const class QDomImplementation &) const) &QDomImplementation::operator==, "C++: QDomImplementation::operator==(const class QDomImplementation &) const --> bool", pybind11::arg(""));
		cl.def("__ne__", (bool (QDomImplementation::*)(const class QDomImplementation &) const) &QDomImplementation::operator!=, "C++: QDomImplementation::operator!=(const class QDomImplementation &) const --> bool", pybind11::arg(""));
		cl.def("hasFeature", (bool (QDomImplementation::*)(const class QString &, const class QString &) const) &QDomImplementation::hasFeature, "C++: QDomImplementation::hasFeature(const class QString &, const class QString &) const --> bool", pybind11::arg("feature"), pybind11::arg("version"));
		cl.def("createDocumentType", (class QDomDocumentType (QDomImplementation::*)(const class QString &, const class QString &, const class QString &)) &QDomImplementation::createDocumentType, "C++: QDomImplementation::createDocumentType(const class QString &, const class QString &, const class QString &) --> class QDomDocumentType", pybind11::arg("qName"), pybind11::arg("publicId"), pybind11::arg("systemId"));
		cl.def("createDocument", (class QDomDocument (QDomImplementation::*)(const class QString &, const class QString &, const class QDomDocumentType &)) &QDomImplementation::createDocument, "C++: QDomImplementation::createDocument(const class QString &, const class QString &, const class QDomDocumentType &) --> class QDomDocument", pybind11::arg("nsURI"), pybind11::arg("qName"), pybind11::arg("doctype"));
		cl.def_static("invalidDataPolicy", (enum QDomImplementation::InvalidDataPolicy (*)()) &QDomImplementation::invalidDataPolicy, "C++: QDomImplementation::invalidDataPolicy() --> enum QDomImplementation::InvalidDataPolicy");
		cl.def_static("setInvalidDataPolicy", (void (*)(enum QDomImplementation::InvalidDataPolicy)) &QDomImplementation::setInvalidDataPolicy, "C++: QDomImplementation::setInvalidDataPolicy(enum QDomImplementation::InvalidDataPolicy) --> void", pybind11::arg("policy"));
		cl.def("isNull", (bool (QDomImplementation::*)()) &QDomImplementation::isNull, "C++: QDomImplementation::isNull() --> bool");
	}
	std::cout << "B659_[QDomNode] ";
	{ // QDomNode file: line:125
		pybind11::class_<QDomNode, std::shared_ptr<QDomNode>> cl(M(""), "QDomNode", "");
		cl.def( pybind11::init( [](){ return new QDomNode(); } ) );
		cl.def( pybind11::init( [](QDomNode const &o){ return new QDomNode(o); } ) );

		pybind11::enum_<QDomNode::NodeType>(cl, "NodeType", pybind11::arithmetic(), "")
			.value("ElementNode", QDomNode::ElementNode)
			.value("AttributeNode", QDomNode::AttributeNode)
			.value("TextNode", QDomNode::TextNode)
			.value("CDATASectionNode", QDomNode::CDATASectionNode)
			.value("EntityReferenceNode", QDomNode::EntityReferenceNode)
			.value("EntityNode", QDomNode::EntityNode)
			.value("ProcessingInstructionNode", QDomNode::ProcessingInstructionNode)
			.value("CommentNode", QDomNode::CommentNode)
			.value("DocumentNode", QDomNode::DocumentNode)
			.value("DocumentTypeNode", QDomNode::DocumentTypeNode)
			.value("DocumentFragmentNode", QDomNode::DocumentFragmentNode)
			.value("NotationNode", QDomNode::NotationNode)
			.value("BaseNode", QDomNode::BaseNode)
			.value("CharacterDataNode", QDomNode::CharacterDataNode)
			.export_values();


		pybind11::enum_<QDomNode::EncodingPolicy>(cl, "EncodingPolicy", pybind11::arithmetic(), "")
			.value("EncodingFromDocument", QDomNode::EncodingFromDocument)
			.value("EncodingFromTextStream", QDomNode::EncodingFromTextStream)
			.export_values();

		cl.def("assign", (class QDomNode & (QDomNode::*)(const class QDomNode &)) &QDomNode::operator=, "C++: QDomNode::operator=(const class QDomNode &) --> class QDomNode &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("__eq__", (bool (QDomNode::*)(const class QDomNode &) const) &QDomNode::operator==, "C++: QDomNode::operator==(const class QDomNode &) const --> bool", pybind11::arg(""));
		cl.def("__ne__", (bool (QDomNode::*)(const class QDomNode &) const) &QDomNode::operator!=, "C++: QDomNode::operator!=(const class QDomNode &) const --> bool", pybind11::arg(""));
		cl.def("insertBefore", (class QDomNode (QDomNode::*)(const class QDomNode &, const class QDomNode &)) &QDomNode::insertBefore, "C++: QDomNode::insertBefore(const class QDomNode &, const class QDomNode &) --> class QDomNode", pybind11::arg("newChild"), pybind11::arg("refChild"));
		cl.def("insertAfter", (class QDomNode (QDomNode::*)(const class QDomNode &, const class QDomNode &)) &QDomNode::insertAfter, "C++: QDomNode::insertAfter(const class QDomNode &, const class QDomNode &) --> class QDomNode", pybind11::arg("newChild"), pybind11::arg("refChild"));
		cl.def("replaceChild", (class QDomNode (QDomNode::*)(const class QDomNode &, const class QDomNode &)) &QDomNode::replaceChild, "C++: QDomNode::replaceChild(const class QDomNode &, const class QDomNode &) --> class QDomNode", pybind11::arg("newChild"), pybind11::arg("oldChild"));
		cl.def("removeChild", (class QDomNode (QDomNode::*)(const class QDomNode &)) &QDomNode::removeChild, "C++: QDomNode::removeChild(const class QDomNode &) --> class QDomNode", pybind11::arg("oldChild"));
		cl.def("appendChild", (class QDomNode (QDomNode::*)(const class QDomNode &)) &QDomNode::appendChild, "C++: QDomNode::appendChild(const class QDomNode &) --> class QDomNode", pybind11::arg("newChild"));
		cl.def("hasChildNodes", (bool (QDomNode::*)() const) &QDomNode::hasChildNodes, "C++: QDomNode::hasChildNodes() const --> bool");
		cl.def("cloneNode", [](QDomNode const &o) -> QDomNode { return o.cloneNode(); }, "");
		cl.def("cloneNode", (class QDomNode (QDomNode::*)(bool) const) &QDomNode::cloneNode, "C++: QDomNode::cloneNode(bool) const --> class QDomNode", pybind11::arg("deep"));
		cl.def("normalize", (void (QDomNode::*)()) &QDomNode::normalize, "C++: QDomNode::normalize() --> void");
		cl.def("isSupported", (bool (QDomNode::*)(const class QString &, const class QString &) const) &QDomNode::isSupported, "C++: QDomNode::isSupported(const class QString &, const class QString &) const --> bool", pybind11::arg("feature"), pybind11::arg("version"));
		cl.def("nodeName", (class QString (QDomNode::*)() const) &QDomNode::nodeName, "C++: QDomNode::nodeName() const --> class QString");
		cl.def("nodeType", (enum QDomNode::NodeType (QDomNode::*)() const) &QDomNode::nodeType, "C++: QDomNode::nodeType() const --> enum QDomNode::NodeType");
		cl.def("parentNode", (class QDomNode (QDomNode::*)() const) &QDomNode::parentNode, "C++: QDomNode::parentNode() const --> class QDomNode");
		cl.def("childNodes", (class QDomNodeList (QDomNode::*)() const) &QDomNode::childNodes, "C++: QDomNode::childNodes() const --> class QDomNodeList");
		cl.def("firstChild", (class QDomNode (QDomNode::*)() const) &QDomNode::firstChild, "C++: QDomNode::firstChild() const --> class QDomNode");
		cl.def("lastChild", (class QDomNode (QDomNode::*)() const) &QDomNode::lastChild, "C++: QDomNode::lastChild() const --> class QDomNode");
		cl.def("previousSibling", (class QDomNode (QDomNode::*)() const) &QDomNode::previousSibling, "C++: QDomNode::previousSibling() const --> class QDomNode");
		cl.def("nextSibling", (class QDomNode (QDomNode::*)() const) &QDomNode::nextSibling, "C++: QDomNode::nextSibling() const --> class QDomNode");
		cl.def("attributes", (class QDomNamedNodeMap (QDomNode::*)() const) &QDomNode::attributes, "C++: QDomNode::attributes() const --> class QDomNamedNodeMap");
		cl.def("ownerDocument", (class QDomDocument (QDomNode::*)() const) &QDomNode::ownerDocument, "C++: QDomNode::ownerDocument() const --> class QDomDocument");
		cl.def("namespaceURI", (class QString (QDomNode::*)() const) &QDomNode::namespaceURI, "C++: QDomNode::namespaceURI() const --> class QString");
		cl.def("localName", (class QString (QDomNode::*)() const) &QDomNode::localName, "C++: QDomNode::localName() const --> class QString");
		cl.def("hasAttributes", (bool (QDomNode::*)() const) &QDomNode::hasAttributes, "C++: QDomNode::hasAttributes() const --> bool");
		cl.def("nodeValue", (class QString (QDomNode::*)() const) &QDomNode::nodeValue, "C++: QDomNode::nodeValue() const --> class QString");
		cl.def("setNodeValue", (void (QDomNode::*)(const class QString &)) &QDomNode::setNodeValue, "C++: QDomNode::setNodeValue(const class QString &) --> void", pybind11::arg(""));
		cl.def("prefix", (class QString (QDomNode::*)() const) &QDomNode::prefix, "C++: QDomNode::prefix() const --> class QString");
		cl.def("setPrefix", (void (QDomNode::*)(const class QString &)) &QDomNode::setPrefix, "C++: QDomNode::setPrefix(const class QString &) --> void", pybind11::arg("pre"));
		cl.def("isAttr", (bool (QDomNode::*)() const) &QDomNode::isAttr, "C++: QDomNode::isAttr() const --> bool");
		cl.def("isCDATASection", (bool (QDomNode::*)() const) &QDomNode::isCDATASection, "C++: QDomNode::isCDATASection() const --> bool");
		cl.def("isDocumentFragment", (bool (QDomNode::*)() const) &QDomNode::isDocumentFragment, "C++: QDomNode::isDocumentFragment() const --> bool");
		cl.def("isDocument", (bool (QDomNode::*)() const) &QDomNode::isDocument, "C++: QDomNode::isDocument() const --> bool");
		cl.def("isDocumentType", (bool (QDomNode::*)() const) &QDomNode::isDocumentType, "C++: QDomNode::isDocumentType() const --> bool");
		cl.def("isElement", (bool (QDomNode::*)() const) &QDomNode::isElement, "C++: QDomNode::isElement() const --> bool");
		cl.def("isEntityReference", (bool (QDomNode::*)() const) &QDomNode::isEntityReference, "C++: QDomNode::isEntityReference() const --> bool");
		cl.def("isText", (bool (QDomNode::*)() const) &QDomNode::isText, "C++: QDomNode::isText() const --> bool");
		cl.def("isEntity", (bool (QDomNode::*)() const) &QDomNode::isEntity, "C++: QDomNode::isEntity() const --> bool");
		cl.def("isNotation", (bool (QDomNode::*)() const) &QDomNode::isNotation, "C++: QDomNode::isNotation() const --> bool");
		cl.def("isProcessingInstruction", (bool (QDomNode::*)() const) &QDomNode::isProcessingInstruction, "C++: QDomNode::isProcessingInstruction() const --> bool");
		cl.def("isCharacterData", (bool (QDomNode::*)() const) &QDomNode::isCharacterData, "C++: QDomNode::isCharacterData() const --> bool");
		cl.def("isComment", (bool (QDomNode::*)() const) &QDomNode::isComment, "C++: QDomNode::isComment() const --> bool");
		cl.def("namedItem", (class QDomNode (QDomNode::*)(const class QString &) const) &QDomNode::namedItem, "Shortcut to avoid dealing with QDomNodeList\n all the time.\n\nC++: QDomNode::namedItem(const class QString &) const --> class QDomNode", pybind11::arg("name"));
		cl.def("isNull", (bool (QDomNode::*)() const) &QDomNode::isNull, "C++: QDomNode::isNull() const --> bool");
		cl.def("clear", (void (QDomNode::*)()) &QDomNode::clear, "C++: QDomNode::clear() --> void");
		cl.def("toAttr", (class QDomAttr (QDomNode::*)() const) &QDomNode::toAttr, "C++: QDomNode::toAttr() const --> class QDomAttr");
		cl.def("toCDATASection", (class QDomCDATASection (QDomNode::*)() const) &QDomNode::toCDATASection, "C++: QDomNode::toCDATASection() const --> class QDomCDATASection");
		cl.def("toDocumentFragment", (class QDomDocumentFragment (QDomNode::*)() const) &QDomNode::toDocumentFragment, "C++: QDomNode::toDocumentFragment() const --> class QDomDocumentFragment");
		cl.def("toDocument", (class QDomDocument (QDomNode::*)() const) &QDomNode::toDocument, "C++: QDomNode::toDocument() const --> class QDomDocument");
		cl.def("toDocumentType", (class QDomDocumentType (QDomNode::*)() const) &QDomNode::toDocumentType, "C++: QDomNode::toDocumentType() const --> class QDomDocumentType");
		cl.def("toElement", (class QDomElement (QDomNode::*)() const) &QDomNode::toElement, "C++: QDomNode::toElement() const --> class QDomElement");
		cl.def("toEntityReference", (class QDomEntityReference (QDomNode::*)() const) &QDomNode::toEntityReference, "C++: QDomNode::toEntityReference() const --> class QDomEntityReference");
		cl.def("toText", (class QDomText (QDomNode::*)() const) &QDomNode::toText, "C++: QDomNode::toText() const --> class QDomText");
		cl.def("toEntity", (class QDomEntity (QDomNode::*)() const) &QDomNode::toEntity, "C++: QDomNode::toEntity() const --> class QDomEntity");
		cl.def("toNotation", (class QDomNotation (QDomNode::*)() const) &QDomNode::toNotation, "C++: QDomNode::toNotation() const --> class QDomNotation");
		cl.def("toProcessingInstruction", (class QDomProcessingInstruction (QDomNode::*)() const) &QDomNode::toProcessingInstruction, "C++: QDomNode::toProcessingInstruction() const --> class QDomProcessingInstruction");
		cl.def("toCharacterData", (class QDomCharacterData (QDomNode::*)() const) &QDomNode::toCharacterData, "C++: QDomNode::toCharacterData() const --> class QDomCharacterData");
		cl.def("toComment", (class QDomComment (QDomNode::*)() const) &QDomNode::toComment, "C++: QDomNode::toComment() const --> class QDomComment");
		cl.def("save", [](QDomNode const &o, class QTextStream & a0, int const & a1) -> void { return o.save(a0, a1); }, "", pybind11::arg(""), pybind11::arg(""));
		cl.def("save", (void (QDomNode::*)(class QTextStream &, int, enum QDomNode::EncodingPolicy) const) &QDomNode::save, "C++: QDomNode::save(class QTextStream &, int, enum QDomNode::EncodingPolicy) const --> void", pybind11::arg(""), pybind11::arg(""), pybind11::arg(""));
		cl.def("firstChildElement", [](QDomNode const &o) -> QDomElement { return o.firstChildElement(); }, "");
		cl.def("firstChildElement", (class QDomElement (QDomNode::*)(const class QString &) const) &QDomNode::firstChildElement, "C++: QDomNode::firstChildElement(const class QString &) const --> class QDomElement", pybind11::arg("tagName"));
		cl.def("lastChildElement", [](QDomNode const &o) -> QDomElement { return o.lastChildElement(); }, "");
		cl.def("lastChildElement", (class QDomElement (QDomNode::*)(const class QString &) const) &QDomNode::lastChildElement, "C++: QDomNode::lastChildElement(const class QString &) const --> class QDomElement", pybind11::arg("tagName"));
		cl.def("previousSiblingElement", [](QDomNode const &o) -> QDomElement { return o.previousSiblingElement(); }, "");
		cl.def("previousSiblingElement", (class QDomElement (QDomNode::*)(const class QString &) const) &QDomNode::previousSiblingElement, "C++: QDomNode::previousSiblingElement(const class QString &) const --> class QDomElement", pybind11::arg("tagName"));
		cl.def("nextSiblingElement", [](QDomNode const &o) -> QDomElement { return o.nextSiblingElement(); }, "");
		cl.def("nextSiblingElement", (class QDomElement (QDomNode::*)(const class QString &) const) &QDomNode::nextSiblingElement, "C++: QDomNode::nextSiblingElement(const class QString &) const --> class QDomElement", pybind11::arg("taName"));
		cl.def("lineNumber", (int (QDomNode::*)() const) &QDomNode::lineNumber, "C++: QDomNode::lineNumber() const --> int");
		cl.def("columnNumber", (int (QDomNode::*)() const) &QDomNode::columnNumber, "C++: QDomNode::columnNumber() const --> int");
	}
	std::cout << "B660_[QDomNodeList] ";
	{ // QDomNodeList file: line:249
		pybind11::class_<QDomNodeList, std::shared_ptr<QDomNodeList>> cl(M(""), "QDomNodeList", "");
		cl.def( pybind11::init( [](){ return new QDomNodeList(); } ) );
		cl.def( pybind11::init( [](QDomNodeList const &o){ return new QDomNodeList(o); } ) );
		cl.def("assign", (class QDomNodeList & (QDomNodeList::*)(const class QDomNodeList &)) &QDomNodeList::operator=, "C++: QDomNodeList::operator=(const class QDomNodeList &) --> class QDomNodeList &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("__eq__", (bool (QDomNodeList::*)(const class QDomNodeList &) const) &QDomNodeList::operator==, "C++: QDomNodeList::operator==(const class QDomNodeList &) const --> bool", pybind11::arg(""));
		cl.def("__ne__", (bool (QDomNodeList::*)(const class QDomNodeList &) const) &QDomNodeList::operator!=, "C++: QDomNodeList::operator!=(const class QDomNodeList &) const --> bool", pybind11::arg(""));
		cl.def("item", (class QDomNode (QDomNodeList::*)(int) const) &QDomNodeList::item, "C++: QDomNodeList::item(int) const --> class QDomNode", pybind11::arg("index"));
		cl.def("at", (class QDomNode (QDomNodeList::*)(int) const) &QDomNodeList::at, "C++: QDomNodeList::at(int) const --> class QDomNode", pybind11::arg("index"));
		cl.def("length", (int (QDomNodeList::*)() const) &QDomNodeList::length, "C++: QDomNodeList::length() const --> int");
		cl.def("count", (int (QDomNodeList::*)() const) &QDomNodeList::count, "C++: QDomNodeList::count() const --> int");
		cl.def("size", (int (QDomNodeList::*)() const) &QDomNodeList::size, "C++: QDomNodeList::size() const --> int");
		cl.def("isEmpty", (bool (QDomNodeList::*)() const) &QDomNodeList::isEmpty, "C++: QDomNodeList::isEmpty() const --> bool");
	}
}
