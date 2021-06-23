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
#include <core/IO/AudioOutput.h> // H2Core::AudioOutput
#include <core/IO/TransportInfo.h> // H2Core::TransportInfo
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

// H2Core::Note file:core/Basics/Note.h line:68
struct PyCallBack_H2Core_Note : public H2Core::Note {
	using H2Core::Note::Note;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::Note *>(this), "toQString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return Note::toQString(a0, a1);
	}
};

// H2Core::TransportInfo file:core/IO/TransportInfo.h line:35
struct PyCallBack_H2Core_TransportInfo : public H2Core::TransportInfo {
	using H2Core::TransportInfo::TransportInfo;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::TransportInfo *>(this), "toQString");
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

// H2Core::AudioOutput file:core/IO/AudioOutput.h line:38
struct PyCallBack_H2Core_AudioOutput : public H2Core::AudioOutput {
	using H2Core::AudioOutput::AudioOutput;

	int init(unsigned int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AudioOutput *>(this), "init");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"AudioOutput::init\"");
	}
	int connect() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AudioOutput *>(this), "connect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"AudioOutput::connect\"");
	}
	void disconnect() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AudioOutput *>(this), "disconnect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"AudioOutput::disconnect\"");
	}
	unsigned int getBufferSize() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AudioOutput *>(this), "getBufferSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned int>::value) {
				static pybind11::detail::override_caster_t<unsigned int> caster;
				return pybind11::detail::cast_ref<unsigned int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned int>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"AudioOutput::getBufferSize\"");
	}
	unsigned int getSampleRate() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AudioOutput *>(this), "getSampleRate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned int>::value) {
				static pybind11::detail::override_caster_t<unsigned int> caster;
				return pybind11::detail::cast_ref<unsigned int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned int>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"AudioOutput::getSampleRate\"");
	}
	float * getOut_L() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AudioOutput *>(this), "getOut_L");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<float *>::value) {
				static pybind11::detail::override_caster_t<float *> caster;
				return pybind11::detail::cast_ref<float *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<float *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"AudioOutput::getOut_L\"");
	}
	float * getOut_R() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AudioOutput *>(this), "getOut_R");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<float *>::value) {
				static pybind11::detail::override_caster_t<float *> caster;
				return pybind11::detail::cast_ref<float *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<float *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"AudioOutput::getOut_R\"");
	}
	void updateTransportInfo() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AudioOutput *>(this), "updateTransportInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"AudioOutput::updateTransportInfo\"");
	}
	void play() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AudioOutput *>(this), "play");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"AudioOutput::play\"");
	}
	void stop() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AudioOutput *>(this), "stop");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"AudioOutput::stop\"");
	}
	void locate(unsigned long a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AudioOutput *>(this), "locate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"AudioOutput::locate\"");
	}
	void setBpm(float a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AudioOutput *>(this), "setBpm");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"AudioOutput::setBpm\"");
	}
	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AudioOutput *>(this), "toQString");
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

