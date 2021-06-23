#include <QtCore/qbytearray.h> // 
#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qbytearray.h> // QByteArrayDataPtr
#include <QtCore/qbytearray.h> // QByteRef
#include <QtCore/qcborvalue.h> // QCborContainerPrivate
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qcoreevent.h> // 
#include <QtCore/qcoreevent.h> // QChildEvent
#include <QtCore/qcoreevent.h> // QEvent
#include <QtCore/qcoreevent.h> // QTimerEvent
#include <QtCore/qdatetime.h> // QDate
#include <QtCore/qdatetime.h> // QDateTime
#include <QtCore/qdatetime.h> // QTime
#include <QtCore/qfileinfo.h> // QFileInfoPrivate
#include <QtCore/qflags.h> // QFlag
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qhash.h> // QHash
#include <QtCore/qiodevice.h> // 
#include <QtCore/qiodevice.h> // QIODevice
#include <QtCore/qjsonvalue.h> // QJsonValue
#include <QtCore/qlist.h> // QList
#include <QtCore/qlocale.h> // QLocale
#include <QtCore/qlocale.h> // QLocalePrivate
#include <QtCore/qmap.h> // QMap
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::QPairVariantInterfaceImpl
#include <QtCore/qmimetype.h> // QMimeTypePrivate
#include <QtCore/qnamespace.h> // Qt::AspectRatioMode
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::ConnectionType
#include <QtCore/qnamespace.h> // Qt::DropAction
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::ItemFlag
#include <QtCore/qnamespace.h> // Qt::MatchFlag
#include <QtCore/qnamespace.h> // Qt::Orientation
#include <QtCore/qnamespace.h> // Qt::SortOrder
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
#include <QtCore/qnamespace.h> // Qt::TimerType
#include <QtCore/qobject.h> // QObject
#include <QtCore/qobject.h> // QObjectUserData
#include <QtCore/qobjectdefs.h> // QGenericArgument
#include <QtCore/qobjectdefs.h> // QGenericReturnArgument
#include <QtCore/qobjectdefs.h> // QMetaObject
#include <QtCore/qpoint.h> // QPoint
#include <QtCore/qpoint.h> // QPointF
#include <QtCore/qregexp.h> // QRegExp
#include <QtCore/qregularexpression.h> // QRegularExpression
#include <QtCore/qregularexpression.h> // QRegularExpressionMatch
#include <QtCore/qregularexpression.h> // QRegularExpressionMatchIteratorPrivate
#include <QtCore/qregularexpression.h> // QRegularExpressionMatchPrivate
#include <QtCore/qregularexpression.h> // QRegularExpressionPrivate
#include <QtCore/qshareddata.h> // QExplicitlySharedDataPointer
#include <QtCore/qshareddata.h> // QSharedData
#include <QtCore/qshareddata.h> // QSharedDataPointer
#include <QtCore/qshareddata.h> // swap
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

// QIODevice file:QtCore/qiodevice.h line:62
struct PyCallBack_QIODevice : public QIODevice {
	using QIODevice::QIODevice;

