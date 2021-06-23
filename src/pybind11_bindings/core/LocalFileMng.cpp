#include <QtCore/qbytearray.h> // 
#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qbytearray.h> // QByteArrayDataPtr
#include <QtCore/qbytearray.h> // QByteRef
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qchar.h> // QLatin1Char
#include <QtCore/qflags.h> // QFlag
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qiodevice.h> // QIODevice
#include <QtCore/qlist.h> // QList
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::GlobalColor
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
#include <QtGui/qrgba64.h> // QRgba64
#include <core/Basics/AutomationPath.h> // H2Core::AutomationPath
#include <core/Basics/DrumkitComponent.h> // H2Core::DrumkitComponent
#include <core/Basics/Instrument.h> // H2Core::Instrument
#include <core/Basics/InstrumentList.h> // H2Core::InstrumentList
#include <core/Basics/PatternList.h> // H2Core::PatternList
#include <core/Basics/Song.h> // 
#include <core/Basics/Song.h> // H2Core::Song
#include <core/LocalFileMng.h> // H2Core::LocalFileMng
#include <core/LocalFileMng.h> // H2Core::SongWriter
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

// H2Core::LocalFileMng file:core/LocalFileMng.h line:51
struct PyCallBack_H2Core_LocalFileMng : public H2Core::LocalFileMng {
	using H2Core::LocalFileMng::LocalFileMng;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::LocalFileMng *>(this), "toQString");
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

// H2Core::SongWriter file:core/LocalFileMng.h line:84
struct PyCallBack_H2Core_SongWriter : public H2Core::SongWriter {
	using H2Core::SongWriter::SongWriter;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::SongWriter *>(this), "toQString");
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

