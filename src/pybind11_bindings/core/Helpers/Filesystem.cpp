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
#include <core/Helpers/Filesystem.h> // 
#include <core/Helpers/Filesystem.h> // H2Core::Filesystem
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

// H2Core::Filesystem file:core/Helpers/Filesystem.h line:14
struct PyCallBack_H2Core_Filesystem : public H2Core::Filesystem {
	using H2Core::Filesystem::Filesystem;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::Filesystem *>(this), "toQString");
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

void bind_core_Helpers_Filesystem(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B649_[H2Core::Filesystem] ";
	{ // H2Core::Filesystem file:core/Helpers/Filesystem.h line:14
		pybind11::class_<H2Core::Filesystem, std::shared_ptr<H2Core::Filesystem>, PyCallBack_H2Core_Filesystem, H2Core::Object> cl(M("H2Core"), "Filesystem", "Filesystem is a thin layer over QDir, QFile and QFileInfo");
		cl.def( pybind11::init( [](){ return new H2Core::Filesystem(); }, [](){ return new PyCallBack_H2Core_Filesystem(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_Filesystem const &o){ return new PyCallBack_H2Core_Filesystem(o); } ) );
		cl.def( pybind11::init( [](H2Core::Filesystem const &o){ return new H2Core::Filesystem(o); } ) );

		pybind11::enum_<H2Core::Filesystem::file_perms>(cl, "file_perms", pybind11::arithmetic(), "flags available for check_permissions() ")
			.value("is_dir", H2Core::Filesystem::is_dir)
			.value("is_file", H2Core::Filesystem::is_file)
			.value("is_readable", H2Core::Filesystem::is_readable)
			.value("is_writable", H2Core::Filesystem::is_writable)
			.value("is_executable", H2Core::Filesystem::is_executable)
			.export_values();


		pybind11::enum_<H2Core::Filesystem::Lookup>(cl, "Lookup", "Whenever a drumkit is loaded by name a collision between a\n user and a system drumkit carrying the same name can\n occur.")
			.value("stacked", H2Core::Filesystem::Lookup::stacked)
			.value("user", H2Core::Filesystem::Lookup::user)
			.value("system", H2Core::Filesystem::Lookup::system);

		cl.def_static("class_name", (const char * (*)()) &H2Core::Filesystem::class_name, "C++: H2Core::Filesystem::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def_static("bootstrap", [](class H2Core::Logger * a0) -> bool { return H2Core::Filesystem::bootstrap(a0); }, "", pybind11::arg("logger"));
		cl.def_static("bootstrap", (bool (*)(class H2Core::Logger *, const class QString &)) &H2Core::Filesystem::bootstrap, "check user and system filesystem usability\n \n\n is a pointer to the logger instance which will be used\n \n\n an alternate system data path\n\nC++: H2Core::Filesystem::bootstrap(class H2Core::Logger *, const class QString &) --> bool", pybind11::arg("logger"), pybind11::arg("sys_path"));
		cl.def_static("sys_data_path", (class QString (*)()) &H2Core::Filesystem::sys_data_path, "returns system data path \n\nC++: H2Core::Filesystem::sys_data_path() --> class QString");
		cl.def_static("usr_data_path", (class QString (*)()) &H2Core::Filesystem::usr_data_path, "returns user data path \n\nC++: H2Core::Filesystem::usr_data_path() --> class QString");
		cl.def_static("sys_config_path", (class QString (*)()) &H2Core::Filesystem::sys_config_path, "returns system config path \n\nC++: H2Core::Filesystem::sys_config_path() --> class QString");
		cl.def_static("usr_config_path", (class QString (*)()) &H2Core::Filesystem::usr_config_path, "returns user config path \n\nC++: H2Core::Filesystem::usr_config_path() --> class QString");
		cl.def_static("empty_sample_path", (class QString (*)()) &H2Core::Filesystem::empty_sample_path, "returns system empty sample file path \n\nC++: H2Core::Filesystem::empty_sample_path() --> class QString");
		cl.def_static("empty_song_path", (class QString (*)()) &H2Core::Filesystem::empty_song_path, "returns system empty song file path \n\nC++: H2Core::Filesystem::empty_song_path() --> class QString");
		cl.def_static("untitled_song_file_name", (class QString (*)()) &H2Core::Filesystem::untitled_song_file_name, "returns untitled song file name \n\nC++: H2Core::Filesystem::untitled_song_file_name() --> class QString");
		cl.def_static("click_file_path", (class QString (*)()) &H2Core::Filesystem::click_file_path, "Returns a string containing the path to the\n		    _click.wav_ file used in the metronome. \n\n It is a concatenation of #__sys_data_path and\n #CLICK_SAMPLE.\n\nC++: H2Core::Filesystem::click_file_path() --> class QString");
		cl.def_static("usr_click_file_path", (class QString (*)()) &H2Core::Filesystem::usr_click_file_path, "returns click file path from user directory if exists, otherwise from system \n\nC++: H2Core::Filesystem::usr_click_file_path() --> class QString");
		cl.def_static("drumkit_xsd_path", (class QString (*)()) &H2Core::Filesystem::drumkit_xsd_path, "returns the path to the drumkit XSD (xml schema definition) file \n\nC++: H2Core::Filesystem::drumkit_xsd_path() --> class QString");
		cl.def_static("pattern_xsd_path", (class QString (*)()) &H2Core::Filesystem::pattern_xsd_path, "returns the path to the pattern XSD (xml schema definition) file \n\nC++: H2Core::Filesystem::pattern_xsd_path() --> class QString");
		cl.def_static("playlist_xsd_path", (class QString (*)()) &H2Core::Filesystem::playlist_xsd_path, "returns the path to the playlist pattern XSD (xml schema definition) file \n\nC++: H2Core::Filesystem::playlist_xsd_path() --> class QString");
		cl.def_static("log_file_path", (class QString (*)()) &H2Core::Filesystem::log_file_path, "returns the full path (including filename) of the logfile \n\nC++: H2Core::Filesystem::log_file_path() --> class QString");
		cl.def_static("img_dir", (class QString (*)()) &H2Core::Filesystem::img_dir, "returns gui image path \n\nC++: H2Core::Filesystem::img_dir() --> class QString");
		cl.def_static("doc_dir", (class QString (*)()) &H2Core::Filesystem::doc_dir, "returns documentation path \n\nC++: H2Core::Filesystem::doc_dir() --> class QString");
		cl.def_static("i18n_dir", (class QString (*)()) &H2Core::Filesystem::i18n_dir, "returns internationalization path \n\nC++: H2Core::Filesystem::i18n_dir() --> class QString");
		cl.def_static("scripts_dir", (class QString (*)()) &H2Core::Filesystem::scripts_dir, "returns user scripts path \n\nC++: H2Core::Filesystem::scripts_dir() --> class QString");
		cl.def_static("songs_dir", (class QString (*)()) &H2Core::Filesystem::songs_dir, "returns user songs path \n\nC++: H2Core::Filesystem::songs_dir() --> class QString");
		cl.def_static("song_path", (class QString (*)(const class QString &)) &H2Core::Filesystem::song_path, "returns user song path, add file extension \n\nC++: H2Core::Filesystem::song_path(const class QString &) --> class QString", pybind11::arg("sg_name"));
		cl.def_static("patterns_dir", (class QString (*)()) &H2Core::Filesystem::patterns_dir, "returns user patterns path \n\nC++: H2Core::Filesystem::patterns_dir() --> class QString");
		cl.def_static("patterns_dir", (class QString (*)(const class QString &)) &H2Core::Filesystem::patterns_dir, "returns user patterns path for a specific drumkit \n\nC++: H2Core::Filesystem::patterns_dir(const class QString &) --> class QString", pybind11::arg("dk_name"));
		cl.def_static("pattern_path", (class QString (*)(const class QString &, const class QString &)) &H2Core::Filesystem::pattern_path, "returns user patterns path, add file extension\n\nC++: H2Core::Filesystem::pattern_path(const class QString &, const class QString &) --> class QString", pybind11::arg("dk_name"), pybind11::arg("p_name"));
		cl.def_static("plugins_dir", (class QString (*)()) &H2Core::Filesystem::plugins_dir, "returns user plugins path \n\nC++: H2Core::Filesystem::plugins_dir() --> class QString");
		cl.def_static("sys_drumkits_dir", (class QString (*)()) &H2Core::Filesystem::sys_drumkits_dir, "returns system drumkits path \n\nC++: H2Core::Filesystem::sys_drumkits_dir() --> class QString");
		cl.def_static("usr_drumkits_dir", (class QString (*)()) &H2Core::Filesystem::usr_drumkits_dir, "returns user drumkits path \n\nC++: H2Core::Filesystem::usr_drumkits_dir() --> class QString");
		cl.def_static("playlists_dir", (class QString (*)()) &H2Core::Filesystem::playlists_dir, "returns user playlist path \n\nC++: H2Core::Filesystem::playlists_dir() --> class QString");
		cl.def_static("playlist_path", (class QString (*)(const class QString &)) &H2Core::Filesystem::playlist_path, "returns user playlist path, add file extension \n\nC++: H2Core::Filesystem::playlist_path(const class QString &) --> class QString", pybind11::arg("pl_name"));
		cl.def_static("untitled_playlist_file_name", (class QString (*)()) &H2Core::Filesystem::untitled_playlist_file_name, "returns untitled playlist file name \n\nC++: H2Core::Filesystem::untitled_playlist_file_name() --> class QString");
		cl.def_static("cache_dir", (class QString (*)()) &H2Core::Filesystem::cache_dir, "returns user cache path \n\nC++: H2Core::Filesystem::cache_dir() --> class QString");
		cl.def_static("repositories_cache_dir", (class QString (*)()) &H2Core::Filesystem::repositories_cache_dir, "returns user repository cache path \n\nC++: H2Core::Filesystem::repositories_cache_dir() --> class QString");
		cl.def_static("demos_dir", (class QString (*)()) &H2Core::Filesystem::demos_dir, "returns system demos path \n\nC++: H2Core::Filesystem::demos_dir() --> class QString");
		cl.def_static("xsd_dir", (class QString (*)()) &H2Core::Filesystem::xsd_dir, "returns system xsd path \n\nC++: H2Core::Filesystem::xsd_dir() --> class QString");
		cl.def_static("tmp_dir", (class QString (*)()) &H2Core::Filesystem::tmp_dir, "returns temp path \n\nC++: H2Core::Filesystem::tmp_dir() --> class QString");
		cl.def_static("tmp_file_path", (class QString (*)(const class QString &)) &H2Core::Filesystem::tmp_file_path, "touch a temporary file under tmp_dir() and return it's path.\n if base has a suffix it will be preserved, spaces will be replaced by underscores.\n \n\n part of the path\n\nC++: H2Core::Filesystem::tmp_file_path(const class QString &) --> class QString", pybind11::arg("base"));
		cl.def_static("prepare_sample_path", (class QString (*)(const class QString &)) &H2Core::Filesystem::prepare_sample_path, "Returns the basename if the given path is under an existing user or system drumkit path, otherwise the given fname \n\nC++: H2Core::Filesystem::prepare_sample_path(const class QString &) --> class QString", pybind11::arg("fname"));
		cl.def_static("file_is_under_drumkit", (bool (*)(const class QString &)) &H2Core::Filesystem::file_is_under_drumkit, "Checks if the given filepath is under an existing user or system drumkit path, not the existence of the file \n\nC++: H2Core::Filesystem::file_is_under_drumkit(const class QString &) --> bool", pybind11::arg("fname"));
		cl.def_static("get_basename_idx_under_drumkit", (int (*)(const class QString &)) &H2Core::Filesystem::get_basename_idx_under_drumkit, "Returns the index of the basename if the given path is under an existing user or system drumkit path, otherwise -1 \n\nC++: H2Core::Filesystem::get_basename_idx_under_drumkit(const class QString &) --> int", pybind11::arg("fname"));
		cl.def_static("drumkit_exists", (bool (*)(const class QString &)) &H2Core::Filesystem::drumkit_exists, "returns true if the drumkit exists within usable system or user drumkits\n \n\n the drumkit name\n\nC++: H2Core::Filesystem::drumkit_exists(const class QString &) --> bool", pybind11::arg("dk_name"));
		cl.def_static("drumkit_usr_path", (class QString (*)(const class QString &)) &H2Core::Filesystem::drumkit_usr_path, "returns path for a drumkit within user drumkit path\n \n\n the drumkit name\n\nC++: H2Core::Filesystem::drumkit_usr_path(const class QString &) --> class QString", pybind11::arg("dk_name"));
		cl.def_static("drumkit_path_search", [](const class QString & a0) -> QString { return H2Core::Filesystem::drumkit_path_search(a0); }, "", pybind11::arg("dk_name"));
		cl.def_static("drumkit_path_search", [](const class QString & a0, enum H2Core::Filesystem::Lookup const & a1) -> QString { return H2Core::Filesystem::drumkit_path_search(a0, a1); }, "", pybind11::arg("dk_name"), pybind11::arg("lookup"));
		cl.def_static("drumkit_path_search", (class QString (*)(const class QString &, enum H2Core::Filesystem::Lookup, bool)) &H2Core::Filesystem::drumkit_path_search, "Returns the path to a H2Core::Drumkit folder.\n\n The search will first be performed within user-level\n drumkits system drumkits using usr_drumkit_list() and\n usr_drumkits_dir() and later, in case the H2Core::Drumkit\n could not be found, within the system-level drumkits using\n sys_drumkit_list() and sys_drumkits_dir().\n\n When under session management (see\n NsmClient::m_bUnderSessionManagement) the function will\n first look for a \"drumkit\" symlink or folder within\n NsmClient::m_sSessionFolderPath. If it either is not a\n valid H2Core::Drumkit or the not the one corresponding to\n  the user- and system-level drumkits will be\n searched instead.\n\n \n Name of the H2Core::Drumkit. In the\n   user-level and system-level lookup it has to correspond\n   to the name of the folder holding the samples and the\n   #DRUMKIT_XML file. For the usage of a local\n   H2Core::Drumkit under session management it has to match\n   the second-level \"name\" node within the\n   #DRUMKIT_XML file.\n \n\n Where to search (system/user folder or both)\n for the drumkit.\n \n\n whether the function should trigger log\n   messages. If set to true, the calling function is\n   expected to handle the log messages instead.\n\n \n Full path to the folder containing the samples of\n   the H2Core::Drumkit corresponding to \n		 \n\nC++: H2Core::Filesystem::drumkit_path_search(const class QString &, enum H2Core::Filesystem::Lookup, bool) --> class QString", pybind11::arg("dk_name"), pybind11::arg("lookup"), pybind11::arg("bSilent"));
		cl.def_static("drumkit_dir_search", (class QString (*)(const class QString &, enum H2Core::Filesystem::Lookup)) &H2Core::Filesystem::drumkit_dir_search, "returns the directory holding the named drumkit searching within user then system drumkits\n \n\n the drumkit name\n \n\n Where to search (system/user folder or both)\n for the drumkit. \n\nC++: H2Core::Filesystem::drumkit_dir_search(const class QString &, enum H2Core::Filesystem::Lookup) --> class QString", pybind11::arg("dk_name"), pybind11::arg("lookup"));
		cl.def_static("drumkit_valid", (bool (*)(const class QString &)) &H2Core::Filesystem::drumkit_valid, "returns true if the path contains a usable drumkit\n \n\n the root drumkit location\n\nC++: H2Core::Filesystem::drumkit_valid(const class QString &) --> bool", pybind11::arg("dk_path"));
		cl.def_static("drumkit_file", (class QString (*)(const class QString &)) &H2Core::Filesystem::drumkit_file, "returns the path to the xml file within a supposed drumkit path\n \n\n the path to the drumkit\n\nC++: H2Core::Filesystem::drumkit_file(const class QString &) --> class QString", pybind11::arg("dk_path"));
		cl.def_static("song_exists", (bool (*)(const class QString &)) &H2Core::Filesystem::song_exists, "returns true if the song file exists\n \n\n the song name\n\nC++: H2Core::Filesystem::song_exists(const class QString &) --> bool", pybind11::arg("sg_name"));
		cl.def_static("info", (void (*)()) &H2Core::Filesystem::info, "send current settings information to logger with INFO severity \n\nC++: H2Core::Filesystem::info() --> void");
		cl.def_static("file_exists", [](const class QString & a0) -> bool { return H2Core::Filesystem::file_exists(a0); }, "", pybind11::arg("path"));
		cl.def_static("file_exists", (bool (*)(const class QString &, bool)) &H2Core::Filesystem::file_exists, "returns true if the given path is an existing regular file\n \n\n the path to the file to check\n \n\n output not messages if set to true\n\nC++: H2Core::Filesystem::file_exists(const class QString &, bool) --> bool", pybind11::arg("path"), pybind11::arg("silent"));
		cl.def_static("file_readable", [](const class QString & a0) -> bool { return H2Core::Filesystem::file_readable(a0); }, "", pybind11::arg("path"));
		cl.def_static("file_readable", (bool (*)(const class QString &, bool)) &H2Core::Filesystem::file_readable, "returns true if the given path is an existing readable regular file\n \n\n the path to the file to check\n \n\n output not messages if set to true\n\nC++: H2Core::Filesystem::file_readable(const class QString &, bool) --> bool", pybind11::arg("path"), pybind11::arg("silent"));
		cl.def_static("file_writable", [](const class QString & a0) -> bool { return H2Core::Filesystem::file_writable(a0); }, "", pybind11::arg("path"));
		cl.def_static("file_writable", (bool (*)(const class QString &, bool)) &H2Core::Filesystem::file_writable, "returns true if the given path is a possibly writable file (may exist or not)\n \n\n the path to the file to check\n \n\n output not messages if set to true\n\nC++: H2Core::Filesystem::file_writable(const class QString &, bool) --> bool", pybind11::arg("path"), pybind11::arg("silent"));
		cl.def_static("file_executable", [](const class QString & a0) -> bool { return H2Core::Filesystem::file_executable(a0); }, "", pybind11::arg("path"));
		cl.def_static("file_executable", (bool (*)(const class QString &, bool)) &H2Core::Filesystem::file_executable, "returns true if the given path is an existing executable regular file\n \n\n the path to the file to check\n \n\n output not messages if set to true\n\nC++: H2Core::Filesystem::file_executable(const class QString &, bool) --> bool", pybind11::arg("path"), pybind11::arg("silent"));
		cl.def_static("dir_readable", [](const class QString & a0) -> bool { return H2Core::Filesystem::dir_readable(a0); }, "", pybind11::arg("path"));
		cl.def_static("dir_readable", (bool (*)(const class QString &, bool)) &H2Core::Filesystem::dir_readable, "returns true if the given path is a readable regular directory\n \n\n the path to the file to check\n \n\n output not messages if set to true\n\nC++: H2Core::Filesystem::dir_readable(const class QString &, bool) --> bool", pybind11::arg("path"), pybind11::arg("silent"));
		cl.def_static("dir_writable", [](const class QString & a0) -> bool { return H2Core::Filesystem::dir_writable(a0); }, "", pybind11::arg("path"));
		cl.def_static("dir_writable", (bool (*)(const class QString &, bool)) &H2Core::Filesystem::dir_writable, "returns true if the given path is a writable regular directory\n \n\n the path to the file to check\n \n\n output not messages if set to true\n\nC++: H2Core::Filesystem::dir_writable(const class QString &, bool) --> bool", pybind11::arg("path"), pybind11::arg("silent"));
		cl.def_static("path_usable", [](const class QString & a0) -> bool { return H2Core::Filesystem::path_usable(a0); }, "", pybind11::arg("path"));
		cl.def_static("path_usable", [](const class QString & a0, bool const & a1) -> bool { return H2Core::Filesystem::path_usable(a0, a1); }, "", pybind11::arg("path"), pybind11::arg("create"));
		cl.def_static("path_usable", (bool (*)(const class QString &, bool, bool)) &H2Core::Filesystem::path_usable, "returns true if the path is a readable and writable regular directory, create if it not exists\n \n\n the path to the file to check\n \n\n will try to create path if not exists and set to true\n \n\n output not messages if set to true\n\nC++: H2Core::Filesystem::path_usable(const class QString &, bool, bool) --> bool", pybind11::arg("path"), pybind11::arg("create"), pybind11::arg("silent"));
		cl.def_static("write_to_file", (bool (*)(const class QString &, const class QString &)) &H2Core::Filesystem::write_to_file, "writes to a file\n \n\n the destination path\n \n\n then string to write\n\nC++: H2Core::Filesystem::write_to_file(const class QString &, const class QString &) --> bool", pybind11::arg("dst"), pybind11::arg("content"));
		cl.def_static("file_copy", [](const class QString & a0, const class QString & a1) -> bool { return H2Core::Filesystem::file_copy(a0, a1); }, "", pybind11::arg("src"), pybind11::arg("dst"));
		cl.def_static("file_copy", (bool (*)(const class QString &, const class QString &, bool)) &H2Core::Filesystem::file_copy, "copy a source file to a destination\n \n\n source file path\n \n\n destination file path\n \n\n allow to overwrite an existing file if set to true\n\nC++: H2Core::Filesystem::file_copy(const class QString &, const class QString &, bool) --> bool", pybind11::arg("src"), pybind11::arg("dst"), pybind11::arg("overwrite"));
		cl.def_static("rm", [](const class QString & a0) -> bool { return H2Core::Filesystem::rm(a0); }, "", pybind11::arg("path"));
		cl.def_static("rm", (bool (*)(const class QString &, bool)) &H2Core::Filesystem::rm, "remove a path\n \n\n the path to be removed\n \n\n perform recursive removal if set to true\n\nC++: H2Core::Filesystem::rm(const class QString &, bool) --> bool", pybind11::arg("path"), pybind11::arg("recursive"));
		cl.def_static("mkdir", (bool (*)(const class QString &)) &H2Core::Filesystem::mkdir, "create a path\n \n\n the path to the directory to be created\n\nC++: H2Core::Filesystem::mkdir(const class QString &) --> bool", pybind11::arg("path"));
		cl.def_static("getPreferencesOverwritePath", (const class QString & (*)()) &H2Core::Filesystem::getPreferencesOverwritePath, "m_sPreferencesOverwritePath\n\nC++: H2Core::Filesystem::getPreferencesOverwritePath() --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def_static("setPreferencesOverwritePath", (void (*)(const class QString &)) &H2Core::Filesystem::setPreferencesOverwritePath, "Sets m_sPreferencesOverwritePath\n\nC++: H2Core::Filesystem::setPreferencesOverwritePath(const class QString &) --> void", pybind11::arg("sPath"));
		cl.def("assign", (class H2Core::Filesystem & (H2Core::Filesystem::*)(const class H2Core::Filesystem &)) &H2Core::Filesystem::operator=, "C++: H2Core::Filesystem::operator=(const class H2Core::Filesystem &) --> class H2Core::Filesystem &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
