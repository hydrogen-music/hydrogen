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
#include <core/AudioEngine.h> // H2Core::AudioEngineLocking
#include <core/Basics/AutomationPath.h> // H2Core::AutomationPath
#include <iostream> // --trace
#include <iterator> // __gnu_cxx::__normal_iterator
#include <iterator> // std::reverse_iterator
#include <memory> // std::allocator
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

// H2Core::AutomationPath file:core/Basics/AutomationPath.h line:35
struct PyCallBack_H2Core_AutomationPath : public H2Core::AutomationPath {
	using H2Core::AutomationPath::AutomationPath;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AutomationPath *>(this), "toQString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return AutomationPath::toQString(a0, a1);
	}
};

void bind_core_AudioEngine_1(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B656_[H2Core::AudioEngineLocking] ";
	{ // H2Core::AudioEngineLocking file:core/AudioEngine.h line:1009
		pybind11::class_<H2Core::AudioEngineLocking, std::shared_ptr<H2Core::AudioEngineLocking>> cl(M("H2Core"), "AudioEngineLocking", "AudioEngineLocking\n\n This is a base class for shared data structures which may be\n modified by the AudioEngine. These should only be modified or\n trusted by a thread holding the AudioEngine lock.\n\n Any class which implements a data structure which can be modified\n by the AudioEngine can inherit from this, and use the protected\n \"assertLocked()\" method to ensure that methods are called only\n with appropriate locking.\n\n Checking is only done on debug builds.");
		cl.def( pybind11::init( [](){ return new H2Core::AudioEngineLocking(); } ) );
		cl.def( pybind11::init( [](H2Core::AudioEngineLocking const &o){ return new H2Core::AudioEngineLocking(o); } ) );
		cl.def("setNeedsLock", (void (H2Core::AudioEngineLocking::*)(bool)) &H2Core::AudioEngineLocking::setNeedsLock, "The audio processing thread can modify some PatternLists. For\n these structures, the audio engine lock must be held for any\n thread to access them.\n\nC++: H2Core::AudioEngineLocking::setNeedsLock(bool) --> void", pybind11::arg("bNeedsLock"));
		cl.def("assign", (class H2Core::AudioEngineLocking & (H2Core::AudioEngineLocking::*)(const class H2Core::AudioEngineLocking &)) &H2Core::AudioEngineLocking::operator=, "C++: H2Core::AudioEngineLocking::operator=(const class H2Core::AudioEngineLocking &) --> class H2Core::AudioEngineLocking &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B657_[H2Core::AutomationPath] ";
	{ // H2Core::AutomationPath file:core/Basics/AutomationPath.h line:35
		pybind11::class_<H2Core::AutomationPath, std::shared_ptr<H2Core::AutomationPath>, PyCallBack_H2Core_AutomationPath, H2Core::Object> cl(M("H2Core"), "AutomationPath", "");
		cl.def( pybind11::init<float, float, float>(), pybind11::arg("min"), pybind11::arg("max"), pybind11::arg("def") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_AutomationPath const &o){ return new PyCallBack_H2Core_AutomationPath(o); } ) );
		cl.def( pybind11::init( [](H2Core::AutomationPath const &o){ return new H2Core::AutomationPath(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::AutomationPath::class_name, "C++: H2Core::AutomationPath::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("empty", (bool (H2Core::AutomationPath::*)() const) &H2Core::AutomationPath::empty, "C++: H2Core::AutomationPath::empty() const --> bool");
		cl.def("get_min", (float (H2Core::AutomationPath::*)() const) &H2Core::AutomationPath::get_min, "C++: H2Core::AutomationPath::get_min() const --> float");
		cl.def("get_max", (float (H2Core::AutomationPath::*)() const) &H2Core::AutomationPath::get_max, "C++: H2Core::AutomationPath::get_max() const --> float");
		cl.def("get_default", (float (H2Core::AutomationPath::*)() const) &H2Core::AutomationPath::get_default, "C++: H2Core::AutomationPath::get_default() const --> float");
		cl.def("get_value", (float (H2Core::AutomationPath::*)(float) const) &H2Core::AutomationPath::get_value, "C++: H2Core::AutomationPath::get_value(float) const --> float", pybind11::arg("x"));
		cl.def("add_point", (void (H2Core::AutomationPath::*)(float, float)) &H2Core::AutomationPath::add_point, "C++: H2Core::AutomationPath::add_point(float, float) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("remove_point", (void (H2Core::AutomationPath::*)(float)) &H2Core::AutomationPath::remove_point, "C++: H2Core::AutomationPath::remove_point(float) --> void", pybind11::arg("x"));
		cl.def("toQString", [](H2Core::AutomationPath const &o, const class QString & a0) -> QString { return o.toQString(a0); }, "", pybind11::arg("sPrefix"));
		cl.def("toQString", (class QString (H2Core::AutomationPath::*)(const class QString &, bool) const) &H2Core::AutomationPath::toQString, "Formatted string version for debugging purposes.\n \n\n String prefix which will be added in front of\n every new line\n \n\n Instead of the whole content of all classes\n stored as members just a single unique identifier will be\n displayed without line breaks.\n\n \n String presentation of current object.\n\nC++: H2Core::AutomationPath::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
		cl.def("assign", (class H2Core::AutomationPath & (H2Core::AutomationPath::*)(const class H2Core::AutomationPath &)) &H2Core::AutomationPath::operator=, "C++: H2Core::AutomationPath::operator=(const class H2Core::AutomationPath &) --> class H2Core::AutomationPath &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
