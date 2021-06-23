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
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qhash.h> // QHash
#include <QtCore/qjsonvalue.h> // QJsonValue
#include <QtCore/qlist.h> // QList
#include <QtCore/qlocale.h> // QLocale
#include <QtCore/qmap.h> // QMap
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::QPairVariantInterfaceImpl
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::ConnectionType
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
#include <QtCore/qnamespace.h> // Qt::TimerType
#include <QtCore/qobject.h> // QObject
#include <QtCore/qobject.h> // QObjectUserData
#include <QtCore/qobject.h> // QSignalBlocker
#include <QtCore/qobjectdefs.h> // QGenericArgument
#include <QtCore/qobjectdefs.h> // QGenericReturnArgument
#include <QtCore/qobjectdefs.h> // QMetaObject
#include <QtCore/qpoint.h> // QPoint
#include <QtCore/qpoint.h> // QPointF
#include <QtCore/qregexp.h> // QRegExp
#include <QtCore/qregularexpression.h> // QRegularExpression
#include <QtCore/qregularexpression.h> // QRegularExpressionMatch
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

// QObject file:QtCore/qobject.h line:127
struct PyCallBack_QObject : public QObject {
	using QObject::QObject;

	const struct QMetaObject * metaObject() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QObject *>(this), "metaObject");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const struct QMetaObject *>::value) {
				static pybind11::detail::override_caster_t<const struct QMetaObject *> caster;
				return pybind11::detail::cast_ref<const struct QMetaObject *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const struct QMetaObject *>(std::move(o));
		}
		return QObject::metaObject();
	}
	void * qt_metacast(const char * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QObject *>(this), "qt_metacast");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void *>::value) {
				static pybind11::detail::override_caster_t<void *> caster;
				return pybind11::detail::cast_ref<void *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void *>(std::move(o));
		}
		return QObject::qt_metacast(a0);
	}
	void timerEvent(class QTimerEvent * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QObject *>(this), "timerEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const QObject *>(this), "childEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const QObject *>(this), "connectNotify");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const QObject *>(this), "disconnectNotify");
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

