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
#include <core/Basics/Instrument.h> // H2Core::Instrument
#include <core/Basics/InstrumentList.h> // H2Core::InstrumentList
#include <core/Basics/Note.h> // 
#include <core/Basics/Note.h> // H2Core::Note
#include <core/Basics/Note.h> // H2Core::SelectedLayerInfo
#include <core/Helpers/Xml.h> // H2Core::XMLNode
#include <core/IO/AlsaAudioDriver.h> // H2Core::AlsaAudioDriver
#include <core/IO/AlsaMidiDriver.h> // H2Core::AlsaMidiDriver
#include <core/IO/DiskWriterDriver.h> // H2Core::DiskWriterDriver
#include <core/IO/FakeDriver.h> // H2Core::FakeDriver
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

// H2Core::AlsaAudioDriver file:core/IO/AlsaAudioDriver.h line:36
struct PyCallBack_H2Core_AlsaAudioDriver : public H2Core::AlsaAudioDriver {
	using H2Core::AlsaAudioDriver::AlsaAudioDriver;

	int init(unsigned int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaAudioDriver *>(this), "init");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return AlsaAudioDriver::init(a0);
	}
	int connect() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaAudioDriver *>(this), "connect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return AlsaAudioDriver::connect();
	}
	void disconnect() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaAudioDriver *>(this), "disconnect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return AlsaAudioDriver::disconnect();
	}
	unsigned int getBufferSize() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaAudioDriver *>(this), "getBufferSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned int>::value) {
				static pybind11::detail::override_caster_t<unsigned int> caster;
				return pybind11::detail::cast_ref<unsigned int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned int>(std::move(o));
		}
		return AlsaAudioDriver::getBufferSize();
	}
	unsigned int getSampleRate() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaAudioDriver *>(this), "getSampleRate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned int>::value) {
				static pybind11::detail::override_caster_t<unsigned int> caster;
				return pybind11::detail::cast_ref<unsigned int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned int>(std::move(o));
		}
		return AlsaAudioDriver::getSampleRate();
	}
	float * getOut_L() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaAudioDriver *>(this), "getOut_L");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<float *>::value) {
				static pybind11::detail::override_caster_t<float *> caster;
				return pybind11::detail::cast_ref<float *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<float *>(std::move(o));
		}
		return AlsaAudioDriver::getOut_L();
	}
	float * getOut_R() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaAudioDriver *>(this), "getOut_R");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<float *>::value) {
				static pybind11::detail::override_caster_t<float *> caster;
				return pybind11::detail::cast_ref<float *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<float *>(std::move(o));
		}
		return AlsaAudioDriver::getOut_R();
	}
	void updateTransportInfo() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaAudioDriver *>(this), "updateTransportInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return AlsaAudioDriver::updateTransportInfo();
	}
	void play() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaAudioDriver *>(this), "play");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return AlsaAudioDriver::play();
	}
	void stop() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaAudioDriver *>(this), "stop");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return AlsaAudioDriver::stop();
	}
	void locate(unsigned long a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaAudioDriver *>(this), "locate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return AlsaAudioDriver::locate(a0);
	}
	void setBpm(float a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaAudioDriver *>(this), "setBpm");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return AlsaAudioDriver::setBpm(a0);
	}
	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaAudioDriver *>(this), "toQString");
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

// H2Core::AlsaMidiDriver file:core/IO/AlsaMidiDriver.h line:42
struct PyCallBack_H2Core_AlsaMidiDriver : public H2Core::AlsaMidiDriver {
	using H2Core::AlsaMidiDriver::AlsaMidiDriver;

	void open() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaMidiDriver *>(this), "open");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return AlsaMidiDriver::open();
	}
	void close() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaMidiDriver *>(this), "close");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return AlsaMidiDriver::close();
	}
	void handleQueueNote(class H2Core::Note * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaMidiDriver *>(this), "handleQueueNote");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return AlsaMidiDriver::handleQueueNote(a0);
	}
	void handleQueueNoteOff(int a0, int a1, int a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaMidiDriver *>(this), "handleQueueNoteOff");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return AlsaMidiDriver::handleQueueNoteOff(a0, a1, a2);
	}
	void handleQueueAllNoteOff() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaMidiDriver *>(this), "handleQueueAllNoteOff");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return AlsaMidiDriver::handleQueueAllNoteOff();
	}
	void handleOutgoingControlChange(int a0, int a1, int a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaMidiDriver *>(this), "handleOutgoingControlChange");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return AlsaMidiDriver::handleOutgoingControlChange(a0, a1, a2);
	}
	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::AlsaMidiDriver *>(this), "toQString");
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

