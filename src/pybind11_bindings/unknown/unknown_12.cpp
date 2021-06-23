#include <QtCore/qbytearray.h> // 
#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qbytearray.h> // QByteArrayDataPtr
#include <QtCore/qbytearray.h> // QByteRef
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qchar.h> // QLatin1Char
#include <QtCore/qcoreevent.h> // 
#include <QtCore/qcoreevent.h> // QChildEvent
#include <QtCore/qcoreevent.h> // QEvent
#include <QtCore/qcoreevent.h> // QTimerEvent
#include <QtCore/qdatetime.h> // QDate
#include <QtCore/qdatetime.h> // QDateTime
#include <QtCore/qdatetime.h> // QTime
#include <QtCore/qeventloop.h> // 
#include <QtCore/qflags.h> // QFlag
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qhash.h> // QHash
#include <QtCore/qjsonvalue.h> // QJsonValue
#include <QtCore/qlist.h> // QList
#include <QtCore/qlocale.h> // 
#include <QtCore/qlocale.h> // QLocale
#include <QtCore/qmap.h> // QMap
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::QPairVariantInterfaceImpl
#include <QtCore/qmutex.h> // 
#include <QtCore/qmutex.h> // QMutex
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
#include <QtCore/qpair.h> // QPair
#include <QtCore/qpoint.h> // QPoint
#include <QtCore/qpoint.h> // QPointF
#include <QtCore/qregexp.h> // 
#include <QtCore/qregexp.h> // QRegExp
#include <QtCore/qregularexpression.h> // 
#include <QtCore/qregularexpression.h> // QRegularExpression
#include <QtCore/qregularexpression.h> // QRegularExpressionMatch
#include <QtCore/qregularexpression.h> // QRegularExpressionMatchIterator
#include <QtCore/qrunnable.h> // QRunnable
#include <QtCore/qshareddata.h> // QSharedDataPointer
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
#include <QtCore/quuid.h> // QUuid
#include <QtCore/qvariant.h> // 
#include <QtCore/qvariant.h> // QVariant
#include <QtCore/qvector.h> // QVector
#include <QtCore/qxmlstream.h> // +include_for_class
#include <chrono> // std::chrono::duration
#include <functional> // std::function
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

// QThread file: line:68
struct PyCallBack_QThread : public QThread {
	using QThread::QThread;

	const struct QMetaObject * metaObject() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QThread *>(this), "metaObject");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const struct QMetaObject *>::value) {
				static pybind11::detail::override_caster_t<const struct QMetaObject *> caster;
				return pybind11::detail::cast_ref<const struct QMetaObject *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const struct QMetaObject *>(std::move(o));
		}
		return QThread::metaObject();
	}
	void * qt_metacast(const char * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QThread *>(this), "qt_metacast");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void *>::value) {
				static pybind11::detail::override_caster_t<void *> caster;
				return pybind11::detail::cast_ref<void *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void *>(std::move(o));
		}
		return QThread::qt_metacast(a0);
	}
	void run() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QThread *>(this), "run");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return QThread::run();
	}
	void timerEvent(class QTimerEvent * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QThread *>(this), "timerEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const QThread *>(this), "childEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const QThread *>(this), "connectNotify");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const QThread *>(this), "disconnectNotify");
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

// QXmlStreamEntityResolver file: line:327
struct PyCallBack_QXmlStreamEntityResolver : public QXmlStreamEntityResolver {
	using QXmlStreamEntityResolver::QXmlStreamEntityResolver;

	class QString resolveEntity(const class QString & a0, const class QString & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QXmlStreamEntityResolver *>(this), "resolveEntity");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return QXmlStreamEntityResolver::resolveEntity(a0, a1);
	}
	class QString resolveUndeclaredEntity(const class QString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QXmlStreamEntityResolver *>(this), "resolveUndeclaredEntity");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return QXmlStreamEntityResolver::resolveUndeclaredEntity(a0);
	}
};

