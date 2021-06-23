#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qchar.h> // QLatin1Char
#include <QtCore/qdatetime.h> // QDate
#include <QtCore/qdatetime.h> // QDateTime
#include <QtCore/qdatetime.h> // QTime
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qhash.h> // QHash
#include <QtCore/qjsonarray.h> // +include_for_class
#include <QtCore/qjsonobject.h> // +include_for_class
#include <QtCore/qjsonvalue.h> // 
#include <QtCore/qjsonvalue.h> // QJsonValue
#include <QtCore/qjsonvalue.h> // QJsonValuePtr
#include <QtCore/qjsonvalue.h> // QJsonValueRef
#include <QtCore/qjsonvalue.h> // QJsonValueRefPtr
#include <QtCore/qjsonvalue.h> // qHash
#include <QtCore/qlist.h> // QList
#include <QtCore/qlocale.h> // QLocale
#include <QtCore/qmap.h> // QMap
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::QPairVariantInterfaceImpl
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
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

void bind_QtCore_qjsonvalue(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B499_[QJsonValue] ";
	{ // QJsonValue file:QtCore/qjsonvalue.h line:59
		pybind11::class_<QJsonValue, std::shared_ptr<QJsonValue>> cl(M(""), "QJsonValue", "");
		cl.def( pybind11::init( [](){ return new QJsonValue(); } ), "doc" );
		cl.def( pybind11::init<enum QJsonValue::Type>(), pybind11::arg("") );

		cl.def( pybind11::init<bool>(), pybind11::arg("b") );

		cl.def( pybind11::init<double>(), pybind11::arg("n") );

		cl.def( pybind11::init<int>(), pybind11::arg("n") );

		cl.def( pybind11::init<long long>(), pybind11::arg("v") );

		cl.def( pybind11::init<const class QString &>(), pybind11::arg("s") );

		cl.def( pybind11::init<class QLatin1String>(), pybind11::arg("s") );

		cl.def( pybind11::init<const char *>(), pybind11::arg("s") );

		cl.def( pybind11::init<const class QJsonArray &>(), pybind11::arg("a") );

		cl.def( pybind11::init<const class QJsonObject &>(), pybind11::arg("o") );

		cl.def( pybind11::init( [](QJsonValue const &o){ return new QJsonValue(o); } ) );

		pybind11::enum_<QJsonValue::Type>(cl, "Type", pybind11::arithmetic(), "")
			.value("Null", QJsonValue::Null)
			.value("Bool", QJsonValue::Bool)
			.value("Double", QJsonValue::Double)
			.value("String", QJsonValue::String)
			.value("Array", QJsonValue::Array)
			.value("Object", QJsonValue::Object)
			.value("Undefined", QJsonValue::Undefined)
			.export_values();

		cl.def("assign", (class QJsonValue & (QJsonValue::*)(const class QJsonValue &)) &QJsonValue::operator=, "C++: QJsonValue::operator=(const class QJsonValue &) --> class QJsonValue &", pybind11::return_value_policy::automatic, pybind11::arg("other"));
		cl.def("swap", (void (QJsonValue::*)(class QJsonValue &)) &QJsonValue::swap, "C++: QJsonValue::swap(class QJsonValue &) --> void", pybind11::arg("other"));
		cl.def_static("fromVariant", (class QJsonValue (*)(const class QVariant &)) &QJsonValue::fromVariant, "C++: QJsonValue::fromVariant(const class QVariant &) --> class QJsonValue", pybind11::arg("variant"));
		cl.def("toVariant", (class QVariant (QJsonValue::*)() const) &QJsonValue::toVariant, "C++: QJsonValue::toVariant() const --> class QVariant");
		cl.def("type", (enum QJsonValue::Type (QJsonValue::*)() const) &QJsonValue::type, "C++: QJsonValue::type() const --> enum QJsonValue::Type");
		cl.def("isNull", (bool (QJsonValue::*)() const) &QJsonValue::isNull, "C++: QJsonValue::isNull() const --> bool");
		cl.def("isBool", (bool (QJsonValue::*)() const) &QJsonValue::isBool, "C++: QJsonValue::isBool() const --> bool");
		cl.def("isDouble", (bool (QJsonValue::*)() const) &QJsonValue::isDouble, "C++: QJsonValue::isDouble() const --> bool");
		cl.def("isString", (bool (QJsonValue::*)() const) &QJsonValue::isString, "C++: QJsonValue::isString() const --> bool");
		cl.def("isArray", (bool (QJsonValue::*)() const) &QJsonValue::isArray, "C++: QJsonValue::isArray() const --> bool");
		cl.def("isObject", (bool (QJsonValue::*)() const) &QJsonValue::isObject, "C++: QJsonValue::isObject() const --> bool");
		cl.def("isUndefined", (bool (QJsonValue::*)() const) &QJsonValue::isUndefined, "C++: QJsonValue::isUndefined() const --> bool");
		cl.def("toBool", [](QJsonValue const &o) -> bool { return o.toBool(); }, "");
		cl.def("toBool", (bool (QJsonValue::*)(bool) const) &QJsonValue::toBool, "C++: QJsonValue::toBool(bool) const --> bool", pybind11::arg("defaultValue"));
		cl.def("toInt", [](QJsonValue const &o) -> int { return o.toInt(); }, "");
		cl.def("toInt", (int (QJsonValue::*)(int) const) &QJsonValue::toInt, "C++: QJsonValue::toInt(int) const --> int", pybind11::arg("defaultValue"));
		cl.def("toDouble", [](QJsonValue const &o) -> double { return o.toDouble(); }, "");
		cl.def("toDouble", (double (QJsonValue::*)(double) const) &QJsonValue::toDouble, "C++: QJsonValue::toDouble(double) const --> double", pybind11::arg("defaultValue"));
		cl.def("toString", (class QString (QJsonValue::*)() const) &QJsonValue::toString, "C++: QJsonValue::toString() const --> class QString");
		cl.def("toString", (class QString (QJsonValue::*)(const class QString &) const) &QJsonValue::toString, "C++: QJsonValue::toString(const class QString &) const --> class QString", pybind11::arg("defaultValue"));
		cl.def("toArray", (class QJsonArray (QJsonValue::*)() const) &QJsonValue::toArray, "C++: QJsonValue::toArray() const --> class QJsonArray");
		cl.def("toArray", (class QJsonArray (QJsonValue::*)(const class QJsonArray &) const) &QJsonValue::toArray, "C++: QJsonValue::toArray(const class QJsonArray &) const --> class QJsonArray", pybind11::arg("defaultValue"));
		cl.def("toObject", (class QJsonObject (QJsonValue::*)() const) &QJsonValue::toObject, "C++: QJsonValue::toObject() const --> class QJsonObject");
		cl.def("toObject", (class QJsonObject (QJsonValue::*)(const class QJsonObject &) const) &QJsonValue::toObject, "C++: QJsonValue::toObject(const class QJsonObject &) const --> class QJsonObject", pybind11::arg("defaultValue"));
		cl.def("__getitem__", (const class QJsonValue (QJsonValue::*)(const class QString &) const) &QJsonValue::operator[], "C++: QJsonValue::operator[](const class QString &) const --> const class QJsonValue", pybind11::arg("key"));
		cl.def("__getitem__", (const class QJsonValue (QJsonValue::*)(class QStringView) const) &QJsonValue::operator[], "C++: QJsonValue::operator[](class QStringView) const --> const class QJsonValue", pybind11::arg("key"));
		cl.def("__getitem__", (const class QJsonValue (QJsonValue::*)(class QLatin1String) const) &QJsonValue::operator[], "C++: QJsonValue::operator[](class QLatin1String) const --> const class QJsonValue", pybind11::arg("key"));
		cl.def("__getitem__", (const class QJsonValue (QJsonValue::*)(int) const) &QJsonValue::operator[], "C++: QJsonValue::operator[](int) const --> const class QJsonValue", pybind11::arg("i"));
		cl.def("__eq__", (bool (QJsonValue::*)(const class QJsonValue &) const) &QJsonValue::operator==, "C++: QJsonValue::operator==(const class QJsonValue &) const --> bool", pybind11::arg("other"));
		cl.def("__ne__", (bool (QJsonValue::*)(const class QJsonValue &) const) &QJsonValue::operator!=, "C++: QJsonValue::operator!=(const class QJsonValue &) const --> bool", pybind11::arg("other"));
	}
	std::cout << "B500_[QJsonValueRef] ";
	{ // QJsonValueRef file:QtCore/qjsonvalue.h line:158
		pybind11::class_<QJsonValueRef, std::shared_ptr<QJsonValueRef>> cl(M(""), "QJsonValueRef", "");
		cl.def( pybind11::init( [](QJsonValueRef const &o){ return new QJsonValueRef(o); } ) );
		cl.def( pybind11::init<class QJsonArray *, int>(), pybind11::arg("array"), pybind11::arg("idx") );

		cl.def( pybind11::init<class QJsonObject *, int>(), pybind11::arg("object"), pybind11::arg("idx") );

		cl.def("assign", (class QJsonValueRef & (QJsonValueRef::*)(const class QJsonValue &)) &QJsonValueRef::operator=, "C++: QJsonValueRef::operator=(const class QJsonValue &) --> class QJsonValueRef &", pybind11::return_value_policy::automatic, pybind11::arg("val"));
		cl.def("assign", (class QJsonValueRef & (QJsonValueRef::*)(const class QJsonValueRef &)) &QJsonValueRef::operator=, "C++: QJsonValueRef::operator=(const class QJsonValueRef &) --> class QJsonValueRef &", pybind11::return_value_policy::automatic, pybind11::arg("val"));
		cl.def("toVariant", (class QVariant (QJsonValueRef::*)() const) &QJsonValueRef::toVariant, "C++: QJsonValueRef::toVariant() const --> class QVariant");
		cl.def("type", (enum QJsonValue::Type (QJsonValueRef::*)() const) &QJsonValueRef::type, "C++: QJsonValueRef::type() const --> enum QJsonValue::Type");
		cl.def("isNull", (bool (QJsonValueRef::*)() const) &QJsonValueRef::isNull, "C++: QJsonValueRef::isNull() const --> bool");
		cl.def("isBool", (bool (QJsonValueRef::*)() const) &QJsonValueRef::isBool, "C++: QJsonValueRef::isBool() const --> bool");
		cl.def("isDouble", (bool (QJsonValueRef::*)() const) &QJsonValueRef::isDouble, "C++: QJsonValueRef::isDouble() const --> bool");
		cl.def("isString", (bool (QJsonValueRef::*)() const) &QJsonValueRef::isString, "C++: QJsonValueRef::isString() const --> bool");
		cl.def("isArray", (bool (QJsonValueRef::*)() const) &QJsonValueRef::isArray, "C++: QJsonValueRef::isArray() const --> bool");
		cl.def("isObject", (bool (QJsonValueRef::*)() const) &QJsonValueRef::isObject, "C++: QJsonValueRef::isObject() const --> bool");
		cl.def("isUndefined", (bool (QJsonValueRef::*)() const) &QJsonValueRef::isUndefined, "C++: QJsonValueRef::isUndefined() const --> bool");
		cl.def("toBool", (bool (QJsonValueRef::*)() const) &QJsonValueRef::toBool, "C++: QJsonValueRef::toBool() const --> bool");
		cl.def("toInt", (int (QJsonValueRef::*)() const) &QJsonValueRef::toInt, "C++: QJsonValueRef::toInt() const --> int");
		cl.def("toDouble", (double (QJsonValueRef::*)() const) &QJsonValueRef::toDouble, "C++: QJsonValueRef::toDouble() const --> double");
		cl.def("toString", (class QString (QJsonValueRef::*)() const) &QJsonValueRef::toString, "C++: QJsonValueRef::toString() const --> class QString");
		cl.def("toArray", (class QJsonArray (QJsonValueRef::*)() const) &QJsonValueRef::toArray, "C++: QJsonValueRef::toArray() const --> class QJsonArray");
		cl.def("toObject", (class QJsonObject (QJsonValueRef::*)() const) &QJsonValueRef::toObject, "C++: QJsonValueRef::toObject() const --> class QJsonObject");
		cl.def("toBool", (bool (QJsonValueRef::*)(bool) const) &QJsonValueRef::toBool, "C++: QJsonValueRef::toBool(bool) const --> bool", pybind11::arg("defaultValue"));
		cl.def("toInt", (int (QJsonValueRef::*)(int) const) &QJsonValueRef::toInt, "C++: QJsonValueRef::toInt(int) const --> int", pybind11::arg("defaultValue"));
		cl.def("toDouble", (double (QJsonValueRef::*)(double) const) &QJsonValueRef::toDouble, "C++: QJsonValueRef::toDouble(double) const --> double", pybind11::arg("defaultValue"));
		cl.def("toString", (class QString (QJsonValueRef::*)(const class QString &) const) &QJsonValueRef::toString, "C++: QJsonValueRef::toString(const class QString &) const --> class QString", pybind11::arg("defaultValue"));
		cl.def("__eq__", (bool (QJsonValueRef::*)(const class QJsonValue &) const) &QJsonValueRef::operator==, "C++: QJsonValueRef::operator==(const class QJsonValue &) const --> bool", pybind11::arg("other"));
		cl.def("__ne__", (bool (QJsonValueRef::*)(const class QJsonValue &) const) &QJsonValueRef::operator!=, "C++: QJsonValueRef::operator!=(const class QJsonValue &) const --> bool", pybind11::arg("other"));
	}
	std::cout << "B501_[QJsonValuePtr] ";
	std::cout << "B502_[QJsonValueRefPtr] ";
	std::cout << "B503_[QTypeInfo<QJsonValue>] ";
	std::cout << "B504_[void swap(class QJsonValue &, class QJsonValue &)] ";
	std::cout << "B505_[unsigned int qHash(const class QJsonValue &, unsigned int)] ";
	std::cout << "B506_[QJsonArray] ";
	{ // QJsonArray file: line:55
		pybind11::class_<QJsonArray, std::shared_ptr<QJsonArray>> cl(M(""), "QJsonArray", "");
		cl.def( pybind11::init( [](){ return new QJsonArray(); } ) );
		cl.def( pybind11::init( [](QJsonArray const &o){ return new QJsonArray(o); } ) );
		cl.def("assign", (class QJsonArray & (QJsonArray::*)(const class QJsonArray &)) &QJsonArray::operator=, "C++: QJsonArray::operator=(const class QJsonArray &) --> class QJsonArray &", pybind11::return_value_policy::automatic, pybind11::arg("other"));
		cl.def("size", (int (QJsonArray::*)() const) &QJsonArray::size, "C++: QJsonArray::size() const --> int");
		cl.def("count", (int (QJsonArray::*)() const) &QJsonArray::count, "C++: QJsonArray::count() const --> int");
		cl.def("isEmpty", (bool (QJsonArray::*)() const) &QJsonArray::isEmpty, "C++: QJsonArray::isEmpty() const --> bool");
		cl.def("at", (class QJsonValue (QJsonArray::*)(int) const) &QJsonArray::at, "C++: QJsonArray::at(int) const --> class QJsonValue", pybind11::arg("i"));
		cl.def("first", (class QJsonValue (QJsonArray::*)() const) &QJsonArray::first, "C++: QJsonArray::first() const --> class QJsonValue");
		cl.def("last", (class QJsonValue (QJsonArray::*)() const) &QJsonArray::last, "C++: QJsonArray::last() const --> class QJsonValue");
		cl.def("prepend", (void (QJsonArray::*)(const class QJsonValue &)) &QJsonArray::prepend, "C++: QJsonArray::prepend(const class QJsonValue &) --> void", pybind11::arg("value"));
		cl.def("append", (void (QJsonArray::*)(const class QJsonValue &)) &QJsonArray::append, "C++: QJsonArray::append(const class QJsonValue &) --> void", pybind11::arg("value"));
		cl.def("removeAt", (void (QJsonArray::*)(int)) &QJsonArray::removeAt, "C++: QJsonArray::removeAt(int) --> void", pybind11::arg("i"));
		cl.def("takeAt", (class QJsonValue (QJsonArray::*)(int)) &QJsonArray::takeAt, "C++: QJsonArray::takeAt(int) --> class QJsonValue", pybind11::arg("i"));
		cl.def("removeFirst", (void (QJsonArray::*)()) &QJsonArray::removeFirst, "C++: QJsonArray::removeFirst() --> void");
		cl.def("removeLast", (void (QJsonArray::*)()) &QJsonArray::removeLast, "C++: QJsonArray::removeLast() --> void");
		cl.def("insert", (void (QJsonArray::*)(int, const class QJsonValue &)) &QJsonArray::insert, "C++: QJsonArray::insert(int, const class QJsonValue &) --> void", pybind11::arg("i"), pybind11::arg("value"));
		cl.def("replace", (void (QJsonArray::*)(int, const class QJsonValue &)) &QJsonArray::replace, "C++: QJsonArray::replace(int, const class QJsonValue &) --> void", pybind11::arg("i"), pybind11::arg("value"));
		cl.def("contains", (bool (QJsonArray::*)(const class QJsonValue &) const) &QJsonArray::contains, "C++: QJsonArray::contains(const class QJsonValue &) const --> bool", pybind11::arg("element"));
		cl.def("__getitem__", (class QJsonValueRef (QJsonArray::*)(int)) &QJsonArray::operator[], "C++: QJsonArray::operator[](int) --> class QJsonValueRef", pybind11::arg("i"));
		cl.def("__eq__", (bool (QJsonArray::*)(const class QJsonArray &) const) &QJsonArray::operator==, "C++: QJsonArray::operator==(const class QJsonArray &) const --> bool", pybind11::arg("other"));
		cl.def("__ne__", (bool (QJsonArray::*)(const class QJsonArray &) const) &QJsonArray::operator!=, "C++: QJsonArray::operator!=(const class QJsonArray &) const --> bool", pybind11::arg("other"));
		cl.def("swap", (void (QJsonArray::*)(class QJsonArray &)) &QJsonArray::swap, "C++: QJsonArray::swap(class QJsonArray &) --> void", pybind11::arg("other"));
		cl.def("begin", (class QJsonArray::iterator (QJsonArray::*)()) &QJsonArray::begin, "C++: QJsonArray::begin() --> class QJsonArray::iterator");
		cl.def("constBegin", (class QJsonArray::const_iterator (QJsonArray::*)() const) &QJsonArray::constBegin, "C++: QJsonArray::constBegin() const --> class QJsonArray::const_iterator");
		cl.def("cbegin", (class QJsonArray::const_iterator (QJsonArray::*)() const) &QJsonArray::cbegin, "C++: QJsonArray::cbegin() const --> class QJsonArray::const_iterator");
		cl.def("end", (class QJsonArray::iterator (QJsonArray::*)()) &QJsonArray::end, "C++: QJsonArray::end() --> class QJsonArray::iterator");
		cl.def("constEnd", (class QJsonArray::const_iterator (QJsonArray::*)() const) &QJsonArray::constEnd, "C++: QJsonArray::constEnd() const --> class QJsonArray::const_iterator");
		cl.def("cend", (class QJsonArray::const_iterator (QJsonArray::*)() const) &QJsonArray::cend, "C++: QJsonArray::cend() const --> class QJsonArray::const_iterator");
		cl.def("insert", (class QJsonArray::iterator (QJsonArray::*)(class QJsonArray::iterator, const class QJsonValue &)) &QJsonArray::insert, "C++: QJsonArray::insert(class QJsonArray::iterator, const class QJsonValue &) --> class QJsonArray::iterator", pybind11::arg("before"), pybind11::arg("value"));
		cl.def("erase", (class QJsonArray::iterator (QJsonArray::*)(class QJsonArray::iterator)) &QJsonArray::erase, "C++: QJsonArray::erase(class QJsonArray::iterator) --> class QJsonArray::iterator", pybind11::arg("it"));
		cl.def("__add__", (class QJsonArray (QJsonArray::*)(const class QJsonValue &) const) &QJsonArray::operator+, "C++: QJsonArray::operator+(const class QJsonValue &) const --> class QJsonArray", pybind11::arg("v"));
		cl.def("__iadd__", (class QJsonArray & (QJsonArray::*)(const class QJsonValue &)) &QJsonArray::operator+=, "C++: QJsonArray::operator+=(const class QJsonValue &) --> class QJsonArray &", pybind11::return_value_policy::automatic, pybind11::arg("v"));
		cl.def("push_back", (void (QJsonArray::*)(const class QJsonValue &)) &QJsonArray::push_back, "C++: QJsonArray::push_back(const class QJsonValue &) --> void", pybind11::arg("t"));
		cl.def("push_front", (void (QJsonArray::*)(const class QJsonValue &)) &QJsonArray::push_front, "C++: QJsonArray::push_front(const class QJsonValue &) --> void", pybind11::arg("t"));
		cl.def("pop_front", (void (QJsonArray::*)()) &QJsonArray::pop_front, "C++: QJsonArray::pop_front() --> void");
		cl.def("pop_back", (void (QJsonArray::*)()) &QJsonArray::pop_back, "C++: QJsonArray::pop_back() --> void");
		cl.def("empty", (bool (QJsonArray::*)() const) &QJsonArray::empty, "C++: QJsonArray::empty() const --> bool");

		{ // QJsonArray::const_iterator file: line:156
			auto & enclosing_class = cl;
			pybind11::class_<QJsonArray::const_iterator, std::shared_ptr<QJsonArray::const_iterator>> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init( [](){ return new QJsonArray::const_iterator(); } ) );
			cl.def( pybind11::init<const class QJsonArray *, int>(), pybind11::arg("array"), pybind11::arg("index") );

			cl.def( pybind11::init( [](QJsonArray::const_iterator const &o){ return new QJsonArray::const_iterator(o); } ) );
			cl.def( pybind11::init<const class QJsonArray::iterator &>(), pybind11::arg("o") );

			cl.def_readwrite("i", &QJsonArray::const_iterator::i);
			cl.def("__mul__", (class QJsonValue (QJsonArray::const_iterator::*)() const) &QJsonArray::const_iterator::operator*, "C++: QJsonArray::const_iterator::operator*() const --> class QJsonValue");
			cl.def("__getitem__", (class QJsonValue (QJsonArray::const_iterator::*)(int) const) &QJsonArray::const_iterator::operator[], "C++: QJsonArray::const_iterator::operator[](int) const --> class QJsonValue", pybind11::arg("j"));
			cl.def("__eq__", (bool (QJsonArray::const_iterator::*)(const class QJsonArray::const_iterator &) const) &QJsonArray::const_iterator::operator==, "C++: QJsonArray::const_iterator::operator==(const class QJsonArray::const_iterator &) const --> bool", pybind11::arg("o"));
			cl.def("__ne__", (bool (QJsonArray::const_iterator::*)(const class QJsonArray::const_iterator &) const) &QJsonArray::const_iterator::operator!=, "C++: QJsonArray::const_iterator::operator!=(const class QJsonArray::const_iterator &) const --> bool", pybind11::arg("o"));
			cl.def("plus_plus", (class QJsonArray::const_iterator & (QJsonArray::const_iterator::*)()) &QJsonArray::const_iterator::operator++, "C++: QJsonArray::const_iterator::operator++() --> class QJsonArray::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class QJsonArray::const_iterator (QJsonArray::const_iterator::*)(int)) &QJsonArray::const_iterator::operator++, "C++: QJsonArray::const_iterator::operator++(int) --> class QJsonArray::const_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class QJsonArray::const_iterator & (QJsonArray::const_iterator::*)()) &QJsonArray::const_iterator::operator--, "C++: QJsonArray::const_iterator::operator--() --> class QJsonArray::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (class QJsonArray::const_iterator (QJsonArray::const_iterator::*)(int)) &QJsonArray::const_iterator::operator--, "C++: QJsonArray::const_iterator::operator--(int) --> class QJsonArray::const_iterator", pybind11::arg(""));
			cl.def("__iadd__", (class QJsonArray::const_iterator & (QJsonArray::const_iterator::*)(int)) &QJsonArray::const_iterator::operator+=, "C++: QJsonArray::const_iterator::operator+=(int) --> class QJsonArray::const_iterator &", pybind11::return_value_policy::automatic, pybind11::arg("j"));
			cl.def("__isub__", (class QJsonArray::const_iterator & (QJsonArray::const_iterator::*)(int)) &QJsonArray::const_iterator::operator-=, "C++: QJsonArray::const_iterator::operator-=(int) --> class QJsonArray::const_iterator &", pybind11::return_value_policy::automatic, pybind11::arg("j"));
			cl.def("__add__", (class QJsonArray::const_iterator (QJsonArray::const_iterator::*)(int) const) &QJsonArray::const_iterator::operator+, "C++: QJsonArray::const_iterator::operator+(int) const --> class QJsonArray::const_iterator", pybind11::arg("j"));
			cl.def("__sub__", (class QJsonArray::const_iterator (QJsonArray::const_iterator::*)(int) const) &QJsonArray::const_iterator::operator-, "C++: QJsonArray::const_iterator::operator-(int) const --> class QJsonArray::const_iterator", pybind11::arg("j"));
			cl.def("__sub__", (int (QJsonArray::const_iterator::*)(class QJsonArray::const_iterator) const) &QJsonArray::const_iterator::operator-, "C++: QJsonArray::const_iterator::operator-(class QJsonArray::const_iterator) const --> int", pybind11::arg("j"));
		}

		{ // QJsonArray::iterator file: line:111
			auto & enclosing_class = cl;
			pybind11::class_<QJsonArray::iterator, std::shared_ptr<QJsonArray::iterator>> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init( [](){ return new QJsonArray::iterator(); } ) );
			cl.def( pybind11::init<class QJsonArray *, int>(), pybind11::arg("array"), pybind11::arg("index") );

			cl.def( pybind11::init( [](QJsonArray::iterator const &o){ return new QJsonArray::iterator(o); } ) );
			cl.def_readwrite("i", &QJsonArray::iterator::i);
			cl.def("__mul__", (class QJsonValueRef (QJsonArray::iterator::*)() const) &QJsonArray::iterator::operator*, "C++: QJsonArray::iterator::operator*() const --> class QJsonValueRef");
			cl.def("__getitem__", (class QJsonValueRef (QJsonArray::iterator::*)(int) const) &QJsonArray::iterator::operator[], "C++: QJsonArray::iterator::operator[](int) const --> class QJsonValueRef", pybind11::arg("j"));
			cl.def("__eq__", (bool (QJsonArray::iterator::*)(const class QJsonArray::iterator &) const) &QJsonArray::iterator::operator==, "C++: QJsonArray::iterator::operator==(const class QJsonArray::iterator &) const --> bool", pybind11::arg("o"));
			cl.def("__ne__", (bool (QJsonArray::iterator::*)(const class QJsonArray::iterator &) const) &QJsonArray::iterator::operator!=, "C++: QJsonArray::iterator::operator!=(const class QJsonArray::iterator &) const --> bool", pybind11::arg("o"));
			cl.def("__eq__", (bool (QJsonArray::iterator::*)(const class QJsonArray::const_iterator &) const) &QJsonArray::iterator::operator==, "C++: QJsonArray::iterator::operator==(const class QJsonArray::const_iterator &) const --> bool", pybind11::arg("o"));
			cl.def("__ne__", (bool (QJsonArray::iterator::*)(const class QJsonArray::const_iterator &) const) &QJsonArray::iterator::operator!=, "C++: QJsonArray::iterator::operator!=(const class QJsonArray::const_iterator &) const --> bool", pybind11::arg("o"));
			cl.def("plus_plus", (class QJsonArray::iterator & (QJsonArray::iterator::*)()) &QJsonArray::iterator::operator++, "C++: QJsonArray::iterator::operator++() --> class QJsonArray::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class QJsonArray::iterator (QJsonArray::iterator::*)(int)) &QJsonArray::iterator::operator++, "C++: QJsonArray::iterator::operator++(int) --> class QJsonArray::iterator", pybind11::arg(""));
			cl.def("minus_minus", (class QJsonArray::iterator & (QJsonArray::iterator::*)()) &QJsonArray::iterator::operator--, "C++: QJsonArray::iterator::operator--() --> class QJsonArray::iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (class QJsonArray::iterator (QJsonArray::iterator::*)(int)) &QJsonArray::iterator::operator--, "C++: QJsonArray::iterator::operator--(int) --> class QJsonArray::iterator", pybind11::arg(""));
			cl.def("__iadd__", (class QJsonArray::iterator & (QJsonArray::iterator::*)(int)) &QJsonArray::iterator::operator+=, "C++: QJsonArray::iterator::operator+=(int) --> class QJsonArray::iterator &", pybind11::return_value_policy::automatic, pybind11::arg("j"));
			cl.def("__isub__", (class QJsonArray::iterator & (QJsonArray::iterator::*)(int)) &QJsonArray::iterator::operator-=, "C++: QJsonArray::iterator::operator-=(int) --> class QJsonArray::iterator &", pybind11::return_value_policy::automatic, pybind11::arg("j"));
			cl.def("__add__", (class QJsonArray::iterator (QJsonArray::iterator::*)(int) const) &QJsonArray::iterator::operator+, "C++: QJsonArray::iterator::operator+(int) const --> class QJsonArray::iterator", pybind11::arg("j"));
			cl.def("__sub__", (class QJsonArray::iterator (QJsonArray::iterator::*)(int) const) &QJsonArray::iterator::operator-, "C++: QJsonArray::iterator::operator-(int) const --> class QJsonArray::iterator", pybind11::arg("j"));
			cl.def("__sub__", (int (QJsonArray::iterator::*)(class QJsonArray::iterator) const) &QJsonArray::iterator::operator-, "C++: QJsonArray::iterator::operator-(class QJsonArray::iterator) const --> int", pybind11::arg("j"));
		}

	}
}
