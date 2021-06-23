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
#include <QtCore/qtextstream.h> // QTextStream
#include <QtCore/qvector.h> // QVector
#include <core/AutomationPathSerializer.h> // H2Core::AutomationPathSerializer
#include <core/Basics/AutomationPath.h> // H2Core::AutomationPath
#include <core/Basics/Drumkit.h> // H2Core::Drumkit
#include <core/Basics/DrumkitComponent.h> // H2Core::DrumkitComponent
#include <core/Basics/Instrument.h> // H2Core::Instrument
#include <core/Basics/InstrumentList.h> // H2Core::InstrumentList
#include <core/Helpers/Filesystem.h> // 
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

// H2Core::DrumkitComponent file:core/Basics/DrumkitComponent.h line:38
struct PyCallBack_H2Core_DrumkitComponent : public H2Core::DrumkitComponent {
	using H2Core::DrumkitComponent::DrumkitComponent;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::DrumkitComponent *>(this), "toQString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return DrumkitComponent::toQString(a0, a1);
	}
};

// H2Core::Drumkit file:core/Basics/Drumkit.h line:39
struct PyCallBack_H2Core_Drumkit : public H2Core::Drumkit {
	using H2Core::Drumkit::Drumkit;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::Drumkit *>(this), "toQString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return Drumkit::toQString(a0, a1);
	}
};

