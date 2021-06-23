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
#include <QtCore/qvector.h> // QVector
#include <QtCore/qxmlstream.h> // +include_for_class
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

void bind_unknown_unknown_13(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B637_[QXmlStreamReader] ";
	{ // QXmlStreamReader file: line:336
		pybind11::class_<QXmlStreamReader, std::shared_ptr<QXmlStreamReader>> cl(M(""), "QXmlStreamReader", "");
		cl.def( pybind11::init( [](){ return new QXmlStreamReader(); } ) );
		cl.def( pybind11::init<class QIODevice *>(), pybind11::arg("device") );

		cl.def( pybind11::init<const class QString &>(), pybind11::arg("data") );

		cl.def( pybind11::init<const char *>(), pybind11::arg("data") );


		pybind11::enum_<QXmlStreamReader::TokenType>(cl, "TokenType", pybind11::arithmetic(), "")
			.value("NoToken", QXmlStreamReader::NoToken)
			.value("Invalid", QXmlStreamReader::Invalid)
			.value("StartDocument", QXmlStreamReader::StartDocument)
			.value("EndDocument", QXmlStreamReader::EndDocument)
			.value("StartElement", QXmlStreamReader::StartElement)
			.value("EndElement", QXmlStreamReader::EndElement)
			.value("Characters", QXmlStreamReader::Characters)
			.value("Comment", QXmlStreamReader::Comment)
			.value("DTD", QXmlStreamReader::DTD)
			.value("EntityReference", QXmlStreamReader::EntityReference)
			.value("ProcessingInstruction", QXmlStreamReader::ProcessingInstruction)
			.export_values();


		pybind11::enum_<QXmlStreamReader::ReadElementTextBehaviour>(cl, "ReadElementTextBehaviour", pybind11::arithmetic(), "")
			.value("ErrorOnUnexpectedElement", QXmlStreamReader::ErrorOnUnexpectedElement)
			.value("IncludeChildElements", QXmlStreamReader::IncludeChildElements)
			.value("SkipChildElements", QXmlStreamReader::SkipChildElements)
			.export_values();


		pybind11::enum_<QXmlStreamReader::Error>(cl, "Error", pybind11::arithmetic(), "")
			.value("NoError", QXmlStreamReader::NoError)
			.value("UnexpectedElementError", QXmlStreamReader::UnexpectedElementError)
			.value("CustomError", QXmlStreamReader::CustomError)
			.value("NotWellFormedError", QXmlStreamReader::NotWellFormedError)
			.value("PrematureEndOfDocumentError", QXmlStreamReader::PrematureEndOfDocumentError)
			.export_values();

		cl.def("setDevice", (void (QXmlStreamReader::*)(class QIODevice *)) &QXmlStreamReader::setDevice, "C++: QXmlStreamReader::setDevice(class QIODevice *) --> void", pybind11::arg("device"));
		cl.def("device", (class QIODevice * (QXmlStreamReader::*)() const) &QXmlStreamReader::device, "C++: QXmlStreamReader::device() const --> class QIODevice *", pybind11::return_value_policy::automatic);
		cl.def("addData", (void (QXmlStreamReader::*)(const class QString &)) &QXmlStreamReader::addData, "C++: QXmlStreamReader::addData(const class QString &) --> void", pybind11::arg("data"));
		cl.def("addData", (void (QXmlStreamReader::*)(const char *)) &QXmlStreamReader::addData, "C++: QXmlStreamReader::addData(const char *) --> void", pybind11::arg("data"));
		cl.def("clear", (void (QXmlStreamReader::*)()) &QXmlStreamReader::clear, "C++: QXmlStreamReader::clear() --> void");
		cl.def("atEnd", (bool (QXmlStreamReader::*)() const) &QXmlStreamReader::atEnd, "C++: QXmlStreamReader::atEnd() const --> bool");
		cl.def("readNext", (enum QXmlStreamReader::TokenType (QXmlStreamReader::*)()) &QXmlStreamReader::readNext, "C++: QXmlStreamReader::readNext() --> enum QXmlStreamReader::TokenType");
		cl.def("readNextStartElement", (bool (QXmlStreamReader::*)()) &QXmlStreamReader::readNextStartElement, "C++: QXmlStreamReader::readNextStartElement() --> bool");
		cl.def("skipCurrentElement", (void (QXmlStreamReader::*)()) &QXmlStreamReader::skipCurrentElement, "C++: QXmlStreamReader::skipCurrentElement() --> void");
		cl.def("tokenType", (enum QXmlStreamReader::TokenType (QXmlStreamReader::*)() const) &QXmlStreamReader::tokenType, "C++: QXmlStreamReader::tokenType() const --> enum QXmlStreamReader::TokenType");
		cl.def("tokenString", (class QString (QXmlStreamReader::*)() const) &QXmlStreamReader::tokenString, "C++: QXmlStreamReader::tokenString() const --> class QString");
		cl.def("setNamespaceProcessing", (void (QXmlStreamReader::*)(bool)) &QXmlStreamReader::setNamespaceProcessing, "C++: QXmlStreamReader::setNamespaceProcessing(bool) --> void", pybind11::arg(""));
		cl.def("namespaceProcessing", (bool (QXmlStreamReader::*)() const) &QXmlStreamReader::namespaceProcessing, "C++: QXmlStreamReader::namespaceProcessing() const --> bool");
		cl.def("isStartDocument", (bool (QXmlStreamReader::*)() const) &QXmlStreamReader::isStartDocument, "C++: QXmlStreamReader::isStartDocument() const --> bool");
		cl.def("isEndDocument", (bool (QXmlStreamReader::*)() const) &QXmlStreamReader::isEndDocument, "C++: QXmlStreamReader::isEndDocument() const --> bool");
		cl.def("isStartElement", (bool (QXmlStreamReader::*)() const) &QXmlStreamReader::isStartElement, "C++: QXmlStreamReader::isStartElement() const --> bool");
		cl.def("isEndElement", (bool (QXmlStreamReader::*)() const) &QXmlStreamReader::isEndElement, "C++: QXmlStreamReader::isEndElement() const --> bool");
		cl.def("isCharacters", (bool (QXmlStreamReader::*)() const) &QXmlStreamReader::isCharacters, "C++: QXmlStreamReader::isCharacters() const --> bool");
		cl.def("isWhitespace", (bool (QXmlStreamReader::*)() const) &QXmlStreamReader::isWhitespace, "C++: QXmlStreamReader::isWhitespace() const --> bool");
		cl.def("isCDATA", (bool (QXmlStreamReader::*)() const) &QXmlStreamReader::isCDATA, "C++: QXmlStreamReader::isCDATA() const --> bool");
		cl.def("isComment", (bool (QXmlStreamReader::*)() const) &QXmlStreamReader::isComment, "C++: QXmlStreamReader::isComment() const --> bool");
		cl.def("isDTD", (bool (QXmlStreamReader::*)() const) &QXmlStreamReader::isDTD, "C++: QXmlStreamReader::isDTD() const --> bool");
		cl.def("isEntityReference", (bool (QXmlStreamReader::*)() const) &QXmlStreamReader::isEntityReference, "C++: QXmlStreamReader::isEntityReference() const --> bool");
		cl.def("isProcessingInstruction", (bool (QXmlStreamReader::*)() const) &QXmlStreamReader::isProcessingInstruction, "C++: QXmlStreamReader::isProcessingInstruction() const --> bool");
		cl.def("isStandaloneDocument", (bool (QXmlStreamReader::*)() const) &QXmlStreamReader::isStandaloneDocument, "C++: QXmlStreamReader::isStandaloneDocument() const --> bool");
		cl.def("documentVersion", (class QStringRef (QXmlStreamReader::*)() const) &QXmlStreamReader::documentVersion, "C++: QXmlStreamReader::documentVersion() const --> class QStringRef");
		cl.def("documentEncoding", (class QStringRef (QXmlStreamReader::*)() const) &QXmlStreamReader::documentEncoding, "C++: QXmlStreamReader::documentEncoding() const --> class QStringRef");
		cl.def("lineNumber", (long long (QXmlStreamReader::*)() const) &QXmlStreamReader::lineNumber, "C++: QXmlStreamReader::lineNumber() const --> long long");
		cl.def("columnNumber", (long long (QXmlStreamReader::*)() const) &QXmlStreamReader::columnNumber, "C++: QXmlStreamReader::columnNumber() const --> long long");
		cl.def("characterOffset", (long long (QXmlStreamReader::*)() const) &QXmlStreamReader::characterOffset, "C++: QXmlStreamReader::characterOffset() const --> long long");
		cl.def("attributes", (class QXmlStreamAttributes (QXmlStreamReader::*)() const) &QXmlStreamReader::attributes, "C++: QXmlStreamReader::attributes() const --> class QXmlStreamAttributes");
		cl.def("readElementText", [](QXmlStreamReader &o) -> QString { return o.readElementText(); }, "");
		cl.def("readElementText", (class QString (QXmlStreamReader::*)(enum QXmlStreamReader::ReadElementTextBehaviour)) &QXmlStreamReader::readElementText, "C++: QXmlStreamReader::readElementText(enum QXmlStreamReader::ReadElementTextBehaviour) --> class QString", pybind11::arg("behaviour"));
		cl.def("name", (class QStringRef (QXmlStreamReader::*)() const) &QXmlStreamReader::name, "C++: QXmlStreamReader::name() const --> class QStringRef");
		cl.def("namespaceUri", (class QStringRef (QXmlStreamReader::*)() const) &QXmlStreamReader::namespaceUri, "C++: QXmlStreamReader::namespaceUri() const --> class QStringRef");
		cl.def("qualifiedName", (class QStringRef (QXmlStreamReader::*)() const) &QXmlStreamReader::qualifiedName, "C++: QXmlStreamReader::qualifiedName() const --> class QStringRef");
		cl.def("prefix", (class QStringRef (QXmlStreamReader::*)() const) &QXmlStreamReader::prefix, "C++: QXmlStreamReader::prefix() const --> class QStringRef");
		cl.def("processingInstructionTarget", (class QStringRef (QXmlStreamReader::*)() const) &QXmlStreamReader::processingInstructionTarget, "C++: QXmlStreamReader::processingInstructionTarget() const --> class QStringRef");
		cl.def("processingInstructionData", (class QStringRef (QXmlStreamReader::*)() const) &QXmlStreamReader::processingInstructionData, "C++: QXmlStreamReader::processingInstructionData() const --> class QStringRef");
		cl.def("text", (class QStringRef (QXmlStreamReader::*)() const) &QXmlStreamReader::text, "C++: QXmlStreamReader::text() const --> class QStringRef");
		cl.def("addExtraNamespaceDeclaration", (void (QXmlStreamReader::*)(const class QXmlStreamNamespaceDeclaration &)) &QXmlStreamReader::addExtraNamespaceDeclaration, "C++: QXmlStreamReader::addExtraNamespaceDeclaration(const class QXmlStreamNamespaceDeclaration &) --> void", pybind11::arg("extraNamespaceDeclaraction"));
		cl.def("dtdName", (class QStringRef (QXmlStreamReader::*)() const) &QXmlStreamReader::dtdName, "C++: QXmlStreamReader::dtdName() const --> class QStringRef");
		cl.def("dtdPublicId", (class QStringRef (QXmlStreamReader::*)() const) &QXmlStreamReader::dtdPublicId, "C++: QXmlStreamReader::dtdPublicId() const --> class QStringRef");
		cl.def("dtdSystemId", (class QStringRef (QXmlStreamReader::*)() const) &QXmlStreamReader::dtdSystemId, "C++: QXmlStreamReader::dtdSystemId() const --> class QStringRef");
		cl.def("entityExpansionLimit", (int (QXmlStreamReader::*)() const) &QXmlStreamReader::entityExpansionLimit, "C++: QXmlStreamReader::entityExpansionLimit() const --> int");
		cl.def("setEntityExpansionLimit", (void (QXmlStreamReader::*)(int)) &QXmlStreamReader::setEntityExpansionLimit, "C++: QXmlStreamReader::setEntityExpansionLimit(int) --> void", pybind11::arg("limit"));
		cl.def("raiseError", [](QXmlStreamReader &o) -> void { return o.raiseError(); }, "");
		cl.def("raiseError", (void (QXmlStreamReader::*)(const class QString &)) &QXmlStreamReader::raiseError, "C++: QXmlStreamReader::raiseError(const class QString &) --> void", pybind11::arg("message"));
		cl.def("errorString", (class QString (QXmlStreamReader::*)() const) &QXmlStreamReader::errorString, "C++: QXmlStreamReader::errorString() const --> class QString");
		cl.def("error", (enum QXmlStreamReader::Error (QXmlStreamReader::*)() const) &QXmlStreamReader::error, "C++: QXmlStreamReader::error() const --> enum QXmlStreamReader::Error");
		cl.def("hasError", (bool (QXmlStreamReader::*)() const) &QXmlStreamReader::hasError, "C++: QXmlStreamReader::hasError() const --> bool");
		cl.def("setEntityResolver", (void (QXmlStreamReader::*)(class QXmlStreamEntityResolver *)) &QXmlStreamReader::setEntityResolver, "C++: QXmlStreamReader::setEntityResolver(class QXmlStreamEntityResolver *) --> void", pybind11::arg("resolver"));
		cl.def("entityResolver", (class QXmlStreamEntityResolver * (QXmlStreamReader::*)() const) &QXmlStreamReader::entityResolver, "C++: QXmlStreamReader::entityResolver() const --> class QXmlStreamEntityResolver *", pybind11::return_value_policy::automatic);
	}
	std::cout << "B638_[QXmlStreamWriter] ";
}
