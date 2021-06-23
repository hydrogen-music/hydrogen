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
#include <core/Basics/InstrumentComponent.h> // H2Core::InstrumentComponent
#include <core/Basics/InstrumentList.h> // H2Core::InstrumentList
#include <core/Basics/PatternList.h> // H2Core::PatternList
#include <core/Basics/Song.h> // 
#include <core/Basics/Song.h> // H2Core::Song
#include <core/IO/JackAudioDriver.h> // 
#include <core/IO/JackAudioDriver.h> // H2Core::JackAudioDriver
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

// H2Core::JackAudioDriver file:core/IO/JackAudioDriver.h line:115
struct PyCallBack_H2Core_JackAudioDriver : public H2Core::JackAudioDriver {
	using H2Core::JackAudioDriver::JackAudioDriver;

	int connect() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackAudioDriver *>(this), "connect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return JackAudioDriver::connect();
	}
	void disconnect() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackAudioDriver *>(this), "disconnect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return JackAudioDriver::disconnect();
	}
	unsigned int getBufferSize() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackAudioDriver *>(this), "getBufferSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned int>::value) {
				static pybind11::detail::override_caster_t<unsigned int> caster;
				return pybind11::detail::cast_ref<unsigned int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned int>(std::move(o));
		}
		return JackAudioDriver::getBufferSize();
	}
	unsigned int getSampleRate() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackAudioDriver *>(this), "getSampleRate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned int>::value) {
				static pybind11::detail::override_caster_t<unsigned int> caster;
				return pybind11::detail::cast_ref<unsigned int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned int>(std::move(o));
		}
		return JackAudioDriver::getSampleRate();
	}
	float * getOut_L() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackAudioDriver *>(this), "getOut_L");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<float *>::value) {
				static pybind11::detail::override_caster_t<float *> caster;
				return pybind11::detail::cast_ref<float *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<float *>(std::move(o));
		}
		return JackAudioDriver::getOut_L();
	}
	float * getOut_R() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackAudioDriver *>(this), "getOut_R");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<float *>::value) {
				static pybind11::detail::override_caster_t<float *> caster;
				return pybind11::detail::cast_ref<float *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<float *>(std::move(o));
		}
		return JackAudioDriver::getOut_R();
	}
	int init(unsigned int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackAudioDriver *>(this), "init");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return JackAudioDriver::init(a0);
	}
	void play() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackAudioDriver *>(this), "play");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return JackAudioDriver::play();
	}
	void stop() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackAudioDriver *>(this), "stop");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return JackAudioDriver::stop();
	}
	void locate(unsigned long a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackAudioDriver *>(this), "locate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return JackAudioDriver::locate(a0);
	}
	void updateTransportInfo() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackAudioDriver *>(this), "updateTransportInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return JackAudioDriver::updateTransportInfo();
	}
	void setBpm(float a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackAudioDriver *>(this), "setBpm");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return JackAudioDriver::setBpm(a0);
	}
	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::JackAudioDriver *>(this), "toQString");
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

