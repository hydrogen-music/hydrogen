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
#include <core/Object.h> // H2Core::Object
#include <core/Smf/SMF.h> // H2Core::SMF
#include <core/Smf/SMF.h> // H2Core::SMFHeader
#include <core/Smf/SMF.h> // H2Core::SMFTrack
#include <core/Smf/SMF.h> // H2Core::SMFWriter
#include <core/Smf/SMFEvent.h> // H2Core::SMFBase
#include <core/Smf/SMFEvent.h> // H2Core::SMFBuffer
#include <core/Smf/SMFEvent.h> // H2Core::SMFCopyRightNoticeMetaEvent
#include <core/Smf/SMFEvent.h> // H2Core::SMFEvent
#include <core/Smf/SMFEvent.h> // H2Core::SMFEventType
#include <core/Smf/SMFEvent.h> // H2Core::SMFMetaEventType
#include <core/Smf/SMFEvent.h> // H2Core::SMFNoteOffEvent
#include <core/Smf/SMFEvent.h> // H2Core::SMFNoteOnEvent
#include <core/Smf/SMFEvent.h> // H2Core::SMFSetTempoMetaEvent
#include <core/Smf/SMFEvent.h> // H2Core::SMFTimeSignatureMetaEvent
#include <core/Smf/SMFEvent.h> // H2Core::SMFTrackNameMetaEvent
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

// H2Core::SMFBuffer file:core/Smf/SMFEvent.h line:32
struct PyCallBack_H2Core_SMFBuffer : public H2Core::SMFBuffer {
	using H2Core::SMFBuffer::SMFBuffer;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMFBuffer *>(this), "toQString");
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

// H2Core::SMFTrackNameMetaEvent file:core/Smf/SMFEvent.h line:98
struct PyCallBack_H2Core_SMFTrackNameMetaEvent : public H2Core::SMFTrackNameMetaEvent {
	using H2Core::SMFTrackNameMetaEvent::SMFTrackNameMetaEvent;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMFTrackNameMetaEvent *>(this), "toQString");
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

// H2Core::SMFSetTempoMetaEvent file:core/Smf/SMFEvent.h line:112
struct PyCallBack_H2Core_SMFSetTempoMetaEvent : public H2Core::SMFSetTempoMetaEvent {
	using H2Core::SMFSetTempoMetaEvent::SMFSetTempoMetaEvent;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMFSetTempoMetaEvent *>(this), "toQString");
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

// H2Core::SMFCopyRightNoticeMetaEvent file:core/Smf/SMFEvent.h line:126
struct PyCallBack_H2Core_SMFCopyRightNoticeMetaEvent : public H2Core::SMFCopyRightNoticeMetaEvent {
	using H2Core::SMFCopyRightNoticeMetaEvent::SMFCopyRightNoticeMetaEvent;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMFCopyRightNoticeMetaEvent *>(this), "toQString");
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

// H2Core::SMFTimeSignatureMetaEvent file:core/Smf/SMFEvent.h line:140
struct PyCallBack_H2Core_SMFTimeSignatureMetaEvent : public H2Core::SMFTimeSignatureMetaEvent {
	using H2Core::SMFTimeSignatureMetaEvent::SMFTimeSignatureMetaEvent;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMFTimeSignatureMetaEvent *>(this), "toQString");
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

// H2Core::SMFNoteOnEvent file:core/Smf/SMFEvent.h line:154
struct PyCallBack_H2Core_SMFNoteOnEvent : public H2Core::SMFNoteOnEvent {
	using H2Core::SMFNoteOnEvent::SMFNoteOnEvent;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMFNoteOnEvent *>(this), "toQString");
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

// H2Core::SMFNoteOffEvent file:core/Smf/SMFEvent.h line:170
struct PyCallBack_H2Core_SMFNoteOffEvent : public H2Core::SMFNoteOffEvent {
	using H2Core::SMFNoteOffEvent::SMFNoteOffEvent;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMFNoteOffEvent *>(this), "toQString");
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

// H2Core::SMFHeader file:core/Smf/SMF.h line:40
struct PyCallBack_H2Core_SMFHeader : public H2Core::SMFHeader {
	using H2Core::SMFHeader::SMFHeader;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMFHeader *>(this), "toQString");
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

