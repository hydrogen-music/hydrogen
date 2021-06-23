#include <QtCore/qnamespace.h> // Qt::AspectRatioMode
#include <QtCore/qpoint.h> // QPoint
#include <QtCore/qsize.h> // QSize
#include <QtCore/qsize.h> // QSizeF
#include <iostream> // --trace
#include <sstream> // __str__
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

void bind_QtCore_qsize(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B563_[QSize] ";
	{ // QSize file:QtCore/qsize.h line:53
		pybind11::class_<QSize, std::shared_ptr<QSize>> cl(M(""), "QSize", "");
		cl.def( pybind11::init( [](){ return new QSize(); } ) );
		cl.def( pybind11::init<int, int>(), pybind11::arg("w"), pybind11::arg("h") );

		cl.def( pybind11::init( [](QSize const &o){ return new QSize(o); } ) );
		cl.def("isNull", (bool (QSize::*)() const) &QSize::isNull, "C++: QSize::isNull() const --> bool");
		cl.def("isEmpty", (bool (QSize::*)() const) &QSize::isEmpty, "C++: QSize::isEmpty() const --> bool");
		cl.def("isValid", (bool (QSize::*)() const) &QSize::isValid, "C++: QSize::isValid() const --> bool");
		cl.def("width", (int (QSize::*)() const) &QSize::width, "C++: QSize::width() const --> int");
		cl.def("height", (int (QSize::*)() const) &QSize::height, "C++: QSize::height() const --> int");
		cl.def("setWidth", (void (QSize::*)(int)) &QSize::setWidth, "C++: QSize::setWidth(int) --> void", pybind11::arg("w"));
		cl.def("setHeight", (void (QSize::*)(int)) &QSize::setHeight, "C++: QSize::setHeight(int) --> void", pybind11::arg("h"));
		cl.def("transpose", (void (QSize::*)()) &QSize::transpose, "C++: QSize::transpose() --> void");
		cl.def("transposed", (class QSize (QSize::*)() const) &QSize::transposed, "C++: QSize::transposed() const --> class QSize");
		cl.def("scale", (void (QSize::*)(int, int, enum Qt::AspectRatioMode)) &QSize::scale, "C++: QSize::scale(int, int, enum Qt::AspectRatioMode) --> void", pybind11::arg("w"), pybind11::arg("h"), pybind11::arg("mode"));
		cl.def("scale", (void (QSize::*)(const class QSize &, enum Qt::AspectRatioMode)) &QSize::scale, "C++: QSize::scale(const class QSize &, enum Qt::AspectRatioMode) --> void", pybind11::arg("s"), pybind11::arg("mode"));
		cl.def("scaled", (class QSize (QSize::*)(int, int, enum Qt::AspectRatioMode) const) &QSize::scaled, "C++: QSize::scaled(int, int, enum Qt::AspectRatioMode) const --> class QSize", pybind11::arg("w"), pybind11::arg("h"), pybind11::arg("mode"));
		cl.def("scaled", (class QSize (QSize::*)(const class QSize &, enum Qt::AspectRatioMode) const) &QSize::scaled, "C++: QSize::scaled(const class QSize &, enum Qt::AspectRatioMode) const --> class QSize", pybind11::arg("s"), pybind11::arg("mode"));
		cl.def("expandedTo", (class QSize (QSize::*)(const class QSize &) const) &QSize::expandedTo, "C++: QSize::expandedTo(const class QSize &) const --> class QSize", pybind11::arg(""));
		cl.def("boundedTo", (class QSize (QSize::*)(const class QSize &) const) &QSize::boundedTo, "C++: QSize::boundedTo(const class QSize &) const --> class QSize", pybind11::arg(""));
		cl.def("grownBy", (class QSize (QSize::*)(class QMargins) const) &QSize::grownBy, "C++: QSize::grownBy(class QMargins) const --> class QSize", pybind11::arg("m"));
		cl.def("shrunkBy", (class QSize (QSize::*)(class QMargins) const) &QSize::shrunkBy, "C++: QSize::shrunkBy(class QMargins) const --> class QSize", pybind11::arg("m"));
		cl.def("rwidth", (int & (QSize::*)()) &QSize::rwidth, "C++: QSize::rwidth() --> int &", pybind11::return_value_policy::automatic);
		cl.def("rheight", (int & (QSize::*)()) &QSize::rheight, "C++: QSize::rheight() --> int &", pybind11::return_value_policy::automatic);
		cl.def("__iadd__", (class QSize & (QSize::*)(const class QSize &)) &QSize::operator+=, "C++: QSize::operator+=(const class QSize &) --> class QSize &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("__isub__", (class QSize & (QSize::*)(const class QSize &)) &QSize::operator-=, "C++: QSize::operator-=(const class QSize &) --> class QSize &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("__imul__", (class QSize & (QSize::*)(double)) &QSize::operator*=, "C++: QSize::operator*=(double) --> class QSize &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("__idiv__", (class QSize & (QSize::*)(double)) &QSize::operator/=, "C++: QSize::operator/=(double) --> class QSize &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("assign", (class QSize & (QSize::*)(const class QSize &)) &QSize::operator=, "C++: QSize::operator=(const class QSize &) --> class QSize &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B564_[QTypeInfo<QSize>] ";
	std::cout << "B565_[QSizeF] ";
	{ // QSizeF file:QtCore/qsize.h line:221
		pybind11::class_<QSizeF, std::shared_ptr<QSizeF>> cl(M(""), "QSizeF", "");
		cl.def( pybind11::init( [](){ return new QSizeF(); } ) );
		cl.def( pybind11::init<const class QSize &>(), pybind11::arg("sz") );

		cl.def( pybind11::init<double, double>(), pybind11::arg("w"), pybind11::arg("h") );

		cl.def( pybind11::init( [](QSizeF const &o){ return new QSizeF(o); } ) );
		cl.def("isNull", (bool (QSizeF::*)() const) &QSizeF::isNull, "C++: QSizeF::isNull() const --> bool");
		cl.def("isEmpty", (bool (QSizeF::*)() const) &QSizeF::isEmpty, "C++: QSizeF::isEmpty() const --> bool");
		cl.def("isValid", (bool (QSizeF::*)() const) &QSizeF::isValid, "C++: QSizeF::isValid() const --> bool");
		cl.def("width", (double (QSizeF::*)() const) &QSizeF::width, "C++: QSizeF::width() const --> double");
		cl.def("height", (double (QSizeF::*)() const) &QSizeF::height, "C++: QSizeF::height() const --> double");
		cl.def("setWidth", (void (QSizeF::*)(double)) &QSizeF::setWidth, "C++: QSizeF::setWidth(double) --> void", pybind11::arg("w"));
		cl.def("setHeight", (void (QSizeF::*)(double)) &QSizeF::setHeight, "C++: QSizeF::setHeight(double) --> void", pybind11::arg("h"));
		cl.def("transpose", (void (QSizeF::*)()) &QSizeF::transpose, "C++: QSizeF::transpose() --> void");
		cl.def("transposed", (class QSizeF (QSizeF::*)() const) &QSizeF::transposed, "C++: QSizeF::transposed() const --> class QSizeF");
		cl.def("scale", (void (QSizeF::*)(double, double, enum Qt::AspectRatioMode)) &QSizeF::scale, "C++: QSizeF::scale(double, double, enum Qt::AspectRatioMode) --> void", pybind11::arg("w"), pybind11::arg("h"), pybind11::arg("mode"));
		cl.def("scale", (void (QSizeF::*)(const class QSizeF &, enum Qt::AspectRatioMode)) &QSizeF::scale, "C++: QSizeF::scale(const class QSizeF &, enum Qt::AspectRatioMode) --> void", pybind11::arg("s"), pybind11::arg("mode"));
		cl.def("scaled", (class QSizeF (QSizeF::*)(double, double, enum Qt::AspectRatioMode) const) &QSizeF::scaled, "C++: QSizeF::scaled(double, double, enum Qt::AspectRatioMode) const --> class QSizeF", pybind11::arg("w"), pybind11::arg("h"), pybind11::arg("mode"));
		cl.def("scaled", (class QSizeF (QSizeF::*)(const class QSizeF &, enum Qt::AspectRatioMode) const) &QSizeF::scaled, "C++: QSizeF::scaled(const class QSizeF &, enum Qt::AspectRatioMode) const --> class QSizeF", pybind11::arg("s"), pybind11::arg("mode"));
		cl.def("expandedTo", (class QSizeF (QSizeF::*)(const class QSizeF &) const) &QSizeF::expandedTo, "C++: QSizeF::expandedTo(const class QSizeF &) const --> class QSizeF", pybind11::arg(""));
		cl.def("boundedTo", (class QSizeF (QSizeF::*)(const class QSizeF &) const) &QSizeF::boundedTo, "C++: QSizeF::boundedTo(const class QSizeF &) const --> class QSizeF", pybind11::arg(""));
		cl.def("grownBy", (class QSizeF (QSizeF::*)(class QMarginsF) const) &QSizeF::grownBy, "C++: QSizeF::grownBy(class QMarginsF) const --> class QSizeF", pybind11::arg("m"));
		cl.def("shrunkBy", (class QSizeF (QSizeF::*)(class QMarginsF) const) &QSizeF::shrunkBy, "C++: QSizeF::shrunkBy(class QMarginsF) const --> class QSizeF", pybind11::arg("m"));
		cl.def("rwidth", (double & (QSizeF::*)()) &QSizeF::rwidth, "C++: QSizeF::rwidth() --> double &", pybind11::return_value_policy::automatic);
		cl.def("rheight", (double & (QSizeF::*)()) &QSizeF::rheight, "C++: QSizeF::rheight() --> double &", pybind11::return_value_policy::automatic);
		cl.def("__iadd__", (class QSizeF & (QSizeF::*)(const class QSizeF &)) &QSizeF::operator+=, "C++: QSizeF::operator+=(const class QSizeF &) --> class QSizeF &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("__isub__", (class QSizeF & (QSizeF::*)(const class QSizeF &)) &QSizeF::operator-=, "C++: QSizeF::operator-=(const class QSizeF &) --> class QSizeF &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("__imul__", (class QSizeF & (QSizeF::*)(double)) &QSizeF::operator*=, "C++: QSizeF::operator*=(double) --> class QSizeF &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("__idiv__", (class QSizeF & (QSizeF::*)(double)) &QSizeF::operator/=, "C++: QSizeF::operator/=(double) --> class QSizeF &", pybind11::return_value_policy::automatic, pybind11::arg("c"));
		cl.def("toSize", (class QSize (QSizeF::*)() const) &QSizeF::toSize, "C++: QSizeF::toSize() const --> class QSize");
		cl.def("assign", (class QSizeF & (QSizeF::*)(const class QSizeF &)) &QSizeF::operator=, "C++: QSizeF::operator=(const class QSizeF &) --> class QSizeF &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B566_[QTypeInfo<QSizeF>] ";
	std::cout << "B567_[QRect] ";
	{ // QRect file: line:57
		pybind11::class_<QRect, std::shared_ptr<QRect>> cl(M(""), "QRect", "");
		cl.def( pybind11::init( [](){ return new QRect(); } ) );
		cl.def( pybind11::init<int, int, int, int>(), pybind11::arg("left"), pybind11::arg("top"), pybind11::arg("width"), pybind11::arg("height") );

		cl.def( pybind11::init( [](QRect const &o){ return new QRect(o); } ) );
		cl.def("isNull", (bool (QRect::*)() const) &QRect::isNull, "C++: QRect::isNull() const --> bool");
		cl.def("isEmpty", (bool (QRect::*)() const) &QRect::isEmpty, "C++: QRect::isEmpty() const --> bool");
		cl.def("isValid", (bool (QRect::*)() const) &QRect::isValid, "C++: QRect::isValid() const --> bool");
		cl.def("left", (int (QRect::*)() const) &QRect::left, "C++: QRect::left() const --> int");
		cl.def("top", (int (QRect::*)() const) &QRect::top, "C++: QRect::top() const --> int");
		cl.def("right", (int (QRect::*)() const) &QRect::right, "C++: QRect::right() const --> int");
		cl.def("bottom", (int (QRect::*)() const) &QRect::bottom, "C++: QRect::bottom() const --> int");
		cl.def("normalized", (class QRect (QRect::*)() const) &QRect::normalized, "C++: QRect::normalized() const --> class QRect");
		cl.def("x", (int (QRect::*)() const) &QRect::x, "C++: QRect::x() const --> int");
		cl.def("y", (int (QRect::*)() const) &QRect::y, "C++: QRect::y() const --> int");
		cl.def("setLeft", (void (QRect::*)(int)) &QRect::setLeft, "C++: QRect::setLeft(int) --> void", pybind11::arg("pos"));
		cl.def("setTop", (void (QRect::*)(int)) &QRect::setTop, "C++: QRect::setTop(int) --> void", pybind11::arg("pos"));
		cl.def("setRight", (void (QRect::*)(int)) &QRect::setRight, "C++: QRect::setRight(int) --> void", pybind11::arg("pos"));
		cl.def("setBottom", (void (QRect::*)(int)) &QRect::setBottom, "C++: QRect::setBottom(int) --> void", pybind11::arg("pos"));
		cl.def("setX", (void (QRect::*)(int)) &QRect::setX, "C++: QRect::setX(int) --> void", pybind11::arg("x"));
		cl.def("setY", (void (QRect::*)(int)) &QRect::setY, "C++: QRect::setY(int) --> void", pybind11::arg("y"));
		cl.def("moveLeft", (void (QRect::*)(int)) &QRect::moveLeft, "C++: QRect::moveLeft(int) --> void", pybind11::arg("pos"));
		cl.def("moveTop", (void (QRect::*)(int)) &QRect::moveTop, "C++: QRect::moveTop(int) --> void", pybind11::arg("pos"));
		cl.def("moveRight", (void (QRect::*)(int)) &QRect::moveRight, "C++: QRect::moveRight(int) --> void", pybind11::arg("pos"));
		cl.def("moveBottom", (void (QRect::*)(int)) &QRect::moveBottom, "C++: QRect::moveBottom(int) --> void", pybind11::arg("pos"));
		cl.def("translate", (void (QRect::*)(int, int)) &QRect::translate, "C++: QRect::translate(int, int) --> void", pybind11::arg("dx"), pybind11::arg("dy"));
		cl.def("translated", (class QRect (QRect::*)(int, int) const) &QRect::translated, "C++: QRect::translated(int, int) const --> class QRect", pybind11::arg("dx"), pybind11::arg("dy"));
		cl.def("transposed", (class QRect (QRect::*)() const) &QRect::transposed, "C++: QRect::transposed() const --> class QRect");
		cl.def("moveTo", (void (QRect::*)(int, int)) &QRect::moveTo, "C++: QRect::moveTo(int, int) --> void", pybind11::arg("x"), pybind11::arg("t"));
		cl.def("setRect", (void (QRect::*)(int, int, int, int)) &QRect::setRect, "C++: QRect::setRect(int, int, int, int) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"));
		cl.def("getRect", (void (QRect::*)(int *, int *, int *, int *) const) &QRect::getRect, "C++: QRect::getRect(int *, int *, int *, int *) const --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"));
		cl.def("setCoords", (void (QRect::*)(int, int, int, int)) &QRect::setCoords, "C++: QRect::setCoords(int, int, int, int) --> void", pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"));
		cl.def("getCoords", (void (QRect::*)(int *, int *, int *, int *) const) &QRect::getCoords, "C++: QRect::getCoords(int *, int *, int *, int *) const --> void", pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"));
		cl.def("adjust", (void (QRect::*)(int, int, int, int)) &QRect::adjust, "C++: QRect::adjust(int, int, int, int) --> void", pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"));
		cl.def("adjusted", (class QRect (QRect::*)(int, int, int, int) const) &QRect::adjusted, "C++: QRect::adjusted(int, int, int, int) const --> class QRect", pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"));
		cl.def("size", (class QSize (QRect::*)() const) &QRect::size, "C++: QRect::size() const --> class QSize");
		cl.def("width", (int (QRect::*)() const) &QRect::width, "C++: QRect::width() const --> int");
		cl.def("height", (int (QRect::*)() const) &QRect::height, "C++: QRect::height() const --> int");
		cl.def("setWidth", (void (QRect::*)(int)) &QRect::setWidth, "C++: QRect::setWidth(int) --> void", pybind11::arg("w"));
		cl.def("setHeight", (void (QRect::*)(int)) &QRect::setHeight, "C++: QRect::setHeight(int) --> void", pybind11::arg("h"));
		cl.def("setSize", (void (QRect::*)(const class QSize &)) &QRect::setSize, "C++: QRect::setSize(const class QSize &) --> void", pybind11::arg("s"));
		cl.def("contains", [](QRect const &o, const class QRect & a0) -> bool { return o.contains(a0); }, "", pybind11::arg("r"));
		cl.def("contains", (bool (QRect::*)(const class QRect &, bool) const) &QRect::contains, "C++: QRect::contains(const class QRect &, bool) const --> bool", pybind11::arg("r"), pybind11::arg("proper"));
		cl.def("contains", (bool (QRect::*)(int, int) const) &QRect::contains, "C++: QRect::contains(int, int) const --> bool", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("contains", (bool (QRect::*)(int, int, bool) const) &QRect::contains, "C++: QRect::contains(int, int, bool) const --> bool", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("proper"));
		cl.def("united", (class QRect (QRect::*)(const class QRect &) const) &QRect::united, "C++: QRect::united(const class QRect &) const --> class QRect", pybind11::arg("other"));
		cl.def("intersected", (class QRect (QRect::*)(const class QRect &) const) &QRect::intersected, "C++: QRect::intersected(const class QRect &) const --> class QRect", pybind11::arg("other"));
		cl.def("intersects", (bool (QRect::*)(const class QRect &) const) &QRect::intersects, "C++: QRect::intersects(const class QRect &) const --> bool", pybind11::arg("r"));
		cl.def("marginsAdded", (class QRect (QRect::*)(const class QMargins &) const) &QRect::marginsAdded, "C++: QRect::marginsAdded(const class QMargins &) const --> class QRect", pybind11::arg("margins"));
		cl.def("marginsRemoved", (class QRect (QRect::*)(const class QMargins &) const) &QRect::marginsRemoved, "C++: QRect::marginsRemoved(const class QMargins &) const --> class QRect", pybind11::arg("margins"));
		cl.def("__iadd__", (class QRect & (QRect::*)(const class QMargins &)) &QRect::operator+=, "C++: QRect::operator+=(const class QMargins &) --> class QRect &", pybind11::return_value_policy::automatic, pybind11::arg("margins"));
		cl.def("__isub__", (class QRect & (QRect::*)(const class QMargins &)) &QRect::operator-=, "C++: QRect::operator-=(const class QMargins &) --> class QRect &", pybind11::return_value_policy::automatic, pybind11::arg("margins"));
		cl.def("assign", (class QRect & (QRect::*)(const class QRect &)) &QRect::operator=, "C++: QRect::operator=(const class QRect &) --> class QRect &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
