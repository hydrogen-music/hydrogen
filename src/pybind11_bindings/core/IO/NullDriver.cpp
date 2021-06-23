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
#include <core/IO/NullDriver.h> // H2Core::NullDriver
#include <core/Object.h> // H2Core::Object
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

// H2Core::NullDriver file:core/IO/NullDriver.h line:36
struct PyCallBack_H2Core_NullDriver : public H2Core::NullDriver {
	using H2Core::NullDriver::NullDriver;

	int init(unsigned int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::NullDriver *>(this), "init");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return NullDriver::init(a0);
	}
	int connect() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::NullDriver *>(this), "connect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return NullDriver::connect();
	}
	void disconnect() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::NullDriver *>(this), "disconnect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return NullDriver::disconnect();
	}
	unsigned int getBufferSize() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::NullDriver *>(this), "getBufferSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned int>::value) {
				static pybind11::detail::override_caster_t<unsigned int> caster;
				return pybind11::detail::cast_ref<unsigned int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned int>(std::move(o));
		}
		return NullDriver::getBufferSize();
	}
	unsigned int getSampleRate() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::NullDriver *>(this), "getSampleRate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned int>::value) {
				static pybind11::detail::override_caster_t<unsigned int> caster;
				return pybind11::detail::cast_ref<unsigned int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned int>(std::move(o));
		}
		return NullDriver::getSampleRate();
	}
	float * getOut_L() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::NullDriver *>(this), "getOut_L");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<float *>::value) {
				static pybind11::detail::override_caster_t<float *> caster;
				return pybind11::detail::cast_ref<float *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<float *>(std::move(o));
		}
		return NullDriver::getOut_L();
	}
	float * getOut_R() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::NullDriver *>(this), "getOut_R");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<float *>::value) {
				static pybind11::detail::override_caster_t<float *> caster;
				return pybind11::detail::cast_ref<float *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<float *>(std::move(o));
		}
		return NullDriver::getOut_R();
	}
	void play() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::NullDriver *>(this), "play");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return NullDriver::play();
	}
	void stop() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::NullDriver *>(this), "stop");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return NullDriver::stop();
	}
	void locate(unsigned long a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::NullDriver *>(this), "locate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return NullDriver::locate(a0);
	}
	void updateTransportInfo() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::NullDriver *>(this), "updateTransportInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return NullDriver::updateTransportInfo();
	}
	void setBpm(float a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::NullDriver *>(this), "setBpm");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return NullDriver::setBpm(a0);
	}
	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::NullDriver *>(this), "toQString");
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

void bind_core_IO_NullDriver(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B708_[H2Core::NullDriver] ";
	{ // H2Core::NullDriver file:core/IO/NullDriver.h line:36
		pybind11::class_<H2Core::NullDriver, std::shared_ptr<H2Core::NullDriver>, PyCallBack_H2Core_NullDriver, H2Core::AudioOutput> cl(M("H2Core"), "NullDriver", "");
		cl.def( pybind11::init( [](PyCallBack_H2Core_NullDriver const &o){ return new PyCallBack_H2Core_NullDriver(o); } ) );
		cl.def( pybind11::init( [](H2Core::NullDriver const &o){ return new H2Core::NullDriver(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::NullDriver::class_name, "C++: H2Core::NullDriver::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("init", (int (H2Core::NullDriver::*)(unsigned int)) &H2Core::NullDriver::init, "C++: H2Core::NullDriver::init(unsigned int) --> int", pybind11::arg("nBufferSize"));
		cl.def("connect", (int (H2Core::NullDriver::*)()) &H2Core::NullDriver::connect, "C++: H2Core::NullDriver::connect() --> int");
		cl.def("disconnect", (void (H2Core::NullDriver::*)()) &H2Core::NullDriver::disconnect, "C++: H2Core::NullDriver::disconnect() --> void");
		cl.def("getBufferSize", (unsigned int (H2Core::NullDriver::*)()) &H2Core::NullDriver::getBufferSize, "C++: H2Core::NullDriver::getBufferSize() --> unsigned int");
		cl.def("getSampleRate", (unsigned int (H2Core::NullDriver::*)()) &H2Core::NullDriver::getSampleRate, "C++: H2Core::NullDriver::getSampleRate() --> unsigned int");
		cl.def("getOut_L", (float * (H2Core::NullDriver::*)()) &H2Core::NullDriver::getOut_L, "C++: H2Core::NullDriver::getOut_L() --> float *", pybind11::return_value_policy::automatic);
		cl.def("getOut_R", (float * (H2Core::NullDriver::*)()) &H2Core::NullDriver::getOut_R, "C++: H2Core::NullDriver::getOut_R() --> float *", pybind11::return_value_policy::automatic);
		cl.def("play", (void (H2Core::NullDriver::*)()) &H2Core::NullDriver::play, "C++: H2Core::NullDriver::play() --> void");
		cl.def("stop", (void (H2Core::NullDriver::*)()) &H2Core::NullDriver::stop, "C++: H2Core::NullDriver::stop() --> void");
		cl.def("locate", (void (H2Core::NullDriver::*)(unsigned long)) &H2Core::NullDriver::locate, "C++: H2Core::NullDriver::locate(unsigned long) --> void", pybind11::arg("nFrame"));
		cl.def("updateTransportInfo", (void (H2Core::NullDriver::*)()) &H2Core::NullDriver::updateTransportInfo, "C++: H2Core::NullDriver::updateTransportInfo() --> void");
		cl.def("setBpm", (void (H2Core::NullDriver::*)(float)) &H2Core::NullDriver::setBpm, "C++: H2Core::NullDriver::setBpm(float) --> void", pybind11::arg("fBPM"));
		cl.def("assign", (class H2Core::NullDriver & (H2Core::NullDriver::*)(const class H2Core::NullDriver &)) &H2Core::NullDriver::operator=, "C++: H2Core::NullDriver::operator=(const class H2Core::NullDriver &) --> class H2Core::NullDriver &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
