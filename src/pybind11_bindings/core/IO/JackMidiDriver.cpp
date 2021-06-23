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
#include <core/Basics/Adsr.h> // H2Core::ADSR
#include <core/Basics/Instrument.h> // H2Core::Instrument
#include <core/Basics/InstrumentList.h> // H2Core::InstrumentList
#include <core/Basics/Note.h> // 
#include <core/Basics/Note.h> // H2Core::Note
#include <core/Basics/Note.h> // H2Core::SelectedLayerInfo
#include <core/Helpers/Xml.h> // H2Core::XMLNode
#include <core/IO/JackMidiDriver.h> // H2Core::JackMidiDriver
#include <core/Object.h> // H2Core::Object
#include <iostream> // --trace
#include <iterator> // __gnu_cxx::__normal_iterator
#include <iterator> // std::reverse_iterator
#include <memory> // std::allocator
#include <memory> // std::shared_ptr
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

// H2Core::JackMidiDriver file:core/IO/JackMidiDriver.h line:49
struct PyCallBack_H2Core_JackMidiDriver : public H2Core::JackMidiDriver {
	using H2Core::JackMidiDriver::JackMidiDriver;

	void open() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackMidiDriver *>(this), "open");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return JackMidiDriver::open();
	}
	void close() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackMidiDriver *>(this), "close");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return JackMidiDriver::close();
	}
	void handleQueueNote(class H2Core::Note * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackMidiDriver *>(this), "handleQueueNote");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return JackMidiDriver::handleQueueNote(a0);
	}
	void handleQueueNoteOff(int a0, int a1, int a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackMidiDriver *>(this), "handleQueueNoteOff");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return JackMidiDriver::handleQueueNoteOff(a0, a1, a2);
	}
	void handleQueueAllNoteOff() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackMidiDriver *>(this), "handleQueueAllNoteOff");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return JackMidiDriver::handleQueueAllNoteOff();
	}
	void handleOutgoingControlChange(int a0, int a1, int a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackMidiDriver *>(this), "handleOutgoingControlChange");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return JackMidiDriver::handleOutgoingControlChange(a0, a1, a2);
	}
	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackMidiDriver *>(this), "toQString");
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

void bind_core_IO_JackMidiDriver(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B715_[H2Core::JackMidiDriver] ";
	{ // H2Core::JackMidiDriver file:core/IO/JackMidiDriver.h line:49
		pybind11::class_<H2Core::JackMidiDriver, std::shared_ptr<H2Core::JackMidiDriver>, PyCallBack_H2Core_JackMidiDriver, H2Core::MidiInput, H2Core::MidiOutput> cl(M("H2Core"), "JackMidiDriver", "");
		cl.def( pybind11::init( [](){ return new H2Core::JackMidiDriver(); }, [](){ return new PyCallBack_H2Core_JackMidiDriver(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_JackMidiDriver const &o){ return new PyCallBack_H2Core_JackMidiDriver(o); } ) );
		cl.def( pybind11::init( [](H2Core::JackMidiDriver const &o){ return new H2Core::JackMidiDriver(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::JackMidiDriver::class_name, "C++: H2Core::JackMidiDriver::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("open", (void (H2Core::JackMidiDriver::*)()) &H2Core::JackMidiDriver::open, "C++: H2Core::JackMidiDriver::open() --> void");
		cl.def("close", (void (H2Core::JackMidiDriver::*)()) &H2Core::JackMidiDriver::close, "C++: H2Core::JackMidiDriver::close() --> void");
		cl.def("getPortInfo", (void (H2Core::JackMidiDriver::*)(const class QString &, int &, int &)) &H2Core::JackMidiDriver::getPortInfo, "C++: H2Core::JackMidiDriver::getPortInfo(const class QString &, int &, int &) --> void", pybind11::arg("sPortName"), pybind11::arg("nClient"), pybind11::arg("nPort"));
		cl.def("JackMidiWrite", (void (H2Core::JackMidiDriver::*)(unsigned int)) &H2Core::JackMidiDriver::JackMidiWrite, "C++: H2Core::JackMidiDriver::JackMidiWrite(unsigned int) --> void", pybind11::arg("nframes"));
		cl.def("JackMidiRead", (void (H2Core::JackMidiDriver::*)(unsigned int)) &H2Core::JackMidiDriver::JackMidiRead, "C++: H2Core::JackMidiDriver::JackMidiRead(unsigned int) --> void", pybind11::arg("nframes"));
		cl.def("handleQueueNote", (void (H2Core::JackMidiDriver::*)(class H2Core::Note *)) &H2Core::JackMidiDriver::handleQueueNote, "C++: H2Core::JackMidiDriver::handleQueueNote(class H2Core::Note *) --> void", pybind11::arg("pNote"));
		cl.def("handleQueueNoteOff", (void (H2Core::JackMidiDriver::*)(int, int, int)) &H2Core::JackMidiDriver::handleQueueNoteOff, "C++: H2Core::JackMidiDriver::handleQueueNoteOff(int, int, int) --> void", pybind11::arg("channel"), pybind11::arg("key"), pybind11::arg("velocity"));
		cl.def("handleQueueAllNoteOff", (void (H2Core::JackMidiDriver::*)()) &H2Core::JackMidiDriver::handleQueueAllNoteOff, "C++: H2Core::JackMidiDriver::handleQueueAllNoteOff() --> void");
		cl.def("handleOutgoingControlChange", (void (H2Core::JackMidiDriver::*)(int, int, int)) &H2Core::JackMidiDriver::handleOutgoingControlChange, "C++: H2Core::JackMidiDriver::handleOutgoingControlChange(int, int, int) --> void", pybind11::arg("param"), pybind11::arg("value"), pybind11::arg("channel"));
		cl.def("assign", (class H2Core::JackMidiDriver & (H2Core::JackMidiDriver::*)(const class H2Core::JackMidiDriver &)) &H2Core::JackMidiDriver::operator=, "C++: H2Core::JackMidiDriver::operator=(const class H2Core::JackMidiDriver &) --> class H2Core::JackMidiDriver &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
