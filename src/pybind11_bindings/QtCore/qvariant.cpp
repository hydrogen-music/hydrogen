#include <QtCore/qbytearray.h> // 
#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qbytearray.h> // QByteArrayDataPtr
#include <QtCore/qbytearray.h> // QByteRef
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qchar.h> // QLatin1Char
#include <QtCore/qdatetime.h> // 
#include <QtCore/qdatetime.h> // QDate
#include <QtCore/qdatetime.h> // QDateTime
#include <QtCore/qdatetime.h> // QTime
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
#include <QtCore/qnamespace.h> // Qt::AspectRatioMode
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::DateFormat
#include <QtCore/qnamespace.h> // Qt::DayOfWeek
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::ItemFlag
#include <QtCore/qnamespace.h> // Qt::LayoutDirection
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
#include <QtCore/qnamespace.h> // Qt::TimeSpec
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
#include <QtCore/qvariant.h> // QVariant::Handler
#include <QtCore/qvariant.h> // QVariant::Private
#include <QtCore/qvariant.h> // QVariant::Private::Data
#include <QtCore/qvariant.h> // QVariant::PrivateShared
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

void bind_QtCore_qvariant(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B366_[QVariant] ";
	{ // QVariant file:QtCore/qvariant.h line:133
		pybind11::class_<QVariant, std::shared_ptr<QVariant>> cl(M(""), "QVariant", "");
		cl.def( pybind11::init( [](){ return new QVariant(); } ) );
		cl.def( pybind11::init<enum QVariant::Type>(), pybind11::arg("type") );

		cl.def( pybind11::init<int, const void *>(), pybind11::arg("typeId"), pybind11::arg("copy") );

		cl.def( pybind11::init<int, const void *, unsigned int>(), pybind11::arg("typeId"), pybind11::arg("copy"), pybind11::arg("flags") );

		cl.def( pybind11::init( [](QVariant const &o){ return new QVariant(o); } ) );
		cl.def( pybind11::init<class QDataStream &>(), pybind11::arg("s") );

		cl.def( pybind11::init<int>(), pybind11::arg("i") );

		cl.def( pybind11::init<unsigned int>(), pybind11::arg("ui") );

		cl.def( pybind11::init<long long>(), pybind11::arg("ll") );

		cl.def( pybind11::init<unsigned long long>(), pybind11::arg("ull") );

		cl.def( pybind11::init<bool>(), pybind11::arg("b") );

		cl.def( pybind11::init<double>(), pybind11::arg("d") );

		cl.def( pybind11::init<float>(), pybind11::arg("f") );

		cl.def( pybind11::init<const char *>(), pybind11::arg("str") );

		cl.def( pybind11::init<const class QBitArray &>(), pybind11::arg("bitarray") );

		cl.def( pybind11::init<const class QString &>(), pybind11::arg("string") );

		cl.def( pybind11::init<class QLatin1String>(), pybind11::arg("string") );

		cl.def( pybind11::init<class QChar>(), pybind11::arg("qchar") );

		cl.def( pybind11::init<const class QDate &>(), pybind11::arg("date") );

		cl.def( pybind11::init<const class QTime &>(), pybind11::arg("time") );

		cl.def( pybind11::init<const class QDateTime &>(), pybind11::arg("datetime") );

		cl.def( pybind11::init<const class QSize &>(), pybind11::arg("size") );

		cl.def( pybind11::init<const class QSizeF &>(), pybind11::arg("size") );

		cl.def( pybind11::init<const class QPointF &>(), pybind11::arg("pt") );

		cl.def( pybind11::init<const class QLine &>(), pybind11::arg("line") );

		cl.def( pybind11::init<const class QLineF &>(), pybind11::arg("line") );

		cl.def( pybind11::init<const class QRect &>(), pybind11::arg("rect") );

		cl.def( pybind11::init<const class QRectF &>(), pybind11::arg("rect") );

		cl.def( pybind11::init<const class QLocale &>(), pybind11::arg("locale") );

		cl.def( pybind11::init<const class QRegExp &>(), pybind11::arg("regExp") );

		cl.def( pybind11::init<const class QEasingCurve &>(), pybind11::arg("easing") );

		cl.def( pybind11::init<const class QUuid &>(), pybind11::arg("uuid") );

		cl.def( pybind11::init<const class QUrl &>(), pybind11::arg("url") );

		cl.def( pybind11::init<const class QJsonValue &>(), pybind11::arg("jsonValue") );

		cl.def( pybind11::init<const class QJsonObject &>(), pybind11::arg("jsonObject") );

		cl.def( pybind11::init<const class QJsonArray &>(), pybind11::arg("jsonArray") );

		cl.def( pybind11::init<const class QJsonDocument &>(), pybind11::arg("jsonDocument") );

		cl.def( pybind11::init<const class QModelIndex &>(), pybind11::arg("modelIndex") );

		cl.def( pybind11::init<const class QPersistentModelIndex &>(), pybind11::arg("modelIndex") );


		pybind11::enum_<QVariant::Type>(cl, "Type", pybind11::arithmetic(), "")
			.value("Invalid", QVariant::Invalid)
			.value("Bool", QVariant::Bool)
			.value("Int", QVariant::Int)
			.value("UInt", QVariant::UInt)
			.value("LongLong", QVariant::LongLong)
			.value("ULongLong", QVariant::ULongLong)
			.value("Double", QVariant::Double)
			.value("Char", QVariant::Char)
			.value("Map", QVariant::Map)
			.value("List", QVariant::List)
			.value("String", QVariant::String)
			.value("StringList", QVariant::StringList)
			.value("ByteArray", QVariant::ByteArray)
			.value("BitArray", QVariant::BitArray)
			.value("Date", QVariant::Date)
			.value("Time", QVariant::Time)
			.value("DateTime", QVariant::DateTime)
			.value("Url", QVariant::Url)
			.value("Locale", QVariant::Locale)
			.value("Rect", QVariant::Rect)
			.value("RectF", QVariant::RectF)
			.value("Size", QVariant::Size)
			.value("SizeF", QVariant::SizeF)
			.value("Line", QVariant::Line)
			.value("LineF", QVariant::LineF)
			.value("Point", QVariant::Point)
			.value("PointF", QVariant::PointF)
			.value("RegExp", QVariant::RegExp)
			.value("RegularExpression", QVariant::RegularExpression)
			.value("Hash", QVariant::Hash)
			.value("EasingCurve", QVariant::EasingCurve)
			.value("Uuid", QVariant::Uuid)
			.value("ModelIndex", QVariant::ModelIndex)
			.value("PersistentModelIndex", QVariant::PersistentModelIndex)
			.value("LastCoreType", QVariant::LastCoreType)
			.value("Font", QVariant::Font)
			.value("Pixmap", QVariant::Pixmap)
			.value("Brush", QVariant::Brush)
			.value("Color", QVariant::Color)
			.value("Palette", QVariant::Palette)
			.value("Image", QVariant::Image)
			.value("Polygon", QVariant::Polygon)
			.value("Region", QVariant::Region)
			.value("Bitmap", QVariant::Bitmap)
			.value("Cursor", QVariant::Cursor)
			.value("KeySequence", QVariant::KeySequence)
			.value("Pen", QVariant::Pen)
			.value("TextLength", QVariant::TextLength)
			.value("TextFormat", QVariant::TextFormat)
			.value("Matrix", QVariant::Matrix)
			.value("Transform", QVariant::Transform)
			.value("Matrix4x4", QVariant::Matrix4x4)
			.value("Vector2D", QVariant::Vector2D)
			.value("Vector3D", QVariant::Vector3D)
			.value("Vector4D", QVariant::Vector4D)
			.value("Quaternion", QVariant::Quaternion)
			.value("PolygonF", QVariant::PolygonF)
			.value("Icon", QVariant::Icon)
			.value("LastGuiType", QVariant::LastGuiType)
			.value("SizePolicy", QVariant::SizePolicy)
			.value("UserType", QVariant::UserType)
			.value("LastType", QVariant::LastType)
			.export_values();

		cl.def("setValue", (void (QVariant::*)(const class QVariant &)) &QVariant::setValue<QVariant>, "C++: QVariant::setValue(const class QVariant &) --> void", pybind11::arg("avalue"));
		cl.def("value", (class QtMetaTypePrivate::QPairVariantInterfaceImpl (QVariant::*)() const) &QVariant::value<QtMetaTypePrivate::QPairVariantInterfaceImpl>, "C++: QVariant::value() const --> class QtMetaTypePrivate::QPairVariantInterfaceImpl");
		cl.def_static("fromValue", (class QVariant (*)(const class QVariant &)) &QVariant::fromValue<QVariant>, "C++: QVariant::fromValue(const class QVariant &) --> class QVariant", pybind11::arg("value"));
		cl.def("assign", (class QVariant & (QVariant::*)(const class QVariant &)) &QVariant::operator=, "C++: QVariant::operator=(const class QVariant &) --> class QVariant &", pybind11::return_value_policy::automatic, pybind11::arg("other"));
		cl.def("swap", (void (QVariant::*)(class QVariant &)) &QVariant::swap, "C++: QVariant::swap(class QVariant &) --> void", pybind11::arg("other"));
		cl.def("type", (enum QVariant::Type (QVariant::*)() const) &QVariant::type, "C++: QVariant::type() const --> enum QVariant::Type");
		cl.def("userType", (int (QVariant::*)() const) &QVariant::userType, "C++: QVariant::userType() const --> int");
		cl.def("typeName", (const char * (QVariant::*)() const) &QVariant::typeName, "C++: QVariant::typeName() const --> const char *", pybind11::return_value_policy::automatic);
		cl.def("canConvert", (bool (QVariant::*)(int) const) &QVariant::canConvert, "C++: QVariant::canConvert(int) const --> bool", pybind11::arg("targetTypeId"));
		cl.def("convert", (bool (QVariant::*)(int)) &QVariant::convert, "C++: QVariant::convert(int) --> bool", pybind11::arg("targetTypeId"));
		cl.def("isValid", (bool (QVariant::*)() const) &QVariant::isValid, "C++: QVariant::isValid() const --> bool");
		cl.def("isNull", (bool (QVariant::*)() const) &QVariant::isNull, "C++: QVariant::isNull() const --> bool");
		cl.def("clear", (void (QVariant::*)()) &QVariant::clear, "C++: QVariant::clear() --> void");
		cl.def("detach", (void (QVariant::*)()) &QVariant::detach, "C++: QVariant::detach() --> void");
		cl.def("isDetached", (bool (QVariant::*)() const) &QVariant::isDetached, "C++: QVariant::isDetached() const --> bool");
		cl.def("toInt", [](QVariant const &o) -> int { return o.toInt(); }, "");
		cl.def("toInt", (int (QVariant::*)(bool *) const) &QVariant::toInt, "C++: QVariant::toInt(bool *) const --> int", pybind11::arg("ok"));
		cl.def("toUInt", [](QVariant const &o) -> unsigned int { return o.toUInt(); }, "");
		cl.def("toUInt", (unsigned int (QVariant::*)(bool *) const) &QVariant::toUInt, "C++: QVariant::toUInt(bool *) const --> unsigned int", pybind11::arg("ok"));
		cl.def("toLongLong", [](QVariant const &o) -> long long { return o.toLongLong(); }, "");
		cl.def("toLongLong", (long long (QVariant::*)(bool *) const) &QVariant::toLongLong, "C++: QVariant::toLongLong(bool *) const --> long long", pybind11::arg("ok"));
		cl.def("toULongLong", [](QVariant const &o) -> unsigned long long { return o.toULongLong(); }, "");
		cl.def("toULongLong", (unsigned long long (QVariant::*)(bool *) const) &QVariant::toULongLong, "C++: QVariant::toULongLong(bool *) const --> unsigned long long", pybind11::arg("ok"));
		cl.def("toBool", (bool (QVariant::*)() const) &QVariant::toBool, "C++: QVariant::toBool() const --> bool");
		cl.def("toDouble", [](QVariant const &o) -> double { return o.toDouble(); }, "");
		cl.def("toDouble", (double (QVariant::*)(bool *) const) &QVariant::toDouble, "C++: QVariant::toDouble(bool *) const --> double", pybind11::arg("ok"));
		cl.def("toFloat", [](QVariant const &o) -> float { return o.toFloat(); }, "");
		cl.def("toFloat", (float (QVariant::*)(bool *) const) &QVariant::toFloat, "C++: QVariant::toFloat(bool *) const --> float", pybind11::arg("ok"));
		cl.def("toReal", [](QVariant const &o) -> double { return o.toReal(); }, "");
		cl.def("toReal", (double (QVariant::*)(bool *) const) &QVariant::toReal, "C++: QVariant::toReal(bool *) const --> double", pybind11::arg("ok"));
		cl.def("toBitArray", (class QBitArray (QVariant::*)() const) &QVariant::toBitArray, "C++: QVariant::toBitArray() const --> class QBitArray");
		cl.def("toString", (class QString (QVariant::*)() const) &QVariant::toString, "C++: QVariant::toString() const --> class QString");
		cl.def("toChar", (class QChar (QVariant::*)() const) &QVariant::toChar, "C++: QVariant::toChar() const --> class QChar");
		cl.def("toDate", (class QDate (QVariant::*)() const) &QVariant::toDate, "C++: QVariant::toDate() const --> class QDate");
		cl.def("toTime", (class QTime (QVariant::*)() const) &QVariant::toTime, "C++: QVariant::toTime() const --> class QTime");
		cl.def("toDateTime", (class QDateTime (QVariant::*)() const) &QVariant::toDateTime, "C++: QVariant::toDateTime() const --> class QDateTime");
		cl.def("toPointF", (class QPointF (QVariant::*)() const) &QVariant::toPointF, "C++: QVariant::toPointF() const --> class QPointF");
		cl.def("toRect", (class QRect (QVariant::*)() const) &QVariant::toRect, "C++: QVariant::toRect() const --> class QRect");
		cl.def("toSize", (class QSize (QVariant::*)() const) &QVariant::toSize, "C++: QVariant::toSize() const --> class QSize");
		cl.def("toSizeF", (class QSizeF (QVariant::*)() const) &QVariant::toSizeF, "C++: QVariant::toSizeF() const --> class QSizeF");
		cl.def("toLine", (class QLine (QVariant::*)() const) &QVariant::toLine, "C++: QVariant::toLine() const --> class QLine");
		cl.def("toLineF", (class QLineF (QVariant::*)() const) &QVariant::toLineF, "C++: QVariant::toLineF() const --> class QLineF");
		cl.def("toRectF", (class QRectF (QVariant::*)() const) &QVariant::toRectF, "C++: QVariant::toRectF() const --> class QRectF");
		cl.def("toLocale", (class QLocale (QVariant::*)() const) &QVariant::toLocale, "C++: QVariant::toLocale() const --> class QLocale");
		cl.def("toRegExp", (class QRegExp (QVariant::*)() const) &QVariant::toRegExp, "C++: QVariant::toRegExp() const --> class QRegExp");
		cl.def("toEasingCurve", (class QEasingCurve (QVariant::*)() const) &QVariant::toEasingCurve, "C++: QVariant::toEasingCurve() const --> class QEasingCurve");
		cl.def("toUuid", (class QUuid (QVariant::*)() const) &QVariant::toUuid, "C++: QVariant::toUuid() const --> class QUuid");
		cl.def("toUrl", (class QUrl (QVariant::*)() const) &QVariant::toUrl, "C++: QVariant::toUrl() const --> class QUrl");
		cl.def("toJsonValue", (class QJsonValue (QVariant::*)() const) &QVariant::toJsonValue, "C++: QVariant::toJsonValue() const --> class QJsonValue");
		cl.def("toJsonObject", (class QJsonObject (QVariant::*)() const) &QVariant::toJsonObject, "C++: QVariant::toJsonObject() const --> class QJsonObject");
		cl.def("toJsonArray", (class QJsonArray (QVariant::*)() const) &QVariant::toJsonArray, "C++: QVariant::toJsonArray() const --> class QJsonArray");
		cl.def("toJsonDocument", (class QJsonDocument (QVariant::*)() const) &QVariant::toJsonDocument, "C++: QVariant::toJsonDocument() const --> class QJsonDocument");
		cl.def("toModelIndex", (class QModelIndex (QVariant::*)() const) &QVariant::toModelIndex, "C++: QVariant::toModelIndex() const --> class QModelIndex");
		cl.def("toPersistentModelIndex", (class QPersistentModelIndex (QVariant::*)() const) &QVariant::toPersistentModelIndex, "C++: QVariant::toPersistentModelIndex() const --> class QPersistentModelIndex");
		cl.def("load", (void (QVariant::*)(class QDataStream &)) &QVariant::load, "C++: QVariant::load(class QDataStream &) --> void", pybind11::arg("ds"));
		cl.def("save", (void (QVariant::*)(class QDataStream &) const) &QVariant::save, "C++: QVariant::save(class QDataStream &) const --> void", pybind11::arg("ds"));
		cl.def_static("typeToName", (const char * (*)(int)) &QVariant::typeToName, "C++: QVariant::typeToName(int) --> const char *", pybind11::return_value_policy::automatic, pybind11::arg("typeId"));
		cl.def_static("nameToType", (enum QVariant::Type (*)(const char *)) &QVariant::nameToType, "C++: QVariant::nameToType(const char *) --> enum QVariant::Type", pybind11::arg("name"));
		cl.def("data", (void * (QVariant::*)()) &QVariant::data, "C++: QVariant::data() --> void *", pybind11::return_value_policy::automatic);
		cl.def("constData", (const void * (QVariant::*)() const) &QVariant::constData, "C++: QVariant::constData() const --> const void *", pybind11::return_value_policy::automatic);
		cl.def("__eq__", (bool (QVariant::*)(const class QVariant &) const) &QVariant::operator==, "C++: QVariant::operator==(const class QVariant &) const --> bool", pybind11::arg("v"));
		cl.def("__ne__", (bool (QVariant::*)(const class QVariant &) const) &QVariant::operator!=, "C++: QVariant::operator!=(const class QVariant &) const --> bool", pybind11::arg("v"));
		cl.def("data_ptr", (struct QVariant::Private & (QVariant::*)()) &QVariant::data_ptr, "C++: QVariant::data_ptr() --> struct QVariant::Private &", pybind11::return_value_policy::automatic);

		{ // QVariant::PrivateShared file:QtCore/qvariant.h line:388
			auto & enclosing_class = cl;
			pybind11::class_<QVariant::PrivateShared, std::shared_ptr<QVariant::PrivateShared>> cl(enclosing_class, "PrivateShared", "");
			cl.def( pybind11::init<void *>(), pybind11::arg("v") );

			cl.def_readwrite("ref", &QVariant::PrivateShared::ref);
		}

		{ // QVariant::Private file:QtCore/qvariant.h line:394
			auto & enclosing_class = cl;
			pybind11::class_<QVariant::Private, std::shared_ptr<QVariant::Private>> cl(enclosing_class, "Private", "");
			cl.def( pybind11::init( [](){ return new QVariant::Private(); } ) );
			cl.def( pybind11::init<unsigned int>(), pybind11::arg("variantType") );

			cl.def( pybind11::init( [](QVariant::Private const &o){ return new QVariant::Private(o); } ) );
			cl.def_readwrite("data", &QVariant::Private::data);
			cl.def("assign", (struct QVariant::Private & (QVariant::Private::*)(const struct QVariant::Private &)) &QVariant::Private::operator=, "C++: QVariant::Private::operator=(const struct QVariant::Private &) --> struct QVariant::Private &", pybind11::return_value_policy::automatic, pybind11::arg("other"));

			{ // QVariant::Private::Data file:QtCore/qvariant.h line:411
				auto & enclosing_class = cl;
				pybind11::class_<QVariant::Private::Data, std::shared_ptr<QVariant::Private::Data>> cl(enclosing_class, "Data", "");
				cl.def( pybind11::init( [](){ return new QVariant::Private::Data(); } ) );
				cl.def( pybind11::init( [](QVariant::Private::Data const &o){ return new QVariant::Private::Data(o); } ) );
				cl.def_readwrite("c", &QVariant::Private::Data::c);
				cl.def_readwrite("uc", &QVariant::Private::Data::uc);
				cl.def_readwrite("s", &QVariant::Private::Data::s);
				cl.def_readwrite("sc", &QVariant::Private::Data::sc);
				cl.def_readwrite("us", &QVariant::Private::Data::us);
				cl.def_readwrite("i", &QVariant::Private::Data::i);
				cl.def_readwrite("u", &QVariant::Private::Data::u);
				cl.def_readwrite("l", &QVariant::Private::Data::l);
				cl.def_readwrite("ul", &QVariant::Private::Data::ul);
				cl.def_readwrite("b", &QVariant::Private::Data::b);
				cl.def_readwrite("d", &QVariant::Private::Data::d);
				cl.def_readwrite("f", &QVariant::Private::Data::f);
				cl.def_readwrite("real", &QVariant::Private::Data::real);
				cl.def_readwrite("ll", &QVariant::Private::Data::ll);
				cl.def_readwrite("ull", &QVariant::Private::Data::ull);
				cl.def("assign", (union QVariant::Private::Data & (QVariant::Private::Data::*)(const union QVariant::Private::Data &)) &QVariant::Private::Data::operator=, "C++: QVariant::Private::Data::operator=(const union QVariant::Private::Data &) --> union QVariant::Private::Data &", pybind11::return_value_policy::automatic, pybind11::arg(""));
			}

		}

		{ // QVariant::Handler file:QtCore/qvariant.h line:448
			auto & enclosing_class = cl;
			pybind11::class_<QVariant::Handler, std::shared_ptr<QVariant::Handler>> cl(enclosing_class, "Handler", "");
			cl.def( pybind11::init( [](){ return new QVariant::Handler(); } ) );
		}

	}
}
