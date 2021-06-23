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
#include <core/Basics/Drumkit.h> // H2Core::Drumkit
#include <core/Basics/Instrument.h> // 
#include <core/Basics/Instrument.h> // H2Core::Instrument
#include <core/Basics/InstrumentComponent.h> // H2Core::InstrumentComponent
#include <core/Basics/InstrumentList.h> // H2Core::InstrumentList
#include <core/Helpers/Filesystem.h> // 
#include <core/Helpers/Xml.h> // H2Core::XMLNode
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

// H2Core::Instrument file:core/Basics/Instrument.h line:52
struct PyCallBack_H2Core_Instrument : public H2Core::Instrument {
	using H2Core::Instrument::Instrument;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::Instrument *>(this), "toQString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return Instrument::toQString(a0, a1);
	}
};

void bind_core_Basics_Instrument(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B650_[H2Core::Instrument] ";
	{ // H2Core::Instrument file:core/Basics/Instrument.h line:52
		pybind11::class_<H2Core::Instrument, std::shared_ptr<H2Core::Instrument>, PyCallBack_H2Core_Instrument, H2Core::Object> cl(M("H2Core"), "Instrument", "Instrument class");
		cl.def( pybind11::init( [](PyCallBack_H2Core_Instrument const &o){ return new PyCallBack_H2Core_Instrument(o); } ) );
		cl.def( pybind11::init( [](H2Core::Instrument const &o){ return new H2Core::Instrument(o); } ) );

		pybind11::enum_<H2Core::Instrument::SampleSelectionAlgo>(cl, "SampleSelectionAlgo", pybind11::arithmetic(), "")
			.value("VELOCITY", H2Core::Instrument::VELOCITY)
			.value("ROUND_ROBIN", H2Core::Instrument::ROUND_ROBIN)
			.value("RANDOM", H2Core::Instrument::RANDOM)
			.export_values();

		cl.def_static("class_name", (const char * (*)()) &H2Core::Instrument::class_name, "C++: H2Core::Instrument::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("load_from", [](H2Core::Instrument &o, const class QString & a0, const class QString & a1) -> void { return o.load_from(a0, a1); }, "", pybind11::arg("drumkit_name"), pybind11::arg("instrument_name"));
		cl.def("load_from", [](H2Core::Instrument &o, const class QString & a0, const class QString & a1, bool const & a2) -> void { return o.load_from(a0, a1, a2); }, "", pybind11::arg("drumkit_name"), pybind11::arg("instrument_name"), pybind11::arg("is_live"));
		cl.def("load_from", (void (H2Core::Instrument::*)(const class QString &, const class QString &, bool, enum H2Core::Filesystem::Lookup)) &H2Core::Instrument::load_from, "loads instrument from a given instrument within a given drumkit into a `live` Instrument object.\n \n\n the drumkit to search the instrument in\n \n\n the instrument within the drumkit to load samples from\n \n\n is it performed while playing\n \n\n Where to search (system/user folder or both)\n for the drumkit.\n\nC++: H2Core::Instrument::load_from(const class QString &, const class QString &, bool, enum H2Core::Filesystem::Lookup) --> void", pybind11::arg("drumkit_name"), pybind11::arg("instrument_name"), pybind11::arg("is_live"), pybind11::arg("lookup"));
		cl.def("load_samples", (void (H2Core::Instrument::*)()) &H2Core::Instrument::load_samples, "Calls the InstrumentLayer::load_sample() member\n function of all layers of each component of the\n Instrument.\n\nC++: H2Core::Instrument::load_samples() --> void");
		cl.def("unload_samples", (void (H2Core::Instrument::*)()) &H2Core::Instrument::unload_samples, "Calls the InstrumentLayer::unload_sample() member\n function of all layers of each component of the\n Instrument.\n\nC++: H2Core::Instrument::unload_samples() --> void");
		cl.def("save_to", (void (H2Core::Instrument::*)(class H2Core::XMLNode *, int)) &H2Core::Instrument::save_to, "save the instrument within the given XMLNode\n \n\n the XMLNode to feed\n \n\n Identifier of the corresponding\n component.\n\nC++: H2Core::Instrument::save_to(class H2Core::XMLNode *, int) --> void", pybind11::arg("node"), pybind11::arg("component_id"));
		cl.def("set_name", (void (H2Core::Instrument::*)(const class QString &)) &H2Core::Instrument::set_name, "C++: H2Core::Instrument::set_name(const class QString &) --> void", pybind11::arg("name"));
		cl.def("get_name", (const class QString & (H2Core::Instrument::*)() const) &H2Core::Instrument::get_name, "C++: H2Core::Instrument::get_name() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("set_id", (void (H2Core::Instrument::*)(const int)) &H2Core::Instrument::set_id, "C++: H2Core::Instrument::set_id(const int) --> void", pybind11::arg("id"));
		cl.def("get_id", (int (H2Core::Instrument::*)() const) &H2Core::Instrument::get_id, "C++: H2Core::Instrument::get_id() const --> int");
		cl.def("set_mute_group", (void (H2Core::Instrument::*)(int)) &H2Core::Instrument::set_mute_group, "set the mute group of the instrument \n\nC++: H2Core::Instrument::set_mute_group(int) --> void", pybind11::arg("group"));
		cl.def("get_mute_group", (int (H2Core::Instrument::*)() const) &H2Core::Instrument::get_mute_group, "get the mute group of the instrument \n\nC++: H2Core::Instrument::get_mute_group() const --> int");
		cl.def("set_midi_out_channel", (void (H2Core::Instrument::*)(int)) &H2Core::Instrument::set_midi_out_channel, "set the midi out channel of the instrument \n\nC++: H2Core::Instrument::set_midi_out_channel(int) --> void", pybind11::arg("channel"));
		cl.def("get_midi_out_channel", (int (H2Core::Instrument::*)() const) &H2Core::Instrument::get_midi_out_channel, "get the midi out channel of the instrument \n\nC++: H2Core::Instrument::get_midi_out_channel() const --> int");
		cl.def("set_midi_out_note", (void (H2Core::Instrument::*)(int)) &H2Core::Instrument::set_midi_out_note, "set the midi out note of the instrument \n\nC++: H2Core::Instrument::set_midi_out_note(int) --> void", pybind11::arg("note"));
		cl.def("get_midi_out_note", (int (H2Core::Instrument::*)() const) &H2Core::Instrument::get_midi_out_note, "get the midi out note of the instrument \n\nC++: H2Core::Instrument::get_midi_out_note() const --> int");
		cl.def("set_muted", (void (H2Core::Instrument::*)(bool)) &H2Core::Instrument::set_muted, "set muted status of the instrument \n\nC++: H2Core::Instrument::set_muted(bool) --> void", pybind11::arg("muted"));
		cl.def("is_muted", (bool (H2Core::Instrument::*)() const) &H2Core::Instrument::is_muted, "get muted status of the instrument \n\nC++: H2Core::Instrument::is_muted() const --> bool");
		cl.def("setPan", (void (H2Core::Instrument::*)(float)) &H2Core::Instrument::setPan, "set pan of the instrument \n\nC++: H2Core::Instrument::setPan(float) --> void", pybind11::arg("val"));
		cl.def("setPanWithRangeFrom0To1", (void (H2Core::Instrument::*)(float)) &H2Core::Instrument::setPanWithRangeFrom0To1, "set pan of the instrument, assuming the input range in [0;1] \n\nC++: H2Core::Instrument::setPanWithRangeFrom0To1(float) --> void", pybind11::arg("fVal"));
		cl.def("getPan", (float (H2Core::Instrument::*)() const) &H2Core::Instrument::getPan, "get pan of the instrument \n\nC++: H2Core::Instrument::getPan() const --> float");
		cl.def("getPanWithRangeFrom0To1", (float (H2Core::Instrument::*)() const) &H2Core::Instrument::getPanWithRangeFrom0To1, "get pan of the instrument scaling and translating the range from [-1;1] to [0;1] \n\nC++: H2Core::Instrument::getPanWithRangeFrom0To1() const --> float");
		cl.def("set_gain", (void (H2Core::Instrument::*)(float)) &H2Core::Instrument::set_gain, "set gain of the instrument \n\nC++: H2Core::Instrument::set_gain(float) --> void", pybind11::arg("gain"));
		cl.def("get_gain", (float (H2Core::Instrument::*)() const) &H2Core::Instrument::get_gain, "get gain of the instrument \n\nC++: H2Core::Instrument::get_gain() const --> float");
		cl.def("set_volume", (void (H2Core::Instrument::*)(float)) &H2Core::Instrument::set_volume, "set the volume of the instrument \n\nC++: H2Core::Instrument::set_volume(float) --> void", pybind11::arg("volume"));
		cl.def("get_volume", (float (H2Core::Instrument::*)() const) &H2Core::Instrument::get_volume, "get the volume of the instrument \n\nC++: H2Core::Instrument::get_volume() const --> float");
		cl.def("set_filter_active", (void (H2Core::Instrument::*)(bool)) &H2Core::Instrument::set_filter_active, "activate the filter of the instrument \n\nC++: H2Core::Instrument::set_filter_active(bool) --> void", pybind11::arg("active"));
		cl.def("is_filter_active", (bool (H2Core::Instrument::*)() const) &H2Core::Instrument::is_filter_active, "get the status of the filter of the instrument \n\nC++: H2Core::Instrument::is_filter_active() const --> bool");
		cl.def("set_filter_resonance", (void (H2Core::Instrument::*)(float)) &H2Core::Instrument::set_filter_resonance, "set the filter resonance of the instrument \n\nC++: H2Core::Instrument::set_filter_resonance(float) --> void", pybind11::arg("val"));
		cl.def("get_filter_resonance", (float (H2Core::Instrument::*)() const) &H2Core::Instrument::get_filter_resonance, "get the filter resonance of the instrument \n\nC++: H2Core::Instrument::get_filter_resonance() const --> float");
		cl.def("set_filter_cutoff", (void (H2Core::Instrument::*)(float)) &H2Core::Instrument::set_filter_cutoff, "set the filter cutoff of the instrument \n\nC++: H2Core::Instrument::set_filter_cutoff(float) --> void", pybind11::arg("val"));
		cl.def("get_filter_cutoff", (float (H2Core::Instrument::*)() const) &H2Core::Instrument::get_filter_cutoff, "get the filter cutoff of the instrument \n\nC++: H2Core::Instrument::get_filter_cutoff() const --> float");
		cl.def("set_peak_l", (void (H2Core::Instrument::*)(float)) &H2Core::Instrument::set_peak_l, "set the left peak of the instrument \n\nC++: H2Core::Instrument::set_peak_l(float) --> void", pybind11::arg("val"));
		cl.def("get_peak_l", (float (H2Core::Instrument::*)() const) &H2Core::Instrument::get_peak_l, "get the left peak of the instrument \n\nC++: H2Core::Instrument::get_peak_l() const --> float");
		cl.def("set_peak_r", (void (H2Core::Instrument::*)(float)) &H2Core::Instrument::set_peak_r, "set the right peak of the instrument \n\nC++: H2Core::Instrument::set_peak_r(float) --> void", pybind11::arg("val"));
		cl.def("get_peak_r", (float (H2Core::Instrument::*)() const) &H2Core::Instrument::get_peak_r, "get the right peak of the instrument \n\nC++: H2Core::Instrument::get_peak_r() const --> float");
		cl.def("set_fx_level", (void (H2Core::Instrument::*)(float, int)) &H2Core::Instrument::set_fx_level, "set the fx level of the instrument \n\nC++: H2Core::Instrument::set_fx_level(float, int) --> void", pybind11::arg("level"), pybind11::arg("index"));
		cl.def("get_fx_level", (float (H2Core::Instrument::*)(int) const) &H2Core::Instrument::get_fx_level, "get the fx level of the instrument \n\nC++: H2Core::Instrument::get_fx_level(int) const --> float", pybind11::arg("index"));
		cl.def("set_random_pitch_factor", (void (H2Core::Instrument::*)(float)) &H2Core::Instrument::set_random_pitch_factor, "set the random pitch factor of the instrument \n\nC++: H2Core::Instrument::set_random_pitch_factor(float) --> void", pybind11::arg("val"));
		cl.def("get_random_pitch_factor", (float (H2Core::Instrument::*)() const) &H2Core::Instrument::get_random_pitch_factor, "get the random pitch factor of the instrument \n\nC++: H2Core::Instrument::get_random_pitch_factor() const --> float");
		cl.def("set_pitch_offset", (void (H2Core::Instrument::*)(float)) &H2Core::Instrument::set_pitch_offset, "set the pitch offset of the instrument \n\nC++: H2Core::Instrument::set_pitch_offset(float) --> void", pybind11::arg("val"));
		cl.def("get_pitch_offset", (float (H2Core::Instrument::*)() const) &H2Core::Instrument::get_pitch_offset, "get the pitch offset of the instrument \n\nC++: H2Core::Instrument::get_pitch_offset() const --> float");
		cl.def("set_active", (void (H2Core::Instrument::*)(bool)) &H2Core::Instrument::set_active, "set the active status of the instrument \n\nC++: H2Core::Instrument::set_active(bool) --> void", pybind11::arg("active"));
		cl.def("is_active", (bool (H2Core::Instrument::*)() const) &H2Core::Instrument::is_active, "get the active status of the instrument \n\nC++: H2Core::Instrument::is_active() const --> bool");
		cl.def("set_soloed", (void (H2Core::Instrument::*)(bool)) &H2Core::Instrument::set_soloed, "set the soloed status of the instrument \n\nC++: H2Core::Instrument::set_soloed(bool) --> void", pybind11::arg("soloed"));
		cl.def("is_soloed", (bool (H2Core::Instrument::*)() const) &H2Core::Instrument::is_soloed, "get the soloed status of the instrument \n\nC++: H2Core::Instrument::is_soloed() const --> bool");
		cl.def("enqueue", (void (H2Core::Instrument::*)()) &H2Core::Instrument::enqueue, "enqueue the instrument \n\nC++: H2Core::Instrument::enqueue() --> void");
		cl.def("dequeue", (void (H2Core::Instrument::*)()) &H2Core::Instrument::dequeue, "dequeue the instrument \n\nC++: H2Core::Instrument::dequeue() --> void");
		cl.def("is_queued", (bool (H2Core::Instrument::*)() const) &H2Core::Instrument::is_queued, "get the queued status of the instrument \n\nC++: H2Core::Instrument::is_queued() const --> bool");
		cl.def("set_stop_notes", (void (H2Core::Instrument::*)(bool)) &H2Core::Instrument::set_stop_notes, "set the stop notes status of the instrument \n\nC++: H2Core::Instrument::set_stop_notes(bool) --> void", pybind11::arg("stopnotes"));
		cl.def("is_stop_notes", (bool (H2Core::Instrument::*)() const) &H2Core::Instrument::is_stop_notes, "get the stop notes of the instrument \n\nC++: H2Core::Instrument::is_stop_notes() const --> bool");
		cl.def("set_sample_selection_alg", (void (H2Core::Instrument::*)(enum H2Core::Instrument::SampleSelectionAlgo)) &H2Core::Instrument::set_sample_selection_alg, "C++: H2Core::Instrument::set_sample_selection_alg(enum H2Core::Instrument::SampleSelectionAlgo) --> void", pybind11::arg("selected_algo"));
		cl.def("sample_selection_alg", (enum H2Core::Instrument::SampleSelectionAlgo (H2Core::Instrument::*)() const) &H2Core::Instrument::sample_selection_alg, "C++: H2Core::Instrument::sample_selection_alg() const --> enum H2Core::Instrument::SampleSelectionAlgo");
		cl.def("set_hihat_grp", (void (H2Core::Instrument::*)(int)) &H2Core::Instrument::set_hihat_grp, "C++: H2Core::Instrument::set_hihat_grp(int) --> void", pybind11::arg("hihat_grp"));
		cl.def("get_hihat_grp", (int (H2Core::Instrument::*)() const) &H2Core::Instrument::get_hihat_grp, "C++: H2Core::Instrument::get_hihat_grp() const --> int");
		cl.def("set_lower_cc", (void (H2Core::Instrument::*)(int)) &H2Core::Instrument::set_lower_cc, "C++: H2Core::Instrument::set_lower_cc(int) --> void", pybind11::arg("message"));
		cl.def("get_lower_cc", (int (H2Core::Instrument::*)() const) &H2Core::Instrument::get_lower_cc, "C++: H2Core::Instrument::get_lower_cc() const --> int");
		cl.def("set_higher_cc", (void (H2Core::Instrument::*)(int)) &H2Core::Instrument::set_higher_cc, "C++: H2Core::Instrument::set_higher_cc(int) --> void", pybind11::arg("message"));
		cl.def("get_higher_cc", (int (H2Core::Instrument::*)() const) &H2Core::Instrument::get_higher_cc, "C++: H2Core::Instrument::get_higher_cc() const --> int");
		cl.def("set_drumkit_name", (void (H2Core::Instrument::*)(const class QString &)) &H2Core::Instrument::set_drumkit_name, "C++: H2Core::Instrument::set_drumkit_name(const class QString &) --> void", pybind11::arg("name"));
		cl.def("get_drumkit_name", (const class QString & (H2Core::Instrument::*)() const) &H2Core::Instrument::get_drumkit_name, "C++: H2Core::Instrument::get_drumkit_name() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("set_is_preview_instrument", (void (H2Core::Instrument::*)(bool)) &H2Core::Instrument::set_is_preview_instrument, "Mark the instrument as hydrogen's preview instrument \n\nC++: H2Core::Instrument::set_is_preview_instrument(bool) --> void", pybind11::arg("isPreview"));
		cl.def("is_preview_instrument", (bool (H2Core::Instrument::*)() const) &H2Core::Instrument::is_preview_instrument, "C++: H2Core::Instrument::is_preview_instrument() const --> bool");
		cl.def("set_is_metronome_instrument", (void (H2Core::Instrument::*)(bool)) &H2Core::Instrument::set_is_metronome_instrument, "Mark the instrument as metronome instrument \n\nC++: H2Core::Instrument::set_is_metronome_instrument(bool) --> void", pybind11::arg("isMetronome"));
		cl.def("is_metronome_instrument", (bool (H2Core::Instrument::*)() const) &H2Core::Instrument::is_metronome_instrument, "C++: H2Core::Instrument::is_metronome_instrument() const --> bool");
		cl.def("set_apply_velocity", (void (H2Core::Instrument::*)(bool)) &H2Core::Instrument::set_apply_velocity, "C++: H2Core::Instrument::set_apply_velocity(bool) --> void", pybind11::arg("apply_velocity"));
		cl.def("get_apply_velocity", (bool (H2Core::Instrument::*)() const) &H2Core::Instrument::get_apply_velocity, "C++: H2Core::Instrument::get_apply_velocity() const --> bool");
		cl.def("is_currently_exported", (bool (H2Core::Instrument::*)() const) &H2Core::Instrument::is_currently_exported, "C++: H2Core::Instrument::is_currently_exported() const --> bool");
		cl.def("set_currently_exported", (void (H2Core::Instrument::*)(bool)) &H2Core::Instrument::set_currently_exported, "C++: H2Core::Instrument::set_currently_exported(bool) --> void", pybind11::arg("isCurrentlyExported"));
		cl.def("has_missing_samples", (bool (H2Core::Instrument::*)() const) &H2Core::Instrument::has_missing_samples, "C++: H2Core::Instrument::has_missing_samples() const --> bool");
		cl.def("set_missing_samples", (void (H2Core::Instrument::*)(bool)) &H2Core::Instrument::set_missing_samples, "C++: H2Core::Instrument::set_missing_samples(bool) --> void", pybind11::arg("bHasMissingSamples"));
		cl.def("toQString", [](H2Core::Instrument const &o, const class QString & a0) -> QString { return o.toQString(a0); }, "", pybind11::arg("sPrefix"));
		cl.def("toQString", (class QString (H2Core::Instrument::*)(const class QString &, bool) const) &H2Core::Instrument::toQString, "Formatted string version for debugging purposes.\n \n\n String prefix which will be added in front of\n every new line\n \n\n Instead of the whole content of all classes\n stored as members just a single unique identifier will be\n displayed without line breaks.\n\n \n String presentation of current object.\n\nC++: H2Core::Instrument::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
		cl.def("assign", (class H2Core::Instrument & (H2Core::Instrument::*)(const class H2Core::Instrument &)) &H2Core::Instrument::operator=, "C++: H2Core::Instrument::operator=(const class H2Core::Instrument &) --> class H2Core::Instrument &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
