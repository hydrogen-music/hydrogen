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
#include <QtCore/qdatetime.h> // 
#include <QtCore/qdatetime.h> // QDate
#include <QtCore/qdatetime.h> // QDateTime
#include <QtCore/qdatetime.h> // QTime
#include <QtCore/qfile.h> // QFile
#include <QtCore/qfiledevice.h> // 
#include <QtCore/qfileinfo.h> // QFileInfo
#include <QtCore/qflags.h> // QFlag
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qhash.h> // QHash
#include <QtCore/qiodevice.h> // 
#include <QtCore/qiodevice.h> // QIODevice
#include <QtCore/qjsonvalue.h> // 
#include <QtCore/qjsonvalue.h> // QJsonValue
#include <QtCore/qjsonvalue.h> // QJsonValueRef
#include <QtCore/qlist.h> // QList
#include <QtCore/qlocale.h> // 
#include <QtCore/qlocale.h> // QLocale
#include <QtCore/qmap.h> // QMap
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::QPairVariantInterfaceImpl
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::VariantData
#include <QtCore/qmimetype.h> // QMimeType
#include <QtCore/qmimetype.h> // qHash
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
#include <QtCore/qvariantanimation.h> // QVariantAnimation
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

// QMimeData file: line:52
struct PyCallBack_QMimeData : public QMimeData {
	using QMimeData::QMimeData;

	const struct QMetaObject * metaObject() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QMimeData *>(this), "metaObject");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const struct QMetaObject *>::value) {
				static pybind11::detail::override_caster_t<const struct QMetaObject *> caster;
				return pybind11::detail::cast_ref<const struct QMetaObject *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const struct QMetaObject *>(std::move(o));
		}
		return QMimeData::metaObject();
	}
	void * qt_metacast(const char * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QMimeData *>(this), "qt_metacast");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void *>::value) {
				static pybind11::detail::override_caster_t<void *> caster;
				return pybind11::detail::cast_ref<void *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void *>(std::move(o));
		}
		return QMimeData::qt_metacast(a0);
	}
	bool hasFormat(const class QString & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QMimeData *>(this), "hasFormat");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QMimeData::hasFormat(a0);
	}
	class QVariant retrieveData(const class QString & a0, enum QVariant::Type a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QMimeData *>(this), "retrieveData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QVariant>::value) {
				static pybind11::detail::override_caster_t<class QVariant> caster;
				return pybind11::detail::cast_ref<class QVariant>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QVariant>(std::move(o));
		}
		return QMimeData::retrieveData(a0, a1);
	}
	void timerEvent(class QTimerEvent * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QMimeData *>(this), "timerEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const QMimeData *>(this), "childEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const QMimeData *>(this), "connectNotify");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const QMimeData *>(this), "disconnectNotify");
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

