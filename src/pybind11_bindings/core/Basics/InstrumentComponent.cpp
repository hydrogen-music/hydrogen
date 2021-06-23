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
#include <core/Basics/Instrument.h> // H2Core::Instrument
#include <core/Basics/InstrumentComponent.h> // H2Core::InstrumentComponent
#include <core/Basics/InstrumentLayer.h> // H2Core::InstrumentLayer
#include <core/Basics/InstrumentList.h> // H2Core::InstrumentList
#include <core/Basics/Sample.h> // H2Core::Sample
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

// H2Core::InstrumentComponent file:core/Basics/InstrumentComponent.h line:41
struct PyCallBack_H2Core_InstrumentComponent : public H2Core::InstrumentComponent {
	using H2Core::InstrumentComponent::InstrumentComponent;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::InstrumentComponent *>(this), "toQString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return InstrumentComponent::toQString(a0, a1);
	}
};

// H2Core::InstrumentLayer file:core/Basics/InstrumentLayer.h line:43
struct PyCallBack_H2Core_InstrumentLayer : public H2Core::InstrumentLayer {
	using H2Core::InstrumentLayer::InstrumentLayer;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::InstrumentLayer *>(this), "toQString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return InstrumentLayer::toQString(a0, a1);
	}
};

// H2Core::InstrumentList file:core/Basics/InstrumentList.h line:38
struct PyCallBack_H2Core_InstrumentList : public H2Core::InstrumentList {
	using H2Core::InstrumentList::InstrumentList;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::InstrumentList *>(this), "toQString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return InstrumentList::toQString(a0, a1);
	}
};