void bind_core_AutomationPathSerializer(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B675_[H2Core::AutomationPathSerializer] ";
	{ // H2Core::AutomationPathSerializer file:core/AutomationPathSerializer.h line:33
		pybind11::class_<H2Core::AutomationPathSerializer, std::shared_ptr<H2Core::AutomationPathSerializer>> cl(M("H2Core"), "AutomationPathSerializer", "");
		cl.def( pybind11::init( [](){ return new H2Core::AutomationPathSerializer(); } ) );
		cl.def( pybind11::init( [](H2Core::AutomationPathSerializer const &o){ return new H2Core::AutomationPathSerializer(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::AutomationPathSerializer::class_name, "C++: H2Core::AutomationPathSerializer::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("read_automation_path", (void (H2Core::AutomationPathSerializer::*)(const class QDomNode &, class H2Core::AutomationPath &)) &H2Core::AutomationPathSerializer::read_automation_path, "C++: H2Core::AutomationPathSerializer::read_automation_path(const class QDomNode &, class H2Core::AutomationPath &) --> void", pybind11::arg("node"), pybind11::arg("path"));
		cl.def("write_automation_path", (void (H2Core::AutomationPathSerializer::*)(class QDomNode &, const class H2Core::AutomationPath &)) &H2Core::AutomationPathSerializer::write_automation_path, "C++: H2Core::AutomationPathSerializer::write_automation_path(class QDomNode &, const class H2Core::AutomationPath &) --> void", pybind11::arg("node"), pybind11::arg("path"));
		cl.def("assign", (class H2Core::AutomationPathSerializer & (H2Core::AutomationPathSerializer::*)(const class H2Core::AutomationPathSerializer &)) &H2Core::AutomationPathSerializer::operator=, "C++: H2Core::AutomationPathSerializer::operator=(const class H2Core::AutomationPathSerializer &) --> class H2Core::AutomationPathSerializer &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B676_[H2Core::DrumkitComponent] ";
	{ // H2Core::DrumkitComponent file:core/Basics/DrumkitComponent.h line:38
		pybind11::class_<H2Core::DrumkitComponent, std::shared_ptr<H2Core::DrumkitComponent>, PyCallBack_H2Core_DrumkitComponent, H2Core::Object> cl(M("H2Core"), "DrumkitComponent", "");
		cl.def( pybind11::init<const int, const class QString &>(), pybind11::arg("id"), pybind11::arg("name") );

		cl.def( pybind11::init<class H2Core::DrumkitComponent *>(), pybind11::arg("other") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_DrumkitComponent const &o){ return new PyCallBack_H2Core_DrumkitComponent(o); } ) );
		cl.def( pybind11::init( [](H2Core::DrumkitComponent const &o){ return new H2Core::DrumkitComponent(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::DrumkitComponent::class_name, "C++: H2Core::DrumkitComponent::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("save_to", (void (H2Core::DrumkitComponent::*)(class H2Core::XMLNode *)) &H2Core::DrumkitComponent::save_to, "C++: H2Core::DrumkitComponent::save_to(class H2Core::XMLNode *) --> void", pybind11::arg("node"));
		cl.def("set_name", (void (H2Core::DrumkitComponent::*)(const class QString &)) &H2Core::DrumkitComponent::set_name, "C++: H2Core::DrumkitComponent::set_name(const class QString &) --> void", pybind11::arg("name"));
		cl.def("get_name", (const class QString & (H2Core::DrumkitComponent::*)() const) &H2Core::DrumkitComponent::get_name, "C++: H2Core::DrumkitComponent::get_name() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("set_id", (void (H2Core::DrumkitComponent::*)(const int)) &H2Core::DrumkitComponent::set_id, "C++: H2Core::DrumkitComponent::set_id(const int) --> void", pybind11::arg("id"));
		cl.def("get_id", (int (H2Core::DrumkitComponent::*)() const) &H2Core::DrumkitComponent::get_id, "C++: H2Core::DrumkitComponent::get_id() const --> int");
		cl.def("set_volume", (void (H2Core::DrumkitComponent::*)(float)) &H2Core::DrumkitComponent::set_volume, "C++: H2Core::DrumkitComponent::set_volume(float) --> void", pybind11::arg("volume"));
		cl.def("get_volume", (float (H2Core::DrumkitComponent::*)() const) &H2Core::DrumkitComponent::get_volume, "C++: H2Core::DrumkitComponent::get_volume() const --> float");
		cl.def("set_muted", (void (H2Core::DrumkitComponent::*)(bool)) &H2Core::DrumkitComponent::set_muted, "C++: H2Core::DrumkitComponent::set_muted(bool) --> void", pybind11::arg("active"));
		cl.def("is_muted", (bool (H2Core::DrumkitComponent::*)() const) &H2Core::DrumkitComponent::is_muted, "C++: H2Core::DrumkitComponent::is_muted() const --> bool");
		cl.def("set_soloed", (void (H2Core::DrumkitComponent::*)(bool)) &H2Core::DrumkitComponent::set_soloed, "C++: H2Core::DrumkitComponent::set_soloed(bool) --> void", pybind11::arg("soloed"));
		cl.def("is_soloed", (bool (H2Core::DrumkitComponent::*)() const) &H2Core::DrumkitComponent::is_soloed, "C++: H2Core::DrumkitComponent::is_soloed() const --> bool");
		cl.def("set_peak_l", (void (H2Core::DrumkitComponent::*)(float)) &H2Core::DrumkitComponent::set_peak_l, "C++: H2Core::DrumkitComponent::set_peak_l(float) --> void", pybind11::arg("val"));
		cl.def("get_peak_l", (float (H2Core::DrumkitComponent::*)() const) &H2Core::DrumkitComponent::get_peak_l, "C++: H2Core::DrumkitComponent::get_peak_l() const --> float");
		cl.def("set_peak_r", (void (H2Core::DrumkitComponent::*)(float)) &H2Core::DrumkitComponent::set_peak_r, "C++: H2Core::DrumkitComponent::set_peak_r(float) --> void", pybind11::arg("val"));
		cl.def("get_peak_r", (float (H2Core::DrumkitComponent::*)() const) &H2Core::DrumkitComponent::get_peak_r, "C++: H2Core::DrumkitComponent::get_peak_r() const --> float");
		cl.def("reset_outs", (void (H2Core::DrumkitComponent::*)(unsigned int)) &H2Core::DrumkitComponent::reset_outs, "C++: H2Core::DrumkitComponent::reset_outs(unsigned int) --> void", pybind11::arg("nFrames"));
		cl.def("set_outs", (void (H2Core::DrumkitComponent::*)(int, float, float)) &H2Core::DrumkitComponent::set_outs, "C++: H2Core::DrumkitComponent::set_outs(int, float, float) --> void", pybind11::arg("nBufferPos"), pybind11::arg("valL"), pybind11::arg("valR"));
		cl.def("get_out_L", (float (H2Core::DrumkitComponent::*)(int)) &H2Core::DrumkitComponent::get_out_L, "C++: H2Core::DrumkitComponent::get_out_L(int) --> float", pybind11::arg("nBufferPos"));
		cl.def("get_out_R", (float (H2Core::DrumkitComponent::*)(int)) &H2Core::DrumkitComponent::get_out_R, "C++: H2Core::DrumkitComponent::get_out_R(int) --> float", pybind11::arg("nBufferPos"));
		cl.def("toQString", [](H2Core::DrumkitComponent const &o, const class QString & a0) -> QString { return o.toQString(a0); }, "", pybind11::arg("sPrefix"));
		cl.def("toQString", (class QString (H2Core::DrumkitComponent::*)(const class QString &, bool) const) &H2Core::DrumkitComponent::toQString, "Formatted string version for debugging purposes.\n \n\n String prefix which will be added in front of\n every new line\n \n\n Instead of the whole content of all classes\n stored as members just a single unique identifier will be\n displayed without line breaks.\n\n \n String presentation of current object.\n\nC++: H2Core::DrumkitComponent::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
		cl.def("assign", (class H2Core::DrumkitComponent & (H2Core::DrumkitComponent::*)(const class H2Core::DrumkitComponent &)) &H2Core::DrumkitComponent::operator=, "C++: H2Core::DrumkitComponent::operator=(const class H2Core::DrumkitComponent &) --> class H2Core::DrumkitComponent &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B677_[H2Core::Drumkit] ";
	{ // H2Core::Drumkit file:core/Basics/Drumkit.h line:39
		pybind11::class_<H2Core::Drumkit, std::shared_ptr<H2Core::Drumkit>, PyCallBack_H2Core_Drumkit, H2Core::Object> cl(M("H2Core"), "Drumkit", "Drumkit info");
		cl.def( pybind11::init( [](){ return new H2Core::Drumkit(); }, [](){ return new PyCallBack_H2Core_Drumkit(); } ) );
		cl.def( pybind11::init<class H2Core::Drumkit *>(), pybind11::arg("other") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_Drumkit const &o){ return new PyCallBack_H2Core_Drumkit(o); } ) );
		cl.def( pybind11::init( [](H2Core::Drumkit const &o){ return new H2Core::Drumkit(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::Drumkit::class_name, "C++: H2Core::Drumkit::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def_static("load", [](const class QString & a0) -> H2Core::Drumkit * { return H2Core::Drumkit::load(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("dk_dir"));
		cl.def_static("load", (class H2Core::Drumkit * (*)(const class QString &, const bool)) &H2Core::Drumkit::load, "Load drumkit information from a directory.\n\n This function is a wrapper around load_file(). The\n provided drumkit directory  is converted\n by Filesystem::drumkit_file() internally.\n\n \n A directory containing a drumkit,\n like those returned by\n Filesystem::drumkit_dir_search().\n \n\n Automatically load sample data\n if set to true.\n\n \n A Drumkit on success, nullptr otherwise.\n\nC++: H2Core::Drumkit::load(const class QString &, const bool) --> class H2Core::Drumkit *", pybind11::return_value_policy::automatic, pybind11::arg("dk_dir"), pybind11::arg("load_samples"));
		cl.def_static("load_by_name", [](const class QString & a0) -> H2Core::Drumkit * { return H2Core::Drumkit::load_by_name(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("dk_name"));
		cl.def_static("load_by_name", [](const class QString & a0, const bool & a1) -> H2Core::Drumkit * { return H2Core::Drumkit::load_by_name(a0, a1); }, "", pybind11::return_value_policy::automatic, pybind11::arg("dk_name"), pybind11::arg("load_samples"));
		cl.def_static("load_by_name", (class H2Core::Drumkit * (*)(const class QString &, const bool, enum H2Core::Filesystem::Lookup)) &H2Core::Drumkit::load_by_name, "Simple wrapper for load() used with the drumkit's\n name instead of its directory.\n\n Uses Filesystem::drumkit_path_search() to determine\n the directory of the Drumkit from \n\n \n Name of the Drumkit.\n \n\n Automatically load sample data\n if set to true.\n \n\n Where to search (system/user folder or both)\n for the drumkit.\n\n \n A Drumkit on success, nullptr otherwise.\n\nC++: H2Core::Drumkit::load_by_name(const class QString &, const bool, enum H2Core::Filesystem::Lookup) --> class H2Core::Drumkit *", pybind11::return_value_policy::automatic, pybind11::arg("dk_name"), pybind11::arg("load_samples"), pybind11::arg("lookup"));
		cl.def_static("load_file", [](const class QString & a0) -> H2Core::Drumkit * { return H2Core::Drumkit::load_file(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("dk_path"));
		cl.def_static("load_file", (class H2Core::Drumkit * (*)(const class QString &, const bool)) &H2Core::Drumkit::load_file, "Load a Drumkit from a file.\n\n If the drumkit in  can not be validated\n against the current XML Schema definition in\n Filesystem::drumkit_xsd_path(), it will be loaded\n using Legacy::load_drumkit() and, if successful,\n saved again using save_file() to update the drumkit\n file to the newest version. If, instead, the\n Drumkit is valid, it is loaded using load_from()\n and load_samples() is triggered if \n is true.\n\n \n is a path to an xml file\n \n\n automatically load sample data if set to true\n\n \n A Drumkit on success, nullptr otherwise.\n\nC++: H2Core::Drumkit::load_file(const class QString &, const bool) --> class H2Core::Drumkit *", pybind11::return_value_policy::automatic, pybind11::arg("dk_path"), pybind11::arg("load_samples"));
		cl.def("load_samples", (void (H2Core::Drumkit::*)()) &H2Core::Drumkit::load_samples, "Calls the InstrumentList::load_samples() member\n function of #__instruments.\n\nC++: H2Core::Drumkit::load_samples() --> void");
		cl.def("unload_samples", (void (H2Core::Drumkit::*)()) &H2Core::Drumkit::unload_samples, "Calls the InstrumentList::unload_samples() member\n function of #__instruments.\n\nC++: H2Core::Drumkit::unload_samples() --> void");
		cl.def_static("upgrade_drumkit", (void (*)(class H2Core::Drumkit *, const class QString &)) &H2Core::Drumkit::upgrade_drumkit, "Saves the current drumkit to dk_path, but makes a backup. \n This is used when the drumkit did not comply to \n our xml schema.\n\nC++: H2Core::Drumkit::upgrade_drumkit(class H2Core::Drumkit *, const class QString &) --> void", pybind11::arg("pDrumkit"), pybind11::arg("dk_path"));
		cl.def_static("user_drumkit_exists", (bool (*)(const class QString &)) &H2Core::Drumkit::user_drumkit_exists, "check if a user drumkit with the given name\n already exists\n \n\n Drumkit path\n \n\n true on success\n\nC++: H2Core::Drumkit::user_drumkit_exists(const class QString &) --> bool", pybind11::arg("dk_path"));
		cl.def("save", [](H2Core::Drumkit &o) -> bool { return o.save(); }, "");
		cl.def("save", (bool (H2Core::Drumkit::*)(bool)) &H2Core::Drumkit::save, "save a drumkit, xml file and samples\n \n\n allows to write over existing drumkit files\n \n\n true on success\n\nC++: H2Core::Drumkit::save(bool) --> bool", pybind11::arg("overwrite"));
		cl.def("save", [](H2Core::Drumkit &o, const class QString & a0) -> bool { return o.save(a0); }, "", pybind11::arg("dk_dir"));
		cl.def("save", (bool (H2Core::Drumkit::*)(const class QString &, bool)) &H2Core::Drumkit::save, "save a drumkit, xml file and samples\n neither #__path nor #__name are updated\n \n\n the directory to save the drumkit into\n \n\n allows to write over existing drumkit files\n \n\n true on success\n\nC++: H2Core::Drumkit::save(const class QString &, bool) --> bool", pybind11::arg("dk_dir"), pybind11::arg("overwrite"));
		cl.def("save_file", [](H2Core::Drumkit &o, const class QString & a0) -> bool { return o.save_file(a0); }, "", pybind11::arg("dk_path"));
		cl.def("save_file", [](H2Core::Drumkit &o, const class QString & a0, bool const & a1) -> bool { return o.save_file(a0, a1); }, "", pybind11::arg("dk_path"), pybind11::arg("overwrite"));
		cl.def("save_file", (bool (H2Core::Drumkit::*)(const class QString &, bool, int)) &H2Core::Drumkit::save_file, "save a drumkit into an xml file\n \n\n the path to save the drumkit into\n \n\n allows to write over existing drumkit file\n \n\n to chose the component to save or -1 for all\n \n\n true on success\n\nC++: H2Core::Drumkit::save_file(const class QString &, bool, int) --> bool", pybind11::arg("dk_path"), pybind11::arg("overwrite"), pybind11::arg("component_id"));
		cl.def("save_samples", [](H2Core::Drumkit &o, const class QString & a0) -> bool { return o.save_samples(a0); }, "", pybind11::arg("dk_dir"));
		cl.def("save_samples", (bool (H2Core::Drumkit::*)(const class QString &, bool)) &H2Core::Drumkit::save_samples, "save a drumkit instruments samples into a directory\n \n\n the directory to save the samples into\n \n\n allows to write over existing drumkit samples files\n \n\n true on success\n\nC++: H2Core::Drumkit::save_samples(const class QString &, bool) --> bool", pybind11::arg("dk_dir"), pybind11::arg("overwrite"));
		cl.def("save_image", [](H2Core::Drumkit &o, const class QString & a0) -> bool { return o.save_image(a0); }, "", pybind11::arg("dk_dir"));
		cl.def("save_image", (bool (H2Core::Drumkit::*)(const class QString &, bool)) &H2Core::Drumkit::save_image, "save the drumkit image into the new directory\n \n\n the directory to save the image into\n \n\n allows to write over existing drumkit image file\n \n\n true on success\n\nC++: H2Core::Drumkit::save_image(const class QString &, bool) --> bool", pybind11::arg("dk_dir"), pybind11::arg("overwrite"));
		cl.def_static("install", (bool (*)(const class QString &)) &H2Core::Drumkit::install, "install a drumkit from a filename\n \n\n the path to the new drumkit archive\n \n\n true on success\n\nC++: H2Core::Drumkit::install(const class QString &) --> bool", pybind11::arg("path"));
		cl.def_static("remove", (bool (*)(const class QString &, enum H2Core::Filesystem::Lookup)) &H2Core::Drumkit::remove, "remove a drumkit from the disk\n \n\n the drumkit name\n \n\n Where to search (system/user folder or both)\n for the drumkit.\n \n\n true on success\n\nC++: H2Core::Drumkit::remove(const class QString &, enum H2Core::Filesystem::Lookup) --> bool", pybind11::arg("dk_name"), pybind11::arg("lookup"));
		cl.def("set_instruments", (void (H2Core::Drumkit::*)(class H2Core::InstrumentList *)) &H2Core::Drumkit::set_instruments, "set __instruments, delete existing one \n\nC++: H2Core::Drumkit::set_instruments(class H2Core::InstrumentList *) --> void", pybind11::arg("instruments"));
		cl.def("get_instruments", (class H2Core::InstrumentList * (H2Core::Drumkit::*)() const) &H2Core::Drumkit::get_instruments, "returns #__instruments \n\nC++: H2Core::Drumkit::get_instruments() const --> class H2Core::InstrumentList *", pybind11::return_value_policy::automatic);
		cl.def("set_path", (void (H2Core::Drumkit::*)(const class QString &)) &H2Core::Drumkit::set_path, "#__path setter \n\nC++: H2Core::Drumkit::set_path(const class QString &) --> void", pybind11::arg("path"));
		cl.def("get_path", (const class QString & (H2Core::Drumkit::*)() const) &H2Core::Drumkit::get_path, "#__path accessor \n\nC++: H2Core::Drumkit::get_path() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("set_name", (void (H2Core::Drumkit::*)(const class QString &)) &H2Core::Drumkit::set_name, "#__name setter \n\nC++: H2Core::Drumkit::set_name(const class QString &) --> void", pybind11::arg("name"));
		cl.def("get_name", (const class QString & (H2Core::Drumkit::*)() const) &H2Core::Drumkit::get_name, "#__name accessor \n\nC++: H2Core::Drumkit::get_name() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("set_author", (void (H2Core::Drumkit::*)(const class QString &)) &H2Core::Drumkit::set_author, "#__author setter \n\nC++: H2Core::Drumkit::set_author(const class QString &) --> void", pybind11::arg("author"));
		cl.def("get_author", (const class QString & (H2Core::Drumkit::*)() const) &H2Core::Drumkit::get_author, "#__author accessor \n\nC++: H2Core::Drumkit::get_author() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("set_info", (void (H2Core::Drumkit::*)(const class QString &)) &H2Core::Drumkit::set_info, "#__info setter \n\nC++: H2Core::Drumkit::set_info(const class QString &) --> void", pybind11::arg("info"));
		cl.def("get_info", (const class QString & (H2Core::Drumkit::*)() const) &H2Core::Drumkit::get_info, "#__info accessor \n\nC++: H2Core::Drumkit::get_info() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("set_license", (void (H2Core::Drumkit::*)(const class QString &)) &H2Core::Drumkit::set_license, "#__license setter \n\nC++: H2Core::Drumkit::set_license(const class QString &) --> void", pybind11::arg("license"));
		cl.def("get_license", (const class QString & (H2Core::Drumkit::*)() const) &H2Core::Drumkit::get_license, "#__license accessor \n\nC++: H2Core::Drumkit::get_license() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("set_image", (void (H2Core::Drumkit::*)(const class QString &)) &H2Core::Drumkit::set_image, "#__image setter \n\nC++: H2Core::Drumkit::set_image(const class QString &) --> void", pybind11::arg("image"));
		cl.def("get_image", (const class QString & (H2Core::Drumkit::*)() const) &H2Core::Drumkit::get_image, "#__image accessor \n\nC++: H2Core::Drumkit::get_image() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("set_image_license", (void (H2Core::Drumkit::*)(const class QString &)) &H2Core::Drumkit::set_image_license, "#__imageLicense setter \n\nC++: H2Core::Drumkit::set_image_license(const class QString &) --> void", pybind11::arg("imageLicense"));
		cl.def("get_image_license", (const class QString & (H2Core::Drumkit::*)() const) &H2Core::Drumkit::get_image_license, "#__imageLicense accessor \n\nC++: H2Core::Drumkit::get_image_license() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("samples_loaded", (const bool (H2Core::Drumkit::*)() const) &H2Core::Drumkit::samples_loaded, "return true if the samples are loaded \n\nC++: H2Core::Drumkit::samples_loaded() const --> const bool");
		cl.def("dump", (void (H2Core::Drumkit::*)()) &H2Core::Drumkit::dump, "C++: H2Core::Drumkit::dump() --> void");
		cl.def("isUserDrumkit", (bool (H2Core::Drumkit::*)() const) &H2Core::Drumkit::isUserDrumkit, "Whether the associated files are located in the\n user or the systems drumkit folder.\n\nC++: H2Core::Drumkit::isUserDrumkit() const --> bool");
		cl.def("toQString", [](H2Core::Drumkit const &o, const class QString & a0) -> QString { return o.toQString(a0); }, "", pybind11::arg("sPrefix"));
		cl.def("toQString", (class QString (H2Core::Drumkit::*)(const class QString &, bool) const) &H2Core::Drumkit::toQString, "Formatted string version for debugging purposes.\n \n\n String prefix which will be added in front of\n every new line\n \n\n Instead of the whole content of all classes\n stored as members just a single unique identifier will be\n displayed without line breaks.\n\n \n String presentation of current object.\n\nC++: H2Core::Drumkit::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
		cl.def("assign", (class H2Core::Drumkit & (H2Core::Drumkit::*)(const class H2Core::Drumkit &)) &H2Core::Drumkit::operator=, "C++: H2Core::Drumkit::operator=(const class H2Core::Drumkit &) --> class H2Core::Drumkit &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
