#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qlogging.h> // QtMsgType
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
#include <QtCore/qpoint.h> // QPoint
#include <QtCore/qpoint.h> // QPointF
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

void bind_QtCore_qpoint(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B511_[QPointF] ";
	{ // QPointF file:QtCore/qpoint.h line:221
		pybind11::class_<QPointF, std::shared_ptr<QPointF>> cl(M(""), "QPointF", "");
		cl.def( pybind11::init( [](){ return new QPointF(); } ) );
		cl.def( pybind11::init<double, double>(), pybind11::arg("xpos"), pybind11::arg("ypos") );

		cl.def( pybind11::init( [](QPointF const &o){ return new QPointF(o); } ) );
		cl.def("manhattanLength", (double (QPointF::*)() const) &QPointF::manhattanLength, "C++: QPointF::manhattanLength() const --> double");
		cl.def("isNull", (bool (QPointF::*)() const) &QPointF::isNull, "C++: QPointF::isNull() const --> bool");
		cl.def("x", (double (QPointF::*)() const) &QPointF::x, "C++: QPointF::x() const --> double");
		cl.def("y", (double (QPointF::*)() const) &QPointF::y, "C++: QPointF::y() const --> double");
		cl.def("setX", (void (QPointF::*)(double)) &QPointF::setX, "C++: QPointF::setX(double) --> void", pybind11::arg("x"));
		cl.def("setY", (void (QPointF::*)(double)) &QPointF::setY, "C++: QPointF::setY(double) --> void", pybind11::arg("y"));
		cl.def("transposed", (class QPointF (QPointF::*)() const) &QPointF::transposed, "C++: QPointF::transposed() const --> class QPointF");
		cl.def("rx", (double & (QPointF::*)()) &QPointF::rx, "C++: QPointF::rx() --> double &", pybind11::return_value_policy::automatic);
		cl.def("ry", (double & (QPointF::*)()) &QPointF::ry, "C++: QPointF::ry() --> double &", pybind11::return_value_policy::automatic);
		cl.def("__iadd__", (class QPointF & (QPointF::*)(const class QPointF &)) &QPointF::operator+=, "C++: QPointF::operator+=(const class QPointF &) --> class QPointF &", pybind11::return_value_policy::automatic, pybind11::arg("p"));
		cl.def("__isub__", (class QPointF & (QPointF::*)(const class QPointF &)) &QPointF::operator-=, "C++: QPointF::operator-=(const class QPointF &) --> class QPointF &", pybind11::return_value_policy::automatic, pybind11::arg("p"));
		cl.def("__imul__", (class QPointF & (QPointF::*)(double)) &QPointF::operator*=, "C++: QPointF::operator*=(double) --> class QPointF &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("__idiv__", (class QPointF & (QPointF::*)(double)) &QPointF::operator/=, "C++: QPointF::operator/=(double) --> class QPointF &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def_static("dotProduct", (double (*)(const class QPointF &, const class QPointF &)) &QPointF::dotProduct, "C++: QPointF::dotProduct(const class QPointF &, const class QPointF &) --> double", pybind11::arg("p1"), pybind11::arg("p2"));
		cl.def("assign", (class QPointF & (QPointF::*)(const class QPointF &)) &QPointF::operator=, "C++: QPointF::operator=(const class QPointF &) --> class QPointF &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B512_[QTypeInfo<QPointF>] ";
	std::cout << "B513_[QLine] ";
	{ // QLine file: line:52
		pybind11::class_<QLine, std::shared_ptr<QLine>> cl(M(""), "QLine", "*****************************************************************************\n class QLine\n*****************************************************************************");
		cl.def( pybind11::init( [](){ return new QLine(); } ) );
		cl.def( pybind11::init<int, int, int, int>(), pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2") );

		cl.def( pybind11::init( [](QLine const &o){ return new QLine(o); } ) );
		cl.def("isNull", (bool (QLine::*)() const) &QLine::isNull, "C++: QLine::isNull() const --> bool");
		cl.def("x1", (int (QLine::*)() const) &QLine::x1, "C++: QLine::x1() const --> int");
		cl.def("y1", (int (QLine::*)() const) &QLine::y1, "C++: QLine::y1() const --> int");
		cl.def("x2", (int (QLine::*)() const) &QLine::x2, "C++: QLine::x2() const --> int");
		cl.def("y2", (int (QLine::*)() const) &QLine::y2, "C++: QLine::y2() const --> int");
		cl.def("dx", (int (QLine::*)() const) &QLine::dx, "C++: QLine::dx() const --> int");
		cl.def("dy", (int (QLine::*)() const) &QLine::dy, "C++: QLine::dy() const --> int");
		cl.def("translate", (void (QLine::*)(int, int)) &QLine::translate, "C++: QLine::translate(int, int) --> void", pybind11::arg("dx"), pybind11::arg("dy"));
		cl.def("translated", (class QLine (QLine::*)(int, int) const) &QLine::translated, "C++: QLine::translated(int, int) const --> class QLine", pybind11::arg("dx"), pybind11::arg("dy"));
		cl.def("setLine", (void (QLine::*)(int, int, int, int)) &QLine::setLine, "C++: QLine::setLine(int, int, int, int) --> void", pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"));
		cl.def("__eq__", (bool (QLine::*)(const class QLine &) const) &QLine::operator==, "C++: QLine::operator==(const class QLine &) const --> bool", pybind11::arg("d"));
		cl.def("__ne__", (bool (QLine::*)(const class QLine &) const) &QLine::operator!=, "C++: QLine::operator!=(const class QLine &) const --> bool", pybind11::arg("d"));
	}
	std::cout << "B514_[QTypeInfo<QLine>] ";
	std::cout << "B515_[QLineF] ";
	{ // QLineF file: line:214
		pybind11::class_<QLineF, std::shared_ptr<QLineF>> cl(M(""), "QLineF", "*****************************************************************************\n class QLineF\n*****************************************************************************");
		cl.def( pybind11::init( [](){ return new QLineF(); } ) );
		cl.def( pybind11::init<const class QPointF &, const class QPointF &>(), pybind11::arg("pt1"), pybind11::arg("pt2") );

		cl.def( pybind11::init<double, double, double, double>(), pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2") );

		cl.def( pybind11::init<const class QLine &>(), pybind11::arg("line") );

		cl.def( pybind11::init( [](QLineF const &o){ return new QLineF(o); } ) );

		pybind11::enum_<QLineF::IntersectType>(cl, "IntersectType", pybind11::arithmetic(), "")
			.value("NoIntersection", QLineF::NoIntersection)
			.value("BoundedIntersection", QLineF::BoundedIntersection)
			.value("UnboundedIntersection", QLineF::UnboundedIntersection)
			.export_values();

		cl.def_static("fromPolar", (class QLineF (*)(double, double)) &QLineF::fromPolar, "C++: QLineF::fromPolar(double, double) --> class QLineF", pybind11::arg("length"), pybind11::arg("angle"));
		cl.def("isNull", (bool (QLineF::*)() const) &QLineF::isNull, "C++: QLineF::isNull() const --> bool");
		cl.def("p1", (class QPointF (QLineF::*)() const) &QLineF::p1, "C++: QLineF::p1() const --> class QPointF");
		cl.def("p2", (class QPointF (QLineF::*)() const) &QLineF::p2, "C++: QLineF::p2() const --> class QPointF");
		cl.def("x1", (double (QLineF::*)() const) &QLineF::x1, "C++: QLineF::x1() const --> double");
		cl.def("y1", (double (QLineF::*)() const) &QLineF::y1, "C++: QLineF::y1() const --> double");
		cl.def("x2", (double (QLineF::*)() const) &QLineF::x2, "C++: QLineF::x2() const --> double");
		cl.def("y2", (double (QLineF::*)() const) &QLineF::y2, "C++: QLineF::y2() const --> double");
		cl.def("dx", (double (QLineF::*)() const) &QLineF::dx, "C++: QLineF::dx() const --> double");
		cl.def("dy", (double (QLineF::*)() const) &QLineF::dy, "C++: QLineF::dy() const --> double");
		cl.def("length", (double (QLineF::*)() const) &QLineF::length, "C++: QLineF::length() const --> double");
		cl.def("setLength", (void (QLineF::*)(double)) &QLineF::setLength, "C++: QLineF::setLength(double) --> void", pybind11::arg("len"));
		cl.def("angle", (double (QLineF::*)() const) &QLineF::angle, "C++: QLineF::angle() const --> double");
		cl.def("setAngle", (void (QLineF::*)(double)) &QLineF::setAngle, "C++: QLineF::setAngle(double) --> void", pybind11::arg("angle"));
		cl.def("angleTo", (double (QLineF::*)(const class QLineF &) const) &QLineF::angleTo, "C++: QLineF::angleTo(const class QLineF &) const --> double", pybind11::arg("l"));
		cl.def("unitVector", (class QLineF (QLineF::*)() const) &QLineF::unitVector, "C++: QLineF::unitVector() const --> class QLineF");
		cl.def("normalVector", (class QLineF (QLineF::*)() const) &QLineF::normalVector, "C++: QLineF::normalVector() const --> class QLineF");
		cl.def("intersects", (enum QLineF::IntersectType (QLineF::*)(const class QLineF &, class QPointF *) const) &QLineF::intersects, "C++: QLineF::intersects(const class QLineF &, class QPointF *) const --> enum QLineF::IntersectType", pybind11::arg("l"), pybind11::arg("intersectionPoint"));
		cl.def("intersect", (enum QLineF::IntersectType (QLineF::*)(const class QLineF &, class QPointF *) const) &QLineF::intersect, "C++: QLineF::intersect(const class QLineF &, class QPointF *) const --> enum QLineF::IntersectType", pybind11::arg("l"), pybind11::arg("intersectionPoint"));
		cl.def("angle", (double (QLineF::*)(const class QLineF &) const) &QLineF::angle, "C++: QLineF::angle(const class QLineF &) const --> double", pybind11::arg("l"));
		cl.def("pointAt", (class QPointF (QLineF::*)(double) const) &QLineF::pointAt, "C++: QLineF::pointAt(double) const --> class QPointF", pybind11::arg("t"));
		cl.def("translate", (void (QLineF::*)(const class QPointF &)) &QLineF::translate, "C++: QLineF::translate(const class QPointF &) --> void", pybind11::arg("p"));
		cl.def("translate", (void (QLineF::*)(double, double)) &QLineF::translate, "C++: QLineF::translate(double, double) --> void", pybind11::arg("dx"), pybind11::arg("dy"));
		cl.def("translated", (class QLineF (QLineF::*)(const class QPointF &) const) &QLineF::translated, "C++: QLineF::translated(const class QPointF &) const --> class QLineF", pybind11::arg("p"));
		cl.def("translated", (class QLineF (QLineF::*)(double, double) const) &QLineF::translated, "C++: QLineF::translated(double, double) const --> class QLineF", pybind11::arg("dx"), pybind11::arg("dy"));
		cl.def("center", (class QPointF (QLineF::*)() const) &QLineF::center, "C++: QLineF::center() const --> class QPointF");
		cl.def("setP1", (void (QLineF::*)(const class QPointF &)) &QLineF::setP1, "C++: QLineF::setP1(const class QPointF &) --> void", pybind11::arg("p1"));
		cl.def("setP2", (void (QLineF::*)(const class QPointF &)) &QLineF::setP2, "C++: QLineF::setP2(const class QPointF &) --> void", pybind11::arg("p2"));
		cl.def("setPoints", (void (QLineF::*)(const class QPointF &, const class QPointF &)) &QLineF::setPoints, "C++: QLineF::setPoints(const class QPointF &, const class QPointF &) --> void", pybind11::arg("p1"), pybind11::arg("p2"));
		cl.def("setLine", (void (QLineF::*)(double, double, double, double)) &QLineF::setLine, "C++: QLineF::setLine(double, double, double, double) --> void", pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"));
		cl.def("__eq__", (bool (QLineF::*)(const class QLineF &) const) &QLineF::operator==, "C++: QLineF::operator==(const class QLineF &) const --> bool", pybind11::arg("d"));
		cl.def("__ne__", (bool (QLineF::*)(const class QLineF &) const) &QLineF::operator!=, "C++: QLineF::operator!=(const class QLineF &) const --> bool", pybind11::arg("d"));
		cl.def("toLine", (class QLine (QLineF::*)() const) &QLineF::toLine, "C++: QLineF::toLine() const --> class QLine");
	}
	std::cout << "B516_[QTypeInfo<QLineF>] ";
	std::cout << "B517_[QLinkedListData] ";
	std::cout << "B518_[QLockFile] ";
	std::cout << "B519_[QLoggingCategory] ";
	std::cout << "B520_[QMargins] ";
	{ // QMargins file: line:51
		pybind11::class_<QMargins, std::shared_ptr<QMargins>> cl(M(""), "QMargins", "***************************************************************************\n  QMargins class\n***************************************************************************");
		cl.def( pybind11::init( [](){ return new QMargins(); } ) );
		cl.def( pybind11::init<int, int, int, int>(), pybind11::arg("left"), pybind11::arg("top"), pybind11::arg("right"), pybind11::arg("bottom") );

		cl.def( pybind11::init( [](QMargins const &o){ return new QMargins(o); } ) );
		cl.def("isNull", (bool (QMargins::*)() const) &QMargins::isNull, "C++: QMargins::isNull() const --> bool");
		cl.def("left", (int (QMargins::*)() const) &QMargins::left, "C++: QMargins::left() const --> int");
		cl.def("top", (int (QMargins::*)() const) &QMargins::top, "C++: QMargins::top() const --> int");
		cl.def("right", (int (QMargins::*)() const) &QMargins::right, "C++: QMargins::right() const --> int");
		cl.def("bottom", (int (QMargins::*)() const) &QMargins::bottom, "C++: QMargins::bottom() const --> int");
		cl.def("setLeft", (void (QMargins::*)(int)) &QMargins::setLeft, "C++: QMargins::setLeft(int) --> void", pybind11::arg("left"));
		cl.def("setTop", (void (QMargins::*)(int)) &QMargins::setTop, "C++: QMargins::setTop(int) --> void", pybind11::arg("top"));
		cl.def("setRight", (void (QMargins::*)(int)) &QMargins::setRight, "C++: QMargins::setRight(int) --> void", pybind11::arg("right"));
		cl.def("setBottom", (void (QMargins::*)(int)) &QMargins::setBottom, "C++: QMargins::setBottom(int) --> void", pybind11::arg("bottom"));
		cl.def("__iadd__", (class QMargins & (QMargins::*)(const class QMargins &)) &QMargins::operator+=, "C++: QMargins::operator+=(const class QMargins &) --> class QMargins &", pybind11::return_value_policy::automatic, pybind11::arg("margins"));
		cl.def("__isub__", (class QMargins & (QMargins::*)(const class QMargins &)) &QMargins::operator-=, "C++: QMargins::operator-=(const class QMargins &) --> class QMargins &", pybind11::return_value_policy::automatic, pybind11::arg("margins"));
		cl.def("__iadd__", (class QMargins & (QMargins::*)(int)) &QMargins::operator+=, "C++: QMargins::operator+=(int) --> class QMargins &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("__isub__", (class QMargins & (QMargins::*)(int)) &QMargins::operator-=, "C++: QMargins::operator-=(int) --> class QMargins &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("__imul__", (class QMargins & (QMargins::*)(int)) &QMargins::operator*=, "C++: QMargins::operator*=(int) --> class QMargins &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("__idiv__", (class QMargins & (QMargins::*)(int)) &QMargins::operator/=, "C++: QMargins::operator/=(int) --> class QMargins &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("__imul__", (class QMargins & (QMargins::*)(double)) &QMargins::operator*=, "C++: QMargins::operator*=(double) --> class QMargins &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("__idiv__", (class QMargins & (QMargins::*)(double)) &QMargins::operator/=, "C++: QMargins::operator/=(double) --> class QMargins &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("assign", (class QMargins & (QMargins::*)(const class QMargins &)) &QMargins::operator=, "C++: QMargins::operator=(const class QMargins &) --> class QMargins &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B521_[QTypeInfo<QMargins>] ";
	std::cout << "B522_[QMarginsF] ";
	{ // QMarginsF file: line:285
		pybind11::class_<QMarginsF, std::shared_ptr<QMarginsF>> cl(M(""), "QMarginsF", "***************************************************************************\n  QMarginsF class\n***************************************************************************");
		cl.def( pybind11::init( [](){ return new QMarginsF(); } ) );
		cl.def( pybind11::init<double, double, double, double>(), pybind11::arg("left"), pybind11::arg("top"), pybind11::arg("right"), pybind11::arg("bottom") );

		cl.def( pybind11::init<const class QMargins &>(), pybind11::arg("margins") );

		cl.def( pybind11::init( [](QMarginsF const &o){ return new QMarginsF(o); } ) );
		cl.def("isNull", (bool (QMarginsF::*)() const) &QMarginsF::isNull, "C++: QMarginsF::isNull() const --> bool");
		cl.def("left", (double (QMarginsF::*)() const) &QMarginsF::left, "C++: QMarginsF::left() const --> double");
		cl.def("top", (double (QMarginsF::*)() const) &QMarginsF::top, "C++: QMarginsF::top() const --> double");
		cl.def("right", (double (QMarginsF::*)() const) &QMarginsF::right, "C++: QMarginsF::right() const --> double");
		cl.def("bottom", (double (QMarginsF::*)() const) &QMarginsF::bottom, "C++: QMarginsF::bottom() const --> double");
		cl.def("setLeft", (void (QMarginsF::*)(double)) &QMarginsF::setLeft, "C++: QMarginsF::setLeft(double) --> void", pybind11::arg("left"));
		cl.def("setTop", (void (QMarginsF::*)(double)) &QMarginsF::setTop, "C++: QMarginsF::setTop(double) --> void", pybind11::arg("top"));
		cl.def("setRight", (void (QMarginsF::*)(double)) &QMarginsF::setRight, "C++: QMarginsF::setRight(double) --> void", pybind11::arg("right"));
		cl.def("setBottom", (void (QMarginsF::*)(double)) &QMarginsF::setBottom, "C++: QMarginsF::setBottom(double) --> void", pybind11::arg("bottom"));
		cl.def("__iadd__", (class QMarginsF & (QMarginsF::*)(const class QMarginsF &)) &QMarginsF::operator+=, "C++: QMarginsF::operator+=(const class QMarginsF &) --> class QMarginsF &", pybind11::return_value_policy::automatic, pybind11::arg("margins"));
		cl.def("__isub__", (class QMarginsF & (QMarginsF::*)(const class QMarginsF &)) &QMarginsF::operator-=, "C++: QMarginsF::operator-=(const class QMarginsF &) --> class QMarginsF &", pybind11::return_value_policy::automatic, pybind11::arg("margins"));
		cl.def("__iadd__", (class QMarginsF & (QMarginsF::*)(double)) &QMarginsF::operator+=, "C++: QMarginsF::operator+=(double) --> class QMarginsF &", pybind11::return_value_policy::automatic, pybind11::arg("addend"));
		cl.def("__isub__", (class QMarginsF & (QMarginsF::*)(double)) &QMarginsF::operator-=, "C++: QMarginsF::operator-=(double) --> class QMarginsF &", pybind11::return_value_policy::automatic, pybind11::arg("subtrahend"));
		cl.def("__imul__", (class QMarginsF & (QMarginsF::*)(double)) &QMarginsF::operator*=, "C++: QMarginsF::operator*=(double) --> class QMarginsF &", pybind11::return_value_policy::automatic, pybind11::arg("factor"));
		cl.def("__idiv__", (class QMarginsF & (QMarginsF::*)(double)) &QMarginsF::operator/=, "C++: QMarginsF::operator/=(double) --> class QMarginsF &", pybind11::return_value_policy::automatic, pybind11::arg("divisor"));
		cl.def("toMargins", (class QMargins (QMarginsF::*)() const) &QMarginsF::toMargins, "C++: QMarginsF::toMargins() const --> class QMargins");
		cl.def("assign", (class QMarginsF & (QMarginsF::*)(const class QMarginsF &)) &QMarginsF::operator=, "C++: QMarginsF::operator=(const class QMarginsF &) --> class QMarginsF &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