void bind_core_Basics_InstrumentComponent(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B678_[H2Core::InstrumentComponent] ";
	{ // H2Core::InstrumentComponent file:core/Basics/InstrumentComponent.h line:41
		pybind11::class_<H2Core::InstrumentComponent, std::shared_ptr<H2Core::InstrumentComponent>, PyCallBack_H2Core_InstrumentComponent, H2Core::Object> cl(M("H2Core"), "InstrumentComponent", "");
		cl.def( pybind11::init<int>(), pybind11::arg("related_drumkit_componentID") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_InstrumentComponent const &o){ return new PyCallBack_H2Core_InstrumentComponent(o); } ) );
		cl.def( pybind11::init( [](H2Core::InstrumentComponent const &o){ return new H2Core::InstrumentComponent(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::InstrumentComponent::class_name, "C++: H2Core::InstrumentComponent::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("save_to", (void (H2Core::InstrumentComponent::*)(class H2Core::XMLNode *, int)) &H2Core::InstrumentComponent::save_to, "C++: H2Core::InstrumentComponent::save_to(class H2Core::XMLNode *, int) --> void", pybind11::arg("node"), pybind11::arg("component_id"));
		cl.def("set_drumkit_componentID", (void (H2Core::InstrumentComponent::*)(int)) &H2Core::InstrumentComponent::set_drumkit_componentID, "C++: H2Core::InstrumentComponent::set_drumkit_componentID(int) --> void", pybind11::arg("related_drumkit_componentID"));
		cl.def("get_drumkit_componentID", (int (H2Core::InstrumentComponent::*)()) &H2Core::InstrumentComponent::get_drumkit_componentID, "C++: H2Core::InstrumentComponent::get_drumkit_componentID() --> int");
		cl.def("set_gain", (void (H2Core::InstrumentComponent::*)(float)) &H2Core::InstrumentComponent::set_gain, "C++: H2Core::InstrumentComponent::set_gain(float) --> void", pybind11::arg("gain"));
		cl.def("get_gain", (float (H2Core::InstrumentComponent::*)() const) &H2Core::InstrumentComponent::get_gain, "C++: H2Core::InstrumentComponent::get_gain() const --> float");
		cl.def_static("getMaxLayers", (int (*)()) &H2Core::InstrumentComponent::getMaxLayers, "#m_nMaxLayers.\n\nC++: H2Core::InstrumentComponent::getMaxLayers() --> int");
		cl.def_static("setMaxLayers", (void (*)(int)) &H2Core::InstrumentComponent::setMaxLayers, "Sets #m_nMaxLayers.\n\nC++: H2Core::InstrumentComponent::setMaxLayers(int) --> void", pybind11::arg("layers"));
		cl.def("toQString", [](H2Core::InstrumentComponent const &o, const class QString & a0) -> QString { return o.toQString(a0); }, "", pybind11::arg("sPrefix"));
		cl.def("toQString", (class QString (H2Core::InstrumentComponent::*)(const class QString &, bool) const) &H2Core::InstrumentComponent::toQString, "Formatted string version for debugging purposes.\n \n\n String prefix which will be added in front of\n every new line\n \n\n Instead of the whole content of all classes\n stored as members just a single unique identifier will be\n displayed without line breaks.\n\n \n String presentation of current object.\n\nC++: H2Core::InstrumentComponent::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
		cl.def("assign", (class H2Core::InstrumentComponent & (H2Core::InstrumentComponent::*)(const class H2Core::InstrumentComponent &)) &H2Core::InstrumentComponent::operator=, "C++: H2Core::InstrumentComponent::operator=(const class H2Core::InstrumentComponent &) --> class H2Core::InstrumentComponent &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B679_[H2Core::InstrumentLayer] ";
	{ // H2Core::InstrumentLayer file:core/Basics/InstrumentLayer.h line:43
		pybind11::class_<H2Core::InstrumentLayer, std::shared_ptr<H2Core::InstrumentLayer>, PyCallBack_H2Core_InstrumentLayer, H2Core::Object> cl(M("H2Core"), "InstrumentLayer", "InstrumentLayer is part of an instrument\n each layer has it's own :\n gain which is the ration between the input sample and the output signal,\n pitch which allows you to play the sample at a faster or lower frequency,\n start velocity and end velocity which allows you to chose between a layer or another within an instrument\n by changing the velocity of the played note. so the only layer of an instrument should start at 0.0 and end at 1.0.");
		cl.def( pybind11::init( [](PyCallBack_H2Core_InstrumentLayer const &o){ return new PyCallBack_H2Core_InstrumentLayer(o); } ) );
		cl.def( pybind11::init( [](H2Core::InstrumentLayer const &o){ return new H2Core::InstrumentLayer(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::InstrumentLayer::class_name, "C++: H2Core::InstrumentLayer::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("set_gain", (void (H2Core::InstrumentLayer::*)(float)) &H2Core::InstrumentLayer::set_gain, "set the gain of the layer \n\nC++: H2Core::InstrumentLayer::set_gain(float) --> void", pybind11::arg("gain"));
		cl.def("get_gain", (float (H2Core::InstrumentLayer::*)() const) &H2Core::InstrumentLayer::get_gain, "get the gain of the layer \n\nC++: H2Core::InstrumentLayer::get_gain() const --> float");
		cl.def("set_pitch", (void (H2Core::InstrumentLayer::*)(float)) &H2Core::InstrumentLayer::set_pitch, "set the pitch of the layer \n\nC++: H2Core::InstrumentLayer::set_pitch(float) --> void", pybind11::arg("pitch"));
		cl.def("get_pitch", (float (H2Core::InstrumentLayer::*)() const) &H2Core::InstrumentLayer::get_pitch, "get the pitch of the layer \n\nC++: H2Core::InstrumentLayer::get_pitch() const --> float");
		cl.def("set_start_velocity", (void (H2Core::InstrumentLayer::*)(float)) &H2Core::InstrumentLayer::set_start_velocity, "set the start ivelocity of the layer \n\nC++: H2Core::InstrumentLayer::set_start_velocity(float) --> void", pybind11::arg("start"));
		cl.def("get_start_velocity", (float (H2Core::InstrumentLayer::*)() const) &H2Core::InstrumentLayer::get_start_velocity, "get the start velocity of the layer \n\nC++: H2Core::InstrumentLayer::get_start_velocity() const --> float");
		cl.def("set_end_velocity", (void (H2Core::InstrumentLayer::*)(float)) &H2Core::InstrumentLayer::set_end_velocity, "set the end velocity of the layer \n\nC++: H2Core::InstrumentLayer::set_end_velocity(float) --> void", pybind11::arg("end"));
		cl.def("get_end_velocity", (float (H2Core::InstrumentLayer::*)() const) &H2Core::InstrumentLayer::get_end_velocity, "get the end velocity of the layer \n\nC++: H2Core::InstrumentLayer::get_end_velocity() const --> float");
		cl.def("load_sample", (void (H2Core::InstrumentLayer::*)()) &H2Core::InstrumentLayer::load_sample, "Calls the #H2Core::Sample::load()\n member function of #__sample.\n\nC++: H2Core::InstrumentLayer::load_sample() --> void");
		cl.def("unload_sample", (void (H2Core::InstrumentLayer::*)()) &H2Core::InstrumentLayer::unload_sample, "C++: H2Core::InstrumentLayer::unload_sample() --> void");
		cl.def("save_to", (void (H2Core::InstrumentLayer::*)(class H2Core::XMLNode *)) &H2Core::InstrumentLayer::save_to, "save the instrument layer within the given XMLNode\n \n\n the XMLNode to feed\n\nC++: H2Core::InstrumentLayer::save_to(class H2Core::XMLNode *) --> void", pybind11::arg("node"));
		cl.def("toQString", [](H2Core::InstrumentLayer const &o, const class QString & a0) -> QString { return o.toQString(a0); }, "", pybind11::arg("sPrefix"));
		cl.def("toQString", (class QString (H2Core::InstrumentLayer::*)(const class QString &, bool) const) &H2Core::InstrumentLayer::toQString, "Formatted string version for debugging purposes.\n \n\n String prefix which will be added in front of\n every new line\n \n\n Instead of the whole content of all classes\n stored as members just a single unique identifier will be\n displayed without line breaks.\n\n \n String presentation of current object.\n\nC++: H2Core::InstrumentLayer::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
		cl.def("assign", (class H2Core::InstrumentLayer & (H2Core::InstrumentLayer::*)(const class H2Core::InstrumentLayer &)) &H2Core::InstrumentLayer::operator=, "C++: H2Core::InstrumentLayer::operator=(const class H2Core::InstrumentLayer &) --> class H2Core::InstrumentLayer &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B680_[H2Core::InstrumentList] ";
	{ // H2Core::InstrumentList file:core/Basics/InstrumentList.h line:38
		pybind11::class_<H2Core::InstrumentList, std::shared_ptr<H2Core::InstrumentList>, PyCallBack_H2Core_InstrumentList, H2Core::Object> cl(M("H2Core"), "InstrumentList", "InstrumentList is a collection of instruments used within a song, a drumkit, ...");
		cl.def( pybind11::init( [](){ return new H2Core::InstrumentList(); }, [](){ return new PyCallBack_H2Core_InstrumentList(); } ) );
		cl.def( pybind11::init<class H2Core::InstrumentList *>(), pybind11::arg("other") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_InstrumentList const &o){ return new PyCallBack_H2Core_InstrumentList(o); } ) );
		cl.def( pybind11::init( [](H2Core::InstrumentList const &o){ return new H2Core::InstrumentList(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::InstrumentList::class_name, "C++: H2Core::InstrumentList::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("size", (int (H2Core::InstrumentList::*)() const) &H2Core::InstrumentList::size, "returns the numbers of instruments \n\nC++: H2Core::InstrumentList::size() const --> int");
		cl.def("is_valid_index", (bool (H2Core::InstrumentList::*)(int) const) &H2Core::InstrumentList::is_valid_index, "check if there is a idx is a valid index for this list\n without throwing an error messaage\n \n\n the index of the instrument\n\nC++: H2Core::InstrumentList::is_valid_index(int) const --> bool", pybind11::arg("idx"));
		cl.def("swap", (void (H2Core::InstrumentList::*)(int, int)) &H2Core::InstrumentList::swap, "swap the instruments of two different indexes\n \n\n the first index\n \n\n the second index\n\nC++: H2Core::InstrumentList::swap(int, int) --> void", pybind11::arg("idx_a"), pybind11::arg("idx_b"));
		cl.def("move", (void (H2Core::InstrumentList::*)(int, int)) &H2Core::InstrumentList::move, "move an instrument from a position to another\n \n\n the start index\n \n\n the finish index\n\nC++: H2Core::InstrumentList::move(int, int) --> void", pybind11::arg("idx_a"), pybind11::arg("idx_b"));
		cl.def("load_samples", (void (H2Core::InstrumentList::*)()) &H2Core::InstrumentList::load_samples, "Calls the Instrument::load_samples() member\n function of all Instruments in #__instruments.\n\nC++: H2Core::InstrumentList::load_samples() --> void");
		cl.def("unload_samples", (void (H2Core::InstrumentList::*)()) &H2Core::InstrumentList::unload_samples, "Calls the Instrument::unload_samples() member\n function of all Instruments in #__instruments.\n\nC++: H2Core::InstrumentList::unload_samples() --> void");
		cl.def("save_to", (void (H2Core::InstrumentList::*)(class H2Core::XMLNode *, int)) &H2Core::InstrumentList::save_to, "save the instrument list within the given XMLNode\n \n\n the XMLNode to feed\n \n\n Identifier of the corresponding\n component.\n\nC++: H2Core::InstrumentList::save_to(class H2Core::XMLNode *, int) --> void", pybind11::arg("node"), pybind11::arg("component_id"));
		cl.def_static("load_from", (class H2Core::InstrumentList * (*)(class H2Core::XMLNode *, const class QString &, const class QString &)) &H2Core::InstrumentList::load_from, "load an instrument list from an XMLNode\n \n\n the XMLDode to read from\n \n\n the directory holding the drumkit\n data\n \n\n\n \n\n a new InstrumentList instance\n\nC++: H2Core::InstrumentList::load_from(class H2Core::XMLNode *, const class QString &, const class QString &) --> class H2Core::InstrumentList *", pybind11::return_value_policy::automatic, pybind11::arg("node"), pybind11::arg("dk_path"), pybind11::arg("dk_name"));
		cl.def("fix_issue_307", (void (H2Core::InstrumentList::*)()) &H2Core::InstrumentList::fix_issue_307, "Fix GitHub issue #307, so called \"Hi Bongo fiasco\".\n\n Check whether the same MIDI note is assignedto every\n instrument - that condition makes MIDI export unusable.\n When so, assign each instrument consecutive MIDI note\n starting from 36.\n\nC++: H2Core::InstrumentList::fix_issue_307() --> void");
		cl.def("has_all_midi_notes_same", (bool (H2Core::InstrumentList::*)() const) &H2Core::InstrumentList::has_all_midi_notes_same, "Check if all instruments have assigned the same\n MIDI out note\n\nC++: H2Core::InstrumentList::has_all_midi_notes_same() const --> bool");
		cl.def("set_default_midi_out_notes", (void (H2Core::InstrumentList::*)()) &H2Core::InstrumentList::set_default_midi_out_notes, "Set each instrument consecuteve MIDI\n out notes, starting from 36\n\nC++: H2Core::InstrumentList::set_default_midi_out_notes() --> void");
		cl.def("toQString", [](H2Core::InstrumentList const &o, const class QString & a0) -> QString { return o.toQString(a0); }, "", pybind11::arg("sPrefix"));
		cl.def("toQString", (class QString (H2Core::InstrumentList::*)(const class QString &, bool) const) &H2Core::InstrumentList::toQString, "Formatted string version for debugging purposes.\n \n\n String prefix which will be added in front of\n every new line\n \n\n Instead of the whole content of all classes\n stored as members just a single unique identifier will be\n displayed without line breaks.\n\n \n String presentation of current object.\n\nC++: H2Core::InstrumentList::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
		cl.def("assign", (class H2Core::InstrumentList & (H2Core::InstrumentList::*)(const class H2Core::InstrumentList &)) &H2Core::InstrumentList::operator=, "C++: H2Core::InstrumentList::operator=(const class H2Core::InstrumentList &) --> class H2Core::InstrumentList &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
