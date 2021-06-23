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
#include <QtCore/qlocale.h> // QLocale
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
#include <QtCore/qobject.h> // QObject
#include <QtCore/qobjectdefs.h> // QMetaObject
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
#include <core/Basics/Instrument.h> // H2Core::Instrument
#include <core/Basics/InstrumentList.h> // H2Core::InstrumentList
#include <core/Basics/Note.h> // 
#include <core/Basics/Note.h> // H2Core::Note
#include <core/Basics/Note.h> // H2Core::SelectedLayerInfo
#include <core/Basics/Pattern.h> // H2Core::Pattern
#include <core/Basics/PatternList.h> // H2Core::PatternList
#include <core/Basics/Playlist.h> // H2Core::Playlist
#include <core/Helpers/Filesystem.h> // 
#include <core/Helpers/Legacy.h> // H2Core::Legacy
#include <core/Helpers/Translations.h> // H2Core::Translations
#include <core/Helpers/Xml.h> // H2Core::XMLNode
#include <core/IO/MidiInput.h> // H2Core::MidiInput
#include <core/IO/MidiOutput.h> // H2Core::MidiOutput
#include <core/Object.h> // H2Core::Object
#include <core/Timeline.h> // H2Core::Timeline
#include <core/Timeline.h> // H2Core::Timeline::Tag
#include <core/Timeline.h> // H2Core::Timeline::TempoMarker
#include <functional> // std::less
#include <iostream> // --trace
#include <iterator> // __gnu_cxx::__normal_iterator
#include <iterator> // std::reverse_iterator
#include <map> // std::multimap
#include <memory> // std::allocator
#include <memory> // std::shared_ptr
#include <set> // std::set
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <utility> // std::pair
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

// H2Core::Legacy file:core/Helpers/Legacy.h line:17
struct PyCallBack_H2Core_Legacy : public H2Core::Legacy {
	using H2Core::Legacy::Legacy;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::Legacy *>(this), "toQString");
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

// H2Core::Translations file:core/Helpers/Translations.h line:20
struct PyCallBack_H2Core_Translations : public H2Core::Translations {
	using H2Core::Translations::Translations;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::Translations *>(this), "toQString");
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

// H2Core::Timeline file:core/Timeline.h line:40
struct PyCallBack_H2Core_Timeline : public H2Core::Timeline {
	using H2Core::Timeline::Timeline;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::Timeline *>(this), "toQString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return Timeline::toQString(a0, a1);
	}
};

