#include <QtCore/qbytearray.h> // 
#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qbytearray.h> // QByteArrayDataPtr
#include <QtCore/qbytearray.h> // QByteRef
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qchar.h> // QLatin1Char
#include <QtCore/qcoreevent.h> // QEvent
#include <QtCore/qdatetime.h> // 
#include <QtCore/qdatetime.h> // QDate
#include <QtCore/qdatetime.h> // QDateTime
#include <QtCore/qdatetime.h> // QTime
#include <QtCore/qeventloop.h> // 
#include <QtCore/qflags.h> // QFlag
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qhash.h> // QHash
#include <QtCore/qhash.h> // QHashData
#include <QtCore/qhash.h> // QHashData::Node
#include <QtCore/qhash.h> // QHashDummyValue
#include <QtCore/qhash.h> // QHashNode
#include <QtCore/qiodevice.h> // 
#include <QtCore/qiodevice.h> // QIODevice
#include <QtCore/qjsonvalue.h> // 
#include <QtCore/qjsonvalue.h> // QJsonValue
#include <QtCore/qjsonvalue.h> // QJsonValueRef
#include <QtCore/qlist.h> // QList
#include <QtCore/qlocale.h> // 
#include <QtCore/qlocale.h> // QLocale
#include <QtCore/qmap.h> // 
#include <QtCore/qmap.h> // QMap
#include <QtCore/qmap.h> // QMapData
#include <QtCore/qmap.h> // QMapDataBase
#include <QtCore/qmap.h> // QMapNode
#include <QtCore/qmap.h> // QMapNodeBase
#include <QtCore/qmap.h> // qMapLessThanKey
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::IteratorCapability
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::QAssociativeIterableImpl
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::QPairVariantInterfaceImpl
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::QSequentialIterableImpl
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::VariantData
#include <QtCore/qnamespace.h> // Qt::AspectRatioMode
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::ConnectionType
#include <QtCore/qnamespace.h> // Qt::DateFormat
#include <QtCore/qnamespace.h> // Qt::DayOfWeek
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::ItemFlag
#include <QtCore/qnamespace.h> // Qt::LayoutDirection
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
#include <QtCore/qurl.h> // 
#include <QtCore/qurl.h> // QUrl
#include <QtCore/qurl.h> // QUrlTwoFlags
#include <QtCore/quuid.h> // 
#include <QtCore/quuid.h> // QUuid
#include <QtCore/qvariant.h> // 
#include <QtCore/qvariant.h> // QVariant
#include <QtCore/qvariant.h> // qvariant_cast
#include <QtCore/qvector.h> // QVector
#include <chrono> // std::chrono::duration
#include <iostream> // --trace
#include <iterator> // __gnu_cxx::__normal_iterator
#include <iterator> // std::reverse_iterator
#include <memory> // std::allocator
#include <ratio> // std::ratio
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