// H2Core::SMFTrack file:core/Smf/SMF.h line:58
struct PyCallBack_H2Core_SMFTrack : public H2Core::SMFTrack {
	using H2Core::SMFTrack::SMFTrack;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMFTrack *>(this), "toQString");
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

// H2Core::SMF file:core/Smf/SMF.h line:76
struct PyCallBack_H2Core_SMF : public H2Core::SMF {
	using H2Core::SMF::SMF;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMF *>(this), "toQString");
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

void bind_core_Smf_SMFEvent(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B735_[H2Core::SMFBuffer] ";
	{ // H2Core::SMFBuffer file:core/Smf/SMFEvent.h line:32
		pybind11::class_<H2Core::SMFBuffer, std::shared_ptr<H2Core::SMFBuffer>, PyCallBack_H2Core_SMFBuffer, H2Core::Object> cl(M("H2Core"), "SMFBuffer", "");
		cl.def( pybind11::init( [](){ return new H2Core::SMFBuffer(); }, [](){ return new PyCallBack_H2Core_SMFBuffer(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_SMFBuffer const &o){ return new PyCallBack_H2Core_SMFBuffer(o); } ) );
		cl.def( pybind11::init( [](H2Core::SMFBuffer const &o){ return new H2Core::SMFBuffer(o); } ) );
		cl.def_readwrite("m_buffer", &H2Core::SMFBuffer::m_buffer);
		cl.def_static("class_name", (const char * (*)()) &H2Core::SMFBuffer::class_name, "C++: H2Core::SMFBuffer::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("writeByte", (void (H2Core::SMFBuffer::*)(short)) &H2Core::SMFBuffer::writeByte, "C++: H2Core::SMFBuffer::writeByte(short) --> void", pybind11::arg("nByte"));
		cl.def("writeWord", (void (H2Core::SMFBuffer::*)(int)) &H2Core::SMFBuffer::writeWord, "C++: H2Core::SMFBuffer::writeWord(int) --> void", pybind11::arg("nVal"));
		cl.def("writeDWord", (void (H2Core::SMFBuffer::*)(long)) &H2Core::SMFBuffer::writeDWord, "C++: H2Core::SMFBuffer::writeDWord(long) --> void", pybind11::arg("nVal"));
		cl.def("writeString", (void (H2Core::SMFBuffer::*)(const class QString &)) &H2Core::SMFBuffer::writeString, "C++: H2Core::SMFBuffer::writeString(const class QString &) --> void", pybind11::arg("sMsg"));
		cl.def("writeVarLen", (void (H2Core::SMFBuffer::*)(long)) &H2Core::SMFBuffer::writeVarLen, "C++: H2Core::SMFBuffer::writeVarLen(long) --> void", pybind11::arg("nVal"));
		cl.def("assign", (class H2Core::SMFBuffer & (H2Core::SMFBuffer::*)(const class H2Core::SMFBuffer &)) &H2Core::SMFBuffer::operator=, "C++: H2Core::SMFBuffer::operator=(const class H2Core::SMFBuffer &) --> class H2Core::SMFBuffer &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B736_[H2Core::SMFEventType] ";
	// H2Core::SMFEventType file:core/Smf/SMFEvent.h line:53
	pybind11::enum_<H2Core::SMFEventType>(M("H2Core"), "SMFEventType", pybind11::arithmetic(), "")
		.value("NOTE_OFF", H2Core::NOTE_OFF)
		.value("NOTE_ON", H2Core::NOTE_ON)
		.export_values();

;

	std::cout << "B737_[H2Core::SMFMetaEventType] ";
	// H2Core::SMFMetaEventType file:core/Smf/SMFEvent.h line:60
	pybind11::enum_<H2Core::SMFMetaEventType>(M("H2Core"), "SMFMetaEventType", pybind11::arithmetic(), "")
		.value("SEQUENCE_NUMBER", H2Core::SEQUENCE_NUMBER)
		.value("TEXT_EVENT", H2Core::TEXT_EVENT)
		.value("COPYRIGHT_NOTICE", H2Core::COPYRIGHT_NOTICE)
		.value("TRACK_NAME", H2Core::TRACK_NAME)
		.value("INSTRUMENT_NAME", H2Core::INSTRUMENT_NAME)
		.value("LYRIC", H2Core::LYRIC)
		.value("MARKER", H2Core::MARKER)
		.value("CUE_POINT", H2Core::CUE_POINT)
		.value("END_OF_TRACK", H2Core::END_OF_TRACK)
		.value("SET_TEMPO", H2Core::SET_TEMPO)
		.value("TIME_SIGNATURE", H2Core::TIME_SIGNATURE)
		.value("KEY_SIGNATURE", H2Core::KEY_SIGNATURE)
		.export_values();

;

	std::cout << "B738_[H2Core::SMFBase] ";
	{ // H2Core::SMFBase file:core/Smf/SMFEvent.h line:76
		pybind11::class_<H2Core::SMFBase, std::shared_ptr<H2Core::SMFBase>> cl(M("H2Core"), "SMFBase", "");
		cl.def("assign", (class H2Core::SMFBase & (H2Core::SMFBase::*)(const class H2Core::SMFBase &)) &H2Core::SMFBase::operator=, "C++: H2Core::SMFBase::operator=(const class H2Core::SMFBase &) --> class H2Core::SMFBase &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B739_[H2Core::SMFEvent] ";
	{ // H2Core::SMFEvent file:core/Smf/SMFEvent.h line:85
		pybind11::class_<H2Core::SMFEvent, std::shared_ptr<H2Core::SMFEvent>, H2Core::SMFBase, H2Core::Object> cl(M("H2Core"), "SMFEvent", "");
		cl.def_readwrite("m_nTicks", &H2Core::SMFEvent::m_nTicks);
		cl.def_readwrite("m_nDeltaTime", &H2Core::SMFEvent::m_nDeltaTime);
		cl.def_static("class_name", (const char * (*)()) &H2Core::SMFEvent::class_name, "C++: H2Core::SMFEvent::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class H2Core::SMFEvent & (H2Core::SMFEvent::*)(const class H2Core::SMFEvent &)) &H2Core::SMFEvent::operator=, "C++: H2Core::SMFEvent::operator=(const class H2Core::SMFEvent &) --> class H2Core::SMFEvent &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B740_[H2Core::SMFTrackNameMetaEvent] ";
	{ // H2Core::SMFTrackNameMetaEvent file:core/Smf/SMFEvent.h line:98
		pybind11::class_<H2Core::SMFTrackNameMetaEvent, std::shared_ptr<H2Core::SMFTrackNameMetaEvent>, PyCallBack_H2Core_SMFTrackNameMetaEvent, H2Core::SMFEvent> cl(M("H2Core"), "SMFTrackNameMetaEvent", "");
		cl.def( pybind11::init<const class QString &, unsigned int>(), pybind11::arg("sTrackName"), pybind11::arg("nDeltaTime") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_SMFTrackNameMetaEvent const &o){ return new PyCallBack_H2Core_SMFTrackNameMetaEvent(o); } ) );
		cl.def( pybind11::init( [](H2Core::SMFTrackNameMetaEvent const &o){ return new H2Core::SMFTrackNameMetaEvent(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::SMFTrackNameMetaEvent::class_name, "C++: H2Core::SMFTrackNameMetaEvent::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class H2Core::SMFTrackNameMetaEvent & (H2Core::SMFTrackNameMetaEvent::*)(const class H2Core::SMFTrackNameMetaEvent &)) &H2Core::SMFTrackNameMetaEvent::operator=, "C++: H2Core::SMFTrackNameMetaEvent::operator=(const class H2Core::SMFTrackNameMetaEvent &) --> class H2Core::SMFTrackNameMetaEvent &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B741_[H2Core::SMFSetTempoMetaEvent] ";
	{ // H2Core::SMFSetTempoMetaEvent file:core/Smf/SMFEvent.h line:112
		pybind11::class_<H2Core::SMFSetTempoMetaEvent, std::shared_ptr<H2Core::SMFSetTempoMetaEvent>, PyCallBack_H2Core_SMFSetTempoMetaEvent, H2Core::SMFEvent> cl(M("H2Core"), "SMFSetTempoMetaEvent", "");
		cl.def( pybind11::init<float, unsigned int>(), pybind11::arg("fBPM"), pybind11::arg("nDeltaTime") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_SMFSetTempoMetaEvent const &o){ return new PyCallBack_H2Core_SMFSetTempoMetaEvent(o); } ) );
		cl.def( pybind11::init( [](H2Core::SMFSetTempoMetaEvent const &o){ return new H2Core::SMFSetTempoMetaEvent(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::SMFSetTempoMetaEvent::class_name, "C++: H2Core::SMFSetTempoMetaEvent::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class H2Core::SMFSetTempoMetaEvent & (H2Core::SMFSetTempoMetaEvent::*)(const class H2Core::SMFSetTempoMetaEvent &)) &H2Core::SMFSetTempoMetaEvent::operator=, "C++: H2Core::SMFSetTempoMetaEvent::operator=(const class H2Core::SMFSetTempoMetaEvent &) --> class H2Core::SMFSetTempoMetaEvent &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B742_[H2Core::SMFCopyRightNoticeMetaEvent] ";
	{ // H2Core::SMFCopyRightNoticeMetaEvent file:core/Smf/SMFEvent.h line:126
		pybind11::class_<H2Core::SMFCopyRightNoticeMetaEvent, std::shared_ptr<H2Core::SMFCopyRightNoticeMetaEvent>, PyCallBack_H2Core_SMFCopyRightNoticeMetaEvent, H2Core::SMFEvent> cl(M("H2Core"), "SMFCopyRightNoticeMetaEvent", "");
		cl.def( pybind11::init<const class QString &, unsigned int>(), pybind11::arg("sAuthor"), pybind11::arg("nDeltaTime") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_SMFCopyRightNoticeMetaEvent const &o){ return new PyCallBack_H2Core_SMFCopyRightNoticeMetaEvent(o); } ) );
		cl.def( pybind11::init( [](H2Core::SMFCopyRightNoticeMetaEvent const &o){ return new H2Core::SMFCopyRightNoticeMetaEvent(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::SMFCopyRightNoticeMetaEvent::class_name, "C++: H2Core::SMFCopyRightNoticeMetaEvent::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class H2Core::SMFCopyRightNoticeMetaEvent & (H2Core::SMFCopyRightNoticeMetaEvent::*)(const class H2Core::SMFCopyRightNoticeMetaEvent &)) &H2Core::SMFCopyRightNoticeMetaEvent::operator=, "C++: H2Core::SMFCopyRightNoticeMetaEvent::operator=(const class H2Core::SMFCopyRightNoticeMetaEvent &) --> class H2Core::SMFCopyRightNoticeMetaEvent &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B743_[H2Core::SMFTimeSignatureMetaEvent] ";
	{ // H2Core::SMFTimeSignatureMetaEvent file:core/Smf/SMFEvent.h line:140
		pybind11::class_<H2Core::SMFTimeSignatureMetaEvent, std::shared_ptr<H2Core::SMFTimeSignatureMetaEvent>, PyCallBack_H2Core_SMFTimeSignatureMetaEvent, H2Core::SMFEvent> cl(M("H2Core"), "SMFTimeSignatureMetaEvent", "");
		cl.def( pybind11::init<unsigned int, unsigned int, unsigned int, unsigned int, unsigned int>(), pybind11::arg("nBeats"), pybind11::arg("nNote"), pybind11::arg("nMTPMC"), pybind11::arg("nTSNP24"), pybind11::arg("nTicks") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_SMFTimeSignatureMetaEvent const &o){ return new PyCallBack_H2Core_SMFTimeSignatureMetaEvent(o); } ) );
		cl.def( pybind11::init( [](H2Core::SMFTimeSignatureMetaEvent const &o){ return new H2Core::SMFTimeSignatureMetaEvent(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::SMFTimeSignatureMetaEvent::class_name, "C++: H2Core::SMFTimeSignatureMetaEvent::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class H2Core::SMFTimeSignatureMetaEvent & (H2Core::SMFTimeSignatureMetaEvent::*)(const class H2Core::SMFTimeSignatureMetaEvent &)) &H2Core::SMFTimeSignatureMetaEvent::operator=, "C++: H2Core::SMFTimeSignatureMetaEvent::operator=(const class H2Core::SMFTimeSignatureMetaEvent &) --> class H2Core::SMFTimeSignatureMetaEvent &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B744_[H2Core::SMFNoteOnEvent] ";
	{ // H2Core::SMFNoteOnEvent file:core/Smf/SMFEvent.h line:154
		pybind11::class_<H2Core::SMFNoteOnEvent, std::shared_ptr<H2Core::SMFNoteOnEvent>, PyCallBack_H2Core_SMFNoteOnEvent, H2Core::SMFEvent> cl(M("H2Core"), "SMFNoteOnEvent", "");
		cl.def( pybind11::init<unsigned int, int, int, int>(), pybind11::arg("nTicks"), pybind11::arg("nChannel"), pybind11::arg("nPitch"), pybind11::arg("nVelocity") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_SMFNoteOnEvent const &o){ return new PyCallBack_H2Core_SMFNoteOnEvent(o); } ) );
		cl.def( pybind11::init( [](H2Core::SMFNoteOnEvent const &o){ return new H2Core::SMFNoteOnEvent(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::SMFNoteOnEvent::class_name, "C++: H2Core::SMFNoteOnEvent::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class H2Core::SMFNoteOnEvent & (H2Core::SMFNoteOnEvent::*)(const class H2Core::SMFNoteOnEvent &)) &H2Core::SMFNoteOnEvent::operator=, "C++: H2Core::SMFNoteOnEvent::operator=(const class H2Core::SMFNoteOnEvent &) --> class H2Core::SMFNoteOnEvent &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B745_[H2Core::SMFNoteOffEvent] ";
	{ // H2Core::SMFNoteOffEvent file:core/Smf/SMFEvent.h line:170
		pybind11::class_<H2Core::SMFNoteOffEvent, std::shared_ptr<H2Core::SMFNoteOffEvent>, PyCallBack_H2Core_SMFNoteOffEvent, H2Core::SMFEvent> cl(M("H2Core"), "SMFNoteOffEvent", "");
		cl.def( pybind11::init<unsigned int, int, int, int>(), pybind11::arg("nTicks"), pybind11::arg("nChannel"), pybind11::arg("nPitch"), pybind11::arg("nVelocity") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_SMFNoteOffEvent const &o){ return new PyCallBack_H2Core_SMFNoteOffEvent(o); } ) );
		cl.def( pybind11::init( [](H2Core::SMFNoteOffEvent const &o){ return new H2Core::SMFNoteOffEvent(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::SMFNoteOffEvent::class_name, "C++: H2Core::SMFNoteOffEvent::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class H2Core::SMFNoteOffEvent & (H2Core::SMFNoteOffEvent::*)(const class H2Core::SMFNoteOffEvent &)) &H2Core::SMFNoteOffEvent::operator=, "C++: H2Core::SMFNoteOffEvent::operator=(const class H2Core::SMFNoteOffEvent &) --> class H2Core::SMFNoteOffEvent &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B746_[H2Core::SMFHeader] ";
	{ // H2Core::SMFHeader file:core/Smf/SMF.h line:40
		pybind11::class_<H2Core::SMFHeader, std::shared_ptr<H2Core::SMFHeader>, PyCallBack_H2Core_SMFHeader, H2Core::SMFBase, H2Core::Object> cl(M("H2Core"), "SMFHeader", "");
		cl.def( pybind11::init<int, int, int>(), pybind11::arg("nFormat"), pybind11::arg("nTracks"), pybind11::arg("nTPQN") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_SMFHeader const &o){ return new PyCallBack_H2Core_SMFHeader(o); } ) );
		cl.def( pybind11::init( [](H2Core::SMFHeader const &o){ return new H2Core::SMFHeader(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::SMFHeader::class_name, "C++: H2Core::SMFHeader::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("addTrack", (void (H2Core::SMFHeader::*)()) &H2Core::SMFHeader::addTrack, "C++: H2Core::SMFHeader::addTrack() --> void");
		cl.def("assign", (class H2Core::SMFHeader & (H2Core::SMFHeader::*)(const class H2Core::SMFHeader &)) &H2Core::SMFHeader::operator=, "C++: H2Core::SMFHeader::operator=(const class H2Core::SMFHeader &) --> class H2Core::SMFHeader &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B747_[H2Core::SMFTrack] ";
	{ // H2Core::SMFTrack file:core/Smf/SMF.h line:58
		pybind11::class_<H2Core::SMFTrack, std::shared_ptr<H2Core::SMFTrack>, PyCallBack_H2Core_SMFTrack, H2Core::SMFBase, H2Core::Object> cl(M("H2Core"), "SMFTrack", "");
		cl.def( pybind11::init( [](){ return new H2Core::SMFTrack(); }, [](){ return new PyCallBack_H2Core_SMFTrack(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_SMFTrack const &o){ return new PyCallBack_H2Core_SMFTrack(o); } ) );
		cl.def( pybind11::init( [](H2Core::SMFTrack const &o){ return new H2Core::SMFTrack(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::SMFTrack::class_name, "C++: H2Core::SMFTrack::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("addEvent", (void (H2Core::SMFTrack::*)(class H2Core::SMFEvent *)) &H2Core::SMFTrack::addEvent, "C++: H2Core::SMFTrack::addEvent(class H2Core::SMFEvent *) --> void", pybind11::arg("pEvent"));
		cl.def("assign", (class H2Core::SMFTrack & (H2Core::SMFTrack::*)(const class H2Core::SMFTrack &)) &H2Core::SMFTrack::operator=, "C++: H2Core::SMFTrack::operator=(const class H2Core::SMFTrack &) --> class H2Core::SMFTrack &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B748_[H2Core::SMF] ";
	{ // H2Core::SMF file:core/Smf/SMF.h line:76
		pybind11::class_<H2Core::SMF, std::shared_ptr<H2Core::SMF>, PyCallBack_H2Core_SMF, H2Core::SMFBase, H2Core::Object> cl(M("H2Core"), "SMF", "");
		cl.def( pybind11::init<int, int>(), pybind11::arg("nFormat"), pybind11::arg("nTPQN") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_SMF const &o){ return new PyCallBack_H2Core_SMF(o); } ) );
		cl.def( pybind11::init( [](H2Core::SMF const &o){ return new H2Core::SMF(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::SMF::class_name, "C++: H2Core::SMF::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("addTrack", (void (H2Core::SMF::*)(class H2Core::SMFTrack *)) &H2Core::SMF::addTrack, "C++: H2Core::SMF::addTrack(class H2Core::SMFTrack *) --> void", pybind11::arg("pTrack"));
		cl.def("assign", (class H2Core::SMF & (H2Core::SMF::*)(const class H2Core::SMF &)) &H2Core::SMF::operator=, "C++: H2Core::SMF::operator=(const class H2Core::SMF &) --> class H2Core::SMF &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B749_[H2Core::SMFWriter] ";
	{ // H2Core::SMFWriter file:core/Smf/SMF.h line:97
		pybind11::class_<H2Core::SMFWriter, std::shared_ptr<H2Core::SMFWriter>, H2Core::Object> cl(M("H2Core"), "SMFWriter", "");
		cl.def_static("class_name", (const char * (*)()) &H2Core::SMFWriter::class_name, "C++: H2Core::SMFWriter::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("save", (void (H2Core::SMFWriter::*)(const class QString &, class H2Core::Song *)) &H2Core::SMFWriter::save, "C++: H2Core::SMFWriter::save(const class QString &, class H2Core::Song *) --> void", pybind11::arg("sFilename"), pybind11::arg("pSong"));
		cl.def("assign", (class H2Core::SMFWriter & (H2Core::SMFWriter::*)(const class H2Core::SMFWriter &)) &H2Core::SMFWriter::operator=, "C++: H2Core::SMFWriter::operator=(const class H2Core::SMFWriter &) --> class H2Core::SMFWriter &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
