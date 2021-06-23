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
#include <core/Basics/DrumkitComponent.h> // H2Core::DrumkitComponent
#include <core/Basics/Instrument.h> // H2Core::Instrument
#include <core/Basics/InstrumentList.h> // H2Core::InstrumentList
#include <core/Basics/Note.h> // 
#include <core/Basics/Note.h> // H2Core::Note
#include <core/Basics/Note.h> // H2Core::SelectedLayerInfo
#include <core/Basics/Pattern.h> // H2Core::Pattern
#include <core/Basics/PatternList.h> // H2Core::PatternList
#include <core/Basics/Sample.h> // H2Core::Sample
#include <core/Basics/Song.h> // 
#include <core/Basics/Song.h> // H2Core::Song
#include <core/Helpers/Xml.h> // H2Core::XMLNode
#include <core/IO/AudioOutput.h> // H2Core::AudioOutput
#include <core/IO/MidiInput.h> // H2Core::MidiInput
#include <core/IO/MidiOutput.h> // H2Core::MidiOutput
#include <core/Object.h> // H2Core::Object
#include <core/Sampler/Interpolation.h> // H2Core::Interpolation::InterpolateMode
#include <core/Sampler/Sampler.h> // H2Core::Sampler
#include <core/Synth/Synth.h> // H2Core::Synth
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

// H2Core::AudioEngine file:core/AudioEngine.h line:101
struct PyCallBack_H2Core_AudioEngine : public H2Core::AudioEngine {
	using H2Core::AudioEngine::AudioEngine;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AudioEngine *>(this), "toQString");
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