void bind_core_Helpers_Legacy(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B701_[H2Core::Legacy] ";
	{ // H2Core::Legacy file:core/Helpers/Legacy.h line:17
		pybind11::class_<H2Core::Legacy, std::shared_ptr<H2Core::Legacy>, PyCallBack_H2Core_Legacy, H2Core::Object> cl(M("H2Core"), "Legacy", "Legacy is a container for legacy code which should be once removed");
		cl.def( pybind11::init( [](){ return new H2Core::Legacy(); }, [](){ return new PyCallBack_H2Core_Legacy(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_Legacy const &o){ return new PyCallBack_H2Core_Legacy(o); } ) );
		cl.def( pybind11::init( [](H2Core::Legacy const &o){ return new H2Core::Legacy(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::Legacy::class_name, "C++: H2Core::Legacy::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def_static("load_drumkit", (class H2Core::Drumkit * (*)(const class QString &)) &H2Core::Legacy::load_drumkit, "load drumkit information from a file\n \n\n is a path to an xml file\n \n\n a Drumkit on success, 0 otherwise\n\nC++: H2Core::Legacy::load_drumkit(const class QString &) --> class H2Core::Drumkit *", pybind11::return_value_policy::automatic, pybind11::arg("dk_path"));
		cl.def_static("load_drumkit_pattern", (class H2Core::Pattern * (*)(const class QString &, class H2Core::InstrumentList *)) &H2Core::Legacy::load_drumkit_pattern, "load pattern from a file\n \n\n is a path to an xml file\n \n\n\n \n\n a Pattern on success, 0 otherwise\n\nC++: H2Core::Legacy::load_drumkit_pattern(const class QString &, class H2Core::InstrumentList *) --> class H2Core::Pattern *", pybind11::return_value_policy::automatic, pybind11::arg("pattern_path"), pybind11::arg("instrList"));
		cl.def_static("load_playlist", (class H2Core::Playlist * (*)(class H2Core::Playlist *, const class QString &)) &H2Core::Legacy::load_playlist, "load playlist from a file\n \n\n the playlist to feed\n \n\n is a path to an xml file\n \n\n a Playlist on success, 0 otherwise\n\nC++: H2Core::Legacy::load_playlist(class H2Core::Playlist *, const class QString &) --> class H2Core::Playlist *", pybind11::return_value_policy::automatic, pybind11::arg("pl"), pybind11::arg("pl_path"));
		cl.def("assign", (class H2Core::Legacy & (H2Core::Legacy::*)(const class H2Core::Legacy &)) &H2Core::Legacy::operator=, "C++: H2Core::Legacy::operator=(const class H2Core::Legacy &) --> class H2Core::Legacy &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B702_[H2Core::Translations] ";
	{ // H2Core::Translations file:core/Helpers/Translations.h line:20
		pybind11::class_<H2Core::Translations, std::shared_ptr<H2Core::Translations>, PyCallBack_H2Core_Translations, H2Core::Object> cl(M("H2Core"), "Translations", "Translations manager");
		cl.def( pybind11::init( [](){ return new H2Core::Translations(); }, [](){ return new PyCallBack_H2Core_Translations(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_Translations const &o){ return new PyCallBack_H2Core_Translations(o); } ) );
		cl.def( pybind11::init( [](H2Core::Translations const &o){ return new H2Core::Translations(o); } ) );
		cl.def("assign", (class H2Core::Translations & (H2Core::Translations::*)(const class H2Core::Translations &)) &H2Core::Translations::operator=, "C++: H2Core::Translations::operator=(const class H2Core::Translations &) --> class H2Core::Translations &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B703_[H2Core::Timeline] ";
	{ // H2Core::Timeline file:core/Timeline.h line:40
		pybind11::class_<H2Core::Timeline, std::shared_ptr<H2Core::Timeline>, PyCallBack_H2Core_Timeline, H2Core::Object> cl(M("H2Core"), "Timeline", "Timeline class storing and handling all TempoMarkers and Tags.\n\n All methods altering the TempoMarker and Tag are members of\n this class and the former are added as const structs to\n m_tempoMarkers or m_tags. To alter one of them, one has to\n delete it and add a new, altered version.");
		cl.def( pybind11::init( [](){ return new H2Core::Timeline(); }, [](){ return new PyCallBack_H2Core_Timeline(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_Timeline const &o){ return new PyCallBack_H2Core_Timeline(o); } ) );
		cl.def( pybind11::init( [](H2Core::Timeline const &o){ return new H2Core::Timeline(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::Timeline::class_name, "C++: H2Core::Timeline::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("addTempoMarker", (void (H2Core::Timeline::*)(int, float)) &H2Core::Timeline::addTempoMarker, "Position of the Timeline to query for a \n   tempo marker.\n \n\n New tempo in beats per minute. All values\n   below 30 and above 500 will be cut.\n\nC++: H2Core::Timeline::addTempoMarker(int, float) --> void", pybind11::arg("nBar"), pybind11::arg("fBpm"));
		cl.def("deleteTempoMarker", (void (H2Core::Timeline::*)(int)) &H2Core::Timeline::deleteTempoMarker, "Position of the Timeline to delete the\n tempo marker at (if one is present).\n\nC++: H2Core::Timeline::deleteTempoMarker(int) --> void", pybind11::arg("nBar"));
		cl.def("deleteAllTempoMarkers", (void (H2Core::Timeline::*)()) &H2Core::Timeline::deleteAllTempoMarkers, "C++: H2Core::Timeline::deleteAllTempoMarkers() --> void");
		cl.def("getTempoAtBar", (float (H2Core::Timeline::*)(int, bool) const) &H2Core::Timeline::getTempoAtBar, "Returns the tempo of the Song at a given bar.\n\n \n Position of the Timeline to query for a \n   tempo marker.\n \n\n If set to true either the tempo marker\n   at `nBar` or - if none is present - the nearest\n   previous tempo marker is returned. If set to false,\n   only the precise position `nBar` is taken into\n   account.\n\n TODO: For now the function returns 0 if the bar is\n positioned _before_ the first tempo marker. The calling\n routine Hydrogen::getTimelineBpm() will take care of\n this and replaces it with pSong->__bpm. This will be\n taken care of with #854.\n\nC++: H2Core::Timeline::getTempoAtBar(int, bool) const --> float", pybind11::arg("nBar"), pybind11::arg("bSticky"));
		cl.def("addTag", (void (H2Core::Timeline::*)(int, class QString)) &H2Core::Timeline::addTag, "Position of the Timeline to query for a \n   tag.\n \n\n New tag in beats per minute.\n\nC++: H2Core::Timeline::addTag(int, class QString) --> void", pybind11::arg("nBar"), pybind11::arg("sTag"));
		cl.def("deleteTag", (void (H2Core::Timeline::*)(int)) &H2Core::Timeline::deleteTag, "Position of the Timeline to delete the tag\n at (if one is present).\n\nC++: H2Core::Timeline::deleteTag(int) --> void", pybind11::arg("nBar"));
		cl.def("deleteAllTags", (void (H2Core::Timeline::*)()) &H2Core::Timeline::deleteAllTags, "C++: H2Core::Timeline::deleteAllTags() --> void");
		cl.def("getTagAtBar", (const class QString (H2Core::Timeline::*)(int, bool) const) &H2Core::Timeline::getTagAtBar, "Returns the tag of the Song at a given bar.\n\n \n Position of the Timeline to query for a \n   tag.\n \n\n If set to true either the tag at `nBar`\n   or - if none is present - the nearest previous tag is\n   returned. If set to false, only the precise position\n   `nBar` is taken into account.\n\n The function returns \"\" if the bar is positioned\n _before_ the first tag or none is present at all.\n\nC++: H2Core::Timeline::getTagAtBar(int, bool) const --> const class QString", pybind11::arg("nBar"), pybind11::arg("bSticky"));
		cl.def("toQString", [](H2Core::Timeline const &o, const class QString & a0) -> QString { return o.toQString(a0); }, "", pybind11::arg("sPrefix"));
		cl.def("toQString", (class QString (H2Core::Timeline::*)(const class QString &, bool) const) &H2Core::Timeline::toQString, "Formatted string version for debugging purposes.\n \n\n String prefix which will be added in front of\n every new line\n \n\n Instead of the whole content of all classes\n stored as members just a single unique identifier will be\n displayed without line breaks.\n\n \n String presentation of current object.\n\nC++: H2Core::Timeline::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
		cl.def("assign", (class H2Core::Timeline & (H2Core::Timeline::*)(const class H2Core::Timeline &)) &H2Core::Timeline::operator=, "C++: H2Core::Timeline::operator=(const class H2Core::Timeline &) --> class H2Core::Timeline &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // H2Core::Timeline::TempoMarker file:core/Timeline.h line:52
			auto & enclosing_class = cl;
			pybind11::class_<H2Core::Timeline::TempoMarker, std::shared_ptr<H2Core::Timeline::TempoMarker>> cl(enclosing_class, "TempoMarker", "TempoMarker specifies a change in speed during the\n Song.");
			cl.def( pybind11::init( [](){ return new H2Core::Timeline::TempoMarker(); } ) );
			cl.def_readwrite("nBar", &H2Core::Timeline::TempoMarker::nBar);
			cl.def_readwrite("fBpm", &H2Core::Timeline::TempoMarker::fBpm);
		}

		{ // H2Core::Timeline::Tag file:core/Timeline.h line:62
			auto & enclosing_class = cl;
			pybind11::class_<H2Core::Timeline::Tag, std::shared_ptr<H2Core::Timeline::Tag>> cl(enclosing_class, "Tag", "Tag specifies a note added to a certain position in the\n			Song.");
			cl.def( pybind11::init( [](){ return new H2Core::Timeline::Tag(); } ) );
			cl.def( pybind11::init( [](H2Core::Timeline::Tag const &o){ return new H2Core::Timeline::Tag(o); } ) );
			cl.def_readwrite("nBar", &H2Core::Timeline::Tag::nBar);
			cl.def_readwrite("sTag", &H2Core::Timeline::Tag::sTag);
			cl.def("assign", (struct H2Core::Timeline::Tag & (H2Core::Timeline::Tag::*)(const struct H2Core::Timeline::Tag &)) &H2Core::Timeline::Tag::operator=, "C++: H2Core::Timeline::Tag::operator=(const struct H2Core::Timeline::Tag &) --> struct H2Core::Timeline::Tag &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		}

	}
	std::cout << "B704_[H2Core::MidiMessage] ";
	{ // H2Core::MidiMessage file: line:34
		pybind11::class_<H2Core::MidiMessage, std::shared_ptr<H2Core::MidiMessage>> cl(M("H2Core"), "MidiMessage", "");
		cl.def( pybind11::init( [](){ return new H2Core::MidiMessage(); } ) );
		cl.def( pybind11::init( [](H2Core::MidiMessage const &o){ return new H2Core::MidiMessage(o); } ) );

		pybind11::enum_<H2Core::MidiMessage::MidiMessageType>(cl, "MidiMessageType", pybind11::arithmetic(), "")
			.value("UNKNOWN", H2Core::MidiMessage::UNKNOWN)
			.value("SYSEX", H2Core::MidiMessage::SYSEX)
			.value("NOTE_ON", H2Core::MidiMessage::NOTE_ON)
			.value("NOTE_OFF", H2Core::MidiMessage::NOTE_OFF)
			.value("POLYPHONIC_KEY_PRESSURE", H2Core::MidiMessage::POLYPHONIC_KEY_PRESSURE)
			.value("CONTROL_CHANGE", H2Core::MidiMessage::CONTROL_CHANGE)
			.value("PROGRAM_CHANGE", H2Core::MidiMessage::PROGRAM_CHANGE)
			.value("CHANNEL_PRESSURE", H2Core::MidiMessage::CHANNEL_PRESSURE)
			.value("PITCH_WHEEL", H2Core::MidiMessage::PITCH_WHEEL)
			.value("SYSTEM_EXCLUSIVE", H2Core::MidiMessage::SYSTEM_EXCLUSIVE)
			.value("START", H2Core::MidiMessage::START)
			.value("CONTINUE", H2Core::MidiMessage::CONTINUE)
			.value("STOP", H2Core::MidiMessage::STOP)
			.value("SONG_POS", H2Core::MidiMessage::SONG_POS)
			.value("QUARTER_FRAME", H2Core::MidiMessage::QUARTER_FRAME)
			.export_values();

		cl.def_readwrite("m_type", &H2Core::MidiMessage::m_type);
		cl.def_readwrite("m_nData1", &H2Core::MidiMessage::m_nData1);
		cl.def_readwrite("m_nData2", &H2Core::MidiMessage::m_nData2);
		cl.def_readwrite("m_nChannel", &H2Core::MidiMessage::m_nChannel);
		cl.def_readwrite("m_sysexData", &H2Core::MidiMessage::m_sysexData);
		cl.def("assign", (class H2Core::MidiMessage & (H2Core::MidiMessage::*)(const class H2Core::MidiMessage &)) &H2Core::MidiMessage::operator=, "C++: H2Core::MidiMessage::operator=(const class H2Core::MidiMessage &) --> class H2Core::MidiMessage &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B705_[H2Core::MidiPortInfo] ";
	{ // H2Core::MidiPortInfo file: line:69
		pybind11::class_<H2Core::MidiPortInfo, std::shared_ptr<H2Core::MidiPortInfo>> cl(M("H2Core"), "MidiPortInfo", "");
		cl.def( pybind11::init( [](){ return new H2Core::MidiPortInfo(); } ) );
		cl.def( pybind11::init( [](H2Core::MidiPortInfo const &o){ return new H2Core::MidiPortInfo(o); } ) );
		cl.def_readwrite("m_sName", &H2Core::MidiPortInfo::m_sName);
		cl.def_readwrite("m_nClient", &H2Core::MidiPortInfo::m_nClient);
		cl.def_readwrite("m_nPort", &H2Core::MidiPortInfo::m_nPort);
		cl.def("assign", (class H2Core::MidiPortInfo & (H2Core::MidiPortInfo::*)(const class H2Core::MidiPortInfo &)) &H2Core::MidiPortInfo::operator=, "C++: H2Core::MidiPortInfo::operator=(const class H2Core::MidiPortInfo &) --> class H2Core::MidiPortInfo &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B706_[H2Core::MidiInput] ";
	{ // H2Core::MidiInput file:core/IO/MidiInput.h line:37
		pybind11::class_<H2Core::MidiInput, std::shared_ptr<H2Core::MidiInput>, H2Core::Object> cl(M("H2Core"), "MidiInput", "MIDI input base class");
		cl.def("open", (void (H2Core::MidiInput::*)()) &H2Core::MidiInput::open, "C++: H2Core::MidiInput::open() --> void");
		cl.def("close", (void (H2Core::MidiInput::*)()) &H2Core::MidiInput::close, "C++: H2Core::MidiInput::close() --> void");
		cl.def("setActive", (void (H2Core::MidiInput::*)(bool)) &H2Core::MidiInput::setActive, "C++: H2Core::MidiInput::setActive(bool) --> void", pybind11::arg("isActive"));
		cl.def("handleMidiMessage", (void (H2Core::MidiInput::*)(const class H2Core::MidiMessage &)) &H2Core::MidiInput::handleMidiMessage, "C++: H2Core::MidiInput::handleMidiMessage(const class H2Core::MidiMessage &) --> void", pybind11::arg("msg"));
		cl.def("handleSysexMessage", (void (H2Core::MidiInput::*)(const class H2Core::MidiMessage &)) &H2Core::MidiInput::handleSysexMessage, "C++: H2Core::MidiInput::handleSysexMessage(const class H2Core::MidiMessage &) --> void", pybind11::arg("msg"));
		cl.def("handleControlChangeMessage", (void (H2Core::MidiInput::*)(const class H2Core::MidiMessage &)) &H2Core::MidiInput::handleControlChangeMessage, "C++: H2Core::MidiInput::handleControlChangeMessage(const class H2Core::MidiMessage &) --> void", pybind11::arg("msg"));
		cl.def("handleProgramChangeMessage", (void (H2Core::MidiInput::*)(const class H2Core::MidiMessage &)) &H2Core::MidiInput::handleProgramChangeMessage, "C++: H2Core::MidiInput::handleProgramChangeMessage(const class H2Core::MidiMessage &) --> void", pybind11::arg("msg"));
		cl.def("handlePolyphonicKeyPressureMessage", (void (H2Core::MidiInput::*)(const class H2Core::MidiMessage &)) &H2Core::MidiInput::handlePolyphonicKeyPressureMessage, "C++: H2Core::MidiInput::handlePolyphonicKeyPressureMessage(const class H2Core::MidiMessage &) --> void", pybind11::arg("msg"));
		cl.def("assign", (class H2Core::MidiInput & (H2Core::MidiInput::*)(const class H2Core::MidiInput &)) &H2Core::MidiInput::operator=, "C++: H2Core::MidiInput::operator=(const class H2Core::MidiInput &) --> class H2Core::MidiInput &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B707_[H2Core::MidiOutput] ";
	{ // H2Core::MidiOutput file:core/IO/MidiOutput.h line:39
		pybind11::class_<H2Core::MidiOutput, std::shared_ptr<H2Core::MidiOutput>, H2Core::Object> cl(M("H2Core"), "MidiOutput", "MIDI input base class");
		cl.def("handleQueueNote", (void (H2Core::MidiOutput::*)(class H2Core::Note *)) &H2Core::MidiOutput::handleQueueNote, "C++: H2Core::MidiOutput::handleQueueNote(class H2Core::Note *) --> void", pybind11::arg("pNote"));
		cl.def("handleQueueNoteOff", (void (H2Core::MidiOutput::*)(int, int, int)) &H2Core::MidiOutput::handleQueueNoteOff, "C++: H2Core::MidiOutput::handleQueueNoteOff(int, int, int) --> void", pybind11::arg("channel"), pybind11::arg("key"), pybind11::arg("velocity"));
		cl.def("handleQueueAllNoteOff", (void (H2Core::MidiOutput::*)()) &H2Core::MidiOutput::handleQueueAllNoteOff, "C++: H2Core::MidiOutput::handleQueueAllNoteOff() --> void");
		cl.def("handleOutgoingControlChange", (void (H2Core::MidiOutput::*)(int, int, int)) &H2Core::MidiOutput::handleOutgoingControlChange, "C++: H2Core::MidiOutput::handleOutgoingControlChange(int, int, int) --> void", pybind11::arg("param"), pybind11::arg("value"), pybind11::arg("channel"));
		cl.def("assign", (class H2Core::MidiOutput & (H2Core::MidiOutput::*)(const class H2Core::MidiOutput &)) &H2Core::MidiOutput::operator=, "C++: H2Core::MidiOutput::operator=(const class H2Core::MidiOutput &) --> class H2Core::MidiOutput &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