	const struct QMetaObject * metaObject() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "metaObject");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const struct QMetaObject *>::value) {
				static pybind11::detail::override_caster_t<const struct QMetaObject *> caster;
				return pybind11::detail::cast_ref<const struct QMetaObject *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const struct QMetaObject *>(std::move(o));
		}
		return QIODevice::metaObject();
	}
	void * qt_metacast(const char * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "qt_metacast");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void *>::value) {
				static pybind11::detail::override_caster_t<void *> caster;
				return pybind11::detail::cast_ref<void *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void *>(std::move(o));
		}
		return QIODevice::qt_metacast(a0);
	}
	bool isSequential() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "isSequential");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QIODevice::isSequential();
	}
	void close() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "close");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return QIODevice::close();
	}
	long long pos() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "pos");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<long long>::value) {
				static pybind11::detail::override_caster_t<long long> caster;
				return pybind11::detail::cast_ref<long long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long long>(std::move(o));
		}
		return QIODevice::pos();
	}
	long long size() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "size");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<long long>::value) {
				static pybind11::detail::override_caster_t<long long> caster;
				return pybind11::detail::cast_ref<long long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long long>(std::move(o));
		}
		return QIODevice::size();
	}
	bool seek(long long a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "seek");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QIODevice::seek(a0);
	}
	bool atEnd() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "atEnd");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QIODevice::atEnd();
	}
	bool reset() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "reset");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QIODevice::reset();
	}
	long long bytesAvailable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "bytesAvailable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<long long>::value) {
				static pybind11::detail::override_caster_t<long long> caster;
				return pybind11::detail::cast_ref<long long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long long>(std::move(o));
		}
		return QIODevice::bytesAvailable();
	}
	long long bytesToWrite() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "bytesToWrite");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<long long>::value) {
				static pybind11::detail::override_caster_t<long long> caster;
				return pybind11::detail::cast_ref<long long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long long>(std::move(o));
		}
		return QIODevice::bytesToWrite();
	}
	bool canReadLine() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "canReadLine");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QIODevice::canReadLine();
	}
	bool waitForReadyRead(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "waitForReadyRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QIODevice::waitForReadyRead(a0);
	}
	bool waitForBytesWritten(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "waitForBytesWritten");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QIODevice::waitForBytesWritten(a0);
	}
	long long readData(char * a0, long long a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "readData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<long long>::value) {
				static pybind11::detail::override_caster_t<long long> caster;
				return pybind11::detail::cast_ref<long long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long long>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"QIODevice::readData\"");
	}
	long long readLineData(char * a0, long long a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "readLineData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<long long>::value) {
				static pybind11::detail::override_caster_t<long long> caster;
				return pybind11::detail::cast_ref<long long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long long>(std::move(o));
		}
		return QIODevice::readLineData(a0, a1);
	}
	long long writeData(const char * a0, long long a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "writeData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<long long>::value) {
				static pybind11::detail::override_caster_t<long long> caster;
				return pybind11::detail::cast_ref<long long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long long>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"QIODevice::writeData\"");
	}
	void timerEvent(class QTimerEvent * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "timerEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "childEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "connectNotify");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const QIODevice *>(this), "disconnectNotify");
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