void bind_QtCore_qobject(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B349_[QObject] ";
	{ // QObject file:QtCore/qobject.h line:127
		pybind11::class_<QObject, std::shared_ptr<QObject>, PyCallBack_QObject> cl(M(""), "QObject", "");
		cl.def( pybind11::init( [](){ return new QObject(); }, [](){ return new PyCallBack_QObject(); } ), "doc");
		cl.def( pybind11::init<class QObject *>(), pybind11::arg("parent") );

		cl.def("metaObject", (const struct QMetaObject * (QObject::*)() const) &QObject::metaObject, "C++: QObject::metaObject() const --> const struct QMetaObject *", pybind11::return_value_policy::automatic);
		cl.def("qt_metacast", (void * (QObject::*)(const char *)) &QObject::qt_metacast, "C++: QObject::qt_metacast(const char *) --> void *", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def_static("tr", [](const char * a0) -> QString { return QObject::tr(a0); }, "", pybind11::arg("s"));
		cl.def_static("tr", [](const char * a0, const char * a1) -> QString { return QObject::tr(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("c"));
		cl.def_static("tr", (class QString (*)(const char *, const char *, int)) &QObject::tr, "C++: QObject::tr(const char *, const char *, int) --> class QString", pybind11::arg("s"), pybind11::arg("c"), pybind11::arg("n"));
		cl.def_static("trUtf8", [](const char * a0) -> QString { return QObject::trUtf8(a0); }, "", pybind11::arg("s"));
		cl.def_static("trUtf8", [](const char * a0, const char * a1) -> QString { return QObject::trUtf8(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("c"));
		cl.def_static("trUtf8", (class QString (*)(const char *, const char *, int)) &QObject::trUtf8, "C++: QObject::trUtf8(const char *, const char *, int) --> class QString", pybind11::arg("s"), pybind11::arg("c"), pybind11::arg("n"));
		cl.def("objectName", (class QString (QObject::*)() const) &QObject::objectName, "C++: QObject::objectName() const --> class QString");
		cl.def("setObjectName", (void (QObject::*)(const class QString &)) &QObject::setObjectName, "C++: QObject::setObjectName(const class QString &) --> void", pybind11::arg("name"));
		cl.def("isWidgetType", (bool (QObject::*)() const) &QObject::isWidgetType, "C++: QObject::isWidgetType() const --> bool");
		cl.def("isWindowType", (bool (QObject::*)() const) &QObject::isWindowType, "C++: QObject::isWindowType() const --> bool");
		cl.def("signalsBlocked", (bool (QObject::*)() const) &QObject::signalsBlocked, "C++: QObject::signalsBlocked() const --> bool");
		cl.def("blockSignals", (bool (QObject::*)(bool)) &QObject::blockSignals, "C++: QObject::blockSignals(bool) --> bool", pybind11::arg("b"));
		cl.def("thread", (class QThread * (QObject::*)() const) &QObject::thread, "C++: QObject::thread() const --> class QThread *", pybind11::return_value_policy::automatic);
		cl.def("moveToThread", (void (QObject::*)(class QThread *)) &QObject::moveToThread, "C++: QObject::moveToThread(class QThread *) --> void", pybind11::arg("thread"));
		cl.def("startTimer", [](QObject &o, int const & a0) -> int { return o.startTimer(a0); }, "", pybind11::arg("interval"));
		cl.def("startTimer", (int (QObject::*)(int, enum Qt::TimerType)) &QObject::startTimer, "C++: QObject::startTimer(int, enum Qt::TimerType) --> int", pybind11::arg("interval"), pybind11::arg("timerType"));
		cl.def("killTimer", (void (QObject::*)(int)) &QObject::killTimer, "C++: QObject::killTimer(int) --> void", pybind11::arg("id"));
		cl.def("setParent", (void (QObject::*)(class QObject *)) &QObject::setParent, "C++: QObject::setParent(class QObject *) --> void", pybind11::arg("parent"));
		cl.def("installEventFilter", (void (QObject::*)(class QObject *)) &QObject::installEventFilter, "C++: QObject::installEventFilter(class QObject *) --> void", pybind11::arg("filterObj"));
		cl.def("removeEventFilter", (void (QObject::*)(class QObject *)) &QObject::removeEventFilter, "C++: QObject::removeEventFilter(class QObject *) --> void", pybind11::arg("obj"));
		cl.def("dumpObjectTree", (void (QObject::*)()) &QObject::dumpObjectTree, "C++: QObject::dumpObjectTree() --> void");
		cl.def("dumpObjectInfo", (void (QObject::*)()) &QObject::dumpObjectInfo, "C++: QObject::dumpObjectInfo() --> void");
		cl.def("setProperty", (bool (QObject::*)(const char *, const class QVariant &)) &QObject::setProperty, "C++: QObject::setProperty(const char *, const class QVariant &) --> bool", pybind11::arg("name"), pybind11::arg("value"));
		cl.def("property", (class QVariant (QObject::*)(const char *) const) &QObject::property, "C++: QObject::property(const char *) const --> class QVariant", pybind11::arg("name"));
		cl.def_static("registerUserData", (unsigned int (*)()) &QObject::registerUserData, "C++: QObject::registerUserData() --> unsigned int");
		cl.def("setUserData", (void (QObject::*)(unsigned int, class QObjectUserData *)) &QObject::setUserData, "C++: QObject::setUserData(unsigned int, class QObjectUserData *) --> void", pybind11::arg("id"), pybind11::arg("data"));
		cl.def("userData", (class QObjectUserData * (QObject::*)(unsigned int) const) &QObject::userData, "C++: QObject::userData(unsigned int) const --> class QObjectUserData *", pybind11::return_value_policy::automatic, pybind11::arg("id"));
		cl.def("destroyed", [](QObject &o) -> void { return o.destroyed(); }, "");
		cl.def("destroyed", (void (QObject::*)(class QObject *)) &QObject::destroyed, "C++: QObject::destroyed(class QObject *) --> void", pybind11::arg(""));
		cl.def("parent", (class QObject * (QObject::*)() const) &QObject::parent, "C++: QObject::parent() const --> class QObject *", pybind11::return_value_policy::automatic);
		cl.def("inherits", (bool (QObject::*)(const char *) const) &QObject::inherits, "C++: QObject::inherits(const char *) const --> bool", pybind11::arg("classname"));
		cl.def("deleteLater", (void (QObject::*)()) &QObject::deleteLater, "C++: QObject::deleteLater() --> void");
	}
	std::cout << "B350_[QObjectUserData] ";
	{ // QObjectUserData file:QtCore/qobject.h line:489
		pybind11::class_<QObjectUserData, std::shared_ptr<QObjectUserData>> cl(M(""), "QObjectUserData", "");
		cl.def( pybind11::init( [](){ return new QObjectUserData(); } ) );
	}
	std::cout << "B351_[QSignalBlocker] ";
}
