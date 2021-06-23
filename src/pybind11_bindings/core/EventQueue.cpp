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
#include <core/EventQueue.h> // H2Core::Event
#include <core/EventQueue.h> // H2Core::EventQueue
#include <core/EventQueue.h> // H2Core::EventQueue::AddMidiNoteVector
#include <core/EventQueue.h> // H2Core::EventType
#include <core/Object.h> // H2Core::Object
#include <iostream> // --trace
#include <iterator> // __gnu_cxx::__normal_iterator
#include <iterator> // std::reverse_iterator
#include <memory> // std::allocator
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

// H2Core::EventQueue file:core/EventQueue.h line:186
struct PyCallBack_H2Core_EventQueue : public H2Core::EventQueue {
	using H2Core::EventQueue::EventQueue;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::EventQueue *>(this), "toQString");
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

void bind_core_EventQueue(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B693_[H2Core::EventQueue] ";
	{ // H2Core::EventQueue file:core/EventQueue.h line:186
		pybind11::class_<H2Core::EventQueue, std::shared_ptr<H2Core::EventQueue>, PyCallBack_H2Core_EventQueue, H2Core::Object> cl(M("H2Core"), "EventQueue", "Object handling the communication between the core of Hydrogen and\n its GUI.\n\n Whenever a specific condition is met or occasion happens within the\n core part of Hydrogen (its engine), an Event will be added to the\n EventQueue singleton. The GUI checks the content of this queue on a\n regular basis using HydrogenApp::onEventQueueTimer(). The actual\n frequency is set in the constructor HydrogenApp::HydrogenApp() to\n 20 times per second. Now, whenever an Event of a certain EventType\n is encountered, the corresponding function in the EventListener\n will be invoked to respond to the condition of the engine. For\n details about the mapping of EventTypes to functions please see the\n documentation of HydrogenApp::onEventQueueTimer().");
		cl.def( pybind11::init( [](PyCallBack_H2Core_EventQueue const &o){ return new PyCallBack_H2Core_EventQueue(o); } ) );
		cl.def( pybind11::init( [](H2Core::EventQueue const &o){ return new H2Core::EventQueue(o); } ) );
		cl.def_readwrite("m_addMidiNoteVector", &H2Core::EventQueue::m_addMidiNoteVector);
		cl.def_static("class_name", (const char * (*)()) &H2Core::EventQueue::class_name, "C++: H2Core::EventQueue::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def_static("create_instance", (void (*)()) &H2Core::EventQueue::create_instance, "If #__instance equals 0, a new EventQueue singleton will be\n created and stored in it.\n\n It is called in Hydrogen::create_instance().\n\nC++: H2Core::EventQueue::create_instance() --> void");
		cl.def_static("get_instance", (class H2Core::EventQueue * (*)()) &H2Core::EventQueue::get_instance, "Returns a pointer to the current EventQueue singleton\n stored in #__instance.\n\nC++: H2Core::EventQueue::get_instance() --> class H2Core::EventQueue *", pybind11::return_value_policy::automatic);
		cl.def("push_event", (void (H2Core::EventQueue::*)(const enum H2Core::EventType, const int)) &H2Core::EventQueue::push_event, "Queues the next event into the EventQueue.\n\n The event itself will be constructed inside the function\n and will be two properties: an EventType  and a\n value  Since the event written to the queue most\n recently is indexed with #__write_index, this variable is\n incremented once and its modulo with respect to #MAX_EVENTS\n is calculated to determine the position of insertion into\n #__events_buffer.\n\n The modulo operation is necessary because #__write_index\n will be only incremented and does not respect the actual\n length of #__events_buffer itself.\n\n \n Type of the event, which will be queued.\n \n\n Value specifying the content of the new event.\n\nC++: H2Core::EventQueue::push_event(const enum H2Core::EventType, const int) --> void", pybind11::arg("type"), pybind11::arg("nValue"));
		cl.def("pop_event", (class H2Core::Event (H2Core::EventQueue::*)()) &H2Core::EventQueue::pop_event, "Reads out the next event of the EventQueue.\n\n Since the event read out most recently is indexed with\n #__read_index, this variable is incremented once and its\n modulo with respect to #MAX_EVENTS is calculated to\n determine the event returned from #__events_buffer. \n\n The modulo operation is necessary because #__read_index\n will be only incremented and does not respect the actual\n length of #__events_buffer itself.\n\n \n Next event in line.\n\nC++: H2Core::EventQueue::pop_event() --> class H2Core::Event");
		cl.def("assign", (class H2Core::EventQueue & (H2Core::EventQueue::*)(const class H2Core::EventQueue &)) &H2Core::EventQueue::operator=, "C++: H2Core::EventQueue::operator=(const class H2Core::EventQueue &) --> class H2Core::EventQueue &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // H2Core::EventQueue::AddMidiNoteVector file:core/EventQueue.h line:238
			auto & enclosing_class = cl;
			pybind11::class_<H2Core::EventQueue::AddMidiNoteVector, std::shared_ptr<H2Core::EventQueue::AddMidiNoteVector>> cl(enclosing_class, "AddMidiNoteVector", "");
			cl.def( pybind11::init( [](){ return new H2Core::EventQueue::AddMidiNoteVector(); } ) );
			cl.def_readwrite("m_column", &H2Core::EventQueue::AddMidiNoteVector::m_column);
			cl.def_readwrite("m_row", &H2Core::EventQueue::AddMidiNoteVector::m_row);
			cl.def_readwrite("m_pattern", &H2Core::EventQueue::AddMidiNoteVector::m_pattern);
			cl.def_readwrite("m_length", &H2Core::EventQueue::AddMidiNoteVector::m_length);
			cl.def_readwrite("f_velocity", &H2Core::EventQueue::AddMidiNoteVector::f_velocity);
			cl.def_readwrite("f_pan", &H2Core::EventQueue::AddMidiNoteVector::f_pan);
			cl.def_readwrite("nk_noteKeyVal", &H2Core::EventQueue::AddMidiNoteVector::nk_noteKeyVal);
			cl.def_readwrite("no_octaveKeyVal", &H2Core::EventQueue::AddMidiNoteVector::no_octaveKeyVal);
			cl.def_readwrite("b_isMidi", &H2Core::EventQueue::AddMidiNoteVector::b_isMidi);
			cl.def_readwrite("b_isInstrumentMode", &H2Core::EventQueue::AddMidiNoteVector::b_isInstrumentMode);
			cl.def_readwrite("b_noteExist", &H2Core::EventQueue::AddMidiNoteVector::b_noteExist);
		}

	}
}