void bind_unknown_unknown_3(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B374_[QAbstractNativeEventFilter] ";
	{ // QAbstractNativeEventFilter file: line:49
		pybind11::class_<QAbstractNativeEventFilter, std::shared_ptr<QAbstractNativeEventFilter>> cl(M(""), "QAbstractNativeEventFilter", "");
	}
	std::cout << "B375_[QAbstractProxyModel] ";
	std::cout << "B376_[QAbstractState] ";
	std::cout << "B377_[QAbstractTransition] ";
	std::cout << "B378_[const struct QMetaObject * qt_getEnumMetaObject(enum QAbstractTransition::TransitionType)] ";
	std::cout << "B379_[const char * qt_getEnumName(enum QAbstractTransition::TransitionType)] ";
	std::cout << "B380_[QAnimationGroup] ";
	std::cout << "B381_[QBasicTimer] ";
	std::cout << "B382_[QTypeInfo<QBasicTimer>] ";
	std::cout << "B383_[void swap(class QBasicTimer &, class QBasicTimer &)] ";
	std::cout << "B384_[QBitArray] ";
	{ // QBitArray file: line:49
		pybind11::class_<QBitArray, std::shared_ptr<QBitArray>> cl(M(""), "QBitArray", "");
		cl.def( pybind11::init( [](){ return new QBitArray(); } ) );
		cl.def( pybind11::init( [](int const & a0){ return new QBitArray(a0); } ), "doc" , pybind11::arg("size"));
		cl.def( pybind11::init<int, bool>(), pybind11::arg("size"), pybind11::arg("val") );

		cl.def( pybind11::init( [](QBitArray const &o){ return new QBitArray(o); } ) );
		cl.def("assign", (class QBitArray & (QBitArray::*)(const class QBitArray &)) &QBitArray::operator=, "C++: QBitArray::operator=(const class QBitArray &) --> class QBitArray &", pybind11::return_value_policy::automatic, pybind11::arg("other"));
		cl.def("swap", (void (QBitArray::*)(class QBitArray &)) &QBitArray::swap, "C++: QBitArray::swap(class QBitArray &) --> void", pybind11::arg("other"));
		cl.def("size", (int (QBitArray::*)() const) &QBitArray::size, "C++: QBitArray::size() const --> int");
		cl.def("count", (int (QBitArray::*)() const) &QBitArray::count, "C++: QBitArray::count() const --> int");
		cl.def("count", (int (QBitArray::*)(bool) const) &QBitArray::count, "C++: QBitArray::count(bool) const --> int", pybind11::arg("on"));
		cl.def("isEmpty", (bool (QBitArray::*)() const) &QBitArray::isEmpty, "C++: QBitArray::isEmpty() const --> bool");
		cl.def("isNull", (bool (QBitArray::*)() const) &QBitArray::isNull, "C++: QBitArray::isNull() const --> bool");
		cl.def("resize", (void (QBitArray::*)(int)) &QBitArray::resize, "C++: QBitArray::resize(int) --> void", pybind11::arg("size"));
		cl.def("detach", (void (QBitArray::*)()) &QBitArray::detach, "C++: QBitArray::detach() --> void");
		cl.def("isDetached", (bool (QBitArray::*)() const) &QBitArray::isDetached, "C++: QBitArray::isDetached() const --> bool");
		cl.def("clear", (void (QBitArray::*)()) &QBitArray::clear, "C++: QBitArray::clear() --> void");
		cl.def("testBit", (bool (QBitArray::*)(int) const) &QBitArray::testBit, "C++: QBitArray::testBit(int) const --> bool", pybind11::arg("i"));
		cl.def("setBit", (void (QBitArray::*)(int)) &QBitArray::setBit, "C++: QBitArray::setBit(int) --> void", pybind11::arg("i"));
		cl.def("setBit", (void (QBitArray::*)(int, bool)) &QBitArray::setBit, "C++: QBitArray::setBit(int, bool) --> void", pybind11::arg("i"), pybind11::arg("val"));
		cl.def("clearBit", (void (QBitArray::*)(int)) &QBitArray::clearBit, "C++: QBitArray::clearBit(int) --> void", pybind11::arg("i"));
		cl.def("toggleBit", (bool (QBitArray::*)(int)) &QBitArray::toggleBit, "C++: QBitArray::toggleBit(int) --> bool", pybind11::arg("i"));
		cl.def("at", (bool (QBitArray::*)(int) const) &QBitArray::at, "C++: QBitArray::at(int) const --> bool", pybind11::arg("i"));
		cl.def("__getitem__", (class QBitRef (QBitArray::*)(int)) &QBitArray::operator[], "C++: QBitArray::operator[](int) --> class QBitRef", pybind11::arg("i"));
		cl.def("__getitem__", (class QBitRef (QBitArray::*)(unsigned int)) &QBitArray::operator[], "C++: QBitArray::operator[](unsigned int) --> class QBitRef", pybind11::arg("i"));
		cl.def("__eq__", (bool (QBitArray::*)(const class QBitArray &) const) &QBitArray::operator==, "C++: QBitArray::operator==(const class QBitArray &) const --> bool", pybind11::arg("other"));
		cl.def("__ne__", (bool (QBitArray::*)(const class QBitArray &) const) &QBitArray::operator!=, "C++: QBitArray::operator!=(const class QBitArray &) const --> bool", pybind11::arg("other"));
		cl.def("fill", [](QBitArray &o, bool const & a0) -> bool { return o.fill(a0); }, "", pybind11::arg("val"));
		cl.def("fill", (bool (QBitArray::*)(bool, int)) &QBitArray::fill, "C++: QBitArray::fill(bool, int) --> bool", pybind11::arg("val"), pybind11::arg("size"));
		cl.def("fill", (void (QBitArray::*)(bool, int, int)) &QBitArray::fill, "C++: QBitArray::fill(bool, int, int) --> void", pybind11::arg("val"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("truncate", (void (QBitArray::*)(int)) &QBitArray::truncate, "C++: QBitArray::truncate(int) --> void", pybind11::arg("pos"));
		cl.def("bits", (const char * (QBitArray::*)() const) &QBitArray::bits, "C++: QBitArray::bits() const --> const char *", pybind11::return_value_policy::automatic);
		cl.def_static("fromBits", (class QBitArray (*)(const char *, long long)) &QBitArray::fromBits, "C++: QBitArray::fromBits(const char *, long long) --> class QBitArray", pybind11::arg("data"), pybind11::arg("len"));
		cl.def("data_ptr", (struct QTypedArrayData<char> *& (QBitArray::*)()) &QBitArray::data_ptr, "C++: QBitArray::data_ptr() --> struct QTypedArrayData<char> *&", pybind11::return_value_policy::automatic);
	}
	std::cout << "B385_[QBitRef] ";
	{ // QBitRef file: line:144
		pybind11::class_<QBitRef, std::shared_ptr<QBitRef>> cl(M(""), "QBitRef", "");
		cl.def( pybind11::init( [](QBitRef const &o){ return new QBitRef(o); } ) );
		cl.def("assign", (class QBitRef & (QBitRef::*)(const class QBitRef &)) &QBitRef::operator=, "C++: QBitRef::operator=(const class QBitRef &) --> class QBitRef &", pybind11::return_value_policy::automatic, pybind11::arg("val"));
		cl.def("assign", (class QBitRef & (QBitRef::*)(bool)) &QBitRef::operator=, "C++: QBitRef::operator=(bool) --> class QBitRef &", pybind11::return_value_policy::automatic, pybind11::arg("val"));
	}
	std::cout << "B386_[QTypeInfo<QBitArray>] ";
	std::cout << "B387_[void swap(class QBitArray &, class QBitArray &)] ";
	std::cout << "B388_[QIODevice] ";
	{ // QIODevice file:QtCore/qiodevice.h line:62
		pybind11::class_<QIODevice, std::shared_ptr<QIODevice>, PyCallBack_QIODevice, QObject> cl(M(""), "QIODevice", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_QIODevice(); } ) );
		cl.def( pybind11::init<class QObject *>(), pybind11::arg("parent") );


		pybind11::enum_<QIODevice::OpenModeFlag>(cl, "OpenModeFlag", pybind11::arithmetic(), "")
			.value("NotOpen", QIODevice::NotOpen)
			.value("ReadOnly", QIODevice::ReadOnly)
			.value("WriteOnly", QIODevice::WriteOnly)
			.value("ReadWrite", QIODevice::ReadWrite)
			.value("Append", QIODevice::Append)
			.value("Truncate", QIODevice::Truncate)
			.value("Text", QIODevice::Text)
			.value("Unbuffered", QIODevice::Unbuffered)
			.value("NewOnly", QIODevice::NewOnly)
			.value("ExistingOnly", QIODevice::ExistingOnly)
			.export_values();

		cl.def("metaObject", (const struct QMetaObject * (QIODevice::*)() const) &QIODevice::metaObject, "C++: QIODevice::metaObject() const --> const struct QMetaObject *", pybind11::return_value_policy::automatic);
		cl.def("qt_metacast", (void * (QIODevice::*)(const char *)) &QIODevice::qt_metacast, "C++: QIODevice::qt_metacast(const char *) --> void *", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def_static("tr", [](const char * a0) -> QString { return QIODevice::tr(a0); }, "", pybind11::arg("s"));
		cl.def_static("tr", [](const char * a0, const char * a1) -> QString { return QIODevice::tr(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("c"));
		cl.def_static("tr", (class QString (*)(const char *, const char *, int)) &QIODevice::tr, "C++: QIODevice::tr(const char *, const char *, int) --> class QString", pybind11::arg("s"), pybind11::arg("c"), pybind11::arg("n"));
		cl.def_static("trUtf8", [](const char * a0) -> QString { return QIODevice::trUtf8(a0); }, "", pybind11::arg("s"));
		cl.def_static("trUtf8", [](const char * a0, const char * a1) -> QString { return QIODevice::trUtf8(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("c"));
		cl.def_static("trUtf8", (class QString (*)(const char *, const char *, int)) &QIODevice::trUtf8, "C++: QIODevice::trUtf8(const char *, const char *, int) --> class QString", pybind11::arg("s"), pybind11::arg("c"), pybind11::arg("n"));
		cl.def("setTextModeEnabled", (void (QIODevice::*)(bool)) &QIODevice::setTextModeEnabled, "C++: QIODevice::setTextModeEnabled(bool) --> void", pybind11::arg("enabled"));
		cl.def("isTextModeEnabled", (bool (QIODevice::*)() const) &QIODevice::isTextModeEnabled, "C++: QIODevice::isTextModeEnabled() const --> bool");
		cl.def("isOpen", (bool (QIODevice::*)() const) &QIODevice::isOpen, "C++: QIODevice::isOpen() const --> bool");
		cl.def("isReadable", (bool (QIODevice::*)() const) &QIODevice::isReadable, "C++: QIODevice::isReadable() const --> bool");
		cl.def("isWritable", (bool (QIODevice::*)() const) &QIODevice::isWritable, "C++: QIODevice::isWritable() const --> bool");
		cl.def("isSequential", (bool (QIODevice::*)() const) &QIODevice::isSequential, "C++: QIODevice::isSequential() const --> bool");
		cl.def("readChannelCount", (int (QIODevice::*)() const) &QIODevice::readChannelCount, "C++: QIODevice::readChannelCount() const --> int");
		cl.def("writeChannelCount", (int (QIODevice::*)() const) &QIODevice::writeChannelCount, "C++: QIODevice::writeChannelCount() const --> int");
		cl.def("currentReadChannel", (int (QIODevice::*)() const) &QIODevice::currentReadChannel, "C++: QIODevice::currentReadChannel() const --> int");
		cl.def("setCurrentReadChannel", (void (QIODevice::*)(int)) &QIODevice::setCurrentReadChannel, "C++: QIODevice::setCurrentReadChannel(int) --> void", pybind11::arg("channel"));
		cl.def("currentWriteChannel", (int (QIODevice::*)() const) &QIODevice::currentWriteChannel, "C++: QIODevice::currentWriteChannel() const --> int");
		cl.def("setCurrentWriteChannel", (void (QIODevice::*)(int)) &QIODevice::setCurrentWriteChannel, "C++: QIODevice::setCurrentWriteChannel(int) --> void", pybind11::arg("channel"));
		cl.def("close", (void (QIODevice::*)()) &QIODevice::close, "C++: QIODevice::close() --> void");
		cl.def("pos", (long long (QIODevice::*)() const) &QIODevice::pos, "C++: QIODevice::pos() const --> long long");
		cl.def("size", (long long (QIODevice::*)() const) &QIODevice::size, "C++: QIODevice::size() const --> long long");
		cl.def("seek", (bool (QIODevice::*)(long long)) &QIODevice::seek, "C++: QIODevice::seek(long long) --> bool", pybind11::arg("pos"));
		cl.def("atEnd", (bool (QIODevice::*)() const) &QIODevice::atEnd, "C++: QIODevice::atEnd() const --> bool");
		cl.def("reset", (bool (QIODevice::*)()) &QIODevice::reset, "C++: QIODevice::reset() --> bool");
		cl.def("bytesAvailable", (long long (QIODevice::*)() const) &QIODevice::bytesAvailable, "C++: QIODevice::bytesAvailable() const --> long long");
		cl.def("bytesToWrite", (long long (QIODevice::*)() const) &QIODevice::bytesToWrite, "C++: QIODevice::bytesToWrite() const --> long long");
		cl.def("read", (long long (QIODevice::*)(char *, long long)) &QIODevice::read, "C++: QIODevice::read(char *, long long) --> long long", pybind11::arg("data"), pybind11::arg("maxlen"));
		cl.def("readLine", (long long (QIODevice::*)(char *, long long)) &QIODevice::readLine, "C++: QIODevice::readLine(char *, long long) --> long long", pybind11::arg("data"), pybind11::arg("maxlen"));
		cl.def("canReadLine", (bool (QIODevice::*)() const) &QIODevice::canReadLine, "C++: QIODevice::canReadLine() const --> bool");
		cl.def("startTransaction", (void (QIODevice::*)()) &QIODevice::startTransaction, "C++: QIODevice::startTransaction() --> void");
		cl.def("commitTransaction", (void (QIODevice::*)()) &QIODevice::commitTransaction, "C++: QIODevice::commitTransaction() --> void");
		cl.def("rollbackTransaction", (void (QIODevice::*)()) &QIODevice::rollbackTransaction, "C++: QIODevice::rollbackTransaction() --> void");
		cl.def("isTransactionStarted", (bool (QIODevice::*)() const) &QIODevice::isTransactionStarted, "C++: QIODevice::isTransactionStarted() const --> bool");
		cl.def("write", (long long (QIODevice::*)(const char *, long long)) &QIODevice::write, "C++: QIODevice::write(const char *, long long) --> long long", pybind11::arg("data"), pybind11::arg("len"));
		cl.def("write", (long long (QIODevice::*)(const char *)) &QIODevice::write, "C++: QIODevice::write(const char *) --> long long", pybind11::arg("data"));
		cl.def("peek", (long long (QIODevice::*)(char *, long long)) &QIODevice::peek, "C++: QIODevice::peek(char *, long long) --> long long", pybind11::arg("data"), pybind11::arg("maxlen"));
		cl.def("skip", (long long (QIODevice::*)(long long)) &QIODevice::skip, "C++: QIODevice::skip(long long) --> long long", pybind11::arg("maxSize"));
		cl.def("waitForReadyRead", (bool (QIODevice::*)(int)) &QIODevice::waitForReadyRead, "C++: QIODevice::waitForReadyRead(int) --> bool", pybind11::arg("msecs"));
		cl.def("waitForBytesWritten", (bool (QIODevice::*)(int)) &QIODevice::waitForBytesWritten, "C++: QIODevice::waitForBytesWritten(int) --> bool", pybind11::arg("msecs"));
		cl.def("ungetChar", (void (QIODevice::*)(char)) &QIODevice::ungetChar, "C++: QIODevice::ungetChar(char) --> void", pybind11::arg("c"));
		cl.def("putChar", (bool (QIODevice::*)(char)) &QIODevice::putChar, "C++: QIODevice::putChar(char) --> bool", pybind11::arg("c"));
		cl.def("getChar", (bool (QIODevice::*)(char *)) &QIODevice::getChar, "C++: QIODevice::getChar(char *) --> bool", pybind11::arg("c"));
		cl.def("errorString", (class QString (QIODevice::*)() const) &QIODevice::errorString, "C++: QIODevice::errorString() const --> class QString");
		cl.def("readyRead", (void (QIODevice::*)()) &QIODevice::readyRead, "C++: QIODevice::readyRead() --> void");
		cl.def("channelReadyRead", (void (QIODevice::*)(int)) &QIODevice::channelReadyRead, "C++: QIODevice::channelReadyRead(int) --> void", pybind11::arg("channel"));
		cl.def("bytesWritten", (void (QIODevice::*)(long long)) &QIODevice::bytesWritten, "C++: QIODevice::bytesWritten(long long) --> void", pybind11::arg("bytes"));
		cl.def("channelBytesWritten", (void (QIODevice::*)(int, long long)) &QIODevice::channelBytesWritten, "C++: QIODevice::channelBytesWritten(int, long long) --> void", pybind11::arg("channel"), pybind11::arg("bytes"));
		cl.def("aboutToClose", (void (QIODevice::*)()) &QIODevice::aboutToClose, "C++: QIODevice::aboutToClose() --> void");
		cl.def("readChannelFinished", (void (QIODevice::*)()) &QIODevice::readChannelFinished, "C++: QIODevice::readChannelFinished() --> void");
	}
	std::cout << "B389_[QBuffer] ";
	std::cout << "B390_[QByteArrayMatcher] ";
	std::cout << "B391_[QStaticByteArrayMatcherBase] ";
	std::cout << "B392_[QSharedDataPointer<QLocalePrivate>] ";
	std::cout << "B393_[QSharedDataPointer<QRegularExpressionMatchPrivate>] ";
	std::cout << "B394_[QSharedDataPointer<QRegularExpressionMatchIteratorPrivate>] ";
	std::cout << "B395_[QSharedDataPointer<QCommandLineOptionPrivate>] ";
	std::cout << "B396_[QSharedDataPointer<QFileInfoPrivate>] ";
	std::cout << "B397_[QSharedDataPointer<QDirPrivate>] ";
	std::cout << "B398_[QSharedDataPointer<QProcessEnvironmentPrivate>] ";
	std::cout << "B399_[QSharedDataPointer<QTimeZonePrivate>] ";
	std::cout << "B400_[QSharedDataPointer<QUrlQueryPrivate>] ";
	std::cout << "B401_[QSharedData] ";
	std::cout << "B402_[QExplicitlySharedDataPointer<QRegularExpressionPrivate>] ";
	std::cout << "B403_[QExplicitlySharedDataPointer<QCborContainerPrivate>] ";
	std::cout << "B404_[QExplicitlySharedDataPointer<QCollatorSortKeyPrivate>] ";
	std::cout << "B405_[QExplicitlySharedDataPointer<QMimeTypePrivate>] ";
	std::cout << "B406_[QExplicitlySharedDataPointer<QStorageInfoPrivate>] ";
	std::cout << "B407_[void swap<QLocalePrivate>(class QSharedDataPointer<class QLocalePrivate> &, class QSharedDataPointer<class QLocalePrivate> &)] ";
	std::cout << "B408_[void swap<QCommandLineOptionPrivate>(class QSharedDataPointer<class QCommandLineOptionPrivate> &, class QSharedDataPointer<class QCommandLineOptionPrivate> &)] ";
	std::cout << "B409_[void swap<QFileInfoPrivate>(class QSharedDataPointer<class QFileInfoPrivate> &, class QSharedDataPointer<class QFileInfoPrivate> &)] ";
}