void bind_unknown_unknown_9(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B524_[QMetaEnum] ";
	{ // QMetaEnum file: line:205
		pybind11::class_<QMetaEnum, std::shared_ptr<QMetaEnum>> cl(M(""), "QMetaEnum", "");
		cl.def( pybind11::init( [](){ return new QMetaEnum(); } ) );
		cl.def( pybind11::init( [](QMetaEnum const &o){ return new QMetaEnum(o); } ) );
		cl.def("name", (const char * (QMetaEnum::*)() const) &QMetaEnum::name, "C++: QMetaEnum::name() const --> const char *", pybind11::return_value_policy::automatic);
		cl.def("enumName", (const char * (QMetaEnum::*)() const) &QMetaEnum::enumName, "C++: QMetaEnum::enumName() const --> const char *", pybind11::return_value_policy::automatic);
		cl.def("isFlag", (bool (QMetaEnum::*)() const) &QMetaEnum::isFlag, "C++: QMetaEnum::isFlag() const --> bool");
		cl.def("isScoped", (bool (QMetaEnum::*)() const) &QMetaEnum::isScoped, "C++: QMetaEnum::isScoped() const --> bool");
		cl.def("keyCount", (int (QMetaEnum::*)() const) &QMetaEnum::keyCount, "C++: QMetaEnum::keyCount() const --> int");
		cl.def("key", (const char * (QMetaEnum::*)(int) const) &QMetaEnum::key, "C++: QMetaEnum::key(int) const --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("index"));
		cl.def("value", (int (QMetaEnum::*)(int) const) &QMetaEnum::value, "C++: QMetaEnum::value(int) const --> int", pybind11::arg("index"));
		cl.def("scope", (const char * (QMetaEnum::*)() const) &QMetaEnum::scope, "C++: QMetaEnum::scope() const --> const char *", pybind11::return_value_policy::automatic);
		cl.def("keyToValue", [](QMetaEnum const &o, const char * a0) -> int { return o.keyToValue(a0); }, "", pybind11::arg("key"));
		cl.def("keyToValue", (int (QMetaEnum::*)(const char *, bool *) const) &QMetaEnum::keyToValue, "C++: QMetaEnum::keyToValue(const char *, bool *) const --> int", pybind11::arg("key"), pybind11::arg("ok"));
		cl.def("valueToKey", (const char * (QMetaEnum::*)(int) const) &QMetaEnum::valueToKey, "C++: QMetaEnum::valueToKey(int) const --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("value"));
		cl.def("keysToValue", [](QMetaEnum const &o, const char * a0) -> int { return o.keysToValue(a0); }, "", pybind11::arg("keys"));
		cl.def("keysToValue", (int (QMetaEnum::*)(const char *, bool *) const) &QMetaEnum::keysToValue, "C++: QMetaEnum::keysToValue(const char *, bool *) const --> int", pybind11::arg("keys"), pybind11::arg("ok"));
		cl.def("enclosingMetaObject", (const struct QMetaObject * (QMetaEnum::*)() const) &QMetaEnum::enclosingMetaObject, "C++: QMetaEnum::enclosingMetaObject() const --> const struct QMetaObject *", pybind11::return_value_policy::automatic);
		cl.def("isValid", (bool (QMetaEnum::*)() const) &QMetaEnum::isValid, "C++: QMetaEnum::isValid() const --> bool");
	}
	std::cout << "B525_[QTypeInfo<QMetaEnum>] ";
	std::cout << "B526_[QMetaProperty] ";
	{ // QMetaProperty file: line:246
		pybind11::class_<QMetaProperty, std::shared_ptr<QMetaProperty>> cl(M(""), "QMetaProperty", "");
		cl.def( pybind11::init( [](){ return new QMetaProperty(); } ) );
		cl.def("name", (const char * (QMetaProperty::*)() const) &QMetaProperty::name, "C++: QMetaProperty::name() const --> const char *", pybind11::return_value_policy::automatic);
		cl.def("typeName", (const char * (QMetaProperty::*)() const) &QMetaProperty::typeName, "C++: QMetaProperty::typeName() const --> const char *", pybind11::return_value_policy::automatic);
		cl.def("type", (enum QVariant::Type (QMetaProperty::*)() const) &QMetaProperty::type, "C++: QMetaProperty::type() const --> enum QVariant::Type");
		cl.def("userType", (int (QMetaProperty::*)() const) &QMetaProperty::userType, "C++: QMetaProperty::userType() const --> int");
		cl.def("propertyIndex", (int (QMetaProperty::*)() const) &QMetaProperty::propertyIndex, "C++: QMetaProperty::propertyIndex() const --> int");
		cl.def("relativePropertyIndex", (int (QMetaProperty::*)() const) &QMetaProperty::relativePropertyIndex, "C++: QMetaProperty::relativePropertyIndex() const --> int");
		cl.def("isReadable", (bool (QMetaProperty::*)() const) &QMetaProperty::isReadable, "C++: QMetaProperty::isReadable() const --> bool");
		cl.def("isWritable", (bool (QMetaProperty::*)() const) &QMetaProperty::isWritable, "C++: QMetaProperty::isWritable() const --> bool");
		cl.def("isResettable", (bool (QMetaProperty::*)() const) &QMetaProperty::isResettable, "C++: QMetaProperty::isResettable() const --> bool");
		cl.def("isDesignable", [](QMetaProperty const &o) -> bool { return o.isDesignable(); }, "");
		cl.def("isDesignable", (bool (QMetaProperty::*)(const class QObject *) const) &QMetaProperty::isDesignable, "C++: QMetaProperty::isDesignable(const class QObject *) const --> bool", pybind11::arg("obj"));
		cl.def("isScriptable", [](QMetaProperty const &o) -> bool { return o.isScriptable(); }, "");
		cl.def("isScriptable", (bool (QMetaProperty::*)(const class QObject *) const) &QMetaProperty::isScriptable, "C++: QMetaProperty::isScriptable(const class QObject *) const --> bool", pybind11::arg("obj"));
		cl.def("isStored", [](QMetaProperty const &o) -> bool { return o.isStored(); }, "");
		cl.def("isStored", (bool (QMetaProperty::*)(const class QObject *) const) &QMetaProperty::isStored, "C++: QMetaProperty::isStored(const class QObject *) const --> bool", pybind11::arg("obj"));
		cl.def("isEditable", [](QMetaProperty const &o) -> bool { return o.isEditable(); }, "");
		cl.def("isEditable", (bool (QMetaProperty::*)(const class QObject *) const) &QMetaProperty::isEditable, "C++: QMetaProperty::isEditable(const class QObject *) const --> bool", pybind11::arg("obj"));
		cl.def("isUser", [](QMetaProperty const &o) -> bool { return o.isUser(); }, "");
		cl.def("isUser", (bool (QMetaProperty::*)(const class QObject *) const) &QMetaProperty::isUser, "C++: QMetaProperty::isUser(const class QObject *) const --> bool", pybind11::arg("obj"));
		cl.def("isConstant", (bool (QMetaProperty::*)() const) &QMetaProperty::isConstant, "C++: QMetaProperty::isConstant() const --> bool");
		cl.def("isFinal", (bool (QMetaProperty::*)() const) &QMetaProperty::isFinal, "C++: QMetaProperty::isFinal() const --> bool");
		cl.def("isRequired", (bool (QMetaProperty::*)() const) &QMetaProperty::isRequired, "C++: QMetaProperty::isRequired() const --> bool");
		cl.def("isFlagType", (bool (QMetaProperty::*)() const) &QMetaProperty::isFlagType, "C++: QMetaProperty::isFlagType() const --> bool");
		cl.def("isEnumType", (bool (QMetaProperty::*)() const) &QMetaProperty::isEnumType, "C++: QMetaProperty::isEnumType() const --> bool");
		cl.def("enumerator", (class QMetaEnum (QMetaProperty::*)() const) &QMetaProperty::enumerator, "C++: QMetaProperty::enumerator() const --> class QMetaEnum");
		cl.def("hasNotifySignal", (bool (QMetaProperty::*)() const) &QMetaProperty::hasNotifySignal, "C++: QMetaProperty::hasNotifySignal() const --> bool");
		cl.def("notifySignal", (class QMetaMethod (QMetaProperty::*)() const) &QMetaProperty::notifySignal, "C++: QMetaProperty::notifySignal() const --> class QMetaMethod");
		cl.def("notifySignalIndex", (int (QMetaProperty::*)() const) &QMetaProperty::notifySignalIndex, "C++: QMetaProperty::notifySignalIndex() const --> int");
		cl.def("revision", (int (QMetaProperty::*)() const) &QMetaProperty::revision, "C++: QMetaProperty::revision() const --> int");
		cl.def("read", (class QVariant (QMetaProperty::*)(const class QObject *) const) &QMetaProperty::read, "C++: QMetaProperty::read(const class QObject *) const --> class QVariant", pybind11::arg("obj"));
		cl.def("write", (bool (QMetaProperty::*)(class QObject *, const class QVariant &) const) &QMetaProperty::write, "C++: QMetaProperty::write(class QObject *, const class QVariant &) const --> bool", pybind11::arg("obj"), pybind11::arg("value"));
		cl.def("reset", (bool (QMetaProperty::*)(class QObject *) const) &QMetaProperty::reset, "C++: QMetaProperty::reset(class QObject *) const --> bool", pybind11::arg("obj"));
		cl.def("readOnGadget", (class QVariant (QMetaProperty::*)(const void *) const) &QMetaProperty::readOnGadget, "C++: QMetaProperty::readOnGadget(const void *) const --> class QVariant", pybind11::arg("gadget"));
		cl.def("writeOnGadget", (bool (QMetaProperty::*)(void *, const class QVariant &) const) &QMetaProperty::writeOnGadget, "C++: QMetaProperty::writeOnGadget(void *, const class QVariant &) const --> bool", pybind11::arg("gadget"), pybind11::arg("value"));
		cl.def("resetOnGadget", (bool (QMetaProperty::*)(void *) const) &QMetaProperty::resetOnGadget, "C++: QMetaProperty::resetOnGadget(void *) const --> bool", pybind11::arg("gadget"));
		cl.def("hasStdCppSet", (bool (QMetaProperty::*)() const) &QMetaProperty::hasStdCppSet, "C++: QMetaProperty::hasStdCppSet() const --> bool");
		cl.def("isValid", (bool (QMetaProperty::*)() const) &QMetaProperty::isValid, "C++: QMetaProperty::isValid() const --> bool");
		cl.def("enclosingMetaObject", (const struct QMetaObject * (QMetaProperty::*)() const) &QMetaProperty::enclosingMetaObject, "C++: QMetaProperty::enclosingMetaObject() const --> const struct QMetaObject *", pybind11::return_value_policy::automatic);
	}
	std::cout << "B527_[QMetaClassInfo] ";
	{ // QMetaClassInfo file: line:305
		pybind11::class_<QMetaClassInfo, std::shared_ptr<QMetaClassInfo>> cl(M(""), "QMetaClassInfo", "");
		cl.def( pybind11::init( [](){ return new QMetaClassInfo(); } ) );
		cl.def("name", (const char * (QMetaClassInfo::*)() const) &QMetaClassInfo::name, "C++: QMetaClassInfo::name() const --> const char *", pybind11::return_value_policy::automatic);
		cl.def("value", (const char * (QMetaClassInfo::*)() const) &QMetaClassInfo::value, "C++: QMetaClassInfo::value() const --> const char *", pybind11::return_value_policy::automatic);
		cl.def("enclosingMetaObject", (const struct QMetaObject * (QMetaClassInfo::*)() const) &QMetaClassInfo::enclosingMetaObject, "C++: QMetaClassInfo::enclosingMetaObject() const --> const struct QMetaObject *", pybind11::return_value_policy::automatic);
	}
	std::cout << "B528_[QTypeInfo<QMetaClassInfo>] ";
	std::cout << "B529_[QMimeData] ";
	{ // QMimeData file: line:52
		pybind11::class_<QMimeData, std::shared_ptr<QMimeData>, PyCallBack_QMimeData, QObject> cl(M(""), "QMimeData", "");
		cl.def( pybind11::init( [](){ return new QMimeData(); }, [](){ return new PyCallBack_QMimeData(); } ) );
		cl.def("metaObject", (const struct QMetaObject * (QMimeData::*)() const) &QMimeData::metaObject, "C++: QMimeData::metaObject() const --> const struct QMetaObject *", pybind11::return_value_policy::automatic);
		cl.def("qt_metacast", (void * (QMimeData::*)(const char *)) &QMimeData::qt_metacast, "C++: QMimeData::qt_metacast(const char *) --> void *", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def_static("tr", [](const char * a0) -> QString { return QMimeData::tr(a0); }, "", pybind11::arg("s"));
		cl.def_static("tr", [](const char * a0, const char * a1) -> QString { return QMimeData::tr(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("c"));
		cl.def_static("tr", (class QString (*)(const char *, const char *, int)) &QMimeData::tr, "C++: QMimeData::tr(const char *, const char *, int) --> class QString", pybind11::arg("s"), pybind11::arg("c"), pybind11::arg("n"));
		cl.def_static("trUtf8", [](const char * a0) -> QString { return QMimeData::trUtf8(a0); }, "", pybind11::arg("s"));
		cl.def_static("trUtf8", [](const char * a0, const char * a1) -> QString { return QMimeData::trUtf8(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("c"));
		cl.def_static("trUtf8", (class QString (*)(const char *, const char *, int)) &QMimeData::trUtf8, "C++: QMimeData::trUtf8(const char *, const char *, int) --> class QString", pybind11::arg("s"), pybind11::arg("c"), pybind11::arg("n"));
		cl.def("hasUrls", (bool (QMimeData::*)() const) &QMimeData::hasUrls, "C++: QMimeData::hasUrls() const --> bool");
		cl.def("text", (class QString (QMimeData::*)() const) &QMimeData::text, "C++: QMimeData::text() const --> class QString");
		cl.def("setText", (void (QMimeData::*)(const class QString &)) &QMimeData::setText, "C++: QMimeData::setText(const class QString &) --> void", pybind11::arg("text"));
		cl.def("hasText", (bool (QMimeData::*)() const) &QMimeData::hasText, "C++: QMimeData::hasText() const --> bool");
		cl.def("html", (class QString (QMimeData::*)() const) &QMimeData::html, "C++: QMimeData::html() const --> class QString");
		cl.def("setHtml", (void (QMimeData::*)(const class QString &)) &QMimeData::setHtml, "C++: QMimeData::setHtml(const class QString &) --> void", pybind11::arg("html"));
		cl.def("hasHtml", (bool (QMimeData::*)() const) &QMimeData::hasHtml, "C++: QMimeData::hasHtml() const --> bool");
		cl.def("imageData", (class QVariant (QMimeData::*)() const) &QMimeData::imageData, "C++: QMimeData::imageData() const --> class QVariant");
		cl.def("setImageData", (void (QMimeData::*)(const class QVariant &)) &QMimeData::setImageData, "C++: QMimeData::setImageData(const class QVariant &) --> void", pybind11::arg("image"));
		cl.def("hasImage", (bool (QMimeData::*)() const) &QMimeData::hasImage, "C++: QMimeData::hasImage() const --> bool");
		cl.def("colorData", (class QVariant (QMimeData::*)() const) &QMimeData::colorData, "C++: QMimeData::colorData() const --> class QVariant");
		cl.def("setColorData", (void (QMimeData::*)(const class QVariant &)) &QMimeData::setColorData, "C++: QMimeData::setColorData(const class QVariant &) --> void", pybind11::arg("color"));
		cl.def("hasColor", (bool (QMimeData::*)() const) &QMimeData::hasColor, "C++: QMimeData::hasColor() const --> bool");
		cl.def("removeFormat", (void (QMimeData::*)(const class QString &)) &QMimeData::removeFormat, "C++: QMimeData::removeFormat(const class QString &) --> void", pybind11::arg("mimetype"));
		cl.def("hasFormat", (bool (QMimeData::*)(const class QString &) const) &QMimeData::hasFormat, "C++: QMimeData::hasFormat(const class QString &) const --> bool", pybind11::arg("mimetype"));
		cl.def("clear", (void (QMimeData::*)()) &QMimeData::clear, "C++: QMimeData::clear() --> void");
	}
	std::cout << "B530_[unsigned int qHash(const class QMimeType &, unsigned int)] ";
	std::cout << "B531_[QMimeType] ";
	std::cout << "B532_[QTypeInfo<QMimeType>] ";
	std::cout << "B533_[void swap(class QMimeType &, class QMimeType &)] ";
	std::cout << "B534_[QMimeDatabase] ";
	std::cout << "B535_[QObjectCleanupHandler] ";
	std::cout << "B536_[QOperatingSystemVersion] ";
	std::cout << "B537_[QTypeInfo<QOperatingSystemVersion>] ";
	std::cout << "B538_[QParallelAnimationGroup] ";
	std::cout << "B539_[QPauseAnimation] ";
	std::cout << "B540_[unsigned char qPluginArchRequirements()] ";
	std::cout << "B541_[QStaticPlugin] ";
	std::cout << "B542_[QTypeInfo<QStaticPlugin>] ";
	std::cout << "B543_[void qRegisterStaticPluginFunction(struct QStaticPlugin)] ";
	std::cout << "B544_[QPluginLoader] ";
	std::cout << "B545_[QProcessEnvironment] ";
	std::cout << "B546_[QTypeInfo<QProcessEnvironment>] ";
	std::cout << "B547_[void swap(class QProcessEnvironment &, class QProcessEnvironment &)] ";
	std::cout << "B548_[QProcess] ";
	std::cout << "B549_[const struct QMetaObject * qt_getEnumMetaObject(enum QProcess::ProcessError)] ";
	std::cout << "B550_[const char * qt_getEnumName(enum QProcess::ProcessError)] ";
	std::cout << "B551_[const struct QMetaObject * qt_getEnumMetaObject(enum QProcess::ProcessState)] ";
	std::cout << "B552_[const char * qt_getEnumName(enum QProcess::ProcessState)] ";
	std::cout << "B553_[const struct QMetaObject * qt_getEnumMetaObject(enum QProcess::ProcessChannel)] ";
	std::cout << "B554_[const char * qt_getEnumName(enum QProcess::ProcessChannel)] ";
	std::cout << "B555_[const struct QMetaObject * qt_getEnumMetaObject(enum QProcess::ProcessChannelMode)] ";
	std::cout << "B556_[const char * qt_getEnumName(enum QProcess::ProcessChannelMode)] ";
	std::cout << "B557_[const struct QMetaObject * qt_getEnumMetaObject(enum QProcess::InputChannelMode)] ";
	std::cout << "B558_[const char * qt_getEnumName(enum QProcess::InputChannelMode)] ";
	std::cout << "B559_[const struct QMetaObject * qt_getEnumMetaObject(enum QProcess::ExitStatus)] ";
	std::cout << "B560_[const char * qt_getEnumName(enum QProcess::ExitStatus)] ";
	std::cout << "B561_[QVariantAnimation] ";
	std::cout << "B562_[QPropertyAnimation] ";
}
