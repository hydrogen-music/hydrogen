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
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qlist.h> // QList
#include <QtCore/qlocale.h> // 
#include <QtCore/qlocale.h> // QLocale
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::QPairVariantInterfaceImpl
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::ConnectionType
#include <QtCore/qnamespace.h> // Qt::DateFormat
#include <QtCore/qnamespace.h> // Qt::DayOfWeek
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::LayoutDirection
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
#include <QtCore/qnamespace.h> // Qt::TimeSpec
#include <QtCore/qnamespace.h> // Qt::TimerType
#include <QtCore/qobject.h> // QObject
#include <QtCore/qobject.h> // QObjectUserData
#include <QtCore/qobjectdefs.h> // QGenericArgument
#include <QtCore/qobjectdefs.h> // QGenericReturnArgument
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

void bind_unknown_unknown_4(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B410_[QCalendar] ";
	{ // QCalendar file: line:88
		pybind11::class_<QCalendar, std::shared_ptr<QCalendar>> cl(M(""), "QCalendar", "");
		cl.def( pybind11::init( [](){ return new QCalendar(); } ) );
		cl.def( pybind11::init<enum QCalendar::System>(), pybind11::arg("system") );

		cl.def( pybind11::init<class QLatin1String>(), pybind11::arg("name") );

		cl.def( pybind11::init<class QStringView>(), pybind11::arg("name") );


		pybind11::enum_<QCalendar::System>(cl, "System", "")
			.value("Gregorian", QCalendar::System::Gregorian)
			.value("Julian", QCalendar::System::Julian)
			.value("Milankovic", QCalendar::System::Milankovic)
			.value("Jalali", QCalendar::System::Jalali)
			.value("IslamicCivil", QCalendar::System::IslamicCivil)
			.value("Last", QCalendar::System::Last)
			.value("User", QCalendar::System::User);

		cl.def("isValid", (bool (QCalendar::*)() const) &QCalendar::isValid, "C++: QCalendar::isValid() const --> bool");
		cl.def("daysInMonth", [](QCalendar const &o, int const & a0) -> int { return o.daysInMonth(a0); }, "", pybind11::arg("month"));
		cl.def("daysInMonth", (int (QCalendar::*)(int, int) const) &QCalendar::daysInMonth, "C++: QCalendar::daysInMonth(int, int) const --> int", pybind11::arg("month"), pybind11::arg("year"));
		cl.def("daysInYear", (int (QCalendar::*)(int) const) &QCalendar::daysInYear, "C++: QCalendar::daysInYear(int) const --> int", pybind11::arg("year"));
		cl.def("monthsInYear", (int (QCalendar::*)(int) const) &QCalendar::monthsInYear, "C++: QCalendar::monthsInYear(int) const --> int", pybind11::arg("year"));
		cl.def("isDateValid", (bool (QCalendar::*)(int, int, int) const) &QCalendar::isDateValid, "C++: QCalendar::isDateValid(int, int, int) const --> bool", pybind11::arg("year"), pybind11::arg("month"), pybind11::arg("day"));
		cl.def("isLeapYear", (bool (QCalendar::*)(int) const) &QCalendar::isLeapYear, "C++: QCalendar::isLeapYear(int) const --> bool", pybind11::arg("year"));
		cl.def("isGregorian", (bool (QCalendar::*)() const) &QCalendar::isGregorian, "C++: QCalendar::isGregorian() const --> bool");
		cl.def("isLunar", (bool (QCalendar::*)() const) &QCalendar::isLunar, "C++: QCalendar::isLunar() const --> bool");
		cl.def("isLuniSolar", (bool (QCalendar::*)() const) &QCalendar::isLuniSolar, "C++: QCalendar::isLuniSolar() const --> bool");
		cl.def("isSolar", (bool (QCalendar::*)() const) &QCalendar::isSolar, "C++: QCalendar::isSolar() const --> bool");
		cl.def("isProleptic", (bool (QCalendar::*)() const) &QCalendar::isProleptic, "C++: QCalendar::isProleptic() const --> bool");
		cl.def("hasYearZero", (bool (QCalendar::*)() const) &QCalendar::hasYearZero, "C++: QCalendar::hasYearZero() const --> bool");
		cl.def("maximumDaysInMonth", (int (QCalendar::*)() const) &QCalendar::maximumDaysInMonth, "C++: QCalendar::maximumDaysInMonth() const --> int");
		cl.def("minimumDaysInMonth", (int (QCalendar::*)() const) &QCalendar::minimumDaysInMonth, "C++: QCalendar::minimumDaysInMonth() const --> int");
		cl.def("maximumMonthsInYear", (int (QCalendar::*)() const) &QCalendar::maximumMonthsInYear, "C++: QCalendar::maximumMonthsInYear() const --> int");
		cl.def("name", (class QString (QCalendar::*)() const) &QCalendar::name, "C++: QCalendar::name() const --> class QString");
		cl.def("dateFromParts", (class QDate (QCalendar::*)(int, int, int) const) &QCalendar::dateFromParts, "C++: QCalendar::dateFromParts(int, int, int) const --> class QDate", pybind11::arg("year"), pybind11::arg("month"), pybind11::arg("day"));
		cl.def("dateFromParts", (class QDate (QCalendar::*)(const struct QCalendar::YearMonthDay &) const) &QCalendar::dateFromParts, "C++: QCalendar::dateFromParts(const struct QCalendar::YearMonthDay &) const --> class QDate", pybind11::arg("parts"));
		cl.def("partsFromDate", (struct QCalendar::YearMonthDay (QCalendar::*)(class QDate) const) &QCalendar::partsFromDate, "C++: QCalendar::partsFromDate(class QDate) const --> struct QCalendar::YearMonthDay", pybind11::arg("date"));
		cl.def("dayOfWeek", (int (QCalendar::*)(class QDate) const) &QCalendar::dayOfWeek, "C++: QCalendar::dayOfWeek(class QDate) const --> int", pybind11::arg("date"));
		cl.def("monthName", [](QCalendar const &o, const class QLocale & a0, int const & a1) -> QString { return o.monthName(a0, a1); }, "", pybind11::arg("locale"), pybind11::arg("month"));
		cl.def("monthName", [](QCalendar const &o, const class QLocale & a0, int const & a1, int const & a2) -> QString { return o.monthName(a0, a1, a2); }, "", pybind11::arg("locale"), pybind11::arg("month"), pybind11::arg("year"));
		cl.def("monthName", (class QString (QCalendar::*)(const class QLocale &, int, int, enum QLocale::FormatType) const) &QCalendar::monthName, "C++: QCalendar::monthName(const class QLocale &, int, int, enum QLocale::FormatType) const --> class QString", pybind11::arg("locale"), pybind11::arg("month"), pybind11::arg("year"), pybind11::arg("format"));
		cl.def("standaloneMonthName", [](QCalendar const &o, const class QLocale & a0, int const & a1) -> QString { return o.standaloneMonthName(a0, a1); }, "", pybind11::arg("locale"), pybind11::arg("month"));
		cl.def("standaloneMonthName", [](QCalendar const &o, const class QLocale & a0, int const & a1, int const & a2) -> QString { return o.standaloneMonthName(a0, a1, a2); }, "", pybind11::arg("locale"), pybind11::arg("month"), pybind11::arg("year"));
		cl.def("standaloneMonthName", (class QString (QCalendar::*)(const class QLocale &, int, int, enum QLocale::FormatType) const) &QCalendar::standaloneMonthName, "C++: QCalendar::standaloneMonthName(const class QLocale &, int, int, enum QLocale::FormatType) const --> class QString", pybind11::arg("locale"), pybind11::arg("month"), pybind11::arg("year"), pybind11::arg("format"));
		cl.def("weekDayName", [](QCalendar const &o, const class QLocale & a0, int const & a1) -> QString { return o.weekDayName(a0, a1); }, "", pybind11::arg("locale"), pybind11::arg("day"));
		cl.def("weekDayName", (class QString (QCalendar::*)(const class QLocale &, int, enum QLocale::FormatType) const) &QCalendar::weekDayName, "C++: QCalendar::weekDayName(const class QLocale &, int, enum QLocale::FormatType) const --> class QString", pybind11::arg("locale"), pybind11::arg("day"), pybind11::arg("format"));
		cl.def("standaloneWeekDayName", [](QCalendar const &o, const class QLocale & a0, int const & a1) -> QString { return o.standaloneWeekDayName(a0, a1); }, "", pybind11::arg("locale"), pybind11::arg("day"));
		cl.def("standaloneWeekDayName", (class QString (QCalendar::*)(const class QLocale &, int, enum QLocale::FormatType) const) &QCalendar::standaloneWeekDayName, "C++: QCalendar::standaloneWeekDayName(const class QLocale &, int, enum QLocale::FormatType) const --> class QString", pybind11::arg("locale"), pybind11::arg("day"), pybind11::arg("format"));
		cl.def("dateTimeToString", (class QString (QCalendar::*)(class QStringView, const class QDateTime &, const class QDate &, const class QTime &, const class QLocale &) const) &QCalendar::dateTimeToString, "C++: QCalendar::dateTimeToString(class QStringView, const class QDateTime &, const class QDate &, const class QTime &, const class QLocale &) const --> class QString", pybind11::arg("format"), pybind11::arg("datetime"), pybind11::arg("dateOnly"), pybind11::arg("timeOnly"), pybind11::arg("locale"));

		{ // QCalendar::YearMonthDay file: line:94
			auto & enclosing_class = cl;
			pybind11::class_<QCalendar::YearMonthDay, std::shared_ptr<QCalendar::YearMonthDay>> cl(enclosing_class, "YearMonthDay", "");
			cl.def( pybind11::init( [](){ return new QCalendar::YearMonthDay(); } ) );
			cl.def( pybind11::init( [](int const & a0){ return new QCalendar::YearMonthDay(a0); } ), "doc" , pybind11::arg("y"));
			cl.def( pybind11::init( [](int const & a0, int const & a1){ return new QCalendar::YearMonthDay(a0, a1); } ), "doc" , pybind11::arg("y"), pybind11::arg("m"));
			cl.def( pybind11::init<int, int, int>(), pybind11::arg("y"), pybind11::arg("m"), pybind11::arg("d") );

			cl.def_readwrite("year", &QCalendar::YearMonthDay::year);
			cl.def_readwrite("month", &QCalendar::YearMonthDay::month);
			cl.def_readwrite("day", &QCalendar::YearMonthDay::day);
			cl.def("isValid", (bool (QCalendar::YearMonthDay::*)() const) &QCalendar::YearMonthDay::isValid, "C++: QCalendar::YearMonthDay::isValid() const --> bool");
		}

	}
	std::cout << "B411_[const struct QMetaObject * qt_getEnumMetaObject(enum QCalendar::System)] ";
	std::cout << "B412_[const char * qt_getEnumName(enum QCalendar::System)] ";
	std::cout << "B413_[QDate] ";
	{ // QDate file:QtCore/qdatetime.h line:63
		pybind11::class_<QDate, std::shared_ptr<QDate>> cl(M(""), "QDate", "");
		cl.def( pybind11::init( [](){ return new QDate(); } ) );
		cl.def( pybind11::init<int, int, int>(), pybind11::arg("y"), pybind11::arg("m"), pybind11::arg("d") );

		cl.def( pybind11::init<int, int, int, class QCalendar>(), pybind11::arg("y"), pybind11::arg("m"), pybind11::arg("d"), pybind11::arg("cal") );

		cl.def( pybind11::init( [](QDate const &o){ return new QDate(o); } ) );

		pybind11::enum_<QDate::MonthNameType>(cl, "MonthNameType", pybind11::arithmetic(), "")
			.value("DateFormat", QDate::DateFormat)
			.value("StandaloneFormat", QDate::StandaloneFormat)
			.export_values();

		cl.def("isNull", (bool (QDate::*)() const) &QDate::isNull, "C++: QDate::isNull() const --> bool");
		cl.def("year", (int (QDate::*)() const) &QDate::year, "C++: QDate::year() const --> int");
		cl.def("month", (int (QDate::*)() const) &QDate::month, "C++: QDate::month() const --> int");
		cl.def("day", (int (QDate::*)() const) &QDate::day, "C++: QDate::day() const --> int");
		cl.def("dayOfWeek", (int (QDate::*)() const) &QDate::dayOfWeek, "C++: QDate::dayOfWeek() const --> int");
		cl.def("dayOfYear", (int (QDate::*)() const) &QDate::dayOfYear, "C++: QDate::dayOfYear() const --> int");
		cl.def("daysInMonth", (int (QDate::*)() const) &QDate::daysInMonth, "C++: QDate::daysInMonth() const --> int");
		cl.def("daysInYear", (int (QDate::*)() const) &QDate::daysInYear, "C++: QDate::daysInYear() const --> int");
		cl.def("weekNumber", [](QDate const &o) -> int { return o.weekNumber(); }, "");
		cl.def("weekNumber", (int (QDate::*)(int *) const) &QDate::weekNumber, "C++: QDate::weekNumber(int *) const --> int", pybind11::arg("yearNum"));
		cl.def("year", (int (QDate::*)(class QCalendar) const) &QDate::year, "C++: QDate::year(class QCalendar) const --> int", pybind11::arg("cal"));
		cl.def("month", (int (QDate::*)(class QCalendar) const) &QDate::month, "C++: QDate::month(class QCalendar) const --> int", pybind11::arg("cal"));
		cl.def("day", (int (QDate::*)(class QCalendar) const) &QDate::day, "C++: QDate::day(class QCalendar) const --> int", pybind11::arg("cal"));
		cl.def("dayOfWeek", (int (QDate::*)(class QCalendar) const) &QDate::dayOfWeek, "C++: QDate::dayOfWeek(class QCalendar) const --> int", pybind11::arg("cal"));
		cl.def("dayOfYear", (int (QDate::*)(class QCalendar) const) &QDate::dayOfYear, "C++: QDate::dayOfYear(class QCalendar) const --> int", pybind11::arg("cal"));
		cl.def("daysInMonth", (int (QDate::*)(class QCalendar) const) &QDate::daysInMonth, "C++: QDate::daysInMonth(class QCalendar) const --> int", pybind11::arg("cal"));
		cl.def("daysInYear", (int (QDate::*)(class QCalendar) const) &QDate::daysInYear, "C++: QDate::daysInYear(class QCalendar) const --> int", pybind11::arg("cal"));
		cl.def("startOfDay", [](QDate const &o) -> QDateTime { return o.startOfDay(); }, "");
		cl.def("startOfDay", [](QDate const &o, enum Qt::TimeSpec const & a0) -> QDateTime { return o.startOfDay(a0); }, "", pybind11::arg("spec"));
		cl.def("startOfDay", (class QDateTime (QDate::*)(enum Qt::TimeSpec, int) const) &QDate::startOfDay, "C++: QDate::startOfDay(enum Qt::TimeSpec, int) const --> class QDateTime", pybind11::arg("spec"), pybind11::arg("offsetSeconds"));
		cl.def("endOfDay", [](QDate const &o) -> QDateTime { return o.endOfDay(); }, "");
		cl.def("endOfDay", [](QDate const &o, enum Qt::TimeSpec const & a0) -> QDateTime { return o.endOfDay(a0); }, "", pybind11::arg("spec"));
		cl.def("endOfDay", (class QDateTime (QDate::*)(enum Qt::TimeSpec, int) const) &QDate::endOfDay, "C++: QDate::endOfDay(enum Qt::TimeSpec, int) const --> class QDateTime", pybind11::arg("spec"), pybind11::arg("offsetSeconds"));
		cl.def("startOfDay", (class QDateTime (QDate::*)(const class QTimeZone &) const) &QDate::startOfDay, "C++: QDate::startOfDay(const class QTimeZone &) const --> class QDateTime", pybind11::arg("zone"));
		cl.def("endOfDay", (class QDateTime (QDate::*)(const class QTimeZone &) const) &QDate::endOfDay, "C++: QDate::endOfDay(const class QTimeZone &) const --> class QDateTime", pybind11::arg("zone"));
		cl.def_static("shortMonthName", [](int const & a0) -> QString { return QDate::shortMonthName(a0); }, "", pybind11::arg("month"));
		cl.def_static("shortMonthName", (class QString (*)(int, enum QDate::MonthNameType)) &QDate::shortMonthName, "C++: QDate::shortMonthName(int, enum QDate::MonthNameType) --> class QString", pybind11::arg("month"), pybind11::arg("type"));
		cl.def_static("shortDayName", [](int const & a0) -> QString { return QDate::shortDayName(a0); }, "", pybind11::arg("weekday"));
		cl.def_static("shortDayName", (class QString (*)(int, enum QDate::MonthNameType)) &QDate::shortDayName, "C++: QDate::shortDayName(int, enum QDate::MonthNameType) --> class QString", pybind11::arg("weekday"), pybind11::arg("type"));
		cl.def_static("longMonthName", [](int const & a0) -> QString { return QDate::longMonthName(a0); }, "", pybind11::arg("month"));
		cl.def_static("longMonthName", (class QString (*)(int, enum QDate::MonthNameType)) &QDate::longMonthName, "C++: QDate::longMonthName(int, enum QDate::MonthNameType) --> class QString", pybind11::arg("month"), pybind11::arg("type"));
		cl.def_static("longDayName", [](int const & a0) -> QString { return QDate::longDayName(a0); }, "", pybind11::arg("weekday"));
		cl.def_static("longDayName", (class QString (*)(int, enum QDate::MonthNameType)) &QDate::longDayName, "C++: QDate::longDayName(int, enum QDate::MonthNameType) --> class QString", pybind11::arg("weekday"), pybind11::arg("type"));
		cl.def("toString", [](QDate const &o) -> QString { return o.toString(); }, "");
		cl.def("toString", (class QString (QDate::*)(enum Qt::DateFormat) const) &QDate::toString, "C++: QDate::toString(enum Qt::DateFormat) const --> class QString", pybind11::arg("format"));
		cl.def("toString", (class QString (QDate::*)(enum Qt::DateFormat, class QCalendar) const) &QDate::toString, "C++: QDate::toString(enum Qt::DateFormat, class QCalendar) const --> class QString", pybind11::arg("format"), pybind11::arg("cal"));
		cl.def("toString", (class QString (QDate::*)(const class QString &) const) &QDate::toString, "C++: QDate::toString(const class QString &) const --> class QString", pybind11::arg("format"));
		cl.def("toString", (class QString (QDate::*)(const class QString &, class QCalendar) const) &QDate::toString, "C++: QDate::toString(const class QString &, class QCalendar) const --> class QString", pybind11::arg("format"), pybind11::arg("cal"));
		cl.def("toString", (class QString (QDate::*)(class QStringView) const) &QDate::toString, "C++: QDate::toString(class QStringView) const --> class QString", pybind11::arg("format"));
		cl.def("toString", (class QString (QDate::*)(class QStringView, class QCalendar) const) &QDate::toString, "C++: QDate::toString(class QStringView, class QCalendar) const --> class QString", pybind11::arg("format"), pybind11::arg("cal"));
		cl.def("setDate", (bool (QDate::*)(int, int, int)) &QDate::setDate, "C++: QDate::setDate(int, int, int) --> bool", pybind11::arg("year"), pybind11::arg("month"), pybind11::arg("day"));
		cl.def("setDate", (bool (QDate::*)(int, int, int, class QCalendar)) &QDate::setDate, "C++: QDate::setDate(int, int, int, class QCalendar) --> bool", pybind11::arg("year"), pybind11::arg("month"), pybind11::arg("day"), pybind11::arg("cal"));
		cl.def("getDate", (void (QDate::*)(int *, int *, int *)) &QDate::getDate, "C++: QDate::getDate(int *, int *, int *) --> void", pybind11::arg("year"), pybind11::arg("month"), pybind11::arg("day"));
		cl.def("addDays", (class QDate (QDate::*)(long long) const) &QDate::addDays, "C++: QDate::addDays(long long) const --> class QDate", pybind11::arg("days"));
		cl.def("addMonths", (class QDate (QDate::*)(int) const) &QDate::addMonths, "C++: QDate::addMonths(int) const --> class QDate", pybind11::arg("months"));
		cl.def("addYears", (class QDate (QDate::*)(int) const) &QDate::addYears, "C++: QDate::addYears(int) const --> class QDate", pybind11::arg("years"));
		cl.def("addMonths", (class QDate (QDate::*)(int, class QCalendar) const) &QDate::addMonths, "C++: QDate::addMonths(int, class QCalendar) const --> class QDate", pybind11::arg("months"), pybind11::arg("cal"));
		cl.def("addYears", (class QDate (QDate::*)(int, class QCalendar) const) &QDate::addYears, "C++: QDate::addYears(int, class QCalendar) const --> class QDate", pybind11::arg("years"), pybind11::arg("cal"));
		cl.def("daysTo", (long long (QDate::*)(const class QDate &) const) &QDate::daysTo, "C++: QDate::daysTo(const class QDate &) const --> long long", pybind11::arg(""));
		cl.def("__eq__", (bool (QDate::*)(const class QDate &) const) &QDate::operator==, "C++: QDate::operator==(const class QDate &) const --> bool", pybind11::arg("other"));
		cl.def("__ne__", (bool (QDate::*)(const class QDate &) const) &QDate::operator!=, "C++: QDate::operator!=(const class QDate &) const --> bool", pybind11::arg("other"));
		cl.def_static("currentDate", (class QDate (*)()) &QDate::currentDate, "C++: QDate::currentDate() --> class QDate");
		cl.def_static("fromString", [](const class QString & a0) -> QDate { return QDate::fromString(a0); }, "", pybind11::arg("s"));
		cl.def_static("fromString", (class QDate (*)(const class QString &, enum Qt::DateFormat)) &QDate::fromString, "C++: QDate::fromString(const class QString &, enum Qt::DateFormat) --> class QDate", pybind11::arg("s"), pybind11::arg("f"));
		cl.def_static("fromString", (class QDate (*)(const class QString &, const class QString &)) &QDate::fromString, "C++: QDate::fromString(const class QString &, const class QString &) --> class QDate", pybind11::arg("s"), pybind11::arg("format"));
		cl.def_static("fromString", (class QDate (*)(const class QString &, const class QString &, class QCalendar)) &QDate::fromString, "C++: QDate::fromString(const class QString &, const class QString &, class QCalendar) --> class QDate", pybind11::arg("s"), pybind11::arg("format"), pybind11::arg("cal"));
		cl.def_static("isLeapYear", (bool (*)(int)) &QDate::isLeapYear, "C++: QDate::isLeapYear(int) --> bool", pybind11::arg("year"));
		cl.def_static("fromJulianDay", (class QDate (*)(long long)) &QDate::fromJulianDay, "C++: QDate::fromJulianDay(long long) --> class QDate", pybind11::arg("jd_"));
		cl.def("toJulianDay", (long long (QDate::*)() const) &QDate::toJulianDay, "C++: QDate::toJulianDay() const --> long long");
	}
}