void bind_unknown_unknown_12(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B608_[QThread] ";
	{ // QThread file: line:68
		pybind11::class_<QThread, std::shared_ptr<QThread>, PyCallBack_QThread, QObject> cl(M(""), "QThread", "");
		cl.def( pybind11::init( [](){ return new QThread(); }, [](){ return new PyCallBack_QThread(); } ), "doc");
		cl.def( pybind11::init<class QObject *>(), pybind11::arg("parent") );


		pybind11::enum_<QThread::Priority>(cl, "Priority", pybind11::arithmetic(), "")
			.value("IdlePriority", QThread::IdlePriority)
			.value("LowestPriority", QThread::LowestPriority)
			.value("LowPriority", QThread::LowPriority)
			.value("NormalPriority", QThread::NormalPriority)
			.value("HighPriority", QThread::HighPriority)
			.value("HighestPriority", QThread::HighestPriority)
			.value("TimeCriticalPriority", QThread::TimeCriticalPriority)
			.value("InheritPriority", QThread::InheritPriority)
			.export_values();

		cl.def("metaObject", (const struct QMetaObject * (QThread::*)() const) &QThread::metaObject, "C++: QThread::metaObject() const --> const struct QMetaObject *", pybind11::return_value_policy::automatic);
		cl.def("qt_metacast", (void * (QThread::*)(const char *)) &QThread::qt_metacast, "C++: QThread::qt_metacast(const char *) --> void *", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def_static("tr", [](const char * a0) -> QString { return QThread::tr(a0); }, "", pybind11::arg("s"));
		cl.def_static("tr", [](const char * a0, const char * a1) -> QString { return QThread::tr(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("c"));
		cl.def_static("tr", (class QString (*)(const char *, const char *, int)) &QThread::tr, "C++: QThread::tr(const char *, const char *, int) --> class QString", pybind11::arg("s"), pybind11::arg("c"), pybind11::arg("n"));
		cl.def_static("trUtf8", [](const char * a0) -> QString { return QThread::trUtf8(a0); }, "", pybind11::arg("s"));
		cl.def_static("trUtf8", [](const char * a0, const char * a1) -> QString { return QThread::trUtf8(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("c"));
		cl.def_static("trUtf8", (class QString (*)(const char *, const char *, int)) &QThread::trUtf8, "C++: QThread::trUtf8(const char *, const char *, int) --> class QString", pybind11::arg("s"), pybind11::arg("c"), pybind11::arg("n"));
		cl.def_static("currentThreadId", (void * (*)()) &QThread::currentThreadId, "C++: QThread::currentThreadId() --> void *", pybind11::return_value_policy::automatic);
		cl.def_static("currentThread", (class QThread * (*)()) &QThread::currentThread, "C++: QThread::currentThread() --> class QThread *", pybind11::return_value_policy::automatic);
		cl.def_static("idealThreadCount", (int (*)()) &QThread::idealThreadCount, "C++: QThread::idealThreadCount() --> int");
		cl.def_static("yieldCurrentThread", (void (*)()) &QThread::yieldCurrentThread, "C++: QThread::yieldCurrentThread() --> void");
		cl.def("setPriority", (void (QThread::*)(enum QThread::Priority)) &QThread::setPriority, "C++: QThread::setPriority(enum QThread::Priority) --> void", pybind11::arg("priority"));
		cl.def("priority", (enum QThread::Priority (QThread::*)() const) &QThread::priority, "C++: QThread::priority() const --> enum QThread::Priority");
		cl.def("isFinished", (bool (QThread::*)() const) &QThread::isFinished, "C++: QThread::isFinished() const --> bool");
		cl.def("isRunning", (bool (QThread::*)() const) &QThread::isRunning, "C++: QThread::isRunning() const --> bool");
		cl.def("requestInterruption", (void (QThread::*)()) &QThread::requestInterruption, "C++: QThread::requestInterruption() --> void");
		cl.def("isInterruptionRequested", (bool (QThread::*)() const) &QThread::isInterruptionRequested, "C++: QThread::isInterruptionRequested() const --> bool");
		cl.def("setStackSize", (void (QThread::*)(unsigned int)) &QThread::setStackSize, "C++: QThread::setStackSize(unsigned int) --> void", pybind11::arg("stackSize"));
		cl.def("stackSize", (unsigned int (QThread::*)() const) &QThread::stackSize, "C++: QThread::stackSize() const --> unsigned int");
		cl.def("exit", [](QThread &o) -> void { return o.exit(); }, "");
		cl.def("exit", (void (QThread::*)(int)) &QThread::exit, "C++: QThread::exit(int) --> void", pybind11::arg("retcode"));
		cl.def("eventDispatcher", (class QAbstractEventDispatcher * (QThread::*)() const) &QThread::eventDispatcher, "C++: QThread::eventDispatcher() const --> class QAbstractEventDispatcher *", pybind11::return_value_policy::automatic);
		cl.def("setEventDispatcher", (void (QThread::*)(class QAbstractEventDispatcher *)) &QThread::setEventDispatcher, "C++: QThread::setEventDispatcher(class QAbstractEventDispatcher *) --> void", pybind11::arg("eventDispatcher"));
		cl.def("loopLevel", (int (QThread::*)() const) &QThread::loopLevel, "C++: QThread::loopLevel() const --> int");
		cl.def("start", [](QThread &o) -> void { return o.start(); }, "");
		cl.def("start", (void (QThread::*)(enum QThread::Priority)) &QThread::start, "C++: QThread::start(enum QThread::Priority) --> void", pybind11::arg(""));
		cl.def("terminate", (void (QThread::*)()) &QThread::terminate, "C++: QThread::terminate() --> void");
		cl.def("quit", (void (QThread::*)()) &QThread::quit, "C++: QThread::quit() --> void");
		cl.def("wait", [](QThread &o) -> bool { return o.wait(); }, "");
		cl.def("wait", (bool (QThread::*)(class QDeadlineTimer)) &QThread::wait, "C++: QThread::wait(class QDeadlineTimer) --> bool", pybind11::arg("deadline"));
		cl.def("wait", (bool (QThread::*)(unsigned long)) &QThread::wait, "C++: QThread::wait(unsigned long) --> bool", pybind11::arg("time"));
		cl.def_static("sleep", (void (*)(unsigned long)) &QThread::sleep, "C++: QThread::sleep(unsigned long) --> void", pybind11::arg(""));
		cl.def_static("msleep", (void (*)(unsigned long)) &QThread::msleep, "C++: QThread::msleep(unsigned long) --> void", pybind11::arg(""));
		cl.def_static("usleep", (void (*)(unsigned long)) &QThread::usleep, "C++: QThread::usleep(unsigned long) --> void", pybind11::arg(""));
	}
	std::cout << "B609_[QThreadPool] ";
	std::cout << "B610_[QThreadStorageData] ";
	std::cout << "B611_[QTimeLine] ";
	std::cout << "B612_[QTimer] ";
	std::cout << "B613_[QTimeZone] ";
	std::cout << "B614_[QTypeInfo<QTimeZone::OffsetData>] ";
	std::cout << "B615_[QTypeInfo<QTimeZone>] ";
	std::cout << "B616_[void swap(class QTimeZone &, class QTimeZone &)] ";
	std::cout << "B617_[QTranslator] ";
	std::cout << "B618_[QTransposeProxyModel] ";
	std::cout << "B619_[unsigned int qHash(const class QUrlQuery &, unsigned int)] ";
	std::cout << "B620_[QUrlQuery] ";
	std::cout << "B621_[QTypeInfo<QUrlQuery>] ";
	std::cout << "B622_[void swap(class QUrlQuery &, class QUrlQuery &)] ";
	std::cout << "B623_[QWaitCondition] ";
	std::cout << "B624_[QXmlStreamStringRef] ";
	std::cout << "B625_[QTypeInfo<QXmlStreamStringRef>] ";
	std::cout << "B626_[void swap(class QXmlStreamStringRef &, class QXmlStreamStringRef &)] ";
	std::cout << "B627_[QXmlStreamAttribute] ";
	{ // QXmlStreamAttribute file: line:96
		pybind11::class_<QXmlStreamAttribute, std::shared_ptr<QXmlStreamAttribute>> cl(M(""), "QXmlStreamAttribute", "");
		cl.def( pybind11::init( [](){ return new QXmlStreamAttribute(); } ) );
		cl.def( pybind11::init<const class QString &, const class QString &>(), pybind11::arg("qualifiedName"), pybind11::arg("value") );

		cl.def( pybind11::init<const class QString &, const class QString &, const class QString &>(), pybind11::arg("namespaceUri"), pybind11::arg("name"), pybind11::arg("value") );

		cl.def( pybind11::init( [](QXmlStreamAttribute const &o){ return new QXmlStreamAttribute(o); } ) );
		cl.def("assign", (class QXmlStreamAttribute & (QXmlStreamAttribute::*)(const class QXmlStreamAttribute &)) &QXmlStreamAttribute::operator=, "C++: QXmlStreamAttribute::operator=(const class QXmlStreamAttribute &) --> class QXmlStreamAttribute &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("namespaceUri", (class QStringRef (QXmlStreamAttribute::*)() const) &QXmlStreamAttribute::namespaceUri, "C++: QXmlStreamAttribute::namespaceUri() const --> class QStringRef");
		cl.def("name", (class QStringRef (QXmlStreamAttribute::*)() const) &QXmlStreamAttribute::name, "C++: QXmlStreamAttribute::name() const --> class QStringRef");
		cl.def("qualifiedName", (class QStringRef (QXmlStreamAttribute::*)() const) &QXmlStreamAttribute::qualifiedName, "C++: QXmlStreamAttribute::qualifiedName() const --> class QStringRef");
		cl.def("prefix", (class QStringRef (QXmlStreamAttribute::*)() const) &QXmlStreamAttribute::prefix, "C++: QXmlStreamAttribute::prefix() const --> class QStringRef");
		cl.def("value", (class QStringRef (QXmlStreamAttribute::*)() const) &QXmlStreamAttribute::value, "C++: QXmlStreamAttribute::value() const --> class QStringRef");
		cl.def("isDefault", (bool (QXmlStreamAttribute::*)() const) &QXmlStreamAttribute::isDefault, "C++: QXmlStreamAttribute::isDefault() const --> bool");
		cl.def("__eq__", (bool (QXmlStreamAttribute::*)(const class QXmlStreamAttribute &) const) &QXmlStreamAttribute::operator==, "C++: QXmlStreamAttribute::operator==(const class QXmlStreamAttribute &) const --> bool", pybind11::arg("other"));
		cl.def("__ne__", (bool (QXmlStreamAttribute::*)(const class QXmlStreamAttribute &) const) &QXmlStreamAttribute::operator!=, "C++: QXmlStreamAttribute::operator!=(const class QXmlStreamAttribute &) const --> bool", pybind11::arg("other"));
	}
	std::cout << "B628_[QTypeInfo<QXmlStreamAttribute>] ";
	std::cout << "B629_[QXmlStreamAttributes] ";
	{ // QXmlStreamAttributes file: line:155
		pybind11::class_<QXmlStreamAttributes, std::shared_ptr<QXmlStreamAttributes>, QVector<QXmlStreamAttribute>> cl(M(""), "QXmlStreamAttributes", "");
		cl.def( pybind11::init( [](){ return new QXmlStreamAttributes(); } ) );
		cl.def( pybind11::init( [](QXmlStreamAttributes const &o){ return new QXmlStreamAttributes(o); } ) );
		cl.def("append", [](QXmlStreamAttributes &o, const class QVector<class QXmlStreamAttribute> & a0) -> void { return o.append(a0); }, "", pybind11::arg("l"));
		cl.def("append", [](QXmlStreamAttributes &o, const class QXmlStreamAttribute & a0) -> void { return o.append(a0); }, "", pybind11::arg("t"));
		cl.def("value", (class QStringRef (QXmlStreamAttributes::*)(const class QString &, const class QString &) const) &QXmlStreamAttributes::value, "C++: QXmlStreamAttributes::value(const class QString &, const class QString &) const --> class QStringRef", pybind11::arg("namespaceUri"), pybind11::arg("name"));
		cl.def("value", (class QStringRef (QXmlStreamAttributes::*)(const class QString &, class QLatin1String) const) &QXmlStreamAttributes::value, "C++: QXmlStreamAttributes::value(const class QString &, class QLatin1String) const --> class QStringRef", pybind11::arg("namespaceUri"), pybind11::arg("name"));
		cl.def("value", (class QStringRef (QXmlStreamAttributes::*)(class QLatin1String, class QLatin1String) const) &QXmlStreamAttributes::value, "C++: QXmlStreamAttributes::value(class QLatin1String, class QLatin1String) const --> class QStringRef", pybind11::arg("namespaceUri"), pybind11::arg("name"));
		cl.def("value", (class QStringRef (QXmlStreamAttributes::*)(const class QString &) const) &QXmlStreamAttributes::value, "C++: QXmlStreamAttributes::value(const class QString &) const --> class QStringRef", pybind11::arg("qualifiedName"));
		cl.def("value", (class QStringRef (QXmlStreamAttributes::*)(class QLatin1String) const) &QXmlStreamAttributes::value, "C++: QXmlStreamAttributes::value(class QLatin1String) const --> class QStringRef", pybind11::arg("qualifiedName"));
		cl.def("append", (void (QXmlStreamAttributes::*)(const class QString &, const class QString &, const class QString &)) &QXmlStreamAttributes::append, "C++: QXmlStreamAttributes::append(const class QString &, const class QString &, const class QString &) --> void", pybind11::arg("namespaceUri"), pybind11::arg("name"), pybind11::arg("value"));
		cl.def("append", (void (QXmlStreamAttributes::*)(const class QString &, const class QString &)) &QXmlStreamAttributes::append, "C++: QXmlStreamAttributes::append(const class QString &, const class QString &) --> void", pybind11::arg("qualifiedName"), pybind11::arg("value"));
		cl.def("hasAttribute", (bool (QXmlStreamAttributes::*)(const class QString &) const) &QXmlStreamAttributes::hasAttribute, "C++: QXmlStreamAttributes::hasAttribute(const class QString &) const --> bool", pybind11::arg("qualifiedName"));
		cl.def("hasAttribute", (bool (QXmlStreamAttributes::*)(class QLatin1String) const) &QXmlStreamAttributes::hasAttribute, "C++: QXmlStreamAttributes::hasAttribute(class QLatin1String) const --> bool", pybind11::arg("qualifiedName"));
		cl.def("hasAttribute", (bool (QXmlStreamAttributes::*)(const class QString &, const class QString &) const) &QXmlStreamAttributes::hasAttribute, "C++: QXmlStreamAttributes::hasAttribute(const class QString &, const class QString &) const --> bool", pybind11::arg("namespaceUri"), pybind11::arg("name"));
		cl.def("assign", (class QXmlStreamAttributes & (QXmlStreamAttributes::*)(const class QXmlStreamAttributes &)) &QXmlStreamAttributes::operator=, "C++: QXmlStreamAttributes::operator=(const class QXmlStreamAttributes &) --> class QXmlStreamAttributes &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B630_[QXmlStreamNamespaceDeclaration] ";
	{ // QXmlStreamNamespaceDeclaration file: line:185
		pybind11::class_<QXmlStreamNamespaceDeclaration, std::shared_ptr<QXmlStreamNamespaceDeclaration>> cl(M(""), "QXmlStreamNamespaceDeclaration", "");
		cl.def( pybind11::init( [](){ return new QXmlStreamNamespaceDeclaration(); } ) );
		cl.def( pybind11::init<const class QString &, const class QString &>(), pybind11::arg("prefix"), pybind11::arg("namespaceUri") );

		cl.def( pybind11::init( [](QXmlStreamNamespaceDeclaration const &o){ return new QXmlStreamNamespaceDeclaration(o); } ) );
		cl.def("assign", (class QXmlStreamNamespaceDeclaration & (QXmlStreamNamespaceDeclaration::*)(const class QXmlStreamNamespaceDeclaration &)) &QXmlStreamNamespaceDeclaration::operator=, "C++: QXmlStreamNamespaceDeclaration::operator=(const class QXmlStreamNamespaceDeclaration &) --> class QXmlStreamNamespaceDeclaration &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("prefix", (class QStringRef (QXmlStreamNamespaceDeclaration::*)() const) &QXmlStreamNamespaceDeclaration::prefix, "C++: QXmlStreamNamespaceDeclaration::prefix() const --> class QStringRef");
		cl.def("namespaceUri", (class QStringRef (QXmlStreamNamespaceDeclaration::*)() const) &QXmlStreamNamespaceDeclaration::namespaceUri, "C++: QXmlStreamNamespaceDeclaration::namespaceUri() const --> class QStringRef");
		cl.def("__eq__", (bool (QXmlStreamNamespaceDeclaration::*)(const class QXmlStreamNamespaceDeclaration &) const) &QXmlStreamNamespaceDeclaration::operator==, "C++: QXmlStreamNamespaceDeclaration::operator==(const class QXmlStreamNamespaceDeclaration &) const --> bool", pybind11::arg("other"));
		cl.def("__ne__", (bool (QXmlStreamNamespaceDeclaration::*)(const class QXmlStreamNamespaceDeclaration &) const) &QXmlStreamNamespaceDeclaration::operator!=, "C++: QXmlStreamNamespaceDeclaration::operator!=(const class QXmlStreamNamespaceDeclaration &) const --> bool", pybind11::arg("other"));
	}
	std::cout << "B631_[QTypeInfo<QXmlStreamNamespaceDeclaration>] ";
	std::cout << "B632_[QXmlStreamNotationDeclaration] ";
	std::cout << "B633_[QTypeInfo<QXmlStreamNotationDeclaration>] ";
	std::cout << "B634_[QXmlStreamEntityDeclaration] ";
	std::cout << "B635_[QTypeInfo<QXmlStreamEntityDeclaration>] ";
	std::cout << "B636_[QXmlStreamEntityResolver] ";
	{ // QXmlStreamEntityResolver file: line:327
		pybind11::class_<QXmlStreamEntityResolver, std::shared_ptr<QXmlStreamEntityResolver>, PyCallBack_QXmlStreamEntityResolver> cl(M(""), "QXmlStreamEntityResolver", "");
		cl.def( pybind11::init( [](){ return new QXmlStreamEntityResolver(); }, [](){ return new PyCallBack_QXmlStreamEntityResolver(); } ) );
		cl.def("resolveEntity", (class QString (QXmlStreamEntityResolver::*)(const class QString &, const class QString &)) &QXmlStreamEntityResolver::resolveEntity, "C++: QXmlStreamEntityResolver::resolveEntity(const class QString &, const class QString &) --> class QString", pybind11::arg("publicId"), pybind11::arg("systemId"));
		cl.def("resolveUndeclaredEntity", (class QString (QXmlStreamEntityResolver::*)(const class QString &)) &QXmlStreamEntityResolver::resolveUndeclaredEntity, "C++: QXmlStreamEntityResolver::resolveUndeclaredEntity(const class QString &) --> class QString", pybind11::arg("name"));
		cl.def("assign", (class QXmlStreamEntityResolver & (QXmlStreamEntityResolver::*)(const class QXmlStreamEntityResolver &)) &QXmlStreamEntityResolver::operator=, "C++: QXmlStreamEntityResolver::operator=(const class QXmlStreamEntityResolver &) --> class QXmlStreamEntityResolver &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
