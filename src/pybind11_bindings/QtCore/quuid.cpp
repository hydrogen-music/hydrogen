#include <QtCore/qbytearray.h> // 
#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qbytearray.h> // QByteArrayDataPtr
#include <QtCore/qbytearray.h> // QByteRef
#include <QtCore/qcborcommon.h> // QCborError
#include <QtCore/qcborcommon.h> // QCborKnownTags
#include <QtCore/qcborcommon.h> // QCborNegativeInteger
#include <QtCore/qcborcommon.h> // QCborSimpleType
#include <QtCore/qcborcommon.h> // QCborTag
#include <QtCore/qcborstreamreader.h> // 
#include <QtCore/qcborstreamreader.h> // QCborStreamReader
#include <QtCore/qcborstreamwriter.h> // QCborStreamWriter
#include <QtCore/qcborvalue.h> // 
#include <QtCore/qcborvalue.h> // QCborParserError
#include <QtCore/qcborvalue.h> // QCborValue
#include <QtCore/qcborvalue.h> // QCborValueRef
#include <QtCore/qcborvalue.h> // qHash
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qchar.h> // QLatin1Char
#include <QtCore/qcoreevent.h> // QEvent
#include <QtCore/qdatetime.h> // QDate
#include <QtCore/qdatetime.h> // QDateTime
#include <QtCore/qdatetime.h> // QTime
#include <QtCore/qflags.h> // QFlag
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qfloat16.h> // qFloatFromFloat16
#include <QtCore/qfloat16.h> // qFloatToFloat16
#include <QtCore/qfloat16.h> // qFpClassify
#include <QtCore/qfloat16.h> // qFuzzyCompare
#include <QtCore/qfloat16.h> // qFuzzyIsNull
#include <QtCore/qfloat16.h> // qIntCast
#include <QtCore/qfloat16.h> // qIsFinite
#include <QtCore/qfloat16.h> // qIsInf
#include <QtCore/qfloat16.h> // qIsNaN
#include <QtCore/qfloat16.h> // qIsNull
#include <QtCore/qfloat16.h> // qRound
#include <QtCore/qfloat16.h> // qRound64
#include <QtCore/qfloat16.h> // qfloat16
#include <QtCore/qhash.h> // QHash
#include <QtCore/qiodevice.h> // QIODevice
#include <QtCore/qjsonvalue.h> // 
#include <QtCore/qjsonvalue.h> // QJsonValue
#include <QtCore/qjsonvalue.h> // QJsonValueRef
#include <QtCore/qlist.h> // QList
#include <QtCore/qlocale.h> // QLocale
#include <QtCore/qmap.h> // QMap
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::QPairVariantInterfaceImpl
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::ConnectionType
#include <QtCore/qnamespace.h> // Qt::DateFormat
#include <QtCore/qnamespace.h> // Qt::Initialization
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
#include <QtCore/quuid.h> // qHash
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

