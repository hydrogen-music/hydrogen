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

void bind_unknown_unknown_17(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B673_[QDomEntityReference] ";
	{ // QDomEntityReference file: line:641
		pybind11::class_<QDomEntityReference, std::shared_ptr<QDomEntityReference>, QDomNode> cl(M(""), "QDomEntityReference", "");
		cl.def( pybind11::init( [](){ return new QDomEntityReference(); } ) );
		cl.def( pybind11::init( [](QDomEntityReference const &o){ return new QDomEntityReference(o); } ) );
		cl.def("assign", (class QDomEntityReference & (QDomEntityReference::*)(const class QDomEntityReference &)) &QDomEntityReference::operator=, "C++: QDomEntityReference::operator=(const class QDomEntityReference &) --> class QDomEntityReference &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("nodeType", (enum QDomNode::NodeType (QDomEntityReference::*)() const) &QDomEntityReference::nodeType, "C++: QDomEntityReference::nodeType() const --> enum QDomNode::NodeType");
	}
	std::cout << "B674_[QDomProcessingInstruction] ";
	{ // QDomProcessingInstruction file: line:658
		pybind11::class_<QDomProcessingInstruction, std::shared_ptr<QDomProcessingInstruction>, QDomNode> cl(M(""), "QDomProcessingInstruction", "");
		cl.def( pybind11::init( [](){ return new QDomProcessingInstruction(); } ) );
		cl.def( pybind11::init( [](QDomProcessingInstruction const &o){ return new QDomProcessingInstruction(o); } ) );
		cl.def("assign", (class QDomProcessingInstruction & (QDomProcessingInstruction::*)(const class QDomProcessingInstruction &)) &QDomProcessingInstruction::operator=, "C++: QDomProcessingInstruction::operator=(const class QDomProcessingInstruction &) --> class QDomProcessingInstruction &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("target", (class QString (QDomProcessingInstruction::*)() const) &QDomProcessingInstruction::target, "C++: QDomProcessingInstruction::target() const --> class QString");
		cl.def("data", (class QString (QDomProcessingInstruction::*)() const) &QDomProcessingInstruction::data, "C++: QDomProcessingInstruction::data() const --> class QString");
		cl.def("setData", (void (QDomProcessingInstruction::*)(const class QString &)) &QDomProcessingInstruction::setData, "C++: QDomProcessingInstruction::setData(const class QString &) --> void", pybind11::arg("d"));
		cl.def("nodeType", (enum QDomNode::NodeType (QDomProcessingInstruction::*)() const) &QDomProcessingInstruction::nodeType, "C++: QDomProcessingInstruction::nodeType() const --> enum QDomNode::NodeType");
	}
}
