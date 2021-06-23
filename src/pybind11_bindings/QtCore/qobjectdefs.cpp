#include <QtCore/qbytearray.h> // 
#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qbytearray.h> // QByteArrayDataPtr
#include <QtCore/qbytearray.h> // QByteRef
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qcoreevent.h> // QEvent
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qlist.h> // QList
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::QPairVariantInterfaceImpl
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::ConnectionType
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
#include <QtCore/qnamespace.h> // Qt::TimerType
#include <QtCore/qobject.h> // QObject
#include <QtCore/qobject.h> // QObjectUserData
#include <QtCore/qobjectdefs.h> // QGenericArgument
#include <QtCore/qobjectdefs.h> // QGenericReturnArgument
#include <QtCore/qobjectdefs.h> // QMetaObject
#include <QtCore/qobjectdefs.h> // QMetaObject::SuperData
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

void bind_QtCore_qobjectdefs(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B297_[QGenericArgument] ";
	{ // QGenericArgument file:QtCore/qobjectdefs.h line:289
		pybind11::class_<QGenericArgument, std::shared_ptr<QGenericArgument>> cl(M(""), "QGenericArgument", "");
		cl.def( pybind11::init( [](){ return new QGenericArgument(); } ), "doc" );
		cl.def( pybind11::init( [](const char * a0){ return new QGenericArgument(a0); } ), "doc" , pybind11::arg("aName"));
		cl.def( pybind11::init<const char *, const void *>(), pybind11::arg("aName"), pybind11::arg("aData") );

		cl.def( pybind11::init( [](QGenericArgument const &o){ return new QGenericArgument(o); } ) );
		cl.def("data", (void * (QGenericArgument::*)() const) &QGenericArgument::data, "C++: QGenericArgument::data() const --> void *", pybind11::return_value_policy::automatic);
		cl.def("name", (const char * (QGenericArgument::*)() const) &QGenericArgument::name, "C++: QGenericArgument::name() const --> const char *", pybind11::return_value_policy::automatic);
	}
	std::cout << "B298_[QGenericReturnArgument] ";
	{ // QGenericReturnArgument file:QtCore/qobjectdefs.h line:302
		pybind11::class_<QGenericReturnArgument, std::shared_ptr<QGenericReturnArgument>, QGenericArgument> cl(M(""), "QGenericReturnArgument", "");
		cl.def( pybind11::init( [](){ return new QGenericReturnArgument(); } ), "doc" );
		cl.def( pybind11::init( [](const char * a0){ return new QGenericReturnArgument(a0); } ), "doc" , pybind11::arg("aName"));
		cl.def( pybind11::init<const char *, void *>(), pybind11::arg("aName"), pybind11::arg("aData") );

		cl.def( pybind11::init( [](QGenericReturnArgument const &o){ return new QGenericReturnArgument(o); } ) );
	}
	std::cout << "B299_[QMetaObject] ";
	{ // QMetaObject file:QtCore/qobjectdefs.h line:337
		pybind11::class_<QMetaObject, std::shared_ptr<QMetaObject>> cl(M(""), "QMetaObject", "");
		cl.def( pybind11::init( [](){ return new QMetaObject(); } ) );
		cl.def( pybind11::init( [](QMetaObject const &o){ return new QMetaObject(o); } ) );

		pybind11::enum_<QMetaObject::Call>(cl, "Call", pybind11::arithmetic(), "")
			.value("InvokeMetaMethod", QMetaObject::InvokeMetaMethod)
			.value("ReadProperty", QMetaObject::ReadProperty)
			.value("WriteProperty", QMetaObject::WriteProperty)
			.value("ResetProperty", QMetaObject::ResetProperty)
			.value("QueryPropertyDesignable", QMetaObject::QueryPropertyDesignable)
			.value("QueryPropertyScriptable", QMetaObject::QueryPropertyScriptable)
			.value("QueryPropertyStored", QMetaObject::QueryPropertyStored)
			.value("QueryPropertyEditable", QMetaObject::QueryPropertyEditable)
			.value("QueryPropertyUser", QMetaObject::QueryPropertyUser)
			.value("CreateInstance", QMetaObject::CreateInstance)
			.value("IndexOfMethod", QMetaObject::IndexOfMethod)
			.value("RegisterPropertyMetaType", QMetaObject::RegisterPropertyMetaType)
			.value("RegisterMethodArgumentMetaType", QMetaObject::RegisterMethodArgumentMetaType)
			.export_values();

		cl.def("className", (const char * (QMetaObject::*)() const) &QMetaObject::className, "C++: QMetaObject::className() const --> const char *", pybind11::return_value_policy::automatic);
		cl.def("superClass", (const struct QMetaObject * (QMetaObject::*)() const) &QMetaObject::superClass, "C++: QMetaObject::superClass() const --> const struct QMetaObject *", pybind11::return_value_policy::automatic);
		cl.def("inherits", (bool (QMetaObject::*)(const struct QMetaObject *) const) &QMetaObject::inherits, "C++: QMetaObject::inherits(const struct QMetaObject *) const --> bool", pybind11::arg("metaObject"));
		cl.def("cast", (class QObject * (QMetaObject::*)(class QObject *) const) &QMetaObject::cast, "C++: QMetaObject::cast(class QObject *) const --> class QObject *", pybind11::return_value_policy::automatic, pybind11::arg("obj"));
		cl.def("cast", (const class QObject * (QMetaObject::*)(const class QObject *) const) &QMetaObject::cast, "C++: QMetaObject::cast(const class QObject *) const --> const class QObject *", pybind11::return_value_policy::automatic, pybind11::arg("obj"));
		cl.def("tr", [](QMetaObject const &o, const char * a0, const char * a1) -> QString { return o.tr(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("c"));
		cl.def("tr", (class QString (QMetaObject::*)(const char *, const char *, int) const) &QMetaObject::tr, "C++: QMetaObject::tr(const char *, const char *, int) const --> class QString", pybind11::arg("s"), pybind11::arg("c"), pybind11::arg("n"));
		cl.def("methodOffset", (int (QMetaObject::*)() const) &QMetaObject::methodOffset, "C++: QMetaObject::methodOffset() const --> int");
		cl.def("enumeratorOffset", (int (QMetaObject::*)() const) &QMetaObject::enumeratorOffset, "C++: QMetaObject::enumeratorOffset() const --> int");
		cl.def("propertyOffset", (int (QMetaObject::*)() const) &QMetaObject::propertyOffset, "C++: QMetaObject::propertyOffset() const --> int");
		cl.def("classInfoOffset", (int (QMetaObject::*)() const) &QMetaObject::classInfoOffset, "C++: QMetaObject::classInfoOffset() const --> int");
		cl.def("constructorCount", (int (QMetaObject::*)() const) &QMetaObject::constructorCount, "C++: QMetaObject::constructorCount() const --> int");
		cl.def("methodCount", (int (QMetaObject::*)() const) &QMetaObject::methodCount, "C++: QMetaObject::methodCount() const --> int");
		cl.def("enumeratorCount", (int (QMetaObject::*)() const) &QMetaObject::enumeratorCount, "C++: QMetaObject::enumeratorCount() const --> int");
		cl.def("propertyCount", (int (QMetaObject::*)() const) &QMetaObject::propertyCount, "C++: QMetaObject::propertyCount() const --> int");
		cl.def("classInfoCount", (int (QMetaObject::*)() const) &QMetaObject::classInfoCount, "C++: QMetaObject::classInfoCount() const --> int");
		cl.def("indexOfConstructor", (int (QMetaObject::*)(const char *) const) &QMetaObject::indexOfConstructor, "C++: QMetaObject::indexOfConstructor(const char *) const --> int", pybind11::arg("constructor"));
		cl.def("indexOfMethod", (int (QMetaObject::*)(const char *) const) &QMetaObject::indexOfMethod, "C++: QMetaObject::indexOfMethod(const char *) const --> int", pybind11::arg("method"));
		cl.def("indexOfSignal", (int (QMetaObject::*)(const char *) const) &QMetaObject::indexOfSignal, "C++: QMetaObject::indexOfSignal(const char *) const --> int", pybind11::arg("signal"));
		cl.def("indexOfSlot", (int (QMetaObject::*)(const char *) const) &QMetaObject::indexOfSlot, "C++: QMetaObject::indexOfSlot(const char *) const --> int", pybind11::arg("slot"));
		cl.def("indexOfEnumerator", (int (QMetaObject::*)(const char *) const) &QMetaObject::indexOfEnumerator, "C++: QMetaObject::indexOfEnumerator(const char *) const --> int", pybind11::arg("name"));
		cl.def("indexOfProperty", (int (QMetaObject::*)(const char *) const) &QMetaObject::indexOfProperty, "C++: QMetaObject::indexOfProperty(const char *) const --> int", pybind11::arg("name"));
		cl.def("indexOfClassInfo", (int (QMetaObject::*)(const char *) const) &QMetaObject::indexOfClassInfo, "C++: QMetaObject::indexOfClassInfo(const char *) const --> int", pybind11::arg("name"));
		cl.def("constructor", (class QMetaMethod (QMetaObject::*)(int) const) &QMetaObject::constructor, "C++: QMetaObject::constructor(int) const --> class QMetaMethod", pybind11::arg("index"));
		cl.def("method", (class QMetaMethod (QMetaObject::*)(int) const) &QMetaObject::method, "C++: QMetaObject::method(int) const --> class QMetaMethod", pybind11::arg("index"));
		cl.def("enumerator", (class QMetaEnum (QMetaObject::*)(int) const) &QMetaObject::enumerator, "C++: QMetaObject::enumerator(int) const --> class QMetaEnum", pybind11::arg("index"));
		cl.def("property", (class QMetaProperty (QMetaObject::*)(int) const) &QMetaObject::property, "C++: QMetaObject::property(int) const --> class QMetaProperty", pybind11::arg("index"));
		cl.def("classInfo", (class QMetaClassInfo (QMetaObject::*)(int) const) &QMetaObject::classInfo, "C++: QMetaObject::classInfo(int) const --> class QMetaClassInfo", pybind11::arg("index"));
		cl.def("userProperty", (class QMetaProperty (QMetaObject::*)() const) &QMetaObject::userProperty, "C++: QMetaObject::userProperty() const --> class QMetaProperty");
		cl.def_static("checkConnectArgs", (bool (*)(const char *, const char *)) &QMetaObject::checkConnectArgs, "C++: QMetaObject::checkConnectArgs(const char *, const char *) --> bool", pybind11::arg("signal"), pybind11::arg("method"));
		cl.def_static("checkConnectArgs", (bool (*)(const class QMetaMethod &, const class QMetaMethod &)) &QMetaObject::checkConnectArgs, "C++: QMetaObject::checkConnectArgs(const class QMetaMethod &, const class QMetaMethod &) --> bool", pybind11::arg("signal"), pybind11::arg("method"));
		cl.def_static("connect", [](const class QObject * a0, int const & a1, const class QObject * a2, int const & a3) -> QMetaObject::Connection { return QMetaObject::connect(a0, a1, a2, a3); }, "", pybind11::arg("sender"), pybind11::arg("signal_index"), pybind11::arg("receiver"), pybind11::arg("method_index"));
		cl.def_static("connect", [](const class QObject * a0, int const & a1, const class QObject * a2, int const & a3, int const & a4) -> QMetaObject::Connection { return QMetaObject::connect(a0, a1, a2, a3, a4); }, "", pybind11::arg("sender"), pybind11::arg("signal_index"), pybind11::arg("receiver"), pybind11::arg("method_index"), pybind11::arg("type"));
		cl.def_static("connect", (class QMetaObject::Connection (*)(const class QObject *, int, const class QObject *, int, int, int *)) &QMetaObject::connect, "C++: QMetaObject::connect(const class QObject *, int, const class QObject *, int, int, int *) --> class QMetaObject::Connection", pybind11::arg("sender"), pybind11::arg("signal_index"), pybind11::arg("receiver"), pybind11::arg("method_index"), pybind11::arg("type"), pybind11::arg("types"));
		cl.def_static("disconnect", (bool (*)(const class QObject *, int, const class QObject *, int)) &QMetaObject::disconnect, "C++: QMetaObject::disconnect(const class QObject *, int, const class QObject *, int) --> bool", pybind11::arg("sender"), pybind11::arg("signal_index"), pybind11::arg("receiver"), pybind11::arg("method_index"));
		cl.def_static("disconnectOne", (bool (*)(const class QObject *, int, const class QObject *, int)) &QMetaObject::disconnectOne, "C++: QMetaObject::disconnectOne(const class QObject *, int, const class QObject *, int) --> bool", pybind11::arg("sender"), pybind11::arg("signal_index"), pybind11::arg("receiver"), pybind11::arg("method_index"));
		cl.def_static("connectSlotsByName", (void (*)(class QObject *)) &QMetaObject::connectSlotsByName, "C++: QMetaObject::connectSlotsByName(class QObject *) --> void", pybind11::arg("o"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericReturnArgument const & a3) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg(""), pybind11::arg("ret"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericReturnArgument const & a3, class QGenericArgument const & a4) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg(""), pybind11::arg("ret"), pybind11::arg("val0"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericReturnArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg(""), pybind11::arg("ret"), pybind11::arg("val0"), pybind11::arg("val1"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericReturnArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg(""), pybind11::arg("ret"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericReturnArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg(""), pybind11::arg("ret"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericReturnArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7, class QGenericArgument const & a8) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7, a8); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg(""), pybind11::arg("ret"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericReturnArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7, class QGenericArgument const & a8, class QGenericArgument const & a9) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg(""), pybind11::arg("ret"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericReturnArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7, class QGenericArgument const & a8, class QGenericArgument const & a9, class QGenericArgument const & a10) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg(""), pybind11::arg("ret"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericReturnArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7, class QGenericArgument const & a8, class QGenericArgument const & a9, class QGenericArgument const & a10, class QGenericArgument const & a11) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg(""), pybind11::arg("ret"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"), pybind11::arg("val7"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericReturnArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7, class QGenericArgument const & a8, class QGenericArgument const & a9, class QGenericArgument const & a10, class QGenericArgument const & a11, class QGenericArgument const & a12) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg(""), pybind11::arg("ret"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"), pybind11::arg("val7"), pybind11::arg("val8"));
		cl.def_static("invokeMethod", (bool (*)(class QObject *, const char *, enum Qt::ConnectionType, class QGenericReturnArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument)) &QMetaObject::invokeMethod, "C++: QMetaObject::invokeMethod(class QObject *, const char *, enum Qt::ConnectionType, class QGenericReturnArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument) --> bool", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg(""), pybind11::arg("ret"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"), pybind11::arg("val7"), pybind11::arg("val8"), pybind11::arg("val9"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericReturnArgument const & a2) -> bool { return QMetaObject::invokeMethod(a0, a1, a2); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("ret"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericReturnArgument const & a2, class QGenericArgument const & a3) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("ret"), pybind11::arg("val0"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericReturnArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("ret"), pybind11::arg("val0"), pybind11::arg("val1"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericReturnArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("ret"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericReturnArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("ret"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericReturnArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("ret"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericReturnArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7, class QGenericArgument const & a8) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7, a8); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("ret"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericReturnArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7, class QGenericArgument const & a8, class QGenericArgument const & a9) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("ret"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericReturnArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7, class QGenericArgument const & a8, class QGenericArgument const & a9, class QGenericArgument const & a10) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("ret"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"), pybind11::arg("val7"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericReturnArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7, class QGenericArgument const & a8, class QGenericArgument const & a9, class QGenericArgument const & a10, class QGenericArgument const & a11) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("ret"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"), pybind11::arg("val7"), pybind11::arg("val8"));
		cl.def_static("invokeMethod", (bool (*)(class QObject *, const char *, class QGenericReturnArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument)) &QMetaObject::invokeMethod, "C++: QMetaObject::invokeMethod(class QObject *, const char *, class QGenericReturnArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument) --> bool", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("ret"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"), pybind11::arg("val7"), pybind11::arg("val8"), pybind11::arg("val9"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2) -> bool { return QMetaObject::invokeMethod(a0, a1, a2); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("type"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericArgument const & a3) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("type"), pybind11::arg("val0"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("type"), pybind11::arg("val0"), pybind11::arg("val1"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("type"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("type"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("type"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7, class QGenericArgument const & a8) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7, a8); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("type"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7, class QGenericArgument const & a8, class QGenericArgument const & a9) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("type"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7, class QGenericArgument const & a8, class QGenericArgument const & a9, class QGenericArgument const & a10) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("type"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"), pybind11::arg("val7"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, enum Qt::ConnectionType const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7, class QGenericArgument const & a8, class QGenericArgument const & a9, class QGenericArgument const & a10, class QGenericArgument const & a11) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("type"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"), pybind11::arg("val7"), pybind11::arg("val8"));
		cl.def_static("invokeMethod", (bool (*)(class QObject *, const char *, enum Qt::ConnectionType, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument)) &QMetaObject::invokeMethod, "C++: QMetaObject::invokeMethod(class QObject *, const char *, enum Qt::ConnectionType, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument) --> bool", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("type"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"), pybind11::arg("val7"), pybind11::arg("val8"), pybind11::arg("val9"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1) -> bool { return QMetaObject::invokeMethod(a0, a1); }, "", pybind11::arg("obj"), pybind11::arg("member"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericArgument const & a2) -> bool { return QMetaObject::invokeMethod(a0, a1, a2); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("val0"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericArgument const & a2, class QGenericArgument const & a3) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("val0"), pybind11::arg("val1"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7, class QGenericArgument const & a8) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7, a8); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7, class QGenericArgument const & a8, class QGenericArgument const & a9) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"), pybind11::arg("val7"));
		cl.def_static("invokeMethod", [](class QObject * a0, const char * a1, class QGenericArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7, class QGenericArgument const & a8, class QGenericArgument const & a9, class QGenericArgument const & a10) -> bool { return QMetaObject::invokeMethod(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); }, "", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"), pybind11::arg("val7"), pybind11::arg("val8"));
		cl.def_static("invokeMethod", (bool (*)(class QObject *, const char *, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument)) &QMetaObject::invokeMethod, "C++: QMetaObject::invokeMethod(class QObject *, const char *, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument) --> bool", pybind11::arg("obj"), pybind11::arg("member"), pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"), pybind11::arg("val7"), pybind11::arg("val8"), pybind11::arg("val9"));
		cl.def("newInstance", [](QMetaObject const &o) -> QObject * { return o.newInstance(); }, "", pybind11::return_value_policy::automatic);
		cl.def("newInstance", [](QMetaObject const &o, class QGenericArgument const & a0) -> QObject * { return o.newInstance(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("val0"));
		cl.def("newInstance", [](QMetaObject const &o, class QGenericArgument const & a0, class QGenericArgument const & a1) -> QObject * { return o.newInstance(a0, a1); }, "", pybind11::return_value_policy::automatic, pybind11::arg("val0"), pybind11::arg("val1"));
		cl.def("newInstance", [](QMetaObject const &o, class QGenericArgument const & a0, class QGenericArgument const & a1, class QGenericArgument const & a2) -> QObject * { return o.newInstance(a0, a1, a2); }, "", pybind11::return_value_policy::automatic, pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"));
		cl.def("newInstance", [](QMetaObject const &o, class QGenericArgument const & a0, class QGenericArgument const & a1, class QGenericArgument const & a2, class QGenericArgument const & a3) -> QObject * { return o.newInstance(a0, a1, a2, a3); }, "", pybind11::return_value_policy::automatic, pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"));
		cl.def("newInstance", [](QMetaObject const &o, class QGenericArgument const & a0, class QGenericArgument const & a1, class QGenericArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4) -> QObject * { return o.newInstance(a0, a1, a2, a3, a4); }, "", pybind11::return_value_policy::automatic, pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"));
		cl.def("newInstance", [](QMetaObject const &o, class QGenericArgument const & a0, class QGenericArgument const & a1, class QGenericArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5) -> QObject * { return o.newInstance(a0, a1, a2, a3, a4, a5); }, "", pybind11::return_value_policy::automatic, pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"));
		cl.def("newInstance", [](QMetaObject const &o, class QGenericArgument const & a0, class QGenericArgument const & a1, class QGenericArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6) -> QObject * { return o.newInstance(a0, a1, a2, a3, a4, a5, a6); }, "", pybind11::return_value_policy::automatic, pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"));
		cl.def("newInstance", [](QMetaObject const &o, class QGenericArgument const & a0, class QGenericArgument const & a1, class QGenericArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7) -> QObject * { return o.newInstance(a0, a1, a2, a3, a4, a5, a6, a7); }, "", pybind11::return_value_policy::automatic, pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"), pybind11::arg("val7"));
		cl.def("newInstance", [](QMetaObject const &o, class QGenericArgument const & a0, class QGenericArgument const & a1, class QGenericArgument const & a2, class QGenericArgument const & a3, class QGenericArgument const & a4, class QGenericArgument const & a5, class QGenericArgument const & a6, class QGenericArgument const & a7, class QGenericArgument const & a8) -> QObject * { return o.newInstance(a0, a1, a2, a3, a4, a5, a6, a7, a8); }, "", pybind11::return_value_policy::automatic, pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"), pybind11::arg("val7"), pybind11::arg("val8"));
		cl.def("newInstance", (class QObject * (QMetaObject::*)(class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument) const) &QMetaObject::newInstance, "C++: QMetaObject::newInstance(class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument, class QGenericArgument) const --> class QObject *", pybind11::return_value_policy::automatic, pybind11::arg("val0"), pybind11::arg("val1"), pybind11::arg("val2"), pybind11::arg("val3"), pybind11::arg("val4"), pybind11::arg("val5"), pybind11::arg("val6"), pybind11::arg("val7"), pybind11::arg("val8"), pybind11::arg("val9"));
		cl.def("assign", (struct QMetaObject & (QMetaObject::*)(const struct QMetaObject &)) &QMetaObject::operator=, "C++: QMetaObject::operator=(const struct QMetaObject &) --> struct QMetaObject &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // QMetaObject::Connection file:QtCore/qobjectdefs.h line:619
			auto & enclosing_class = cl;
			pybind11::class_<QMetaObject::Connection, std::shared_ptr<QMetaObject::Connection>> cl(enclosing_class, "Connection", "");
			cl.def( pybind11::init( [](){ return new QMetaObject::Connection(); } ) );
			cl.def( pybind11::init( [](QMetaObject::Connection const &o){ return new QMetaObject::Connection(o); } ) );
			cl.def("assign", (class QMetaObject::Connection & (QMetaObject::Connection::*)(const class QMetaObject::Connection &)) &QMetaObject::Connection::operator=, "C++: QMetaObject::Connection::operator=(const class QMetaObject::Connection &) --> class QMetaObject::Connection &", pybind11::return_value_policy::automatic, pybind11::arg("other"));
		}

		{ // QMetaObject::SuperData file:QtCore/qobjectdefs.h line:580
			auto & enclosing_class = cl;
			pybind11::class_<QMetaObject::SuperData, std::shared_ptr<QMetaObject::SuperData>> cl(enclosing_class, "SuperData", "");
			cl.def( pybind11::init( [](){ return new QMetaObject::SuperData(); } ) );
			cl.def( pybind11::init<std::nullptr_t>(), pybind11::arg("") );

			cl.def( pybind11::init<const struct QMetaObject *>(), pybind11::arg("mo") );

			cl.def( pybind11::init( [](QMetaObject::SuperData const &o){ return new QMetaObject::SuperData(o); } ) );
			cl.def("assign", (struct QMetaObject::SuperData & (QMetaObject::SuperData::*)(const struct QMetaObject::SuperData &)) &QMetaObject::SuperData::operator=, "C++: QMetaObject::SuperData::operator=(const struct QMetaObject::SuperData &) --> struct QMetaObject::SuperData &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		}

	}
}
