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
#include <core/Basics/Song.h> // 
#include <core/Basics/Song.h> // H2Core::Song
#include <core/Helpers/Xml.h> // H2Core::XMLNode
#include <core/Object.h> // H2Core::Object
#include <core/Smf/SMF.h> // H2Core::SMF
#include <core/Smf/SMF.h> // H2Core::SMF0Writer
#include <core/Smf/SMF.h> // H2Core::SMF1Writer
#include <core/Smf/SMF.h> // H2Core::SMF1WriterMulti
#include <core/Smf/SMF.h> // H2Core::SMF1WriterSingle
#include <core/Smf/SMF.h> // H2Core::SMFTrack
#include <core/Smf/SMFEvent.h> // H2Core::SMFEvent
#include <core/Version.h> // H2Core::get_version
#include <core/Version.h> // H2Core::version_older_than
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

// H2Core::SMF1WriterSingle file:core/Smf/SMF.h line:132
struct PyCallBack_H2Core_SMF1WriterSingle : public H2Core::SMF1WriterSingle {
	using H2Core::SMF1WriterSingle::SMF1WriterSingle;

	void prepareEvents(class H2Core::Song * a0, class H2Core::SMF * a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMF1WriterSingle *>(this), "prepareEvents");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return SMF1WriterSingle::prepareEvents(a0, a1);
	}
	void packEvents(class H2Core::Song * a0, class H2Core::SMF * a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMF1WriterSingle *>(this), "packEvents");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return SMF1WriterSingle::packEvents(a0, a1);
	}
	class H2Core::SMF * createSMF(class H2Core::Song * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMF1WriterSingle *>(this), "createSMF");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class H2Core::SMF *>::value) {
				static pybind11::detail::override_caster_t<class H2Core::SMF *> caster;
				return pybind11::detail::cast_ref<class H2Core::SMF *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class H2Core::SMF *>(std::move(o));
		}
		return SMF1Writer::createSMF(a0);
	}
	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMF1WriterSingle *>(this), "toQString");
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

// H2Core::SMF1WriterMulti file:core/Smf/SMF.h line:147
struct PyCallBack_H2Core_SMF1WriterMulti : public H2Core::SMF1WriterMulti {
	using H2Core::SMF1WriterMulti::SMF1WriterMulti;

	void prepareEvents(class H2Core::Song * a0, class H2Core::SMF * a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMF1WriterMulti *>(this), "prepareEvents");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return SMF1WriterMulti::prepareEvents(a0, a1);
	}
	void packEvents(class H2Core::Song * a0, class H2Core::SMF * a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMF1WriterMulti *>(this), "packEvents");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return SMF1WriterMulti::packEvents(a0, a1);
	}
	class H2Core::SMF * createSMF(class H2Core::Song * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMF1WriterMulti *>(this), "createSMF");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class H2Core::SMF *>::value) {
				static pybind11::detail::override_caster_t<class H2Core::SMF *> caster;
				return pybind11::detail::cast_ref<class H2Core::SMF *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class H2Core::SMF *>(std::move(o));
		}
		return SMF1Writer::createSMF(a0);
	}
	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMF1WriterMulti *>(this), "toQString");
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

// H2Core::SMF0Writer file:core/Smf/SMF.h line:165
struct PyCallBack_H2Core_SMF0Writer : public H2Core::SMF0Writer {
	using H2Core::SMF0Writer::SMF0Writer;

	class H2Core::SMF * createSMF(class H2Core::Song * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMF0Writer *>(this), "createSMF");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class H2Core::SMF *>::value) {
				static pybind11::detail::override_caster_t<class H2Core::SMF *> caster;
				return pybind11::detail::cast_ref<class H2Core::SMF *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class H2Core::SMF *>(std::move(o));
		}
		return SMF0Writer::createSMF(a0);
	}
	void prepareEvents(class H2Core::Song * a0, class H2Core::SMF * a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMF0Writer *>(this), "prepareEvents");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return SMF0Writer::prepareEvents(a0, a1);
	}
	void packEvents(class H2Core::Song * a0, class H2Core::SMF * a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMF0Writer *>(this), "packEvents");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return SMF0Writer::packEvents(a0, a1);
	}
	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SMF0Writer *>(this), "toQString");
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