// H2Core::DiskWriterDriver file:core/IO/DiskWriterDriver.h line:39
struct PyCallBack_H2Core_DiskWriterDriver : public H2Core::DiskWriterDriver {
	using H2Core::DiskWriterDriver::DiskWriterDriver;

	int init(unsigned int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::DiskWriterDriver *>(this), "init");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return DiskWriterDriver::init(a0);
	}
	int connect() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::DiskWriterDriver *>(this), "connect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return DiskWriterDriver::connect();
	}
	void disconnect() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::DiskWriterDriver *>(this), "disconnect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return DiskWriterDriver::disconnect();
	}
	unsigned int getBufferSize() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::DiskWriterDriver *>(this), "getBufferSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned int>::value) {
				static pybind11::detail::override_caster_t<unsigned int> caster;
				return pybind11::detail::cast_ref<unsigned int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned int>(std::move(o));
		}
		return DiskWriterDriver::getBufferSize();
	}
	unsigned int getSampleRate() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::DiskWriterDriver *>(this), "getSampleRate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned int>::value) {
				static pybind11::detail::override_caster_t<unsigned int> caster;
				return pybind11::detail::cast_ref<unsigned int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned int>(std::move(o));
		}
		return DiskWriterDriver::getSampleRate();
	}
	float * getOut_L() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::DiskWriterDriver *>(this), "getOut_L");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<float *>::value) {
				static pybind11::detail::override_caster_t<float *> caster;
				return pybind11::detail::cast_ref<float *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<float *>(std::move(o));
		}
		return DiskWriterDriver::getOut_L();
	}
	float * getOut_R() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::DiskWriterDriver *>(this), "getOut_R");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<float *>::value) {
				static pybind11::detail::override_caster_t<float *> caster;
				return pybind11::detail::cast_ref<float *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<float *>(std::move(o));
		}
		return DiskWriterDriver::getOut_R();
	}
	void play() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::DiskWriterDriver *>(this), "play");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return DiskWriterDriver::play();
	}
	void stop() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::DiskWriterDriver *>(this), "stop");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return DiskWriterDriver::stop();
	}
	void locate(unsigned long a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::DiskWriterDriver *>(this), "locate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return DiskWriterDriver::locate(a0);
	}
	void updateTransportInfo() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::DiskWriterDriver *>(this), "updateTransportInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return DiskWriterDriver::updateTransportInfo();
	}
	void setBpm(float a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::DiskWriterDriver *>(this), "setBpm");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return DiskWriterDriver::setBpm(a0);
	}
	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::DiskWriterDriver *>(this), "toQString");
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

// H2Core::FakeDriver file:core/IO/FakeDriver.h line:34
struct PyCallBack_H2Core_FakeDriver : public H2Core::FakeDriver {
	using H2Core::FakeDriver::FakeDriver;

	int init(unsigned int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::FakeDriver *>(this), "init");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return FakeDriver::init(a0);
	}
	int connect() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::FakeDriver *>(this), "connect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return FakeDriver::connect();
	}
	void disconnect() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::FakeDriver *>(this), "disconnect");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return FakeDriver::disconnect();
	}
	unsigned int getBufferSize() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::FakeDriver *>(this), "getBufferSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned int>::value) {
				static pybind11::detail::override_caster_t<unsigned int> caster;
				return pybind11::detail::cast_ref<unsigned int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned int>(std::move(o));
		}
		return FakeDriver::getBufferSize();
	}
	unsigned int getSampleRate() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::FakeDriver *>(this), "getSampleRate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned int>::value) {
				static pybind11::detail::override_caster_t<unsigned int> caster;
				return pybind11::detail::cast_ref<unsigned int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned int>(std::move(o));
		}
		return FakeDriver::getSampleRate();
	}
	float * getOut_L() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::FakeDriver *>(this), "getOut_L");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<float *>::value) {
				static pybind11::detail::override_caster_t<float *> caster;
				return pybind11::detail::cast_ref<float *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<float *>(std::move(o));
		}
		return FakeDriver::getOut_L();
	}
	float * getOut_R() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::FakeDriver *>(this), "getOut_R");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<float *>::value) {
				static pybind11::detail::override_caster_t<float *> caster;
				return pybind11::detail::cast_ref<float *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<float *>(std::move(o));
		}
		return FakeDriver::getOut_R();
	}
	void play() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::FakeDriver *>(this), "play");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return FakeDriver::play();
	}
	void stop() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::FakeDriver *>(this), "stop");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return FakeDriver::stop();
	}
	void locate(unsigned long a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::FakeDriver *>(this), "locate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return FakeDriver::locate(a0);
	}
	void updateTransportInfo() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::FakeDriver *>(this), "updateTransportInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return FakeDriver::updateTransportInfo();
	}
	void setBpm(float a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::FakeDriver *>(this), "setBpm");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return FakeDriver::setBpm(a0);
	}
	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::FakeDriver *>(this), "toQString");
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

