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
#include <core/Basics/AutomationPath.h> // H2Core::AutomationPath
#include <core/Basics/DrumkitComponent.h> // H2Core::DrumkitComponent
#include <core/Basics/Instrument.h> // H2Core::Instrument
#include <core/Basics/InstrumentList.h> // H2Core::InstrumentList
#include <core/Basics/PatternList.h> // H2Core::PatternList
#include <core/Basics/Song.h> // 
#include <core/Basics/Song.h> // H2Core::Song
#include <core/Basics/Song.h> // H2Core::SongReader
#include <core/CoreActionController.h> // H2Core::CoreActionController
#include <core/EventQueue.h> // H2Core::Event
#include <core/EventQueue.h> // H2Core::EventType
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

// H2Core::SongReader file:core/Basics/Song.h line:557
struct PyCallBack_H2Core_SongReader : public H2Core::SongReader {
	using H2Core::SongReader::SongReader;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SongReader *>(this), "toQString");
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

// H2Core::CoreActionController file:core/CoreActionController.h line:32
struct PyCallBack_H2Core_CoreActionController : public H2Core::CoreActionController {
	using H2Core::CoreActionController::CoreActionController;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::CoreActionController *>(this), "toQString");
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

void bind_core_Basics_Song(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B689_[H2Core::SongReader] ";
	{ // H2Core::SongReader file:core/Basics/Song.h line:557
		pybind11::class_<H2Core::SongReader, std::shared_ptr<H2Core::SongReader>, PyCallBack_H2Core_SongReader, H2Core::Object> cl(M("H2Core"), "SongReader", "	Read XML file of a song");
		cl.def( pybind11::init( [](){ return new H2Core::SongReader(); }, [](){ return new PyCallBack_H2Core_SongReader(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_SongReader const &o){ return new PyCallBack_H2Core_SongReader(o); } ) );
		cl.def( pybind11::init( [](H2Core::SongReader const &o){ return new H2Core::SongReader(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::SongReader::class_name, "C++: H2Core::SongReader::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("getPath", (const class QString (H2Core::SongReader::*)(const class QString &) const) &H2Core::SongReader::getPath, "C++: H2Core::SongReader::getPath(const class QString &) const --> const class QString", pybind11::arg("filename"));
		cl.def("readSong", (class H2Core::Song * (H2Core::SongReader::*)(const class QString &)) &H2Core::SongReader::readSong, "C++: H2Core::SongReader::readSong(const class QString &) --> class H2Core::Song *", pybind11::return_value_policy::automatic, pybind11::arg("filename"));
		cl.def("assign", (class H2Core::SongReader & (H2Core::SongReader::*)(const class H2Core::SongReader &)) &H2Core::SongReader::operator=, "C++: H2Core::SongReader::operator=(const class H2Core::SongReader &) --> class H2Core::SongReader &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B690_[H2Core::CoreActionController] ";
	{ // H2Core::CoreActionController file:core/CoreActionController.h line:32
		pybind11::class_<H2Core::CoreActionController, std::shared_ptr<H2Core::CoreActionController>, PyCallBack_H2Core_CoreActionController, H2Core::Object> cl(M("H2Core"), "CoreActionController", "");
		cl.def( pybind11::init( [](){ return new H2Core::CoreActionController(); }, [](){ return new PyCallBack_H2Core_CoreActionController(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_CoreActionController const &o){ return new PyCallBack_H2Core_CoreActionController(o); } ) );
		cl.def( pybind11::init( [](H2Core::CoreActionController const &o){ return new H2Core::CoreActionController(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::CoreActionController::class_name, "C++: H2Core::CoreActionController::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("setMasterVolume", (void (H2Core::CoreActionController::*)(float)) &H2Core::CoreActionController::setMasterVolume, "C++: H2Core::CoreActionController::setMasterVolume(float) --> void", pybind11::arg("masterVolumeValue"));
		cl.def("setStripVolume", (void (H2Core::CoreActionController::*)(int, float, bool)) &H2Core::CoreActionController::setStripVolume, "Instrument which to set the volume for.\n \n\n New volume.\n \n\n Whether the corresponding instrument\n should be selected.\n\nC++: H2Core::CoreActionController::setStripVolume(int, float, bool) --> void", pybind11::arg("nStrip"), pybind11::arg("fVolumeValue"), pybind11::arg("bSelectStrip"));
		cl.def("setStripPan", (void (H2Core::CoreActionController::*)(int, float, bool)) &H2Core::CoreActionController::setStripPan, "Instrument which to set the pan for.\n \n\n New pan.\n \n\n Whether the corresponding instrument\n should be selected.\n\nC++: H2Core::CoreActionController::setStripPan(int, float, bool) --> void", pybind11::arg("nStrip"), pybind11::arg("fValue"), pybind11::arg("bSelectStrip"));
		cl.def("setStripPanSym", (void (H2Core::CoreActionController::*)(int, float, bool)) &H2Core::CoreActionController::setStripPanSym, "Instrument which to set the pan for.\n \n\n New pan. range in [-1;1] => symmetric respect to 0\n \n\n Whether the corresponding instrument\n should be selected.\n\nC++: H2Core::CoreActionController::setStripPanSym(int, float, bool) --> void", pybind11::arg("nStrip"), pybind11::arg("fValue"), pybind11::arg("bSelectStrip"));
		cl.def("setMetronomeIsActive", (void (H2Core::CoreActionController::*)(bool)) &H2Core::CoreActionController::setMetronomeIsActive, "C++: H2Core::CoreActionController::setMetronomeIsActive(bool) --> void", pybind11::arg("isActive"));
		cl.def("setMasterIsMuted", (void (H2Core::CoreActionController::*)(bool)) &H2Core::CoreActionController::setMasterIsMuted, "C++: H2Core::CoreActionController::setMasterIsMuted(bool) --> void", pybind11::arg("isMuted"));
		cl.def("setStripIsMuted", (void (H2Core::CoreActionController::*)(int, bool)) &H2Core::CoreActionController::setStripIsMuted, "C++: H2Core::CoreActionController::setStripIsMuted(int, bool) --> void", pybind11::arg("nStrip"), pybind11::arg("isMuted"));
		cl.def("toggleStripIsMuted", (void (H2Core::CoreActionController::*)(int)) &H2Core::CoreActionController::toggleStripIsMuted, "C++: H2Core::CoreActionController::toggleStripIsMuted(int) --> void", pybind11::arg("nStrip"));
		cl.def("setStripIsSoloed", (void (H2Core::CoreActionController::*)(int, bool)) &H2Core::CoreActionController::setStripIsSoloed, "C++: H2Core::CoreActionController::setStripIsSoloed(int, bool) --> void", pybind11::arg("nStrip"), pybind11::arg("isSoloed"));
		cl.def("toggleStripIsSoloed", (void (H2Core::CoreActionController::*)(int)) &H2Core::CoreActionController::toggleStripIsSoloed, "C++: H2Core::CoreActionController::toggleStripIsSoloed(int) --> void", pybind11::arg("nStrip"));
		cl.def("initExternalControlInterfaces", (void (H2Core::CoreActionController::*)()) &H2Core::CoreActionController::initExternalControlInterfaces, "C++: H2Core::CoreActionController::initExternalControlInterfaces() --> void");
		cl.def("handleOutgoingControlChange", (void (H2Core::CoreActionController::*)(int, int)) &H2Core::CoreActionController::handleOutgoingControlChange, "C++: H2Core::CoreActionController::handleOutgoingControlChange(int, int) --> void", pybind11::arg("param"), pybind11::arg("value"));
		cl.def("newSong", (bool (H2Core::CoreActionController::*)(const class QString &)) &H2Core::CoreActionController::newSong, "Create an empty #Song, which will be stored in \n\n This will be done immediately and without saving\n the current #Song. All unsaved changes will be lost! In\n addition, the new song won't be saved by this function. You\n can do so using saveSong().\n\n The intended use of this function for session\n management. Therefore, the function will *not* store the\n provided  in Preferences::m_lastSongFilename and\n Hydrogen won't resume with the corresponding song on\n restarting.\n\n \n Absolute path to the .h2song file to be\n    opened.\n \n\n true on success\n\nC++: H2Core::CoreActionController::newSong(const class QString &) --> bool", pybind11::arg("songPath"));
		cl.def("openSong", (bool (H2Core::CoreActionController::*)(const class QString &)) &H2Core::CoreActionController::openSong, "Opens the #Song specified in \n\n This will be done immediately and without saving\n the current #Song. All unsaved changes will be lost!\n\n The intended use of this function for session\n management. Therefore, the function will *not* store the\n provided  in Preferences::m_lastSongFilename and\n Hydrogen won't resume with the corresponding song on\n restarting.\n\n \n Absolute path to the .h2song file to be\n    opened.\n \n\n true on success\n\nC++: H2Core::CoreActionController::openSong(const class QString &) --> bool", pybind11::arg("songPath"));
		cl.def("openSong", (bool (H2Core::CoreActionController::*)(class H2Core::Song *)) &H2Core::CoreActionController::openSong, "Opens the #Song specified in \n\n This will be done immediately and without saving\n the current #Song. All unsaved changes will be lost!\n\n The intended use of this function for session\n management. Therefore, the function will *not* store the\n provided  in Preferences::m_lastSongFilename and\n Hydrogen won't resume with the corresponding song on\n restarting.\n\n \n New Song.\n \n\n true on success\n\nC++: H2Core::CoreActionController::openSong(class H2Core::Song *) --> bool", pybind11::arg("pSong"));
		cl.def("saveSong", (bool (H2Core::CoreActionController::*)()) &H2Core::CoreActionController::saveSong, "Saves the current #Song.\n\n \n true on success\n\nC++: H2Core::CoreActionController::saveSong() --> bool");
		cl.def("saveSongAs", (bool (H2Core::CoreActionController::*)(const class QString &)) &H2Core::CoreActionController::saveSongAs, "Saves the current #Song to the path provided in \n\n The intended use of this function for session\n management. Therefore, the function will *not* store the\n provided  in Preferences::m_lastSongFilename and\n Hydrogen won't resume with the corresponding song on\n restarting.\n\n \n Absolute path to the file to store the\n   current #Song in.\n \n\n true on success\n\nC++: H2Core::CoreActionController::saveSongAs(const class QString &) --> bool", pybind11::arg("songPath"));
		cl.def("savePreferences", (bool (H2Core::CoreActionController::*)()) &H2Core::CoreActionController::savePreferences, "Saves the current state of the #Preferences.\n\n \n true on success\n\nC++: H2Core::CoreActionController::savePreferences() --> bool");
		cl.def("quit", (bool (H2Core::CoreActionController::*)()) &H2Core::CoreActionController::quit, "Triggers the shutdown of Hydrogen.\n\n This will be done immediately and without saving the\n current #Song. All unsaved changes will be lost!\n\n The shutdown will be triggered in both the CLI and the GUI\n via the #H2Core::EVENT_QUIT event.\n\n \n true on success\n\nC++: H2Core::CoreActionController::quit() --> bool");
		cl.def("activateTimeline", (bool (H2Core::CoreActionController::*)(bool)) &H2Core::CoreActionController::activateTimeline, "(De)activates the usage of the Timeline.\n\n Note that this function will fail in the presence of the\n Jack audio driver and an external timebase master (see Hydrogen::getJackTimebaseState()).\n\n \n If true - activate or if false -\n deactivate.\n\n \n bool true on success\n\nC++: H2Core::CoreActionController::activateTimeline(bool) --> bool", pybind11::arg("bActivate"));
		cl.def("addTempoMarker", (bool (H2Core::CoreActionController::*)(int, float)) &H2Core::CoreActionController::addTempoMarker, "Adds a tempo marker to the Timeline.\n\n \n Location of the tempo marker in bars.\n \n\n Speed associated with the tempo marker.\n\n \n bool true on success\n\nC++: H2Core::CoreActionController::addTempoMarker(int, float) --> bool", pybind11::arg("nPosition"), pybind11::arg("fBpm"));
		cl.def("deleteTempoMarker", (bool (H2Core::CoreActionController::*)(int)) &H2Core::CoreActionController::deleteTempoMarker, "Delete a tempo marker from the Timeline.\n\n If no Tempo marker is present at  the function\n will return true as well.\n\n \n Location of the tempo marker in bars.\n\n \n bool true on success\n\nC++: H2Core::CoreActionController::deleteTempoMarker(int) --> bool", pybind11::arg("nPosition"));
		cl.def("activateJackTransport", (bool (H2Core::CoreActionController::*)(bool)) &H2Core::CoreActionController::activateJackTransport, "(De)activates the usage of Jack transport.\n\n Note that this function will fail if Jack is not used as\n audio driver.\n\n \n If true - activate or if false -\n deactivate.\n\n \n bool true on success\n\nC++: H2Core::CoreActionController::activateJackTransport(bool) --> bool", pybind11::arg("bActivate"));
		cl.def("activateJackTimebaseMaster", (bool (H2Core::CoreActionController::*)(bool)) &H2Core::CoreActionController::activateJackTimebaseMaster, "(De)activates the usage of Jack timebase master.\n\n Note that this function will fail if Jack is not used as\n audio driver.\n\n \n If true - activate or if false -\n deactivate.\n\n \n bool true on success\n\nC++: H2Core::CoreActionController::activateJackTimebaseMaster(bool) --> bool", pybind11::arg("bActivate"));
		cl.def("activateSongMode", (bool (H2Core::CoreActionController::*)(bool, bool)) &H2Core::CoreActionController::activateSongMode, "Switches between Song and Pattern mode of playback.\n\n \n If true - activates Song mode or if false -\n activates Pattern mode.\n \n\n Setting this variable to true is\n intended for its use as a batch function from within\n Hydrogen's core, which will inform the GUI via an Event\n about the change of mode. When used from the GUI itself,\n this parameter has to be set to false.\n\n \n bool true on success\n\nC++: H2Core::CoreActionController::activateSongMode(bool, bool) --> bool", pybind11::arg("bActivate"), pybind11::arg("bTriggerEvent"));
		cl.def("activateLoopMode", (bool (H2Core::CoreActionController::*)(bool, bool)) &H2Core::CoreActionController::activateLoopMode, "Toggle loop mode of playback.\n\n \n If true - activates loop mode.\n \n\n Setting this variable to true is\n intended for its use as a batch function from within\n Hydrogen's core, which will inform the GUI via an Event\n about the change of mode. When used from the GUI itself,\n this parameter has to be set to false.\n\n \n bool true on success\n\nC++: H2Core::CoreActionController::activateLoopMode(bool, bool) --> bool", pybind11::arg("bActivate"), pybind11::arg("bTriggerEvent"));
		cl.def("relocate", (bool (H2Core::CoreActionController::*)(int)) &H2Core::CoreActionController::relocate, "Relocates transport to the beginning of a particular\n Pattern.\n\n \n Position of the Song provided as the\n index of a particular pattern group (starting at zero).\n\n \n bool true on success\n\nC++: H2Core::CoreActionController::relocate(int) --> bool", pybind11::arg("nPatternGroup"));
		cl.def("isSongPathValid", (bool (H2Core::CoreActionController::*)(const class QString &)) &H2Core::CoreActionController::isSongPathValid, "Checks the path of the .h2song provided via OSC.\n\n It will be checked whether \n - is absolute\n - has the '.h2song' suffix\n - is writable (if it exists)\n\n \n Absolute path to an .h2song file.\n \n\n true - if valid.\n\nC++: H2Core::CoreActionController::isSongPathValid(const class QString &) --> bool", pybind11::arg("songPath"));
	}
	std::cout << "B691_[H2Core::EventType] ";
	// H2Core::EventType file:core/EventQueue.h line:39
	pybind11::enum_<H2Core::EventType>(M("H2Core"), "EventType", pybind11::arithmetic(), "Basic types of communication between the core part of Hydrogen and\n    its GUI.")
		.value("EVENT_NONE", H2Core::EVENT_NONE)
		.value("EVENT_STATE", H2Core::EVENT_STATE)
		.value("EVENT_PATTERN_CHANGED", H2Core::EVENT_PATTERN_CHANGED)
		.value("EVENT_PATTERN_MODIFIED", H2Core::EVENT_PATTERN_MODIFIED)
		.value("EVENT_SELECTED_PATTERN_CHANGED", H2Core::EVENT_SELECTED_PATTERN_CHANGED)
		.value("EVENT_SELECTED_INSTRUMENT_CHANGED", H2Core::EVENT_SELECTED_INSTRUMENT_CHANGED)
		.value("EVENT_PARAMETERS_INSTRUMENT_CHANGED", H2Core::EVENT_PARAMETERS_INSTRUMENT_CHANGED)
		.value("EVENT_MIDI_ACTIVITY", H2Core::EVENT_MIDI_ACTIVITY)
		.value("EVENT_XRUN", H2Core::EVENT_XRUN)
		.value("EVENT_NOTEON", H2Core::EVENT_NOTEON)
		.value("EVENT_ERROR", H2Core::EVENT_ERROR)
		.value("EVENT_METRONOME", H2Core::EVENT_METRONOME)
		.value("EVENT_RECALCULATERUBBERBAND", H2Core::EVENT_RECALCULATERUBBERBAND)
		.value("EVENT_PROGRESS", H2Core::EVENT_PROGRESS)
		.value("EVENT_JACK_SESSION", H2Core::EVENT_JACK_SESSION)
		.value("EVENT_PLAYLIST_LOADSONG", H2Core::EVENT_PLAYLIST_LOADSONG)
		.value("EVENT_UNDO_REDO", H2Core::EVENT_UNDO_REDO)
		.value("EVENT_SONG_MODIFIED", H2Core::EVENT_SONG_MODIFIED)
		.value("EVENT_TEMPO_CHANGED", H2Core::EVENT_TEMPO_CHANGED)
		.value("EVENT_UPDATE_PREFERENCES", H2Core::EVENT_UPDATE_PREFERENCES)
		.value("EVENT_UPDATE_SONG", H2Core::EVENT_UPDATE_SONG)
		.value("EVENT_QUIT", H2Core::EVENT_QUIT)
		.value("EVENT_TIMELINE_ACTIVATION", H2Core::EVENT_TIMELINE_ACTIVATION)
		.value("EVENT_TIMELINE_UPDATE", H2Core::EVENT_TIMELINE_UPDATE)
		.value("EVENT_JACK_TRANSPORT_ACTIVATION", H2Core::EVENT_JACK_TRANSPORT_ACTIVATION)
		.value("EVENT_JACK_TIMEBASE_ACTIVATION", H2Core::EVENT_JACK_TIMEBASE_ACTIVATION)
		.value("EVENT_SONG_MODE_ACTIVATION", H2Core::EVENT_SONG_MODE_ACTIVATION)
		.value("EVENT_LOOP_MODE_ACTIVATION", H2Core::EVENT_LOOP_MODE_ACTIVATION)
		.value("EVENT_ACTION_MODE_CHANGE", H2Core::EVENT_ACTION_MODE_CHANGE)
		.export_values();

;

	std::cout << "B692_[H2Core::Event] ";
	{ // H2Core::Event file:core/EventQueue.h line:162
		pybind11::class_<H2Core::Event, std::shared_ptr<H2Core::Event>> cl(M("H2Core"), "Event", "Basic building block for the communication between the core of\n Hydrogen and its GUI.  The individual Events will be enlisted in\n the EventQueue singleton.");
		cl.def( pybind11::init( [](){ return new H2Core::Event(); } ) );
		cl.def( pybind11::init( [](H2Core::Event const &o){ return new H2Core::Event(o); } ) );
		cl.def_readwrite("type", &H2Core::Event::type);
		cl.def_readwrite("value", &H2Core::Event::value);
		cl.def("assign", (class H2Core::Event & (H2Core::Event::*)(const class H2Core::Event &)) &H2Core::Event::operator=, "C++: H2Core::Event::operator=(const class H2Core::Event &) --> class H2Core::Event &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
