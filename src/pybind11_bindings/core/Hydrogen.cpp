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
#include <bits/types/struct_timeval.h> // timeval
#include <chrono> // std::chrono::duration
#include <core/AudioEngine.h> // H2Core::AudioEngine
#include <core/Basics/Adsr.h> // H2Core::ADSR
#include <core/Basics/AutomationPath.h> // H2Core::AutomationPath
#include <core/Basics/Drumkit.h> // H2Core::Drumkit
#include <core/Basics/DrumkitComponent.h> // H2Core::DrumkitComponent
#include <core/Basics/Instrument.h> // H2Core::Instrument
#include <core/Basics/InstrumentList.h> // H2Core::InstrumentList
#include <core/Basics/Note.h> // 
#include <core/Basics/Note.h> // H2Core::Note
#include <core/Basics/Note.h> // H2Core::SelectedLayerInfo
#include <core/Basics/Pattern.h> // H2Core::Pattern
#include <core/Basics/PatternList.h> // H2Core::PatternList
#include <core/Basics/Song.h> // 
#include <core/Basics/Song.h> // H2Core::Song
#include <core/CoreActionController.h> // H2Core::CoreActionController
#include <core/Helpers/Filesystem.h> // 
#include <core/Helpers/Xml.h> // H2Core::XMLNode
#include <core/Hydrogen.h> // 
#include <core/Hydrogen.h> // H2Core::Hydrogen
#include <core/IO/AudioOutput.h> // H2Core::AudioOutput
#include <core/IO/JackAudioDriver.h> // 
#include <core/IO/MidiInput.h> // H2Core::MidiInput
#include <core/IO/MidiOutput.h> // H2Core::MidiOutput
#include <core/Sampler/Sampler.h> // H2Core::Sampler
#include <core/Synth/Synth.h> // H2Core::Synth
#include <core/Timeline.h> // H2Core::Timeline
#include <iostream> // --trace
#include <iterator> // __gnu_cxx::__normal_iterator
#include <iterator> // std::reverse_iterator
#include <memory> // std::allocator
#include <memory> // std::shared_ptr
#include <ratio> // std::ratio
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

// H2Core::Hydrogen file:core/Hydrogen.h line:51
struct PyCallBack_H2Core_Hydrogen : public H2Core::Hydrogen {
	using H2Core::Hydrogen::Hydrogen;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::Hydrogen *>(this), "toQString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return Hydrogen::toQString(a0, a1);
	}
};

