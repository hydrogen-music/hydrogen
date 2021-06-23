#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
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

void bind_unknown_unknown(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B110_[H2Core::Logger] ";
	{ // H2Core::Logger file: line:41
		pybind11::class_<H2Core::Logger, std::shared_ptr<H2Core::Logger>> cl(M("H2Core"), "Logger", "Class for writing logs to the console");
		cl.def( pybind11::init( [](H2Core::Logger const &o){ return new H2Core::Logger(o); } ) );

		pybind11::enum_<H2Core::Logger::log_levels>(cl, "log_levels", pybind11::arithmetic(), "possible logging bits ")
			.value("None", H2Core::Logger::None)
			.value("Error", H2Core::Logger::Error)
			.value("Warning", H2Core::Logger::Warning)
			.value("Info", H2Core::Logger::Info)
			.value("Debug", H2Core::Logger::Debug)
			.value("Constructors", H2Core::Logger::Constructors)
			.value("AELockTracing", H2Core::Logger::AELockTracing)
			.export_values();

		cl.def_static("bootstrap", (class H2Core::Logger * (*)(unsigned int)) &H2Core::Logger::bootstrap, "create the logger instance if not exists, set the log level and return the instance\n \n\n the logging level bitmask\n\nC++: H2Core::Logger::bootstrap(unsigned int) --> class H2Core::Logger *", pybind11::return_value_policy::automatic, pybind11::arg("msk"));
		cl.def_static("create_instance", (class H2Core::Logger * (*)()) &H2Core::Logger::create_instance, "If #__instance equals 0, a new H2Core::Logger\n singleton will be created and stored in it.\n\n It is called in Hydrogen::create_instance().\n\nC++: H2Core::Logger::create_instance() --> class H2Core::Logger *", pybind11::return_value_policy::automatic);
		cl.def_static("get_instance", (class H2Core::Logger * (*)()) &H2Core::Logger::get_instance, "Returns a pointer to the current H2Core::Logger\n singleton stored in #__instance.\n\nC++: H2Core::Logger::get_instance() --> class H2Core::Logger *", pybind11::return_value_policy::automatic);
		cl.def("should_log", (bool (H2Core::Logger::*)(unsigned int) const) &H2Core::Logger::should_log, "return true if the level is set in the bitmask\n \n\n the level to check\n\nC++: H2Core::Logger::should_log(unsigned int) const --> bool", pybind11::arg("lvl"));
		cl.def_static("set_bit_mask", (void (*)(unsigned int)) &H2Core::Logger::set_bit_mask, "set the bitmask\n \n\n the new bitmask to set\n\nC++: H2Core::Logger::set_bit_mask(unsigned int) --> void", pybind11::arg("msk"));
		cl.def_static("bit_mask", (unsigned int (*)()) &H2Core::Logger::bit_mask, "return the current log level bit mask \n\nC++: H2Core::Logger::bit_mask() --> unsigned int");
		cl.def("set_use_file", (void (H2Core::Logger::*)(bool)) &H2Core::Logger::set_use_file, "set use file flag\n \n\n the flag status\n\nC++: H2Core::Logger::set_use_file(bool) --> void", pybind11::arg("use"));
		cl.def("use_file", (bool (H2Core::Logger::*)() const) &H2Core::Logger::use_file, "return __use_file \n\nC++: H2Core::Logger::use_file() const --> bool");
		cl.def_static("parse_log_level", (unsigned int (*)(const char *)) &H2Core::Logger::parse_log_level, "parse a log level string and return the corresponding bit mask\n \n\n the log level string\n\nC++: H2Core::Logger::parse_log_level(const char *) --> unsigned int", pybind11::arg("lvl"));
		cl.def("log", (void (H2Core::Logger::*)(unsigned int, const class QString &, const char *, const class QString &)) &H2Core::Logger::log, "the log function\n \n\n used to output the corresponding level string\n \n\n the name of the calling class\n \n\n the name of the calling function/method\n \n\n the message to log\n\nC++: H2Core::Logger::log(unsigned int, const class QString &, const char *, const class QString &) --> void", pybind11::arg("level"), pybind11::arg("class_name"), pybind11::arg("func_name"), pybind11::arg("msg"));
		cl.def("assign", (class H2Core::Logger & (H2Core::Logger::*)(const class H2Core::Logger &)) &H2Core::Logger::operator=, "C++: H2Core::Logger::operator=(const class H2Core::Logger &) --> class H2Core::Logger &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B111_[void * H2Core::loggerThread_func(void *)] ";
}
