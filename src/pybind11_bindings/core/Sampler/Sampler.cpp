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
#include <core/Basics/AutomationPath.h> // H2Core::AutomationPath
#include <core/Basics/DrumkitComponent.h> // H2Core::DrumkitComponent
#include <core/Basics/Instrument.h> // H2Core::Instrument
#include <core/Basics/InstrumentList.h> // H2Core::InstrumentList
#include <core/Basics/Note.h> // 
#include <core/Basics/Note.h> // H2Core::Note
#include <core/Basics/Note.h> // H2Core::SelectedLayerInfo
#include <core/Basics/PatternList.h> // H2Core::PatternList
#include <core/Basics/Sample.h> // H2Core::Sample
#include <core/Basics/Song.h> // 
#include <core/Basics/Song.h> // H2Core::Song
#include <core/Helpers/Xml.h> // H2Core::XMLNode
#include <core/IO/AudioOutput.h> // H2Core::AudioOutput
#include <core/Object.h> // H2Core::Object
#include <core/Sampler/Interpolation.h> // H2Core::Interpolation::InterpolateMode
#include <core/Sampler/Sampler.h> // H2Core::Sampler
#include <core/Synth/Synth.h> // H2Core::Synth
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

// H2Core::Sampler file:core/Sampler/Sampler.h line:50
struct PyCallBack_H2Core_Sampler : public H2Core::Sampler {
	using H2Core::Sampler::Sampler;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::Sampler *>(this), "toQString");
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

// H2Core::Synth file:core/Synth/Synth.h line:41
struct PyCallBack_H2Core_Synth : public H2Core::Synth {
	using H2Core::Synth::Synth;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::Synth *>(this), "toQString");
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

// H2Core::ADSR file:core/Basics/Adsr.h line:36
struct PyCallBack_H2Core_ADSR : public H2Core::ADSR {
	using H2Core::ADSR::ADSR;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::ADSR *>(this), "toQString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return ADSR::toQString(a0, a1);
	}
};