void bind_core_AudioEngine(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B655_[H2Core::AudioEngine] ";
	{ // H2Core::AudioEngine file:core/AudioEngine.h line:101
		pybind11::class_<H2Core::AudioEngine, std::shared_ptr<H2Core::AudioEngine>, PyCallBack_H2Core_AudioEngine, H2Core::Object> cl(M("H2Core"), "AudioEngine", "Audio Engine main class.\n\n It serves as a container for the Sampler and Synth stored in the\n #m_pSampler and #m_pSynth member objects and provides a mutex\n #__engine_mutex enabling the user to synchronize the access of the\n Song object and the AudioEngine itself. lock() and try_lock() can\n be called by a thread to lock the engine and unlock() to make it\n accessible for other threads once again.");
		cl.def( pybind11::init( [](){ return new H2Core::AudioEngine(); }, [](){ return new PyCallBack_H2Core_AudioEngine(); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::AudioEngine::class_name, "C++: H2Core::AudioEngine::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("lock", (void (H2Core::AudioEngine::*)(const char *, unsigned int, const char *)) &H2Core::AudioEngine::lock, "Mutex locking of the AudioEngine.\n\n Lock the AudioEngine for exclusive access by this thread.\n\n The documentation below may serve as a guide for future\n implementations. At the moment the logging of the locking\n is __not supported yet__ and the arguments will be just\n stored in the #__locker variable, which itself won't be\n ever used.\n\n Easy usage:  Use the #RIGHT_HERE macro like this...\n \n\n\n\n More complex usage: The parameters  and \n need to be pointers to null-terminated strings that are\n persistent for the entire session.  This does *not* include\n the return value of std::string::c_str(), or\n QString::toLocal8Bit().data().\n\n Tracing the locks:  Enable the Logger::AELockTracing\n logging level.  When you do, there will be a performance\n penalty because the strings will be converted to a\n QString.  At the moment, you'll have to do that with\n your debugger.\n\n Notes: The order of the parameters match GCC's\n implementation of the assert() macros.\n\n \n File the locking occurs in.\n \n\n Line of the file the locking occurs in.\n \n\n Function the locking occurs in.\n\nC++: H2Core::AudioEngine::lock(const char *, unsigned int, const char *) --> void", pybind11::arg("file"), pybind11::arg("line"), pybind11::arg("function"));
		cl.def("tryLock", (bool (H2Core::AudioEngine::*)(const char *, unsigned int, const char *)) &H2Core::AudioEngine::tryLock, "Mutex locking of the AudioEngine.\n\n This function is equivalent to lock() but returns false\n immediaely if the lock cannot be obtained immediately.\n\n \n File the locking occurs in.\n \n\n Line of the file the locking occurs in.\n \n\n Function the locking occurs in.\n\n \n\n - true : On success\n - false : Else\n\nC++: H2Core::AudioEngine::tryLock(const char *, unsigned int, const char *) --> bool", pybind11::arg("file"), pybind11::arg("line"), pybind11::arg("function"));
		cl.def("unlock", (void (H2Core::AudioEngine::*)()) &H2Core::AudioEngine::unlock, "Mutex unlocking of the AudioEngine.\n\n Unlocks the AudioEngine to allow other threads acces, and leaves #__locker untouched.\n\nC++: H2Core::AudioEngine::unlock() --> void");
		cl.def("assertLocked", (void (H2Core::AudioEngine::*)()) &H2Core::AudioEngine::assertLocked, "Assert that the calling thread is the current holder of the\n AudioEngine lock.\n\nC++: H2Core::AudioEngine::assertLocked() --> void");
		cl.def("destroy", (void (H2Core::AudioEngine::*)()) &H2Core::AudioEngine::destroy, "C++: H2Core::AudioEngine::destroy() --> void");
		cl.def("start", [](H2Core::AudioEngine &o) -> int { return o.start(); }, "");
		cl.def("start", [](H2Core::AudioEngine &o, bool const & a0) -> int { return o.start(a0); }, "", pybind11::arg("bLockEngine"));
		cl.def("start", (int (H2Core::AudioEngine::*)(bool, unsigned int)) &H2Core::AudioEngine::start, "If the audio engine is in state #m_audioEngineState #STATE_READY,\n this function will\n - sets #m_fMasterPeak_L and #m_fMasterPeak_R to 0.0f\n - sets TransportInfo::m_nFrames to \n - sets m_nSongPos and m_nPatternStartTick to -1\n - m_nPatternTickPosition to 0\n - sets #m_audioEngineState to #STATE_PLAYING\n - pushes the #EVENT_STATE #STATE_PLAYING using EventQueue::push_event()\n\n \n Whether or not to lock the audio engine before\n   performing any actions. The audio engine __must__ be locked! This\n   option should only be used, if the process calling this function\n   did already locked it.\n \n\n New value of the transport position.\n \n\n 0 regardless what happens inside the function.\n\nC++: H2Core::AudioEngine::start(bool, unsigned int) --> int", pybind11::arg("bLockEngine"), pybind11::arg("nTotalFrames"));
		cl.def("stop", [](H2Core::AudioEngine &o) -> void { return o.stop(); }, "");
		cl.def("stop", (void (H2Core::AudioEngine::*)(bool)) &H2Core::AudioEngine::stop, "If the audio engine is in state #m_audioEngineState #STATE_PLAYING,\n this function will\n - sets #m_fMasterPeak_L and #m_fMasterPeak_R to 0.0f\n - sets #m_audioEngineState to #STATE_READY\n - sets #m_nPatternStartTick to -1\n - deletes all copied Note in song notes queue #m_songNoteQueue and\n   MIDI notes queue #m_midiNoteQueue\n - calls the _clear()_ member of #m_midiNoteQueue\n\n \n Whether or not to lock the audio engine before\n   performing any actions. The audio engine __must__ be locked! This\n   option should only be used, if the process calling this function\n   did already locked it.\n\nC++: H2Core::AudioEngine::stop(bool) --> void", pybind11::arg("bLockEngine"));
		cl.def("setSong", (void (H2Core::AudioEngine::*)(class H2Core::Song *)) &H2Core::AudioEngine::setSong, "Updates the global objects of the audioEngine according to new\n Song.\n\n It also updates all member variables of the audio driver specific\n to the particular song (BPM and tick size).\n\n \n Song to load.\n\nC++: H2Core::AudioEngine::setSong(class H2Core::Song *) --> void", pybind11::arg("pNewSong"));
		cl.def("removeSong", (void (H2Core::AudioEngine::*)()) &H2Core::AudioEngine::removeSong, "Does the necessary cleanup of the global objects in the audioEngine.\n\n Class the clear() member of #m_pPlayingPatterns and\n #m_pNextPatterns as well as audioEngine_clearNoteQueue();\n\nC++: H2Core::AudioEngine::removeSong() --> void");
		cl.def("noteOn", (void (H2Core::AudioEngine::*)(class H2Core::Note *)) &H2Core::AudioEngine::noteOn, "C++: H2Core::AudioEngine::noteOn(class H2Core::Note *) --> void", pybind11::arg("note"));
		cl.def_static("audioEngine_process", (int (*)(unsigned int, void *)) &H2Core::AudioEngine::audioEngine_process, "Main audio processing function called by the audio drivers whenever\n there is work to do.\n\n In short, it resets the audio buffers, checks the current transport\n position and configuration, updates the queue of notes, which are\n about to be played, plays those notes and writes their output to\n the audio buffers, and, finally, increment the transport position\n in order to move forward in time.\n\n In detail the function\n - calls audioEngine_process_clearAudioBuffers() to reset all audio\n buffers with zeros.\n - calls audioEngine_process_transport() to verify the current\n TransportInfo stored in AudioOutput::m_transport. If e.g. the\n JACK server is used, an external JACK client might have changed the\n speed of the transport (as JACK timebase master) or the transport\n position. In such cases, Hydrogen has to sync its internal transport\n state AudioOutput::m_transport to reflect these changes. Else our\n playback would be off.\n - calls audioEngine_process_checkBPMChanged() to check whether the\n tick size, the number of frames per bar (size of a pattern), has\n changed (see TransportInfo::m_nFrames in case you are unfamiliar\n with the term _frames_). This is necessary because the transport\n position is often given in ticks within Hydrogen and changing the\n speed of the Song, e.g. via Hydrogen::setBPM(), would thus result\n in a(n unintended) relocation of the transport location.\n - calls audioEngine_updateNoteQueue() and\n audioEngine_process_playNotes(), two functions which handle the\n selection and playback of notes and will documented at a later\n point in time\n - If audioEngine_updateNoteQueue() returns with 2, the\n EVENT_PATTERN_CHANGED event will be pushed to the EventQueue.\n - writes the audio output of the Sampler, Synth, and the LadspaFX\n (if #H2CORE_HAVE_LADSPA is defined) to #m_pMainBuffer_L and\n #m_pMainBuffer_R and sets we peak values for #m_fFXPeak_L,\n #m_fFXPeak_R, #m_fMasterPeak_L, and #m_fMasterPeak_R.\n - finally increments the transport position\n TransportInfo::m_nFrames with the buffersize  So, if\n this function is called during the next cycle, the transport is\n already in the correct position.\n\n If the H2Core::m_audioEngineState is neither in #STATE_READY nor\n #STATE_PLAYING or the locking of the AudioEngine failed, the\n function will return 0 without performing any actions.\n\n \n Buffersize. If it doesn't match #m_nBufferSize, the\n	   latter will be set to \n \n\n Unused.\n \n\n\n - __2__ : Failed to aquire the audio engine lock, no processing took place.\n - __1__ : kill the audio driver thread. This will be used if either\n the DiskWriterDriver or FakeDriver are used and the end of the Song\n is reached (audioEngine_updateNoteQueue() returned -1 ). \n - __0__ : else\n\nC++: H2Core::AudioEngine::audioEngine_process(unsigned int, void *) --> int", pybind11::arg("nframes"), pybind11::arg("arg"));
		cl.def("clearNoteQueue", (void (H2Core::AudioEngine::*)()) &H2Core::AudioEngine::clearNoteQueue, "C++: H2Core::AudioEngine::clearNoteQueue() --> void");
		cl.def("processPlayNotes", (void (H2Core::AudioEngine::*)(unsigned long)) &H2Core::AudioEngine::processPlayNotes, "C++: H2Core::AudioEngine::processPlayNotes(unsigned long) --> void", pybind11::arg("nframes"));
		cl.def("processTransport", (void (H2Core::AudioEngine::*)()) &H2Core::AudioEngine::processTransport, "Updating the TransportInfo of the audio driver.\n\n Firstly, it calls AudioOutput::updateTransportInfo() and then\n updates the state of the audio engine #m_audioEngineState depending\n on the status of the audio driver.  E.g. if the JACK transport was\n started by another client, the audio engine has to be started as\n well. If TransportInfo::m_status is TransportInfo::ROLLING,\n audioEngine_start() is called with\n TransportInfo::m_nFrames as argument if the engine is in\n #STATE_READY. If #m_audioEngineState is then still not in\n #STATE_PLAYING, the function will return. Otherwise, the current\n speed is getting updated by calling Hydrogen::setBPM using\n TransportInfo::m_fBPM and #m_nRealtimeFrames is set to\n TransportInfo::m_nFrames.\n\n If the status is TransportInfo::STOPPED but the engine is still\n running, audioEngine_stop() will be called. In any case,\n #m_nRealtimeFrames will be incremented by #m_nBufferSize to support\n realtime keyboard and MIDI event timing.\n\n If the H2Core::m_audioEngineState is neither in #STATE_READY nor\n #STATE_PLAYING the function will immediately return.\n\nC++: H2Core::AudioEngine::processTransport() --> void");
		cl.def("updateNoteQueue", (int (H2Core::AudioEngine::*)(unsigned int)) &H2Core::AudioEngine::updateNoteQueue, "Takes all notes from the current patterns, from the MIDI queue\n #m_midiNoteQueue, and those triggered by the metronome and pushes\n them onto #m_songNoteQueue for playback.\n\n Apart from the MIDI queue, the extraction of all notes will be\n based on their position measured in ticks. Since Hydrogen does\n support humanization, which also involves triggering a Note\n earlier or later than its actual position, the loop over all ticks\n won't be done starting from the current position but at some\n position in the future. This value, also called  is\n set to the sum of the maximum offsets introduced by both the random\n humanization (2000 frames) and the deterministic lead-lag offset (5\n times TransportInfo::m_nFrames) plus 1 (note that it's not given in\n ticks but in frames!). Hydrogen thus loops over  frames\n starting at the current position + the lookahead (or at 0 when at\n the beginning of the Song).\n\n Within this loop all MIDI notes in #m_midiNoteQueue with a\n Note::__position smaller or equal the current tick will be popped\n and added to #m_songNoteQueue and the #EVENT_METRONOME Event is\n pushed to the EventQueue at a periodic rate. If in addition\n Preferences::m_bUseMetronome is set to true,\n #m_pMetronomeInstrument will be used to push a 'click' to the\n #m_songNoteQueue too. All patterns enclosing the current tick will\n be added to #m_pPlayingPatterns and all their containing notes,\n which position enclose the current tick too, will be added to the\n #m_songNoteQueue. If the Song is in Song::PATTERN_MODE, the\n patterns used are not chosen by the actual position but by\n #m_nSelectedPatternNumber and #m_pNextPatterns. \n\n All notes obtained by the current patterns (and only those) are\n also subject to humanization in the onset position of the created\n Note. For now Hydrogen does support three options of altering\n these:\n -  - A deterministic offset determined by Song::__swing_factor\n will be added for some notes in a periodic way.\n -  - A random offset drawn from Gaussian white noise\n with a variance proportional to Song::__humanize_time_value will be\n added to every Note.\n -  - A deterministic offset determined by\n Note::__lead_lag will be added for every note.\n\n If the AudioEngine it not in #STATE_PLAYING, the loop jumps right\n to the next tick.\n\n \n\n - -1 if in Song::SONG_MODE and no patterns left.\n - 2 if the current pattern changed with respect to the last\n cycle.\n\nC++: H2Core::AudioEngine::updateNoteQueue(unsigned int) --> int", pybind11::arg("nFrames"));
		cl.def("findPatternInTick", (int (H2Core::AudioEngine::*)(int, bool, int *)) &H2Core::AudioEngine::findPatternInTick, "Find a PatternList corresponding to the supplied tick position \n\n Adds up the lengths of all pattern columns until  lies in\n between the bounds of a Pattern.\n\n \n Position in ticks.\n \n\n Whether looping is enabled in the Song, see\n   Song::is_loop_enabled(). If true,  is allowed to be\n   larger than the total length of the Song.\n \n\n Pointer to an integer the beginning of the\n   found pattern list will be stored in (in ticks).\n \n\n\n   - -1 : pattern list couldn't be found.\n   - >=0 : PatternList index in Song::__pattern_group_sequence.\n\nC++: H2Core::AudioEngine::findPatternInTick(int, bool, int *) --> int", pybind11::arg("nTick"), pybind11::arg("bLoopMode"), pybind11::arg("pPatternStartTick"));
		cl.def("seek", [](H2Core::AudioEngine &o, long long const & a0) -> void { return o.seek(a0); }, "", pybind11::arg("nFrames"));
		cl.def("seek", (void (H2Core::AudioEngine::*)(long long, bool)) &H2Core::AudioEngine::seek, "C++: H2Core::AudioEngine::seek(long long, bool) --> void", pybind11::arg("nFrames"), pybind11::arg("bLoopMode"));
		cl.def_static("computeTickSize", (float (*)(const int, const float, const int)) &H2Core::AudioEngine::computeTickSize, "C++: H2Core::AudioEngine::computeTickSize(const int, const float, const int) --> float", pybind11::arg("nSampleRate"), pybind11::arg("fBpm"), pybind11::arg("nResolution"));
		cl.def("getSampler", (class H2Core::Sampler * (H2Core::AudioEngine::*)()) &H2Core::AudioEngine::getSampler, "#m_pSampler \n\nC++: H2Core::AudioEngine::getSampler() --> class H2Core::Sampler *", pybind11::return_value_policy::automatic);
		cl.def("getSynth", (class H2Core::Synth * (H2Core::AudioEngine::*)()) &H2Core::AudioEngine::getSynth, "#m_pSynth \n\nC++: H2Core::AudioEngine::getSynth() --> class H2Core::Synth *", pybind11::return_value_policy::automatic);
		cl.def("getElapsedTime", (float (H2Core::AudioEngine::*)() const) &H2Core::AudioEngine::getElapsedTime, "#m_fElapsedTime \n\nC++: H2Core::AudioEngine::getElapsedTime() const --> float");
		cl.def("calculateElapsedTime", (void (H2Core::AudioEngine::*)(unsigned int, unsigned long, int)) &H2Core::AudioEngine::calculateElapsedTime, "Calculates the elapsed time for an arbitrary position.\n\n After locating the transport position to  the function\n calculates the amount of time required to reach the position\n during playback. If the Timeline is activated, it will take all\n markers and the resulting tempo changes into account.\n\n Right now the tempo in the region before the first marker\n is undefined. In order to make reproducible estimates of the\n elapsed time, this function assume it to have the same BPM as\n the first marker.\n\n \n Temporal resolution used by the sound card in\n frames per second.\n \n\n Next transport position in frames.\n \n\n Resolution of the Song (number of ticks per \n   quarter).\n\nC++: H2Core::AudioEngine::calculateElapsedTime(unsigned int, unsigned long, int) --> void", pybind11::arg("sampleRate"), pybind11::arg("nFrame"), pybind11::arg("nResolution"));
		cl.def("updateElapsedTime", (void (H2Core::AudioEngine::*)(unsigned int, unsigned int)) &H2Core::AudioEngine::updateElapsedTime, "Increments #m_fElapsedTime at the end of a process cycle.\n\n At the end of H2Core::audioEngine_process() this function will\n be used to add the time passed during the last process cycle to\n #m_fElapsedTime.\n\n \n Number of frames process during a cycle of\n the audio engine.\n \n\n Temporal resolution used by the sound card in\n frames per second.\n\nC++: H2Core::AudioEngine::updateElapsedTime(unsigned int, unsigned int) --> void", pybind11::arg("bufferSize"), pybind11::arg("sampleRate"));
		cl.def("processCheckBPMChanged", (void (H2Core::AudioEngine::*)(class H2Core::Song *)) &H2Core::AudioEngine::processCheckBPMChanged, "Update the tick size based on the current tempo without affecting\n the current transport position.\n\n To access a change in the tick size, the value stored in\n TransportInfo::m_fTickSize will be compared to the one calculated\n from the AudioOutput::getSampleRate(), Song::__bpm, and\n Song::__resolution. Thus, if any of those quantities did change,\n the transport position will be recalculated.\n\n The new transport position gets calculated by \n \n\n\n\n\n\n If the JackAudioDriver is used and the audio engine is playing, a\n potential mismatch in the transport position is determined by\n JackAudioDriver::calculateFrameOffset() and covered by\n JackAudioDriver::updateTransportInfo() in the next cycle.\n\n Finally, EventQueue::push_event() is called with\n #EVENT_RECALCULATERUBBERBAND and -1 as arguments.\n\n Called in audioEngine_process() and audioEngine_setSong(). The\n function will only perform actions if #m_audioEngineState is in\n either #STATE_READY or #STATE_PLAYING.\n\nC++: H2Core::AudioEngine::processCheckBPMChanged(class H2Core::Song *) --> void", pybind11::arg("pSong"));
		cl.def("locate", (void (H2Core::AudioEngine::*)(unsigned long)) &H2Core::AudioEngine::locate, "Relocate using the audio driver and update the\n #m_fElapsedTime.\n\n \n Next transport position in frames.\n\nC++: H2Core::AudioEngine::locate(unsigned long) --> void", pybind11::arg("nFrame"));
		cl.def("clearAudioBuffers", (void (H2Core::AudioEngine::*)(unsigned int)) &H2Core::AudioEngine::clearAudioBuffers, "Clear all audio buffers.\n\n It locks the audio output buffer using #mutex_OutputPointer, gets\n fresh pointers to the output buffers #m_pMainBuffer_L and\n #m_pMainBuffer_R using AudioOutput::getOut_L() and\n AudioOutput::getOut_R() of the current instance of the audio driver\n #m_pAudioDriver, and overwrites their memory with\n \n\n\n\n zeros.\n\n If the JACK driver is used and Preferences::m_bJackTrackOuts is set\n to true, the stereo buffers for all tracks of the components of\n each instrument will be reset as well.  If LadspaFX are used, the\n output buffers of all effects LadspaFX::m_pBuffer_L and\n LadspaFX::m_pBuffer_L have to be reset as well.\n\n If the audio driver #m_pAudioDriver isn't set yet, it will just\n unlock and return.\n\nC++: H2Core::AudioEngine::clearAudioBuffers(unsigned int) --> void", pybind11::arg("nFrames"));
		cl.def("createDriver", (class H2Core::AudioOutput * (H2Core::AudioEngine::*)(const class QString &)) &H2Core::AudioEngine::createDriver, "Create an audio driver using audioEngine_process() as its argument\n based on the provided choice and calling their _init()_ function to\n trigger their initialization.\n\n For a listing of all possible choices, please see\n Preferences::m_sAudioDriver.\n\n \n String specifying which audio driver should be\n created.\n \n\n Pointer to the freshly created audio driver. If the\n creation resulted in a NullDriver, the corresponding object will be\n deleted and a null pointer returned instead.\n\nC++: H2Core::AudioEngine::createDriver(const class QString &) --> class H2Core::AudioOutput *", pybind11::return_value_policy::automatic, pybind11::arg("sDriver"));
		cl.def("startAudioDrivers", (void (H2Core::AudioEngine::*)()) &H2Core::AudioEngine::startAudioDrivers, "Creation and initialization of all audio and MIDI drivers called in\n Hydrogen::Hydrogen().\n\n Which audio driver to use is specified in\n Preferences::m_sAudioDriver. If \"Auto\" is selected, it will try to\n initialize drivers using createDriver() in the following order: \n - Windows:  \"PortAudio\", \"ALSA\", \"CoreAudio\", \"JACK\", \"OSS\",\n   and \"PulseAudio\" \n - all other systems: \"Jack\", \"ALSA\", \"CoreAudio\", \"PortAudio\",\n   \"OSS\", and \"PulseAudio\".\n If all of them return NULL, #m_pAudioDriver will be initialized\n with the NullDriver instead. If a specific choice is contained in\n Preferences::m_sAudioDriver and createDriver() returns NULL, the\n NullDriver will be initialized too.\n\n It probes Preferences::m_sMidiDriver to create a midi driver using\n either AlsaMidiDriver::AlsaMidiDriver(),\n PortMidiDriver::PortMidiDriver(), CoreMidiDriver::CoreMidiDriver(),\n or JackMidiDriver::JackMidiDriver(). Afterwards, it sets\n #m_pMidiDriverOut and #m_pMidiDriver to the freshly created midi\n driver and calls their open() and setActive( true ) functions.\n\n If a Song is already present, the state of the AudioEngine\n #m_audioEngineState will be set to #STATE_READY, the bpm of the\n #m_pAudioDriver will be set to the tempo of the Song Song::__bpm\n using AudioOutput::setBpm(), and #STATE_READY is pushed on the\n EventQueue. If no Song is present, the state will be\n #STATE_PREPARED and no bpm will be set.\n\n All the actions mentioned so far will be performed after locking\n both the AudioEngine using AudioEngine::lock() and the mutex of the\n audio output buffer #mutex_OutputPointer. When they are completed\n both mutex are unlocked and the audio driver is connected via\n AudioOutput::connect(). If this is not successful, the audio driver\n will be overwritten with the NullDriver and this one is connected\n instead.\n\n Finally, audioEngine_renameJackPorts() (if #H2CORE_HAVE_JACK is set)\n and audioEngine_setupLadspaFX() are called.\n\n The state of the AudioEngine #m_audioEngineState must not be in\n #STATE_INITIALIZED or the function will just unlock both mutex and\n returns.\n\nC++: H2Core::AudioEngine::startAudioDrivers() --> void");
		cl.def("stopAudioDrivers", (void (H2Core::AudioEngine::*)()) &H2Core::AudioEngine::stopAudioDrivers, "Stops all audio and MIDI drivers.\n\n Uses audioEngine_stop() if the AudioEngine is still in state\n #m_audioEngineState #STATE_PLAYING, sets its state to\n #STATE_INITIALIZED, locks the AudioEngine using\n AudioEngine::lock(), deletes #m_pMidiDriver and #m_pAudioDriver and\n reinitializes them to NULL. \n\n If #m_audioEngineState is neither in #STATE_PREPARED or\n #STATE_READY, the function returns before deleting anything.\n\nC++: H2Core::AudioEngine::stopAudioDrivers() --> void");
		cl.def("restartAudioDrivers", (void (H2Core::AudioEngine::*)()) &H2Core::AudioEngine::restartAudioDrivers, "C++: H2Core::AudioEngine::restartAudioDrivers() --> void");
		cl.def("setupLadspaFX", (void (H2Core::AudioEngine::*)(unsigned int)) &H2Core::AudioEngine::setupLadspaFX, "C++: H2Core::AudioEngine::setupLadspaFX(unsigned int) --> void", pybind11::arg("nBufferSize"));
		cl.def("renameJackPorts", (void (H2Core::AudioEngine::*)(class H2Core::Song *)) &H2Core::AudioEngine::renameJackPorts, "Hands the provided Song to JackAudioDriver::makeTrackOutputs() if\n  is not a null pointer and the audio driver #m_pAudioDriver\n is an instance of the JackAudioDriver.\n \n\n Song for which per-track output ports should be generated.\n\nC++: H2Core::AudioEngine::renameJackPorts(class H2Core::Song *) --> void", pybind11::arg("pSong"));
		cl.def("setAudioDriver", (void (H2Core::AudioEngine::*)(class H2Core::AudioOutput *)) &H2Core::AudioEngine::setAudioDriver, "C++: H2Core::AudioEngine::setAudioDriver(class H2Core::AudioOutput *) --> void", pybind11::arg("pAudioDriver"));
		cl.def("getAudioDriver", (class H2Core::AudioOutput * (H2Core::AudioEngine::*)() const) &H2Core::AudioEngine::getAudioDriver, "C++: H2Core::AudioEngine::getAudioDriver() const --> class H2Core::AudioOutput *", pybind11::return_value_policy::automatic);
		cl.def("getMidiDriver", (class H2Core::MidiInput * (H2Core::AudioEngine::*)() const) &H2Core::AudioEngine::getMidiDriver, "C++: H2Core::AudioEngine::getMidiDriver() const --> class H2Core::MidiInput *", pybind11::return_value_policy::automatic);
		cl.def("getMidiOutDriver", (class H2Core::MidiOutput * (H2Core::AudioEngine::*)() const) &H2Core::AudioEngine::getMidiOutDriver, "C++: H2Core::AudioEngine::getMidiOutDriver() const --> class H2Core::MidiOutput *", pybind11::return_value_policy::automatic);
		cl.def("raiseError", (void (H2Core::AudioEngine::*)(unsigned int)) &H2Core::AudioEngine::raiseError, "C++: H2Core::AudioEngine::raiseError(unsigned int) --> void", pybind11::arg("nErrorCode"));
		cl.def("getState", (int (H2Core::AudioEngine::*)() const) &H2Core::AudioEngine::getState, "C++: H2Core::AudioEngine::getState() const --> int");
		cl.def("setState", (void (H2Core::AudioEngine::*)(int)) &H2Core::AudioEngine::setState, "C++: H2Core::AudioEngine::setState(int) --> void", pybind11::arg("state"));
		cl.def("setMainBuffer_L", (void (H2Core::AudioEngine::*)(float *)) &H2Core::AudioEngine::setMainBuffer_L, "C++: H2Core::AudioEngine::setMainBuffer_L(float *) --> void", pybind11::arg("pMainBufferL"));
		cl.def("setMainBuffer_R", (void (H2Core::AudioEngine::*)(float *)) &H2Core::AudioEngine::setMainBuffer_R, "C++: H2Core::AudioEngine::setMainBuffer_R(float *) --> void", pybind11::arg("pMainBufferR"));
		cl.def("setMasterPeak_L", (void (H2Core::AudioEngine::*)(float)) &H2Core::AudioEngine::setMasterPeak_L, "C++: H2Core::AudioEngine::setMasterPeak_L(float) --> void", pybind11::arg("value"));
		cl.def("getMasterPeak_L", (float (H2Core::AudioEngine::*)() const) &H2Core::AudioEngine::getMasterPeak_L, "C++: H2Core::AudioEngine::getMasterPeak_L() const --> float");
		cl.def("setMasterPeak_R", (void (H2Core::AudioEngine::*)(float)) &H2Core::AudioEngine::setMasterPeak_R, "C++: H2Core::AudioEngine::setMasterPeak_R(float) --> void", pybind11::arg("value"));
		cl.def("getMasterPeak_R", (float (H2Core::AudioEngine::*)() const) &H2Core::AudioEngine::getMasterPeak_R, "C++: H2Core::AudioEngine::getMasterPeak_R() const --> float");
		cl.def("getProcessTime", (float (H2Core::AudioEngine::*)() const) &H2Core::AudioEngine::getProcessTime, "C++: H2Core::AudioEngine::getProcessTime() const --> float");
		cl.def("getMaxProcessTime", (float (H2Core::AudioEngine::*)() const) &H2Core::AudioEngine::getMaxProcessTime, "C++: H2Core::AudioEngine::getMaxProcessTime() const --> float");
		cl.def("getSelectedPatternNumber", (int (H2Core::AudioEngine::*)() const) &H2Core::AudioEngine::getSelectedPatternNumber, "C++: H2Core::AudioEngine::getSelectedPatternNumber() const --> int");
		cl.def("setSelectedPatternNumber", (void (H2Core::AudioEngine::*)(int)) &H2Core::AudioEngine::setSelectedPatternNumber, "C++: H2Core::AudioEngine::setSelectedPatternNumber(int) --> void", pybind11::arg("number"));
		cl.def("setPatternStartTick", (void (H2Core::AudioEngine::*)(int)) &H2Core::AudioEngine::setPatternStartTick, "C++: H2Core::AudioEngine::setPatternStartTick(int) --> void", pybind11::arg("tick"));
		cl.def("setPatternTickPosition", (void (H2Core::AudioEngine::*)(int)) &H2Core::AudioEngine::setPatternTickPosition, "C++: H2Core::AudioEngine::setPatternTickPosition(int) --> void", pybind11::arg("tick"));
		cl.def("getPatternTickPosition", (int (H2Core::AudioEngine::*)() const) &H2Core::AudioEngine::getPatternTickPosition, "C++: H2Core::AudioEngine::getPatternTickPosition() const --> int");
		cl.def("setSongPos", (void (H2Core::AudioEngine::*)(int)) &H2Core::AudioEngine::setSongPos, "C++: H2Core::AudioEngine::setSongPos(int) --> void", pybind11::arg("songPos"));
		cl.def("getSongPos", (int (H2Core::AudioEngine::*)() const) &H2Core::AudioEngine::getSongPos, "C++: H2Core::AudioEngine::getSongPos() const --> int");
		cl.def("getNextPatterns", (class H2Core::PatternList * (H2Core::AudioEngine::*)() const) &H2Core::AudioEngine::getNextPatterns, "C++: H2Core::AudioEngine::getNextPatterns() const --> class H2Core::PatternList *", pybind11::return_value_policy::automatic);
		cl.def("getPlayingPatterns", (class H2Core::PatternList * (H2Core::AudioEngine::*)() const) &H2Core::AudioEngine::getPlayingPatterns, "C++: H2Core::AudioEngine::getPlayingPatterns() const --> class H2Core::PatternList *", pybind11::return_value_policy::automatic);
		cl.def("getRealtimeFrames", (unsigned long (H2Core::AudioEngine::*)() const) &H2Core::AudioEngine::getRealtimeFrames, "C++: H2Core::AudioEngine::getRealtimeFrames() const --> unsigned long");
		cl.def("setRealtimeFrames", (void (H2Core::AudioEngine::*)(unsigned long)) &H2Core::AudioEngine::setRealtimeFrames, "C++: H2Core::AudioEngine::setRealtimeFrames(unsigned long) --> void", pybind11::arg("nFrames"));
		cl.def("getAddRealtimeNoteTickPosition", (unsigned int (H2Core::AudioEngine::*)() const) &H2Core::AudioEngine::getAddRealtimeNoteTickPosition, "C++: H2Core::AudioEngine::getAddRealtimeNoteTickPosition() const --> unsigned int");
		cl.def("setAddRealtimeNoteTickPosition", (void (H2Core::AudioEngine::*)(unsigned int)) &H2Core::AudioEngine::setAddRealtimeNoteTickPosition, "C++: H2Core::AudioEngine::setAddRealtimeNoteTickPosition(unsigned int) --> void", pybind11::arg("tickPosition"));
		cl.def("getCurrentTickTime", (struct timeval & (H2Core::AudioEngine::*)()) &H2Core::AudioEngine::getCurrentTickTime, "C++: H2Core::AudioEngine::getCurrentTickTime() --> struct timeval &", pybind11::return_value_policy::automatic);
	}
}
