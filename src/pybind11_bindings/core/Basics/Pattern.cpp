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
#include <core/Basics/Pattern.h> // H2Core::Pattern
#include <core/Basics/PatternList.h> // H2Core::PatternList
#include <core/Helpers/Xml.h> // H2Core::XMLNode
#include <functional> // std::less
#include <iostream> // --trace
#include <iterator> // __gnu_cxx::__normal_iterator
#include <iterator> // std::reverse_iterator
#include <map> // std::_Rb_tree_const_iterator
#include <map> // std::_Rb_tree_iterator
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

// H2Core::Pattern file:core/Basics/Pattern.h line:42
struct PyCallBack_H2Core_Pattern : public H2Core::Pattern {
	using H2Core::Pattern::Pattern;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::Pattern *>(this), "toQString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return Pattern::toQString(a0, a1);
	}
};

// H2Core::PatternList file:core/Basics/PatternList.h line:39
struct PyCallBack_H2Core_PatternList : public H2Core::PatternList {
	using H2Core::PatternList::PatternList;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::PatternList *>(this), "toQString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QString>::value) {
				static pybind11::detail::override_caster_t<class QString> caster;
				return pybind11::detail::cast_ref<class QString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QString>(std::move(o));
		}
		return PatternList::toQString(a0, a1);
	}
};

