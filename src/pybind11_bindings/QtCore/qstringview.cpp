#include <QtCore/qbytearray.h> // 
#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qbytearray.h> // QByteArrayDataPtr
#include <QtCore/qbytearray.h> // QByteRef
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qchar.h> // QLatin1Char
#include <QtCore/qflags.h> // QFlag
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qlist.h> // QList
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
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
#include <vector> // std::vector
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

void bind_QtCore_qstringview(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B307_[QStringView] ";
	{ // QStringView file:QtCore/qstringview.h line:104
		pybind11::class_<QStringView, std::shared_ptr<QStringView>> cl(M(""), "QStringView", "");
		cl.def( pybind11::init( [](){ return new QStringView(); } ) );
		cl.def( pybind11::init<std::nullptr_t>(), pybind11::arg("") );

		cl.def( pybind11::init( [](QStringView const &o){ return new QStringView(o); } ) );
		cl.def("arg", (class QString (QStringView::*)(const class QString &, const class QString &) const) &QStringView::arg<const QString &, const QString &>, "C++: QStringView::arg(const class QString &, const class QString &) const --> class QString", pybind11::arg("args"), pybind11::arg("args"));
		cl.def("arg", (class QString (QStringView::*)(const class QString &, const class QString &, const class QString &) const) &QStringView::arg<const QString &, const QString &, const QString &>, "C++: QStringView::arg(const class QString &, const class QString &, const class QString &) const --> class QString", pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"));
		cl.def("arg", (class QString (QStringView::*)(const class QString &, const class QString &, const class QString &, const class QString &) const) &QStringView::arg<const QString &, const QString &, const QString &, const QString &>, "C++: QStringView::arg(const class QString &, const class QString &, const class QString &, const class QString &) const --> class QString", pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"));
		cl.def("arg", (class QString (QStringView::*)(const class QString &, const class QString &, const class QString &, const class QString &, const class QString &) const) &QStringView::arg<const QString &, const QString &, const QString &, const QString &, const QString &>, "C++: QStringView::arg(const class QString &, const class QString &, const class QString &, const class QString &, const class QString &) const --> class QString", pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"));
		cl.def("arg", (class QString (QStringView::*)(const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &) const) &QStringView::arg<const QString &, const QString &, const QString &, const QString &, const QString &, const QString &>, "C++: QStringView::arg(const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &) const --> class QString", pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"));
		cl.def("arg", (class QString (QStringView::*)(const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &) const) &QStringView::arg<const QString &, const QString &, const QString &, const QString &, const QString &, const QString &, const QString &>, "C++: QStringView::arg(const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &) const --> class QString", pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"));
		cl.def("arg", (class QString (QStringView::*)(const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &) const) &QStringView::arg<const QString &, const QString &, const QString &, const QString &, const QString &, const QString &, const QString &, const QString &>, "C++: QStringView::arg(const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &) const --> class QString", pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"));
		cl.def("arg", (class QString (QStringView::*)(const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &) const) &QStringView::arg<const QString &, const QString &, const QString &, const QString &, const QString &, const QString &, const QString &, const QString &, const QString &>, "C++: QStringView::arg(const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &, const class QString &) const --> class QString", pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"), pybind11::arg("args"));
		cl.def("toString", (class QString (QStringView::*)() const) &QStringView::toString, "C++: QStringView::toString() const --> class QString");
		cl.def("size", (long long (QStringView::*)() const) &QStringView::size, "C++: QStringView::size() const --> long long");
		cl.def("data", (const class QChar * (QStringView::*)() const) &QStringView::data, "C++: QStringView::data() const --> const class QChar *", pybind11::return_value_policy::automatic);
		cl.def("utf16", (const char16_t * (QStringView::*)() const) &QStringView::utf16, "C++: QStringView::utf16() const --> const char16_t *", pybind11::return_value_policy::automatic);
		cl.def("__getitem__", (class QChar (QStringView::*)(long long) const) &QStringView::operator[], "C++: QStringView::operator[](long long) const --> class QChar", pybind11::arg("n"));
		cl.def("toUcs4", (class QVector<unsigned int> (QStringView::*)() const) &QStringView::toUcs4, "C++: QStringView::toUcs4() const --> class QVector<unsigned int>");
		cl.def("at", (class QChar (QStringView::*)(long long) const) &QStringView::at, "C++: QStringView::at(long long) const --> class QChar", pybind11::arg("n"));
		cl.def("mid", (class QStringView (QStringView::*)(long long) const) &QStringView::mid, "C++: QStringView::mid(long long) const --> class QStringView", pybind11::arg("pos"));
		cl.def("mid", (class QStringView (QStringView::*)(long long, long long) const) &QStringView::mid, "C++: QStringView::mid(long long, long long) const --> class QStringView", pybind11::arg("pos"), pybind11::arg("n"));
		cl.def("left", (class QStringView (QStringView::*)(long long) const) &QStringView::left, "C++: QStringView::left(long long) const --> class QStringView", pybind11::arg("n"));
		cl.def("right", (class QStringView (QStringView::*)(long long) const) &QStringView::right, "C++: QStringView::right(long long) const --> class QStringView", pybind11::arg("n"));
		cl.def("chopped", (class QStringView (QStringView::*)(long long) const) &QStringView::chopped, "C++: QStringView::chopped(long long) const --> class QStringView", pybind11::arg("n"));
		cl.def("truncate", (void (QStringView::*)(long long)) &QStringView::truncate, "C++: QStringView::truncate(long long) --> void", pybind11::arg("n"));
		cl.def("chop", (void (QStringView::*)(long long)) &QStringView::chop, "C++: QStringView::chop(long long) --> void", pybind11::arg("n"));
		cl.def("trimmed", (class QStringView (QStringView::*)() const) &QStringView::trimmed, "C++: QStringView::trimmed() const --> class QStringView");
		cl.def("compare", [](QStringView const &o, class QStringView const & a0) -> int { return o.compare(a0); }, "", pybind11::arg("other"));
		cl.def("compare", (int (QStringView::*)(class QStringView, enum Qt::CaseSensitivity) const) &QStringView::compare, "C++: QStringView::compare(class QStringView, enum Qt::CaseSensitivity) const --> int", pybind11::arg("other"), pybind11::arg("cs"));
		cl.def("compare", [](QStringView const &o, class QLatin1String const & a0) -> int { return o.compare(a0); }, "", pybind11::arg("other"));
		cl.def("compare", (int (QStringView::*)(class QLatin1String, enum Qt::CaseSensitivity) const) &QStringView::compare, "C++: QStringView::compare(class QLatin1String, enum Qt::CaseSensitivity) const --> int", pybind11::arg("other"), pybind11::arg("cs"));
		cl.def("compare", (int (QStringView::*)(class QChar) const) &QStringView::compare, "C++: QStringView::compare(class QChar) const --> int", pybind11::arg("c"));
		cl.def("compare", (int (QStringView::*)(class QChar, enum Qt::CaseSensitivity) const) &QStringView::compare, "C++: QStringView::compare(class QChar, enum Qt::CaseSensitivity) const --> int", pybind11::arg("c"), pybind11::arg("cs"));
		cl.def("startsWith", [](QStringView const &o, class QStringView const & a0) -> bool { return o.startsWith(a0); }, "", pybind11::arg("s"));
		cl.def("startsWith", (bool (QStringView::*)(class QStringView, enum Qt::CaseSensitivity) const) &QStringView::startsWith, "C++: QStringView::startsWith(class QStringView, enum Qt::CaseSensitivity) const --> bool", pybind11::arg("s"), pybind11::arg("cs"));
		cl.def("startsWith", [](QStringView const &o, class QLatin1String const & a0) -> bool { return o.startsWith(a0); }, "", pybind11::arg("s"));
		cl.def("startsWith", (bool (QStringView::*)(class QLatin1String, enum Qt::CaseSensitivity) const) &QStringView::startsWith, "C++: QStringView::startsWith(class QLatin1String, enum Qt::CaseSensitivity) const --> bool", pybind11::arg("s"), pybind11::arg("cs"));
		cl.def("startsWith", (bool (QStringView::*)(class QChar) const) &QStringView::startsWith, "C++: QStringView::startsWith(class QChar) const --> bool", pybind11::arg("c"));
		cl.def("startsWith", (bool (QStringView::*)(class QChar, enum Qt::CaseSensitivity) const) &QStringView::startsWith, "C++: QStringView::startsWith(class QChar, enum Qt::CaseSensitivity) const --> bool", pybind11::arg("c"), pybind11::arg("cs"));
		cl.def("endsWith", [](QStringView const &o, class QStringView const & a0) -> bool { return o.endsWith(a0); }, "", pybind11::arg("s"));
		cl.def("endsWith", (bool (QStringView::*)(class QStringView, enum Qt::CaseSensitivity) const) &QStringView::endsWith, "C++: QStringView::endsWith(class QStringView, enum Qt::CaseSensitivity) const --> bool", pybind11::arg("s"), pybind11::arg("cs"));
		cl.def("endsWith", [](QStringView const &o, class QLatin1String const & a0) -> bool { return o.endsWith(a0); }, "", pybind11::arg("s"));
		cl.def("endsWith", (bool (QStringView::*)(class QLatin1String, enum Qt::CaseSensitivity) const) &QStringView::endsWith, "C++: QStringView::endsWith(class QLatin1String, enum Qt::CaseSensitivity) const --> bool", pybind11::arg("s"), pybind11::arg("cs"));
		cl.def("endsWith", (bool (QStringView::*)(class QChar) const) &QStringView::endsWith, "C++: QStringView::endsWith(class QChar) const --> bool", pybind11::arg("c"));
		cl.def("endsWith", (bool (QStringView::*)(class QChar, enum Qt::CaseSensitivity) const) &QStringView::endsWith, "C++: QStringView::endsWith(class QChar, enum Qt::CaseSensitivity) const --> bool", pybind11::arg("c"), pybind11::arg("cs"));
		cl.def("indexOf", [](QStringView const &o, class QChar const & a0) -> long long { return o.indexOf(a0); }, "", pybind11::arg("c"));
		cl.def("indexOf", [](QStringView const &o, class QChar const & a0, long long const & a1) -> long long { return o.indexOf(a0, a1); }, "", pybind11::arg("c"), pybind11::arg("from"));
		cl.def("indexOf", (long long (QStringView::*)(class QChar, long long, enum Qt::CaseSensitivity) const) &QStringView::indexOf, "C++: QStringView::indexOf(class QChar, long long, enum Qt::CaseSensitivity) const --> long long", pybind11::arg("c"), pybind11::arg("from"), pybind11::arg("cs"));
		cl.def("indexOf", [](QStringView const &o, class QStringView const & a0) -> long long { return o.indexOf(a0); }, "", pybind11::arg("s"));
		cl.def("indexOf", [](QStringView const &o, class QStringView const & a0, long long const & a1) -> long long { return o.indexOf(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("from"));
		cl.def("indexOf", (long long (QStringView::*)(class QStringView, long long, enum Qt::CaseSensitivity) const) &QStringView::indexOf, "C++: QStringView::indexOf(class QStringView, long long, enum Qt::CaseSensitivity) const --> long long", pybind11::arg("s"), pybind11::arg("from"), pybind11::arg("cs"));
		cl.def("indexOf", [](QStringView const &o, class QLatin1String const & a0) -> long long { return o.indexOf(a0); }, "", pybind11::arg("s"));
		cl.def("indexOf", [](QStringView const &o, class QLatin1String const & a0, long long const & a1) -> long long { return o.indexOf(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("from"));
		cl.def("indexOf", (long long (QStringView::*)(class QLatin1String, long long, enum Qt::CaseSensitivity) const) &QStringView::indexOf, "C++: QStringView::indexOf(class QLatin1String, long long, enum Qt::CaseSensitivity) const --> long long", pybind11::arg("s"), pybind11::arg("from"), pybind11::arg("cs"));
		cl.def("contains", [](QStringView const &o, class QChar const & a0) -> bool { return o.contains(a0); }, "", pybind11::arg("c"));
		cl.def("contains", (bool (QStringView::*)(class QChar, enum Qt::CaseSensitivity) const) &QStringView::contains, "C++: QStringView::contains(class QChar, enum Qt::CaseSensitivity) const --> bool", pybind11::arg("c"), pybind11::arg("cs"));
		cl.def("contains", [](QStringView const &o, class QStringView const & a0) -> bool { return o.contains(a0); }, "", pybind11::arg("s"));
		cl.def("contains", (bool (QStringView::*)(class QStringView, enum Qt::CaseSensitivity) const) &QStringView::contains, "C++: QStringView::contains(class QStringView, enum Qt::CaseSensitivity) const --> bool", pybind11::arg("s"), pybind11::arg("cs"));
		cl.def("contains", [](QStringView const &o, class QLatin1String const & a0) -> bool { return o.contains(a0); }, "", pybind11::arg("s"));
		cl.def("contains", (bool (QStringView::*)(class QLatin1String, enum Qt::CaseSensitivity) const) &QStringView::contains, "C++: QStringView::contains(class QLatin1String, enum Qt::CaseSensitivity) const --> bool", pybind11::arg("s"), pybind11::arg("cs"));
		cl.def("count", [](QStringView const &o, class QChar const & a0) -> long long { return o.count(a0); }, "", pybind11::arg("c"));
		cl.def("count", (long long (QStringView::*)(class QChar, enum Qt::CaseSensitivity) const) &QStringView::count, "C++: QStringView::count(class QChar, enum Qt::CaseSensitivity) const --> long long", pybind11::arg("c"), pybind11::arg("cs"));
		cl.def("count", [](QStringView const &o, class QStringView const & a0) -> long long { return o.count(a0); }, "", pybind11::arg("s"));
		cl.def("count", (long long (QStringView::*)(class QStringView, enum Qt::CaseSensitivity) const) &QStringView::count, "C++: QStringView::count(class QStringView, enum Qt::CaseSensitivity) const --> long long", pybind11::arg("s"), pybind11::arg("cs"));
		cl.def("lastIndexOf", [](QStringView const &o, class QChar const & a0) -> long long { return o.lastIndexOf(a0); }, "", pybind11::arg("c"));
		cl.def("lastIndexOf", [](QStringView const &o, class QChar const & a0, long long const & a1) -> long long { return o.lastIndexOf(a0, a1); }, "", pybind11::arg("c"), pybind11::arg("from"));
		cl.def("lastIndexOf", (long long (QStringView::*)(class QChar, long long, enum Qt::CaseSensitivity) const) &QStringView::lastIndexOf, "C++: QStringView::lastIndexOf(class QChar, long long, enum Qt::CaseSensitivity) const --> long long", pybind11::arg("c"), pybind11::arg("from"), pybind11::arg("cs"));
		cl.def("lastIndexOf", [](QStringView const &o, class QStringView const & a0) -> long long { return o.lastIndexOf(a0); }, "", pybind11::arg("s"));
		cl.def("lastIndexOf", [](QStringView const &o, class QStringView const & a0, long long const & a1) -> long long { return o.lastIndexOf(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("from"));
		cl.def("lastIndexOf", (long long (QStringView::*)(class QStringView, long long, enum Qt::CaseSensitivity) const) &QStringView::lastIndexOf, "C++: QStringView::lastIndexOf(class QStringView, long long, enum Qt::CaseSensitivity) const --> long long", pybind11::arg("s"), pybind11::arg("from"), pybind11::arg("cs"));
		cl.def("lastIndexOf", [](QStringView const &o, class QLatin1String const & a0) -> long long { return o.lastIndexOf(a0); }, "", pybind11::arg("s"));
		cl.def("lastIndexOf", [](QStringView const &o, class QLatin1String const & a0, long long const & a1) -> long long { return o.lastIndexOf(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("from"));
		cl.def("lastIndexOf", (long long (QStringView::*)(class QLatin1String, long long, enum Qt::CaseSensitivity) const) &QStringView::lastIndexOf, "C++: QStringView::lastIndexOf(class QLatin1String, long long, enum Qt::CaseSensitivity) const --> long long", pybind11::arg("s"), pybind11::arg("from"), pybind11::arg("cs"));
		cl.def("isRightToLeft", (bool (QStringView::*)() const) &QStringView::isRightToLeft, "C++: QStringView::isRightToLeft() const --> bool");
		cl.def("isValidUtf16", (bool (QStringView::*)() const) &QStringView::isValidUtf16, "C++: QStringView::isValidUtf16() const --> bool");
		cl.def("toShort", [](QStringView const &o) -> short { return o.toShort(); }, "");
		cl.def("toShort", [](QStringView const &o, bool * a0) -> short { return o.toShort(a0); }, "", pybind11::arg("ok"));
		cl.def("toShort", (short (QStringView::*)(bool *, int) const) &QStringView::toShort, "C++: QStringView::toShort(bool *, int) const --> short", pybind11::arg("ok"), pybind11::arg("base"));
		cl.def("toUShort", [](QStringView const &o) -> unsigned short { return o.toUShort(); }, "");
		cl.def("toUShort", [](QStringView const &o, bool * a0) -> unsigned short { return o.toUShort(a0); }, "", pybind11::arg("ok"));
		cl.def("toUShort", (unsigned short (QStringView::*)(bool *, int) const) &QStringView::toUShort, "C++: QStringView::toUShort(bool *, int) const --> unsigned short", pybind11::arg("ok"), pybind11::arg("base"));
		cl.def("toInt", [](QStringView const &o) -> int { return o.toInt(); }, "");
		cl.def("toInt", [](QStringView const &o, bool * a0) -> int { return o.toInt(a0); }, "", pybind11::arg("ok"));
		cl.def("toInt", (int (QStringView::*)(bool *, int) const) &QStringView::toInt, "C++: QStringView::toInt(bool *, int) const --> int", pybind11::arg("ok"), pybind11::arg("base"));
		cl.def("toUInt", [](QStringView const &o) -> unsigned int { return o.toUInt(); }, "");
		cl.def("toUInt", [](QStringView const &o, bool * a0) -> unsigned int { return o.toUInt(a0); }, "", pybind11::arg("ok"));
		cl.def("toUInt", (unsigned int (QStringView::*)(bool *, int) const) &QStringView::toUInt, "C++: QStringView::toUInt(bool *, int) const --> unsigned int", pybind11::arg("ok"), pybind11::arg("base"));
		cl.def("toLong", [](QStringView const &o) -> long { return o.toLong(); }, "");
		cl.def("toLong", [](QStringView const &o, bool * a0) -> long { return o.toLong(a0); }, "", pybind11::arg("ok"));
		cl.def("toLong", (long (QStringView::*)(bool *, int) const) &QStringView::toLong, "C++: QStringView::toLong(bool *, int) const --> long", pybind11::arg("ok"), pybind11::arg("base"));
		cl.def("toULong", [](QStringView const &o) -> unsigned long { return o.toULong(); }, "");
		cl.def("toULong", [](QStringView const &o, bool * a0) -> unsigned long { return o.toULong(a0); }, "", pybind11::arg("ok"));
		cl.def("toULong", (unsigned long (QStringView::*)(bool *, int) const) &QStringView::toULong, "C++: QStringView::toULong(bool *, int) const --> unsigned long", pybind11::arg("ok"), pybind11::arg("base"));
		cl.def("toLongLong", [](QStringView const &o) -> long long { return o.toLongLong(); }, "");
		cl.def("toLongLong", [](QStringView const &o, bool * a0) -> long long { return o.toLongLong(a0); }, "", pybind11::arg("ok"));
		cl.def("toLongLong", (long long (QStringView::*)(bool *, int) const) &QStringView::toLongLong, "C++: QStringView::toLongLong(bool *, int) const --> long long", pybind11::arg("ok"), pybind11::arg("base"));
		cl.def("toULongLong", [](QStringView const &o) -> unsigned long long { return o.toULongLong(); }, "");
		cl.def("toULongLong", [](QStringView const &o, bool * a0) -> unsigned long long { return o.toULongLong(a0); }, "", pybind11::arg("ok"));
		cl.def("toULongLong", (unsigned long long (QStringView::*)(bool *, int) const) &QStringView::toULongLong, "C++: QStringView::toULongLong(bool *, int) const --> unsigned long long", pybind11::arg("ok"), pybind11::arg("base"));
		cl.def("toFloat", [](QStringView const &o) -> float { return o.toFloat(); }, "");
		cl.def("toFloat", (float (QStringView::*)(bool *) const) &QStringView::toFloat, "C++: QStringView::toFloat(bool *) const --> float", pybind11::arg("ok"));
		cl.def("toDouble", [](QStringView const &o) -> double { return o.toDouble(); }, "");
		cl.def("toDouble", (double (QStringView::*)(bool *) const) &QStringView::toDouble, "C++: QStringView::toDouble(bool *) const --> double", pybind11::arg("ok"));
		cl.def("toWCharArray", (int (QStringView::*)(wchar_t *) const) &QStringView::toWCharArray, "C++: QStringView::toWCharArray(wchar_t *) const --> int", pybind11::arg("array"));
		cl.def("begin", (const class QChar * (QStringView::*)() const) &QStringView::begin, "C++: QStringView::begin() const --> const class QChar *", pybind11::return_value_policy::automatic);
		cl.def("end", (const class QChar * (QStringView::*)() const) &QStringView::end, "C++: QStringView::end() const --> const class QChar *", pybind11::return_value_policy::automatic);
		cl.def("cbegin", (const class QChar * (QStringView::*)() const) &QStringView::cbegin, "C++: QStringView::cbegin() const --> const class QChar *", pybind11::return_value_policy::automatic);
		cl.def("cend", (const class QChar * (QStringView::*)() const) &QStringView::cend, "C++: QStringView::cend() const --> const class QChar *", pybind11::return_value_policy::automatic);
		cl.def("empty", (bool (QStringView::*)() const) &QStringView::empty, "C++: QStringView::empty() const --> bool");
		cl.def("front", (class QChar (QStringView::*)() const) &QStringView::front, "C++: QStringView::front() const --> class QChar");
		cl.def("back", (class QChar (QStringView::*)() const) &QStringView::back, "C++: QStringView::back() const --> class QChar");
		cl.def("isNull", (bool (QStringView::*)() const) &QStringView::isNull, "C++: QStringView::isNull() const --> bool");
		cl.def("isEmpty", (bool (QStringView::*)() const) &QStringView::isEmpty, "C++: QStringView::isEmpty() const --> bool");
		cl.def("length", (int (QStringView::*)() const) &QStringView::length, "C++: QStringView::length() const --> int");
		cl.def("first", (class QChar (QStringView::*)() const) &QStringView::first, "C++: QStringView::first() const --> class QChar");
		cl.def("last", (class QChar (QStringView::*)() const) &QStringView::last, "C++: QStringView::last() const --> class QChar");
		cl.def("assign", (class QStringView & (QStringView::*)(const class QStringView &)) &QStringView::operator=, "C++: QStringView::operator=(const class QStringView &) --> class QStringView &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
