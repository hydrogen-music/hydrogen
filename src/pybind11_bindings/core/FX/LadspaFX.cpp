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
#include <core/Basics/Note.h> // 
#include <core/Basics/Note.h> // H2Core::Note
#include <core/Basics/Pattern.h> // H2Core::Pattern
#include <core/Basics/PatternList.h> // H2Core::PatternList
#include <core/Basics/Playlist.h> // H2Core::Playlist
#include <core/Basics/Song.h> // 
#include <core/Basics/Song.h> // H2Core::Song
#include <core/FX/Effects.h> // H2Core::Effects
#include <core/FX/LadspaFX.h> // H2Core::LadspaControlPort
#include <core/FX/LadspaFX.h> // H2Core::LadspaFX
#include <core/FX/LadspaFX.h> // H2Core::LadspaFXGroup
#include <core/FX/LadspaFX.h> // H2Core::LadspaFXInfo
#include <core/H2Exception.h> // H2Core::H2Exception
#include <core/Helpers/Files.h> // H2Core::Files
#include <core/Helpers/Xml.h> // H2Core::XMLNode
#include <core/Object.h> // H2Core::Object
#include <functional> // std::less
#include <iostream> // --trace
#include <iterator> // __gnu_cxx::__normal_iterator
#include <iterator> // std::reverse_iterator
#include <map> // std::multimap
#include <memory> // std::allocator
#include <memory> // std::shared_ptr
#include <set> // std::set
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <utility> // std::pair
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

// H2Core::LadspaFXInfo file:core/FX/LadspaFX.h line:38
struct PyCallBack_H2Core_LadspaFXInfo : public H2Core::LadspaFXInfo {
	using H2Core::LadspaFXInfo::LadspaFXInfo;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::LadspaFXInfo *>(this), "toQString");
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

// H2Core::LadspaFXGroup file:core/FX/LadspaFX.h line:60
struct PyCallBack_H2Core_LadspaFXGroup : public H2Core::LadspaFXGroup {
	using H2Core::LadspaFXGroup::LadspaFXGroup;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::LadspaFXGroup *>(this), "toQString");
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

// H2Core::LadspaControlPort file:core/FX/LadspaFX.h line:96
struct PyCallBack_H2Core_LadspaControlPort : public H2Core::LadspaControlPort {
	using H2Core::LadspaControlPort::LadspaControlPort;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::LadspaControlPort *>(this), "toQString");
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

// H2Core::LadspaFX file:core/FX/LadspaFX.h line:113
struct PyCallBack_H2Core_LadspaFX : public H2Core::LadspaFX {
	using H2Core::LadspaFX::LadspaFX;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::LadspaFX *>(this), "toQString");
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

// H2Core::Effects file:core/FX/Effects.h line:38
struct PyCallBack_H2Core_Effects : public H2Core::Effects {
	using H2Core::Effects::Effects;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::Effects *>(this), "toQString");
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

// H2Core::H2Exception file:core/H2Exception.h line:33
struct PyCallBack_H2Core_H2Exception : public H2Core::H2Exception {
	using H2Core::H2Exception::H2Exception;

};

// H2Core::Files file:core/Helpers/Files.h line:18
struct PyCallBack_H2Core_Files : public H2Core::Files {
	using H2Core::Files::Files;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::Files *>(this), "toQString");
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