void bind_core_Basics_Pattern(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B681_[H2Core::Pattern] ";
	{ // H2Core::Pattern file:core/Basics/Pattern.h line:42
		pybind11::class_<H2Core::Pattern, std::shared_ptr<H2Core::Pattern>, PyCallBack_H2Core_Pattern, H2Core::Object> cl(M("H2Core"), "Pattern", "Pattern class is a Note container");
		cl.def( pybind11::init( [](){ return new H2Core::Pattern(); }, [](){ return new PyCallBack_H2Core_Pattern(); } ), "doc");
		cl.def( pybind11::init( [](const class QString & a0){ return new H2Core::Pattern(a0); }, [](const class QString & a0){ return new PyCallBack_H2Core_Pattern(a0); } ), "doc");
		cl.def( pybind11::init( [](const class QString & a0, const class QString & a1){ return new H2Core::Pattern(a0, a1); }, [](const class QString & a0, const class QString & a1){ return new PyCallBack_H2Core_Pattern(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](const class QString & a0, const class QString & a1, const class QString & a2){ return new H2Core::Pattern(a0, a1, a2); }, [](const class QString & a0, const class QString & a1, const class QString & a2){ return new PyCallBack_H2Core_Pattern(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init( [](const class QString & a0, const class QString & a1, const class QString & a2, int const & a3){ return new H2Core::Pattern(a0, a1, a2, a3); }, [](const class QString & a0, const class QString & a1, const class QString & a2, int const & a3){ return new PyCallBack_H2Core_Pattern(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init<const class QString &, const class QString &, const class QString &, int, int>(), pybind11::arg("name"), pybind11::arg("info"), pybind11::arg("category"), pybind11::arg("length"), pybind11::arg("denominator") );

		cl.def( pybind11::init<class H2Core::Pattern *>(), pybind11::arg("other") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_Pattern const &o){ return new PyCallBack_H2Core_Pattern(o); } ) );
		cl.def( pybind11::init( [](H2Core::Pattern const &o){ return new H2Core::Pattern(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::Pattern::class_name, "C++: H2Core::Pattern::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def_static("load_file", (class H2Core::Pattern * (*)(const class QString &, class H2Core::InstrumentList *)) &H2Core::Pattern::load_file, "load a pattern from a file\n \n\n the path to the file to load the pattern from\n \n\n the current instrument list to search instrument into\n\nC++: H2Core::Pattern::load_file(const class QString &, class H2Core::InstrumentList *) --> class H2Core::Pattern *", pybind11::return_value_policy::automatic, pybind11::arg("pattern_path"), pybind11::arg("instruments"));
		cl.def("save_file", [](H2Core::Pattern const &o, const class QString & a0, const class QString & a1, const class QString & a2, const class QString & a3) -> bool { return o.save_file(a0, a1, a2, a3); }, "", pybind11::arg("drumkit_name"), pybind11::arg("author"), pybind11::arg("license"), pybind11::arg("pattern_path"));
		cl.def("save_file", (bool (H2Core::Pattern::*)(const class QString &, const class QString &, const class QString &, const class QString &, bool) const) &H2Core::Pattern::save_file, "save a pattern into an xml file\n \n\n the name of the drumkit it is supposed to play with\n \n\n the name of the author\n \n\n the license that applies to it\n \n\n the path to save the pattern into\n \n\n allows to write over existing pattern file\n \n\n true on success\n\nC++: H2Core::Pattern::save_file(const class QString &, const class QString &, const class QString &, const class QString &, bool) const --> bool", pybind11::arg("drumkit_name"), pybind11::arg("author"), pybind11::arg("license"), pybind11::arg("pattern_path"), pybind11::arg("overwrite"));
		cl.def("set_name", (void (H2Core::Pattern::*)(const class QString &)) &H2Core::Pattern::set_name, "C++: H2Core::Pattern::set_name(const class QString &) --> void", pybind11::arg("name"));
		cl.def("get_name", (const class QString & (H2Core::Pattern::*)() const) &H2Core::Pattern::get_name, "C++: H2Core::Pattern::get_name() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("set_category", (void (H2Core::Pattern::*)(const class QString &)) &H2Core::Pattern::set_category, "C++: H2Core::Pattern::set_category(const class QString &) --> void", pybind11::arg("category"));
		cl.def("set_info", (void (H2Core::Pattern::*)(const class QString &)) &H2Core::Pattern::set_info, "C++: H2Core::Pattern::set_info(const class QString &) --> void", pybind11::arg("info"));
		cl.def("get_info", (const class QString & (H2Core::Pattern::*)() const) &H2Core::Pattern::get_info, "C++: H2Core::Pattern::get_info() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("get_category", (const class QString & (H2Core::Pattern::*)() const) &H2Core::Pattern::get_category, "C++: H2Core::Pattern::get_category() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("set_length", (void (H2Core::Pattern::*)(int)) &H2Core::Pattern::set_length, "C++: H2Core::Pattern::set_length(int) --> void", pybind11::arg("length"));
		cl.def("get_length", (int (H2Core::Pattern::*)() const) &H2Core::Pattern::get_length, "C++: H2Core::Pattern::get_length() const --> int");
		cl.def("set_denominator", (void (H2Core::Pattern::*)(int)) &H2Core::Pattern::set_denominator, "C++: H2Core::Pattern::set_denominator(int) --> void", pybind11::arg("denominator"));
		cl.def("get_denominator", (int (H2Core::Pattern::*)() const) &H2Core::Pattern::get_denominator, "C++: H2Core::Pattern::get_denominator() const --> int");
		cl.def("insert_note", (void (H2Core::Pattern::*)(class H2Core::Note *)) &H2Core::Pattern::insert_note, "insert a new note within __notes\n \n\n the note to be inserted\n\nC++: H2Core::Pattern::insert_note(class H2Core::Note *) --> void", pybind11::arg("note"));
		cl.def("remove_note", (void (H2Core::Pattern::*)(class H2Core::Note *)) &H2Core::Pattern::remove_note, "removes a given note from __notes, it's not deleted\n \n\n the note to be removed\n\nC++: H2Core::Pattern::remove_note(class H2Core::Note *) --> void", pybind11::arg("note"));
		cl.def("set_to_old", (void (H2Core::Pattern::*)()) &H2Core::Pattern::set_to_old, "mark all notes as old\n\nC++: H2Core::Pattern::set_to_old() --> void");
		cl.def("virtual_patterns_empty", (bool (H2Core::Pattern::*)() const) &H2Core::Pattern::virtual_patterns_empty, "C++: H2Core::Pattern::virtual_patterns_empty() const --> bool");
		cl.def("virtual_patterns_clear", (void (H2Core::Pattern::*)()) &H2Core::Pattern::virtual_patterns_clear, "C++: H2Core::Pattern::virtual_patterns_clear() --> void");
		cl.def("virtual_patterns_add", (void (H2Core::Pattern::*)(class H2Core::Pattern *)) &H2Core::Pattern::virtual_patterns_add, "add a pattern to __virtual_patterns\n \n\n the pattern to add\n\nC++: H2Core::Pattern::virtual_patterns_add(class H2Core::Pattern *) --> void", pybind11::arg("pattern"));
		cl.def("virtual_patterns_del", (void (H2Core::Pattern::*)(class H2Core::Pattern *)) &H2Core::Pattern::virtual_patterns_del, "remove a pattern from virtual_pattern set, flattened virtual patterns have to be rebuilt\n\nC++: H2Core::Pattern::virtual_patterns_del(class H2Core::Pattern *) --> void", pybind11::arg("pattern"));
		cl.def("flattened_virtual_patterns_clear", (void (H2Core::Pattern::*)()) &H2Core::Pattern::flattened_virtual_patterns_clear, "C++: H2Core::Pattern::flattened_virtual_patterns_clear() --> void");
		cl.def("flattened_virtual_patterns_compute", (void (H2Core::Pattern::*)()) &H2Core::Pattern::flattened_virtual_patterns_compute, "compute virtual_pattern_transitive_closure_set based on virtual_pattern_transitive_closure_set\n virtual_pattern_transitive_closure_set must have been cleared before which is the case is called\n from PatternList::compute_flattened_virtual_patterns\n\nC++: H2Core::Pattern::flattened_virtual_patterns_compute() --> void");
		cl.def("extand_with_flattened_virtual_patterns", (void (H2Core::Pattern::*)(class H2Core::PatternList *)) &H2Core::Pattern::extand_with_flattened_virtual_patterns, "add content of __flatteened_virtual_patterns into patterns\n \n\n the pattern list to feed\n\nC++: H2Core::Pattern::extand_with_flattened_virtual_patterns(class H2Core::PatternList *) --> void", pybind11::arg("patterns"));
		cl.def("toQString", [](H2Core::Pattern const &o, const class QString & a0) -> QString { return o.toQString(a0); }, "", pybind11::arg("sPrefix"));
		cl.def("toQString", (class QString (H2Core::Pattern::*)(const class QString &, bool) const) &H2Core::Pattern::toQString, "Formatted string version for debugging purposes.\n \n\n String prefix which will be added in front of\n every new line\n \n\n Instead of the whole content of all classes\n stored as members just a single unique identifier will be\n displayed without line breaks.\n\n \n String presentation of current object.\n\nC++: H2Core::Pattern::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
		cl.def("assign", (class H2Core::Pattern & (H2Core::Pattern::*)(const class H2Core::Pattern &)) &H2Core::Pattern::operator=, "C++: H2Core::Pattern::operator=(const class H2Core::Pattern &) --> class H2Core::Pattern &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B682_[H2Core::PatternList] ";
	{ // H2Core::PatternList file:core/Basics/PatternList.h line:39
		pybind11::class_<H2Core::PatternList, std::shared_ptr<H2Core::PatternList>, PyCallBack_H2Core_PatternList, H2Core::Object, H2Core::AudioEngineLocking> cl(M("H2Core"), "PatternList", "PatternList is a collection of patterns");
		cl.def( pybind11::init( [](){ return new H2Core::PatternList(); }, [](){ return new PyCallBack_H2Core_PatternList(); } ) );
		cl.def( pybind11::init<class H2Core::PatternList *>(), pybind11::arg("other") );

		cl.def( pybind11::init( [](PyCallBack_H2Core_PatternList const &o){ return new PyCallBack_H2Core_PatternList(o); } ) );
		cl.def( pybind11::init( [](H2Core::PatternList const &o){ return new H2Core::PatternList(o); } ) );
		cl.def_static("class_name", (const char * (*)()) &H2Core::PatternList::class_name, "C++: H2Core::PatternList::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("size", (int (H2Core::PatternList::*)() const) &H2Core::PatternList::size, "returns the numbers of patterns \n\nC++: H2Core::PatternList::size() const --> int");
		cl.def("__getitem__", (class H2Core::Pattern * (H2Core::PatternList::*)(int)) &H2Core::PatternList::operator[], "get a pattern from  the list\n \n\n the index to get the pattern from\n\nC++: H2Core::PatternList::operator[](int) --> class H2Core::Pattern *", pybind11::return_value_policy::automatic, pybind11::arg("idx"));
		cl.def("add", (void (H2Core::PatternList::*)(class H2Core::Pattern *)) &H2Core::PatternList::add, "add a pattern to the list\n \n\n a pointer to the pattern to add\n\nC++: H2Core::PatternList::add(class H2Core::Pattern *) --> void", pybind11::arg("pattern"));
		cl.def("insert", (void (H2Core::PatternList::*)(int, class H2Core::Pattern *)) &H2Core::PatternList::insert, "insert a pattern into the list\n \n\n the index to insert the pattern at\n \n\n a pointer to the pattern to add\n\nC++: H2Core::PatternList::insert(int, class H2Core::Pattern *) --> void", pybind11::arg("idx"), pybind11::arg("pattern"));
		cl.def("get", (class H2Core::Pattern * (H2Core::PatternList::*)(int)) &H2Core::PatternList::get, "get a pattern from  the list\n \n\n the index to get the pattern from\n\nC++: H2Core::PatternList::get(int) --> class H2Core::Pattern *", pybind11::return_value_policy::automatic, pybind11::arg("idx"));
		cl.def("del", (class H2Core::Pattern * (H2Core::PatternList::*)(int)) &H2Core::PatternList::del, "remove the pattern at a given index, does not delete it\n \n\n the index\n \n\n a pointer to the removed pattern\n\nC++: H2Core::PatternList::del(int) --> class H2Core::Pattern *", pybind11::return_value_policy::automatic, pybind11::arg("idx"));
		cl.def("del", (class H2Core::Pattern * (H2Core::PatternList::*)(class H2Core::Pattern *)) &H2Core::PatternList::del, "remove a pattern from the list, does not delete it\n \n\n the pattern to be removed\n \n\n a pointer to the removed pattern, 0 if not found\n\nC++: H2Core::PatternList::del(class H2Core::Pattern *) --> class H2Core::Pattern *", pybind11::return_value_policy::automatic, pybind11::arg("pattern"));
		cl.def("index", (int (H2Core::PatternList::*)(const class H2Core::Pattern *)) &H2Core::PatternList::index, "get the index of the pattern within the patterns\n \n\n a pointer to the pattern to find\n \n\n -1 if not found\n\nC++: H2Core::PatternList::index(const class H2Core::Pattern *) --> int", pybind11::arg("pattern"));
		cl.def("replace", (class H2Core::Pattern * (H2Core::PatternList::*)(int, class H2Core::Pattern *)) &H2Core::PatternList::replace, "replace the pattern at a given index with a new one\n \n\n the index\n \n\n the new pattern to insert\n \n\n a pointer to the removed pattern, 0 if index out of bounds\n\nC++: H2Core::PatternList::replace(int, class H2Core::Pattern *) --> class H2Core::Pattern *", pybind11::return_value_policy::automatic, pybind11::arg("idx"), pybind11::arg("pattern"));
		cl.def("clear", (void (H2Core::PatternList::*)()) &H2Core::PatternList::clear, "empty the pattern list\n\nC++: H2Core::PatternList::clear() --> void");
		cl.def("set_to_old", (void (H2Core::PatternList::*)()) &H2Core::PatternList::set_to_old, "mark all patterns as old\n\nC++: H2Core::PatternList::set_to_old() --> void");
		cl.def("find", (class H2Core::Pattern * (H2Core::PatternList::*)(const class QString &)) &H2Core::PatternList::find, "find a pattern within the patterns\n \n\n the name of the pattern to find\n \n\n 0 if not found\n\nC++: H2Core::PatternList::find(const class QString &) --> class H2Core::Pattern *", pybind11::return_value_policy::automatic, pybind11::arg("name"));
		cl.def("swap", (void (H2Core::PatternList::*)(int, int)) &H2Core::PatternList::swap, "swap the patterns of two different indexes\n \n\n the first index\n \n\n the second index\n\nC++: H2Core::PatternList::swap(int, int) --> void", pybind11::arg("idx_a"), pybind11::arg("idx_b"));
		cl.def("move", (void (H2Core::PatternList::*)(int, int)) &H2Core::PatternList::move, "move a pattern from a position to another\n \n\n the start index\n \n\n the finish index\n\nC++: H2Core::PatternList::move(int, int) --> void", pybind11::arg("idx_a"), pybind11::arg("idx_b"));
		cl.def("flattened_virtual_patterns_compute", (void (H2Core::PatternList::*)()) &H2Core::PatternList::flattened_virtual_patterns_compute, "call compute_flattened_virtual_patterns on each pattern\n\nC++: H2Core::PatternList::flattened_virtual_patterns_compute() --> void");
		cl.def("virtual_pattern_del", (void (H2Core::PatternList::*)(class H2Core::Pattern *)) &H2Core::PatternList::virtual_pattern_del, "call del_virtual_pattern on each pattern\n \n\n the pattern to remove where it's found\n\nC++: H2Core::PatternList::virtual_pattern_del(class H2Core::Pattern *) --> void", pybind11::arg("pattern"));
		cl.def("check_name", [](H2Core::PatternList &o, class QString const & a0) -> bool { return o.check_name(a0); }, "", pybind11::arg("patternName"));
		cl.def("check_name", (bool (H2Core::PatternList::*)(class QString, class H2Core::Pattern *)) &H2Core::PatternList::check_name, "check if a pattern with name patternName already exists in this list\n \n\n name of a pattern to check\n \n\n optional pattern in the list to ignore\n\nC++: H2Core::PatternList::check_name(class QString, class H2Core::Pattern *) --> bool", pybind11::arg("patternName"), pybind11::arg("ignore"));
		cl.def("find_unused_pattern_name", [](H2Core::PatternList &o, class QString const & a0) -> QString { return o.find_unused_pattern_name(a0); }, "", pybind11::arg("sourceName"));
		cl.def("find_unused_pattern_name", (class QString (H2Core::PatternList::*)(class QString, class H2Core::Pattern *)) &H2Core::PatternList::find_unused_pattern_name, "find unused patternName\n \n\n base name to start with\n \n\n optional pattern in the list to ignore\n\nC++: H2Core::PatternList::find_unused_pattern_name(class QString, class H2Core::Pattern *) --> class QString", pybind11::arg("sourceName"), pybind11::arg("ignore"));
		cl.def("longest_pattern_length", (int (H2Core::PatternList::*)()) &H2Core::PatternList::longest_pattern_length, "Get the length of the longest pattern in the list\n \n\n pattern length in ticks, -1 if list is empty\n\nC++: H2Core::PatternList::longest_pattern_length() --> int");
		cl.def("toQString", [](H2Core::PatternList const &o, const class QString & a0) -> QString { return o.toQString(a0); }, "", pybind11::arg("sPrefix"));
		cl.def("toQString", (class QString (H2Core::PatternList::*)(const class QString &, bool) const) &H2Core::PatternList::toQString, "Formatted string version for debugging purposes.\n \n\n String prefix which will be added in front of\n every new line\n \n\n Instead of the whole content of all classes\n stored as members just a single unique identifier will be\n displayed without line breaks.\n\n \n String presentation of current object.\n\nC++: H2Core::PatternList::toQString(const class QString &, bool) const --> class QString", pybind11::arg("sPrefix"), pybind11::arg("bShort"));
		cl.def("assign", (class H2Core::PatternList & (H2Core::PatternList::*)(const class H2Core::PatternList &)) &H2Core::PatternList::operator=, "C++: H2Core::PatternList::operator=(const class H2Core::PatternList &) --> class H2Core::PatternList &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