void bind_core_IO_JackAudioDriver(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B709_[H2Core::JackAudioDriver] ";
	{ // H2Core::JackAudioDriver file:core/IO/JackAudioDriver.h line:115
		pybind11::class_<H2Core::JackAudioDriver, std::shared_ptr<H2Core::JackAudioDriver>, PyCallBack_H2Core_JackAudioDriver, H2Core::AudioOutput> cl(M("H2Core"), "JackAudioDriver", "JACK (Jack Audio Connection Kit) server driver.\n\n __Transport Control__:\n\n Each JACK client can start and stop the transport or relocate the\n current transport position. The request will take place in\n cycles. During the first the status of the transport changes to\n _JackTransportStarting_ to inform all clients a change is about to\n happen. During the second the status is again\n _JackTransportRolling_ and the transport position is updated\n according to the request. The current timebase master (see below),\n if present, needs another cycle to update the additional transport\n information.\n\n Such a relocation request is also triggered when clicking on the\n timeline or the player control buttons of Hydrogen. Internally,\n audioEngine_stop() is called during the cycle in which the JACK\n transport status is _JackTransportStarting_ and started again by\n audioEngine_start() when in _JackTransportRolling_ in the next\n cycle. Note that if there are slow synchronizing client in JACK's\n connection graph, it can take multiple cycles until the JACK\n transport is rolling again.\n\n Also note that Hydrogen overwrites its local TransportInfo stored\n in AudioOutput::m_transport only with the transport position of the\n JACK server if a relocation did happened or another timebase master\n did change the speed. During normal transport the current position\n TransportInfo::m_nFrames will be always the same as the one of JACK\n during a cycle and incremented by the buffer size in\n audioEngine_process() at the very end of the cycle. The same\n happens for the transport information of the JACK server but in\n parallel.\n\n __Timebase Master__:\n\n The timebase master is responsible to update additional information\n in the transport information of the JACK server apart from the\n transport position in frames (see TransportInfo::m_nFrames if you\n aren't familiar with frames), like the current beat, bar, tick,\n tick size, speed etc. Every client can be registered as timebase\n master by supplying a callback (for Hydrogen this would be\n JackTimebaseCallback()) but there can be at most one timebase\n master at a time. Having none at all is perfectly fine too. Apart\n from this additional responsibility, the registered client has no\n other rights compared to others.\n\n After the status of the JACK transport has changed from\n _JackTransportStarting_ to _JackTransportRolling_, the timebase\n master needs an additional cycle to update its information.\n\n Having an external timebase master present will change the general\n behavior of Hydrogen. All local tempo settings on the Timeline will\n be disregarded and the tempo broadcasted by the JACK server will be\n used instead.\n\n This object will only be accessible if #H2CORE_HAVE_JACK was defined\n during the configuration and the user enables the support of the\n JACK server.");
		cl.def( pybind11::init( [](PyCallBack_H2Core_JackAudioDriver const &o){ return new PyCallBack_H2Core_JackAudioDriver(o); } ) );
		cl.def( pybind11::init( [](H2Core::JackAudioDriver const &o){ return new H2Core::JackAudioDriver(o); } ) );

		pybind11::enum_<H2Core::JackAudioDriver::Timebase>(cl, "Timebase", "Whether Hydrogen or another program is Jack timebase master.")
			.value("Master", H2Core::JackAudioDriver::Timebase::Master)
			.value("Slave", H2Core::JackAudioDriver::Timebase::Slave)
			.value("None", H2Core::JackAudioDriver::Timebase::None);

		cl.def_readwrite("m_currentPos", &H2Core::JackAudioDriver::m_currentPos);
		cl.def_static("class_name", (const char * (*)()) &H2Core::JackAudioDriver::class_name, "C++: H2Core::JackAudioDriver::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("connect", (int (H2Core::JackAudioDriver::*)()) &H2Core::JackAudioDriver::connect, "Connects to output ports via the JACK server.\n\n Starts by telling the JACK server that Hydrogen is ready to\n process audio using the jack_activate function (from the\n jack/jack.h header) and overwriting the memory allocated by\n #m_pTrackOutputPortsL and #m_pTrackOutputPortsR with zeros. If\n the #m_bConnectDefaults variable is true or LashClient is used\n and Hydrogen is not within a new Lash project, the function\n attempts to connect the #m_pOutputPort1 port with\n #m_sOutputPortName1 and the #m_pOutputPort2 port with\n #m_sOutputPortName2. To establish the connection,\n _jack_connect()_ (jack/jack.h) will be used. In case this was\n not successful, the function will look up all ports containing\n the _JackPortIsInput_ (jack/types.h) flag using\n _jack_get_ports()_ (jack/jack.h) and attempts to connect to the\n first two it found.\n\n \n \n - __0__ : if either the connection of the output ports did\n       work, two ports having the _JackPortIsInput_ flag where\n       found and the #m_pOutputPort1 and #m_pOutputPort2 ports\n       where successfully connected to them, or the user enabled\n       Lash during compilation and an established project was\n       used.\n - __1__ : The activation of the JACK client using\n       _jack_activate()_ (jack/jack.h) did fail.\n - __2__ : The connections to #m_sOutputPortName1 and\n       #m_sOutputPortName2 could not be established and the\n       there were either no JACK ports holding the\n       JackPortIsInput flag found or no connection to them could\n       be established.\n\nC++: H2Core::JackAudioDriver::connect() --> int");
		cl.def("disconnect", (void (H2Core::JackAudioDriver::*)()) &H2Core::JackAudioDriver::disconnect, "Disconnects the JACK client of the Hydrogen from the JACK\n server.\n\n Firstly, it calls deactivate(). Then, it closes the connection\n between the JACK server and the local client using\n jack_client_close (jack/jack.h), and sets the #m_pClient\n pointer to nullptr.\n\nC++: H2Core::JackAudioDriver::disconnect() --> void");
		cl.def("deactivate", (void (H2Core::JackAudioDriver::*)()) &H2Core::JackAudioDriver::deactivate, "Deactivates the JACK client of Hydrogen and disconnects all\n ports belonging to it.\n\n It calls the _jack_deactivate()_ (jack/jack.h) function on the\n current client #m_pClient and overwrites the memory allocated\n by #m_pTrackOutputPortsL and #m_pTrackOutputPortsR with zeros.\n\nC++: H2Core::JackAudioDriver::deactivate() --> void");
		cl.def("getBufferSize", (unsigned int (H2Core::JackAudioDriver::*)()) &H2Core::JackAudioDriver::getBufferSize, "Global variable #jackServerBufferSize. \n\nC++: H2Core::JackAudioDriver::getBufferSize() --> unsigned int");
		cl.def("getSampleRate", (unsigned int (H2Core::JackAudioDriver::*)()) &H2Core::JackAudioDriver::getSampleRate, "Global variable #jackServerSampleRate. \n\nC++: H2Core::JackAudioDriver::getSampleRate() --> unsigned int");
		cl.def("clearPerTrackAudioBuffers", (void (H2Core::JackAudioDriver::*)(unsigned int)) &H2Core::JackAudioDriver::clearPerTrackAudioBuffers, "Resets the buffers contained in #m_pTrackOutputPortsL and\n #m_pTrackOutputPortsR.\n\n \n Size of the buffers used in the audio process\n callback function.\n\nC++: H2Core::JackAudioDriver::clearPerTrackAudioBuffers(unsigned int) --> void", pybind11::arg("nFrames"));
		cl.def("makeTrackOutputs", (void (H2Core::JackAudioDriver::*)(class H2Core::Song *)) &H2Core::JackAudioDriver::makeTrackOutputs, "Creates per component output ports for each instrument.\n\n Firstly, it resets #m_trackMap with zeros. Then, it loops\n through all the instruments and their components, creates a new\n output or resets an existing one for each of them using\n setTrackOutput(), and stores the corresponding track number in\n #m_trackMap. Finally, all ports in #m_pTrackOutputPortsL and\n #m_pTrackOutputPortsR, which haven't been used in the previous\n step, are unregistered using _jack_port_unregister()_\n (jack/jack.h) and overwritten with 0. #m_nTrackPortCount will\n be set to biggest track number encountered during the\n creation/reassignment step.\n\n The function will only perform its tasks if the\n Preferences::m_bJackTrackOuts is set to true.\n\nC++: H2Core::JackAudioDriver::makeTrackOutputs(class H2Core::Song *) --> void", pybind11::arg("pSong"));
		cl.def("setConnectDefaults", (void (H2Core::JackAudioDriver::*)(bool)) &H2Core::JackAudioDriver::setConnectDefaults, "Sets #m_bConnectDefaults\n\nC++: H2Core::JackAudioDriver::setConnectDefaults(bool) --> void", pybind11::arg("flag"));
		cl.def("getConnectDefaults", (bool (H2Core::JackAudioDriver::*)()) &H2Core::JackAudioDriver::getConnectDefaults, "#m_bConnectDefaults \n\nC++: H2Core::JackAudioDriver::getConnectDefaults() --> bool");
		cl.def("getOut_L", (float * (H2Core::JackAudioDriver::*)()) &H2Core::JackAudioDriver::getOut_L, "Get content in the left stereo output port.\n\n It calls _jack_port_get_buffer()_ (jack/jack.h) with both the\n port name #m_pOutputPort1 and buffer size\n #jackServerBufferSize.\n\n \n Pointer to buffer content of type\n _jack_default_audio_sample_t*_ (jack/types.h)\n\nC++: H2Core::JackAudioDriver::getOut_L() --> float *", pybind11::return_value_policy::automatic);
		cl.def("getOut_R", (float * (H2Core::JackAudioDriver::*)()) &H2Core::JackAudioDriver::getOut_R, "Get content in the right stereo output port.\n\n It calls _jack_port_get_buffer()_ (jack/jack.h) with both the\n port name #m_pOutputPort2 and buffer size\n #jackServerBufferSize.\n\n \n Pointer to buffer content of type\n _jack_default_audio_sample_t*_ (jack/types.h)\n\nC++: H2Core::JackAudioDriver::getOut_R() --> float *", pybind11::return_value_policy::automatic);
		cl.def("getTrackOut_L", (float * (H2Core::JackAudioDriver::*)(unsigned int)) &H2Core::JackAudioDriver::getTrackOut_L, "Get content of left output port of a specific track.\n\n It calls _jack_port_get_buffer()_ (jack/jack.h) with the port\n in the  element of #m_pTrackOutputPortsL and buffer\n size #jackServerBufferSize.\n\n \n Track number. Must not be bigger than\n #m_nTrackPortCount.\n\n \n Pointer to buffer content of type\n _jack_default_audio_sample_t*_ (jack/types.h)\n\nC++: H2Core::JackAudioDriver::getTrackOut_L(unsigned int) --> float *", pybind11::return_value_policy::automatic, pybind11::arg("nTrack"));
		cl.def("getTrackOut_R", (float * (H2Core::JackAudioDriver::*)(unsigned int)) &H2Core::JackAudioDriver::getTrackOut_R, "Get content of right output port of a specific track.\n\n It calls _jack_port_get_buffer()_ (jack/jack.h) with the port\n in the  element of #m_pTrackOutputPortsR and buffer\n size #jackServerBufferSize.\n\n \n Track number. Must not be bigger than\n #m_nTrackPortCount.\n\n \n Pointer to buffer content of type\n _jack_default_audio_sample_t*_ (jack/types.h)\n\nC++: H2Core::JackAudioDriver::getTrackOut_R(unsigned int) --> float *", pybind11::return_value_policy::automatic, pybind11::arg("nTrack"));
		cl.def("init", (int (H2Core::JackAudioDriver::*)(unsigned int)) &H2Core::JackAudioDriver::init, "Initializes the JACK audio driver.\n\n Firstly, it determines the destination ports\n #m_sOutputPortName1 and #m_sOutputPortName2 the output ports of\n Hydrogen will be connected to in connect() from\n Preferences::m_sJackPortName1 and\n Preferences::m_sJackPortName2. The name of the JACK client is\n either set to \"Hydrogen\" or, if #H2CORE_HAVE_OSC was defined\n during compilation and OSC support is enabled, to\n Preferences::m_sNsmClientId via Preferences::getNsmClientId().\n\n Next, the function attempts to open an external client session\n with the JACK server using _jack_client_open()_ (jack/jack.h)\n and saves it to the pointer #m_pClient. In case this didn't\n work properly, it will start two more attempts. Sometime JACK\n doesn't stop and start fast enough. If the compiler flag\n #H2CORE_HAVE_JACKSESSION was set and the user enabled the usage\n of JACK session, the client will be opened using the\n corresponding option and the sessionID Token\n Preferences::jackSessionUUID, obtained via\n Preferences::getJackSessionUUID(), will be provided so the\n sessionmanager can identify the client again.\n\n If the client was opened properly, the function will get its\n sample rate using _jack_get_sample_rate()_ and buffer size\n using _jack_get_buffer_size()_ (both jack/jack.h) and stores\n them in #jackServerSampleRate, Preferences::m_nSampleRate,\n #jackServerBufferSize, and Preferences::m_nBufferSize. In\n addition, it also registers JackAudioDriver::m_processCallback,\n jackDriverSampleRate, jackDriverBufferSize, and\n jackDriverShutdown using _jack_set_process_callback()_,\n _jack_set_sample_rate_callback()_,\n _jack_set_buffer_size_callback()_, and _jack_on_shutdown()_\n (all in jack/jack.h).\n\n Next, two output ports called \"out_L\" and \"out_R\" are\n registered for the client #m_pClient using\n _jack_port_register()_.\n\n If everything worked properly, LASH is used\n (Preferences::useLash()) by the user, and the LashClient is\n LashClient::isConnected() the name of the client will be stored\n in LashClient::jackClientName using\n LashClient::setJackClientName. If JACK session was enabled, the\n jack_session_callback() will be registered using\n _jack_set_session_callback()_ (jack/session.h).\n\n Finally, the function will check whether Hydrogen should be the\n JACK timebase master or not via Preferences::m_bJackMasterMode\n and calls initTimebaseMaster() if its indeed the case.\n\n \n Unused and only present to assure\n compatibility with the JACK API.\n\n \n\n -  __0__ : on success.\n - __-1__ : if the pointer #m_pClient obtained via\n _jack_client_open()_ (jack/jack.h) is 0.\n - __4__ : unable to register the \"out_L\" and/or \"out_R\"\n output ports for the JACK client using\n _jack_port_register()_ (jack/jack.h).\n\nC++: H2Core::JackAudioDriver::init(unsigned int) --> int", pybind11::arg("bufferSize"));
		cl.def("play", (void (H2Core::JackAudioDriver::*)()) &H2Core::JackAudioDriver::play, "Starts the JACK transport.\n\n If the JACK transport was activated in the GUI by clicking\n either the \"J.TRANS\" or \"J.MASTER\" button, the\n _jack_transport_start()_ (jack/transport.h) function will be\n called to start the JACK transport. Else, the internal\n TransportInfo::m_status will be set to TransportInfo::ROLLING\n instead.\n\nC++: H2Core::JackAudioDriver::play() --> void");
		cl.def("stop", (void (H2Core::JackAudioDriver::*)()) &H2Core::JackAudioDriver::stop, "Stops the JACK transport.\n\n If the JACK transport was activated in the GUI by clicking\n either the \"J.TRANS\" or \"J.MASTER\" button, the\n _jack_transport_stop()_ (jack/transport.h) function will be\n called to stop the JACK transport. Else, the internal\n TransportInfo::m_status will be set to TransportInfo::STOPPED\n instead.\n\nC++: H2Core::JackAudioDriver::stop() --> void");
		cl.def("locate", (void (H2Core::JackAudioDriver::*)(unsigned long)) &H2Core::JackAudioDriver::locate, "Re-positions the transport position to \n\n If the Preferences::USE_JACK_TRANSPORT mode is chosen in\n Preferences::m_bJackTransportMode, the\n _jack_transport_locate()_ (jack/transport.h) function will be\n used to request the new transport position. If not, \n will be assigned to TransportInfo::m_nFrames of the local\n instance of the TransportInfo AudioOutput::m_transport.\n\n The new position takes effect in two process cycles during\n which JACK's state will be in JackTransportStarting and the\n transport won't be rolling.\n\n \n Requested new transport position.\n\nC++: H2Core::JackAudioDriver::locate(unsigned long) --> void", pybind11::arg("nFrame"));
		cl.def("updateTransportInfo", (void (H2Core::JackAudioDriver::*)()) &H2Core::JackAudioDriver::updateTransportInfo, "Updating the local instance of the TransportInfo\n AudioOutput::m_transport.\n\n The function queries the transport position and additional\n information from the JACK server, writes them to\n #m_JackTransportPos and in #m_JackTransportState, and updates\n the information stored in AudioOutput::m_transport in case of a\n mismatch.\n\n If #m_JackTransportState is either _JackTransportStopped_ or\n _JackTransportStarting_, transport will be (temporarily)\n stopped - TransportInfo::m_status will be set to\n TransportInfo::STOPPED. If it's _JackTransportRolling_,\n transport will be started - TransportInfo::m_status will be set\n to TransportInfo::ROLLING.\n\n The function will check whether a relocation took place by the\n JACK server and whether the current tempo did\n change with respect to the last transport cycle and updates the\n transport information accordingly.\n\n If Preferences::USE_JACK_TRANSPORT was not selected in\n Preferences::m_bJackTransportMode, the function will return\n without performing any action.\n\nC++: H2Core::JackAudioDriver::updateTransportInfo() --> void");
		cl.def("setBpm", (void (H2Core::JackAudioDriver::*)(float)) &H2Core::JackAudioDriver::setBpm, "Set the tempo stored TransportInfo::m_fBPM of the local\n instance of the TransportInfo AudioOutput::m_transport.\n\n Only sets the tempo to  if its value is at least\n 1. Sometime (especially during the first cycle after locating\n with transport stopped) the JACK server sends some artifacts\n (6.95334e-310) which should not be assigned.\n\n \n new tempo. \n\nC++: H2Core::JackAudioDriver::setBpm(float) --> void", pybind11::arg("fBPM"));
		cl.def("calculateFrameOffset", (void (H2Core::JackAudioDriver::*)(long long)) &H2Core::JackAudioDriver::calculateFrameOffset, "Calculates the difference between the true transport position\n and the internal one.\n\n The internal transport position used in most parts of Hydrogen\n is given in ticks. But since the size of a tick is\n tempo-dependent, passing a tempo marker in the Timeline will\n cause the corresponding internal transport position in frames\n to diverge from the external one by a constant offset. This\n function will calculate and store it in #m_frameOffset.\n\n \n Provides the previous transport position in\n frames prior to the change in tick size. This is required if\n transport is not rolling during the relocation into a region of\n different speed since there is no up-to-date JACK query\n providing these information.\n\nC++: H2Core::JackAudioDriver::calculateFrameOffset(long long) --> void", pybind11::arg("oldFrame"));
		cl.def("initTimebaseMaster", (void (H2Core::JackAudioDriver::*)()) &H2Core::JackAudioDriver::initTimebaseMaster, "Registers Hydrogen as JACK timebase master.\n\n If for some reason registering Hydrogen as timebase master does\n not work, the function sets Preferences::m_bJackMasterMode to\n Preferences::NO_JACK_TIME_MASTER.\n\n If the function is called with Preferences::m_bJackMasterMode\n set to Preferences::NO_JACK_TIME_MASTER,\n releaseTimebaseMaster() will be called instead.\n\nC++: H2Core::JackAudioDriver::initTimebaseMaster() --> void");
		cl.def("releaseTimebaseMaster", (void (H2Core::JackAudioDriver::*)()) &H2Core::JackAudioDriver::releaseTimebaseMaster, "Calls _jack_release_timebase()_ (jack/transport.h) to release\n Hydrogen from the JACK timebase master responsibilities. This\n causes the JackTimebaseCallback() callback function to not be\n called by the JACK server anymore.\n\nC++: H2Core::JackAudioDriver::releaseTimebaseMaster() --> void");
		cl.def("getTimebaseState", (enum H2Core::JackAudioDriver::Timebase (H2Core::JackAudioDriver::*)() const) &H2Core::JackAudioDriver::getTimebaseState, "#m_timebaseState\n\nC++: H2Core::JackAudioDriver::getTimebaseState() const --> enum H2Core::JackAudioDriver::Timebase");
		cl.def_static("jackDriverSampleRate", (int (*)(unsigned int, void *)) &H2Core::JackAudioDriver::jackDriverSampleRate, "Callback function for the JACK audio server to set the sample\n rate #jackServerSampleRate and prints a message to the\n #__INFOLOG, which has to be included via a Logger instance in\n the provided \n\n It gets registered as a callback function of the JACK server in\n JackAudioDriver::init() using\n _jack_set_sample_rate_callback()_.\n\n \n New sample rate. The object has to be of type\n _jack_nframes_t_, which is defined in the jack/types.h header.\n \n\n Object containing a Logger member to display the\n change in the sample rate in its INFOLOG.\n\n \n 0 on success\n\nC++: H2Core::JackAudioDriver::jackDriverSampleRate(unsigned int, void *) --> int", pybind11::arg("nframes"), pybind11::arg("param"));
		cl.def_static("jackDriverBufferSize", (int (*)(unsigned int, void *)) &H2Core::JackAudioDriver::jackDriverBufferSize, "Callback function for the JACK audio server to set the buffer\n size #jackServerBufferSize.\n\n It gets registered as a callback function of the JACK server in\n JackAudioDriver::init() using\n _jack_set_buffer_size_callback()_.\n\n \n New buffer size. The object has to be of type \n which is defined in the jack/types.h header.\n \n\n Not used within the function but kept for compatibility\n reasons since the _JackBufferSizeCallback_ (jack/types.h) requires a\n second input argument  of type _void_, which is a pointer\n supplied by the jack_set_buffer_size_callback() function.\n\n \n 0 on success\n\nC++: H2Core::JackAudioDriver::jackDriverBufferSize(unsigned int, void *) --> int", pybind11::arg("nframes"), pybind11::arg("arg"));
		cl.def("assign", (class H2Core::JackAudioDriver & (H2Core::JackAudioDriver::*)(const class H2Core::JackAudioDriver &)) &H2Core::JackAudioDriver::operator=, "C++: H2Core::JackAudioDriver::operator=(const class H2Core::JackAudioDriver &) --> class H2Core::JackAudioDriver &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
