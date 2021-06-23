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
#include <QtCore/qflags.h> // QFlag
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qiodevice.h> // 
#include <QtCore/qiodevice.h> // QIODevice
#include <QtCore/qlist.h> // QList
#include <QtCore/qlocale.h> // 
#include <QtCore/qlocale.h> // QLocale
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::DayOfWeek
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::LayoutDirection
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
#include <QtCore/qobject.h> // QObject
#include <QtCore/qobjectdefs.h> // QMetaObject
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
#include <QtCore/qtextstream.h> // 
#include <QtCore/qtextstream.h> // QTextStream
#include <QtCore/qtextstream.h> // QTextStreamManipulator
#include <QtCore/qvector.h> // QVector
#include <cstdio> // _IO_FILE
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

void bind_QtCore_qtextstream(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B414_[QTextStream] ";
	{ // QTextStream file:QtCore/qtextstream.h line:62
		pybind11::class_<QTextStream, std::shared_ptr<QTextStream>> cl(M(""), "QTextStream", "");
		cl.def( pybind11::init( [](){ return new QTextStream(); } ) );
		cl.def( pybind11::init<class QIODevice *>(), pybind11::arg("device") );


		pybind11::enum_<QTextStream::RealNumberNotation>(cl, "RealNumberNotation", pybind11::arithmetic(), "")
			.value("SmartNotation", QTextStream::SmartNotation)
			.value("FixedNotation", QTextStream::FixedNotation)
			.value("ScientificNotation", QTextStream::ScientificNotation)
			.export_values();


		pybind11::enum_<QTextStream::FieldAlignment>(cl, "FieldAlignment", pybind11::arithmetic(), "")
			.value("AlignLeft", QTextStream::AlignLeft)
			.value("AlignRight", QTextStream::AlignRight)
			.value("AlignCenter", QTextStream::AlignCenter)
			.value("AlignAccountingStyle", QTextStream::AlignAccountingStyle)
			.export_values();


		pybind11::enum_<QTextStream::Status>(cl, "Status", pybind11::arithmetic(), "")
			.value("Ok", QTextStream::Ok)
			.value("ReadPastEnd", QTextStream::ReadPastEnd)
			.value("ReadCorruptData", QTextStream::ReadCorruptData)
			.value("WriteFailed", QTextStream::WriteFailed)
			.export_values();


		pybind11::enum_<QTextStream::NumberFlag>(cl, "NumberFlag", pybind11::arithmetic(), "")
			.value("ShowBase", QTextStream::ShowBase)
			.value("ForcePoint", QTextStream::ForcePoint)
			.value("ForceSign", QTextStream::ForceSign)
			.value("UppercaseBase", QTextStream::UppercaseBase)
			.value("UppercaseDigits", QTextStream::UppercaseDigits)
			.export_values();

		cl.def("setCodec", (void (QTextStream::*)(class QTextCodec *)) &QTextStream::setCodec, "C++: QTextStream::setCodec(class QTextCodec *) --> void", pybind11::arg("codec"));
		cl.def("setCodec", (void (QTextStream::*)(const char *)) &QTextStream::setCodec, "C++: QTextStream::setCodec(const char *) --> void", pybind11::arg("codecName"));
		cl.def("codec", (class QTextCodec * (QTextStream::*)() const) &QTextStream::codec, "C++: QTextStream::codec() const --> class QTextCodec *", pybind11::return_value_policy::automatic);
		cl.def("setAutoDetectUnicode", (void (QTextStream::*)(bool)) &QTextStream::setAutoDetectUnicode, "C++: QTextStream::setAutoDetectUnicode(bool) --> void", pybind11::arg("enabled"));
		cl.def("autoDetectUnicode", (bool (QTextStream::*)() const) &QTextStream::autoDetectUnicode, "C++: QTextStream::autoDetectUnicode() const --> bool");
		cl.def("setGenerateByteOrderMark", (void (QTextStream::*)(bool)) &QTextStream::setGenerateByteOrderMark, "C++: QTextStream::setGenerateByteOrderMark(bool) --> void", pybind11::arg("generate"));
		cl.def("generateByteOrderMark", (bool (QTextStream::*)() const) &QTextStream::generateByteOrderMark, "C++: QTextStream::generateByteOrderMark() const --> bool");
		cl.def("setLocale", (void (QTextStream::*)(const class QLocale &)) &QTextStream::setLocale, "C++: QTextStream::setLocale(const class QLocale &) --> void", pybind11::arg("locale"));
		cl.def("locale", (class QLocale (QTextStream::*)() const) &QTextStream::locale, "C++: QTextStream::locale() const --> class QLocale");
		cl.def("setDevice", (void (QTextStream::*)(class QIODevice *)) &QTextStream::setDevice, "C++: QTextStream::setDevice(class QIODevice *) --> void", pybind11::arg("device"));
		cl.def("device", (class QIODevice * (QTextStream::*)() const) &QTextStream::device, "C++: QTextStream::device() const --> class QIODevice *", pybind11::return_value_policy::automatic);
		cl.def("string", (class QString * (QTextStream::*)() const) &QTextStream::string, "C++: QTextStream::string() const --> class QString *", pybind11::return_value_policy::automatic);
		cl.def("status", (enum QTextStream::Status (QTextStream::*)() const) &QTextStream::status, "C++: QTextStream::status() const --> enum QTextStream::Status");
		cl.def("setStatus", (void (QTextStream::*)(enum QTextStream::Status)) &QTextStream::setStatus, "C++: QTextStream::setStatus(enum QTextStream::Status) --> void", pybind11::arg("status"));
		cl.def("resetStatus", (void (QTextStream::*)()) &QTextStream::resetStatus, "C++: QTextStream::resetStatus() --> void");
		cl.def("atEnd", (bool (QTextStream::*)() const) &QTextStream::atEnd, "C++: QTextStream::atEnd() const --> bool");
		cl.def("reset", (void (QTextStream::*)()) &QTextStream::reset, "C++: QTextStream::reset() --> void");
		cl.def("flush", (void (QTextStream::*)()) &QTextStream::flush, "C++: QTextStream::flush() --> void");
		cl.def("seek", (bool (QTextStream::*)(long long)) &QTextStream::seek, "C++: QTextStream::seek(long long) --> bool", pybind11::arg("pos"));
		cl.def("pos", (long long (QTextStream::*)() const) &QTextStream::pos, "C++: QTextStream::pos() const --> long long");
		cl.def("skipWhiteSpace", (void (QTextStream::*)()) &QTextStream::skipWhiteSpace, "C++: QTextStream::skipWhiteSpace() --> void");
		cl.def("readLine", [](QTextStream &o) -> QString { return o.readLine(); }, "");
		cl.def("readLine", (class QString (QTextStream::*)(long long)) &QTextStream::readLine, "C++: QTextStream::readLine(long long) --> class QString", pybind11::arg("maxlen"));
		cl.def("readLineInto", [](QTextStream &o, class QString * a0) -> bool { return o.readLineInto(a0); }, "", pybind11::arg("line"));
		cl.def("readLineInto", (bool (QTextStream::*)(class QString *, long long)) &QTextStream::readLineInto, "C++: QTextStream::readLineInto(class QString *, long long) --> bool", pybind11::arg("line"), pybind11::arg("maxlen"));
		cl.def("readAll", (class QString (QTextStream::*)()) &QTextStream::readAll, "C++: QTextStream::readAll() --> class QString");
		cl.def("read", (class QString (QTextStream::*)(long long)) &QTextStream::read, "C++: QTextStream::read(long long) --> class QString", pybind11::arg("maxlen"));
		cl.def("setFieldAlignment", (void (QTextStream::*)(enum QTextStream::FieldAlignment)) &QTextStream::setFieldAlignment, "C++: QTextStream::setFieldAlignment(enum QTextStream::FieldAlignment) --> void", pybind11::arg("alignment"));
		cl.def("fieldAlignment", (enum QTextStream::FieldAlignment (QTextStream::*)() const) &QTextStream::fieldAlignment, "C++: QTextStream::fieldAlignment() const --> enum QTextStream::FieldAlignment");
		cl.def("setPadChar", (void (QTextStream::*)(class QChar)) &QTextStream::setPadChar, "C++: QTextStream::setPadChar(class QChar) --> void", pybind11::arg("ch"));
		cl.def("padChar", (class QChar (QTextStream::*)() const) &QTextStream::padChar, "C++: QTextStream::padChar() const --> class QChar");
		cl.def("setFieldWidth", (void (QTextStream::*)(int)) &QTextStream::setFieldWidth, "C++: QTextStream::setFieldWidth(int) --> void", pybind11::arg("width"));
		cl.def("fieldWidth", (int (QTextStream::*)() const) &QTextStream::fieldWidth, "C++: QTextStream::fieldWidth() const --> int");
		cl.def("setIntegerBase", (void (QTextStream::*)(int)) &QTextStream::setIntegerBase, "C++: QTextStream::setIntegerBase(int) --> void", pybind11::arg("base"));
		cl.def("integerBase", (int (QTextStream::*)() const) &QTextStream::integerBase, "C++: QTextStream::integerBase() const --> int");
		cl.def("setRealNumberNotation", (void (QTextStream::*)(enum QTextStream::RealNumberNotation)) &QTextStream::setRealNumberNotation, "C++: QTextStream::setRealNumberNotation(enum QTextStream::RealNumberNotation) --> void", pybind11::arg("notation"));
		cl.def("realNumberNotation", (enum QTextStream::RealNumberNotation (QTextStream::*)() const) &QTextStream::realNumberNotation, "C++: QTextStream::realNumberNotation() const --> enum QTextStream::RealNumberNotation");
		cl.def("setRealNumberPrecision", (void (QTextStream::*)(int)) &QTextStream::setRealNumberPrecision, "C++: QTextStream::setRealNumberPrecision(int) --> void", pybind11::arg("precision"));
		cl.def("realNumberPrecision", (int (QTextStream::*)() const) &QTextStream::realNumberPrecision, "C++: QTextStream::realNumberPrecision() const --> int");
	}
	std::cout << "B415_[QTextStreamManipulator] ";
}
