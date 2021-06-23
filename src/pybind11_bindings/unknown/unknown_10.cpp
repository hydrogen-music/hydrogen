#include <QtCore/qbytearray.h> // 
#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qbytearray.h> // QByteArrayDataPtr
#include <QtCore/qbytearray.h> // QByteRef
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qcoreevent.h> // 
#include <QtCore/qcoreevent.h> // QChildEvent
#include <QtCore/qcoreevent.h> // QEvent
#include <QtCore/qcoreevent.h> // QTimerEvent
#include <QtCore/qdatetime.h> // QDate
#include <QtCore/qdatetime.h> // QDateTime
#include <QtCore/qdatetime.h> // QTime
#include <QtCore/qflags.h> // QFlag
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qhash.h> // QHash
#include <QtCore/qiodevice.h> // 
#include <QtCore/qjsonvalue.h> // QJsonValue
#include <QtCore/qlist.h> // QList
#include <QtCore/qlocale.h> // 
#include <QtCore/qlocale.h> // QLocale
#include <QtCore/qmap.h> // QMap
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::QPairVariantInterfaceImpl
#include <QtCore/qnamespace.h> // Qt::AspectRatioMode
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::ConnectionType
#include <QtCore/qnamespace.h> // Qt::DateFormat
#include <QtCore/qnamespace.h> // Qt::DayOfWeek
#include <QtCore/qnamespace.h> // Qt::DropAction
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::ItemFlag
#include <QtCore/qnamespace.h> // Qt::LayoutDirection
#include <QtCore/qnamespace.h> // Qt::MatchFlag
#include <QtCore/qnamespace.h> // Qt::Orientation
#include <QtCore/qnamespace.h> // Qt::SortOrder
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
#include <QtCore/qnamespace.h> // Qt::TimeSpec
#include <QtCore/qnamespace.h> // Qt::TimerType
#include <QtCore/qobject.h> // QObject
#include <QtCore/qobject.h> // QObjectUserData
#include <QtCore/qobjectdefs.h> // QGenericArgument
#include <QtCore/qobjectdefs.h> // QGenericReturnArgument
#include <QtCore/qobjectdefs.h> // QMetaObject
#include <QtCore/qpoint.h> // QPoint
#include <QtCore/qpoint.h> // QPointF
#include <QtCore/qregexp.h> // 
#include <QtCore/qregexp.h> // QRegExp
#include <QtCore/qregularexpression.h> // 
#include <QtCore/qregularexpression.h> // QRegularExpression
#include <QtCore/qregularexpression.h> // QRegularExpressionMatch
#include <QtCore/qregularexpression.h> // QRegularExpressionMatchIterator
#include <QtCore/qsize.h> // QSize
#include <QtCore/qsize.h> // QSizeF
#include <QtCore/qstring.h> // 
#include <QtCore/qstring.h> // QCharRef
#include <QtCore/qstring.h> // QLatin1String
#include <QtCore/qstring.h> // QString
#include <QtCore/qstring.h> // QStringRef
#include <QtCore/qstringlist.h> // QStringList
#include <QtCore/qstringliteral.h> // QStringDataPtr
#include <QtCore/qstringview.h> // QStringView
#include <QtCore/qurl.h> // QUrl
#include <QtCore/quuid.h> // QUuid
#include <QtCore/qvariant.h> // 
#include <QtCore/qvariant.h> // QVariant
#include <QtCore/qvector.h> // QVector
#include <chrono> // std::chrono::duration
#include <iostream> // --trace
#include <iterator> // std::reverse_iterator
#include <memory> // std::allocator
#include <ratio> // std::ratio
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

// QSocketNotifier file: line:49
struct PyCallBack_QSocketNotifier : public QSocketNotifier {
	using QSocketNotifier::QSocketNotifier;