void bind_core_Basics_Note(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B651_[H2Core::SelectedLayerInfo] ";
	{ // H2Core::SelectedLayerInfo file:core/Basics/Note.h line:60
		pybind11::class_<H2Core::SelectedLayerInfo, std::shared_ptr<H2Core::SelectedLayerInfo>> cl(M("H2Core"), "SelectedLayerInfo", "");
		cl.def( pybind11::init( [](){ return new H2Core::SelectedLayerInfo(); } ) );
		cl.def_readwrite("SelectedLayer", &H2Core::SelectedLayerInfo::SelectedLayer);
		cl.def_readwrite("SamplePosition", &H2Core::SelectedLayerInfo::SamplePosition);
	}
	std::cout << "B652_[H2Core::Note] ";
	{ // H2Core::Note file:core/Basics/Note.h line:68
		pybind11::class_<H2Core::Note, std::shared_ptr<H2Core::Note>, PyCallBack_H2Core_Note, H2Core::Object> cl(M("H2Core"), "Note", "A note plays an associated instrument with a velocity left and right pan");
		cl.def( pybind11::init( [](PyCallBack_H2Core_Note const &o){ return new PyCallBack_H2Core_Note(o); } ) );
		cl.def( pybind11::init( [](H2Core::Note const &o){ return new H2Core::Note(o); } ) );

		pybind11::enum_<H2Core::Note::Key>(cl, "Key", pybind11::arithmetic(), "possible keys ")
			.value("C", H2Core::Note::C)
			.value("Cs", H2Core::Note::Cs)
			.value("D", H2Core::Note::D)
			.value("Ef", H2Core::Note::Ef)
			.value("E", H2Core::Note::E)
			.value("F", H2Core::Note::F)
			.value("Fs", H2Core::Note::Fs)
			.value("G", H2Core::Note::G)
			.value("Af", H2Core::Note::Af)
			.value("A", H2Core::Note::A)
			.value("Bf", H2Core::Note::Bf)
			.value("B", H2Core::Note::B)
			.export_values();


		pybind11::enum_<H2Core::Note::Octave>(cl, "Octave", pybind11::arithmetic(), "possible octaves ")
			.value("P8Z", H2Core::Note::P8Z)
			.value("P8Y", H2Core::Note::P8Y)
			.value("P8X", H2Core::Note::P8X)
			.value("P8", H2Core::Note::P8)
			.value("P8A", H2Core::Note::P8A)
			.value("P8B", H2Core::Note::P8B)
			.value("P8C", H2Core::Note::P8C)
			.export_values();

		cl.def_static("class_name", (const char * (*)()) &H2Core::Note::class_name, "C++: H2Core::Note::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("save_to", (void (H2Core::Note::*)(class H2Core::XMLNode *)) &H2Core::Note::save_to, "C++: H2Core::Note::save_to(class H2Core::XMLNode *) --> void", pybind11::arg("node"));
		cl.def_static("load_from", (class H2Core::Note * (*)(class H2Core::XMLNode *, class H2Core::InstrumentList *)) &H2Core::Note::load_from, "load a note from an XMLNode\n \n\n the XMLDode to read from\n \n\n the current instrument list to search instrument into\n \n\n a new Note instance\n\nC++: H2Core::Note::load_from(class H2Core::XMLNode *, class H2Core::InstrumentList *) --> class H2Core::Note *", pybind11::return_value_policy::automatic, pybind11::arg("node"), pybind11::arg("instruments"));
		cl.def("dump", (void (H2Core::Note::*)()) &H2Core::Note::dump, "output details through logger with DEBUG severity \n\nC++: H2Core::Note::dump() --> void");
		cl.def("map_instrument", (void (H2Core::Note::*)(class H2Core::InstrumentList *)) &H2Core::Note::map_instrument, "find the corresponding instrument and point to it, or an empty instrument\n \n\n the list of instrument to look into\n\nC++: H2Core::Note::map_instrument(class H2Core::InstrumentList *) --> void", pybind11::arg("instruments"));
		cl.def("has_instrument", (bool (H2Core::Note::*)() const) &H2Core::Note::has_instrument, "return true if #__instrument is set \n\nC++: H2Core::Note::has_instrument() const --> bool");
		cl.def("set_instrument_id", (void (H2Core::Note::*)(int)) &H2Core::Note::set_instrument_id, "#__instrument_id setter\n \n\n the new value\n\nC++: H2Core::Note::set_instrument_id(int) --> void", pybind11::arg("value"));
		cl.def("get_instrument_id", (int (H2Core::Note::*)() const) &H2Core::Note::get_instrument_id, "#__instrument_id accessor \n\nC++: H2Core::Note::get_instrument_id() const --> int");
		cl.def("set_specific_compo_id", (void (H2Core::Note::*)(int)) &H2Core::Note::set_specific_compo_id, "#__specific_compo_id setter\n \n\n the new value\n\nC++: H2Core::Note::set_specific_compo_id(int) --> void", pybind11::arg("value"));
		cl.def("get_specific_compo_id", (int (H2Core::Note::*)() const) &H2Core::Note::get_specific_compo_id, "#__specific_compo_id accessor \n\nC++: H2Core::Note::get_specific_compo_id() const --> int");
		cl.def("set_position", (void (H2Core::Note::*)(int)) &H2Core::Note::set_position, "#__position setter\n \n\n the new value\n\nC++: H2Core::Note::set_position(int) --> void", pybind11::arg("value"));
		cl.def("get_position", (int (H2Core::Note::*)() const) &H2Core::Note::get_position, "#__position accessor \n\nC++: H2Core::Note::get_position() const --> int");
		cl.def("set_velocity", (void (H2Core::Note::*)(float)) &H2Core::Note::set_velocity, "#__velocity setter\n \n\n the new value\n\nC++: H2Core::Note::set_velocity(float) --> void", pybind11::arg("value"));
		cl.def("get_velocity", (float (H2Core::Note::*)() const) &H2Core::Note::get_velocity, "#__velocity accessor \n\nC++: H2Core::Note::get_velocity() const --> float");
		cl.def("setPan", (void (H2Core::Note::*)(float)) &H2Core::Note::setPan, "set pan of the note. assumes the input range in [-1;1]\n\nC++: H2Core::Note::setPan(float) --> void", pybind11::arg("val"));
		cl.def("setPanWithRangeFrom0To1", (void (H2Core::Note::*)(float)) &H2Core::Note::setPanWithRangeFrom0To1, "set pan of the note, assuming the input range in [0;1] \n\nC++: H2Core::Note::setPanWithRangeFrom0To1(float) --> void", pybind11::arg("fVal"));
		cl.def("getPan", (float (H2Core::Note::*)() const) &H2Core::Note::getPan, "get pan of the note. Output pan range: [-1;1] \n\nC++: H2Core::Note::getPan() const --> float");
		cl.def("getPanWithRangeFrom0To1", (float (H2Core::Note::*)() const) &H2Core::Note::getPanWithRangeFrom0To1, "get pan of the note, scaling and translating the range from [-1;1] to [0;1] \n\nC++: H2Core::Note::getPanWithRangeFrom0To1() const --> float");
		cl.def("set_lead_lag", (void (H2Core::Note::*)(float)) &H2Core::Note::set_lead_lag, "#__lead_lag setter\n \n\n the new value\n\nC++: H2Core::Note::set_lead_lag(float) --> void", pybind11::arg("value"));
		cl.def("get_lead_lag", (float (H2Core::Note::*)() const) &H2Core::Note::get_lead_lag, "#__lead_lag accessor \n\nC++: H2Core::Note::get_lead_lag() const --> float");
		cl.def("set_length", (void (H2Core::Note::*)(int)) &H2Core::Note::set_length, "#__length setter\n \n\n the new value\n\nC++: H2Core::Note::set_length(int) --> void", pybind11::arg("value"));
		cl.def("get_length", (int (H2Core::Note::*)() const) &H2Core::Note::get_length, "#__length accessor \n\nC++: H2Core::Note::get_length() const --> int");
		cl.def("set_pitch", (void (H2Core::Note::*)(float)) &H2Core::Note::set_pitch, "#__pitch setter\n \n\n the new value\n\nC++: H2Core::Note::set_pitch(float) --> void", pybind11::arg("value"));
		cl.def("get_pitch", (float (H2Core::Note::*)() const) &H2Core::Note::get_pitch, "#__pitch accessor \n\nC++: H2Core::Note::get_pitch() const --> float");
		cl.def("set_note_off", (void (H2Core::Note::*)(bool)) &H2Core::Note::set_note_off, "#__note_off setter\n \n\n the new value\n\nC++: H2Core::Note::set_note_off(bool) --> void", pybind11::arg("value"));
		cl.def("get_note_off", (bool (H2Core::Note::*)() const) &H2Core::Note::get_note_off, "#__note_off accessor \n\nC++: H2Core::Note::get_note_off() const --> bool");
		cl.def("get_midi_msg", (int (H2Core::Note::*)() const) &H2Core::Note::get_midi_msg, "#__midi_msg accessor \n\nC++: H2Core::Note::get_midi_msg() const --> int");
		cl.def("set_pattern_idx", (void (H2Core::Note::*)(int)) &H2Core::Note::set_pattern_idx, "#__pattern_idx setter\n \n\n the new value\n\nC++: H2Core::Note::set_pattern_idx(int) --> void", pybind11::arg("value"));
		cl.def("get_pattern_idx", (int (H2Core::Note::*)() const) &H2Core::Note::get_pattern_idx, "#__pattern_idx accessor \n\nC++: H2Core::Note::get_pattern_idx() const --> int");
		cl.def("set_just_recorded", (void (H2Core::Note::*)(bool)) &H2Core::Note::set_just_recorded, "#__just_recorded setter\n \n\n the new value\n\nC++: H2Core::Note::set_just_recorded(bool) --> void", pybind11::arg("value"));
		cl.def("get_just_recorded", (bool (H2Core::Note::*)() const) &H2Core::Note::get_just_recorded, "#__just_recorded accessor \n\nC++: H2Core::Note::get_just_recorded() const --> bool");
		cl.def("get_layer_selected", (struct H2Core::SelectedLayerInfo * (H2Core::Note::*)(int)) &H2Core::Note::get_layer_selected, "C++: H2Core::Note::get_layer_selected(int) --> struct H2Core::SelectedLayerInfo *", pybind11::return_value_policy::automatic, pybind11::arg("CompoID"));
		cl.def("set_probability", (void (H2Core::Note::*)(float)) &H2Core::Note::set_probability, "C++: H2Core::Note::set_probability(float) --> void", pybind11::arg("value"));
		cl.def("get_probability", (float (H2Core::Note::*)() const) &H2Core::Note::get_probability, "C++: H2Core::Note::get_probability() const --> float");
		cl.def("set_humanize_delay", (void (H2Core::Note::*)(int)) &H2Core::Note::set_humanize_delay, "#__humanize_delay setter\n \n\n the new value\n\nC++: H2Core::Note::set_humanize_delay(int) --> void", pybind11::arg("value"));
		cl.def("get_humanize_delay", (int (H2Core::Note::*)() const) &H2Core::Note::get_humanize_delay, "#__humanize_delay accessor \n\nC++: H2Core::Note::get_humanize_delay() const --> int");
		cl.def("get_cut_off", (float (H2Core::Note::*)() const) &H2Core::Note::get_cut_off, "#__cut_off accessor \n\nC++: H2Core::Note::get_cut_off() const --> float");
		cl.def("get_resonance", (float (H2Core::Note::*)() const) &H2Core::Note::get_resonance, "#__resonance accessor \n\nC++: H2Core::Note::get_resonance() const --> float");
		cl.def("get_bpfb_l", (float (H2Core::Note::*)() const) &H2Core::Note::get_bpfb_l, "#__bpfb_l accessor \n\nC++: H2Core::Note::get_bpfb_l() const --> float");
		cl.def("get_bpfb_r", (float (H2Core::Note::*)() const) &H2Core::Note::get_bpfb_r, "#__bpfb_r accessor \n\nC++: H2Core::Note::get_bpfb_r() const --> float");
		cl.def("get_lpfb_l", (float (H2Core::Note::*)() const) &H2Core::Note::get_lpfb_l, "#__lpfb_l accessor \n\nC++: H2Core::Note::get_lpfb_l() const --> float");
		cl.def("get_lpfb_r", (float (H2Core::Note::*)() const) &H2Core::Note::get_lpfb_r, "#__lpfb_r accessor \n\nC++: H2Core::Note::get_lpfb_r() const --> float");
		cl.def("get_key", (enum H2Core::Note::Key (H2Core::Note::*)()) &H2Core::Note::get_key, "#__key accessor \n\nC++: H2Core::Note::get_key() --> enum H2Core::Note::Key");
		cl.def("get_octave", (enum H2Core::Note::Octave (H2Core::Note::*)()) &H2Core::Note::get_octave, "#__octave accessor \n\nC++: H2Core::Note::get_octave() --> enum H2Core::Note::Octave");
		cl.def("get_midi_key", (int (H2Core::Note::*)() const) &H2Core::Note::get_midi_key, "return scaled key for midi output, !!! DO NOT CHECK IF INSTRUMENT IS SET !!! \n\nC++: H2Core::Note::get_midi_key() const --> int");
		cl.def("get_midi_velocity", (int (H2Core::Note::*)() const) &H2Core::Note::get_midi_velocity, "midi velocity accessor \n \n\n\n\n \n\nC++: H2Core::Note::get_midi_velocity() const --> int");
		cl.def("get_notekey_pitch", (float (H2Core::Note::*)() const) &H2Core::Note::get_notekey_pitch, "note key pitch accessor\n \n\n\n\n \n\nC++: H2Core::Note::get_notekey_pitch() const --> float");
		cl.def("get_total_pitch", (float (H2Core::Note::*)() const) &H2Core::Note::get_total_pitch, "returns\n \n\n\n\nC++: H2Core::Note::get_total_pitch() const --> float");
		cl.def("key_to_string", (class QString (H2Core::Note::*)()) &H2Core::Note::key_to_string, "return a string representation of key-octave \n\nC++: H2Core::Note::key_to_string() --> class QString");
		cl.def("set_key_octave", (void (H2Core::Note::*)(const class QString &)) &H2Core::Note::set_key_octave, "parse str and set #__key and #__octave\n \n\n the string to be parsed\n\nC++: H2Core::Note::set_key_octave(const class QString &) --> void", pybind11::arg("str"));
		cl.def("set_key_octave", (void (H2Core::Note::*)(enum H2Core::Note::Key, enum H2Core::Note::Octave)) &H2Core::Note::set_key_octave, "set #__key and #__octave only if within acceptable range\n \n\n the key to set\n \n\n the octave to be set\n\nC++: H2Core::Note::set_key_octave(enum H2Core::Note::Key, enum H2Core::Note::Octave) --> void", pybind11::arg("key"), pybind11::arg("octave"));
		cl.def("set_midi_info", (void (H2Core::Note::*)(enum H2Core::Note::Key, enum H2Core::Note::Octave, int)) &H2Core::Note::set_midi_info, "set #__key, #__octave and #__midi_msg only if within acceptable range\n \n\n the key to set\n \n\n the octave to be set\n \n\n\n		 \n\nC++: H2Core::Note::set_midi_info(enum H2Core::Note::Key, enum H2Core::Note::Octave, int) --> void", pybind11::arg("key"), pybind11::arg("octave"), pybind11::arg("msg"));
		cl.def("match", (bool (H2Core::Note::*)(const class H2Core::Note *) const) &H2Core::Note::match, "Return true if two notes match in instrument, key and octave. \n\nC++: H2Core::Note::match(const class H2Core::Note *) const --> bool", pybind11::arg("pNote"));
		cl.def("compute_lr_values", (void (H2Core::Note::*)(float *, float *)) &H2Core::Note::compute_lr_values, "compute left and right output based on filters\n \n\n the left channel value\n \n\n the right channel value\n\nC++: H2Core::Note::compute_lr_values(float *, float *) --> void", pybind11::arg("val_l"), pybind11::arg("val_r"));
		cl.def("toQString", [](H2Core::Note const &o, const class QString & a0) -> QString { return o.toQString(a0); }, "", pybind11::arg("sPrefix"));
		cl.def("toQString", (class QString (H2Core::Note::*)(const class QString &, bool) const) &H2Core::Note::toQString, "Formatted string version for debugging purposes.\n \n\n String prefix which will be added in front of\n every new line\n \n\n Instead of the whole content of all classes\n stored as members just a single unique identifier will be\n displayed without line breaks.\n\n \n String presentation of current object.\n\nC++: H2Core::Note::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
		cl.def("assign", (class H2Core::Note & (H2Core::Note::*)(const class H2Core::Note &)) &H2Core::Note::operator=, "C++: H2Core::Note::operator=(const class H2Core::Note &) --> class H2Core::Note &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B653_[H2Core::TransportInfo] ";
	{ // H2Core::TransportInfo file:core/IO/TransportInfo.h line:35
		pybind11::class_<H2Core::TransportInfo, std::shared_ptr<H2Core::TransportInfo>, PyCallBack_H2Core_TransportInfo, H2Core::Object> cl(M("H2Core"), "TransportInfo", "Object holding most of the information about the transport state of\n the AudioEngine, like if it is playing or stopped or its current\n transport position and speed.");
		cl.def( pybind11::init( [](){ return new H2Core::TransportInfo(); }, [](){ return new PyCallBack_H2Core_TransportInfo(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_TransportInfo const &o){ return new PyCallBack_H2Core_TransportInfo(o); } ) );
		cl.def( pybind11::init( [](H2Core::TransportInfo const &o){ return new H2Core::TransportInfo(o); } ) );
		cl.def_readwrite("m_status", &H2Core::TransportInfo::m_status);
		cl.def_readwrite("m_nFrames", &H2Core::TransportInfo::m_nFrames);
		cl.def_readwrite("m_fTickSize", &H2Core::TransportInfo::m_fTickSize);
		cl.def_readwrite("m_fBPM", &H2Core::TransportInfo::m_fBPM);
		cl.def_static("class_name", (const char * (*)()) &H2Core::TransportInfo::class_name, "C++: H2Core::TransportInfo::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("printInfo", (void (H2Core::TransportInfo::*)()) &H2Core::TransportInfo::printInfo, "Displays general information about the transport state in\n	    the #INFOLOG\n\n Prints out #m_status, #m_nFrames, and #m_fTickSize.\n\nC++: H2Core::TransportInfo::printInfo() --> void");
		cl.def("assign", (class H2Core::TransportInfo & (H2Core::TransportInfo::*)(const class H2Core::TransportInfo &)) &H2Core::TransportInfo::operator=, "C++: H2Core::TransportInfo::operator=(const class H2Core::TransportInfo &) --> class H2Core::TransportInfo &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B654_[H2Core::AudioOutput] ";
	{ // H2Core::AudioOutput file:core/IO/AudioOutput.h line:38
		pybind11::class_<H2Core::AudioOutput, std::shared_ptr<H2Core::AudioOutput>, PyCallBack_H2Core_AudioOutput, H2Core::Object> cl(M("H2Core"), "AudioOutput", "Base abstract class for audio output classes.");
		cl.def( pybind11::init<const char *>(), pybind11::arg("class_name") );

		cl.def(pybind11::init<PyCallBack_H2Core_AudioOutput const &>());
		cl.def_readwrite("m_transport", &H2Core::AudioOutput::m_transport);
		cl.def("init", (int (H2Core::AudioOutput::*)(unsigned int)) &H2Core::AudioOutput::init, "C++: H2Core::AudioOutput::init(unsigned int) --> int", pybind11::arg("nBufferSize"));
		cl.def("connect", (int (H2Core::AudioOutput::*)()) &H2Core::AudioOutput::connect, "C++: H2Core::AudioOutput::connect() --> int");
		cl.def("disconnect", (void (H2Core::AudioOutput::*)()) &H2Core::AudioOutput::disconnect, "C++: H2Core::AudioOutput::disconnect() --> void");
		cl.def("getBufferSize", (unsigned int (H2Core::AudioOutput::*)()) &H2Core::AudioOutput::getBufferSize, "C++: H2Core::AudioOutput::getBufferSize() --> unsigned int");
		cl.def("getSampleRate", (unsigned int (H2Core::AudioOutput::*)()) &H2Core::AudioOutput::getSampleRate, "C++: H2Core::AudioOutput::getSampleRate() --> unsigned int");
		cl.def("getOut_L", (float * (H2Core::AudioOutput::*)()) &H2Core::AudioOutput::getOut_L, "C++: H2Core::AudioOutput::getOut_L() --> float *", pybind11::return_value_policy::automatic);
		cl.def("getOut_R", (float * (H2Core::AudioOutput::*)()) &H2Core::AudioOutput::getOut_R, "C++: H2Core::AudioOutput::getOut_R() --> float *", pybind11::return_value_policy::automatic);
		cl.def("updateTransportInfo", (void (H2Core::AudioOutput::*)()) &H2Core::AudioOutput::updateTransportInfo, "C++: H2Core::AudioOutput::updateTransportInfo() --> void");
		cl.def("play", (void (H2Core::AudioOutput::*)()) &H2Core::AudioOutput::play, "C++: H2Core::AudioOutput::play() --> void");
		cl.def("stop", (void (H2Core::AudioOutput::*)()) &H2Core::AudioOutput::stop, "C++: H2Core::AudioOutput::stop() --> void");
		cl.def("locate", (void (H2Core::AudioOutput::*)(unsigned long)) &H2Core::AudioOutput::locate, "C++: H2Core::AudioOutput::locate(unsigned long) --> void", pybind11::arg("nFrame"));
		cl.def("setBpm", (void (H2Core::AudioOutput::*)(float)) &H2Core::AudioOutput::setBpm, "C++: H2Core::AudioOutput::setBpm(float) --> void", pybind11::arg("fBPM"));
		cl.def("assign", (class H2Core::AudioOutput & (H2Core::AudioOutput::*)(const class H2Core::AudioOutput &)) &H2Core::AudioOutput::operator=, "C++: H2Core::AudioOutput::operator=(const class H2Core::AudioOutput &) --> class H2Core::AudioOutput &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
