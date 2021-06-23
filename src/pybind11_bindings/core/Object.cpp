#include <QtCore/qbytearray.h> // 
#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qbytearray.h> // QByteArrayDataPtr
#include <QtCore/qbytearray.h> // QByteRef
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qchar.h> // QLatin1Char
#include <QtCore/qflags.h> // QFlag
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qlist.h> // QList
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
#include <QtCore/qregexp.h> // 
#include <QtCore/qregexp.h> // QRegExp
#include <QtCore/qregularexpression.h> // 
#include <QtCore/qregularexpression.h> // QRegularExpression
#include <QtCore/qregularexpression.h> // QRegularExpressionMatch
#include <QtCore/qregularexpression.h> // QRegularExpressionMatchIterator
#include <QtCore/qstring.h> // 
#include <QtCore/qstring.h> // QCharRef
#include <QtCore/qstring.h> // QLatin1String
#include <QtCore/qstring.h> // QString
#include <QtCore/qstring.h> // QStringRef
#include <QtCore/qstringlist.h> // QStringList
#include <QtCore/qstringliteral.h> // QStringDataPtr
#include <QtCore/qstringview.h> // QStringView
#include <QtCore/qvector.h> // QVector
#include <core/Object.h> // H2Core::Object
#include <core/Object.h> // H2Core::operator<<
#include <ios> // std::_Ios_Seekdir
#include <iostream> // --trace
#include <iterator> // __gnu_cxx::__normal_iterator
#include <iterator> // std::reverse_iterator
#include <locale> // std::locale
#include <memory> // std::allocator
#include <ostream> // std::basic_ostream
#include <sstream> // __str__
#include <streambuf> // std::basic_streambuf
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

// H2Core::Object file:core/Object.h line:40
struct PyCallBack_H2Core_Object : public H2Core::Object {
	using H2Core::Object::Object;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::Object *>(this), "toQString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return Object::toQString(a0, a1);
	}
};

void bind_core_Object(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B639_[H2Core::Object] ";
	{ // H2Core::Object file:core/Object.h line:40
		pybind11::class_<H2Core::Object, std::shared_ptr<H2Core::Object>, PyCallBack_H2Core_Object> cl(M("H2Core"), "Object", "Base class.");
		cl.def( pybind11::init( [](){ return new H2Core::Object(); }, [](){ return new PyCallBack_H2Core_Object(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_Object const &o){ return new PyCallBack_H2Core_Object(o); } ) );
		cl.def( pybind11::init( [](H2Core::Object const &o){ return new H2Core::Object(o); } ) );
		cl.def( pybind11::init<const char *>(), pybind11::arg("class_name") );

		cl.def("class_name", (const char * (H2Core::Object::*)() const) &H2Core::Object::class_name, "C++: H2Core::Object::class_name() const --> const char *", pybind11::return_value_policy::automatic);
		cl.def_static("set_count", (void (*)(bool)) &H2Core::Object::set_count, "enable/disable class instances counting\n \n\n the counting status to set\n\nC++: H2Core::Object::set_count(bool) --> void", pybind11::arg("flag"));
		cl.def_static("count_active", (bool (*)()) &H2Core::Object::count_active, "C++: H2Core::Object::count_active() --> bool");
		cl.def_static("objects_count", (int (*)()) &H2Core::Object::objects_count, "C++: H2Core::Object::objects_count() --> int");
		cl.def_static("write_objects_map_to_cerr", (void (*)()) &H2Core::Object::write_objects_map_to_cerr, "C++: H2Core::Object::write_objects_map_to_cerr() --> void");
		cl.def_static("bootstrap", [](class H2Core::Logger * a0) -> int { return H2Core::Object::bootstrap(a0); }, "", pybind11::arg("logger"));
		cl.def_static("bootstrap", (int (*)(class H2Core::Logger *, bool)) &H2Core::Object::bootstrap, "must be called before any Object instantiation !\n \n\n the logger instance used to send messages to\n \n\n should we count objects instances or not\n\nC++: H2Core::Object::bootstrap(class H2Core::Logger *, bool) --> int", pybind11::arg("logger"), pybind11::arg("count"));
		cl.def_static("logger", (class H2Core::Logger * (*)()) &H2Core::Object::logger, "C++: H2Core::Object::logger() --> class H2Core::Logger *", pybind11::return_value_policy::automatic);
		cl.def_static("getAliveObjectCount", (int (*)()) &H2Core::Object::getAliveObjectCount, "Total numbers of objects being alive. \n\nC++: H2Core::Object::getAliveObjectCount() --> int");
		cl.def("toQString", [](H2Core::Object const &o, const class QString & a0) -> QString { return o.toQString(a0); }, "", pybind11::arg("sPrefix"));
		cl.def("toQString", (class QString (H2Core::Object::*)(const class QString &, bool) const) &H2Core::Object::toQString, "Formatted string version for debugging purposes.\n \n\n String prefix which will be added in front of\n every new line\n \n\n Instead of the whole content of all classes\n stored as members just a single unique identifier will be\n displayed without line breaks.\n\n \n String presentation of current object.\n\nC++: H2Core::Object::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
		cl.def("Print", [](H2Core::Object const &o) -> void { return o.Print(); }, "");
		cl.def("Print", (void (H2Core::Object::*)(bool) const) &H2Core::Object::Print, "Prints content of toQString() via DEBUGLOG\n\n \n Whether to display the content of the member\n class variables and to use line breaks.\n\nC++: H2Core::Object::Print(bool) const --> void", pybind11::arg("bShort"));
		cl.def("assign", (class H2Core::Object & (H2Core::Object::*)(const class H2Core::Object &)) &H2Core::Object::operator=, "C++: H2Core::Object::operator=(const class H2Core::Object &) --> class H2Core::Object &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		cl.def("__str__", [](H2Core::Object const &o) -> std::string { std::ostringstream s; s << o; return s.str(); } );
	}
}
