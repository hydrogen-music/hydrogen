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
#include <core/Basics/Playlist.h> // H2Core::Playlist
#include <core/Basics/Playlist.h> // H2Core::Playlist::Entry
#include <core/Helpers/Xml.h> // H2Core::XMLDoc
#include <core/Helpers/Xml.h> // H2Core::XMLNode
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

// H2Core::XMLNode file:core/Helpers/Xml.h line:15
struct PyCallBack_H2Core_XMLNode : public H2Core::XMLNode {
	using H2Core::XMLNode::XMLNode;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::XMLNode *>(this), "toQString");
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

// H2Core::XMLDoc file:core/Helpers/Xml.h line:130
struct PyCallBack_H2Core_XMLDoc : public H2Core::XMLDoc {
	using H2Core::XMLDoc::XMLDoc;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::XMLDoc *>(this), "toQString");
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

// H2Core::Playlist file:core/Basics/Playlist.h line:35
struct PyCallBack_H2Core_Playlist : public H2Core::Playlist {
	using H2Core::Playlist::Playlist;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::Playlist *>(this), "toQString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return Playlist::toQString(a0, a1);
	}
};

void bind_core_Helpers_Xml(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B683_[H2Core::XMLNode] ";
	{ // H2Core::XMLNode file:core/Helpers/Xml.h line:15
		pybind11::class_<H2Core::XMLNode, std::shared_ptr<H2Core::XMLNode>, PyCallBack_H2Core_XMLNode, H2Core::Object, QDomNode> cl(M("H2Core"), "XMLNode", "XMLNode is a subclass of QDomNode with read and write values methods");
		cl.def( pybind11::init( [](){ return new H2Core::XMLNode(); }, [](){ return new PyCallBack_H2Core_XMLNode(); } ) );
		cl.def( pybind11::init<class QDomNode>(), pybind11::arg("node") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_XMLNode const &o){ return new PyCallBack_H2Core_XMLNode(o); } ) );
		cl.def( pybind11::init( [](H2Core::XMLNode const &o){ return new H2Core::XMLNode(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::XMLNode::class_name, "C++: H2Core::XMLNode::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("createNode", (class H2Core::XMLNode (H2Core::XMLNode::*)(const class QString &)) &H2Core::XMLNode::createNode, "create a new XMLNode that has to be appended into de XMLDoc\n \n\n the name of the node to create\n \n\n the newly created node\n\nC++: H2Core::XMLNode::createNode(const class QString &) --> class H2Core::XMLNode", pybind11::arg("name"));
		cl.def("read_int", [](H2Core::XMLNode &o, const class QString & a0, int const & a1) -> int { return o.read_int(a0, a1); }, "", pybind11::arg("node"), pybind11::arg("default_value"));
		cl.def("read_int", [](H2Core::XMLNode &o, const class QString & a0, int const & a1, bool const & a2) -> int { return o.read_int(a0, a1, a2); }, "", pybind11::arg("node"), pybind11::arg("default_value"), pybind11::arg("inexistent_ok"));
		cl.def("read_int", (int (H2Core::XMLNode::*)(const class QString &, int, bool, bool)) &H2Core::XMLNode::read_int, "reads an integer stored into a child node\n \n\n the name of the child node to read into\n \n\n the value returned if something goes wrong\n \n\n if set to false output a DEBUG log line if the node doesn't exists\n \n\n if set to false output a DEBUG log line if the child node is empty\n\nC++: H2Core::XMLNode::read_int(const class QString &, int, bool, bool) --> int", pybind11::arg("node"), pybind11::arg("default_value"), pybind11::arg("inexistent_ok"), pybind11::arg("empty_ok"));
		cl.def("read_bool", [](H2Core::XMLNode &o, const class QString & a0, bool const & a1) -> bool { return o.read_bool(a0, a1); }, "", pybind11::arg("node"), pybind11::arg("default_value"));
		cl.def("read_bool", [](H2Core::XMLNode &o, const class QString & a0, bool const & a1, bool const & a2) -> bool { return o.read_bool(a0, a1, a2); }, "", pybind11::arg("node"), pybind11::arg("default_value"), pybind11::arg("inexistent_ok"));
		cl.def("read_bool", (bool (H2Core::XMLNode::*)(const class QString &, bool, bool, bool)) &H2Core::XMLNode::read_bool, "reads a boolean stored into a child node\n \n\n the name of the child node to read into\n \n\n the value returned if something goes wrong\n \n\n if set to false output a DEBUG log line if the node doesn't exists\n \n\n if set to false output a DEBUG log line if the child node is empty\n\nC++: H2Core::XMLNode::read_bool(const class QString &, bool, bool, bool) --> bool", pybind11::arg("node"), pybind11::arg("default_value"), pybind11::arg("inexistent_ok"), pybind11::arg("empty_ok"));
		cl.def("read_float", [](H2Core::XMLNode &o, const class QString & a0, float const & a1) -> float { return o.read_float(a0, a1); }, "", pybind11::arg("node"), pybind11::arg("default_value"));
		cl.def("read_float", [](H2Core::XMLNode &o, const class QString & a0, float const & a1, bool const & a2) -> float { return o.read_float(a0, a1, a2); }, "", pybind11::arg("node"), pybind11::arg("default_value"), pybind11::arg("inexistent_ok"));
		cl.def("read_float", (float (H2Core::XMLNode::*)(const class QString &, float, bool, bool)) &H2Core::XMLNode::read_float, "reads a float stored into a child node\n \n\n the name of the child node to read into\n \n\n the value returned if something goes wrong\n \n\n if set to false output a DEBUG log line if the node doesn't exists\n \n\n if set to false output a DEBUG log line if the child node is empty\n\nC++: H2Core::XMLNode::read_float(const class QString &, float, bool, bool) --> float", pybind11::arg("node"), pybind11::arg("default_value"), pybind11::arg("inexistent_ok"), pybind11::arg("empty_ok"));
		cl.def("read_float", [](H2Core::XMLNode &o, const class QString & a0, float const & a1, bool * a2) -> float { return o.read_float(a0, a1, a2); }, "", pybind11::arg("node"), pybind11::arg("default_value"), pybind11::arg("pFound"));
		cl.def("read_float", [](H2Core::XMLNode &o, const class QString & a0, float const & a1, bool * a2, bool const & a3) -> float { return o.read_float(a0, a1, a2, a3); }, "", pybind11::arg("node"), pybind11::arg("default_value"), pybind11::arg("pFound"), pybind11::arg("inexistent_ok"));
		cl.def("read_float", (float (H2Core::XMLNode::*)(const class QString &, float, bool *, bool, bool)) &H2Core::XMLNode::read_float, "C++: H2Core::XMLNode::read_float(const class QString &, float, bool *, bool, bool) --> float", pybind11::arg("node"), pybind11::arg("default_value"), pybind11::arg("pFound"), pybind11::arg("inexistent_ok"), pybind11::arg("empty_ok"));
		cl.def("read_string", [](H2Core::XMLNode &o, const class QString & a0, const class QString & a1) -> QString { return o.read_string(a0, a1); }, "", pybind11::arg("node"), pybind11::arg("default_value"));
		cl.def("read_string", [](H2Core::XMLNode &o, const class QString & a0, const class QString & a1, bool const & a2) -> QString { return o.read_string(a0, a1, a2); }, "", pybind11::arg("node"), pybind11::arg("default_value"), pybind11::arg("inexistent_ok"));
		cl.def("read_string", (class QString (H2Core::XMLNode::*)(const class QString &, const class QString &, bool, bool)) &H2Core::XMLNode::read_string, "reads a string stored into a child node\n \n\n the name of the child node to read into\n \n\n the value returned if something goes wrong\n \n\n if set to false output a DEBUG log line if the node doesn't exists\n \n\n if set to false output a DEBUG log line if the child node is empty\n\nC++: H2Core::XMLNode::read_string(const class QString &, const class QString &, bool, bool) --> class QString", pybind11::arg("node"), pybind11::arg("default_value"), pybind11::arg("inexistent_ok"), pybind11::arg("empty_ok"));
		cl.def("read_attribute", (class QString (H2Core::XMLNode::*)(const class QString &, const class QString &, bool, bool)) &H2Core::XMLNode::read_attribute, "reads an attribute from the node\n \n\n the name of the attribute to read\n \n\n the value returned if something goes wrong\n \n\n if set to false output a DEBUG log line if the attribute doesn't exists\n \n\n if set to false output a DEBUG log line if the attribute is empty\n\nC++: H2Core::XMLNode::read_attribute(const class QString &, const class QString &, bool, bool) --> class QString", pybind11::arg("attribute"), pybind11::arg("default_value"), pybind11::arg("inexistent_ok"), pybind11::arg("empty_ok"));
		cl.def("read_text", (class QString (H2Core::XMLNode::*)(bool)) &H2Core::XMLNode::read_text, "reads the text (content) from the node\n \n\n if set to false output a DEBUG log line if the node is empty\n\nC++: H2Core::XMLNode::read_text(bool) --> class QString", pybind11::arg("empty_ok"));
		cl.def("write_int", (void (H2Core::XMLNode::*)(const class QString &, const int)) &H2Core::XMLNode::write_int, "write an integer into a child node\n \n\n the name of the child node to create\n \n\n the value to write\n\nC++: H2Core::XMLNode::write_int(const class QString &, const int) --> void", pybind11::arg("node"), pybind11::arg("value"));
		cl.def("write_bool", (void (H2Core::XMLNode::*)(const class QString &, const bool)) &H2Core::XMLNode::write_bool, "write a boolean into a child node\n \n\n the name of the child node to create\n \n\n the value to write\n\nC++: H2Core::XMLNode::write_bool(const class QString &, const bool) --> void", pybind11::arg("node"), pybind11::arg("value"));
		cl.def("write_float", (void (H2Core::XMLNode::*)(const class QString &, const float)) &H2Core::XMLNode::write_float, "write a float into a child node\n \n\n the name of the child node to create\n \n\n the value to write\n\nC++: H2Core::XMLNode::write_float(const class QString &, const float) --> void", pybind11::arg("node"), pybind11::arg("value"));
		cl.def("write_string", (void (H2Core::XMLNode::*)(const class QString &, const class QString &)) &H2Core::XMLNode::write_string, "write a string into a child node\n \n\n the name of the child node to create\n \n\n the value to write\n\nC++: H2Core::XMLNode::write_string(const class QString &, const class QString &) --> void", pybind11::arg("node"), pybind11::arg("value"));
		cl.def("write_attribute", (void (H2Core::XMLNode::*)(const class QString &, const class QString &)) &H2Core::XMLNode::write_attribute, "write a string as an attribute of the node\n \n\n the name of the attribute to create\n \n\n the value to write in the attribute\n\nC++: H2Core::XMLNode::write_attribute(const class QString &, const class QString &) --> void", pybind11::arg("attribute"), pybind11::arg("value"));
		cl.def("assign", (class H2Core::XMLNode & (H2Core::XMLNode::*)(const class H2Core::XMLNode &)) &H2Core::XMLNode::operator=, "C++: H2Core::XMLNode::operator=(const class H2Core::XMLNode &) --> class H2Core::XMLNode &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B684_[H2Core::XMLDoc] ";
	{ // H2Core::XMLDoc file:core/Helpers/Xml.h line:130
		pybind11::class_<H2Core::XMLDoc, std::shared_ptr<H2Core::XMLDoc>, PyCallBack_H2Core_XMLDoc, H2Core::Object, QDomDocument> cl(M("H2Core"), "XMLDoc", "XMLDoc is a subclass of QDomDocument with read and write methods");
		cl.def( pybind11::init( [](){ return new H2Core::XMLDoc(); }, [](){ return new PyCallBack_H2Core_XMLDoc(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_XMLDoc const &o){ return new PyCallBack_H2Core_XMLDoc(o); } ) );
		cl.def( pybind11::init( [](H2Core::XMLDoc const &o){ return new H2Core::XMLDoc(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::XMLDoc::class_name, "C++: H2Core::XMLDoc::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("read", [](H2Core::XMLDoc &o, const class QString & a0) -> bool { return o.read(a0); }, "", pybind11::arg("filepath"));
		cl.def("read", (bool (H2Core::XMLDoc::*)(const class QString &, const class QString &)) &H2Core::XMLDoc::read, "read the content of an xml file\n \n\n the path to the file to read from\n \n\n the path to the XML Schema file\n\nC++: H2Core::XMLDoc::read(const class QString &, const class QString &) --> bool", pybind11::arg("filepath"), pybind11::arg("schemapath"));
		cl.def("write", (bool (H2Core::XMLDoc::*)(const class QString &)) &H2Core::XMLDoc::write, "write itself into a file\n \n\n the path to the file to write to\n\nC++: H2Core::XMLDoc::write(const class QString &) --> bool", pybind11::arg("filepath"));
		cl.def("set_root", [](H2Core::XMLDoc &o, const class QString & a0) -> H2Core::XMLNode { return o.set_root(a0); }, "", pybind11::arg("node_name"));
		cl.def("set_root", (class H2Core::XMLNode (H2Core::XMLDoc::*)(const class QString &, const class QString &)) &H2Core::XMLDoc::set_root, "create the xml header and root node\n \n\n the name of the rootnode to build\n \n\n the xml namespace prefix to add after XMLNS_BASE\n\nC++: H2Core::XMLDoc::set_root(const class QString &, const class QString &) --> class H2Core::XMLNode", pybind11::arg("node_name"), pybind11::arg("xmlns"));
		cl.def("assign", (class H2Core::XMLDoc & (H2Core::XMLDoc::*)(const class H2Core::XMLDoc &)) &H2Core::XMLDoc::operator=, "C++: H2Core::XMLDoc::operator=(const class H2Core::XMLDoc &) --> class H2Core::XMLDoc &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B685_[H2Core::Playlist] ";
	{ // H2Core::Playlist file:core/Basics/Playlist.h line:35
		pybind11::class_<H2Core::Playlist, std::shared_ptr<H2Core::Playlist>, PyCallBack_H2Core_Playlist, H2Core::Object> cl(M("H2Core"), "Playlist", "Drumkit info");
		cl.def( pybind11::init( [](PyCallBack_H2Core_Playlist const &o){ return new PyCallBack_H2Core_Playlist(o); } ) );
		cl.def( pybind11::init( [](H2Core::Playlist const &o){ return new H2Core::Playlist(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::Playlist::class_name, "C++: H2Core::Playlist::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def_static("create_instance", (void (*)()) &H2Core::Playlist::create_instance, "If #__instance equals 0, a new Playlist singleton\n will be created and stored in it.\n\n It is called in Hydrogen::audioEngine_init().\n\nC++: H2Core::Playlist::create_instance() --> void");
		cl.def_static("get_instance", (class H2Core::Playlist * (*)()) &H2Core::Playlist::get_instance, "Returns a pointer to the current Playlist singleton\n stored in #__instance.\n\nC++: H2Core::Playlist::get_instance() --> class H2Core::Playlist *", pybind11::return_value_policy::automatic);
		cl.def("activateSong", (void (H2Core::Playlist::*)(int)) &H2Core::Playlist::activateSong, "C++: H2Core::Playlist::activateSong(int) --> void", pybind11::arg("SongNumber"));
		cl.def("size", (int (H2Core::Playlist::*)() const) &H2Core::Playlist::size, "C++: H2Core::Playlist::size() const --> int");
		cl.def("get", (struct H2Core::Playlist::Entry * (H2Core::Playlist::*)(int)) &H2Core::Playlist::get, "C++: H2Core::Playlist::get(int) --> struct H2Core::Playlist::Entry *", pybind11::return_value_policy::automatic, pybind11::arg("idx"));
		cl.def("clear", (void (H2Core::Playlist::*)()) &H2Core::Playlist::clear, "C++: H2Core::Playlist::clear() --> void");
		cl.def("add", (void (H2Core::Playlist::*)(struct H2Core::Playlist::Entry *)) &H2Core::Playlist::add, "C++: H2Core::Playlist::add(struct H2Core::Playlist::Entry *) --> void", pybind11::arg("entry"));
		cl.def("setNextSongByNumber", (void (H2Core::Playlist::*)(int)) &H2Core::Playlist::setNextSongByNumber, "C++: H2Core::Playlist::setNextSongByNumber(int) --> void", pybind11::arg("SongNumber"));
		cl.def("getSelectedSongNr", (int (H2Core::Playlist::*)()) &H2Core::Playlist::getSelectedSongNr, "C++: H2Core::Playlist::getSelectedSongNr() --> int");
		cl.def("setSelectedSongNr", (void (H2Core::Playlist::*)(int)) &H2Core::Playlist::setSelectedSongNr, "C++: H2Core::Playlist::setSelectedSongNr(int) --> void", pybind11::arg("songNumber"));
		cl.def("getActiveSongNumber", (int (H2Core::Playlist::*)()) &H2Core::Playlist::getActiveSongNumber, "C++: H2Core::Playlist::getActiveSongNumber() --> int");
		cl.def("setActiveSongNumber", (void (H2Core::Playlist::*)(int)) &H2Core::Playlist::setActiveSongNumber, "C++: H2Core::Playlist::setActiveSongNumber(int) --> void", pybind11::arg("ActiveSongNumber"));
		cl.def("getSongFilenameByNumber", (bool (H2Core::Playlist::*)(int, class QString &)) &H2Core::Playlist::getSongFilenameByNumber, "C++: H2Core::Playlist::getSongFilenameByNumber(int, class QString &) --> bool", pybind11::arg("songNumber"), pybind11::arg("fileName"));
		cl.def("getFilename", (const class QString & (H2Core::Playlist::*)()) &H2Core::Playlist::getFilename, "C++: H2Core::Playlist::getFilename() --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("setFilename", (void (H2Core::Playlist::*)(const class QString &)) &H2Core::Playlist::setFilename, "C++: H2Core::Playlist::setFilename(const class QString &) --> void", pybind11::arg("filename"));
		cl.def("getIsModified", (bool (H2Core::Playlist::*)()) &H2Core::Playlist::getIsModified, "C++: H2Core::Playlist::getIsModified() --> bool");
		cl.def("setIsModified", (void (H2Core::Playlist::*)(bool)) &H2Core::Playlist::setIsModified, "C++: H2Core::Playlist::setIsModified(bool) --> void", pybind11::arg("IsModified"));
		cl.def_static("load", (class H2Core::Playlist * (*)(const class QString &, bool)) &H2Core::Playlist::load, "C++: H2Core::Playlist::load(const class QString &, bool) --> class H2Core::Playlist *", pybind11::return_value_policy::automatic, pybind11::arg("filename"), pybind11::arg("useRelativePaths"));
		cl.def_static("load_file", (class H2Core::Playlist * (*)(const class QString &, bool)) &H2Core::Playlist::load_file, "C++: H2Core::Playlist::load_file(const class QString &, bool) --> class H2Core::Playlist *", pybind11::return_value_policy::automatic, pybind11::arg("pl_path"), pybind11::arg("useRelativePaths"));
		cl.def("save_file", (bool (H2Core::Playlist::*)(const class QString &, const class QString &, bool, bool)) &H2Core::Playlist::save_file, "C++: H2Core::Playlist::save_file(const class QString &, const class QString &, bool, bool) --> bool", pybind11::arg("pl_path"), pybind11::arg("name"), pybind11::arg("overwrite"), pybind11::arg("useRelativePaths"));
		cl.def("toQString", [](H2Core::Playlist const &o, const class QString & a0) -> QString { return o.toQString(a0); }, "", pybind11::arg("sPrefix"));
		cl.def("toQString", (class QString (H2Core::Playlist::*)(const class QString &, bool) const) &H2Core::Playlist::toQString, "Formatted string version for debugging purposes.\n \n\n String prefix which will be added in front of\n every new line\n \n\n Instead of the whole content of all classes\n stored as members just a single unique identifier will be\n displayed without line breaks.\n\n \n String presentation of current object.\n\nC++: H2Core::Playlist::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
		cl.def("assign", (class H2Core::Playlist & (H2Core::Playlist::*)(const class H2Core::Playlist &)) &H2Core::Playlist::operator=, "C++: H2Core::Playlist::operator=(const class H2Core::Playlist &) --> class H2Core::Playlist &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // H2Core::Playlist::Entry file:core/Basics/Playlist.h line:41
			auto & enclosing_class = cl;
			pybind11::class_<H2Core::Playlist::Entry, std::shared_ptr<H2Core::Playlist::Entry>> cl(enclosing_class, "Entry", "");
			cl.def( pybind11::init( [](){ return new H2Core::Playlist::Entry(); } ) );
			cl.def( pybind11::init( [](H2Core::Playlist::Entry const &o){ return new H2Core::Playlist::Entry(o); } ) );
			cl.def_readwrite("filePath", &H2Core::Playlist::Entry::filePath);
			cl.def_readwrite("fileExists", &H2Core::Playlist::Entry::fileExists);
			cl.def_readwrite("scriptPath", &H2Core::Playlist::Entry::scriptPath);
			cl.def_readwrite("scriptEnabled", &H2Core::Playlist::Entry::scriptEnabled);
			cl.def("assign", (struct H2Core::Playlist::Entry & (H2Core::Playlist::Entry::*)(const struct H2Core::Playlist::Entry &)) &H2Core::Playlist::Entry::operator=, "C++: H2Core::Playlist::Entry::operator=(const struct H2Core::Playlist::Entry &) --> struct H2Core::Playlist::Entry &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		}

	}
}