void bind_core_LocalFileMng(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B730_[H2Core::LocalFileMng] ";
	{ // H2Core::LocalFileMng file:core/LocalFileMng.h line:51
		pybind11::class_<H2Core::LocalFileMng, std::shared_ptr<H2Core::LocalFileMng>, PyCallBack_H2Core_LocalFileMng, H2Core::Object> cl(M("H2Core"), "LocalFileMng", "");
		cl.def( pybind11::init( [](){ return new H2Core::LocalFileMng(); }, [](){ return new PyCallBack_H2Core_LocalFileMng(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_LocalFileMng const &o){ return new PyCallBack_H2Core_LocalFileMng(o); } ) );
		cl.def( pybind11::init( [](H2Core::LocalFileMng const &o){ return new H2Core::LocalFileMng(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::LocalFileMng::class_name, "C++: H2Core::LocalFileMng::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("getDrumkitNameForPattern", (class QString (H2Core::LocalFileMng::*)(const class QString &)) &H2Core::LocalFileMng::getDrumkitNameForPattern, "C++: H2Core::LocalFileMng::getDrumkitNameForPattern(const class QString &) --> class QString", pybind11::arg("patternDir"));
		cl.def_static("writeXmlString", (void (*)(class QDomNode, const class QString &, const class QString &)) &H2Core::LocalFileMng::writeXmlString, "C++: H2Core::LocalFileMng::writeXmlString(class QDomNode, const class QString &, const class QString &) --> void", pybind11::arg("parent"), pybind11::arg("name"), pybind11::arg("text"));
		cl.def_static("writeXmlColor", (void (*)(class QDomNode, const class QString &, const class QColor &)) &H2Core::LocalFileMng::writeXmlColor, "C++: H2Core::LocalFileMng::writeXmlColor(class QDomNode, const class QString &, const class QColor &) --> void", pybind11::arg("parent"), pybind11::arg("name"), pybind11::arg("color"));
		cl.def_static("writeXmlBool", (void (*)(class QDomNode, const class QString &, bool)) &H2Core::LocalFileMng::writeXmlBool, "C++: H2Core::LocalFileMng::writeXmlBool(class QDomNode, const class QString &, bool) --> void", pybind11::arg("parent"), pybind11::arg("name"), pybind11::arg("value"));
		cl.def_static("readXmlString", [](class QDomNode const & a0, const class QString & a1, const class QString & a2) -> QString { return H2Core::LocalFileMng::readXmlString(a0, a1, a2); }, "", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"));
		cl.def_static("readXmlString", [](class QDomNode const & a0, const class QString & a1, const class QString & a2, bool const & a3) -> QString { return H2Core::LocalFileMng::readXmlString(a0, a1, a2, a3); }, "", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"), pybind11::arg("bCanBeEmpty"));
		cl.def_static("readXmlString", [](class QDomNode const & a0, const class QString & a1, const class QString & a2, bool const & a3, bool const & a4) -> QString { return H2Core::LocalFileMng::readXmlString(a0, a1, a2, a3, a4); }, "", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"), pybind11::arg("bCanBeEmpty"), pybind11::arg("bShouldExists"));
		cl.def_static("readXmlString", (class QString (*)(class QDomNode, const class QString &, const class QString &, bool, bool, bool)) &H2Core::LocalFileMng::readXmlString, "C++: H2Core::LocalFileMng::readXmlString(class QDomNode, const class QString &, const class QString &, bool, bool, bool) --> class QString", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"), pybind11::arg("bCanBeEmpty"), pybind11::arg("bShouldExists"), pybind11::arg("tinyXmlCompatMode"));
		cl.def_static("readXmlColor", [](class QDomNode const & a0, const class QString & a1) -> QColor { return H2Core::LocalFileMng::readXmlColor(a0, a1); }, "", pybind11::arg(""), pybind11::arg("nodeName"));
		cl.def_static("readXmlColor", [](class QDomNode const & a0, const class QString & a1, const class QColor & a2) -> QColor { return H2Core::LocalFileMng::readXmlColor(a0, a1, a2); }, "", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"));
		cl.def_static("readXmlColor", [](class QDomNode const & a0, const class QString & a1, const class QColor & a2, bool const & a3) -> QColor { return H2Core::LocalFileMng::readXmlColor(a0, a1, a2, a3); }, "", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"), pybind11::arg("bCanBeEmpty"));
		cl.def_static("readXmlColor", [](class QDomNode const & a0, const class QString & a1, const class QColor & a2, bool const & a3, bool const & a4) -> QColor { return H2Core::LocalFileMng::readXmlColor(a0, a1, a2, a3, a4); }, "", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"), pybind11::arg("bCanBeEmpty"), pybind11::arg("bShouldExists"));
		cl.def_static("readXmlColor", (class QColor (*)(class QDomNode, const class QString &, const class QColor &, bool, bool, bool)) &H2Core::LocalFileMng::readXmlColor, "C++: H2Core::LocalFileMng::readXmlColor(class QDomNode, const class QString &, const class QColor &, bool, bool, bool) --> class QColor", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"), pybind11::arg("bCanBeEmpty"), pybind11::arg("bShouldExists"), pybind11::arg("tinyXmlCompatMode"));
		cl.def_static("readXmlFloat", [](class QDomNode const & a0, const class QString & a1, float const & a2) -> float { return H2Core::LocalFileMng::readXmlFloat(a0, a1, a2); }, "", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"));
		cl.def_static("readXmlFloat", [](class QDomNode const & a0, const class QString & a1, float const & a2, bool const & a3) -> float { return H2Core::LocalFileMng::readXmlFloat(a0, a1, a2, a3); }, "", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"), pybind11::arg("bCanBeEmpty"));
		cl.def_static("readXmlFloat", [](class QDomNode const & a0, const class QString & a1, float const & a2, bool const & a3, bool const & a4) -> float { return H2Core::LocalFileMng::readXmlFloat(a0, a1, a2, a3, a4); }, "", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"), pybind11::arg("bCanBeEmpty"), pybind11::arg("bShouldExists"));
		cl.def_static("readXmlFloat", (float (*)(class QDomNode, const class QString &, float, bool, bool, bool)) &H2Core::LocalFileMng::readXmlFloat, "C++: H2Core::LocalFileMng::readXmlFloat(class QDomNode, const class QString &, float, bool, bool, bool) --> float", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"), pybind11::arg("bCanBeEmpty"), pybind11::arg("bShouldExists"), pybind11::arg("tinyXmlCompatMode"));
		cl.def_static("readXmlFloat", [](class QDomNode const & a0, const class QString & a1, float const & a2, bool * a3) -> float { return H2Core::LocalFileMng::readXmlFloat(a0, a1, a2, a3); }, "", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"), pybind11::arg("pFound"));
		cl.def_static("readXmlFloat", [](class QDomNode const & a0, const class QString & a1, float const & a2, bool * a3, bool const & a4) -> float { return H2Core::LocalFileMng::readXmlFloat(a0, a1, a2, a3, a4); }, "", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"), pybind11::arg("pFound"), pybind11::arg("bCanBeEmpty"));
		cl.def_static("readXmlFloat", [](class QDomNode const & a0, const class QString & a1, float const & a2, bool * a3, bool const & a4, bool const & a5) -> float { return H2Core::LocalFileMng::readXmlFloat(a0, a1, a2, a3, a4, a5); }, "", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"), pybind11::arg("pFound"), pybind11::arg("bCanBeEmpty"), pybind11::arg("bShouldExists"));
		cl.def_static("readXmlFloat", (float (*)(class QDomNode, const class QString &, float, bool *, bool, bool, bool)) &H2Core::LocalFileMng::readXmlFloat, "C++: H2Core::LocalFileMng::readXmlFloat(class QDomNode, const class QString &, float, bool *, bool, bool, bool) --> float", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"), pybind11::arg("pFound"), pybind11::arg("bCanBeEmpty"), pybind11::arg("bShouldExists"), pybind11::arg("tinyXmlCompatMode"));
		cl.def_static("readXmlInt", [](class QDomNode const & a0, const class QString & a1, int const & a2) -> int { return H2Core::LocalFileMng::readXmlInt(a0, a1, a2); }, "", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"));
		cl.def_static("readXmlInt", [](class QDomNode const & a0, const class QString & a1, int const & a2, bool const & a3) -> int { return H2Core::LocalFileMng::readXmlInt(a0, a1, a2, a3); }, "", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"), pybind11::arg("bCanBeEmpty"));
		cl.def_static("readXmlInt", [](class QDomNode const & a0, const class QString & a1, int const & a2, bool const & a3, bool const & a4) -> int { return H2Core::LocalFileMng::readXmlInt(a0, a1, a2, a3, a4); }, "", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"), pybind11::arg("bCanBeEmpty"), pybind11::arg("bShouldExists"));
		cl.def_static("readXmlInt", (int (*)(class QDomNode, const class QString &, int, bool, bool, bool)) &H2Core::LocalFileMng::readXmlInt, "C++: H2Core::LocalFileMng::readXmlInt(class QDomNode, const class QString &, int, bool, bool, bool) --> int", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"), pybind11::arg("bCanBeEmpty"), pybind11::arg("bShouldExists"), pybind11::arg("tinyXmlCompatMode"));
		cl.def_static("readXmlBool", [](class QDomNode const & a0, const class QString & a1, bool const & a2) -> bool { return H2Core::LocalFileMng::readXmlBool(a0, a1, a2); }, "", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"));
		cl.def_static("readXmlBool", [](class QDomNode const & a0, const class QString & a1, bool const & a2, bool const & a3) -> bool { return H2Core::LocalFileMng::readXmlBool(a0, a1, a2, a3); }, "", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"), pybind11::arg("bShouldExists"));
		cl.def_static("readXmlBool", (bool (*)(class QDomNode, const class QString &, bool, bool, bool)) &H2Core::LocalFileMng::readXmlBool, "C++: H2Core::LocalFileMng::readXmlBool(class QDomNode, const class QString &, bool, bool, bool) --> bool", pybind11::arg(""), pybind11::arg("nodeName"), pybind11::arg("defaultValue"), pybind11::arg("bShouldExists"), pybind11::arg("tinyXmlCompatMode"));
		cl.def_static("checkTinyXMLCompatMode", (bool (*)(const class QString &)) &H2Core::LocalFileMng::checkTinyXMLCompatMode, "C++: H2Core::LocalFileMng::checkTinyXMLCompatMode(const class QString &) --> bool", pybind11::arg("filename"));
		cl.def_static("openXmlDocument", (class QDomDocument (*)(const class QString &)) &H2Core::LocalFileMng::openXmlDocument, "C++: H2Core::LocalFileMng::openXmlDocument(const class QString &) --> class QDomDocument", pybind11::arg("filename"));
		cl.def("assign", (class H2Core::LocalFileMng & (H2Core::LocalFileMng::*)(const class H2Core::LocalFileMng &)) &H2Core::LocalFileMng::operator=, "C++: H2Core::LocalFileMng::operator=(const class H2Core::LocalFileMng &) --> class H2Core::LocalFileMng &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B731_[H2Core::SongWriter] ";
	{ // H2Core::SongWriter file:core/LocalFileMng.h line:84
		pybind11::class_<H2Core::SongWriter, std::shared_ptr<H2Core::SongWriter>, PyCallBack_H2Core_SongWriter, H2Core::Object> cl(M("H2Core"), "SongWriter", "Write XML file of a song");
		cl.def( pybind11::init( [](){ return new H2Core::SongWriter(); }, [](){ return new PyCallBack_H2Core_SongWriter(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_SongWriter const &o){ return new PyCallBack_H2Core_SongWriter(o); } ) );
		cl.def( pybind11::init( [](H2Core::SongWriter const &o){ return new H2Core::SongWriter(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::SongWriter::class_name, "C++: H2Core::SongWriter::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("writeSong", (int (H2Core::SongWriter::*)(class H2Core::Song *, const class QString &)) &H2Core::SongWriter::writeSong, "C++: H2Core::SongWriter::writeSong(class H2Core::Song *, const class QString &) --> int", pybind11::arg("song"), pybind11::arg("filename"));
		cl.def("assign", (class H2Core::SongWriter & (H2Core::SongWriter::*)(const class H2Core::SongWriter &)) &H2Core::SongWriter::operator=, "C++: H2Core::SongWriter::operator=(const class H2Core::SongWriter &) --> class H2Core::SongWriter &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