	const struct QMetaObject * metaObject() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QSocketNotifier *>(this), "metaObject");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const struct QMetaObject *>::value) {
				static pybind11::detail::override_caster_t<const struct QMetaObject *> caster;
				return pybind11::detail::cast_ref<const struct QMetaObject *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const struct QMetaObject *>(std::move(o));
		}
		return QSocketNotifier::metaObject();
	}
	void * qt_metacast(const char * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QSocketNotifier *>(this), "qt_metacast");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void *>::value) {
				static pybind11::detail::override_caster_t<void *> caster;
				return pybind11::detail::cast_ref<void *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void *>(std::move(o));
		}
		return QSocketNotifier::qt_metacast(a0);
	}
	void timerEvent(class QTimerEvent * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QSocketNotifier *>(this), "timerEvent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return QObject::timerEvent(a0);
	}
	void childEvent(class QChildEvent * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QSocketNotifier *>(this), "childEvent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return QObject::childEvent(a0);
	}
	void connectNotify(const class QMetaMethod & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QSocketNotifier *>(this), "connectNotify");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return QObject::connectNotify(a0);
	}
	void disconnectNotify(const class QMetaMethod & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QSocketNotifier *>(this), "disconnectNotify");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return QObject::disconnectNotify(a0);
	}
};

