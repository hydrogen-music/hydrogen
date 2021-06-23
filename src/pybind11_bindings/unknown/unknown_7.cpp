#include <QtCore/qbytearray.h> // 
#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qbytearray.h> // QByteArrayDataPtr
#include <QtCore/qbytearray.h> // QByteRef
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qchar.h> // QLatin1Char
#include <QtCore/qdatetime.h> // QDate
#include <QtCore/qdatetime.h> // QDateTime
#include <QtCore/qdatetime.h> // QTime
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qhash.h> // QHash
#include <QtCore/qjsonvalue.h> // 
#include <QtCore/qjsonvalue.h> // QJsonValue
#include <QtCore/qjsonvalue.h> // QJsonValueRef
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

void bind_unknown_unknown_7(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B507_[QJsonDocument] ";
	{ // QJsonDocument file: line:82
		pybind11::class_<QJsonDocument, std::shared_ptr<QJsonDocument>> cl(M(""), "QJsonDocument", "");
		cl.def( pybind11::init( [](){ return new QJsonDocument(); } ) );
		cl.def( pybind11::init<const class QJsonObject &>(), pybind11::arg("object") );

		cl.def( pybind11::init<const class QJsonArray &>(), pybind11::arg("array") );

		cl.def( pybind11::init( [](QJsonDocument const &o){ return new QJsonDocument(o); } ) );

		pybind11::enum_<QJsonDocument::DataValidation>(cl, "DataValidation", pybind11::arithmetic(), "")
			.value("Validate", QJsonDocument::Validate)
			.value("BypassValidation", QJsonDocument::BypassValidation)
			.export_values();


		pybind11::enum_<QJsonDocument::JsonFormat>(cl, "JsonFormat", pybind11::arithmetic(), "")
			.value("Indented", QJsonDocument::Indented)
			.value("Compact", QJsonDocument::Compact)
			.export_values();

		cl.def("assign", (class QJsonDocument & (QJsonDocument::*)(const class QJsonDocument &)) &QJsonDocument::operator=, "C++: QJsonDocument::operator=(const class QJsonDocument &) --> class QJsonDocument &", pybind11::return_value_policy::automatic, pybind11::arg("other"));
		cl.def("swap", (void (QJsonDocument::*)(class QJsonDocument &)) &QJsonDocument::swap, "C++: QJsonDocument::swap(class QJsonDocument &) --> void", pybind11::arg("other"));
		cl.def_static("fromRawData", [](const char * a0, int const & a1) -> QJsonDocument { return QJsonDocument::fromRawData(a0, a1); }, "", pybind11::arg("data"), pybind11::arg("size"));
		cl.def_static("fromRawData", (class QJsonDocument (*)(const char *, int, enum QJsonDocument::DataValidation)) &QJsonDocument::fromRawData, "C++: QJsonDocument::fromRawData(const char *, int, enum QJsonDocument::DataValidation) --> class QJsonDocument", pybind11::arg("data"), pybind11::arg("size"), pybind11::arg("validation"));
		cl.def("rawData", (const char * (QJsonDocument::*)(int *) const) &QJsonDocument::rawData, "C++: QJsonDocument::rawData(int *) const --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("size"));
		cl.def_static("fromVariant", (class QJsonDocument (*)(const class QVariant &)) &QJsonDocument::fromVariant, "C++: QJsonDocument::fromVariant(const class QVariant &) --> class QJsonDocument", pybind11::arg("variant"));
		cl.def("toVariant", (class QVariant (QJsonDocument::*)() const) &QJsonDocument::toVariant, "C++: QJsonDocument::toVariant() const --> class QVariant");
		cl.def("isEmpty", (bool (QJsonDocument::*)() const) &QJsonDocument::isEmpty, "C++: QJsonDocument::isEmpty() const --> bool");
		cl.def("isArray", (bool (QJsonDocument::*)() const) &QJsonDocument::isArray, "C++: QJsonDocument::isArray() const --> bool");
		cl.def("isObject", (bool (QJsonDocument::*)() const) &QJsonDocument::isObject, "C++: QJsonDocument::isObject() const --> bool");
		cl.def("object", (class QJsonObject (QJsonDocument::*)() const) &QJsonDocument::object, "C++: QJsonDocument::object() const --> class QJsonObject");
		cl.def("array", (class QJsonArray (QJsonDocument::*)() const) &QJsonDocument::array, "C++: QJsonDocument::array() const --> class QJsonArray");
		cl.def("setObject", (void (QJsonDocument::*)(const class QJsonObject &)) &QJsonDocument::setObject, "C++: QJsonDocument::setObject(const class QJsonObject &) --> void", pybind11::arg("object"));
		cl.def("setArray", (void (QJsonDocument::*)(const class QJsonArray &)) &QJsonDocument::setArray, "C++: QJsonDocument::setArray(const class QJsonArray &) --> void", pybind11::arg("array"));
		cl.def("__getitem__", (const class QJsonValue (QJsonDocument::*)(const class QString &) const) &QJsonDocument::operator[], "C++: QJsonDocument::operator[](const class QString &) const --> const class QJsonValue", pybind11::arg("key"));
		cl.def("__getitem__", (const class QJsonValue (QJsonDocument::*)(class QStringView) const) &QJsonDocument::operator[], "C++: QJsonDocument::operator[](class QStringView) const --> const class QJsonValue", pybind11::arg("key"));
		cl.def("__getitem__", (const class QJsonValue (QJsonDocument::*)(class QLatin1String) const) &QJsonDocument::operator[], "C++: QJsonDocument::operator[](class QLatin1String) const --> const class QJsonValue", pybind11::arg("key"));
		cl.def("__getitem__", (const class QJsonValue (QJsonDocument::*)(int) const) &QJsonDocument::operator[], "C++: QJsonDocument::operator[](int) const --> const class QJsonValue", pybind11::arg("i"));
		cl.def("__eq__", (bool (QJsonDocument::*)(const class QJsonDocument &) const) &QJsonDocument::operator==, "C++: QJsonDocument::operator==(const class QJsonDocument &) const --> bool", pybind11::arg("other"));
		cl.def("__ne__", (bool (QJsonDocument::*)(const class QJsonDocument &) const) &QJsonDocument::operator!=, "C++: QJsonDocument::operator!=(const class QJsonDocument &) const --> bool", pybind11::arg("other"));
		cl.def("isNull", (bool (QJsonDocument::*)() const) &QJsonDocument::isNull, "C++: QJsonDocument::isNull() const --> bool");
	}
	std::cout << "B508_[QTypeInfo<QJsonDocument>] ";
	std::cout << "B509_[void swap(class QJsonDocument &, class QJsonDocument &)] ";
	std::cout << "B510_[QJsonObject] ";
	{ // QJsonObject file: line:59
		pybind11::class_<QJsonObject, std::shared_ptr<QJsonObject>> cl(M(""), "QJsonObject", "");
		cl.def( pybind11::init( [](){ return new QJsonObject(); } ) );
		cl.def( pybind11::init( [](QJsonObject const &o){ return new QJsonObject(o); } ) );
		cl.def("assign", (class QJsonObject & (QJsonObject::*)(const class QJsonObject &)) &QJsonObject::operator=, "C++: QJsonObject::operator=(const class QJsonObject &) --> class QJsonObject &", pybind11::return_value_policy::automatic, pybind11::arg("other"));
		cl.def("swap", (void (QJsonObject::*)(class QJsonObject &)) &QJsonObject::swap, "C++: QJsonObject::swap(class QJsonObject &) --> void", pybind11::arg("other"));
		cl.def("size", (int (QJsonObject::*)() const) &QJsonObject::size, "C++: QJsonObject::size() const --> int");
		cl.def("count", (int (QJsonObject::*)() const) &QJsonObject::count, "C++: QJsonObject::count() const --> int");
		cl.def("length", (int (QJsonObject::*)() const) &QJsonObject::length, "C++: QJsonObject::length() const --> int");
		cl.def("isEmpty", (bool (QJsonObject::*)() const) &QJsonObject::isEmpty, "C++: QJsonObject::isEmpty() const --> bool");
		cl.def("value", (class QJsonValue (QJsonObject::*)(const class QString &) const) &QJsonObject::value, "C++: QJsonObject::value(const class QString &) const --> class QJsonValue", pybind11::arg("key"));
		cl.def("__getitem__", (class QJsonValueRef (QJsonObject::*)(const class QString &)) &QJsonObject::operator[], "C++: QJsonObject::operator[](const class QString &) --> class QJsonValueRef", pybind11::arg("key"));
		cl.def("value", (class QJsonValue (QJsonObject::*)(class QStringView) const) &QJsonObject::value, "C++: QJsonObject::value(class QStringView) const --> class QJsonValue", pybind11::arg("key"));
		cl.def("value", (class QJsonValue (QJsonObject::*)(class QLatin1String) const) &QJsonObject::value, "C++: QJsonObject::value(class QLatin1String) const --> class QJsonValue", pybind11::arg("key"));
		cl.def("__getitem__", (class QJsonValueRef (QJsonObject::*)(class QStringView)) &QJsonObject::operator[], "C++: QJsonObject::operator[](class QStringView) --> class QJsonValueRef", pybind11::arg("key"));
		cl.def("__getitem__", (class QJsonValueRef (QJsonObject::*)(class QLatin1String)) &QJsonObject::operator[], "C++: QJsonObject::operator[](class QLatin1String) --> class QJsonValueRef", pybind11::arg("key"));
		cl.def("remove", (void (QJsonObject::*)(const class QString &)) &QJsonObject::remove, "C++: QJsonObject::remove(const class QString &) --> void", pybind11::arg("key"));
		cl.def("take", (class QJsonValue (QJsonObject::*)(const class QString &)) &QJsonObject::take, "C++: QJsonObject::take(const class QString &) --> class QJsonValue", pybind11::arg("key"));
		cl.def("contains", (bool (QJsonObject::*)(const class QString &) const) &QJsonObject::contains, "C++: QJsonObject::contains(const class QString &) const --> bool", pybind11::arg("key"));
		cl.def("remove", (void (QJsonObject::*)(class QStringView)) &QJsonObject::remove, "C++: QJsonObject::remove(class QStringView) --> void", pybind11::arg("key"));
		cl.def("remove", (void (QJsonObject::*)(class QLatin1String)) &QJsonObject::remove, "C++: QJsonObject::remove(class QLatin1String) --> void", pybind11::arg("key"));
		cl.def("take", (class QJsonValue (QJsonObject::*)(class QStringView)) &QJsonObject::take, "C++: QJsonObject::take(class QStringView) --> class QJsonValue", pybind11::arg("key"));
		cl.def("take", (class QJsonValue (QJsonObject::*)(class QLatin1String)) &QJsonObject::take, "C++: QJsonObject::take(class QLatin1String) --> class QJsonValue", pybind11::arg("key"));
		cl.def("contains", (bool (QJsonObject::*)(class QStringView) const) &QJsonObject::contains, "C++: QJsonObject::contains(class QStringView) const --> bool", pybind11::arg("key"));
		cl.def("contains", (bool (QJsonObject::*)(class QLatin1String) const) &QJsonObject::contains, "C++: QJsonObject::contains(class QLatin1String) const --> bool", pybind11::arg("key"));
		cl.def("__eq__", (bool (QJsonObject::*)(const class QJsonObject &) const) &QJsonObject::operator==, "C++: QJsonObject::operator==(const class QJsonObject &) const --> bool", pybind11::arg("other"));
		cl.def("__ne__", (bool (QJsonObject::*)(const class QJsonObject &) const) &QJsonObject::operator!=, "C++: QJsonObject::operator!=(const class QJsonObject &) const --> bool", pybind11::arg("other"));
		cl.def("begin", (class QJsonObject::iterator (QJsonObject::*)()) &QJsonObject::begin, "C++: QJsonObject::begin() --> class QJsonObject::iterator");
		cl.def("constBegin", (class QJsonObject::const_iterator (QJsonObject::*)() const) &QJsonObject::constBegin, "C++: QJsonObject::constBegin() const --> class QJsonObject::const_iterator");
		cl.def("end", (class QJsonObject::iterator (QJsonObject::*)()) &QJsonObject::end, "C++: QJsonObject::end() --> class QJsonObject::iterator");
		cl.def("constEnd", (class QJsonObject::const_iterator (QJsonObject::*)() const) &QJsonObject::constEnd, "C++: QJsonObject::constEnd() const --> class QJsonObject::const_iterator");
		cl.def("erase", (class QJsonObject::iterator (QJsonObject::*)(class QJsonObject::iterator)) &QJsonObject::erase, "C++: QJsonObject::erase(class QJsonObject::iterator) --> class QJsonObject::iterator", pybind11::arg("it"));
		cl.def("find", (class QJsonObject::iterator (QJsonObject::*)(const class QString &)) &QJsonObject::find, "C++: QJsonObject::find(const class QString &) --> class QJsonObject::iterator", pybind11::arg("key"));
		cl.def("constFind", (class QJsonObject::const_iterator (QJsonObject::*)(const class QString &) const) &QJsonObject::constFind, "C++: QJsonObject::constFind(const class QString &) const --> class QJsonObject::const_iterator", pybind11::arg("key"));
		cl.def("insert", (class QJsonObject::iterator (QJsonObject::*)(const class QString &, const class QJsonValue &)) &QJsonObject::insert, "C++: QJsonObject::insert(const class QString &, const class QJsonValue &) --> class QJsonObject::iterator", pybind11::arg("key"), pybind11::arg("value"));
		cl.def("find", (class QJsonObject::iterator (QJsonObject::*)(class QStringView)) &QJsonObject::find, "C++: QJsonObject::find(class QStringView) --> class QJsonObject::iterator", pybind11::arg("key"));
		cl.def("find", (class QJsonObject::iterator (QJsonObject::*)(class QLatin1String)) &QJsonObject::find, "C++: QJsonObject::find(class QLatin1String) --> class QJsonObject::iterator", pybind11::arg("key"));
		cl.def("constFind", (class QJsonObject::const_iterator (QJsonObject::*)(class QStringView) const) &QJsonObject::constFind, "C++: QJsonObject::constFind(class QStringView) const --> class QJsonObject::const_iterator", pybind11::arg("key"));
		cl.def("constFind", (class QJsonObject::const_iterator (QJsonObject::*)(class QLatin1String) const) &QJsonObject::constFind, "C++: QJsonObject::constFind(class QLatin1String) const --> class QJsonObject::const_iterator", pybind11::arg("key"));
		cl.def("insert", (class QJsonObject::iterator (QJsonObject::*)(class QStringView, const class QJsonValue &)) &QJsonObject::insert, "C++: QJsonObject::insert(class QStringView, const class QJsonValue &) --> class QJsonObject::iterator", pybind11::arg("key"), pybind11::arg("value"));
		cl.def("insert", (class QJsonObject::iterator (QJsonObject::*)(class QLatin1String, const class QJsonValue &)) &QJsonObject::insert, "C++: QJsonObject::insert(class QLatin1String, const class QJsonValue &) --> class QJsonObject::iterator", pybind11::arg("key"), pybind11::arg("value"));
		cl.def("empty", (bool (QJsonObject::*)() const) &QJsonObject::empty, "C++: QJsonObject::empty() const --> bool");

		{ // QJsonObject::const_iterator file: line:179
			auto & enclosing_class = cl;
			pybind11::class_<QJsonObject::const_iterator, std::shared_ptr<QJsonObject::const_iterator>> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init( [](){ return new QJsonObject::const_iterator(); } ) );
			cl.def( pybind11::init<const class QJsonObject *, int>(), pybind11::arg("obj"), pybind11::arg("index") );

			cl.def( pybind11::init<const class QJsonObject::iterator &>(), pybind11::arg("other") );

			cl.def( pybind11::init( [](QJsonObject::const_iterator const &o){ return new QJsonObject::const_iterator(o); } ) );
			cl.def("key", (class QString (QJsonObject::const_iterator::*)() const) &QJsonObject::const_iterator::key, "C++: QJsonObject::const_iterator::key() const --> class QString");
			cl.def("value", (class QJsonValue (QJsonObject::const_iterator::*)() const) &QJsonObject::const_iterator::value, "C++: QJsonObject::const_iterator::value() const --> class QJsonValue");
			cl.def("__mul__", (class QJsonValue (QJsonObject::const_iterator::*)() const) &QJsonObject::const_iterator::operator*, "C++: QJsonObject::const_iterator::operator*() const --> class QJsonValue");
			cl.def("__getitem__", (const class QJsonValue (QJsonObject::const_iterator::*)(int)) &QJsonObject::const_iterator::operator[], "C++: QJsonObject::const_iterator::operator[](int) --> const class QJsonValue", pybind11::arg("j"));
			cl.def("__eq__", (bool (QJsonObject::const_iterator::*)(const class QJsonObject::const_iterator &) const) &QJsonObject::const_iterator::operator==, "C++: QJsonObject::const_iterator::operator==(const class QJsonObject::const_iterator &) const --> bool", pybind11::arg("other"));
			cl.def("__ne__", (bool (QJsonObject::const_iterator::*)(const class QJsonObject::const_iterator &) const) &QJsonObject::const_iterator::operator!=, "C++: QJsonObject::const_iterator::operator!=(const class QJsonObject::const_iterator &) const --> bool", pybind11::arg("other"));
			cl.def("plus_plus", (class QJsonObject::const_iterator & (QJsonObject::const_iterator::*)()) &QJsonObject::const_iterator::operator++, "C++: QJsonObject::const_iterator::operator++() --> class QJsonObject::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class QJsonObject::const_iterator (QJsonObject::const_iterator::*)(int)) &QJsonObject::const_iterator::operator++, "C++: QJsonObject::const_iterator::operator++(int) --> class QJsonObject::const_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class QJsonObject::const_iterator & (QJsonObject::const_iterator::*)()) &QJsonObject::const_iterator::operator--, "C++: QJsonObject::const_iterator::operator--() --> class QJsonObject::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (class QJsonObject::const_iterator (QJsonObject::const_iterator::*)(int)) &QJsonObject::const_iterator::operator--, "C++: QJsonObject::const_iterator::operator--(int) --> class QJsonObject::const_iterator", pybind11::arg(""));
			cl.def("__add__", (class QJsonObject::const_iterator (QJsonObject::const_iterator::*)(int) const) &QJsonObject::const_iterator::operator+, "C++: QJsonObject::const_iterator::operator+(int) const --> class QJsonObject::const_iterator", pybind11::arg("j"));
			cl.def("__sub__", (class QJsonObject::const_iterator (QJsonObject::const_iterator::*)(int) const) &QJsonObject::const_iterator::operator-, "C++: QJsonObject::const_iterator::operator-(int) const --> class QJsonObject::const_iterator", pybind11::arg("j"));
			cl.def("__iadd__", (class QJsonObject::const_iterator & (QJsonObject::const_iterator::*)(int)) &QJsonObject::const_iterator::operator+=, "C++: QJsonObject::const_iterator::operator+=(int) --> class QJsonObject::const_iterator &", pybind11::return_value_policy::automatic, pybind11::arg("j"));
			cl.def("__isub__", (class QJsonObject::const_iterator & (QJsonObject::const_iterator::*)(int)) &QJsonObject::const_iterator::operator-=, "C++: QJsonObject::const_iterator::operator-=(int) --> class QJsonObject::const_iterator &", pybind11::return_value_policy::automatic, pybind11::arg("j"));
			cl.def("__sub__", (int (QJsonObject::const_iterator::*)(class QJsonObject::const_iterator) const) &QJsonObject::const_iterator::operator-, "C++: QJsonObject::const_iterator::operator-(class QJsonObject::const_iterator) const --> int", pybind11::arg("j"));
			cl.def("__eq__", (bool (QJsonObject::const_iterator::*)(const class QJsonObject::iterator &) const) &QJsonObject::const_iterator::operator==, "C++: QJsonObject::const_iterator::operator==(const class QJsonObject::iterator &) const --> bool", pybind11::arg("other"));
			cl.def("__ne__", (bool (QJsonObject::const_iterator::*)(const class QJsonObject::iterator &) const) &QJsonObject::const_iterator::operator!=, "C++: QJsonObject::const_iterator::operator!=(const class QJsonObject::iterator &) const --> bool", pybind11::arg("other"));
		}

		{ // QJsonObject::iterator file: line:124
			auto & enclosing_class = cl;
			pybind11::class_<QJsonObject::iterator, std::shared_ptr<QJsonObject::iterator>> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init( [](){ return new QJsonObject::iterator(); } ) );
			cl.def( pybind11::init<class QJsonObject *, int>(), pybind11::arg("obj"), pybind11::arg("index") );

			cl.def( pybind11::init( [](QJsonObject::iterator const &o){ return new QJsonObject::iterator(o); } ) );
			cl.def("key", (class QString (QJsonObject::iterator::*)() const) &QJsonObject::iterator::key, "C++: QJsonObject::iterator::key() const --> class QString");
			cl.def("value", (class QJsonValueRef (QJsonObject::iterator::*)() const) &QJsonObject::iterator::value, "C++: QJsonObject::iterator::value() const --> class QJsonValueRef");
			cl.def("__mul__", (class QJsonValueRef (QJsonObject::iterator::*)() const) &QJsonObject::iterator::operator*, "C++: QJsonObject::iterator::operator*() const --> class QJsonValueRef");
			cl.def("__getitem__", (const class QJsonValueRef (QJsonObject::iterator::*)(int)) &QJsonObject::iterator::operator[], "C++: QJsonObject::iterator::operator[](int) --> const class QJsonValueRef", pybind11::arg("j"));
			cl.def("__eq__", (bool (QJsonObject::iterator::*)(const class QJsonObject::iterator &) const) &QJsonObject::iterator::operator==, "C++: QJsonObject::iterator::operator==(const class QJsonObject::iterator &) const --> bool", pybind11::arg("other"));
			cl.def("__ne__", (bool (QJsonObject::iterator::*)(const class QJsonObject::iterator &) const) &QJsonObject::iterator::operator!=, "C++: QJsonObject::iterator::operator!=(const class QJsonObject::iterator &) const --> bool", pybind11::arg("other"));
			cl.def("plus_plus", (class QJsonObject::iterator & (QJsonObject::iterator::*)()) &QJsonObject::iterator::operator++, "C++: QJsonObject::iterator::operator++() --> class QJsonObject::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class QJsonObject::iterator (QJsonObject::iterator::*)(int)) &QJsonObject::iterator::operator++, "C++: QJsonObject::iterator::operator++(int) --> class QJsonObject::iterator", pybind11::arg(""));
			cl.def("minus_minus", (class QJsonObject::iterator & (QJsonObject::iterator::*)()) &QJsonObject::iterator::operator--, "C++: QJsonObject::iterator::operator--() --> class QJsonObject::iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (class QJsonObject::iterator (QJsonObject::iterator::*)(int)) &QJsonObject::iterator::operator--, "C++: QJsonObject::iterator::operator--(int) --> class QJsonObject::iterator", pybind11::arg(""));
			cl.def("__add__", (class QJsonObject::iterator (QJsonObject::iterator::*)(int) const) &QJsonObject::iterator::operator+, "C++: QJsonObject::iterator::operator+(int) const --> class QJsonObject::iterator", pybind11::arg("j"));
			cl.def("__sub__", (class QJsonObject::iterator (QJsonObject::iterator::*)(int) const) &QJsonObject::iterator::operator-, "C++: QJsonObject::iterator::operator-(int) const --> class QJsonObject::iterator", pybind11::arg("j"));
			cl.def("__iadd__", (class QJsonObject::iterator & (QJsonObject::iterator::*)(int)) &QJsonObject::iterator::operator+=, "C++: QJsonObject::iterator::operator+=(int) --> class QJsonObject::iterator &", pybind11::return_value_policy::automatic, pybind11::arg("j"));
			cl.def("__isub__", (class QJsonObject::iterator & (QJsonObject::iterator::*)(int)) &QJsonObject::iterator::operator-=, "C++: QJsonObject::iterator::operator-=(int) --> class QJsonObject::iterator &", pybind11::return_value_policy::automatic, pybind11::arg("j"));
			cl.def("__sub__", (int (QJsonObject::iterator::*)(class QJsonObject::iterator) const) &QJsonObject::iterator::operator-, "C++: QJsonObject::iterator::operator-(class QJsonObject::iterator) const --> int", pybind11::arg("j"));
			cl.def("__eq__", (bool (QJsonObject::iterator::*)(const class QJsonObject::const_iterator &) const) &QJsonObject::iterator::operator==, "C++: QJsonObject::iterator::operator==(const class QJsonObject::const_iterator &) const --> bool", pybind11::arg("other"));
			cl.def("__ne__", (bool (QJsonObject::iterator::*)(const class QJsonObject::const_iterator &) const) &QJsonObject::iterator::operator!=, "C++: QJsonObject::iterator::operator!=(const class QJsonObject::const_iterator &) const --> bool", pybind11::arg("other"));
		}

	}
}