void bind_core_Smf_SMF(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B750_[H2Core::SMF1Writer] ";
	{ // H2Core::SMF1Writer file:core/Smf/SMF.h line:121
		pybind11::class_<H2Core::SMF1Writer, std::shared_ptr<H2Core::SMF1Writer>, H2Core::SMFWriter> cl(M("H2Core"), "SMF1Writer", "");
		cl.def_static("class_name", (const char * (*)()) &H2Core::SMF1Writer::class_name, "C++: H2Core::SMF1Writer::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class H2Core::SMF1Writer & (H2Core::SMF1Writer::*)(const class H2Core::SMF1Writer &)) &H2Core::SMF1Writer::operator=, "C++: H2Core::SMF1Writer::operator=(const class H2Core::SMF1Writer &) --> class H2Core::SMF1Writer &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B751_[H2Core::SMF1WriterSingle] ";
	{ // H2Core::SMF1WriterSingle file:core/Smf/SMF.h line:132
		pybind11::class_<H2Core::SMF1WriterSingle, std::shared_ptr<H2Core::SMF1WriterSingle>, PyCallBack_H2Core_SMF1WriterSingle, H2Core::SMF1Writer> cl(M("H2Core"), "SMF1WriterSingle", "");
		cl.def( pybind11::init( [](){ return new H2Core::SMF1WriterSingle(); }, [](){ return new PyCallBack_H2Core_SMF1WriterSingle(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_SMF1WriterSingle const &o){ return new PyCallBack_H2Core_SMF1WriterSingle(o); } ) );
		cl.def( pybind11::init( [](H2Core::SMF1WriterSingle const &o){ return new H2Core::SMF1WriterSingle(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::SMF1WriterSingle::class_name, "C++: H2Core::SMF1WriterSingle::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class H2Core::SMF1WriterSingle & (H2Core::SMF1WriterSingle::*)(const class H2Core::SMF1WriterSingle &)) &H2Core::SMF1WriterSingle::operator=, "C++: H2Core::SMF1WriterSingle::operator=(const class H2Core::SMF1WriterSingle &) --> class H2Core::SMF1WriterSingle &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B752_[H2Core::SMF1WriterMulti] ";
	{ // H2Core::SMF1WriterMulti file:core/Smf/SMF.h line:147
		pybind11::class_<H2Core::SMF1WriterMulti, std::shared_ptr<H2Core::SMF1WriterMulti>, PyCallBack_H2Core_SMF1WriterMulti, H2Core::SMF1Writer> cl(M("H2Core"), "SMF1WriterMulti", "");
		cl.def( pybind11::init( [](){ return new H2Core::SMF1WriterMulti(); }, [](){ return new PyCallBack_H2Core_SMF1WriterMulti(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_SMF1WriterMulti const &o){ return new PyCallBack_H2Core_SMF1WriterMulti(o); } ) );
		cl.def( pybind11::init( [](H2Core::SMF1WriterMulti const &o){ return new H2Core::SMF1WriterMulti(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::SMF1WriterMulti::class_name, "C++: H2Core::SMF1WriterMulti::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class H2Core::SMF1WriterMulti & (H2Core::SMF1WriterMulti::*)(const class H2Core::SMF1WriterMulti &)) &H2Core::SMF1WriterMulti::operator=, "C++: H2Core::SMF1WriterMulti::operator=(const class H2Core::SMF1WriterMulti &) --> class H2Core::SMF1WriterMulti &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B753_[H2Core::SMF0Writer] ";
	{ // H2Core::SMF0Writer file:core/Smf/SMF.h line:165
		pybind11::class_<H2Core::SMF0Writer, std::shared_ptr<H2Core::SMF0Writer>, PyCallBack_H2Core_SMF0Writer, H2Core::SMFWriter> cl(M("H2Core"), "SMF0Writer", "");
		cl.def( pybind11::init( [](){ return new H2Core::SMF0Writer(); }, [](){ return new PyCallBack_H2Core_SMF0Writer(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_SMF0Writer const &o){ return new PyCallBack_H2Core_SMF0Writer(o); } ) );
		cl.def( pybind11::init( [](H2Core::SMF0Writer const &o){ return new H2Core::SMF0Writer(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::SMF0Writer::class_name, "C++: H2Core::SMF0Writer::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class H2Core::SMF0Writer & (H2Core::SMF0Writer::*)(const class H2Core::SMF0Writer &)) &H2Core::SMF0Writer::operator=, "C++: H2Core::SMF0Writer::operator=(const class H2Core::SMF0Writer &) --> class H2Core::SMF0Writer &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B754_[class std::basic_string<char> H2Core::get_version()] ";
	std::cout << "B755_[bool H2Core::version_older_than(int, int, int)] ";
	// H2Core::version_older_than(int, int, int) file:core/Version.h line:15
	M("H2Core").def("version_older_than", (bool (*)(int, int, int)) &H2Core::version_older_than, "return true of the current version is older than the given values\n\nC++: H2Core::version_older_than(int, int, int) --> bool", pybind11::arg("major"), pybind11::arg("minor"), pybind11::arg("patch"));

}
