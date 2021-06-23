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
#include <core/Object.h> // H2Core::Object
#include <core/Preferences.h> // 
#include <core/Preferences.h> // H2Core::Preferences
#include <core/Preferences.h> // H2Core::UIStyle
#include <core/Preferences.h> // H2Core::WindowProperties
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

// H2Core::WindowProperties file:core/Preferences.h line:49
struct PyCallBack_H2Core_WindowProperties : public H2Core::WindowProperties {
	using H2Core::WindowProperties::WindowProperties;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::WindowProperties *>(this), "toQString");
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

// H2Core::UIStyle file:core/Preferences.h line:74
struct PyCallBack_H2Core_UIStyle : public H2Core::UIStyle {
	using H2Core::UIStyle::UIStyle;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::UIStyle *>(this), "toQString");
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

// H2Core::Preferences file:core/Preferences.h line:109
struct PyCallBack_H2Core_Preferences : public H2Core::Preferences {
	using H2Core::Preferences::Preferences;

	class QString toQString(const class QString & a0, bool a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const H2Core::Preferences *>(this), "toQString");
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

void bind_core_Preferences(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B732_[H2Core::WindowProperties] ";
	{ // H2Core::WindowProperties file:core/Preferences.h line:49
		pybind11::class_<H2Core::WindowProperties, std::shared_ptr<H2Core::WindowProperties>, PyCallBack_H2Core_WindowProperties, H2Core::Object> cl(M("H2Core"), "WindowProperties", "");
		cl.def( pybind11::init( [](){ return new H2Core::WindowProperties(); }, [](){ return new PyCallBack_H2Core_WindowProperties(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_WindowProperties const &o){ return new PyCallBack_H2Core_WindowProperties(o); } ) );
		cl.def( pybind11::init( [](H2Core::WindowProperties const &o){ return new H2Core::WindowProperties(o); } ) );
		cl.def_readwrite("x", &H2Core::WindowProperties::x);
		cl.def_readwrite("y", &H2Core::WindowProperties::y);
		cl.def_readwrite("width", &H2Core::WindowProperties::width);
		cl.def_readwrite("height", &H2Core::WindowProperties::height);
		cl.def_readwrite("visible", &H2Core::WindowProperties::visible);
		cl.def_static("class_name", (const char * (*)()) &H2Core::WindowProperties::class_name, "C++: H2Core::WindowProperties::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("set", (void (H2Core::WindowProperties::*)(int, int, int, int, bool)) &H2Core::WindowProperties::set, "C++: H2Core::WindowProperties::set(int, int, int, int, bool) --> void", pybind11::arg("_x"), pybind11::arg("_y"), pybind11::arg("_width"), pybind11::arg("_height"), pybind11::arg("_visible"));
		cl.def("assign", (class H2Core::WindowProperties & (H2Core::WindowProperties::*)(const class H2Core::WindowProperties &)) &H2Core::WindowProperties::operator=, "C++: H2Core::WindowProperties::operator=(const class H2Core::WindowProperties &) --> class H2Core::WindowProperties &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B733_[H2Core::UIStyle] ";
	{ // H2Core::UIStyle file:core/Preferences.h line:74
		pybind11::class_<H2Core::UIStyle, std::shared_ptr<H2Core::UIStyle>, PyCallBack_H2Core_UIStyle, H2Core::Object> cl(M("H2Core"), "UIStyle", "	Colors for hydrogen");
		cl.def( pybind11::init( [](){ return new H2Core::UIStyle(); }, [](){ return new PyCallBack_H2Core_UIStyle(); } ) );
		cl.def( pybind11::init( [](PyCallBack_H2Core_UIStyle const &o){ return new PyCallBack_H2Core_UIStyle(o); } ) );
		cl.def( pybind11::init( [](H2Core::UIStyle const &o){ return new H2Core::UIStyle(o); } ) );
		cl.def_readwrite("m_songEditor_backgroundColor", &H2Core::UIStyle::m_songEditor_backgroundColor);
		cl.def_readwrite("m_songEditor_alternateRowColor", &H2Core::UIStyle::m_songEditor_alternateRowColor);
		cl.def_readwrite("m_songEditor_selectedRowColor", &H2Core::UIStyle::m_songEditor_selectedRowColor);
		cl.def_readwrite("m_songEditor_lineColor", &H2Core::UIStyle::m_songEditor_lineColor);
		cl.def_readwrite("m_songEditor_textColor", &H2Core::UIStyle::m_songEditor_textColor);
		cl.def_readwrite("m_songEditor_pattern1Color", &H2Core::UIStyle::m_songEditor_pattern1Color);
		cl.def_readwrite("m_patternEditor_backgroundColor", &H2Core::UIStyle::m_patternEditor_backgroundColor);
		cl.def_readwrite("m_patternEditor_alternateRowColor", &H2Core::UIStyle::m_patternEditor_alternateRowColor);
		cl.def_readwrite("m_patternEditor_selectedRowColor", &H2Core::UIStyle::m_patternEditor_selectedRowColor);
		cl.def_readwrite("m_patternEditor_textColor", &H2Core::UIStyle::m_patternEditor_textColor);
		cl.def_readwrite("m_patternEditor_noteColor", &H2Core::UIStyle::m_patternEditor_noteColor);
		cl.def_readwrite("m_patternEditor_noteoffColor", &H2Core::UIStyle::m_patternEditor_noteoffColor);
		cl.def_readwrite("m_patternEditor_lineColor", &H2Core::UIStyle::m_patternEditor_lineColor);
		cl.def_readwrite("m_patternEditor_line1Color", &H2Core::UIStyle::m_patternEditor_line1Color);
		cl.def_readwrite("m_patternEditor_line2Color", &H2Core::UIStyle::m_patternEditor_line2Color);
		cl.def_readwrite("m_patternEditor_line3Color", &H2Core::UIStyle::m_patternEditor_line3Color);
		cl.def_readwrite("m_patternEditor_line4Color", &H2Core::UIStyle::m_patternEditor_line4Color);
		cl.def_readwrite("m_patternEditor_line5Color", &H2Core::UIStyle::m_patternEditor_line5Color);
		cl.def_readwrite("m_selectionHighlightColor", &H2Core::UIStyle::m_selectionHighlightColor);
		cl.def_readwrite("m_selectionInactiveColor", &H2Core::UIStyle::m_selectionInactiveColor);
		cl.def_static("class_name", (const char * (*)()) &H2Core::UIStyle::class_name, "C++: H2Core::UIStyle::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class H2Core::UIStyle & (H2Core::UIStyle::*)(const class H2Core::UIStyle &)) &H2Core::UIStyle::operator=, "C++: H2Core::UIStyle::operator=(const class H2Core::UIStyle &) --> class H2Core::UIStyle &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B734_[H2Core::Preferences] ";
	{ // H2Core::Preferences file:core/Preferences.h line:109
		pybind11::class_<H2Core::Preferences, std::shared_ptr<H2Core::Preferences>, PyCallBack_H2Core_Preferences, H2Core::Object> cl(M("H2Core"), "Preferences", "	Manager for User Preferences File (singleton)");
		cl.def( pybind11::init( [](PyCallBack_H2Core_Preferences const &o){ return new PyCallBack_H2Core_Preferences(o); } ) );
		cl.def( pybind11::init( [](H2Core::Preferences const &o){ return new H2Core::Preferences(o); } ) );

		pybind11::enum_<H2Core::Preferences::FontSize>(cl, "FontSize", "Enables custom scaling of the font size in the GUI.")
			.value("Normal", H2Core::Preferences::FontSize::Normal)
			.value("Small", H2Core::Preferences::FontSize::Small)
			.value("Large", H2Core::Preferences::FontSize::Large);


		pybind11::enum_<H2Core::Preferences::UI_LAYOUT_TYPES>(cl, "UI_LAYOUT_TYPES", pybind11::arithmetic(), "")
			.value("UI_LAYOUT_SINGLE_PANE", H2Core::Preferences::UI_LAYOUT_SINGLE_PANE)
			.value("UI_LAYOUT_TABBED", H2Core::Preferences::UI_LAYOUT_TABBED)
			.export_values();


		pybind11::enum_<H2Core::Preferences::UI_SCALING_POLICY>(cl, "UI_SCALING_POLICY", pybind11::arithmetic(), "")
			.value("UI_SCALING_SMALLER", H2Core::Preferences::UI_SCALING_SMALLER)
			.value("UI_SCALING_SYSTEM", H2Core::Preferences::UI_SCALING_SYSTEM)
			.value("UI_SCALING_LARGER", H2Core::Preferences::UI_SCALING_LARGER)
			.export_values();


		pybind11::enum_<H2Core::Preferences::JackTrackOutputMode>(cl, "JackTrackOutputMode", "Specifies which audio settings will be applied to the sample\n		supplied in the JACK per track output ports.")
			.value("postFader", H2Core::Preferences::JackTrackOutputMode::postFader)
			.value("preFader", H2Core::Preferences::JackTrackOutputMode::preFader);


		pybind11::enum_<H2Core::Preferences::JackBBTSyncMethod>(cl, "JackBBTSyncMethod", "Specifies the variable, which has to remain constant in order\n to guarantee a working synchronization and relocation with\n Hydrogen as JACK timebase client.")
			.value("constMeasure", H2Core::Preferences::JackBBTSyncMethod::constMeasure)
			.value("identicalBars", H2Core::Preferences::JackBBTSyncMethod::identicalBars);

		cl.def_readwrite("__lastspatternDirectory", &H2Core::Preferences::__lastspatternDirectory);
		cl.def_readwrite("__lastsampleDirectory", &H2Core::Preferences::__lastsampleDirectory);
		cl.def_readwrite("__playsamplesonclicking", &H2Core::Preferences::__playsamplesonclicking);
		cl.def_readwrite("__playselectedinstrument", &H2Core::Preferences::__playselectedinstrument);
		cl.def_readwrite("m_bFollowPlayhead", &H2Core::Preferences::m_bFollowPlayhead);
		cl.def_readwrite("m_brestartLash", &H2Core::Preferences::m_brestartLash);
		cl.def_readwrite("m_bsetLash", &H2Core::Preferences::m_bsetLash);
		cl.def_readwrite("__expandSongItem", &H2Core::Preferences::__expandSongItem);
		cl.def_readwrite("__expandPatternItem", &H2Core::Preferences::__expandPatternItem);
		cl.def_readwrite("m_bbc", &H2Core::Preferences::m_bbc);
		cl.def_readwrite("m_mmcsetplay", &H2Core::Preferences::m_mmcsetplay);
		cl.def_readwrite("m_countOffset", &H2Core::Preferences::m_countOffset);
		cl.def_readwrite("m_startOffset", &H2Core::Preferences::m_startOffset);
		cl.def_readwrite("sServerList", &H2Core::Preferences::sServerList);
		cl.def_readwrite("m_patternCategories", &H2Core::Preferences::m_patternCategories);
		cl.def_readwrite("m_sAudioDriver", &H2Core::Preferences::m_sAudioDriver);
		cl.def_readwrite("m_bUseMetronome", &H2Core::Preferences::m_bUseMetronome);
		cl.def_readwrite("m_fMetronomeVolume", &H2Core::Preferences::m_fMetronomeVolume);
		cl.def_readwrite("m_nMaxNotes", &H2Core::Preferences::m_nMaxNotes);
		cl.def_readwrite("m_nBufferSize", &H2Core::Preferences::m_nBufferSize);
		cl.def_readwrite("m_nSampleRate", &H2Core::Preferences::m_nSampleRate);
		cl.def_readwrite("m_sOSSDevice", &H2Core::Preferences::m_sOSSDevice);
		cl.def_readwrite("m_sMidiDriver", &H2Core::Preferences::m_sMidiDriver);
		cl.def_readwrite("m_sMidiPortName", &H2Core::Preferences::m_sMidiPortName);
		cl.def_readwrite("m_sMidiOutputPortName", &H2Core::Preferences::m_sMidiOutputPortName);
		cl.def_readwrite("m_nMidiChannelFilter", &H2Core::Preferences::m_nMidiChannelFilter);
		cl.def_readwrite("m_bMidiNoteOffIgnore", &H2Core::Preferences::m_bMidiNoteOffIgnore);
		cl.def_readwrite("m_bMidiFixedMapping", &H2Core::Preferences::m_bMidiFixedMapping);
		cl.def_readwrite("m_bMidiDiscardNoteAfterAction", &H2Core::Preferences::m_bMidiDiscardNoteAfterAction);
		cl.def_readwrite("m_bEnableMidiFeedback", &H2Core::Preferences::m_bEnableMidiFeedback);
		cl.def_readwrite("m_bOscServerEnabled", &H2Core::Preferences::m_bOscServerEnabled);
		cl.def_readwrite("m_bOscFeedbackEnabled", &H2Core::Preferences::m_bOscFeedbackEnabled);
		cl.def_readwrite("m_nOscTemporaryPort", &H2Core::Preferences::m_nOscTemporaryPort);
		cl.def_readwrite("m_nOscServerPort", &H2Core::Preferences::m_nOscServerPort);
		cl.def_readwrite("m_sAlsaAudioDevice", &H2Core::Preferences::m_sAlsaAudioDevice);
		cl.def_readwrite("m_sJackPortName1", &H2Core::Preferences::m_sJackPortName1);
		cl.def_readwrite("m_sJackPortName2", &H2Core::Preferences::m_sJackPortName2);
		cl.def_readwrite("m_bJackTransportMode", &H2Core::Preferences::m_bJackTransportMode);
		cl.def_readwrite("m_bJackConnectDefaults", &H2Core::Preferences::m_bJackConnectDefaults);
		cl.def_readwrite("m_bJackTrackOuts", &H2Core::Preferences::m_bJackTrackOuts);
		cl.def_readwrite("m_JackTrackOutputMode", &H2Core::Preferences::m_JackTrackOutputMode);
		cl.def_readwrite("m_bJackTimebaseEnabled", &H2Core::Preferences::m_bJackTimebaseEnabled);
		cl.def_readwrite("m_JackBBTSync", &H2Core::Preferences::m_JackBBTSync);
		cl.def_readwrite("m_bJackMasterMode", &H2Core::Preferences::m_bJackMasterMode);
		cl.def_readwrite("m_sDefaultEditor", &H2Core::Preferences::m_sDefaultEditor);
		cl.def_readwrite("m_rubberBandCLIexecutable", &H2Core::Preferences::m_rubberBandCLIexecutable);
		cl.def_static("class_name", (const char * (*)()) &H2Core::Preferences::class_name, "C++: H2Core::Preferences::class_name() --> const char *", pybind11::return_value_policy::automatic);
		cl.def_static("create_instance", (void (*)()) &H2Core::Preferences::create_instance, "If #__instance equals 0, a new Preferences singleton will\n be created and stored in it.\n\n It is called in Hydrogen::create_instance().\n\nC++: H2Core::Preferences::create_instance() --> void");
		cl.def_static("get_instance", (class H2Core::Preferences * (*)()) &H2Core::Preferences::get_instance, "Returns a pointer to the current Preferences singleton\n stored in #__instance.\n\nC++: H2Core::Preferences::get_instance() --> class H2Core::Preferences *", pybind11::return_value_policy::automatic);
		cl.def("loadPreferences", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::loadPreferences, "Load the preferences file\n\nC++: H2Core::Preferences::loadPreferences(bool) --> void", pybind11::arg("bGlobal"));
		cl.def("savePreferences", (void (H2Core::Preferences::*)()) &H2Core::Preferences::savePreferences, "Save the preferences file\n\nC++: H2Core::Preferences::savePreferences() --> void");
		cl.def("getDefaultEditor", (const class QString & (H2Core::Preferences::*)()) &H2Core::Preferences::getDefaultEditor, "C++: H2Core::Preferences::getDefaultEditor() --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("setDefaultEditor", (void (H2Core::Preferences::*)(class QString)) &H2Core::Preferences::setDefaultEditor, "C++: H2Core::Preferences::setDefaultEditor(class QString) --> void", pybind11::arg("editor"));
		cl.def("getDefaultUILayout", (int (H2Core::Preferences::*)()) &H2Core::Preferences::getDefaultUILayout, "C++: H2Core::Preferences::getDefaultUILayout() --> int");
		cl.def("setDefaultUILayout", (void (H2Core::Preferences::*)(int)) &H2Core::Preferences::setDefaultUILayout, "C++: H2Core::Preferences::setDefaultUILayout(int) --> void", pybind11::arg("layout"));
		cl.def("getUIScalingPolicy", (int (H2Core::Preferences::*)()) &H2Core::Preferences::getUIScalingPolicy, "C++: H2Core::Preferences::getUIScalingPolicy() --> int");
		cl.def("setUIScalingPolicy", (void (H2Core::Preferences::*)(int)) &H2Core::Preferences::setUIScalingPolicy, "C++: H2Core::Preferences::setUIScalingPolicy(int) --> void", pybind11::arg("nPolicy"));
		cl.def("getPreferredLanguage", (const class QString & (H2Core::Preferences::*)()) &H2Core::Preferences::getPreferredLanguage, "C++: H2Core::Preferences::getPreferredLanguage() --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("setPreferredLanguage", (void (H2Core::Preferences::*)(const class QString &)) &H2Core::Preferences::setPreferredLanguage, "C++: H2Core::Preferences::setPreferredLanguage(const class QString &) --> void", pybind11::arg("sLanguage"));
		cl.def("setRestoreLastSongEnabled", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setRestoreLastSongEnabled, "C++: H2Core::Preferences::setRestoreLastSongEnabled(bool) --> void", pybind11::arg("restore"));
		cl.def("setRestoreLastPlaylistEnabled", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setRestoreLastPlaylistEnabled, "C++: H2Core::Preferences::setRestoreLastPlaylistEnabled(bool) --> void", pybind11::arg("restore"));
		cl.def("setUseRelativeFilenamesForPlaylists", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setUseRelativeFilenamesForPlaylists, "C++: H2Core::Preferences::setUseRelativeFilenamesForPlaylists(bool) --> void", pybind11::arg("value"));
		cl.def("setShowDevelWarning", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setShowDevelWarning, "C++: H2Core::Preferences::setShowDevelWarning(bool) --> void", pybind11::arg("value"));
		cl.def("getShowDevelWarning", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::getShowDevelWarning, "C++: H2Core::Preferences::getShowDevelWarning() --> bool");
		cl.def("getShowNoteOverwriteWarning", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::getShowNoteOverwriteWarning, "C++: H2Core::Preferences::getShowNoteOverwriteWarning() --> bool");
		cl.def("setShowNoteOverwriteWarning", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setShowNoteOverwriteWarning, "C++: H2Core::Preferences::setShowNoteOverwriteWarning(bool) --> void", pybind11::arg("bValue"));
		cl.def("isRestoreLastSongEnabled", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::isRestoreLastSongEnabled, "C++: H2Core::Preferences::isRestoreLastSongEnabled() --> bool");
		cl.def("isRestoreLastPlaylistEnabled", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::isRestoreLastPlaylistEnabled, "C++: H2Core::Preferences::isRestoreLastPlaylistEnabled() --> bool");
		cl.def("isPlaylistUsingRelativeFilenames", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::isPlaylistUsingRelativeFilenames, "C++: H2Core::Preferences::isPlaylistUsingRelativeFilenames() --> bool");
		cl.def("setLastSongFilename", (void (H2Core::Preferences::*)(const class QString &)) &H2Core::Preferences::setLastSongFilename, "C++: H2Core::Preferences::setLastSongFilename(const class QString &) --> void", pybind11::arg("filename"));
		cl.def("getLastSongFilename", (const class QString & (H2Core::Preferences::*)()) &H2Core::Preferences::getLastSongFilename, "C++: H2Core::Preferences::getLastSongFilename() --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("setLastPlaylistFilename", (void (H2Core::Preferences::*)(const class QString &)) &H2Core::Preferences::setLastPlaylistFilename, "C++: H2Core::Preferences::setLastPlaylistFilename(const class QString &) --> void", pybind11::arg("filename"));
		cl.def("getLastPlaylistFilename", (const class QString & (H2Core::Preferences::*)()) &H2Core::Preferences::getLastPlaylistFilename, "C++: H2Core::Preferences::getLastPlaylistFilename() --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("setHearNewNotes", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setHearNewNotes, "C++: H2Core::Preferences::setHearNewNotes(bool) --> void", pybind11::arg("value"));
		cl.def("getHearNewNotes", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::getHearNewNotes, "C++: H2Core::Preferences::getHearNewNotes() --> bool");
		cl.def("setRecordEvents", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setRecordEvents, "C++: H2Core::Preferences::setRecordEvents(bool) --> void", pybind11::arg("value"));
		cl.def("getRecordEvents", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::getRecordEvents, "C++: H2Core::Preferences::getRecordEvents() --> bool");
		cl.def("setPunchInPos", (void (H2Core::Preferences::*)(unsigned int)) &H2Core::Preferences::setPunchInPos, "C++: H2Core::Preferences::setPunchInPos(unsigned int) --> void", pybind11::arg("pos"));
		cl.def("getPunchInPos", (int (H2Core::Preferences::*)()) &H2Core::Preferences::getPunchInPos, "C++: H2Core::Preferences::getPunchInPos() --> int");
		cl.def("setPunchOutPos", (void (H2Core::Preferences::*)(unsigned int)) &H2Core::Preferences::setPunchOutPos, "C++: H2Core::Preferences::setPunchOutPos(unsigned int) --> void", pybind11::arg("pos"));
		cl.def("getPunchOutPos", (int (H2Core::Preferences::*)()) &H2Core::Preferences::getPunchOutPos, "C++: H2Core::Preferences::getPunchOutPos() --> int");
		cl.def("inPunchArea", (bool (H2Core::Preferences::*)(int)) &H2Core::Preferences::inPunchArea, "C++: H2Core::Preferences::inPunchArea(int) --> bool", pybind11::arg("pos"));
		cl.def("unsetPunchArea", (void (H2Core::Preferences::*)()) &H2Core::Preferences::unsetPunchArea, "C++: H2Core::Preferences::unsetPunchArea() --> void");
		cl.def("setQuantizeEvents", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setQuantizeEvents, "C++: H2Core::Preferences::setQuantizeEvents(bool) --> void", pybind11::arg("value"));
		cl.def("getQuantizeEvents", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::getQuantizeEvents, "C++: H2Core::Preferences::getQuantizeEvents() --> bool");
		cl.def("insertRecentFile", (void (H2Core::Preferences::*)(const class QString)) &H2Core::Preferences::insertRecentFile, "C++: H2Core::Preferences::insertRecentFile(const class QString) --> void", pybind11::arg("sFilename"));
		cl.def("setMostRecentFX", (void (H2Core::Preferences::*)(class QString)) &H2Core::Preferences::setMostRecentFX, "C++: H2Core::Preferences::setMostRecentFX(class QString) --> void", pybind11::arg(""));
		cl.def("getQTStyle", (const class QString & (H2Core::Preferences::*)()) &H2Core::Preferences::getQTStyle, "C++: H2Core::Preferences::getQTStyle() --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("setQTStyle", (void (H2Core::Preferences::*)(const class QString &)) &H2Core::Preferences::setQTStyle, "C++: H2Core::Preferences::setQTStyle(const class QString &) --> void", pybind11::arg("sStyle"));
		cl.def("getApplicationFontFamily", (const class QString & (H2Core::Preferences::*)() const) &H2Core::Preferences::getApplicationFontFamily, "C++: H2Core::Preferences::getApplicationFontFamily() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("setApplicationFontFamily", (void (H2Core::Preferences::*)(const class QString &)) &H2Core::Preferences::setApplicationFontFamily, "C++: H2Core::Preferences::setApplicationFontFamily(const class QString &) --> void", pybind11::arg("family"));
		cl.def("getLevel2FontFamily", (const class QString & (H2Core::Preferences::*)() const) &H2Core::Preferences::getLevel2FontFamily, "C++: H2Core::Preferences::getLevel2FontFamily() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("setLevel2FontFamily", (void (H2Core::Preferences::*)(const class QString &)) &H2Core::Preferences::setLevel2FontFamily, "C++: H2Core::Preferences::setLevel2FontFamily(const class QString &) --> void", pybind11::arg("family"));
		cl.def("getLevel3FontFamily", (const class QString & (H2Core::Preferences::*)() const) &H2Core::Preferences::getLevel3FontFamily, "C++: H2Core::Preferences::getLevel3FontFamily() const --> const class QString &", pybind11::return_value_policy::automatic);
		cl.def("setLevel3FontFamily", (void (H2Core::Preferences::*)(const class QString &)) &H2Core::Preferences::setLevel3FontFamily, "C++: H2Core::Preferences::setLevel3FontFamily(const class QString &) --> void", pybind11::arg("family"));
		cl.def("getFontSize", (enum H2Core::Preferences::FontSize (H2Core::Preferences::*)() const) &H2Core::Preferences::getFontSize, "C++: H2Core::Preferences::getFontSize() const --> enum H2Core::Preferences::FontSize");
		cl.def("setFontSize", (void (H2Core::Preferences::*)(enum H2Core::Preferences::FontSize)) &H2Core::Preferences::setFontSize, "C++: H2Core::Preferences::setFontSize(enum H2Core::Preferences::FontSize) --> void", pybind11::arg("fontSize"));
		cl.def("getMixerFalloffSpeed", (float (H2Core::Preferences::*)()) &H2Core::Preferences::getMixerFalloffSpeed, "C++: H2Core::Preferences::getMixerFalloffSpeed() --> float");
		cl.def("setMixerFalloffSpeed", (void (H2Core::Preferences::*)(float)) &H2Core::Preferences::setMixerFalloffSpeed, "C++: H2Core::Preferences::setMixerFalloffSpeed(float) --> void", pybind11::arg("value"));
		cl.def("showInstrumentPeaks", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::showInstrumentPeaks, "C++: H2Core::Preferences::showInstrumentPeaks() --> bool");
		cl.def("setInstrumentPeaks", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setInstrumentPeaks, "C++: H2Core::Preferences::setInstrumentPeaks(bool) --> void", pybind11::arg("value"));
		cl.def("getPatternEditorGridResolution", (int (H2Core::Preferences::*)()) &H2Core::Preferences::getPatternEditorGridResolution, "C++: H2Core::Preferences::getPatternEditorGridResolution() --> int");
		cl.def("setPatternEditorGridResolution", (void (H2Core::Preferences::*)(int)) &H2Core::Preferences::setPatternEditorGridResolution, "C++: H2Core::Preferences::setPatternEditorGridResolution(int) --> void", pybind11::arg("value"));
		cl.def("isPatternEditorUsingTriplets", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::isPatternEditorUsingTriplets, "C++: H2Core::Preferences::isPatternEditorUsingTriplets() --> bool");
		cl.def("setPatternEditorUsingTriplets", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setPatternEditorUsingTriplets, "C++: H2Core::Preferences::setPatternEditorUsingTriplets(bool) --> void", pybind11::arg("value"));
		cl.def("isFXTabVisible", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::isFXTabVisible, "C++: H2Core::Preferences::isFXTabVisible() --> bool");
		cl.def("setFXTabVisible", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setFXTabVisible, "C++: H2Core::Preferences::setFXTabVisible(bool) --> void", pybind11::arg("value"));
		cl.def("getShowAutomationArea", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::getShowAutomationArea, "C++: H2Core::Preferences::getShowAutomationArea() --> bool");
		cl.def("setShowAutomationArea", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setShowAutomationArea, "C++: H2Core::Preferences::setShowAutomationArea(bool) --> void", pybind11::arg("value"));
		cl.def("getPatternEditorGridHeight", (unsigned int (H2Core::Preferences::*)()) &H2Core::Preferences::getPatternEditorGridHeight, "C++: H2Core::Preferences::getPatternEditorGridHeight() --> unsigned int");
		cl.def("setPatternEditorGridHeight", (void (H2Core::Preferences::*)(unsigned int)) &H2Core::Preferences::setPatternEditorGridHeight, "C++: H2Core::Preferences::setPatternEditorGridHeight(unsigned int) --> void", pybind11::arg("value"));
		cl.def("getPatternEditorGridWidth", (unsigned int (H2Core::Preferences::*)()) &H2Core::Preferences::getPatternEditorGridWidth, "C++: H2Core::Preferences::getPatternEditorGridWidth() --> unsigned int");
		cl.def("setPatternEditorGridWidth", (void (H2Core::Preferences::*)(unsigned int)) &H2Core::Preferences::setPatternEditorGridWidth, "C++: H2Core::Preferences::setPatternEditorGridWidth(unsigned int) --> void", pybind11::arg("value"));
		cl.def("getSongEditorGridHeight", (unsigned int (H2Core::Preferences::*)()) &H2Core::Preferences::getSongEditorGridHeight, "C++: H2Core::Preferences::getSongEditorGridHeight() --> unsigned int");
		cl.def("setSongEditorGridHeight", (void (H2Core::Preferences::*)(unsigned int)) &H2Core::Preferences::setSongEditorGridHeight, "C++: H2Core::Preferences::setSongEditorGridHeight(unsigned int) --> void", pybind11::arg("value"));
		cl.def("getSongEditorGridWidth", (unsigned int (H2Core::Preferences::*)()) &H2Core::Preferences::getSongEditorGridWidth, "C++: H2Core::Preferences::getSongEditorGridWidth() --> unsigned int");
		cl.def("setSongEditorGridWidth", (void (H2Core::Preferences::*)(unsigned int)) &H2Core::Preferences::setSongEditorGridWidth, "C++: H2Core::Preferences::setSongEditorGridWidth(unsigned int) --> void", pybind11::arg("value"));
		cl.def("setColoringMethod", (void (H2Core::Preferences::*)(int)) &H2Core::Preferences::setColoringMethod, "C++: H2Core::Preferences::setColoringMethod(int) --> void", pybind11::arg("nValue"));
		cl.def("getColoringMethod", (int (H2Core::Preferences::*)() const) &H2Core::Preferences::getColoringMethod, "C++: H2Core::Preferences::getColoringMethod() const --> int");
		cl.def("setMaxPatternColors", (void (H2Core::Preferences::*)(int)) &H2Core::Preferences::setMaxPatternColors, "C++: H2Core::Preferences::setMaxPatternColors(int) --> void", pybind11::arg("nValue"));
		cl.def("getMaxPatternColors", (int (H2Core::Preferences::*)() const) &H2Core::Preferences::getMaxPatternColors, "C++: H2Core::Preferences::getMaxPatternColors() const --> int");
		cl.def("setVisiblePatternColors", (void (H2Core::Preferences::*)(int)) &H2Core::Preferences::setVisiblePatternColors, "C++: H2Core::Preferences::setVisiblePatternColors(int) --> void", pybind11::arg("nValue"));
		cl.def("getVisiblePatternColors", (int (H2Core::Preferences::*)() const) &H2Core::Preferences::getVisiblePatternColors, "C++: H2Core::Preferences::getVisiblePatternColors() const --> int");
		cl.def("getMainFormProperties", (class H2Core::WindowProperties (H2Core::Preferences::*)()) &H2Core::Preferences::getMainFormProperties, "C++: H2Core::Preferences::getMainFormProperties() --> class H2Core::WindowProperties");
		cl.def("setMainFormProperties", (void (H2Core::Preferences::*)(const class H2Core::WindowProperties &)) &H2Core::Preferences::setMainFormProperties, "C++: H2Core::Preferences::setMainFormProperties(const class H2Core::WindowProperties &) --> void", pybind11::arg("prop"));
		cl.def("getMixerProperties", (class H2Core::WindowProperties (H2Core::Preferences::*)()) &H2Core::Preferences::getMixerProperties, "C++: H2Core::Preferences::getMixerProperties() --> class H2Core::WindowProperties");
		cl.def("setMixerProperties", (void (H2Core::Preferences::*)(const class H2Core::WindowProperties &)) &H2Core::Preferences::setMixerProperties, "C++: H2Core::Preferences::setMixerProperties(const class H2Core::WindowProperties &) --> void", pybind11::arg("prop"));
		cl.def("getPatternEditorProperties", (class H2Core::WindowProperties (H2Core::Preferences::*)()) &H2Core::Preferences::getPatternEditorProperties, "C++: H2Core::Preferences::getPatternEditorProperties() --> class H2Core::WindowProperties");
		cl.def("setPatternEditorProperties", (void (H2Core::Preferences::*)(const class H2Core::WindowProperties &)) &H2Core::Preferences::setPatternEditorProperties, "C++: H2Core::Preferences::setPatternEditorProperties(const class H2Core::WindowProperties &) --> void", pybind11::arg("prop"));
		cl.def("getSongEditorProperties", (class H2Core::WindowProperties (H2Core::Preferences::*)()) &H2Core::Preferences::getSongEditorProperties, "C++: H2Core::Preferences::getSongEditorProperties() --> class H2Core::WindowProperties");
		cl.def("setSongEditorProperties", (void (H2Core::Preferences::*)(const class H2Core::WindowProperties &)) &H2Core::Preferences::setSongEditorProperties, "C++: H2Core::Preferences::setSongEditorProperties(const class H2Core::WindowProperties &) --> void", pybind11::arg("prop"));
		cl.def("getInstrumentRackProperties", (class H2Core::WindowProperties (H2Core::Preferences::*)()) &H2Core::Preferences::getInstrumentRackProperties, "C++: H2Core::Preferences::getInstrumentRackProperties() --> class H2Core::WindowProperties");
		cl.def("setInstrumentRackProperties", (void (H2Core::Preferences::*)(const class H2Core::WindowProperties &)) &H2Core::Preferences::setInstrumentRackProperties, "C++: H2Core::Preferences::setInstrumentRackProperties(const class H2Core::WindowProperties &) --> void", pybind11::arg("prop"));
		cl.def("getAudioEngineInfoProperties", (class H2Core::WindowProperties (H2Core::Preferences::*)()) &H2Core::Preferences::getAudioEngineInfoProperties, "C++: H2Core::Preferences::getAudioEngineInfoProperties() --> class H2Core::WindowProperties");
		cl.def("setAudioEngineInfoProperties", (void (H2Core::Preferences::*)(const class H2Core::WindowProperties &)) &H2Core::Preferences::setAudioEngineInfoProperties, "C++: H2Core::Preferences::setAudioEngineInfoProperties(const class H2Core::WindowProperties &) --> void", pybind11::arg("prop"));
		cl.def("getLadspaProperties", (class H2Core::WindowProperties (H2Core::Preferences::*)(unsigned int)) &H2Core::Preferences::getLadspaProperties, "C++: H2Core::Preferences::getLadspaProperties(unsigned int) --> class H2Core::WindowProperties", pybind11::arg("nFX"));
		cl.def("setLadspaProperties", (void (H2Core::Preferences::*)(unsigned int, const class H2Core::WindowProperties &)) &H2Core::Preferences::setLadspaProperties, "C++: H2Core::Preferences::setLadspaProperties(unsigned int, const class H2Core::WindowProperties &) --> void", pybind11::arg("nFX"), pybind11::arg("prop"));
		cl.def("getDefaultUIStyle", (class H2Core::UIStyle * (H2Core::Preferences::*)()) &H2Core::Preferences::getDefaultUIStyle, "C++: H2Core::Preferences::getDefaultUIStyle() --> class H2Core::UIStyle *", pybind11::return_value_policy::automatic);
		cl.def("patternModePlaysSelected", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::patternModePlaysSelected, "#m_bPatternModePlaysSelected\n\nC++: H2Core::Preferences::patternModePlaysSelected() --> bool");
		cl.def("setPatternModePlaysSelected", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setPatternModePlaysSelected, "Sets #m_bPatternModePlaysSelected\n\nC++: H2Core::Preferences::setPatternModePlaysSelected(bool) --> void", pybind11::arg("b"));
		cl.def("useLash", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::useLash, "C++: H2Core::Preferences::useLash() --> bool");
		cl.def("setUseLash", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setUseLash, "C++: H2Core::Preferences::setUseLash(bool) --> void", pybind11::arg("b"));
		cl.def("hideKeyboardCursor", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::hideKeyboardCursor, "C++: H2Core::Preferences::hideKeyboardCursor() --> bool");
		cl.def("setHideKeyboardCursor", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setHideKeyboardCursor, "C++: H2Core::Preferences::setHideKeyboardCursor(bool) --> void", pybind11::arg("b"));
		cl.def("setMaxBars", (void (H2Core::Preferences::*)(const int)) &H2Core::Preferences::setMaxBars, "Sets #m_nMaxBars.\n\nC++: H2Core::Preferences::setMaxBars(const int) --> void", pybind11::arg("bars"));
		cl.def("getMaxBars", (int (H2Core::Preferences::*)() const) &H2Core::Preferences::getMaxBars, "#m_nMaxBars.\n\nC++: H2Core::Preferences::getMaxBars() const --> int");
		cl.def("setMaxLayers", (void (H2Core::Preferences::*)(const int)) &H2Core::Preferences::setMaxLayers, "Sets #m_nMaxLayers.\n\nC++: H2Core::Preferences::setMaxLayers(const int) --> void", pybind11::arg("layers"));
		cl.def("getMaxLayers", (int (H2Core::Preferences::*)() const) &H2Core::Preferences::getMaxLayers, "#m_nMaxLayers.\n\nC++: H2Core::Preferences::getMaxLayers() const --> int");
		cl.def("setWaitForSessionHandler", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setWaitForSessionHandler, "C++: H2Core::Preferences::setWaitForSessionHandler(bool) --> void", pybind11::arg("value"));
		cl.def("getWaitForSessionHandler", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::getWaitForSessionHandler, "C++: H2Core::Preferences::getWaitForSessionHandler() --> bool");
		cl.def("setNsmClientId", (void (H2Core::Preferences::*)(const class QString &)) &H2Core::Preferences::setNsmClientId, "C++: H2Core::Preferences::setNsmClientId(const class QString &) --> void", pybind11::arg("nsmClientId"));
		cl.def("getNsmClientId", (class QString (H2Core::Preferences::*)()) &H2Core::Preferences::getNsmClientId, "C++: H2Core::Preferences::getNsmClientId() --> class QString");
		cl.def("setNsmSongName", (void (H2Core::Preferences::*)(const class QString &)) &H2Core::Preferences::setNsmSongName, "C++: H2Core::Preferences::setNsmSongName(const class QString &) --> void", pybind11::arg("nsmSongName"));
		cl.def("getNsmSongName", (class QString (H2Core::Preferences::*)()) &H2Core::Preferences::getNsmSongName, "C++: H2Core::Preferences::getNsmSongName() --> class QString");
		cl.def("getOscServerEnabled", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::getOscServerEnabled, "#m_bOscServerEnabled\n\nC++: H2Core::Preferences::getOscServerEnabled() --> bool");
		cl.def("setOscServerEnabled", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setOscServerEnabled, "Sets #m_bOscServerEnabled\n\nC++: H2Core::Preferences::setOscServerEnabled(bool) --> void", pybind11::arg("val"));
		cl.def("getOscFeedbackEnabled", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::getOscFeedbackEnabled, "#m_bOscFeedbackEnabled\n\nC++: H2Core::Preferences::getOscFeedbackEnabled() --> bool");
		cl.def("setOscFeedbackEnabled", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setOscFeedbackEnabled, "Sets #m_bOscFeedbackEnabled\n\nC++: H2Core::Preferences::setOscFeedbackEnabled(bool) --> void", pybind11::arg("val"));
		cl.def("getOscServerPort", (int (H2Core::Preferences::*)()) &H2Core::Preferences::getOscServerPort, "#m_nOscServerPort\n\nC++: H2Core::Preferences::getOscServerPort() --> int");
		cl.def("setOscServerPort", (void (H2Core::Preferences::*)(int)) &H2Core::Preferences::setOscServerPort, "Sets #m_nOscServerPort\n\nC++: H2Core::Preferences::setOscServerPort(int) --> void", pybind11::arg("oscPort"));
		cl.def("getUseTimelineBpm", (bool (H2Core::Preferences::*)()) &H2Core::Preferences::getUseTimelineBpm, "Whether to use the bpm of the timeline.\n \n\n #__useTimelineBpm \n\nC++: H2Core::Preferences::getUseTimelineBpm() --> bool");
		cl.def("setUseTimelineBpm", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setUseTimelineBpm, "Setting #__useTimelineBpm.\n \n\n New choice. \n\nC++: H2Core::Preferences::setUseTimelineBpm(bool) --> void", pybind11::arg("val"));
		cl.def("setShowPlaybackTrack", (void (H2Core::Preferences::*)(bool)) &H2Core::Preferences::setShowPlaybackTrack, "C++: H2Core::Preferences::setShowPlaybackTrack(bool) --> void", pybind11::arg("val"));
		cl.def("getShowPlaybackTrack", (bool (H2Core::Preferences::*)() const) &H2Core::Preferences::getShowPlaybackTrack, "C++: H2Core::Preferences::getShowPlaybackTrack() const --> bool");
		cl.def("getRubberBandCalcTime", (int (H2Core::Preferences::*)()) &H2Core::Preferences::getRubberBandCalcTime, "C++: H2Core::Preferences::getRubberBandCalcTime() --> int");
		cl.def("setRubberBandCalcTime", (void (H2Core::Preferences::*)(int)) &H2Core::Preferences::setRubberBandCalcTime, "C++: H2Core::Preferences::setRubberBandCalcTime(int) --> void", pybind11::arg("val"));
		cl.def("getRubberBandBatchMode", (int (H2Core::Preferences::*)()) &H2Core::Preferences::getRubberBandBatchMode, "C++: H2Core::Preferences::getRubberBandBatchMode() --> int");
		cl.def("setRubberBandBatchMode", (void (H2Core::Preferences::*)(int)) &H2Core::Preferences::setRubberBandBatchMode, "C++: H2Core::Preferences::setRubberBandBatchMode(int) --> void", pybind11::arg("val"));
		cl.def("getLastOpenTab", (int (H2Core::Preferences::*)()) &H2Core::Preferences::getLastOpenTab, "C++: H2Core::Preferences::getLastOpenTab() --> int");
		cl.def("setLastOpenTab", (void (H2Core::Preferences::*)(int)) &H2Core::Preferences::setLastOpenTab, "C++: H2Core::Preferences::setLastOpenTab(int) --> void", pybind11::arg("n"));
		cl.def("setH2ProcessName", (void (H2Core::Preferences::*)(const class QString &)) &H2Core::Preferences::setH2ProcessName, "C++: H2Core::Preferences::setH2ProcessName(const class QString &) --> void", pybind11::arg("processName"));
		cl.def("getH2ProcessName", (class QString (H2Core::Preferences::*)()) &H2Core::Preferences::getH2ProcessName, "C++: H2Core::Preferences::getH2ProcessName() --> class QString");
		cl.def("getExportSampleDepthIdx", (int (H2Core::Preferences::*)() const) &H2Core::Preferences::getExportSampleDepthIdx, "C++: H2Core::Preferences::getExportSampleDepthIdx() const --> int");
		cl.def("setExportSampleDepthIdx", (void (H2Core::Preferences::*)(int)) &H2Core::Preferences::setExportSampleDepthIdx, "C++: H2Core::Preferences::setExportSampleDepthIdx(int) --> void", pybind11::arg("nExportSampleDepthIdx"));
		cl.def("getExportSampleRateIdx", (int (H2Core::Preferences::*)() const) &H2Core::Preferences::getExportSampleRateIdx, "C++: H2Core::Preferences::getExportSampleRateIdx() const --> int");
		cl.def("setExportSampleRateIdx", (void (H2Core::Preferences::*)(int)) &H2Core::Preferences::setExportSampleRateIdx, "C++: H2Core::Preferences::setExportSampleRateIdx(int) --> void", pybind11::arg("nExportSampleRateIdx"));
		cl.def("getExportModeIdx", (int (H2Core::Preferences::*)() const) &H2Core::Preferences::getExportModeIdx, "C++: H2Core::Preferences::getExportModeIdx() const --> int");
		cl.def("setExportModeIdx", (void (H2Core::Preferences::*)(int)) &H2Core::Preferences::setExportModeIdx, "C++: H2Core::Preferences::setExportModeIdx(int) --> void", pybind11::arg("nExportMode"));
		cl.def("getExportDirectory", (class QString (H2Core::Preferences::*)() const) &H2Core::Preferences::getExportDirectory, "C++: H2Core::Preferences::getExportDirectory() const --> class QString");
		cl.def("setExportDirectory", (void (H2Core::Preferences::*)(const class QString &)) &H2Core::Preferences::setExportDirectory, "C++: H2Core::Preferences::setExportDirectory(const class QString &) --> void", pybind11::arg("sExportDirectory"));
		cl.def("getExportTemplateIdx", (int (H2Core::Preferences::*)() const) &H2Core::Preferences::getExportTemplateIdx, "C++: H2Core::Preferences::getExportTemplateIdx() const --> int");
		cl.def("setExportTemplateIdx", (void (H2Core::Preferences::*)(int)) &H2Core::Preferences::setExportTemplateIdx, "C++: H2Core::Preferences::setExportTemplateIdx(int) --> void", pybind11::arg("nExportTemplateIdx"));
		cl.def("getMidiExportMode", (int (H2Core::Preferences::*)() const) &H2Core::Preferences::getMidiExportMode, "C++: H2Core::Preferences::getMidiExportMode() const --> int");
		cl.def("setMidiExportMode", (void (H2Core::Preferences::*)(int)) &H2Core::Preferences::setMidiExportMode, "C++: H2Core::Preferences::setMidiExportMode(int) --> void", pybind11::arg("nExportMode"));
		cl.def("getMidiExportDirectory", (class QString (H2Core::Preferences::*)() const) &H2Core::Preferences::getMidiExportDirectory, "C++: H2Core::Preferences::getMidiExportDirectory() const --> class QString");
		cl.def("setMidiExportDirectory", (void (H2Core::Preferences::*)(const class QString &)) &H2Core::Preferences::setMidiExportDirectory, "C++: H2Core::Preferences::setMidiExportDirectory(const class QString &) --> void", pybind11::arg("sExportDirectory"));
		cl.def("assign", (class H2Core::Preferences & (H2Core::Preferences::*)(const class H2Core::Preferences &)) &H2Core::Preferences::operator=, "C++: H2Core::Preferences::operator=(const class H2Core::Preferences &) --> class H2Core::Preferences &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