void bind_core_Hydrogen(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B710_[H2Core::Hydrogen] ";
	{ // H2Core::Hydrogen file:core/Hydrogen.h line:51
		pybind11::class_<H2Core::Hydrogen, std::shared_ptr<H2Core::Hydrogen>, PyCallBack_H2Core_Hydrogen, H2Core::Object> cl(M("H2Core"), "Hydrogen", "Hydrogen Audio Engine.");
		cl.def( pybind11::init( [](PyCallBack_H2Core_Hydrogen const &o){ return new PyCallBack_H2Core_Hydrogen(o); } ) );
		cl.def( pybind11::init( [](H2Core::Hydrogen const &o){ return new H2Core::Hydrogen(o); } ) );

		pybind11::enum_<H2Core::Hydrogen::ErrorMessages>(cl, "ErrorMessages", pybind11::arithmetic(), "")
			.value("UNKNOWN_DRIVER", H2Core::Hydrogen::UNKNOWN_DRIVER)
			.value("ERROR_STARTING_DRIVER", H2Core::Hydrogen::ERROR_STARTING_DRIVER)
			.value("JACK_SERVER_SHUTDOWN", H2Core::Hydrogen::JACK_SERVER_SHUTDOWN)
			.value("JACK_CANNOT_ACTIVATE_CLIENT", H2Core::Hydrogen::JACK_CANNOT_ACTIVATE_CLIENT)
			.value("JACK_CANNOT_CONNECT_OUTPUT_PORT", H2Core::Hydrogen::JACK_CANNOT_CONNECT_OUTPUT_PORT)
			.value("JACK_CANNOT_CLOSE_CLIENT", H2Core::Hydrogen::JACK_CANNOT_CLOSE_CLIENT)
			.value("JACK_ERROR_IN_PORT_REGISTER", H2Core::Hydrogen::JACK_ERROR_IN_PORT_REGISTER)
			.value("OSC_CANNOT_CONNECT_TO_PORT", H2Core::Hydrogen::OSC_CANNOT_CONNECT_TO_PORT)
			.export_values();


		pybind11::enum_<H2Core::Hydrogen::GUIState>(cl, "GUIState", "Specifies the state of the Qt GUI")
			.value("notReady", H2Core::Hydrogen::GUIState::notReady)
			.value("unavailable", H2Core::Hydrogen::GUIState::unavailable)
			.value("ready", H2Core::Hydrogen::GUIState::ready);

		cl.def_readwrite("lastMidiEvent", &H2Core::Hydrogen::lastMidiEvent);
		cl.def_readwrite("lastMidiEventParameter", &H2Core::Hydrogen::lastMidiEventParameter);
		cl.def_readwrite("m_nMaxTimeHumanize", &H2Core::Hydrogen::m_nMaxTimeHumanize);
		cl.def_static("class_name", (const char * (*)()) &H2Core::Hydrogen::class_name, "C++: H2Core::Hydrogen::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def_static("create_instance", (void (*)()) &H2Core::Hydrogen::create_instance, "Creates all the instances used within Hydrogen in the right\n order. \n\n -# H2Core::Logger::create_instance()\n -# MidiMap::create_instance()\n -# Preferences::create_instance()\n -# EventQueue::create_instance()\n -# MidiActionManager::create_instance()\n\n If #H2CORE_HAVE_OSC was set during compilation, the\n following instances will be created as well.\n\n -# NsmClient::create_instance()\n -# OscServer::create_instance() using\n    Preferences::get_instance() as input\n\n If all instances are created and the actual Hydrogen\n instance #__instance is still 0, it will be properly\n constructed via Hydrogen().\n\n The AudioEngine::create_instance(),\n Effects::create_instance(), and Playlist::create_instance()\n functions will be called from within audioEngine_init().\n\nC++: H2Core::Hydrogen::create_instance() --> void");
		cl.def_static("get_instance", []() -> H2Core::Hydrogen * { return H2Core::Hydrogen::get_instance(); }, "", pybind11::return_value_policy::automatic);
		cl.def_static("get_instance", [](bool const & a0) -> H2Core::Hydrogen * { return H2Core::Hydrogen::get_instance(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("nullok"));
		cl.def_static("get_instance", (class H2Core::Hydrogen * (*)(bool, bool)) &H2Core::Hydrogen::get_instance, "Returns the current Hydrogen instance #__instance.\n\nC++: H2Core::Hydrogen::get_instance(bool, bool) --> class H2Core::Hydrogen *", pybind11::return_value_policy::automatic, pybind11::arg("nullok"), pybind11::arg("create"));
		cl.def("getAudioEngine", (class H2Core::AudioEngine * (H2Core::Hydrogen::*)() const) &H2Core::Hydrogen::getAudioEngine, "C++: H2Core::Hydrogen::getAudioEngine() const --> class H2Core::AudioEngine *", pybind11::return_value_policy::automatic);
		cl.def("sequencer_play", (void (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::sequencer_play, "Start the internal sequencer\n\nC++: H2Core::Hydrogen::sequencer_play() --> void");
		cl.def("sequencer_stop", (void (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::sequencer_stop, "Stop the internal sequencer\n\nC++: H2Core::Hydrogen::sequencer_stop() --> void");
		cl.def("midi_noteOn", (void (H2Core::Hydrogen::*)(class H2Core::Note *)) &H2Core::Hydrogen::midi_noteOn, "C++: H2Core::Hydrogen::midi_noteOn(class H2Core::Note *) --> void", pybind11::arg("note"));
		cl.def("sequencer_setNextPattern", (void (H2Core::Hydrogen::*)(int)) &H2Core::Hydrogen::sequencer_setNextPattern, "Adding and removing a Pattern from #m_pNextPatterns.\n\n After locking the AudioEngine the function retrieves the\n particular pattern  from the Song::m_pPatternList and\n either deletes it from #m_pNextPatterns if already present or\n add it to the same pattern list if not present yet.\n\n If the Song is not in Song::PATTERN_MODE or  is not\n within the range of Song::m_pPatternList, #m_pNextPatterns will\n be cleared instead.\n\n \n Index of a particular pattern in\n Song::m_pPatternList, which should be added to\n #m_pNextPatterns.\n\nC++: H2Core::Hydrogen::sequencer_setNextPattern(int) --> void", pybind11::arg("pos"));
		cl.def("sequencer_setOnlyNextPattern", (void (H2Core::Hydrogen::*)(int)) &H2Core::Hydrogen::sequencer_setOnlyNextPattern, "Clear #m_pNextPatterns and add one Pattern.\n\n After locking the AudioEngine the function clears\n #m_pNextPatterns, fills it with all currently played one in\n #m_pPlayingPatterns, and appends the particular pattern \n from the Song::m_pPatternList.\n\n If the Song is not in Song::PATTERN_MODE or  is not\n within the range of Song::m_pPatternList, #m_pNextPatterns will\n be just cleared.\n\n \n Index of a particular pattern in\n Song::m_pPatternList, which should be added to\n #m_pNextPatterns.\n\nC++: H2Core::Hydrogen::sequencer_setOnlyNextPattern(int) --> void", pybind11::arg("pos"));
		cl.def("togglePlaysSelected", (void (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::togglePlaysSelected, "Switches playback to focused pattern.\n\n If the current Song is in Song::PATTERN_MODE, the AudioEngine\n will be locked and Preferences::m_bPatternModePlaysSelected\n negated. If the latter was true before calling this function,\n #m_pPlayingPatterns will be cleared and replaced by the\n Pattern indexed with #m_nSelectedPatternNumber.\n\n This function will be called either by MainForm::eventFilter()\n when pressing Qt::Key_L or by\n SongEditorPanel::modeActionBtnPressed().\n\nC++: H2Core::Hydrogen::togglePlaysSelected() --> void");
		cl.def("getSong", (class H2Core::Song * (H2Core::Hydrogen::*)() const) &H2Core::Hydrogen::getSong, "Get the current song.\n \n\n #__song\n\nC++: H2Core::Hydrogen::getSong() const --> class H2Core::Song *", pybind11::return_value_policy::automatic);
		cl.def("setSong", (void (H2Core::Hydrogen::*)(class H2Core::Song *)) &H2Core::Hydrogen::setSong, "Sets the current song #__song to \n \n\n Pointer to the new Song object.\n\nC++: H2Core::Hydrogen::setSong(class H2Core::Song *) --> void", pybind11::arg("newSong"));
		cl.def("removeSong", (void (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::removeSong, "C++: H2Core::Hydrogen::removeSong() --> void");
		cl.def("addRealtimeNote", [](H2Core::Hydrogen &o, int const & a0, float const & a1) -> void { return o.addRealtimeNote(a0, a1); }, "", pybind11::arg("instrument"), pybind11::arg("velocity"));
		cl.def("addRealtimeNote", [](H2Core::Hydrogen &o, int const & a0, float const & a1, float const & a2) -> void { return o.addRealtimeNote(a0, a1, a2); }, "", pybind11::arg("instrument"), pybind11::arg("velocity"), pybind11::arg("fPan"));
		cl.def("addRealtimeNote", [](H2Core::Hydrogen &o, int const & a0, float const & a1, float const & a2, float const & a3) -> void { return o.addRealtimeNote(a0, a1, a2, a3); }, "", pybind11::arg("instrument"), pybind11::arg("velocity"), pybind11::arg("fPan"), pybind11::arg("pitch"));
		cl.def("addRealtimeNote", [](H2Core::Hydrogen &o, int const & a0, float const & a1, float const & a2, float const & a3, bool const & a4) -> void { return o.addRealtimeNote(a0, a1, a2, a3, a4); }, "", pybind11::arg("instrument"), pybind11::arg("velocity"), pybind11::arg("fPan"), pybind11::arg("pitch"), pybind11::arg("noteoff"));
		cl.def("addRealtimeNote", [](H2Core::Hydrogen &o, int const & a0, float const & a1, float const & a2, float const & a3, bool const & a4, bool const & a5) -> void { return o.addRealtimeNote(a0, a1, a2, a3, a4, a5); }, "", pybind11::arg("instrument"), pybind11::arg("velocity"), pybind11::arg("fPan"), pybind11::arg("pitch"), pybind11::arg("noteoff"), pybind11::arg("forcePlay"));
		cl.def("addRealtimeNote", (void (H2Core::Hydrogen::*)(int, float, float, float, bool, bool, int)) &H2Core::Hydrogen::addRealtimeNote, "C++: H2Core::Hydrogen::addRealtimeNote(int, float, float, float, bool, bool, int) --> void", pybind11::arg("instrument"), pybind11::arg("velocity"), pybind11::arg("fPan"), pybind11::arg("pitch"), pybind11::arg("noteoff"), pybind11::arg("forcePlay"), pybind11::arg("msg1"));
		cl.def("getTickPosition", (unsigned long (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::getTickPosition, "#m_nPatternTickPosition \n\nC++: H2Core::Hydrogen::getTickPosition() --> unsigned long");
		cl.def("getRealtimeTickPosition", (unsigned long (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::getRealtimeTickPosition, "Keep track of the tick position in realtime.\n\n Firstly, it gets the current transport position in frames\n #m_nRealtimeFrames and converts it into ticks using\n TransportInfo::m_fTickSize. Afterwards, it accesses how\n much time passed since the last update of\n #m_currentTickTime, converts the time difference +\n AudioOutput::getBufferSize()/ AudioOutput::getSampleRate()\n in frames, and adds the result to the first value to\n support keyboard and MIDI events as well.\n\n \n Current position in ticks.\n\nC++: H2Core::Hydrogen::getRealtimeTickPosition() --> unsigned long");
		cl.def("getTotalFrames", (unsigned long (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::getTotalFrames, "C++: H2Core::Hydrogen::getTotalFrames() --> unsigned long");
		cl.def("setRealtimeFrames", (void (H2Core::Hydrogen::*)(unsigned long)) &H2Core::Hydrogen::setRealtimeFrames, "Sets #m_nRealtimeFrames\n \n\n Current transport realtime position\n\nC++: H2Core::Hydrogen::setRealtimeFrames(unsigned long) --> void", pybind11::arg("frames"));
		cl.def("getRealtimeFrames", (unsigned long (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::getRealtimeFrames, "Returns the current realtime transport position\n TransportInfo::m_nFrames.\n \n\n #m_nRealtimeFrames \n\nC++: H2Core::Hydrogen::getRealtimeFrames() --> unsigned long");
		cl.def("getCurrentPatternList", (class H2Core::PatternList * (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::getCurrentPatternList, "#m_pPlayingPatterns\n\nC++: H2Core::Hydrogen::getCurrentPatternList() --> class H2Core::PatternList *", pybind11::return_value_policy::automatic);
		cl.def("setCurrentPatternList", (void (H2Core::Hydrogen::*)(class H2Core::PatternList *)) &H2Core::Hydrogen::setCurrentPatternList, "Sets #m_pPlayingPatterns.\n\n Before setting the variable it first locks the AudioEngine. In\n addition, it also pushes the Event #EVENT_PATTERN_CHANGED with\n the value -1 to the EventQueue.\n\n \n Sets #m_pPlayingPatterns.\n\nC++: H2Core::Hydrogen::setCurrentPatternList(class H2Core::PatternList *) --> void", pybind11::arg("pPatternList"));
		cl.def("getNextPatterns", (class H2Core::PatternList * (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::getNextPatterns, "#m_pNextPatterns\n\nC++: H2Core::Hydrogen::getNextPatterns() --> class H2Core::PatternList *", pybind11::return_value_policy::automatic);
		cl.def("getPatternPos", (int (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::getPatternPos, "Get the position of the current Pattern in the Song.\n \n\n #m_nSongPos \n\nC++: H2Core::Hydrogen::getPatternPos() --> int");
		cl.def("setPatternPos", (void (H2Core::Hydrogen::*)(int)) &H2Core::Hydrogen::setPatternPos, "Relocate the position to another Pattern in the Song.\n\n The position of a Pattern in frames (see\n TransportInfo::m_nFrames for details) will be determined by\n retrieving the tick number the Pattern is located at using\n getTickForPosition() and multiplying it with\n TransportInfo::m_fTickSize. The resulting value will be\n used by the AudioOutput::locate() function of your audio\n driver to relocate the playback position.\n\n If #m_audioEngineState is not #STATE_PLAYING, the variables\n #m_nSongPos and #m_nPatternTickPosition will be set to \n and 0 right away.\n\n \n Position of the Pattern to relocate at. All\n   values smaller than -1 will be set to -1, which marks the\n   beginning of the Song.\n\nC++: H2Core::Hydrogen::setPatternPos(int) --> void", pybind11::arg("pos"));
		cl.def("getPosForTick", (int (H2Core::Hydrogen::*)(unsigned long, int *)) &H2Core::Hydrogen::getPosForTick, "Returns the pattern number corresponding to the tick\n position \n\n Wrapper around function findPatternInTick() (globally defined\n in hydrogen.cpp).\n\n \n Position in ticks.\n \n\n Pointer to an int the starting\n position (in ticks) of the corresponding pattern will be\n written to.\n\n \n \n - __0__ : if the Song isn't specified yet.\n - the output of the findPatternInTick() function called\n   with  and Song::getIsLoopEnabled() as input\n   arguments.\n\nC++: H2Core::Hydrogen::getPosForTick(unsigned long, int *) --> int", pybind11::arg("TickPos"), pybind11::arg("nPatternStartTick"));
		cl.def("resetPatternStartTick", (void (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::resetPatternStartTick, "Move playback in Pattern mode to the beginning of the pattern.\n\n Resetting the global variable #m_nPatternStartTick to -1 if the\n current Song mode is Song::PATTERN_MODE.\n\nC++: H2Core::Hydrogen::resetPatternStartTick() --> void");
		cl.def("getTickForPosition", (long (H2Core::Hydrogen::*)(int)) &H2Core::Hydrogen::getTickForPosition, "Get the total number of ticks passed up to a Pattern at\n position \n\n The function will loop over all and sums up their\n Pattern::__length. If one of the Pattern is NULL or no\n Pattern is present one of the PatternList, #MAX_NOTES will\n be added instead.\n\n The driver should be LOCKED when calling this!\n\n \n Position of the Pattern in the\n   Song::__pattern_group_sequence.\n \n\n\n  - -1 : if  is bigger than the number of patterns in\n   the Song and Song::getIsLoopEnabled() is set to false or\n   no Patterns could be found at all.\n  - >= 0 : the total number of ticks passed.\n\nC++: H2Core::Hydrogen::getTickForPosition(int) --> long", pybind11::arg("pos"));
		cl.def("restartDrivers", (void (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::restartDrivers, "C++: H2Core::Hydrogen::restartDrivers() --> void");
		cl.def("getAudioOutput", (class H2Core::AudioOutput * (H2Core::Hydrogen::*)() const) &H2Core::Hydrogen::getAudioOutput, "C++: H2Core::Hydrogen::getAudioOutput() const --> class H2Core::AudioOutput *", pybind11::return_value_policy::automatic);
		cl.def("getMidiInput", (class H2Core::MidiInput * (H2Core::Hydrogen::*)() const) &H2Core::Hydrogen::getMidiInput, "C++: H2Core::Hydrogen::getMidiInput() const --> class H2Core::MidiInput *", pybind11::return_value_policy::automatic);
		cl.def("getMidiOutput", (class H2Core::MidiOutput * (H2Core::Hydrogen::*)() const) &H2Core::Hydrogen::getMidiOutput, "C++: H2Core::Hydrogen::getMidiOutput() const --> class H2Core::MidiOutput *", pybind11::return_value_policy::automatic);
		cl.def("getState", (int (H2Core::Hydrogen::*)() const) &H2Core::Hydrogen::getState, "Returns the current state of the audio engine.\n \n\n #m_audioEngineState\n\nC++: H2Core::Hydrogen::getState() const --> int");
		cl.def("loadDrumkit", (int (H2Core::Hydrogen::*)(class H2Core::Drumkit *)) &H2Core::Hydrogen::loadDrumkit, "Wrapper around loadDrumkit( Drumkit, bool ) with the\n			conditional argument set to true.\n\n \n 0 In case something unexpected happens, it will be\n   indicated with #ERRORLOG messages.\n\nC++: H2Core::Hydrogen::loadDrumkit(class H2Core::Drumkit *) --> int", pybind11::arg("pDrumkitInfo"));
		cl.def("loadDrumkit", (int (H2Core::Hydrogen::*)(class H2Core::Drumkit *, bool)) &H2Core::Hydrogen::loadDrumkit, "Loads the H2Core::Drumkit provided in  into\n the current session.\n\n When under session management (see\n NsmClient::m_bUnderSessionManagement) the function will\n create a symlink to the loaded H2Core::Drumkit using the\n name \"drumkit\" in the folder\n NsmClient::m_sSessionFolderPath.\n\n \n Full-fledged H2Core::Drumkit to load.\n \n\n Argument passed on as second input\n   argument to removeInstrument().\n\n \n 0 In case something unexpected happens, it will be\n   indicated with #ERRORLOG messages.\n\nC++: H2Core::Hydrogen::loadDrumkit(class H2Core::Drumkit *, bool) --> int", pybind11::arg("pDrumkitInfo"), pybind11::arg("conditional"));
		cl.def("removeInstrument", (void (H2Core::Hydrogen::*)(int, bool)) &H2Core::Hydrogen::removeInstrument, "Delete an Instrument. If  is true, and there\n		    are some Pattern that are using this Instrument, it's not\n		    deleted anyway.\n\nC++: H2Core::Hydrogen::removeInstrument(int, bool) --> void", pybind11::arg("instrumentnumber"), pybind11::arg("conditional"));
		cl.def("getCurrentDrumkitName", (const class QString & (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::getCurrentDrumkitName, "m_sCurrentDrumkitName \n\nC++: H2Core::Hydrogen::getCurrentDrumkitName() --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("setCurrentDrumkitName", (void (H2Core::Hydrogen::*)(const class QString &)) &H2Core::Hydrogen::setCurrentDrumkitName, "sets m_sCurrentDrumkitName \n\nC++: H2Core::Hydrogen::setCurrentDrumkitName(const class QString &) --> void", pybind11::arg("sName"));
		cl.def("getCurrentDrumkitLookup", (enum H2Core::Filesystem::Lookup (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::getCurrentDrumkitLookup, "m_currentDrumkitLookup \n\nC++: H2Core::Hydrogen::getCurrentDrumkitLookup() --> enum H2Core::Filesystem::Lookup");
		cl.def("setCurrentDrumkitLookup", (void (H2Core::Hydrogen::*)(enum H2Core::Filesystem::Lookup)) &H2Core::Hydrogen::setCurrentDrumkitLookup, "sets m_currentDrumkitLookup \n\nC++: H2Core::Hydrogen::setCurrentDrumkitLookup(enum H2Core::Filesystem::Lookup) --> void", pybind11::arg("lookup"));
		cl.def("raiseError", (void (H2Core::Hydrogen::*)(unsigned int)) &H2Core::Hydrogen::raiseError, "C++: H2Core::Hydrogen::raiseError(unsigned int) --> void", pybind11::arg("nErrorCode"));
		cl.def("onTapTempoAccelEvent", (void (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::onTapTempoAccelEvent, "C++: H2Core::Hydrogen::onTapTempoAccelEvent() --> void");
		cl.def("setTapTempo", (void (H2Core::Hydrogen::*)(float)) &H2Core::Hydrogen::setTapTempo, "C++: H2Core::Hydrogen::setTapTempo(float) --> void", pybind11::arg("fInterval"));
		cl.def("setBPM", (void (H2Core::Hydrogen::*)(float)) &H2Core::Hydrogen::setBPM, "Updates the speed.\n\n It calls AudioOutput::setBpm() and setNewBpmJTM() with \n as input argument and sets Song::m_fBpm to \n\n This function will be called with the AudioEngine in LOCKED\n state.\n \n\n New speed in beats per minute.\n\nC++: H2Core::Hydrogen::setBPM(float) --> void", pybind11::arg("fBPM"));
		cl.def("restartLadspaFX", (void (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::restartLadspaFX, "C++: H2Core::Hydrogen::restartLadspaFX() --> void");
		cl.def("getSelectedPatternNumber", (int (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::getSelectedPatternNumber, "#m_nSelectedPatternNumber\n\nC++: H2Core::Hydrogen::getSelectedPatternNumber() --> int");
		cl.def("setSelectedPatternNumber", (void (H2Core::Hydrogen::*)(int)) &H2Core::Hydrogen::setSelectedPatternNumber, "Sets #m_nSelectedPatternNumber.\n\n If Preferences::m_pPatternModePlaysSelected is set to true, the\n AudioEngine is locked before  will be assigned. But in\n any case the function will push the\n #EVENT_SELECTED_PATTERN_CHANGED Event to the EventQueue.\n\n If  is equal to #m_nSelectedPatternNumber, the function\n will return right away.\n\n Sets #m_nSelectedPatternNumber\n\nC++: H2Core::Hydrogen::setSelectedPatternNumber(int) --> void", pybind11::arg("nPat"));
		cl.def("getSelectedInstrumentNumber", (int (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::getSelectedInstrumentNumber, "C++: H2Core::Hydrogen::getSelectedInstrumentNumber() --> int");
		cl.def("setSelectedInstrumentNumber", (void (H2Core::Hydrogen::*)(int)) &H2Core::Hydrogen::setSelectedInstrumentNumber, "C++: H2Core::Hydrogen::setSelectedInstrumentNumber(int) --> void", pybind11::arg("nInstrument"));
		cl.def("refreshInstrumentParameters", (void (H2Core::Hydrogen::*)(int)) &H2Core::Hydrogen::refreshInstrumentParameters, "C++: H2Core::Hydrogen::refreshInstrumentParameters(int) --> void", pybind11::arg("nInstrument"));
		cl.def("renameJackPorts", (void (H2Core::Hydrogen::*)(class H2Core::Song *)) &H2Core::Hydrogen::renameJackPorts, "Calls audioEngine_renameJackPorts() if\n Preferences::m_bJackTrackOuts is set to true.\n \n\n Handed to audioEngine_renameJackPorts().\n\nC++: H2Core::Hydrogen::renameJackPorts(class H2Core::Song *) --> void", pybind11::arg("pSong"));
		cl.def("toggleOscServer", (void (H2Core::Hydrogen::*)(bool)) &H2Core::Hydrogen::toggleOscServer, "Starts/stops the OSC server\n \n\n `true` = start, `false` = stop.\n\nC++: H2Core::Hydrogen::toggleOscServer(bool) --> void", pybind11::arg("bEnable"));
		cl.def("recreateOscServer", (void (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::recreateOscServer, "Destroys and recreates the OscServer singleton in order to\n		adopt a new OSC port.\n\nC++: H2Core::Hydrogen::recreateOscServer() --> void");
		cl.def("startNsmClient", (void (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::startNsmClient, "C++: H2Core::Hydrogen::startNsmClient() --> void");
		cl.def("setbeatsToCount", (void (H2Core::Hydrogen::*)(int)) &H2Core::Hydrogen::setbeatsToCount, "C++: H2Core::Hydrogen::setbeatsToCount(int) --> void", pybind11::arg("beatstocount"));
		cl.def("getbeatsToCount", (int (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::getbeatsToCount, "C++: H2Core::Hydrogen::getbeatsToCount() --> int");
		cl.def("setNoteLength", (void (H2Core::Hydrogen::*)(float)) &H2Core::Hydrogen::setNoteLength, "C++: H2Core::Hydrogen::setNoteLength(float) --> void", pybind11::arg("notelength"));
		cl.def("getNoteLength", (float (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::getNoteLength, "C++: H2Core::Hydrogen::getNoteLength() --> float");
		cl.def("getBcStatus", (int (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::getBcStatus, "C++: H2Core::Hydrogen::getBcStatus() --> int");
		cl.def("handleBeatCounter", (void (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::handleBeatCounter, "C++: H2Core::Hydrogen::handleBeatCounter() --> void");
		cl.def("setBcOffsetAdjust", (void (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::setBcOffsetAdjust, "C++: H2Core::Hydrogen::setBcOffsetAdjust() --> void");
		cl.def("offJackMaster", (void (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::offJackMaster, "Calling JackAudioDriver::releaseTimebaseMaster() directly from\n	    the GUI\n\nC++: H2Core::Hydrogen::offJackMaster() --> void");
		cl.def("onJackMaster", (void (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::onJackMaster, "Calling JackAudioDriver::initTimebaseMaster() directly from\n	    the GUI\n\nC++: H2Core::Hydrogen::onJackMaster() --> void");
		cl.def("getPatternLength", (long (H2Core::Hydrogen::*)(int)) &H2Core::Hydrogen::getPatternLength, "Get the length (in ticks) of the  th pattern.\n\n Access the length of the first Pattern found in the\n PatternList at  - 1.\n\n This function should also work if the loop mode is enabled\n in Song::getIsLoopEnabled().\n\n \n Position + 1 of the desired PatternList.\n \n\n \n - __-1__ : if not Song was initialized yet.\n - #MAX_NOTES : if  was smaller than 1, larger\n than the length of the vector of the PatternList in\n Song::m_pPatternGroupSequence or no Pattern could be found\n in the PatternList at  - 1.\n - __else__ : length of first Pattern found at \n	 \n\nC++: H2Core::Hydrogen::getPatternLength(int) --> long", pybind11::arg("nPattern"));
		cl.def("getNewBpmJTM", (float (H2Core::Hydrogen::*)() const) &H2Core::Hydrogen::getNewBpmJTM, "Returns the fallback speed.\n \n\n #m_fNewBpmJTM \n\nC++: H2Core::Hydrogen::getNewBpmJTM() const --> float");
		cl.def("setNewBpmJTM", (void (H2Core::Hydrogen::*)(float)) &H2Core::Hydrogen::setNewBpmJTM, "Set the fallback speed #m_nNewBpmJTM.\n \n\n New default tempo. \n\nC++: H2Core::Hydrogen::setNewBpmJTM(float) --> void", pybind11::arg("bpmJTM"));
		cl.def("__panic", (void (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::__panic, "C++: H2Core::Hydrogen::__panic() --> void");
		cl.def("setTimelineBpm", (void (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::setTimelineBpm, "Updates Song::m_fBpm, TransportInfo::m_fBPM, and #m_fNewBpmJTM\n to the local speed.\n\n The local speed will be obtained by calling getTimelineBpm()\n with getPatternPos() as input argument and set for the current\n song and transport. For setting the\n fallback speed #m_fNewBpmJTM, getRealtimeTickPosition() will be\n used instead.\n\n If Preferences::__useTimelineBpm is set to false or Hydrogen\n uses JACK transport in the presence of an external timebase\n master, the function will return without performing any\n actions.\n\nC++: H2Core::Hydrogen::setTimelineBpm() --> void");
		cl.def("getTimelineBpm", (float (H2Core::Hydrogen::*)(int)) &H2Core::Hydrogen::getTimelineBpm, "Returns the local speed at a specific  in the\n Timeline.\n\n If Hydrogen is in Song::PATTERN_MODE or\n Preferences::__useTimelineBpm is set to false, the global\n speed of the current Song Song::m_fBpm or, if no Song is\n present yet, the result of getNewBpmJTM() will be\n returned. \n\n Its counterpart is setTimelineBpm().\n\n \n Position (in whole patterns) along the Timeline to\n   access the tempo at.\n\n \n Speed in beats per minute.\n\nC++: H2Core::Hydrogen::getTimelineBpm(int) --> float", pybind11::arg("nBar"));
		cl.def("getTimeline", (class H2Core::Timeline * (H2Core::Hydrogen::*)() const) &H2Core::Hydrogen::getTimeline, "C++: H2Core::Hydrogen::getTimeline() const --> class H2Core::Timeline *", pybind11::return_value_policy::automatic);
		cl.def("getIsExportSessionActive", (bool (H2Core::Hydrogen::*)() const) &H2Core::Hydrogen::getIsExportSessionActive, "C++: H2Core::Hydrogen::getIsExportSessionActive() const --> bool");
		cl.def("startExportSession", (void (H2Core::Hydrogen::*)(int, int)) &H2Core::Hydrogen::startExportSession, "C++: H2Core::Hydrogen::startExportSession(int, int) --> void", pybind11::arg("rate"), pybind11::arg("depth"));
		cl.def("stopExportSession", (void (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::stopExportSession, "C++: H2Core::Hydrogen::stopExportSession() --> void");
		cl.def("startExportSong", (void (H2Core::Hydrogen::*)(const class QString &)) &H2Core::Hydrogen::startExportSong, "C++: H2Core::Hydrogen::startExportSong(const class QString &) --> void", pybind11::arg("filename"));
		cl.def("stopExportSong", (void (H2Core::Hydrogen::*)()) &H2Core::Hydrogen::stopExportSong, "C++: H2Core::Hydrogen::stopExportSong() --> void");
		cl.def("getCoreActionController", (class H2Core::CoreActionController * (H2Core::Hydrogen::*)() const) &H2Core::Hydrogen::getCoreActionController, "C++: H2Core::Hydrogen::getCoreActionController() const --> class H2Core::CoreActionController *", pybind11::return_value_policy::automatic);
		cl.def("setPlaybackTrackState", (bool (H2Core::Hydrogen::*)(const bool)) &H2Core::Hydrogen::setPlaybackTrackState, "*********************************************************\n\n******************** Playback track *********************\n\n Wrapper around Song::setPlaybackTrackEnabled().\n\n \n Whether the playback track is enabled. It will\n be replaced by false, if no Song was selected (getSong()\n return nullptr).\n\nC++: H2Core::Hydrogen::setPlaybackTrackState(const bool) --> bool", pybind11::arg("state"));
		cl.def("getPlaybackTrackState", (bool (H2Core::Hydrogen::*)() const) &H2Core::Hydrogen::getPlaybackTrackState, "Wrapper around Song::getPlaybackTrackEnabled().\n\n \n Whether the playback track is enabled or false, if\n no Song was selected (getSong() return nullptr).\n\nC++: H2Core::Hydrogen::getPlaybackTrackState() const --> bool");
		cl.def("loadPlaybackTrack", (void (H2Core::Hydrogen::*)(const class QString)) &H2Core::Hydrogen::loadPlaybackTrack, "Wrapper function for loading the playback track.\n\n Calls Song::setPlaybackTrackFilename() and\n Sampler::reinitialize_playback_track(). While the former\n one is responsible to store metadata about the playback\n track, the latter one does load it to a new\n InstrumentLayer. The function is called by\n SongEditorPanel::editPlaybackTrackBtnPressed()\n\n \n Name of the file to load as the playback\n track\n\nC++: H2Core::Hydrogen::loadPlaybackTrack(const class QString) --> void", pybind11::arg("filename"));
		cl.def("getGUIState", (enum H2Core::Hydrogen::GUIState (H2Core::Hydrogen::*)() const) &H2Core::Hydrogen::getGUIState, "#m_GUIState\n\nC++: H2Core::Hydrogen::getGUIState() const --> enum H2Core::Hydrogen::GUIState");
		cl.def("setGUIState", (void (H2Core::Hydrogen::*)(const enum H2Core::Hydrogen::GUIState)) &H2Core::Hydrogen::setGUIState, "Specifies whether the Qt5 GUI is active. Sets\n	   #m_GUIState.\n\nC++: H2Core::Hydrogen::setGUIState(const enum H2Core::Hydrogen::GUIState) --> void", pybind11::arg("state"));
		cl.def("calculateLeadLagFactor", (int (H2Core::Hydrogen::*)(float)) &H2Core::Hydrogen::calculateLeadLagFactor, "Calculates the lookahead for a specific tick size.\n\n During the humanization the onset of a Note will be moved\n Note::__lead_lag times the value calculated by this function.\n\n Since the size of a tick is tempo dependent, \n allows you to calculate the lead-lag factor for an arbitrary\n position on the Timeline.\n\n \n Number of frames that make up one tick.\n\n \n Five times the current size of a tick\n (TransportInfo::m_fTickSize) (in frames)\n\nC++: H2Core::Hydrogen::calculateLeadLagFactor(float) --> int", pybind11::arg("fTickSize"));
		cl.def("calculateLookahead", (int (H2Core::Hydrogen::*)(float)) &H2Core::Hydrogen::calculateLookahead, "Calculates time offset (in frames) used to determine the notes\n process by the audio engine.\n\n Due to the humanization there might be negative offset in the\n position of a particular note. To be able to still render it\n appropriately, we have to look into and handle notes from the\n future.\n\n The Lookahead is the sum of the #m_nMaxTimeHumanize and\n calculateLeadLagFactor() plus one (since it has to be larger\n than that).\n\n \n Number of frames that make up one tick. Passed\n to calculateLeadLagFactor().\n\n \n Frame offset\n\nC++: H2Core::Hydrogen::calculateLookahead(float) --> int", pybind11::arg("fTickSize"));
		cl.def("haveJackAudioDriver", (bool (H2Core::Hydrogen::*)() const) &H2Core::Hydrogen::haveJackAudioDriver, "Whether JackAudioDriver is used as current audio\n driver.\n\nC++: H2Core::Hydrogen::haveJackAudioDriver() const --> bool");
		cl.def("haveJackTransport", (bool (H2Core::Hydrogen::*)() const) &H2Core::Hydrogen::haveJackTransport, "Whether JackAudioDriver is used as current audio driver\n and JACK transport was activated via the GUI\n (#Preferences::m_bJackTransportMode).\n\nC++: H2Core::Hydrogen::haveJackTransport() const --> bool");
		cl.def("getJackTimebaseState", (enum H2Core::JackAudioDriver::Timebase (H2Core::Hydrogen::*)() const) &H2Core::Hydrogen::getJackTimebaseState, "Whether we haveJackTransport() and there is an external\n JACK timebase master broadcasting us tempo information and\n making use disregard Hydrogen's Timeline information (see\n #JackAudioDriver::m_timebaseState).\n\nC++: H2Core::Hydrogen::getJackTimebaseState() const --> enum H2Core::JackAudioDriver::Timebase");
		cl.def("isUnderSessionManagement", (bool (H2Core::Hydrogen::*)() const) &H2Core::Hydrogen::isUnderSessionManagement, "NsmClient::m_bUnderSessionManagement if NSM is\n		supported.\n\nC++: H2Core::Hydrogen::isUnderSessionManagement() const --> bool");
		cl.def("toQString", [](H2Core::Hydrogen const &o, const class QString & a0) -> QString { return o.toQString(a0); }, "", pybind11::arg("sPrefix"));
		cl.def("toQString", (class QString (H2Core::Hydrogen::*)(const class QString &, bool) const) &H2Core::Hydrogen::toQString, "Formatted string version for debugging purposes.\n \n\n String prefix which will be added in front of\n every new line\n \n\n Instead of the whole content of all classes\n stored as members just a single unique identifier will be\n displayed without line breaks.\n\n \n String presentation of current object.\n\nC++: H2Core::Hydrogen::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
		cl.def("assign", (class H2Core::Hydrogen & (H2Core::Hydrogen::*)(const class H2Core::Hydrogen &)) &H2Core::Hydrogen::operator=, "C++: H2Core::Hydrogen::operator=(const class H2Core::Hydrogen &) --> class H2Core::Hydrogen &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