void bind_core_FX_LadspaFX(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B694_[H2Core::LadspaFXInfo] ";
	{ // H2Core::LadspaFXInfo file:core/FX/LadspaFX.h line:38
		pybind11::class_<H2Core::LadspaFXInfo, std::shared_ptr<H2Core::LadspaFXInfo>, PyCallBack_H2Core_LadspaFXInfo, H2Core::Object> cl(M("H2Core"), "LadspaFXInfo", "");
		cl.def( pybind11::init<const class QString &>(), pybind11::arg("sName") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_LadspaFXInfo const &o){ return new PyCallBack_H2Core_LadspaFXInfo(o); } ) );
		cl.def( pybind11::init( [](H2Core::LadspaFXInfo const &o){ return new H2Core::LadspaFXInfo(o); } ) );
		cl.def_readwrite("m_sFilename", &H2Core::LadspaFXInfo::m_sFilename);
		cl.def_readwrite("m_sID", &H2Core::LadspaFXInfo::m_sID);
		cl.def_readwrite("m_sLabel", &H2Core::LadspaFXInfo::m_sLabel);
		cl.def_readwrite("m_sName", &H2Core::LadspaFXInfo::m_sName);
		cl.def_readwrite("m_sMaker", &H2Core::LadspaFXInfo::m_sMaker);
		cl.def_readwrite("m_sCopyright", &H2Core::LadspaFXInfo::m_sCopyright);
		cl.def_readwrite("m_nICPorts", &H2Core::LadspaFXInfo::m_nICPorts);
		cl.def_readwrite("m_nOCPorts", &H2Core::LadspaFXInfo::m_nOCPorts);
		cl.def_readwrite("m_nIAPorts", &H2Core::LadspaFXInfo::m_nIAPorts);
		cl.def_readwrite("m_nOAPorts", &H2Core::LadspaFXInfo::m_nOAPorts);
		cl.def_static("class_name", (const char * (*)()) &H2Core::LadspaFXInfo::class_name, "C++: H2Core::LadspaFXInfo::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def_static("alphabeticOrder", (bool (*)(class H2Core::LadspaFXInfo *, class H2Core::LadspaFXInfo *)) &H2Core::LadspaFXInfo::alphabeticOrder, "C++: H2Core::LadspaFXInfo::alphabeticOrder(class H2Core::LadspaFXInfo *, class H2Core::LadspaFXInfo *) --> bool", pybind11::arg("a"), pybind11::arg("b"));
		cl.def("assign", (class H2Core::LadspaFXInfo & (H2Core::LadspaFXInfo::*)(const class H2Core::LadspaFXInfo &)) &H2Core::LadspaFXInfo::operator=, "C++: H2Core::LadspaFXInfo::operator=(const class H2Core::LadspaFXInfo &) --> class H2Core::LadspaFXInfo &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B695_[H2Core::LadspaFXGroup] ";
	{ // H2Core::LadspaFXGroup file:core/FX/LadspaFX.h line:60
		pybind11::class_<H2Core::LadspaFXGroup, std::shared_ptr<H2Core::LadspaFXGroup>, PyCallBack_H2Core_LadspaFXGroup, H2Core::Object> cl(M("H2Core"), "LadspaFXGroup", "");
		cl.def( pybind11::init<const class QString &>(), pybind11::arg("sName") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_LadspaFXGroup const &o){ return new PyCallBack_H2Core_LadspaFXGroup(o); } ) );
		cl.def( pybind11::init( [](H2Core::LadspaFXGroup const &o){ return new H2Core::LadspaFXGroup(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::LadspaFXGroup::class_name, "C++: H2Core::LadspaFXGroup::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("getName", (const class QString & (H2Core::LadspaFXGroup::*)()) &H2Core::LadspaFXGroup::getName, "C++: H2Core::LadspaFXGroup::getName() --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("addLadspaInfo", (void (H2Core::LadspaFXGroup::*)(class H2Core::LadspaFXInfo *)) &H2Core::LadspaFXGroup::addLadspaInfo, "C++: H2Core::LadspaFXGroup::addLadspaInfo(class H2Core::LadspaFXInfo *) --> void", pybind11::arg("pInfo"));
		cl.def("addChild", (void (H2Core::LadspaFXGroup::*)(class H2Core::LadspaFXGroup *)) &H2Core::LadspaFXGroup::addChild, "C++: H2Core::LadspaFXGroup::addChild(class H2Core::LadspaFXGroup *) --> void", pybind11::arg("pChild"));
		cl.def("clear", (void (H2Core::LadspaFXGroup::*)()) &H2Core::LadspaFXGroup::clear, "C++: H2Core::LadspaFXGroup::clear() --> void");
		cl.def_static("alphabeticOrder", (bool (*)(class H2Core::LadspaFXGroup *, class H2Core::LadspaFXGroup *)) &H2Core::LadspaFXGroup::alphabeticOrder, "C++: H2Core::LadspaFXGroup::alphabeticOrder(class H2Core::LadspaFXGroup *, class H2Core::LadspaFXGroup *) --> bool", pybind11::arg(""), pybind11::arg(""));
		cl.def("sort", (void (H2Core::LadspaFXGroup::*)()) &H2Core::LadspaFXGroup::sort, "C++: H2Core::LadspaFXGroup::sort() --> void");
		cl.def("assign", (class H2Core::LadspaFXGroup & (H2Core::LadspaFXGroup::*)(const class H2Core::LadspaFXGroup &)) &H2Core::LadspaFXGroup::operator=, "C++: H2Core::LadspaFXGroup::operator=(const class H2Core::LadspaFXGroup &) --> class H2Core::LadspaFXGroup &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B696_[H2Core::LadspaControlPort] ";
	{ // H2Core::LadspaControlPort file:core/FX/LadspaFX.h line:96
		pybind11::class_<H2Core::LadspaControlPort, std::shared_ptr<H2Core::LadspaControlPort>, PyCallBack_H2Core_LadspaControlPort, H2Core::Object> cl(M("H2Core"), "LadspaControlPort", "");
		cl.def( pybind11::init( [](){ return new H2Core::LadspaControlPort(); }, [](){ return new PyCallBack_H2Core_LadspaControlPort(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_LadspaControlPort const &o){ return new PyCallBack_H2Core_LadspaControlPort(o); } ) );
		cl.def( pybind11::init( [](H2Core::LadspaControlPort const &o){ return new H2Core::LadspaControlPort(o); } ) );
		cl.def_readwrite("sName", &H2Core::LadspaControlPort::sName);
		cl.def_readwrite("isToggle", &H2Core::LadspaControlPort::isToggle);
		cl.def_readwrite("m_bIsInteger", &H2Core::LadspaControlPort::m_bIsInteger);
		cl.def_readwrite("fDefaultValue", &H2Core::LadspaControlPort::fDefaultValue);
		cl.def_readwrite("fControlValue", &H2Core::LadspaControlPort::fControlValue);
		cl.def_readwrite("fLowerBound", &H2Core::LadspaControlPort::fLowerBound);
		cl.def_readwrite("fUpperBound", &H2Core::LadspaControlPort::fUpperBound);
		cl.def_static("class_name", (const char * (*)()) &H2Core::LadspaControlPort::class_name, "C++: H2Core::LadspaControlPort::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class H2Core::LadspaControlPort & (H2Core::LadspaControlPort::*)(const class H2Core::LadspaControlPort &)) &H2Core::LadspaControlPort::operator=, "C++: H2Core::LadspaControlPort::operator=(const class H2Core::LadspaControlPort &) --> class H2Core::LadspaControlPort &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B697_[H2Core::LadspaFX] ";
	{ // H2Core::LadspaFX file:core/FX/LadspaFX.h line:113
		pybind11::class_<H2Core::LadspaFX, std::shared_ptr<H2Core::LadspaFX>, PyCallBack_H2Core_LadspaFX, H2Core::Object> cl(M("H2Core"), "LadspaFX", "");
		cl.def( pybind11::init( [](PyCallBack_H2Core_LadspaFX const &o){ return new PyCallBack_H2Core_LadspaFX(o); } ) );
		cl.def( pybind11::init( [](H2Core::LadspaFX const &o){ return new H2Core::LadspaFX(o); } ) );
		cl.def_readwrite("inputControlPorts", &H2Core::LadspaFX::inputControlPorts);
		cl.def_readwrite("outputControlPorts", &H2Core::LadspaFX::outputControlPorts);
		cl.def_static("class_name", (const char * (*)()) &H2Core::LadspaFX::class_name, "C++: H2Core::LadspaFX::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("connectAudioPorts", (void (H2Core::LadspaFX::*)(float *, float *, float *, float *)) &H2Core::LadspaFX::connectAudioPorts, "C++: H2Core::LadspaFX::connectAudioPorts(float *, float *, float *, float *) --> void", pybind11::arg("pIn_L"), pybind11::arg("pIn_R"), pybind11::arg("pOut_L"), pybind11::arg("pOut_R"));
		cl.def("activate", (void (H2Core::LadspaFX::*)()) &H2Core::LadspaFX::activate, "C++: H2Core::LadspaFX::activate() --> void");
		cl.def("deactivate", (void (H2Core::LadspaFX::*)()) &H2Core::LadspaFX::deactivate, "C++: H2Core::LadspaFX::deactivate() --> void");
		cl.def("processFX", (void (H2Core::LadspaFX::*)(unsigned int)) &H2Core::LadspaFX::processFX, "C++: H2Core::LadspaFX::processFX(unsigned int) --> void", pybind11::arg("nFrames"));
		cl.def("getPluginLabel", (const class QString & (H2Core::LadspaFX::*)()) &H2Core::LadspaFX::getPluginLabel, "C++: H2Core::LadspaFX::getPluginLabel() --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("getPluginName", (const class QString & (H2Core::LadspaFX::*)()) &H2Core::LadspaFX::getPluginName, "C++: H2Core::LadspaFX::getPluginName() --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("setPluginName", (void (H2Core::LadspaFX::*)(const class QString &)) &H2Core::LadspaFX::setPluginName, "C++: H2Core::LadspaFX::setPluginName(const class QString &) --> void", pybind11::arg("sName"));
		cl.def("getLibraryPath", (const class QString & (H2Core::LadspaFX::*)()) &H2Core::LadspaFX::getLibraryPath, "C++: H2Core::LadspaFX::getLibraryPath() --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("isEnabled", (bool (H2Core::LadspaFX::*)()) &H2Core::LadspaFX::isEnabled, "C++: H2Core::LadspaFX::isEnabled() --> bool");
		cl.def("setEnabled", (void (H2Core::LadspaFX::*)(bool)) &H2Core::LadspaFX::setEnabled, "C++: H2Core::LadspaFX::setEnabled(bool) --> void", pybind11::arg("value"));
		cl.def_static("load", (class H2Core::LadspaFX * (*)(const class QString &, const class QString &, long)) &H2Core::LadspaFX::load, "C++: H2Core::LadspaFX::load(const class QString &, const class QString &, long) --> class H2Core::LadspaFX *", pybind11::return_value_policy::automatic, pybind11::arg("sLibraryPath"), pybind11::arg("sPluginLabel"), pybind11::arg("nSampleRate"));
		cl.def("getPluginType", (int (H2Core::LadspaFX::*)()) &H2Core::LadspaFX::getPluginType, "C++: H2Core::LadspaFX::getPluginType() --> int");
		cl.def("setVolume", (void (H2Core::LadspaFX::*)(float)) &H2Core::LadspaFX::setVolume, "C++: H2Core::LadspaFX::setVolume(float) --> void", pybind11::arg("fValue"));
		cl.def("getVolume", (float (H2Core::LadspaFX::*)()) &H2Core::LadspaFX::getVolume, "C++: H2Core::LadspaFX::getVolume() --> float");
		cl.def("assign", (class H2Core::LadspaFX & (H2Core::LadspaFX::*)(const class H2Core::LadspaFX &)) &H2Core::LadspaFX::operator=, "C++: H2Core::LadspaFX::operator=(const class H2Core::LadspaFX &) --> class H2Core::LadspaFX &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B698_[H2Core::Effects] ";
	{ // H2Core::Effects file:core/FX/Effects.h line:38
		pybind11::class_<H2Core::Effects, std::shared_ptr<H2Core::Effects>, PyCallBack_H2Core_Effects, H2Core::Object> cl(M("H2Core"), "Effects", "");
		cl.def( pybind11::init( [](PyCallBack_H2Core_Effects const &o){ return new PyCallBack_H2Core_Effects(o); } ) );
		cl.def( pybind11::init( [](H2Core::Effects const &o){ return new H2Core::Effects(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::Effects::class_name, "C++: H2Core::Effects::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def_static("create_instance", (void (*)()) &H2Core::Effects::create_instance, "If #__instance equals 0, a new Effects\n singleton will be created and stored in it.\n\n It is called in Hydrogen::audioEngine_init().\n\nC++: H2Core::Effects::create_instance() --> void");
		cl.def_static("get_instance", (class H2Core::Effects * (*)()) &H2Core::Effects::get_instance, "Returns a pointer to the current Effects singleton\n stored in #__instance.\n\nC++: H2Core::Effects::get_instance() --> class H2Core::Effects *", pybind11::return_value_policy::automatic);
		cl.def("getLadspaFX", (class H2Core::LadspaFX * (H2Core::Effects::*)(int)) &H2Core::Effects::getLadspaFX, "C++: H2Core::Effects::getLadspaFX(int) --> class H2Core::LadspaFX *", pybind11::return_value_policy::automatic, pybind11::arg("nFX"));
		cl.def("setLadspaFX", (void (H2Core::Effects::*)(class H2Core::LadspaFX *, int)) &H2Core::Effects::setLadspaFX, "C++: H2Core::Effects::setLadspaFX(class H2Core::LadspaFX *, int) --> void", pybind11::arg("pFX"), pybind11::arg("nFX"));
		cl.def("getLadspaFXGroup", (class H2Core::LadspaFXGroup * (H2Core::Effects::*)()) &H2Core::Effects::getLadspaFXGroup, "C++: H2Core::Effects::getLadspaFXGroup() --> class H2Core::LadspaFXGroup *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class H2Core::Effects & (H2Core::Effects::*)(const class H2Core::Effects &)) &H2Core::Effects::operator=, "C++: H2Core::Effects::operator=(const class H2Core::Effects &) --> class H2Core::Effects &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B699_[H2Core::H2Exception] ";
	{ // H2Core::H2Exception file:core/H2Exception.h line:33
		pybind11::class_<H2Core::H2Exception, std::shared_ptr<H2Core::H2Exception>, PyCallBack_H2Core_H2Exception> cl(M("H2Core"), "H2Exception", "");
		cl.def( pybind11::init<const class QString &>(), pybind11::arg("msg") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_H2Exception const &o){ return new PyCallBack_H2Core_H2Exception(o); } ) );
		cl.def( pybind11::init( [](H2Core::H2Exception const &o){ return new H2Core::H2Exception(o); } ) );
		cl.def("assign", (class H2Core::H2Exception & (H2Core::H2Exception::*)(const class H2Core::H2Exception &)) &H2Core::H2Exception::operator=, "C++: H2Core::H2Exception::operator=(const class H2Core::H2Exception &) --> class H2Core::H2Exception &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B700_[H2Core::Files] ";
	{ // H2Core::Files file:core/Helpers/Files.h line:18
		pybind11::class_<H2Core::Files, std::shared_ptr<H2Core::Files>, PyCallBack_H2Core_Files, H2Core::Object> cl(M("H2Core"), "Files", "Files is in charge of writing and reading Patterns, Drumkits, Songs to the filesystem");
		cl.def( pybind11::init( [](){ return new H2Core::Files(); }, [](){ return new PyCallBack_H2Core_Files(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_Files const &o){ return new PyCallBack_H2Core_Files(o); } ) );
		cl.def( pybind11::init( [](H2Core::Files const &o){ return new H2Core::Files(o); } ) );

		pybind11::enum_<H2Core::Files::SaveMode>(cl, "SaveMode", pybind11::arithmetic(), "")
			.value("SAVE_NEW", H2Core::Files::SAVE_NEW)
			.value("SAVE_OVERWRITE", H2Core::Files::SAVE_OVERWRITE)
			.value("SAVE_PATH", H2Core::Files::SAVE_PATH)
			.value("SAVE_TMP", H2Core::Files::SAVE_TMP)
			.export_values();

		cl.def_static("class_name", (const char * (*)()) &H2Core::Files::class_name, "C++: H2Core::Files::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def_static("savePatternNew", (class QString (*)(const class QString &, class H2Core::Pattern *, class H2Core::Song *, const class QString &)) &H2Core::Files::savePatternNew, "save the given pattern to <user_data_path>/pattern/<drumkitName>/<fileName>.h2pattern\n will NOT overwrite an existing file\n \n\n to build filePath from\n \n\n the one to be saved\n \n\n to access license, author info\n \n\n to build filePath from and to access name info\n \n\n filePath on success, NULL on failure\n\nC++: H2Core::Files::savePatternNew(const class QString &, class H2Core::Pattern *, class H2Core::Song *, const class QString &) --> class QString", pybind11::arg("fileName"), pybind11::arg("pattern"), pybind11::arg("song"), pybind11::arg("drumkitName"));
		cl.def_static("savePatternOver", (class QString (*)(const class QString &, class H2Core::Pattern *, class H2Core::Song *, const class QString &)) &H2Core::Files::savePatternOver, "save the given pattern to <user_data_path>/pattern/<drumkitName>/<fileName>.h2pattern\n will overwrite an existing file\n \n\n to build filePath from\n \n\n the one to be saved\n \n\n to access license, author info\n \n\n to build filePath from and to access name info\n \n\n filePath on success, NULL on failure\n\nC++: H2Core::Files::savePatternOver(const class QString &, class H2Core::Pattern *, class H2Core::Song *, const class QString &) --> class QString", pybind11::arg("fileName"), pybind11::arg("pattern"), pybind11::arg("song"), pybind11::arg("drumkitName"));
		cl.def_static("savePatternPath", (class QString (*)(const class QString &, class H2Core::Pattern *, class H2Core::Song *, const class QString &)) &H2Core::Files::savePatternPath, "save the given pattern to \n will overwrite an existing file\n \n\n to write the pattern to\n \n\n the one to be saved\n \n\n to access license, author info\n \n\n to access name info\n \n\n  on success, NULL on failure\n\nC++: H2Core::Files::savePatternPath(const class QString &, class H2Core::Pattern *, class H2Core::Song *, const class QString &) --> class QString", pybind11::arg("filePath"), pybind11::arg("pattern"), pybind11::arg("song"), pybind11::arg("drumkitName"));
		cl.def_static("savePatternTmp", (class QString (*)(const class QString &, class H2Core::Pattern *, class H2Core::Song *, const class QString &)) &H2Core::Files::savePatternTmp, "save the given pattern under <Tmp_directory> with a unique filename built from <fileName>\n will overwrite an existing file\n \n\n to build filePath from\n \n\n the one to be saved\n \n\n to access license, author info\n \n\n to access name info\n \n\n filePath on success, NULL on failure\n\nC++: H2Core::Files::savePatternTmp(const class QString &, class H2Core::Pattern *, class H2Core::Song *, const class QString &) --> class QString", pybind11::arg("fileName"), pybind11::arg("pattern"), pybind11::arg("song"), pybind11::arg("drumkitName"));
		cl.def_static("savePlaylistPath", (class QString (*)(const class QString &, class H2Core::Playlist *, bool)) &H2Core::Files::savePlaylistPath, "save the given playlist to filePath\n will overwrite an existing file\n \n\n to write the playlist to\n \n\n the one to be saved\n \n\n should the path to the songs be relative to the playlist instead of absolute\n \n\n filePath on success, NULL on failure\n\nC++: H2Core::Files::savePlaylistPath(const class QString &, class H2Core::Playlist *, bool) --> class QString", pybind11::arg("filePath"), pybind11::arg("playlist"), pybind11::arg("relativePaths"));
		cl.def("assign", (class H2Core::Files & (H2Core::Files::*)(const class H2Core::Files &)) &H2Core::Files::operator=, "C++: H2Core::Files::operator=(const class H2Core::Files &) --> class H2Core::Files &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
