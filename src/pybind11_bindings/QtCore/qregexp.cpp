#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qchar.h> // QLatin1Char
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qlist.h> // QList
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
#include <QtCore/qregexp.h> // 
#include <QtCore/qregexp.h> // QRegExp
#include <QtCore/qregularexpression.h> // 
#include <QtCore/qregularexpression.h> // QRegularExpression
#include <QtCore/qregularexpression.h> // QRegularExpressionMatch
#include <QtCore/qregularexpression.h> // QRegularExpressionMatchIterator
#include <QtCore/qstring.h> // 
#include <QtCore/qstring.h> // QCharRef
#include <QtCore/qstring.h> // QLatin1String
#include <QtCore/qstring.h> // QString
#include <QtCore/qstring.h> // QStringRef
#include <QtCore/qstringlist.h> // QListSpecialMethods
#include <QtCore/qstringlist.h> // QStringList
#include <QtCore/qstringliteral.h> // QStringDataPtr
#include <QtCore/qstringmatcher.h> // QStringMatcher
#include <QtCore/qstringview.h> // QStringView
#include <QtCore/qvector.h> // QVector
#include <iostream> // --trace
#include <iterator> // std::reverse_iterator
#include <list> // std::list
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

void bind_QtCore_qregexp(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B311_[QRegExp] ";
	{ // QRegExp file:QtCore/qregexp.h line:58
		pybind11::class_<QRegExp, std::shared_ptr<QRegExp>> cl(M(""), "QRegExp", "");
		cl.def( pybind11::init( [](){ return new QRegExp(); } ) );
		cl.def( pybind11::init( [](const class QString & a0){ return new QRegExp(a0); } ), "doc" , pybind11::arg("pattern"));
		cl.def( pybind11::init( [](const class QString & a0, enum Qt::CaseSensitivity const & a1){ return new QRegExp(a0, a1); } ), "doc" , pybind11::arg("pattern"), pybind11::arg("cs"));
		cl.def( pybind11::init<const class QString &, enum Qt::CaseSensitivity, enum QRegExp::PatternSyntax>(), pybind11::arg("pattern"), pybind11::arg("cs"), pybind11::arg("syntax") );

		cl.def( pybind11::init( [](QRegExp const &o){ return new QRegExp(o); } ) );

		pybind11::enum_<QRegExp::PatternSyntax>(cl, "PatternSyntax", pybind11::arithmetic(), "")
			.value("RegExp", QRegExp::RegExp)
			.value("Wildcard", QRegExp::Wildcard)
			.value("FixedString", QRegExp::FixedString)
			.value("RegExp2", QRegExp::RegExp2)
			.value("WildcardUnix", QRegExp::WildcardUnix)
			.value("W3CXmlSchema11", QRegExp::W3CXmlSchema11)
			.export_values();


		pybind11::enum_<QRegExp::CaretMode>(cl, "CaretMode", pybind11::arithmetic(), "")
			.value("CaretAtZero", QRegExp::CaretAtZero)
			.value("CaretAtOffset", QRegExp::CaretAtOffset)
			.value("CaretWontMatch", QRegExp::CaretWontMatch)
			.export_values();

		cl.def("assign", (class QRegExp & (QRegExp::*)(const class QRegExp &)) &QRegExp::operator=, "C++: QRegExp::operator=(const class QRegExp &) --> class QRegExp &", pybind11::return_value_policy::automatic, pybind11::arg("rx"));
		cl.def("swap", (void (QRegExp::*)(class QRegExp &)) &QRegExp::swap, "C++: QRegExp::swap(class QRegExp &) --> void", pybind11::arg("other"));
		cl.def("__eq__", (bool (QRegExp::*)(const class QRegExp &) const) &QRegExp::operator==, "C++: QRegExp::operator==(const class QRegExp &) const --> bool", pybind11::arg("rx"));
		cl.def("__ne__", (bool (QRegExp::*)(const class QRegExp &) const) &QRegExp::operator!=, "C++: QRegExp::operator!=(const class QRegExp &) const --> bool", pybind11::arg("rx"));
		cl.def("isEmpty", (bool (QRegExp::*)() const) &QRegExp::isEmpty, "C++: QRegExp::isEmpty() const --> bool");
		cl.def("isValid", (bool (QRegExp::*)() const) &QRegExp::isValid, "C++: QRegExp::isValid() const --> bool");
		cl.def("pattern", (class QString (QRegExp::*)() const) &QRegExp::pattern, "C++: QRegExp::pattern() const --> class QString");
		cl.def("setPattern", (void (QRegExp::*)(const class QString &)) &QRegExp::setPattern, "C++: QRegExp::setPattern(const class QString &) --> void", pybind11::arg("pattern"));
		cl.def("caseSensitivity", (enum Qt::CaseSensitivity (QRegExp::*)() const) &QRegExp::caseSensitivity, "C++: QRegExp::caseSensitivity() const --> enum Qt::CaseSensitivity");
		cl.def("setCaseSensitivity", (void (QRegExp::*)(enum Qt::CaseSensitivity)) &QRegExp::setCaseSensitivity, "C++: QRegExp::setCaseSensitivity(enum Qt::CaseSensitivity) --> void", pybind11::arg("cs"));
		cl.def("patternSyntax", (enum QRegExp::PatternSyntax (QRegExp::*)() const) &QRegExp::patternSyntax, "C++: QRegExp::patternSyntax() const --> enum QRegExp::PatternSyntax");
		cl.def("setPatternSyntax", (void (QRegExp::*)(enum QRegExp::PatternSyntax)) &QRegExp::setPatternSyntax, "C++: QRegExp::setPatternSyntax(enum QRegExp::PatternSyntax) --> void", pybind11::arg("syntax"));
		cl.def("isMinimal", (bool (QRegExp::*)() const) &QRegExp::isMinimal, "C++: QRegExp::isMinimal() const --> bool");
		cl.def("setMinimal", (void (QRegExp::*)(bool)) &QRegExp::setMinimal, "C++: QRegExp::setMinimal(bool) --> void", pybind11::arg("minimal"));
		cl.def("exactMatch", (bool (QRegExp::*)(const class QString &) const) &QRegExp::exactMatch, "C++: QRegExp::exactMatch(const class QString &) const --> bool", pybind11::arg("str"));
		cl.def("indexIn", [](QRegExp const &o, const class QString & a0) -> int { return o.indexIn(a0); }, "", pybind11::arg("str"));
		cl.def("indexIn", [](QRegExp const &o, const class QString & a0, int const & a1) -> int { return o.indexIn(a0, a1); }, "", pybind11::arg("str"), pybind11::arg("offset"));
		cl.def("indexIn", (int (QRegExp::*)(const class QString &, int, enum QRegExp::CaretMode) const) &QRegExp::indexIn, "C++: QRegExp::indexIn(const class QString &, int, enum QRegExp::CaretMode) const --> int", pybind11::arg("str"), pybind11::arg("offset"), pybind11::arg("caretMode"));
		cl.def("lastIndexIn", [](QRegExp const &o, const class QString & a0) -> int { return o.lastIndexIn(a0); }, "", pybind11::arg("str"));
		cl.def("lastIndexIn", [](QRegExp const &o, const class QString & a0, int const & a1) -> int { return o.lastIndexIn(a0, a1); }, "", pybind11::arg("str"), pybind11::arg("offset"));
		cl.def("lastIndexIn", (int (QRegExp::*)(const class QString &, int, enum QRegExp::CaretMode) const) &QRegExp::lastIndexIn, "C++: QRegExp::lastIndexIn(const class QString &, int, enum QRegExp::CaretMode) const --> int", pybind11::arg("str"), pybind11::arg("offset"), pybind11::arg("caretMode"));
		cl.def("matchedLength", (int (QRegExp::*)() const) &QRegExp::matchedLength, "C++: QRegExp::matchedLength() const --> int");
		cl.def("captureCount", (int (QRegExp::*)() const) &QRegExp::captureCount, "C++: QRegExp::captureCount() const --> int");
		cl.def("cap", [](QRegExp &o) -> QString { return o.cap(); }, "");
		cl.def("cap", (class QString (QRegExp::*)(int)) &QRegExp::cap, "C++: QRegExp::cap(int) --> class QString", pybind11::arg("nth"));
		cl.def("pos", [](QRegExp &o) -> int { return o.pos(); }, "");
		cl.def("pos", (int (QRegExp::*)(int)) &QRegExp::pos, "C++: QRegExp::pos(int) --> int", pybind11::arg("nth"));
		cl.def("errorString", (class QString (QRegExp::*)()) &QRegExp::errorString, "C++: QRegExp::errorString() --> class QString");
		cl.def_static("escape", (class QString (*)(const class QString &)) &QRegExp::escape, "C++: QRegExp::escape(const class QString &) --> class QString", pybind11::arg("str"));
	}
	std::cout << "B312_[QTypeInfo<QRegExp>] ";
	std::cout << "B313_[QStringMatcher] ";
	std::cout << "B314_[QListSpecialMethods<QString>] ";
	std::cout << "B315_[QStringList] ";
	std::cout << "B316_[QTypeInfo<QStringList>] ";
}