void bind_QtCore_quuid(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B416_[QUuid] ";
	{ // QUuid file:QtCore/quuid.h line:66
		pybind11::class_<QUuid, std::shared_ptr<QUuid>> cl(M(""), "QUuid", "");
		cl.def( pybind11::init( [](){ return new QUuid(); } ) );
		cl.def( pybind11::init<unsigned int, unsigned short, unsigned short, unsigned char, unsigned char, unsigned char, unsigned char, unsigned char, unsigned char, unsigned char, unsigned char>(), pybind11::arg("l"), pybind11::arg("w1"), pybind11::arg("w2"), pybind11::arg("b1"), pybind11::arg("b2"), pybind11::arg("b3"), pybind11::arg("b4"), pybind11::arg("b5"), pybind11::arg("b6"), pybind11::arg("b7"), pybind11::arg("b8") );

		cl.def( pybind11::init<const class QString &>(), pybind11::arg("") );

		cl.def( pybind11::init<const char *>(), pybind11::arg("") );

		cl.def( pybind11::init( [](QUuid const &o){ return new QUuid(o); } ) );

		pybind11::enum_<QUuid::Variant>(cl, "Variant", pybind11::arithmetic(), "")
			.value("VarUnknown", QUuid::VarUnknown)
			.value("NCS", QUuid::NCS)
			.value("DCE", QUuid::DCE)
			.value("Microsoft", QUuid::Microsoft)
			.value("Reserved", QUuid::Reserved)
			.export_values();


		pybind11::enum_<QUuid::Version>(cl, "Version", pybind11::arithmetic(), "")
			.value("VerUnknown", QUuid::VerUnknown)
			.value("Time", QUuid::Time)
			.value("EmbeddedPOSIX", QUuid::EmbeddedPOSIX)
			.value("Md5", QUuid::Md5)
			.value("Name", QUuid::Name)
			.value("Random", QUuid::Random)
			.value("Sha1", QUuid::Sha1)
			.export_values();


		pybind11::enum_<QUuid::StringFormat>(cl, "StringFormat", pybind11::arithmetic(), "")
			.value("WithBraces", QUuid::WithBraces)
			.value("WithoutBraces", QUuid::WithoutBraces)
			.value("Id128", QUuid::Id128)
			.export_values();

		cl.def_readwrite("data1", &QUuid::data1);
		cl.def_readwrite("data2", &QUuid::data2);
		cl.def_readwrite("data3", &QUuid::data3);
		cl.def_static("fromString", (class QUuid (*)(class QStringView)) &QUuid::fromString, "C++: QUuid::fromString(class QStringView) --> class QUuid", pybind11::arg("string"));
		cl.def_static("fromString", (class QUuid (*)(class QLatin1String)) &QUuid::fromString, "C++: QUuid::fromString(class QLatin1String) --> class QUuid", pybind11::arg("string"));
		cl.def("toString", (class QString (QUuid::*)() const) &QUuid::toString, "C++: QUuid::toString() const --> class QString");
		cl.def("toString", (class QString (QUuid::*)(enum QUuid::StringFormat) const) &QUuid::toString, "C++: QUuid::toString(enum QUuid::StringFormat) const --> class QString", pybind11::arg("mode"));
		cl.def("isNull", (bool (QUuid::*)() const) &QUuid::isNull, "C++: QUuid::isNull() const --> bool");
		cl.def("__eq__", (bool (QUuid::*)(const class QUuid &) const) &QUuid::operator==, "C++: QUuid::operator==(const class QUuid &) const --> bool", pybind11::arg("orig"));
		cl.def("__ne__", (bool (QUuid::*)(const class QUuid &) const) &QUuid::operator!=, "C++: QUuid::operator!=(const class QUuid &) const --> bool", pybind11::arg("orig"));
		cl.def_static("createUuid", (class QUuid (*)()) &QUuid::createUuid, "C++: QUuid::createUuid() --> class QUuid");
		cl.def_static("createUuidV3", (class QUuid (*)(const class QUuid &, const class QString &)) &QUuid::createUuidV3, "C++: QUuid::createUuidV3(const class QUuid &, const class QString &) --> class QUuid", pybind11::arg("ns"), pybind11::arg("baseData"));
		cl.def_static("createUuidV5", (class QUuid (*)(const class QUuid &, const class QString &)) &QUuid::createUuidV5, "C++: QUuid::createUuidV5(const class QUuid &, const class QString &) --> class QUuid", pybind11::arg("ns"), pybind11::arg("baseData"));
		cl.def("variant", (enum QUuid::Variant (QUuid::*)() const) &QUuid::variant, "C++: QUuid::variant() const --> enum QUuid::Variant");
		cl.def("version", (enum QUuid::Version (QUuid::*)() const) &QUuid::version, "C++: QUuid::version() const --> enum QUuid::Version");
	}
	std::cout << "B417_[QTypeInfo<QUuid>] ";
	std::cout << "B418_[unsigned int qHash(const class QUuid &, unsigned int)] ";
	std::cout << "B419_[QCborParserError] ";
	std::cout << "B420_[QCborValue] ";
	std::cout << "B421_[const struct QMetaObject * qt_getEnumMetaObject(enum QCborValue::Type)] ";
	std::cout << "B422_[const char * qt_getEnumName(enum QCborValue::Type)] ";
	std::cout << "B423_[QTypeInfo<QCborValue>] ";
	std::cout << "B424_[void swap(class QCborValue &, class QCborValue &)] ";
	std::cout << "B425_[QCborValueRef] ";
	std::cout << "B426_[unsigned int qHash(const class QCborValue &, unsigned int)] ";
	std::cout << "B427_[QCborArray] ";
	std::cout << "B428_[QTypeInfo<QCborArray>] ";
	std::cout << "B429_[void swap(class QCborArray &, class QCborArray &)] ";
	std::cout << "B430_[unsigned int qHash(const class QCborArray &, unsigned int)] ";
	std::cout << "B431_[QCborMap] ";
	std::cout << "B432_[QTypeInfo<QCborMap>] ";
	std::cout << "B433_[void swap(class QCborMap &, class QCborMap &)] ";
	std::cout << "B434_[unsigned int qHash(const class QCborMap &, unsigned int)] ";
	std::cout << "B435_[qfloat16] ";
	std::cout << "B436_[bool qIsNull(class qfloat16)] ";
	std::cout << "B437_[QTypeInfo<qfloat16>] ";
	std::cout << "B438_[void qFloatToFloat16(class qfloat16 *, const float *, long long)] ";
	std::cout << "B439_[void qFloatFromFloat16(float *, const class qfloat16 *, long long)] ";
	std::cout << "B440_[bool qIsInf(class qfloat16)] ";
	std::cout << "B441_[bool qIsNaN(class qfloat16)] ";
	std::cout << "B442_[bool qIsFinite(class qfloat16)] ";
	std::cout << "B443_[int qFpClassify(class qfloat16)] ";
	std::cout << "B444_[int qRound(class qfloat16)] ";
	std::cout << "B445_[long long qRound64(class qfloat16)] ";
	std::cout << "B446_[bool qFuzzyCompare(class qfloat16, class qfloat16)] ";
	std::cout << "B447_[int qIntCast(class qfloat16)] ";
	std::cout << "B448_[bool qFuzzyIsNull(class qfloat16)] ";
	std::cout << "B449_[QMetaTypeId<qfloat16>] ";
}