void bind_core_Sampler_Sampler(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B646_[H2Core::Sampler] ";
	{ // H2Core::Sampler file:core/Sampler/Sampler.h line:50
		pybind11::class_<H2Core::Sampler, std::shared_ptr<H2Core::Sampler>, PyCallBack_H2Core_Sampler, H2Core::Object> cl(M("H2Core"), "Sampler", "Waveform based sampler.");
		cl.def( pybind11::init( [](){ return new H2Core::Sampler(); }, [](){ return new PyCallBack_H2Core_Sampler(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_Sampler const &o){ return new PyCallBack_H2Core_Sampler(o); } ) );
		cl.def( pybind11::init( [](H2Core::Sampler const &o){ return new H2Core::Sampler(o); } ) );

		pybind11::enum_<H2Core::Sampler::PAN_LAW_TYPES>(cl, "PAN_LAW_TYPES", pybind11::arithmetic(), "PAN LAWS\n The following pan law functions return pan_L (==L, which is the gain for Left channel).\n They assume the fPan argument domain = [-1;1], which is used in Note and Instrument classes.\n----------------------------\n For the right channel use: R(p) == pan_R(p) = pan_L(-p) == L(-p)\n thanks to the Left-Right symmetry.\n--------------------------------------\n The prefix of the function name tells the interpretation of the fPan argument:\n\n \"ratio\" parameter:\n 	 fPan = R/L - 1	if panned to the left,\n 	 fPan = 1 - L/R	if panned to the right.\n\n \"linear\" parameter (arithmetic mean with linear weights):\n	 fPan = ( R - L ) / ( R + L ).\n\n \"polar\" parameter (polar coordinate in LR plane):\n    fPan = 4 / pi * arctan( R/L ) - 1	if L != 0,\n    fPan = 1	if L == 0.\n\n \"quadratic\" parameter (arithmetic mean with squared weights):\n	 fPan = ( R^2 - L^2 ) / ( R^2 + L^2 ).\n\n Note: using a different fPan interpretation makes the output signal more central or more lateral.\n From more central to more lateral:\n \"quadratic\" ---> \"ratio\" ---> \"polar\" ---> \"linear\"\n---------------------------------------------\n After the prefix, the name describes the Image of the pan law in the LR plane.\n (remember that each pan law is a parametrized curve in LR plane.\n E.g.:\n	\"ConstantSum\":\n		it's properly used in an anechoic room, where there are no reflections.\n		Ensures uniform volumes in MONO export,\n		has -6.02 dB center compensation.\n	\"ConstantPower\":\n		probably makes uniform volumes in a common room,\n		has -3.01 dB center compensation.\n	\"ConstantKNorm\":\n		L^k + R^k = const\n		generalises constant sum (k = 1) and constant power (k = 2)\n 	\"StraightPolygonal\":\n		one gain is constant while the other varies.\n		It's ideal as BALANCE law of DUAL-channel track,\n		has 0 dB center compensation.\n------------------------------------------------\n Some pan laws use expensive math functions like pow() and sqrt().\n Pan laws can be approximated by polynomials, e.g. with degree = 2, to adjust the center compensation,\n but then you cannot control the interpretation of the fPan argument exactly.")
			.value("RATIO_STRAIGHT_POLYGONAL", H2Core::Sampler::RATIO_STRAIGHT_POLYGONAL)
			.value("RATIO_CONST_POWER", H2Core::Sampler::RATIO_CONST_POWER)
			.value("RATIO_CONST_SUM", H2Core::Sampler::RATIO_CONST_SUM)
			.value("LINEAR_STRAIGHT_POLYGONAL", H2Core::Sampler::LINEAR_STRAIGHT_POLYGONAL)
			.value("LINEAR_CONST_POWER", H2Core::Sampler::LINEAR_CONST_POWER)
			.value("LINEAR_CONST_SUM", H2Core::Sampler::LINEAR_CONST_SUM)
			.value("POLAR_STRAIGHT_POLYGONAL", H2Core::Sampler::POLAR_STRAIGHT_POLYGONAL)
			.value("POLAR_CONST_POWER", H2Core::Sampler::POLAR_CONST_POWER)
			.value("POLAR_CONST_SUM", H2Core::Sampler::POLAR_CONST_SUM)
			.value("QUADRATIC_STRAIGHT_POLYGONAL", H2Core::Sampler::QUADRATIC_STRAIGHT_POLYGONAL)
			.value("QUADRATIC_CONST_POWER", H2Core::Sampler::QUADRATIC_CONST_POWER)
			.value("QUADRATIC_CONST_SUM", H2Core::Sampler::QUADRATIC_CONST_SUM)
			.value("LINEAR_CONST_K_NORM", H2Core::Sampler::LINEAR_CONST_K_NORM)
			.value("RATIO_CONST_K_NORM", H2Core::Sampler::RATIO_CONST_K_NORM)
			.value("POLAR_CONST_K_NORM", H2Core::Sampler::POLAR_CONST_K_NORM)
			.value("QUADRATIC_CONST_K_NORM", H2Core::Sampler::QUADRATIC_CONST_K_NORM)
			.export_values();

		cl.def_static("class_name", (const char * (*)()) &H2Core::Sampler::class_name, "C++: H2Core::Sampler::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def_static("ratioStraightPolygonalPanLaw", (float (*)(float)) &H2Core::Sampler::ratioStraightPolygonalPanLaw, "C++: H2Core::Sampler::ratioStraightPolygonalPanLaw(float) --> float", pybind11::arg("fPan"));
		cl.def_static("ratioConstPowerPanLaw", (float (*)(float)) &H2Core::Sampler::ratioConstPowerPanLaw, "C++: H2Core::Sampler::ratioConstPowerPanLaw(float) --> float", pybind11::arg("fPan"));
		cl.def_static("ratioConstSumPanLaw", (float (*)(float)) &H2Core::Sampler::ratioConstSumPanLaw, "C++: H2Core::Sampler::ratioConstSumPanLaw(float) --> float", pybind11::arg("fPan"));
		cl.def_static("linearStraightPolygonalPanLaw", (float (*)(float)) &H2Core::Sampler::linearStraightPolygonalPanLaw, "C++: H2Core::Sampler::linearStraightPolygonalPanLaw(float) --> float", pybind11::arg("fPan"));
		cl.def_static("linearConstPowerPanLaw", (float (*)(float)) &H2Core::Sampler::linearConstPowerPanLaw, "C++: H2Core::Sampler::linearConstPowerPanLaw(float) --> float", pybind11::arg("fPan"));
		cl.def_static("linearConstSumPanLaw", (float (*)(float)) &H2Core::Sampler::linearConstSumPanLaw, "C++: H2Core::Sampler::linearConstSumPanLaw(float) --> float", pybind11::arg("fPan"));
		cl.def_static("polarStraightPolygonalPanLaw", (float (*)(float)) &H2Core::Sampler::polarStraightPolygonalPanLaw, "C++: H2Core::Sampler::polarStraightPolygonalPanLaw(float) --> float", pybind11::arg("fPan"));
		cl.def_static("polarConstPowerPanLaw", (float (*)(float)) &H2Core::Sampler::polarConstPowerPanLaw, "C++: H2Core::Sampler::polarConstPowerPanLaw(float) --> float", pybind11::arg("fPan"));
		cl.def_static("polarConstSumPanLaw", (float (*)(float)) &H2Core::Sampler::polarConstSumPanLaw, "C++: H2Core::Sampler::polarConstSumPanLaw(float) --> float", pybind11::arg("fPan"));
		cl.def_static("quadraticStraightPolygonalPanLaw", (float (*)(float)) &H2Core::Sampler::quadraticStraightPolygonalPanLaw, "C++: H2Core::Sampler::quadraticStraightPolygonalPanLaw(float) --> float", pybind11::arg("fPan"));
		cl.def_static("quadraticConstPowerPanLaw", (float (*)(float)) &H2Core::Sampler::quadraticConstPowerPanLaw, "C++: H2Core::Sampler::quadraticConstPowerPanLaw(float) --> float", pybind11::arg("fPan"));
		cl.def_static("quadraticConstSumPanLaw", (float (*)(float)) &H2Core::Sampler::quadraticConstSumPanLaw, "C++: H2Core::Sampler::quadraticConstSumPanLaw(float) --> float", pybind11::arg("fPan"));
		cl.def_static("linearConstKNormPanLaw", (float (*)(float, float)) &H2Core::Sampler::linearConstKNormPanLaw, "C++: H2Core::Sampler::linearConstKNormPanLaw(float, float) --> float", pybind11::arg("fPan"), pybind11::arg("k"));
		cl.def_static("polarConstKNormPanLaw", (float (*)(float, float)) &H2Core::Sampler::polarConstKNormPanLaw, "C++: H2Core::Sampler::polarConstKNormPanLaw(float, float) --> float", pybind11::arg("fPan"), pybind11::arg("k"));
		cl.def_static("ratioConstKNormPanLaw", (float (*)(float, float)) &H2Core::Sampler::ratioConstKNormPanLaw, "C++: H2Core::Sampler::ratioConstKNormPanLaw(float, float) --> float", pybind11::arg("fPan"), pybind11::arg("k"));
		cl.def_static("quadraticConstKNormPanLaw", (float (*)(float, float)) &H2Core::Sampler::quadraticConstKNormPanLaw, "C++: H2Core::Sampler::quadraticConstKNormPanLaw(float, float) --> float", pybind11::arg("fPan"), pybind11::arg("k"));
		cl.def_static("getRatioPan", (float (*)(float, float)) &H2Core::Sampler::getRatioPan, "This function is used to load old version files (v<=1.1).\n It returns the single pan parameter in [-1,1] from the L,R gains\n as it was input from the GUI (up to scale and translation, which is arbitrary).\n Default output is 0 (=central pan) if arguments are invalid.\n-----Historical Note-----\n Originally (version <= 1.0) pan_L,pan_R were actually gains for each channel;\n	\"instrument\" and \"note\" pans were multiplied as in a gain CHAIN in each separate channel,\n	so the chain killed the signal if instrument and note pans were hard-sided to opposites sides!\n In v1.1, pan_L and pan_R were still the members of Note/Instrument representing the pan knob position,\n	still using the ratioStraightPolygonalPanLaw() for the correspondence (up to constant multiplication),\n	but pan_L,pan_R were reconverted to single parameter in the Sampler, and fPan was used in the selected pan law.\n\nC++: H2Core::Sampler::getRatioPan(float, float) --> float", pybind11::arg("fPan_L"), pybind11::arg("fPan_R"));
		cl.def("process", (void (H2Core::Sampler::*)(unsigned int, class H2Core::Song *)) &H2Core::Sampler::process, "C++: H2Core::Sampler::process(unsigned int, class H2Core::Song *) --> void", pybind11::arg("nFrames"), pybind11::arg("pSong"));
		cl.def("noteOn", (void (H2Core::Sampler::*)(class H2Core::Note *)) &H2Core::Sampler::noteOn, "Start playing a note\n\nC++: H2Core::Sampler::noteOn(class H2Core::Note *) --> void", pybind11::arg("pNote"));
		cl.def("noteOff", (void (H2Core::Sampler::*)(class H2Core::Note *)) &H2Core::Sampler::noteOff, "Stop playing a note.\n\nC++: H2Core::Sampler::noteOff(class H2Core::Note *) --> void", pybind11::arg("pNote"));
		cl.def("midiKeyboardNoteOff", (void (H2Core::Sampler::*)(int)) &H2Core::Sampler::midiKeyboardNoteOff, "C++: H2Core::Sampler::midiKeyboardNoteOff(int) --> void", pybind11::arg("key"));
		cl.def("getPlayingNotesNumber", (int (H2Core::Sampler::*)()) &H2Core::Sampler::getPlayingNotesNumber, "C++: H2Core::Sampler::getPlayingNotesNumber() --> int");
		cl.def("setInterpolateMode", (void (H2Core::Sampler::*)(enum H2Core::Interpolation::InterpolateMode)) &H2Core::Sampler::setInterpolateMode, "C++: H2Core::Sampler::setInterpolateMode(enum H2Core::Interpolation::InterpolateMode) --> void", pybind11::arg("mode"));
		cl.def("getInterpolateMode", (enum H2Core::Interpolation::InterpolateMode (H2Core::Sampler::*)()) &H2Core::Sampler::getInterpolateMode, "C++: H2Core::Sampler::getInterpolateMode() --> enum H2Core::Interpolation::InterpolateMode");
		cl.def("reinitializePlaybackTrack", (void (H2Core::Sampler::*)()) &H2Core::Sampler::reinitializePlaybackTrack, "Loading of the playback track.\n\n The playback track is added to\n #__playback_instrument as a new InstrumentLayer\n containing the loaded Sample. If\n Song::__playback_track_filename is empty, the\n layer will be loaded with a nullptr instead.\n\nC++: H2Core::Sampler::reinitializePlaybackTrack() --> void");
		cl.def("assign", (class H2Core::Sampler & (H2Core::Sampler::*)(const class H2Core::Sampler &)) &H2Core::Sampler::operator=, "C++: H2Core::Sampler::operator=(const class H2Core::Sampler &) --> class H2Core::Sampler &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B647_[H2Core::Synth] ";
	{ // H2Core::Synth file:core/Synth/Synth.h line:41
		pybind11::class_<H2Core::Synth, std::shared_ptr<H2Core::Synth>, PyCallBack_H2Core_Synth, H2Core::Object> cl(M("H2Core"), "Synth", "A simple synthetizer...");
		cl.def( pybind11::init( [](){ return new H2Core::Synth(); }, [](){ return new PyCallBack_H2Core_Synth(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_Synth const &o){ return new PyCallBack_H2Core_Synth(o); } ) );
		cl.def( pybind11::init( [](H2Core::Synth const &o){ return new H2Core::Synth(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::Synth::class_name, "C++: H2Core::Synth::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("noteOn", (void (H2Core::Synth::*)(class H2Core::Note *)) &H2Core::Synth::noteOn, "Start playing a note\n\nC++: H2Core::Synth::noteOn(class H2Core::Note *) --> void", pybind11::arg("pNote"));
		cl.def("noteOff", (void (H2Core::Synth::*)(class H2Core::Note *)) &H2Core::Synth::noteOff, "Stop playing a note.\n\nC++: H2Core::Synth::noteOff(class H2Core::Note *) --> void", pybind11::arg("pNote"));
		cl.def("process", (void (H2Core::Synth::*)(unsigned int)) &H2Core::Synth::process, "C++: H2Core::Synth::process(unsigned int) --> void", pybind11::arg("nFrames"));
		cl.def("setAudioOutput", (void (H2Core::Synth::*)(class H2Core::AudioOutput *)) &H2Core::Synth::setAudioOutput, "C++: H2Core::Synth::setAudioOutput(class H2Core::AudioOutput *) --> void", pybind11::arg("pAudioOutput"));
		cl.def("getPlayingNotesNumber", (int (H2Core::Synth::*)()) &H2Core::Synth::getPlayingNotesNumber, "C++: H2Core::Synth::getPlayingNotesNumber() --> int");
		cl.def("assign", (class H2Core::Synth & (H2Core::Synth::*)(const class H2Core::Synth &)) &H2Core::Synth::operator=, "C++: H2Core::Synth::operator=(const class H2Core::Synth &) --> class H2Core::Synth &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B648_[H2Core::ADSR] ";
	{ // H2Core::ADSR file:core/Basics/Adsr.h line:36
		pybind11::class_<H2Core::ADSR, std::shared_ptr<H2Core::ADSR>, PyCallBack_H2Core_ADSR, H2Core::Object> cl(M("H2Core"), "ADSR", "Attack Decay Sustain Release envelope.");
		cl.def( pybind11::init( [](){ return new H2Core::ADSR(); }, [](){ return new PyCallBack_H2Core_ADSR(); } ), "doc");
		cl.def( pybind11::init( [](unsigned int const & a0){ return new H2Core::ADSR(a0); }, [](unsigned int const & a0){ return new PyCallBack_H2Core_ADSR(a0); } ), "doc");
		cl.def( pybind11::init( [](unsigned int const & a0, unsigned int const & a1){ return new H2Core::ADSR(a0, a1); }, [](unsigned int const & a0, unsigned int const & a1){ return new PyCallBack_H2Core_ADSR(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](unsigned int const & a0, unsigned int const & a1, float const & a2){ return new H2Core::ADSR(a0, a1, a2); }, [](unsigned int const & a0, unsigned int const & a1, float const & a2){ return new PyCallBack_H2Core_ADSR(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init<unsigned int, unsigned int, float, unsigned int>(), pybind11::arg("attack"), pybind11::arg("decay"), pybind11::arg("sustain"), pybind11::arg("release") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_ADSR const &o){ return new PyCallBack_H2Core_ADSR(o); } ) );
		cl.def( pybind11::init( [](H2Core::ADSR const &o){ return new H2Core::ADSR(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::ADSR::class_name, "C++: H2Core::ADSR::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("set_attack", (void (H2Core::ADSR::*)(unsigned int)) &H2Core::ADSR::set_attack, "__attack setter\n \n\n the new value\n\nC++: H2Core::ADSR::set_attack(unsigned int) --> void", pybind11::arg("value"));
		cl.def("get_attack", (unsigned int (H2Core::ADSR::*)()) &H2Core::ADSR::get_attack, "__attack accessor \n\nC++: H2Core::ADSR::get_attack() --> unsigned int");
		cl.def("set_decay", (void (H2Core::ADSR::*)(unsigned int)) &H2Core::ADSR::set_decay, "__decay setter\n \n\n the new value\n\nC++: H2Core::ADSR::set_decay(unsigned int) --> void", pybind11::arg("value"));
		cl.def("get_decay", (unsigned int (H2Core::ADSR::*)()) &H2Core::ADSR::get_decay, "__decay accessor \n\nC++: H2Core::ADSR::get_decay() --> unsigned int");
		cl.def("set_sustain", (void (H2Core::ADSR::*)(float)) &H2Core::ADSR::set_sustain, "__sustain setter\n \n\n the new value\n\nC++: H2Core::ADSR::set_sustain(float) --> void", pybind11::arg("value"));
		cl.def("get_sustain", (float (H2Core::ADSR::*)()) &H2Core::ADSR::get_sustain, "__sustain accessor \n\nC++: H2Core::ADSR::get_sustain() --> float");
		cl.def("set_release", (void (H2Core::ADSR::*)(unsigned int)) &H2Core::ADSR::set_release, "__release setter\n \n\n the new value\n\nC++: H2Core::ADSR::set_release(unsigned int) --> void", pybind11::arg("value"));
		cl.def("get_release", (unsigned int (H2Core::ADSR::*)()) &H2Core::ADSR::get_release, "__release accessor \n\nC++: H2Core::ADSR::get_release() --> unsigned int");
		cl.def("attack", (void (H2Core::ADSR::*)()) &H2Core::ADSR::attack, "sets state to ATTACK\n\nC++: H2Core::ADSR::attack() --> void");
		cl.def("get_value", (float (H2Core::ADSR::*)(float)) &H2Core::ADSR::get_value, "compute the value and return it\n \n\n the increment to be added to __ticks\n\nC++: H2Core::ADSR::get_value(float) --> float", pybind11::arg("step"));
		cl.def("release", (float (H2Core::ADSR::*)()) &H2Core::ADSR::release, "sets state to RELEASE,\n returns 0 if the state is IDLE,\n __value if the state is RELEASE,\n set state to RELEASE, save __release_value and return it.\n\nC++: H2Core::ADSR::release() --> float");
		cl.def("toQString", [](H2Core::ADSR const &o, const class QString & a0) -> QString { return o.toQString(a0); }, "", pybind11::arg("sPrefix"));
		cl.def("toQString", (class QString (H2Core::ADSR::*)(const class QString &, bool) const) &H2Core::ADSR::toQString, "Formatted string version for debugging purposes.\n \n\n String prefix which will be added in front of\n every new line\n \n\n Instead of the whole content of all classes\n stored as members just a single unique identifier will be\n displayed without line breaks.\n\n \n String presentation of current object.\n\nC++: H2Core::ADSR::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
		cl.def("assign", (class H2Core::ADSR & (H2Core::ADSR::*)(const class H2Core::ADSR &)) &H2Core::ADSR::operator=, "C++: H2Core::ADSR::operator=(const class H2Core::ADSR &) --> class H2Core::ADSR &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