void bind_unknown_unknown_10(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B568_[QRectF] ";
	{ // QRectF file: line:511
		pybind11::class_<QRectF, std::shared_ptr<QRectF>> cl(M(""), "QRectF", "");
		cl.def( pybind11::init( [](){ return new QRectF(); } ) );
		cl.def( pybind11::init<const class QPointF &, const class QSizeF &>(), pybind11::arg("topleft"), pybind11::arg("size") );

		cl.def( pybind11::init<const class QPointF &, const class QPointF &>(), pybind11::arg("topleft"), pybind11::arg("bottomRight") );

		cl.def( pybind11::init<double, double, double, double>(), pybind11::arg("left"), pybind11::arg("top"), pybind11::arg("width"), pybind11::arg("height") );

		cl.def( pybind11::init<const class QRect &>(), pybind11::arg("rect") );

		cl.def( pybind11::init( [](QRectF const &o){ return new QRectF(o); } ) );
		cl.def("isNull", (bool (QRectF::*)() const) &QRectF::isNull, "C++: QRectF::isNull() const --> bool");
		cl.def("isEmpty", (bool (QRectF::*)() const) &QRectF::isEmpty, "C++: QRectF::isEmpty() const --> bool");
		cl.def("isValid", (bool (QRectF::*)() const) &QRectF::isValid, "C++: QRectF::isValid() const --> bool");
		cl.def("normalized", (class QRectF (QRectF::*)() const) &QRectF::normalized, "C++: QRectF::normalized() const --> class QRectF");
		cl.def("left", (double (QRectF::*)() const) &QRectF::left, "C++: QRectF::left() const --> double");
		cl.def("top", (double (QRectF::*)() const) &QRectF::top, "C++: QRectF::top() const --> double");
		cl.def("right", (double (QRectF::*)() const) &QRectF::right, "C++: QRectF::right() const --> double");
		cl.def("bottom", (double (QRectF::*)() const) &QRectF::bottom, "C++: QRectF::bottom() const --> double");
		cl.def("x", (double (QRectF::*)() const) &QRectF::x, "C++: QRectF::x() const --> double");
		cl.def("y", (double (QRectF::*)() const) &QRectF::y, "C++: QRectF::y() const --> double");
		cl.def("setLeft", (void (QRectF::*)(double)) &QRectF::setLeft, "C++: QRectF::setLeft(double) --> void", pybind11::arg("pos"));
		cl.def("setTop", (void (QRectF::*)(double)) &QRectF::setTop, "C++: QRectF::setTop(double) --> void", pybind11::arg("pos"));
		cl.def("setRight", (void (QRectF::*)(double)) &QRectF::setRight, "C++: QRectF::setRight(double) --> void", pybind11::arg("pos"));
		cl.def("setBottom", (void (QRectF::*)(double)) &QRectF::setBottom, "C++: QRectF::setBottom(double) --> void", pybind11::arg("pos"));
		cl.def("setX", (void (QRectF::*)(double)) &QRectF::setX, "C++: QRectF::setX(double) --> void", pybind11::arg("pos"));
		cl.def("setY", (void (QRectF::*)(double)) &QRectF::setY, "C++: QRectF::setY(double) --> void", pybind11::arg("pos"));
		cl.def("topLeft", (class QPointF (QRectF::*)() const) &QRectF::topLeft, "C++: QRectF::topLeft() const --> class QPointF");
		cl.def("bottomRight", (class QPointF (QRectF::*)() const) &QRectF::bottomRight, "C++: QRectF::bottomRight() const --> class QPointF");
		cl.def("topRight", (class QPointF (QRectF::*)() const) &QRectF::topRight, "C++: QRectF::topRight() const --> class QPointF");
		cl.def("bottomLeft", (class QPointF (QRectF::*)() const) &QRectF::bottomLeft, "C++: QRectF::bottomLeft() const --> class QPointF");
		cl.def("center", (class QPointF (QRectF::*)() const) &QRectF::center, "C++: QRectF::center() const --> class QPointF");
		cl.def("setTopLeft", (void (QRectF::*)(const class QPointF &)) &QRectF::setTopLeft, "C++: QRectF::setTopLeft(const class QPointF &) --> void", pybind11::arg("p"));
		cl.def("setBottomRight", (void (QRectF::*)(const class QPointF &)) &QRectF::setBottomRight, "C++: QRectF::setBottomRight(const class QPointF &) --> void", pybind11::arg("p"));
		cl.def("setTopRight", (void (QRectF::*)(const class QPointF &)) &QRectF::setTopRight, "C++: QRectF::setTopRight(const class QPointF &) --> void", pybind11::arg("p"));
		cl.def("setBottomLeft", (void (QRectF::*)(const class QPointF &)) &QRectF::setBottomLeft, "C++: QRectF::setBottomLeft(const class QPointF &) --> void", pybind11::arg("p"));
		cl.def("moveLeft", (void (QRectF::*)(double)) &QRectF::moveLeft, "C++: QRectF::moveLeft(double) --> void", pybind11::arg("pos"));
		cl.def("moveTop", (void (QRectF::*)(double)) &QRectF::moveTop, "C++: QRectF::moveTop(double) --> void", pybind11::arg("pos"));
		cl.def("moveRight", (void (QRectF::*)(double)) &QRectF::moveRight, "C++: QRectF::moveRight(double) --> void", pybind11::arg("pos"));
		cl.def("moveBottom", (void (QRectF::*)(double)) &QRectF::moveBottom, "C++: QRectF::moveBottom(double) --> void", pybind11::arg("pos"));
		cl.def("moveTopLeft", (void (QRectF::*)(const class QPointF &)) &QRectF::moveTopLeft, "C++: QRectF::moveTopLeft(const class QPointF &) --> void", pybind11::arg("p"));
		cl.def("moveBottomRight", (void (QRectF::*)(const class QPointF &)) &QRectF::moveBottomRight, "C++: QRectF::moveBottomRight(const class QPointF &) --> void", pybind11::arg("p"));
		cl.def("moveTopRight", (void (QRectF::*)(const class QPointF &)) &QRectF::moveTopRight, "C++: QRectF::moveTopRight(const class QPointF &) --> void", pybind11::arg("p"));
		cl.def("moveBottomLeft", (void (QRectF::*)(const class QPointF &)) &QRectF::moveBottomLeft, "C++: QRectF::moveBottomLeft(const class QPointF &) --> void", pybind11::arg("p"));
		cl.def("moveCenter", (void (QRectF::*)(const class QPointF &)) &QRectF::moveCenter, "C++: QRectF::moveCenter(const class QPointF &) --> void", pybind11::arg("p"));
		cl.def("translate", (void (QRectF::*)(double, double)) &QRectF::translate, "C++: QRectF::translate(double, double) --> void", pybind11::arg("dx"), pybind11::arg("dy"));
		cl.def("translate", (void (QRectF::*)(const class QPointF &)) &QRectF::translate, "C++: QRectF::translate(const class QPointF &) --> void", pybind11::arg("p"));
		cl.def("translated", (class QRectF (QRectF::*)(double, double) const) &QRectF::translated, "C++: QRectF::translated(double, double) const --> class QRectF", pybind11::arg("dx"), pybind11::arg("dy"));
		cl.def("translated", (class QRectF (QRectF::*)(const class QPointF &) const) &QRectF::translated, "C++: QRectF::translated(const class QPointF &) const --> class QRectF", pybind11::arg("p"));
		cl.def("transposed", (class QRectF (QRectF::*)() const) &QRectF::transposed, "C++: QRectF::transposed() const --> class QRectF");
		cl.def("moveTo", (void (QRectF::*)(double, double)) &QRectF::moveTo, "C++: QRectF::moveTo(double, double) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("moveTo", (void (QRectF::*)(const class QPointF &)) &QRectF::moveTo, "C++: QRectF::moveTo(const class QPointF &) --> void", pybind11::arg("p"));
		cl.def("setRect", (void (QRectF::*)(double, double, double, double)) &QRectF::setRect, "C++: QRectF::setRect(double, double, double, double) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"));
		cl.def("getRect", (void (QRectF::*)(double *, double *, double *, double *) const) &QRectF::getRect, "C++: QRectF::getRect(double *, double *, double *, double *) const --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"));
		cl.def("setCoords", (void (QRectF::*)(double, double, double, double)) &QRectF::setCoords, "C++: QRectF::setCoords(double, double, double, double) --> void", pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"));
		cl.def("getCoords", (void (QRectF::*)(double *, double *, double *, double *) const) &QRectF::getCoords, "C++: QRectF::getCoords(double *, double *, double *, double *) const --> void", pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"));
		cl.def("adjust", (void (QRectF::*)(double, double, double, double)) &QRectF::adjust, "C++: QRectF::adjust(double, double, double, double) --> void", pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"));
		cl.def("adjusted", (class QRectF (QRectF::*)(double, double, double, double) const) &QRectF::adjusted, "C++: QRectF::adjusted(double, double, double, double) const --> class QRectF", pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"));
		cl.def("size", (class QSizeF (QRectF::*)() const) &QRectF::size, "C++: QRectF::size() const --> class QSizeF");
		cl.def("width", (double (QRectF::*)() const) &QRectF::width, "C++: QRectF::width() const --> double");
		cl.def("height", (double (QRectF::*)() const) &QRectF::height, "C++: QRectF::height() const --> double");
		cl.def("setWidth", (void (QRectF::*)(double)) &QRectF::setWidth, "C++: QRectF::setWidth(double) --> void", pybind11::arg("w"));
		cl.def("setHeight", (void (QRectF::*)(double)) &QRectF::setHeight, "C++: QRectF::setHeight(double) --> void", pybind11::arg("h"));
		cl.def("setSize", (void (QRectF::*)(const class QSizeF &)) &QRectF::setSize, "C++: QRectF::setSize(const class QSizeF &) --> void", pybind11::arg("s"));
		cl.def("contains", (bool (QRectF::*)(const class QRectF &) const) &QRectF::contains, "C++: QRectF::contains(const class QRectF &) const --> bool", pybind11::arg("r"));
		cl.def("contains", (bool (QRectF::*)(const class QPointF &) const) &QRectF::contains, "C++: QRectF::contains(const class QPointF &) const --> bool", pybind11::arg("p"));
		cl.def("contains", (bool (QRectF::*)(double, double) const) &QRectF::contains, "C++: QRectF::contains(double, double) const --> bool", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("united", (class QRectF (QRectF::*)(const class QRectF &) const) &QRectF::united, "C++: QRectF::united(const class QRectF &) const --> class QRectF", pybind11::arg("other"));
		cl.def("intersected", (class QRectF (QRectF::*)(const class QRectF &) const) &QRectF::intersected, "C++: QRectF::intersected(const class QRectF &) const --> class QRectF", pybind11::arg("other"));
		cl.def("intersects", (bool (QRectF::*)(const class QRectF &) const) &QRectF::intersects, "C++: QRectF::intersects(const class QRectF &) const --> bool", pybind11::arg("r"));
		cl.def("marginsAdded", (class QRectF (QRectF::*)(const class QMarginsF &) const) &QRectF::marginsAdded, "C++: QRectF::marginsAdded(const class QMarginsF &) const --> class QRectF", pybind11::arg("margins"));
		cl.def("marginsRemoved", (class QRectF (QRectF::*)(const class QMarginsF &) const) &QRectF::marginsRemoved, "C++: QRectF::marginsRemoved(const class QMarginsF &) const --> class QRectF", pybind11::arg("margins"));
		cl.def("__iadd__", (class QRectF & (QRectF::*)(const class QMarginsF &)) &QRectF::operator+=, "C++: QRectF::operator+=(const class QMarginsF &) --> class QRectF &", pybind11::return_value_policy::automatic, pybind11::arg("margins"));
		cl.def("__isub__", (class QRectF & (QRectF::*)(const class QMarginsF &)) &QRectF::operator-=, "C++: QRectF::operator-=(const class QMarginsF &) --> class QRectF &", pybind11::return_value_policy::automatic, pybind11::arg("margins"));
		cl.def("toRect", (class QRect (QRectF::*)() const) &QRectF::toRect, "C++: QRectF::toRect() const --> class QRect");
		cl.def("toAlignedRect", (class QRect (QRectF::*)() const) &QRectF::toAlignedRect, "C++: QRectF::toAlignedRect() const --> class QRect");
		cl.def("assign", (class QRectF & (QRectF::*)(const class QRectF &)) &QRectF::operator=, "C++: QRectF::operator=(const class QRectF &) --> class QRectF &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B569_[QTypeInfo<QRectF>] ";
	std::cout << "B570_[QResource] ";
	std::cout << "B571_[QSaveFile] ";
	std::cout << "B572_[QSemaphore] ";
	std::cout << "B573_[QSemaphoreReleaser] ";
	std::cout << "B574_[QSequentialAnimationGroup] ";
	std::cout << "B575_[QSettings] ";
	std::cout << "B576_[const struct QMetaObject * qt_getEnumMetaObject(enum QSettings::Status)] ";
	std::cout << "B577_[const char * qt_getEnumName(enum QSettings::Status)] ";
	std::cout << "B578_[const struct QMetaObject * qt_getEnumMetaObject(enum QSettings::Format)] ";
	std::cout << "B579_[const char * qt_getEnumName(enum QSettings::Format)] ";
	std::cout << "B580_[const struct QMetaObject * qt_getEnumMetaObject(enum QSettings::Scope)] ";
	std::cout << "B581_[const char * qt_getEnumName(enum QSettings::Scope)] ";
	std::cout << "B582_[QSharedMemory] ";
	std::cout << "B583_[QSignalMapper] ";
	std::cout << "B584_[QSignalTransition] ";
	std::cout << "B585_[QSocketNotifier] ";
	{ // QSocketNotifier file: line:49
		pybind11::class_<QSocketNotifier, std::shared_ptr<QSocketNotifier>, PyCallBack_QSocketNotifier, QObject> cl(M(""), "QSocketNotifier", "");
		cl.def( pybind11::init( [](long long const & a0, enum QSocketNotifier::Type const & a1){ return new QSocketNotifier(a0, a1); }, [](long long const & a0, enum QSocketNotifier::Type const & a1){ return new PyCallBack_QSocketNotifier(a0, a1); } ), "doc");
		cl.def( pybind11::init<long long, enum QSocketNotifier::Type, class QObject *>(), pybind11::arg("socket"), pybind11::arg(""), pybind11::arg("parent") );


		pybind11::enum_<QSocketNotifier::Type>(cl, "Type", pybind11::arithmetic(), "")
			.value("Read", QSocketNotifier::Read)
			.value("Write", QSocketNotifier::Write)
			.value("Exception", QSocketNotifier::Exception)
			.export_values();

		cl.def("metaObject", (const struct QMetaObject * (QSocketNotifier::*)() const) &QSocketNotifier::metaObject, "C++: QSocketNotifier::metaObject() const --> const struct QMetaObject *", pybind11::return_value_policy::automatic);
		cl.def("qt_metacast", (void * (QSocketNotifier::*)(const char *)) &QSocketNotifier::qt_metacast, "C++: QSocketNotifier::qt_metacast(const char *) --> void *", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def_static("tr", [](const char * a0) -> QString { return QSocketNotifier::tr(a0); }, "", pybind11::arg("s"));
		cl.def_static("tr", [](const char * a0, const char * a1) -> QString { return QSocketNotifier::tr(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("c"));
		cl.def_static("tr", (class QString (*)(const char *, const char *, int)) &QSocketNotifier::tr, "C++: QSocketNotifier::tr(const char *, const char *, int) --> class QString", pybind11::arg("s"), pybind11::arg("c"), pybind11::arg("n"));
		cl.def_static("trUtf8", [](const char * a0) -> QString { return QSocketNotifier::trUtf8(a0); }, "", pybind11::arg("s"));
		cl.def_static("trUtf8", [](const char * a0, const char * a1) -> QString { return QSocketNotifier::trUtf8(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("c"));
		cl.def_static("trUtf8", (class QString (*)(const char *, const char *, int)) &QSocketNotifier::trUtf8, "C++: QSocketNotifier::trUtf8(const char *, const char *, int) --> class QString", pybind11::arg("s"), pybind11::arg("c"), pybind11::arg("n"));
		cl.def("socket", (long long (QSocketNotifier::*)() const) &QSocketNotifier::socket, "C++: QSocketNotifier::socket() const --> long long");
		cl.def("type", (enum QSocketNotifier::Type (QSocketNotifier::*)() const) &QSocketNotifier::type, "C++: QSocketNotifier::type() const --> enum QSocketNotifier::Type");
		cl.def("isEnabled", (bool (QSocketNotifier::*)() const) &QSocketNotifier::isEnabled, "C++: QSocketNotifier::isEnabled() const --> bool");
		cl.def("setEnabled", (void (QSocketNotifier::*)(bool)) &QSocketNotifier::setEnabled, "C++: QSocketNotifier::setEnabled(bool) --> void", pybind11::arg(""));
	}
	std::cout << "B586_[QSocketDescriptor] ";
	std::cout << "B587_[QMetaTypeId<QSocketNotifier::Type>] ";
	std::cout << "B588_[QMetaTypeId<QSocketDescriptor>] ";
	std::cout << "B589_[QSortFilterProxyModel] ";
	std::cout << "B590_[QStandardPaths] ";
	std::cout << "B591_[const struct QMetaObject * qt_getEnumMetaObject(enum QStandardPaths::StandardLocation)] ";
	std::cout << "B592_[const char * qt_getEnumName(enum QStandardPaths::StandardLocation)] ";
	std::cout << "B593_[const struct QMetaObject * qt_getEnumMetaObject(class QFlags<enum QStandardPaths::LocateOption>)] ";
	std::cout << "B594_[const char * qt_getEnumName(class QFlags<enum QStandardPaths::LocateOption>)] ";
	std::cout << "B595_[QState] ";
	std::cout << "B596_[const struct QMetaObject * qt_getEnumMetaObject(enum QState::ChildMode)] ";
	std::cout << "B597_[const char * qt_getEnumName(enum QState::ChildMode)] ";
	std::cout << "B598_[const struct QMetaObject * qt_getEnumMetaObject(enum QState::RestorePolicy)] ";
	std::cout << "B599_[const char * qt_getEnumName(enum QState::RestorePolicy)] ";
	std::cout << "B600_[QStateMachine] ";
	std::cout << "B601_[QStorageInfo] ";
	std::cout << "B602_[QTypeInfo<QStorageInfo>] ";
	std::cout << "B603_[void swap(class QStorageInfo &, class QStorageInfo &)] ";
	std::cout << "B604_[QMetaTypeId<QStorageInfo>] ";
}
