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
#include <core/Basics/Pattern.h> // H2Core::Pattern
#include <core/Basics/PatternList.h> // H2Core::PatternList
#include <core/Basics/Sample.h> // 
#include <core/Basics/Sample.h> // H2Core::EnvelopePoint
#include <core/Basics/Sample.h> // H2Core::EnvelopePoint::Comparator
#include <core/Basics/Sample.h> // H2Core::Sample
#include <core/Basics/Sample.h> // H2Core::Sample::Loops
#include <core/Basics/Sample.h> // H2Core::Sample::Rubberband
#include <core/Basics/Song.h> // 
#include <core/Basics/Song.h> // H2Core::Song
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

// H2Core::Sample file:core/Basics/Sample.h line:67
struct PyCallBack_H2Core_Sample : public H2Core::Sample {
	using H2Core::Sample::Sample;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::Sample *>(this), "toQString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return Sample::toQString(a0, a1);
	}
};

// H2Core::Song file:core/Basics/Song.h line:55
struct PyCallBack_H2Core_Song : public H2Core::Song {
	using H2Core::Song::Song;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::Song *>(this), "toQString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return Song::toQString(a0, a1);
	}
};

void bind_core_Basics_Sample(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B686_[H2Core::EnvelopePoint] ";
	{ // H2Core::EnvelopePoint file:core/Basics/Sample.h line:40
		pybind11::class_<H2Core::EnvelopePoint, std::shared_ptr<H2Core::EnvelopePoint>> cl(M("H2Core"), "EnvelopePoint", "an envelope point within a frame ");
		cl.def( pybind11::init( [](){ return new H2Core::EnvelopePoint(); } ) );
		cl.def( pybind11::init( [](H2Core::EnvelopePoint const &o){ return new H2Core::EnvelopePoint(o); } ) );
		cl.def( pybind11::init<int, int>(), pybind11::arg("f"), pybind11::arg("v") );

		cl.def_readwrite("frame", &H2Core::EnvelopePoint::frame);
		cl.def_readwrite("value", &H2Core::EnvelopePoint::value);

		{ // H2Core::EnvelopePoint::Comparator file:core/Basics/Sample.h line:47
			auto & enclosing_class = cl;
			pybind11::class_<H2Core::EnvelopePoint::Comparator, std::shared_ptr<H2Core::EnvelopePoint::Comparator>> cl(enclosing_class, "Comparator", "to be able to sort velocity points vectors ");
			cl.def( pybind11::init( [](){ return new H2Core::EnvelopePoint::Comparator(); } ) );
			cl.def("__call__", (bool (H2Core::EnvelopePoint::Comparator::*)(const class H2Core::EnvelopePoint &, const class H2Core::EnvelopePoint &)) &H2Core::EnvelopePoint::Comparator::operator(), "C++: H2Core::EnvelopePoint::Comparator::operator()(const class H2Core::EnvelopePoint &, const class H2Core::EnvelopePoint &) --> bool", pybind11::arg("a"), pybind11::arg("b"));
		}

	}
	std::cout << "B687_[H2Core::Sample] ";
	{ // H2Core::Sample file:core/Basics/Sample.h line:67
		pybind11::class_<H2Core::Sample, std::shared_ptr<H2Core::Sample>, PyCallBack_H2Core_Sample, H2Core::Object> cl(M("H2Core"), "Sample", "");
		cl.def( pybind11::init( [](){ return new H2Core::Sample(); }, [](){ return new PyCallBack_H2Core_Sample(); } ) );
		cl.def( pybind11::init( [](const class QString & a0){ return new H2Core::Sample(a0); }, [](const class QString & a0){ return new PyCallBack_H2Core_Sample(a0); } ), "doc");
		cl.def( pybind11::init( [](const class QString & a0, int const & a1){ return new H2Core::Sample(a0, a1); }, [](const class QString & a0, int const & a1){ return new PyCallBack_H2Core_Sample(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](const class QString & a0, int const & a1, int const & a2){ return new H2Core::Sample(a0, a1, a2); }, [](const class QString & a0, int const & a1, int const & a2){ return new PyCallBack_H2Core_Sample(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init( [](const class QString & a0, int const & a1, int const & a2, float * a3){ return new H2Core::Sample(a0, a1, a2, a3); }, [](const class QString & a0, int const & a1, int const & a2, float * a3){ return new PyCallBack_H2Core_Sample(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init<const class QString &, int, int, float *, float *>(), pybind11::arg("filepath"), pybind11::arg("frames"), pybind11::arg("sample_rate"), pybind11::arg("data_l"), pybind11::arg("data_r") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_Sample const &o){ return new PyCallBack_H2Core_Sample(o); } ) );
		cl.def( pybind11::init( [](H2Core::Sample const &o){ return new H2Core::Sample(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::Sample::class_name, "C++: H2Core::Sample::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("write", [](H2Core::Sample &o, const class QString & a0) -> bool { return o.write(a0); }, "", pybind11::arg("path"));
		cl.def("write", (bool (H2Core::Sample::*)(const class QString &, int)) &H2Core::Sample::write, "write sample to a file\n \n\n the path to write the sample to\n \n\n the format of the output\n\nC++: H2Core::Sample::write(const class QString &, int) --> bool", pybind11::arg("path"), pybind11::arg("format"));
		cl.def("load", (bool (H2Core::Sample::*)()) &H2Core::Sample::load, "Load the sample stored in #__filepath into\n #__data_l and #__data_r.\n\n It uses libsndfile for reading both the content and\n the metadata of the sample file. The latter is\n stored in #__frames and #__sample_rate.\n\n Hydrogen does only support up to #SAMPLE_CHANNELS\n (two per default) channels in the audio file. If\n there are more, Hydrogen will _NOT_ downmix its\n content but simply extract the first two channels\n and display a warning message. For mono file the\n same content will be assigned to both the left\n (#__data_l) and right channel (#__data_r).\n\n If the total number of frames in the file is larger\n than the maximum value of an `int', the content is\n truncated and a warning log message will be\n displayed.\n\n \n\n		 \n\nC++: H2Core::Sample::load() --> bool");
		cl.def("unload", (void (H2Core::Sample::*)()) &H2Core::Sample::unload, "Flush the current content of the left and right\n channel and the current metadata.\n\nC++: H2Core::Sample::unload() --> void");
		cl.def("apply_loops", (bool (H2Core::Sample::*)(const class H2Core::Sample::Loops &)) &H2Core::Sample::apply_loops, "apply loop transformation to the sample\n \n\n loops parameters\n\nC++: H2Core::Sample::apply_loops(const class H2Core::Sample::Loops &) --> bool", pybind11::arg("lo"));
		cl.def("apply_rubberband", (void (H2Core::Sample::*)(const class H2Core::Sample::Rubberband &)) &H2Core::Sample::apply_rubberband, "apply rubberband transformation to the sample\n \n\n rubberband parameters\n\nC++: H2Core::Sample::apply_rubberband(const class H2Core::Sample::Rubberband &) --> void", pybind11::arg("rb"));
		cl.def("exec_rubberband_cli", (bool (H2Core::Sample::*)(const class H2Core::Sample::Rubberband &)) &H2Core::Sample::exec_rubberband_cli, "call rubberband cli to modify the sample\n \n\n rubberband parameters\n\nC++: H2Core::Sample::exec_rubberband_cli(const class H2Core::Sample::Rubberband &) --> bool", pybind11::arg("rb"));
		cl.def("is_empty", (bool (H2Core::Sample::*)() const) &H2Core::Sample::is_empty, "true if both data channels are null pointers \n\nC++: H2Core::Sample::is_empty() const --> bool");
		cl.def("get_filepath", (const class QString (H2Core::Sample::*)() const) &H2Core::Sample::get_filepath, "#__filepath \n\nC++: H2Core::Sample::get_filepath() const --> const class QString");
		cl.def("get_filename", (const class QString (H2Core::Sample::*)() const) &H2Core::Sample::get_filename, "Filename part of #__filepath \n\nC++: H2Core::Sample::get_filename() const --> const class QString");
		cl.def("set_filepath", (void (H2Core::Sample::*)(const class QString &)) &H2Core::Sample::set_filepath, "the file to load audio data from\n\nC++: H2Core::Sample::set_filepath(const class QString &) --> void", pybind11::arg("filepath"));
		cl.def("set_filename", (void (H2Core::Sample::*)(const class QString &)) &H2Core::Sample::set_filename, "Filename part of #__filepath\n\nC++: H2Core::Sample::set_filename(const class QString &) --> void", pybind11::arg("filename"));
		cl.def("set_frames", (void (H2Core::Sample::*)(int)) &H2Core::Sample::set_frames, "#__frames setter\n \n\n the new value for #__frames\n\nC++: H2Core::Sample::set_frames(int) --> void", pybind11::arg("value"));
		cl.def("get_frames", (int (H2Core::Sample::*)() const) &H2Core::Sample::get_frames, "#__frames accessor \n\nC++: H2Core::Sample::get_frames() const --> int");
		cl.def("set_sample_rate", (void (H2Core::Sample::*)(const int)) &H2Core::Sample::set_sample_rate, "Sets #__sample_rate.\n\nC++: H2Core::Sample::set_sample_rate(const int) --> void", pybind11::arg("sampleRate"));
		cl.def("get_sample_rate", (int (H2Core::Sample::*)() const) &H2Core::Sample::get_sample_rate, "#__sample_rate \n\nC++: H2Core::Sample::get_sample_rate() const --> int");
		cl.def("get_sample_duration", (double (H2Core::Sample::*)() const) &H2Core::Sample::get_sample_duration, "sample duration in seconds \n\nC++: H2Core::Sample::get_sample_duration() const --> double");
		cl.def("get_size", (int (H2Core::Sample::*)() const) &H2Core::Sample::get_size, "data size, which is calculated by\n #__frames time sizeof( float ) * 2 \n\nC++: H2Core::Sample::get_size() const --> int");
		cl.def("get_data_l", (float * (H2Core::Sample::*)() const) &H2Core::Sample::get_data_l, "#__data_l\n\nC++: H2Core::Sample::get_data_l() const --> float *", pybind11::return_value_policy::automatic);
		cl.def("get_data_r", (float * (H2Core::Sample::*)() const) &H2Core::Sample::get_data_r, "#__data_r\n\nC++: H2Core::Sample::get_data_r() const --> float *", pybind11::return_value_policy::automatic);
		cl.def("set_is_modified", (void (H2Core::Sample::*)(bool)) &H2Core::Sample::set_is_modified, "#__is_modified setter\n \n\n the new value for #__is_modified\n\nC++: H2Core::Sample::set_is_modified(bool) --> void", pybind11::arg("value"));
		cl.def("get_is_modified", (bool (H2Core::Sample::*)() const) &H2Core::Sample::get_is_modified, "#__is_modified \n\nC++: H2Core::Sample::get_is_modified() const --> bool");
		cl.def("get_loops", (class H2Core::Sample::Loops (H2Core::Sample::*)() const) &H2Core::Sample::get_loops, "#__loops parameters \n\nC++: H2Core::Sample::get_loops() const --> class H2Core::Sample::Loops");
		cl.def("get_rubberband", (class H2Core::Sample::Rubberband (H2Core::Sample::*)() const) &H2Core::Sample::get_rubberband, "#__rubberband parameters \n\nC++: H2Core::Sample::get_rubberband() const --> class H2Core::Sample::Rubberband");
		cl.def_static("parse_loop_mode", (enum H2Core::Sample::Loops::LoopMode (*)(const class QString &)) &H2Core::Sample::parse_loop_mode, "parse the given string and rturn the corresponding loop_mode\n \n\n the loop mode text to be parsed\n\nC++: H2Core::Sample::parse_loop_mode(const class QString &) --> enum H2Core::Sample::Loops::LoopMode", pybind11::arg("string"));
		cl.def("get_loop_mode_string", (class QString (H2Core::Sample::*)() const) &H2Core::Sample::get_loop_mode_string, "mode member of #__loops as a string \n\nC++: H2Core::Sample::get_loop_mode_string() const --> class QString");
		cl.def("toQString", [](H2Core::Sample const &o, const class QString & a0) -> QString { return o.toQString(a0); }, "", pybind11::arg("sPrefix"));
		cl.def("toQString", (class QString (H2Core::Sample::*)(const class QString &, bool) const) &H2Core::Sample::toQString, "Formatted string version for debugging purposes.\n \n\n String prefix which will be added in front of\n every new line\n \n\n Instead of the whole content of all classes\n stored as members just a single unique identifier will be\n displayed without line breaks.\n\n \n String presentation of current object.\n\nC++: H2Core::Sample::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
		cl.def("assign", (class H2Core::Sample & (H2Core::Sample::*)(const class H2Core::Sample &)) &H2Core::Sample::operator=, "C++: H2Core::Sample::operator=(const class H2Core::Sample &) --> class H2Core::Sample &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // H2Core::Sample::Loops file:core/Basics/Sample.h line:77
			auto & enclosing_class = cl;
			pybind11::class_<H2Core::Sample::Loops, std::shared_ptr<H2Core::Sample::Loops>> cl(enclosing_class, "Loops", "set of loop configuration flags ");
			cl.def( pybind11::init( [](){ return new H2Core::Sample::Loops(); } ) );
			cl.def( pybind11::init<const class H2Core::Sample::Loops *>(), pybind11::arg("other") );

			cl.def( pybind11::init( [](H2Core::Sample::Loops const &o){ return new H2Core::Sample::Loops(o); } ) );

			pybind11::enum_<H2Core::Sample::Loops::LoopMode>(cl, "LoopMode", pybind11::arithmetic(), "possible sample editing loop mode ")
				.value("FORWARD", H2Core::Sample::Loops::FORWARD)
				.value("REVERSE", H2Core::Sample::Loops::REVERSE)
				.value("PINGPONG", H2Core::Sample::Loops::PINGPONG)
				.export_values();

			cl.def_readwrite("start_frame", &H2Core::Sample::Loops::start_frame);
			cl.def_readwrite("loop_frame", &H2Core::Sample::Loops::loop_frame);
			cl.def_readwrite("end_frame", &H2Core::Sample::Loops::end_frame);
			cl.def_readwrite("count", &H2Core::Sample::Loops::count);
			cl.def_readwrite("mode", &H2Core::Sample::Loops::mode);
			cl.def("__eq__", (bool (H2Core::Sample::Loops::*)(const class H2Core::Sample::Loops &) const) &H2Core::Sample::Loops::operator==, "equal to operator \n\nC++: H2Core::Sample::Loops::operator==(const class H2Core::Sample::Loops &) const --> bool", pybind11::arg("b"));
			cl.def("toQString", (class QString (H2Core::Sample::Loops::*)(const class QString &, bool) const) &H2Core::Sample::Loops::toQString, "C++: H2Core::Sample::Loops::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
			cl.def("assign", (class H2Core::Sample::Loops & (H2Core::Sample::Loops::*)(const class H2Core::Sample::Loops &)) &H2Core::Sample::Loops::operator=, "C++: H2Core::Sample::Loops::operator=(const class H2Core::Sample::Loops &) --> class H2Core::Sample::Loops &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		}

		{ // H2Core::Sample::Rubberband file:core/Basics/Sample.h line:109
			auto & enclosing_class = cl;
			pybind11::class_<H2Core::Sample::Rubberband, std::shared_ptr<H2Core::Sample::Rubberband>> cl(enclosing_class, "Rubberband", "set of rubberband configuration flags ");
			cl.def( pybind11::init( [](){ return new H2Core::Sample::Rubberband(); } ) );
			cl.def( pybind11::init<const class H2Core::Sample::Rubberband *>(), pybind11::arg("other") );

			cl.def( pybind11::init( [](H2Core::Sample::Rubberband const &o){ return new H2Core::Sample::Rubberband(o); } ) );
			cl.def_readwrite("use", &H2Core::Sample::Rubberband::use);
			cl.def_readwrite("divider", &H2Core::Sample::Rubberband::divider);
			cl.def_readwrite("pitch", &H2Core::Sample::Rubberband::pitch);
			cl.def_readwrite("c_settings", &H2Core::Sample::Rubberband::c_settings);
			cl.def("__eq__", (bool (H2Core::Sample::Rubberband::*)(const class H2Core::Sample::Rubberband &) const) &H2Core::Sample::Rubberband::operator==, "equal to operator \n\nC++: H2Core::Sample::Rubberband::operator==(const class H2Core::Sample::Rubberband &) const --> bool", pybind11::arg("b"));
			cl.def("toQString", (class QString (H2Core::Sample::Rubberband::*)(const class QString &, bool) const) &H2Core::Sample::Rubberband::toQString, "C++: H2Core::Sample::Rubberband::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
			cl.def("assign", (class H2Core::Sample::Rubberband & (H2Core::Sample::Rubberband::*)(const class H2Core::Sample::Rubberband &)) &H2Core::Sample::Rubberband::operator=, "C++: H2Core::Sample::Rubberband::operator=(const class H2Core::Sample::Rubberband &) --> class H2Core::Sample::Rubberband &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		}

	}
	std::cout << "B688_[H2Core::Song] ";
	{ // H2Core::Song file:core/Basics/Song.h line:55
		pybind11::class_<H2Core::Song, std::shared_ptr<H2Core::Song>, PyCallBack_H2Core_Song, H2Core::Object> cl(M("H2Core"), "Song", "	Song class");
		cl.def( pybind11::init<const class QString &, const class QString &, float, float>(), pybind11::arg("sName"), pybind11::arg("sAuthor"), pybind11::arg("fBpm"), pybind11::arg("fVolume") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_Song const &o){ return new PyCallBack_H2Core_Song(o); } ) );
		cl.def( pybind11::init( [](H2Core::Song const &o){ return new H2Core::Song(o); } ) );

		pybind11::enum_<H2Core::Song::SongMode>(cl, "SongMode", pybind11::arithmetic(), "")
			.value("PATTERN_MODE", H2Core::Song::PATTERN_MODE)
			.value("SONG_MODE", H2Core::Song::SONG_MODE)
			.export_values();


		pybind11::enum_<H2Core::Song::ActionMode>(cl, "ActionMode", "Defines the type of user interaction experienced in the \n			SongEditor.")
			.value("selectMode", H2Core::Song::ActionMode::selectMode)
			.value("drawMode", H2Core::Song::ActionMode::drawMode);

		cl.def_static("class_name", (const char * (*)()) &H2Core::Song::class_name, "C++: H2Core::Song::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def_static("getEmptySong", (class H2Core::Song * (*)()) &H2Core::Song::getEmptySong, "C++: H2Core::Song::getEmptySong() --> class H2Core::Song *", pybind11::return_value_policy::automatic);
		cl.def_static("getDefaultSong", (class H2Core::Song * (*)()) &H2Core::Song::getDefaultSong, "C++: H2Core::Song::getDefaultSong() --> class H2Core::Song *", pybind11::return_value_policy::automatic);
		cl.def("getIsMuted", (bool (H2Core::Song::*)() const) &H2Core::Song::getIsMuted, "C++: H2Core::Song::getIsMuted() const --> bool");
		cl.def("setIsMuted", (void (H2Core::Song::*)(bool)) &H2Core::Song::setIsMuted, "C++: H2Core::Song::setIsMuted(bool) --> void", pybind11::arg("bIsMuted"));
		cl.def("getResolution", (unsigned int (H2Core::Song::*)() const) &H2Core::Song::getResolution, "C++: H2Core::Song::getResolution() const --> unsigned int");
		cl.def("setResolution", (void (H2Core::Song::*)(unsigned int)) &H2Core::Song::setResolution, "C++: H2Core::Song::setResolution(unsigned int) --> void", pybind11::arg("resolution"));
		cl.def("getBpm", (float (H2Core::Song::*)() const) &H2Core::Song::getBpm, "C++: H2Core::Song::getBpm() const --> float");
		cl.def("setBpm", (void (H2Core::Song::*)(float)) &H2Core::Song::setBpm, "C++: H2Core::Song::setBpm(float) --> void", pybind11::arg("fBpm"));
		cl.def("getName", (const class QString & (H2Core::Song::*)() const) &H2Core::Song::getName, "C++: H2Core::Song::getName() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("setName", (void (H2Core::Song::*)(const class QString &)) &H2Core::Song::setName, "C++: H2Core::Song::setName(const class QString &) --> void", pybind11::arg("sName"));
		cl.def("setVolume", (void (H2Core::Song::*)(float)) &H2Core::Song::setVolume, "C++: H2Core::Song::setVolume(float) --> void", pybind11::arg("fVolume"));
		cl.def("getVolume", (float (H2Core::Song::*)() const) &H2Core::Song::getVolume, "C++: H2Core::Song::getVolume() const --> float");
		cl.def("setMetronomeVolume", (void (H2Core::Song::*)(float)) &H2Core::Song::setMetronomeVolume, "C++: H2Core::Song::setMetronomeVolume(float) --> void", pybind11::arg("fVolume"));
		cl.def("getMetronomeVolume", (float (H2Core::Song::*)() const) &H2Core::Song::getMetronomeVolume, "C++: H2Core::Song::getMetronomeVolume() const --> float");
		cl.def("getPatternList", (class H2Core::PatternList * (H2Core::Song::*)() const) &H2Core::Song::getPatternList, "C++: H2Core::Song::getPatternList() const --> class H2Core::PatternList *", pybind11::return_value_policy::automatic);
		cl.def("setPatternList", (void (H2Core::Song::*)(class H2Core::PatternList *)) &H2Core::Song::setPatternList, "C++: H2Core::Song::setPatternList(class H2Core::PatternList *) --> void", pybind11::arg("pList"));
		cl.def("lengthInTicks", (int (H2Core::Song::*)() const) &H2Core::Song::lengthInTicks, "get the length of the song, in tick units \n\nC++: H2Core::Song::lengthInTicks() const --> int");
		cl.def_static("load", (class H2Core::Song * (*)(const class QString &)) &H2Core::Song::load, "C++: H2Core::Song::load(const class QString &) --> class H2Core::Song *", pybind11::return_value_policy::automatic, pybind11::arg("sFilename"));
		cl.def("save", (bool (H2Core::Song::*)(const class QString &)) &H2Core::Song::save, "C++: H2Core::Song::save(const class QString &) --> bool", pybind11::arg("sFilename"));
		cl.def("getInstrumentList", (class H2Core::InstrumentList * (H2Core::Song::*)() const) &H2Core::Song::getInstrumentList, "C++: H2Core::Song::getInstrumentList() const --> class H2Core::InstrumentList *", pybind11::return_value_policy::automatic);
		cl.def("setInstrumentList", (void (H2Core::Song::*)(class H2Core::InstrumentList *)) &H2Core::Song::setInstrumentList, "C++: H2Core::Song::setInstrumentList(class H2Core::InstrumentList *) --> void", pybind11::arg("pList"));
		cl.def("setNotes", (void (H2Core::Song::*)(const class QString &)) &H2Core::Song::setNotes, "C++: H2Core::Song::setNotes(const class QString &) --> void", pybind11::arg("sNotes"));
		cl.def("getNotes", (const class QString & (H2Core::Song::*)() const) &H2Core::Song::getNotes, "C++: H2Core::Song::getNotes() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("setLicense", (void (H2Core::Song::*)(const class QString &)) &H2Core::Song::setLicense, "C++: H2Core::Song::setLicense(const class QString &) --> void", pybind11::arg("sLicense"));
		cl.def("getLicense", (const class QString & (H2Core::Song::*)() const) &H2Core::Song::getLicense, "C++: H2Core::Song::getLicense() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("setAuthor", (void (H2Core::Song::*)(const class QString &)) &H2Core::Song::setAuthor, "C++: H2Core::Song::setAuthor(const class QString &) --> void", pybind11::arg("sAuthor"));
		cl.def("getAuthor", (const class QString & (H2Core::Song::*)() const) &H2Core::Song::getAuthor, "C++: H2Core::Song::getAuthor() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("getFilename", (const class QString & (H2Core::Song::*)() const) &H2Core::Song::getFilename, "C++: H2Core::Song::getFilename() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("setFilename", (void (H2Core::Song::*)(const class QString &)) &H2Core::Song::setFilename, "C++: H2Core::Song::setFilename(const class QString &) --> void", pybind11::arg("sFilename"));
		cl.def("getIsLoopEnabled", (bool (H2Core::Song::*)() const) &H2Core::Song::getIsLoopEnabled, "C++: H2Core::Song::getIsLoopEnabled() const --> bool");
		cl.def("setIsLoopEnabled", (void (H2Core::Song::*)(bool)) &H2Core::Song::setIsLoopEnabled, "C++: H2Core::Song::setIsLoopEnabled(bool) --> void", pybind11::arg("bEnabled"));
		cl.def("getHumanizeTimeValue", (float (H2Core::Song::*)() const) &H2Core::Song::getHumanizeTimeValue, "C++: H2Core::Song::getHumanizeTimeValue() const --> float");
		cl.def("setHumanizeTimeValue", (void (H2Core::Song::*)(float)) &H2Core::Song::setHumanizeTimeValue, "C++: H2Core::Song::setHumanizeTimeValue(float) --> void", pybind11::arg("fValue"));
		cl.def("getHumanizeVelocityValue", (float (H2Core::Song::*)() const) &H2Core::Song::getHumanizeVelocityValue, "C++: H2Core::Song::getHumanizeVelocityValue() const --> float");
		cl.def("setHumanizeVelocityValue", (void (H2Core::Song::*)(float)) &H2Core::Song::setHumanizeVelocityValue, "C++: H2Core::Song::setHumanizeVelocityValue(float) --> void", pybind11::arg("fValue"));
		cl.def("getSwingFactor", (float (H2Core::Song::*)() const) &H2Core::Song::getSwingFactor, "C++: H2Core::Song::getSwingFactor() const --> float");
		cl.def("setSwingFactor", (void (H2Core::Song::*)(float)) &H2Core::Song::setSwingFactor, "C++: H2Core::Song::setSwingFactor(float) --> void", pybind11::arg("fFactor"));
		cl.def("getMode", (enum H2Core::Song::SongMode (H2Core::Song::*)() const) &H2Core::Song::getMode, "C++: H2Core::Song::getMode() const --> enum H2Core::Song::SongMode");
		cl.def("setMode", (void (H2Core::Song::*)(enum H2Core::Song::SongMode)) &H2Core::Song::setMode, "C++: H2Core::Song::setMode(enum H2Core::Song::SongMode) --> void", pybind11::arg("mode"));
		cl.def("setIsModified", (void (H2Core::Song::*)(bool)) &H2Core::Song::setIsModified, "C++: H2Core::Song::setIsModified(bool) --> void", pybind11::arg("bIsModified"));
		cl.def("getIsModified", (bool (H2Core::Song::*)() const) &H2Core::Song::getIsModified, "C++: H2Core::Song::getIsModified() const --> bool");
		cl.def("getVelocityAutomationPath", (class H2Core::AutomationPath * (H2Core::Song::*)() const) &H2Core::Song::getVelocityAutomationPath, "C++: H2Core::Song::getVelocityAutomationPath() const --> class H2Core::AutomationPath *", pybind11::return_value_policy::automatic);
		cl.def("getComponent", (class H2Core::DrumkitComponent * (H2Core::Song::*)(int) const) &H2Core::Song::getComponent, "C++: H2Core::Song::getComponent(int) const --> class H2Core::DrumkitComponent *", pybind11::return_value_policy::automatic, pybind11::arg("nID"));
		cl.def("readTempPatternList", (void (H2Core::Song::*)(const class QString &)) &H2Core::Song::readTempPatternList, "C++: H2Core::Song::readTempPatternList(const class QString &) --> void", pybind11::arg("sFilename"));
		cl.def("writeTempPatternList", (bool (H2Core::Song::*)(const class QString &)) &H2Core::Song::writeTempPatternList, "C++: H2Core::Song::writeTempPatternList(const class QString &) --> bool", pybind11::arg("sFilename"));
		cl.def("copyInstrumentLineToString", (class QString (H2Core::Song::*)(int, int)) &H2Core::Song::copyInstrumentLineToString, "C++: H2Core::Song::copyInstrumentLineToString(int, int) --> class QString", pybind11::arg("nSelectedPattern"), pybind11::arg("selectedInstrument"));
		cl.def("getLatestRoundRobin", (int (H2Core::Song::*)(float)) &H2Core::Song::getLatestRoundRobin, "C++: H2Core::Song::getLatestRoundRobin(float) --> int", pybind11::arg("fStartVelocity"));
		cl.def("setLatestRoundRobin", (void (H2Core::Song::*)(float, int)) &H2Core::Song::setLatestRoundRobin, "C++: H2Core::Song::setLatestRoundRobin(float, int) --> void", pybind11::arg("fStartVelocity"), pybind11::arg("nLatestRoundRobin"));
		cl.def("getPlaybackTrackFilename", (const class QString & (H2Core::Song::*)() const) &H2Core::Song::getPlaybackTrackFilename, "#m_sPlaybackTrackFilename \n\nC++: H2Core::Song::getPlaybackTrackFilename() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("setPlaybackTrackFilename", (void (H2Core::Song::*)(const class QString)) &H2Core::Song::setPlaybackTrackFilename, "Sets #m_sPlaybackTrackFilename. \n\nC++: H2Core::Song::setPlaybackTrackFilename(const class QString) --> void", pybind11::arg("sFilename"));
		cl.def("getPlaybackTrackEnabled", (bool (H2Core::Song::*)() const) &H2Core::Song::getPlaybackTrackEnabled, "#m_bPlaybackTrackEnabled \n\nC++: H2Core::Song::getPlaybackTrackEnabled() const --> bool");
		cl.def("setPlaybackTrackEnabled", (bool (H2Core::Song::*)(const bool)) &H2Core::Song::setPlaybackTrackEnabled, "Specifies whether a playback track should be used.\n\n If #m_sPlaybackTrackFilename is set to nullptr,\n #m_bPlaybackTrackEnabled will be set to false\n regardless of the choice in \n\n \n Sets #m_bPlaybackTrackEnabled. \n\nC++: H2Core::Song::setPlaybackTrackEnabled(const bool) --> bool", pybind11::arg("bEnabled"));
		cl.def("getPlaybackTrackVolume", (float (H2Core::Song::*)() const) &H2Core::Song::getPlaybackTrackVolume, "#m_fPlaybackTrackVolume \n\nC++: H2Core::Song::getPlaybackTrackVolume() const --> float");
		cl.def("setPlaybackTrackVolume", (void (H2Core::Song::*)(const float)) &H2Core::Song::setPlaybackTrackVolume, "Sets #m_fPlaybackTrackVolume. \n\nC++: H2Core::Song::setPlaybackTrackVolume(const float) --> void", pybind11::arg("fVolume"));
		cl.def("getActionMode", (enum H2Core::Song::ActionMode (H2Core::Song::*)() const) &H2Core::Song::getActionMode, "C++: H2Core::Song::getActionMode() const --> enum H2Core::Song::ActionMode");
		cl.def("setActionMode", (void (H2Core::Song::*)(const enum H2Core::Song::ActionMode)) &H2Core::Song::setActionMode, "C++: H2Core::Song::setActionMode(const enum H2Core::Song::ActionMode) --> void", pybind11::arg("actionMode"));
		cl.def("hasMissingSamples", (bool (H2Core::Song::*)() const) &H2Core::Song::hasMissingSamples, "Song was incompletely loaded from file (missing samples)\n\nC++: H2Core::Song::hasMissingSamples() const --> bool");
		cl.def("clearMissingSamples", (void (H2Core::Song::*)()) &H2Core::Song::clearMissingSamples, "C++: H2Core::Song::clearMissingSamples() --> void");
		cl.def("setPanLawType", (void (H2Core::Song::*)(int)) &H2Core::Song::setPanLawType, "C++: H2Core::Song::setPanLawType(int) --> void", pybind11::arg("nPanLawType"));
		cl.def("getPanLawType", (int (H2Core::Song::*)() const) &H2Core::Song::getPanLawType, "C++: H2Core::Song::getPanLawType() const --> int");
		cl.def("setPanLawKNorm", (void (H2Core::Song::*)(float)) &H2Core::Song::setPanLawKNorm, "C++: H2Core::Song::setPanLawKNorm(float) --> void", pybind11::arg("fKNorm"));
		cl.def("getPanLawKNorm", (float (H2Core::Song::*)() const) &H2Core::Song::getPanLawKNorm, "C++: H2Core::Song::getPanLawKNorm() const --> float");
		cl.def("toQString", [](H2Core::Song const &o, const class QString & a0) -> QString { return o.toQString(a0); }, "", pybind11::arg("sPrefix"));
		cl.def("toQString", (class QString (H2Core::Song::*)(const class QString &, bool) const) &H2Core::Song::toQString, "Formatted string version for debugging purposes.\n \n\n String prefix which will be added in front of\n every new line\n \n\n Instead of the whole content of all classes\n stored as members just a single unique identifier will be\n displayed without line breaks.\n\n \n String presentation of current object.\n\nC++: H2Core::Song::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
		cl.def("assign", (class H2Core::Song & (H2Core::Song::*)(const class H2Core::Song &)) &H2Core::Song::operator=, "C++: H2Core::Song::operator=(const class H2Core::Song &) --> class H2Core::Song &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