void bind_core_IO_AlsaAudioDriver(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B711_[H2Core::AlsaAudioDriver] ";
	{ // H2Core::AlsaAudioDriver file:core/IO/AlsaAudioDriver.h line:36
		pybind11::class_<H2Core::AlsaAudioDriver, std::shared_ptr<H2Core::AlsaAudioDriver>, PyCallBack_H2Core_AlsaAudioDriver, H2Core::AudioOutput> cl(M("H2Core"), "AlsaAudioDriver", "");
		cl.def( pybind11::init( [](PyCallBack_H2Core_AlsaAudioDriver const &o){ return new PyCallBack_H2Core_AlsaAudioDriver(o); } ) );
		cl.def( pybind11::init( [](H2Core::AlsaAudioDriver const &o){ return new H2Core::AlsaAudioDriver(o); } ) );
		cl.def_readwrite("m_bIsRunning", &H2Core::AlsaAudioDriver::m_bIsRunning);
		cl.def_readwrite("m_nBufferSize", &H2Core::AlsaAudioDriver::m_nBufferSize);
		cl.def_readwrite("m_nXRuns", &H2Core::AlsaAudioDriver::m_nXRuns);
		cl.def_readwrite("m_sAlsaAudioDevice", &H2Core::AlsaAudioDriver::m_sAlsaAudioDevice);
		cl.def_static("class_name", (const char * (*)()) &H2Core::AlsaAudioDriver::class_name, "C++: H2Core::AlsaAudioDriver::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("init", (int (H2Core::AlsaAudioDriver::*)(unsigned int)) &H2Core::AlsaAudioDriver::init, "C++: H2Core::AlsaAudioDriver::init(unsigned int) --> int", pybind11::arg("nBufferSize"));
		cl.def("connect", (int (H2Core::AlsaAudioDriver::*)()) &H2Core::AlsaAudioDriver::connect, "C++: H2Core::AlsaAudioDriver::connect() --> int");
		cl.def("disconnect", (void (H2Core::AlsaAudioDriver::*)()) &H2Core::AlsaAudioDriver::disconnect, "C++: H2Core::AlsaAudioDriver::disconnect() --> void");
		cl.def("getBufferSize", (unsigned int (H2Core::AlsaAudioDriver::*)()) &H2Core::AlsaAudioDriver::getBufferSize, "C++: H2Core::AlsaAudioDriver::getBufferSize() --> unsigned int");
		cl.def("getSampleRate", (unsigned int (H2Core::AlsaAudioDriver::*)()) &H2Core::AlsaAudioDriver::getSampleRate, "C++: H2Core::AlsaAudioDriver::getSampleRate() --> unsigned int");
		cl.def("getOut_L", (float * (H2Core::AlsaAudioDriver::*)()) &H2Core::AlsaAudioDriver::getOut_L, "C++: H2Core::AlsaAudioDriver::getOut_L() --> float *", pybind11::return_value_policy::automatic);
		cl.def("getOut_R", (float * (H2Core::AlsaAudioDriver::*)()) &H2Core::AlsaAudioDriver::getOut_R, "C++: H2Core::AlsaAudioDriver::getOut_R() --> float *", pybind11::return_value_policy::automatic);
		cl.def("updateTransportInfo", (void (H2Core::AlsaAudioDriver::*)()) &H2Core::AlsaAudioDriver::updateTransportInfo, "C++: H2Core::AlsaAudioDriver::updateTransportInfo() --> void");
		cl.def("play", (void (H2Core::AlsaAudioDriver::*)()) &H2Core::AlsaAudioDriver::play, "C++: H2Core::AlsaAudioDriver::play() --> void");
		cl.def("stop", (void (H2Core::AlsaAudioDriver::*)()) &H2Core::AlsaAudioDriver::stop, "C++: H2Core::AlsaAudioDriver::stop() --> void");
		cl.def("locate", (void (H2Core::AlsaAudioDriver::*)(unsigned long)) &H2Core::AlsaAudioDriver::locate, "C++: H2Core::AlsaAudioDriver::locate(unsigned long) --> void", pybind11::arg("nFrame"));
		cl.def("setBpm", (void (H2Core::AlsaAudioDriver::*)(float)) &H2Core::AlsaAudioDriver::setBpm, "C++: H2Core::AlsaAudioDriver::setBpm(float) --> void", pybind11::arg("fBPM"));
		cl.def("assign", (class H2Core::AlsaAudioDriver & (H2Core::AlsaAudioDriver::*)(const class H2Core::AlsaAudioDriver &)) &H2Core::AlsaAudioDriver::operator=, "C++: H2Core::AlsaAudioDriver::operator=(const class H2Core::AlsaAudioDriver &) --> class H2Core::AlsaAudioDriver &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B712_[H2Core::AlsaMidiDriver] ";
	{ // H2Core::AlsaMidiDriver file:core/IO/AlsaMidiDriver.h line:42
		pybind11::class_<H2Core::AlsaMidiDriver, std::shared_ptr<H2Core::AlsaMidiDriver>, PyCallBack_H2Core_AlsaMidiDriver, H2Core::MidiInput, H2Core::MidiOutput> cl(M("H2Core"), "AlsaMidiDriver", "Alsa Midi Driver\n Based on Matthias Nagorni alsa sequencer example");
		cl.def( pybind11::init( [](){ return new H2Core::AlsaMidiDriver(); }, [](){ return new PyCallBack_H2Core_AlsaMidiDriver(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_AlsaMidiDriver const &o){ return new PyCallBack_H2Core_AlsaMidiDriver(o); } ) );
		cl.def( pybind11::init( [](H2Core::AlsaMidiDriver const &o){ return new H2Core::AlsaMidiDriver(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::AlsaMidiDriver::class_name, "C++: H2Core::AlsaMidiDriver::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("open", (void (H2Core::AlsaMidiDriver::*)()) &H2Core::AlsaMidiDriver::open, "C++: H2Core::AlsaMidiDriver::open() --> void");
		cl.def("close", (void (H2Core::AlsaMidiDriver::*)()) &H2Core::AlsaMidiDriver::close, "C++: H2Core::AlsaMidiDriver::close() --> void");
		cl.def("getPortInfo", (void (H2Core::AlsaMidiDriver::*)(const class QString &, int &, int &)) &H2Core::AlsaMidiDriver::getPortInfo, "C++: H2Core::AlsaMidiDriver::getPortInfo(const class QString &, int &, int &) --> void", pybind11::arg("sPortName"), pybind11::arg("nClient"), pybind11::arg("nPort"));
		cl.def("handleQueueNote", (void (H2Core::AlsaMidiDriver::*)(class H2Core::Note *)) &H2Core::AlsaMidiDriver::handleQueueNote, "C++: H2Core::AlsaMidiDriver::handleQueueNote(class H2Core::Note *) --> void", pybind11::arg("pNote"));
		cl.def("handleQueueNoteOff", (void (H2Core::AlsaMidiDriver::*)(int, int, int)) &H2Core::AlsaMidiDriver::handleQueueNoteOff, "C++: H2Core::AlsaMidiDriver::handleQueueNoteOff(int, int, int) --> void", pybind11::arg("channel"), pybind11::arg("key"), pybind11::arg("velocity"));
		cl.def("handleQueueAllNoteOff", (void (H2Core::AlsaMidiDriver::*)()) &H2Core::AlsaMidiDriver::handleQueueAllNoteOff, "C++: H2Core::AlsaMidiDriver::handleQueueAllNoteOff() --> void");
		cl.def("handleOutgoingControlChange", (void (H2Core::AlsaMidiDriver::*)(int, int, int)) &H2Core::AlsaMidiDriver::handleOutgoingControlChange, "C++: H2Core::AlsaMidiDriver::handleOutgoingControlChange(int, int, int) --> void", pybind11::arg("param"), pybind11::arg("value"), pybind11::arg("channel"));
		cl.def("assign", (class H2Core::AlsaMidiDriver & (H2Core::AlsaMidiDriver::*)(const class H2Core::AlsaMidiDriver &)) &H2Core::AlsaMidiDriver::operator=, "C++: H2Core::AlsaMidiDriver::operator=(const class H2Core::AlsaMidiDriver &) --> class H2Core::AlsaMidiDriver &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B713_[H2Core::DiskWriterDriver] ";
	{ // H2Core::DiskWriterDriver file:core/IO/DiskWriterDriver.h line:39
		pybind11::class_<H2Core::DiskWriterDriver, std::shared_ptr<H2Core::DiskWriterDriver>, PyCallBack_H2Core_DiskWriterDriver, H2Core::AudioOutput> cl(M("H2Core"), "DiskWriterDriver", "Driver for export audio to disk");
		cl.def( pybind11::init( [](PyCallBack_H2Core_DiskWriterDriver const &o){ return new PyCallBack_H2Core_DiskWriterDriver(o); } ) );
		cl.def( pybind11::init( [](H2Core::DiskWriterDriver const &o){ return new H2Core::DiskWriterDriver(o); } ) );
		cl.def_readwrite("m_nSampleRate", &H2Core::DiskWriterDriver::m_nSampleRate);
		cl.def_readwrite("m_sFilename", &H2Core::DiskWriterDriver::m_sFilename);
		cl.def_readwrite("m_nBufferSize", &H2Core::DiskWriterDriver::m_nBufferSize);
		cl.def_readwrite("m_nSampleDepth", &H2Core::DiskWriterDriver::m_nSampleDepth);
		cl.def_static("class_name", (const char * (*)()) &H2Core::DiskWriterDriver::class_name, "C++: H2Core::DiskWriterDriver::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("init", (int (H2Core::DiskWriterDriver::*)(unsigned int)) &H2Core::DiskWriterDriver::init, "C++: H2Core::DiskWriterDriver::init(unsigned int) --> int", pybind11::arg("nBufferSize"));
		cl.def("connect", (int (H2Core::DiskWriterDriver::*)()) &H2Core::DiskWriterDriver::connect, "C++: H2Core::DiskWriterDriver::connect() --> int");
		cl.def("disconnect", (void (H2Core::DiskWriterDriver::*)()) &H2Core::DiskWriterDriver::disconnect, "C++: H2Core::DiskWriterDriver::disconnect() --> void");
		cl.def("audioEngine_process_checkBPMChanged", (void (H2Core::DiskWriterDriver::*)()) &H2Core::DiskWriterDriver::audioEngine_process_checkBPMChanged, "C++: H2Core::DiskWriterDriver::audioEngine_process_checkBPMChanged() --> void");
		cl.def("getBufferSize", (unsigned int (H2Core::DiskWriterDriver::*)()) &H2Core::DiskWriterDriver::getBufferSize, "C++: H2Core::DiskWriterDriver::getBufferSize() --> unsigned int");
		cl.def("getSampleRate", (unsigned int (H2Core::DiskWriterDriver::*)()) &H2Core::DiskWriterDriver::getSampleRate, "C++: H2Core::DiskWriterDriver::getSampleRate() --> unsigned int");
		cl.def("getOut_L", (float * (H2Core::DiskWriterDriver::*)()) &H2Core::DiskWriterDriver::getOut_L, "C++: H2Core::DiskWriterDriver::getOut_L() --> float *", pybind11::return_value_policy::automatic);
		cl.def("getOut_R", (float * (H2Core::DiskWriterDriver::*)()) &H2Core::DiskWriterDriver::getOut_R, "C++: H2Core::DiskWriterDriver::getOut_R() --> float *", pybind11::return_value_policy::automatic);
		cl.def("setFileName", (void (H2Core::DiskWriterDriver::*)(const class QString &)) &H2Core::DiskWriterDriver::setFileName, "C++: H2Core::DiskWriterDriver::setFileName(const class QString &) --> void", pybind11::arg("sFilename"));
		cl.def("play", (void (H2Core::DiskWriterDriver::*)()) &H2Core::DiskWriterDriver::play, "C++: H2Core::DiskWriterDriver::play() --> void");
		cl.def("stop", (void (H2Core::DiskWriterDriver::*)()) &H2Core::DiskWriterDriver::stop, "C++: H2Core::DiskWriterDriver::stop() --> void");
		cl.def("locate", (void (H2Core::DiskWriterDriver::*)(unsigned long)) &H2Core::DiskWriterDriver::locate, "C++: H2Core::DiskWriterDriver::locate(unsigned long) --> void", pybind11::arg("nFrame"));
		cl.def("updateTransportInfo", (void (H2Core::DiskWriterDriver::*)()) &H2Core::DiskWriterDriver::updateTransportInfo, "C++: H2Core::DiskWriterDriver::updateTransportInfo() --> void");
		cl.def("setBpm", (void (H2Core::DiskWriterDriver::*)(float)) &H2Core::DiskWriterDriver::setBpm, "C++: H2Core::DiskWriterDriver::setBpm(float) --> void", pybind11::arg("fBPM"));
		cl.def("assign", (class H2Core::DiskWriterDriver & (H2Core::DiskWriterDriver::*)(const class H2Core::DiskWriterDriver &)) &H2Core::DiskWriterDriver::operator=, "C++: H2Core::DiskWriterDriver::operator=(const class H2Core::DiskWriterDriver &) --> class H2Core::DiskWriterDriver &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B714_[H2Core::FakeDriver] ";
	{ // H2Core::FakeDriver file:core/IO/FakeDriver.h line:34
		pybind11::class_<H2Core::FakeDriver, std::shared_ptr<H2Core::FakeDriver>, PyCallBack_H2Core_FakeDriver, H2Core::AudioOutput> cl(M("H2Core"), "FakeDriver", "Fake audio driver. Used only for profiling.");
		cl.def( pybind11::init( [](PyCallBack_H2Core_FakeDriver const &o){ return new PyCallBack_H2Core_FakeDriver(o); } ) );
		cl.def( pybind11::init( [](H2Core::FakeDriver const &o){ return new H2Core::FakeDriver(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::FakeDriver::class_name, "C++: H2Core::FakeDriver::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("init", (int (H2Core::FakeDriver::*)(unsigned int)) &H2Core::FakeDriver::init, "C++: H2Core::FakeDriver::init(unsigned int) --> int", pybind11::arg("nBufferSize"));
		cl.def("connect", (int (H2Core::FakeDriver::*)()) &H2Core::FakeDriver::connect, "C++: H2Core::FakeDriver::connect() --> int");
		cl.def("disconnect", (void (H2Core::FakeDriver::*)()) &H2Core::FakeDriver::disconnect, "C++: H2Core::FakeDriver::disconnect() --> void");
		cl.def("getBufferSize", (unsigned int (H2Core::FakeDriver::*)()) &H2Core::FakeDriver::getBufferSize, "C++: H2Core::FakeDriver::getBufferSize() --> unsigned int");
		cl.def("getSampleRate", (unsigned int (H2Core::FakeDriver::*)()) &H2Core::FakeDriver::getSampleRate, "C++: H2Core::FakeDriver::getSampleRate() --> unsigned int");
		cl.def("getOut_L", (float * (H2Core::FakeDriver::*)()) &H2Core::FakeDriver::getOut_L, "C++: H2Core::FakeDriver::getOut_L() --> float *", pybind11::return_value_policy::automatic);
		cl.def("getOut_R", (float * (H2Core::FakeDriver::*)()) &H2Core::FakeDriver::getOut_R, "C++: H2Core::FakeDriver::getOut_R() --> float *", pybind11::return_value_policy::automatic);
		cl.def("play", (void (H2Core::FakeDriver::*)()) &H2Core::FakeDriver::play, "C++: H2Core::FakeDriver::play() --> void");
		cl.def("stop", (void (H2Core::FakeDriver::*)()) &H2Core::FakeDriver::stop, "C++: H2Core::FakeDriver::stop() --> void");
		cl.def("locate", (void (H2Core::FakeDriver::*)(unsigned long)) &H2Core::FakeDriver::locate, "C++: H2Core::FakeDriver::locate(unsigned long) --> void", pybind11::arg("nFrame"));
		cl.def("updateTransportInfo", (void (H2Core::FakeDriver::*)()) &H2Core::FakeDriver::updateTransportInfo, "C++: H2Core::FakeDriver::updateTransportInfo() --> void");
		cl.def("setBpm", (void (H2Core::FakeDriver::*)(float)) &H2Core::FakeDriver::setBpm, "C++: H2Core::FakeDriver::setBpm(float) --> void", pybind11::arg("fBPM"));
		cl.def("assign", (class H2Core::FakeDriver & (H2Core::FakeDriver::*)(const class H2Core::FakeDriver &)) &H2Core::FakeDriver::operator=, "C++: H2Core::FakeDriver::operator=(const class H2Core::FakeDriver &) --> class H2Core::FakeDriver &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