void bind_unknown_unknown_1(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B352_[QAbstractEventDispatcher] ";
	{ // QAbstractEventDispatcher file: line:56
		pybind11::class_<QAbstractEventDispatcher, std::shared_ptr<QAbstractEventDispatcher>, QObject> cl(M(""), "QAbstractEventDispatcher", "");
		cl.def("metaObject", (const struct QMetaObject * (QAbstractEventDispatcher::*)() const) &QAbstractEventDispatcher::metaObject, "C++: QAbstractEventDispatcher::metaObject() const --> const struct QMetaObject *", pybind11::return_value_policy::automatic);
		cl.def("qt_metacast", (void * (QAbstractEventDispatcher::*)(const char *)) &QAbstractEventDispatcher::qt_metacast, "C++: QAbstractEventDispatcher::qt_metacast(const char *) --> void *", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def_static("tr", [](const char * a0) -> QString { return QAbstractEventDispatcher::tr(a0); }, "", pybind11::arg("s"));
		cl.def_static("tr", [](const char * a0, const char * a1) -> QString { return QAbstractEventDispatcher::tr(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("c"));
		cl.def_static("tr", (class QString (*)(const char *, const char *, int)) &QAbstractEventDispatcher::tr, "C++: QAbstractEventDispatcher::tr(const char *, const char *, int) --> class QString", pybind11::arg("s"), pybind11::arg("c"), pybind11::arg("n"));
		cl.def_static("trUtf8", [](const char * a0) -> QString { return QAbstractEventDispatcher::trUtf8(a0); }, "", pybind11::arg("s"));
		cl.def_static("trUtf8", [](const char * a0, const char * a1) -> QString { return QAbstractEventDispatcher::trUtf8(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("c"));
		cl.def_static("trUtf8", (class QString (*)(const char *, const char *, int)) &QAbstractEventDispatcher::trUtf8, "C++: QAbstractEventDispatcher::trUtf8(const char *, const char *, int) --> class QString", pybind11::arg("s"), pybind11::arg("c"), pybind11::arg("n"));
		cl.def_static("instance", []() -> QAbstractEventDispatcher * { return QAbstractEventDispatcher::instance(); }, "", pybind11::return_value_policy::automatic);
		cl.def_static("instance", (class QAbstractEventDispatcher * (*)(class QThread *)) &QAbstractEventDispatcher::instance, "C++: QAbstractEventDispatcher::instance(class QThread *) --> class QAbstractEventDispatcher *", pybind11::return_value_policy::automatic, pybind11::arg("thread"));
		cl.def("hasPendingEvents", (bool (QAbstractEventDispatcher::*)()) &QAbstractEventDispatcher::hasPendingEvents, "C++: QAbstractEventDispatcher::hasPendingEvents() --> bool");
		cl.def("registerSocketNotifier", (void (QAbstractEventDispatcher::*)(class QSocketNotifier *)) &QAbstractEventDispatcher::registerSocketNotifier, "C++: QAbstractEventDispatcher::registerSocketNotifier(class QSocketNotifier *) --> void", pybind11::arg("notifier"));
		cl.def("unregisterSocketNotifier", (void (QAbstractEventDispatcher::*)(class QSocketNotifier *)) &QAbstractEventDispatcher::unregisterSocketNotifier, "C++: QAbstractEventDispatcher::unregisterSocketNotifier(class QSocketNotifier *) --> void", pybind11::arg("notifier"));
		cl.def("registerTimer", (int (QAbstractEventDispatcher::*)(int, enum Qt::TimerType, class QObject *)) &QAbstractEventDispatcher::registerTimer, "C++: QAbstractEventDispatcher::registerTimer(int, enum Qt::TimerType, class QObject *) --> int", pybind11::arg("interval"), pybind11::arg("timerType"), pybind11::arg("object"));
		cl.def("registerTimer", (void (QAbstractEventDispatcher::*)(int, int, enum Qt::TimerType, class QObject *)) &QAbstractEventDispatcher::registerTimer, "C++: QAbstractEventDispatcher::registerTimer(int, int, enum Qt::TimerType, class QObject *) --> void", pybind11::arg("timerId"), pybind11::arg("interval"), pybind11::arg("timerType"), pybind11::arg("object"));
		cl.def("unregisterTimer", (bool (QAbstractEventDispatcher::*)(int)) &QAbstractEventDispatcher::unregisterTimer, "C++: QAbstractEventDispatcher::unregisterTimer(int) --> bool", pybind11::arg("timerId"));
		cl.def("unregisterTimers", (bool (QAbstractEventDispatcher::*)(class QObject *)) &QAbstractEventDispatcher::unregisterTimers, "C++: QAbstractEventDispatcher::unregisterTimers(class QObject *) --> bool", pybind11::arg("object"));
		cl.def("remainingTime", (int (QAbstractEventDispatcher::*)(int)) &QAbstractEventDispatcher::remainingTime, "C++: QAbstractEventDispatcher::remainingTime(int) --> int", pybind11::arg("timerId"));
		cl.def("wakeUp", (void (QAbstractEventDispatcher::*)()) &QAbstractEventDispatcher::wakeUp, "C++: QAbstractEventDispatcher::wakeUp() --> void");
		cl.def("interrupt", (void (QAbstractEventDispatcher::*)()) &QAbstractEventDispatcher::interrupt, "C++: QAbstractEventDispatcher::interrupt() --> void");
		cl.def("flush", (void (QAbstractEventDispatcher::*)()) &QAbstractEventDispatcher::flush, "C++: QAbstractEventDispatcher::flush() --> void");
		cl.def("startingUp", (void (QAbstractEventDispatcher::*)()) &QAbstractEventDispatcher::startingUp, "C++: QAbstractEventDispatcher::startingUp() --> void");
		cl.def("closingDown", (void (QAbstractEventDispatcher::*)()) &QAbstractEventDispatcher::closingDown, "C++: QAbstractEventDispatcher::closingDown() --> void");
		cl.def("installNativeEventFilter", (void (QAbstractEventDispatcher::*)(class QAbstractNativeEventFilter *)) &QAbstractEventDispatcher::installNativeEventFilter, "C++: QAbstractEventDispatcher::installNativeEventFilter(class QAbstractNativeEventFilter *) --> void", pybind11::arg("filterObj"));
		cl.def("removeNativeEventFilter", (void (QAbstractEventDispatcher::*)(class QAbstractNativeEventFilter *)) &QAbstractEventDispatcher::removeNativeEventFilter, "C++: QAbstractEventDispatcher::removeNativeEventFilter(class QAbstractNativeEventFilter *) --> void", pybind11::arg("filterObj"));
		cl.def("aboutToBlock", (void (QAbstractEventDispatcher::*)()) &QAbstractEventDispatcher::aboutToBlock, "C++: QAbstractEventDispatcher::aboutToBlock() --> void");
		cl.def("awake", (void (QAbstractEventDispatcher::*)()) &QAbstractEventDispatcher::awake, "C++: QAbstractEventDispatcher::awake() --> void");

		{ // QAbstractEventDispatcher::TimerInfo file: line:62
			auto & enclosing_class = cl;
			pybind11::class_<QAbstractEventDispatcher::TimerInfo, std::shared_ptr<QAbstractEventDispatcher::TimerInfo>> cl(enclosing_class, "TimerInfo", "");
			cl.def( pybind11::init<int, int, enum Qt::TimerType>(), pybind11::arg("id"), pybind11::arg("i"), pybind11::arg("t") );

			cl.def_readwrite("timerId", &QAbstractEventDispatcher::TimerInfo::timerId);
			cl.def_readwrite("interval", &QAbstractEventDispatcher::TimerInfo::interval);
			cl.def_readwrite("timerType", &QAbstractEventDispatcher::TimerInfo::timerType);
		}

	}
	std::cout << "B353_[QTypeInfo<QAbstractEventDispatcher::TimerInfo>] ";
	std::cout << "B354_[bool qMapLessThanKey<QString>(const class QString &, const class QString &)] ";
	std::cout << "B355_[QMapData<QString,QVariant>] ";
	std::cout << "B356_[QMapNodeBase] ";
	std::cout << "B357_[QMapNode<QString,QVariant>] ";
	std::cout << "B358_[QMapDataBase] ";
	std::cout << "B359_[QHashData] ";
	std::cout << "B360_[QHashDummyValue] ";
	std::cout << "B361_[QTypeInfo<QHashDummyValue>] ";
	std::cout << "B362_[QHashNode<QString,QVariant>] ";
	std::cout << "B363_[class QtMetaTypePrivate::QSequentialIterableImpl qvariant_cast<QtMetaTypePrivate::QSequentialIterableImpl>(const class QVariant &)] ";
	std::cout << "B364_[class QtMetaTypePrivate::QAssociativeIterableImpl qvariant_cast<QtMetaTypePrivate::QAssociativeIterableImpl>(const class QVariant &)] ";
	std::cout << "B365_[class QtMetaTypePrivate::QPairVariantInterfaceImpl qvariant_cast<QtMetaTypePrivate::QPairVariantInterfaceImpl>(const class QVariant &)] ";
}
