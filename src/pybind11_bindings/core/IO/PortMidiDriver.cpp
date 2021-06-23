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
#include <core/IO/PortMidiDriver.h> // H2Core::PortMidiDriver
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

// H2Core::PortMidiDriver file:core/IO/PortMidiDriver.h line:35
struct PyCallBack_H2Core_PortMidiDriver : public H2Core::PortMidiDriver {
	using H2Core::PortMidiDriver::PortMidiDriver;

	void open() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::PortMidiDriver *>(this), "open");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return PortMidiDriver::open();
	}
	void close() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::PortMidiDriver *>(this), "close");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return PortMidiDriver::close();
	}
	void handleQueueNote(class H2Core::Note * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::PortMidiDriver *>(this), "handleQueueNote");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return PortMidiDriver::handleQueueNote(a0);
	}
	void handleQueueNoteOff(int a0, int a1, int a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::PortMidiDriver *>(this), "handleQueueNoteOff");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return PortMidiDriver::handleQueueNoteOff(a0, a1, a2);
	}
	void handleQueueAllNoteOff() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::PortMidiDriver *>(this), "handleQueueAllNoteOff");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return PortMidiDriver::handleQueueAllNoteOff();
	}
	void handleOutgoingControlChange(int a0, int a1, int a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::PortMidiDriver *>(this), "handleOutgoingControlChange");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return PortMidiDriver::handleOutgoingControlChange(a0, a1, a2);
	}
	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::PortMidiDriver *>(this), "toQString");
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

void bind_core_IO_PortMidiDriver(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B717_[H2Core::PortMidiDriver] ";
	{ // H2Core::PortMidiDriver file:core/IO/PortMidiDriver.h line:35
		pybind11::class_<H2Core::PortMidiDriver, std::shared_ptr<H2Core::PortMidiDriver>, PyCallBack_H2Core_PortMidiDriver, H2Core::MidiInput, H2Core::MidiOutput> cl(M("H2Core"), "PortMidiDriver", "");
		cl.def( pybind11::init( [](){ return new H2Core::PortMidiDriver(); }, [](){ return new PyCallBack_H2Core_PortMidiDriver(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_PortMidiDriver const &o){ return new PyCallBack_H2Core_PortMidiDriver(o); } ) );
		cl.def( pybind11::init( [](H2Core::PortMidiDriver const &o){ return new H2Core::PortMidiDriver(o); } ) );
		cl.def_readwrite("m_bRunning", &H2Core::PortMidiDriver::m_bRunning);
		cl.def_static("class_name", (const char * (*)()) &H2Core::PortMidiDriver::class_name, "C++: H2Core::PortMidiDriver::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("open", (void (H2Core::PortMidiDriver::*)()) &H2Core::PortMidiDriver::open, "C++: H2Core::PortMidiDriver::open() --> void");
		cl.def("close", (void (H2Core::PortMidiDriver::*)()) &H2Core::PortMidiDriver::close, "C++: H2Core::PortMidiDriver::close() --> void");
		cl.def("handleQueueNote", (void (H2Core::PortMidiDriver::*)(class H2Core::Note *)) &H2Core::PortMidiDriver::handleQueueNote, "C++: H2Core::PortMidiDriver::handleQueueNote(class H2Core::Note *) --> void", pybind11::arg("pNote"));
		cl.def("handleQueueNoteOff", (void (H2Core::PortMidiDriver::*)(int, int, int)) &H2Core::PortMidiDriver::handleQueueNoteOff, "C++: H2Core::PortMidiDriver::handleQueueNoteOff(int, int, int) --> void", pybind11::arg("channel"), pybind11::arg("key"), pybind11::arg("velocity"));
		cl.def("handleQueueAllNoteOff", (void (H2Core::PortMidiDriver::*)()) &H2Core::PortMidiDriver::handleQueueAllNoteOff, "C++: H2Core::PortMidiDriver::handleQueueAllNoteOff() --> void");
		cl.def("handleOutgoingControlChange", (void (H2Core::PortMidiDriver::*)(int, int, int)) &H2Core::PortMidiDriver::handleOutgoingControlChange, "C++: H2Core::PortMidiDriver::handleOutgoingControlChange(int, int, int) --> void", pybind11::arg("param"), pybind11::arg("value"), pybind11::arg("channel"));
		cl.def("assign", (class H2Core::PortMidiDriver & (H2Core::PortMidiDriver::*)(const class H2Core::PortMidiDriver &)) &H2Core::PortMidiDriver::operator=, "C++: H2Core::PortMidiDriver::operator=(const class H2Core::PortMidiDriver &) --> class H2Core::PortMidiDriver &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
