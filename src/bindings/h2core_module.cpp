#include <h2core_module.hpp>
namespace py = pybind11;

using namespace H2Core;
PYBIND11_MODULE(h2core, m) {

	py::class_<H2Core::Object, std::shared_ptr<H2Core::Object>> _Object(m, "Object");
	_Object.def(py::init<const H2Core::Object &>());
	_Object.def(py::init<const char *>());
	_Object.def("class_name", &H2Core::Object::class_name);
	_Object.def_static("set_count", &H2Core::Object::set_count,
		"enable/disable class instances counting",
		py::arg("flag"));
	_Object.def_static("count_active", &H2Core::Object::count_active);
	_Object.def_static("objects_count", &H2Core::Object::objects_count);
	// [<TypeDef 'ostream'>] _Object.def_static("write_objects_map_to", &H2Core::Object::write_objects_map_to,
		// [<TypeDef 'ostream'>] "output the full objects map to a given ostream",
	// [<TypeDef 'ostream'>] 	py::arg("out"),
	// [<TypeDef 'ostream'>] 	py::arg("map"));
	_Object.def_static("write_objects_map_to_cerr", &H2Core::Object::write_objects_map_to_cerr);
	_Object.def_static("bootstrap", &H2Core::Object::bootstrap,
		"must be called before any Object instantiation !",
		py::arg("logger"),
		py::arg("count"));
	_Object.def_static("logger", &H2Core::Object::logger);
	_Object.def_static("getAliveObjectCount", &H2Core::Object::getAliveObjectCount,
		"Returns Total numbers of objects being alive.");
	// [<TypeDef 'object_map_t'>] _Object.def_static("getObjectMap", &H2Core::Object::getObjectMap,
		// [<TypeDef 'object_map_t'>] "Returns Copy of the object map.");
	// [<TypeDef 'object_map_t'>] _Object.def_static("printObjectMapDiff", &H2Core::Object::printObjectMapDiff,
		// [<TypeDef 'object_map_t'>] "Creates the difference between a snapshot of the object map and its current state and prints it to std::cout.",
	// [<TypeDef 'object_map_t'>] 	py::arg("map"));
	_Object.def("toQString", &H2Core::Object::toQString,
		"Formatted string version for debugging purposes.",
		py::arg("sPrefix"),
		py::arg("bShort"));
	_Object.def("Print", &H2Core::Object::Print,
		"Prints content of toQString() via DEBUGLOG",
		py::arg("bShort"));

	py::class_<H2Core::Logger, std::shared_ptr<H2Core::Logger>> _Logger(m, "Logger");
	_Logger.def_static("bootstrap", &H2Core::Logger::bootstrap,
		"create the logger instance if not exists, set the log level and return the instance",
		py::arg("msk"));
	_Logger.def_static("create_instance", &H2Core::Logger::create_instance,
		"If #__instance equals 0, a new H2Core::Logger singleton will be created and stored in it.");
	_Logger.def_static("get_instance", &H2Core::Logger::get_instance,
		"Returns a pointer to the current H2Core::Logger singleton stored in #__instance.");
	_Logger.def("should_log", &H2Core::Logger::should_log,
		"return true if the level is set in the bitmask",
		py::arg("lvl"));
	_Logger.def_static("set_bit_mask", &H2Core::Logger::set_bit_mask,
		"set the bitmask",
		py::arg("msk"));
	_Logger.def_static("bit_mask", &H2Core::Logger::bit_mask,
		"return the current log level bit mask");
	_Logger.def("set_use_file", &H2Core::Logger::set_use_file,
		"set use file flag",
		py::arg("use"));
	_Logger.def("use_file", &H2Core::Logger::use_file,
		"return __use_file");
	_Logger.def_static("parse_log_level", &H2Core::Logger::parse_log_level,
		"parse a log level string and return the corresponding bit mask",
		py::arg("lvl"));
	_Logger.def("log", &H2Core::Logger::log,
		"the log function",
		py::arg("level"),
		py::arg("class_name"),
		py::arg("func_name"),
		py::arg("msg"));

	py::class_<H2Core::LilyPond, std::shared_ptr<H2Core::LilyPond>> _LilyPond(m, "LilyPond");
	_LilyPond.def(py::init<>());
	_LilyPond.def("extractData", &H2Core::LilyPond::extractData,
		py::arg("song"));
	_LilyPond.def("write", &H2Core::LilyPond::write,
		py::arg("sFilename"));

	py::class_<H2Core::MidiPortInfo, std::shared_ptr<H2Core::MidiPortInfo>> _MidiPortInfo(m, "MidiPortInfo");
	_MidiPortInfo.def_readwrite("m_sName", &H2Core::MidiPortInfo::m_sName);
	_MidiPortInfo.def_readwrite("m_nClient", &H2Core::MidiPortInfo::m_nClient);
	_MidiPortInfo.def_readwrite("m_nPort", &H2Core::MidiPortInfo::m_nPort);

	py::class_<H2Core::MidiMessage, std::shared_ptr<H2Core::MidiMessage>> _MidiMessage(m, "MidiMessage");
	_MidiMessage.def(py::init<>());
	_MidiMessage.def_readwrite("m_type", &H2Core::MidiMessage::m_type);
	_MidiMessage.def_readwrite("m_nData1", &H2Core::MidiMessage::m_nData1);
	_MidiMessage.def_readwrite("m_nData2", &H2Core::MidiMessage::m_nData2);
	_MidiMessage.def_readwrite("m_nChannel", &H2Core::MidiMessage::m_nChannel);
	_MidiMessage.def_readwrite("m_sysexData", &H2Core::MidiMessage::m_sysexData);

	py::class_<std::runtime_error, std::shared_ptr<std::runtime_error>> _runtime_error(m, "runtime_error");
	_runtime_error.def(py::init<const std::string &>());
	_runtime_error.def(py::init<const char *>());
	_runtime_error.def(py::init<const std::runtime_error &>());
	// [banned] _runtime_error.def("operator=", py::overload_cast<std::runtime_error &&>(&std::runtime_error::operator=),
	// [banned] 	py::arg(""));
	_runtime_error.def("operator=", py::overload_cast<const std::runtime_error &>(&std::runtime_error::operator=),
		py::arg(""));
	_runtime_error.def("what", &std::runtime_error::what);

	py::class_<H2Core::Event, std::shared_ptr<H2Core::Event>> _Event(m, "Event");
	_Event.def_readwrite("type", &H2Core::Event::type);
	_Event.def_readwrite("value", &H2Core::Event::value);

	py::class_<H2Core::AudioEngineLocking, std::shared_ptr<H2Core::AudioEngineLocking>> _AudioEngineLocking(m, "AudioEngineLocking");
	_AudioEngineLocking.def(py::init<>());
	_AudioEngineLocking.def("setNeedsLock", &H2Core::AudioEngineLocking::setNeedsLock,
		"The audio processing thread can modify some PatternLists. For these structures, the audio engine lock must be held for any thread to access them.",
		py::arg("bNeedsLock"));

	py::class_<QColor, std::shared_ptr<QColor>> _QColor(m, "QColor");
	_QColor.def(py::init<>());
	_QColor.def(py::init<Qt::GlobalColor>());
	_QColor.def(py::init<int, int, int, int>());
	_QColor.def(py::init<QRgb>());
	_QColor.def(py::init<QRgba64>());
	_QColor.def(py::init<const QString &>());
	_QColor.def(py::init<QStringView>());
	_QColor.def(py::init<const char *>());
	_QColor.def(py::init<QLatin1String>());
	_QColor.def(py::init<QColor::Spec>());
	_QColor.def(py::init<const QColor &>());
	_QColor.def(py::init<QColor::Spec, ushort, ushort, ushort, ushort, ushort>());
	// [banned] _QColor.def("operator=", py::overload_cast<QColor &&>(&QColor::operator=),
	// [banned] 	py::arg("other"));
	// [banned] _QColor.def("operator=", py::overload_cast<const QColor &>(&QColor::operator=),
	// [banned] 	py::arg(""));
	// [banned] _QColor.def("operator=", py::overload_cast<Qt::GlobalColor>(&QColor::operator=),
	// [banned] 	py::arg("color"));
	_QColor.def("isValid", &QColor::isValid);
	_QColor.def("name",
	[](const QColor &color) {
  return color.name();
}
);
	// [banned] _QColor.def("name", py::overload_cast<QColor::NameFormat>(&QColor::name),
	// [banned] 	py::arg("format"));
	_QColor.def("setNamedColor", py::overload_cast<const QString &>(&QColor::setNamedColor),
		py::arg("name"));
	// [<Class 'QStringView'>] _QColor.def("setNamedColor", py::overload_cast<QStringView>(&QColor::setNamedColor),
	// [<Class 'QStringView'>] 	py::arg("name"));
	// [<Class 'QLatin1String'>] _QColor.def("setNamedColor", py::overload_cast<QLatin1String>(&QColor::setNamedColor),
	// [<Class 'QLatin1String'>] 	py::arg("name"));
	// [<Class 'QStringList'>] _QColor.def_static("colorNames", &QColor::colorNames);
	_QColor.def("spec", &QColor::spec);
	_QColor.def("alpha", &QColor::alpha);
	_QColor.def("setAlpha", &QColor::setAlpha,
		py::arg("alpha"));
	_QColor.def("alphaF", &QColor::alphaF);
	_QColor.def("setAlphaF", &QColor::setAlphaF,
		py::arg("alpha"));
	_QColor.def("red", &QColor::red);
	_QColor.def("green", &QColor::green);
	_QColor.def("blue", &QColor::blue);
	_QColor.def("setRed", &QColor::setRed,
		py::arg("red"));
	_QColor.def("setGreen", &QColor::setGreen,
		py::arg("green"));
	_QColor.def("setBlue", &QColor::setBlue,
		py::arg("blue"));
	_QColor.def("redF", &QColor::redF);
	_QColor.def("greenF", &QColor::greenF);
	_QColor.def("blueF", &QColor::blueF);
	_QColor.def("setRedF", &QColor::setRedF,
		py::arg("red"));
	_QColor.def("setGreenF", &QColor::setGreenF,
		py::arg("green"));
	_QColor.def("setBlueF", &QColor::setBlueF,
		py::arg("blue"));
	_QColor.def("getRgb", &QColor::getRgb,
		py::arg("r"),
		py::arg("g"),
		py::arg("b"),
		py::arg("a"));
	_QColor.def("setRgb", py::overload_cast<int, int, int, int>(&QColor::setRgb),
		py::arg("r"),
		py::arg("g"),
		py::arg("b"),
		py::arg("a"));
	_QColor.def("setRgb", py::overload_cast<QRgb>(&QColor::setRgb),
		py::arg("rgb"));
	_QColor.def("getRgbF", &QColor::getRgbF,
		py::arg("r"),
		py::arg("g"),
		py::arg("b"),
		py::arg("a"));
	_QColor.def("setRgbF", &QColor::setRgbF,
		py::arg("r"),
		py::arg("g"),
		py::arg("b"),
		py::arg("a"));
	_QColor.def("rgba64", &QColor::rgba64);
	_QColor.def("setRgba64", &QColor::setRgba64,
		py::arg("rgba"));
	_QColor.def("rgba", &QColor::rgba);
	_QColor.def("setRgba", &QColor::setRgba,
		py::arg("rgba"));
	_QColor.def("rgb", &QColor::rgb);
	_QColor.def("hue", &QColor::hue);
	_QColor.def("saturation", &QColor::saturation);
	_QColor.def("hsvHue", &QColor::hsvHue);
	_QColor.def("hsvSaturation", &QColor::hsvSaturation);
	_QColor.def("value", &QColor::value);
	_QColor.def("hueF", &QColor::hueF);
	_QColor.def("saturationF", &QColor::saturationF);
	_QColor.def("hsvHueF", &QColor::hsvHueF);
	_QColor.def("hsvSaturationF", &QColor::hsvSaturationF);
	_QColor.def("valueF", &QColor::valueF);
	_QColor.def("getHsv", &QColor::getHsv,
		py::arg("h"),
		py::arg("s"),
		py::arg("v"),
		py::arg("a"));
	_QColor.def("setHsv", &QColor::setHsv,
		py::arg("h"),
		py::arg("s"),
		py::arg("v"),
		py::arg("a"));
	_QColor.def("getHsvF", &QColor::getHsvF,
		py::arg("h"),
		py::arg("s"),
		py::arg("v"),
		py::arg("a"));
	_QColor.def("setHsvF", &QColor::setHsvF,
		py::arg("h"),
		py::arg("s"),
		py::arg("v"),
		py::arg("a"));
	_QColor.def("cyan", &QColor::cyan);
	_QColor.def("magenta", &QColor::magenta);
	_QColor.def("yellow", &QColor::yellow);
	_QColor.def("black", &QColor::black);
	_QColor.def("cyanF", &QColor::cyanF);
	_QColor.def("magentaF", &QColor::magentaF);
	_QColor.def("yellowF", &QColor::yellowF);
	_QColor.def("blackF", &QColor::blackF);
	_QColor.def("getCmyk", py::overload_cast<int *, int *, int *, int *, int *>(&QColor::getCmyk),
		py::arg("c"),
		py::arg("m"),
		py::arg("y"),
		py::arg("k"),
		py::arg("a"));
	_QColor.def("getCmyk", py::overload_cast<int *, int *, int *, int *, int *>(&QColor::getCmyk),
		py::arg("c"),
		py::arg("m"),
		py::arg("y"),
		py::arg("k"),
		py::arg("a"));
	_QColor.def("setCmyk", &QColor::setCmyk,
		py::arg("c"),
		py::arg("m"),
		py::arg("y"),
		py::arg("k"),
		py::arg("a"));
	_QColor.def("getCmykF", py::overload_cast<qreal *, qreal *, qreal *, qreal *, qreal *>(&QColor::getCmykF),
		py::arg("c"),
		py::arg("m"),
		py::arg("y"),
		py::arg("k"),
		py::arg("a"));
	_QColor.def("getCmykF", py::overload_cast<qreal *, qreal *, qreal *, qreal *, qreal *>(&QColor::getCmykF),
		py::arg("c"),
		py::arg("m"),
		py::arg("y"),
		py::arg("k"),
		py::arg("a"));
	_QColor.def("setCmykF", &QColor::setCmykF,
		py::arg("c"),
		py::arg("m"),
		py::arg("y"),
		py::arg("k"),
		py::arg("a"));
	_QColor.def("hslHue", &QColor::hslHue);
	_QColor.def("hslSaturation", &QColor::hslSaturation);
	_QColor.def("lightness", &QColor::lightness);
	_QColor.def("hslHueF", &QColor::hslHueF);
	_QColor.def("hslSaturationF", &QColor::hslSaturationF);
	_QColor.def("lightnessF", &QColor::lightnessF);
	_QColor.def("getHsl", &QColor::getHsl,
		py::arg("h"),
		py::arg("s"),
		py::arg("l"),
		py::arg("a"));
	_QColor.def("setHsl", &QColor::setHsl,
		py::arg("h"),
		py::arg("s"),
		py::arg("l"),
		py::arg("a"));
	_QColor.def("getHslF", &QColor::getHslF,
		py::arg("h"),
		py::arg("s"),
		py::arg("l"),
		py::arg("a"));
	_QColor.def("setHslF", &QColor::setHslF,
		py::arg("h"),
		py::arg("s"),
		py::arg("l"),
		py::arg("a"));
	_QColor.def("toRgb", &QColor::toRgb);
	_QColor.def("toHsv", &QColor::toHsv);
	_QColor.def("toCmyk", &QColor::toCmyk);
	_QColor.def("toHsl", &QColor::toHsl);
	_QColor.def("toExtendedRgb", &QColor::toExtendedRgb);
	_QColor.def("convertTo", &QColor::convertTo,
		py::arg("colorSpec"));
	_QColor.def_static("fromRgb_static", py::overload_cast<QRgb>(&QColor::fromRgb),
		py::arg("rgb"));
	_QColor.def_static("fromRgb_static", py::overload_cast<int, int, int, int>(&QColor::fromRgb),
		py::arg("r"),
		py::arg("g"),
		py::arg("b"),
		py::arg("a"));
	_QColor.def_static("fromRgba", &QColor::fromRgba,
		py::arg("rgba"));
	_QColor.def_static("fromRgbF", &QColor::fromRgbF,
		py::arg("r"),
		py::arg("g"),
		py::arg("b"),
		py::arg("a"));
	_QColor.def_static("fromRgba64_static", py::overload_cast<ushort, ushort, ushort, ushort>(&QColor::fromRgba64),
		py::arg("r"),
		py::arg("g"),
		py::arg("b"),
		py::arg("a"));
	_QColor.def_static("fromRgba64_static", py::overload_cast<QRgba64>(&QColor::fromRgba64),
		py::arg("rgba"));
	_QColor.def_static("fromHsv", &QColor::fromHsv,
		py::arg("h"),
		py::arg("s"),
		py::arg("v"),
		py::arg("a"));
	_QColor.def_static("fromHsvF", &QColor::fromHsvF,
		py::arg("h"),
		py::arg("s"),
		py::arg("v"),
		py::arg("a"));
	_QColor.def_static("fromCmyk", &QColor::fromCmyk,
		py::arg("c"),
		py::arg("m"),
		py::arg("y"),
		py::arg("k"),
		py::arg("a"));
	_QColor.def_static("fromCmykF", &QColor::fromCmykF,
		py::arg("c"),
		py::arg("m"),
		py::arg("y"),
		py::arg("k"),
		py::arg("a"));
	_QColor.def_static("fromHsl", &QColor::fromHsl,
		py::arg("h"),
		py::arg("s"),
		py::arg("l"),
		py::arg("a"));
	_QColor.def_static("fromHslF", &QColor::fromHslF,
		py::arg("h"),
		py::arg("s"),
		py::arg("l"),
		py::arg("a"));
	_QColor.def("light", &QColor::light,
		py::arg("f"));
	_QColor.def("dark", &QColor::dark,
		py::arg("f"));
	_QColor.def("lighter", &QColor::lighter,
		py::arg("f"));
	_QColor.def("darker", &QColor::darker,
		py::arg("f"));
	_QColor.def("operator==", &QColor::operator==,
		py::arg("c"));
	_QColor.def("operator!=", &QColor::operator!=,
		py::arg("c"));
	_QColor.def_static("isValidColor_static", py::overload_cast<const QString &>(&QColor::isValidColor),
		py::arg("name"));
	// [<Class 'QStringView'>] _QColor.def_static("isValidColor_static", py::overload_cast<QStringView>(&QColor::isValidColor),
	// [<Class 'QStringView'>] 	py::arg(""));
	// [<Class 'QLatin1String'>] _QColor.def_static("isValidColor_static", py::overload_cast<QLatin1String>(&QColor::isValidColor),
	// [<Class 'QLatin1String'>] 	py::arg(""));
	_QColor.def("__repr__",
	[](const QColor &color) {
  return "QColor(\"" + color.name() + "\")";
}
);

	py::class_<QRgba64, std::shared_ptr<QRgba64>> _QRgba64(m, "QRgba64");
	_QRgba64.def(py::init<>());
	_QRgba64.def_static("fromRgba64_static", py::overload_cast<quint64>(&QRgba64::fromRgba64),
		py::arg("c"));
	_QRgba64.def_static("fromRgba64_static", py::overload_cast<quint16, quint16, quint16, quint16>(&QRgba64::fromRgba64),
		py::arg("red"),
		py::arg("green"),
		py::arg("blue"),
		py::arg("alpha"));
	_QRgba64.def_static("fromRgba", &QRgba64::fromRgba,
		py::arg("red"),
		py::arg("green"),
		py::arg("blue"),
		py::arg("alpha"));
	_QRgba64.def_static("fromArgb32", &QRgba64::fromArgb32,
		py::arg("rgb"));
	_QRgba64.def("isOpaque", &QRgba64::isOpaque);
	_QRgba64.def("isTransparent", &QRgba64::isTransparent);
	_QRgba64.def("red", &QRgba64::red);
	_QRgba64.def("green", &QRgba64::green);
	_QRgba64.def("blue", &QRgba64::blue);
	_QRgba64.def("alpha", &QRgba64::alpha);
	_QRgba64.def("setRed", &QRgba64::setRed,
		py::arg("_red"));
	_QRgba64.def("setGreen", &QRgba64::setGreen,
		py::arg("_green"));
	_QRgba64.def("setBlue", &QRgba64::setBlue,
		py::arg("_blue"));
	_QRgba64.def("setAlpha", &QRgba64::setAlpha,
		py::arg("_alpha"));
	_QRgba64.def("red8", &QRgba64::red8);
	_QRgba64.def("green8", &QRgba64::green8);
	_QRgba64.def("blue8", &QRgba64::blue8);
	_QRgba64.def("alpha8", &QRgba64::alpha8);
	_QRgba64.def("toArgb32", &QRgba64::toArgb32);
	_QRgba64.def("toRgb16", &QRgba64::toRgb16);
	_QRgba64.def("premultiplied", &QRgba64::premultiplied);
	_QRgba64.def("unpremultiplied", &QRgba64::unpremultiplied);
	// [banned] _QRgba64.def("operator=", &QRgba64::operator=,
	// [banned] 	py::arg("_rgba"));

	py::class_<H2Core::Synth, H2Core::Object, std::shared_ptr<H2Core::Synth>> _Synth(m, "Synth");
	_Synth.def(py::init<>());
	_Synth.def_readwrite("m_pOut_L", &H2Core::Synth::m_pOut_L);
	_Synth.def_readwrite("m_pOut_R", &H2Core::Synth::m_pOut_R);
	_Synth.def_static("class_name", &H2Core::Synth::class_name);
	_Synth.def("noteOn", &H2Core::Synth::noteOn,
		"Start playing a note",
		py::arg("pNote"));
	_Synth.def("noteOff", &H2Core::Synth::noteOff,
		"Stop playing a note.",
		py::arg("pNote"));
	_Synth.def("process", &H2Core::Synth::process,
		py::arg("nFrames"));
	_Synth.def("setAudioOutput", &H2Core::Synth::setAudioOutput,
		py::arg("pAudioOutput"));
	_Synth.def("getPlayingNotesNumber", &H2Core::Synth::getPlayingNotesNumber);

	py::class_<H2Core::Note, H2Core::Object, std::shared_ptr<H2Core::Note>> _Note(m, "Note");
	_Note.def(py::init<std::shared_ptr<Instrument>, int, float, float, int, float>());
	_Note.def(py::init<H2Core::Note *, std::shared_ptr<Instrument>>());
	_Note.def_static("class_name", &H2Core::Note::class_name);
	_Note.def("save_to", &H2Core::Note::save_to,
		py::arg("node"));
	_Note.def_static("load_from", &H2Core::Note::load_from,
		"load a note from an XMLNode",
		py::arg("node"),
		py::arg("instruments"));
	_Note.def("dump", &H2Core::Note::dump,
		"output details through logger with DEBUG severity");
	_Note.def("map_instrument", &H2Core::Note::map_instrument,
		"find the corresponding instrument and point to it, or an empty instrument",
		py::arg("instruments"));
	_Note.def("get_instrument", &H2Core::Note::get_instrument,
		"#__instrument accessor");
	_Note.def("has_instrument", &H2Core::Note::has_instrument,
		"return true if #__instrument is set");
	_Note.def("set_instrument_id", &H2Core::Note::set_instrument_id,
		"#__instrument_id setter",
		py::arg("value"));
	_Note.def("get_instrument_id", &H2Core::Note::get_instrument_id,
		"#__instrument_id accessor");
	_Note.def("set_specific_compo_id", &H2Core::Note::set_specific_compo_id,
		"#__specific_compo_id setter",
		py::arg("value"));
	_Note.def("get_specific_compo_id", &H2Core::Note::get_specific_compo_id,
		"#__specific_compo_id accessor");
	_Note.def("set_position", &H2Core::Note::set_position,
		"#__position setter",
		py::arg("value"));
	_Note.def("get_position", &H2Core::Note::get_position,
		"#__position accessor");
	_Note.def("set_velocity", &H2Core::Note::set_velocity,
		"#__velocity setter",
		py::arg("value"));
	_Note.def("get_velocity", &H2Core::Note::get_velocity,
		"#__velocity accessor");
	_Note.def("setPan", &H2Core::Note::setPan,
		"set pan of the note. assumes the input range in [-1;1]",
		py::arg("val"));
	_Note.def("setPanWithRangeFrom0To1", &H2Core::Note::setPanWithRangeFrom0To1,
		"set pan of the note, assuming the input range in [0;1]",
		py::arg("fVal"));
	_Note.def("getPan", &H2Core::Note::getPan,
		"get pan of the note. Output pan range: [-1;1]");
	_Note.def("getPanWithRangeFrom0To1", &H2Core::Note::getPanWithRangeFrom0To1,
		"get pan of the note, scaling and translating the range from [-1;1] to [0;1]");
	_Note.def("set_lead_lag", &H2Core::Note::set_lead_lag,
		"#__lead_lag setter",
		py::arg("value"));
	_Note.def("get_lead_lag", &H2Core::Note::get_lead_lag,
		"#__lead_lag accessor");
	_Note.def("set_length", &H2Core::Note::set_length,
		"#__length setter",
		py::arg("value"));
	_Note.def("get_length", &H2Core::Note::get_length,
		"#__length accessor");
	_Note.def("set_pitch", &H2Core::Note::set_pitch,
		"#__pitch setter",
		py::arg("value"));
	_Note.def("get_pitch", &H2Core::Note::get_pitch,
		"#__pitch accessor");
	_Note.def("set_note_off", &H2Core::Note::set_note_off,
		"#__note_off setter",
		py::arg("value"));
	_Note.def("get_note_off", &H2Core::Note::get_note_off,
		"#__note_off accessor");
	_Note.def("get_midi_msg", &H2Core::Note::get_midi_msg,
		"#__midi_msg accessor");
	_Note.def("set_pattern_idx", &H2Core::Note::set_pattern_idx,
		"#__pattern_idx setter",
		py::arg("value"));
	_Note.def("get_pattern_idx", &H2Core::Note::get_pattern_idx,
		"#__pattern_idx accessor");
	_Note.def("set_just_recorded", &H2Core::Note::set_just_recorded,
		"#__just_recorded setter",
		py::arg("value"));
	_Note.def("get_just_recorded", &H2Core::Note::get_just_recorded,
		"#__just_recorded accessor");
	_Note.def("get_layer_selected", &H2Core::Note::get_layer_selected,
		py::arg("CompoID"));
	_Note.def("set_probability", &H2Core::Note::set_probability,
		py::arg("value"));
	_Note.def("get_probability", &H2Core::Note::get_probability);
	_Note.def("set_humanize_delay", &H2Core::Note::set_humanize_delay,
		"#__humanize_delay setter",
		py::arg("value"));
	_Note.def("get_humanize_delay", &H2Core::Note::get_humanize_delay,
		"#__humanize_delay accessor");
	_Note.def("get_cut_off", &H2Core::Note::get_cut_off,
		"#__cut_off accessor");
	_Note.def("get_resonance", &H2Core::Note::get_resonance,
		"#__resonance accessor");
	_Note.def("get_bpfb_l", &H2Core::Note::get_bpfb_l,
		"#__bpfb_l accessor");
	_Note.def("get_bpfb_r", &H2Core::Note::get_bpfb_r,
		"#__bpfb_r accessor");
	_Note.def("get_lpfb_l", &H2Core::Note::get_lpfb_l,
		"#__lpfb_l accessor");
	_Note.def("get_lpfb_r", &H2Core::Note::get_lpfb_r,
		"#__lpfb_r accessor");
	_Note.def("get_key", &H2Core::Note::get_key,
		"#__key accessor");
	_Note.def("get_octave", &H2Core::Note::get_octave,
		"#__octave accessor");
	_Note.def("get_midi_key", &H2Core::Note::get_midi_key,
		"return scaled key for midi output, !!! DO NOT CHECK IF INSTRUMENT IS SET !!!");
	_Note.def("get_midi_velocity", &H2Core::Note::get_midi_velocity,
		"midi velocity accessor");
	_Note.def("get_notekey_pitch", &H2Core::Note::get_notekey_pitch,
		"note key pitch accessor");
	_Note.def("get_total_pitch", &H2Core::Note::get_total_pitch,
		"returns");
	_Note.def("key_to_string", &H2Core::Note::key_to_string,
		"return a string representation of key-octave");
	_Note.def("set_key_octave", py::overload_cast<const QString &>(&H2Core::Note::set_key_octave),
		"parse str and set #__key and #__octave",
		py::arg("str"));
	_Note.def("set_key_octave", py::overload_cast<H2Core::Note::Key, H2Core::Note::Octave>(&H2Core::Note::set_key_octave),
		"set #__key and #__octave only if within acceptable range",
		py::arg("key"),
		py::arg("octave"));
	_Note.def("set_midi_info", &H2Core::Note::set_midi_info,
		"set #__key, #__octave and #__midi_msg only if within acceptable range",
		py::arg("key"),
		py::arg("octave"),
		py::arg("msg"));
	_Note.def("get_adsr", &H2Core::Note::get_adsr,
		"get the ADSR of the note");
	// [banned] _Note.def("match", py::overload_cast<std::shared_ptr<Instrument>, H2Core::Note::Key, H2Core::Note::Octave>(&H2Core::Note::match),
		// [banned] "return true if instrument, key and octave matches with internal",
	// [banned] 	py::arg("instrument"),
	// [banned] 	py::arg("key"),
	// [banned] 	py::arg("octave"));
	// [banned] _Note.def("match", py::overload_cast<const H2Core::Note *>(&H2Core::Note::match),
		// [banned] "Return true if two notes match in instrument, key and octave.",
	// [banned] 	py::arg("pNote"));
	_Note.def("compute_lr_values", &H2Core::Note::compute_lr_values,
		"compute left and right output based on filters",
		py::arg("val_l"),
		py::arg("val_r"));
	_Note.def("toQString", &H2Core::Note::toQString,
		"Formatted string version for debugging purposes.",
		py::arg("sPrefix"),
		py::arg("bShort"));

	py::class_<H2Core::Sampler, H2Core::Object, std::shared_ptr<H2Core::Sampler>> _Sampler(m, "Sampler");
	_Sampler.def(py::init<>());
	_Sampler.def_readwrite("m_pMainOut_L", &H2Core::Sampler::m_pMainOut_L);
	_Sampler.def_readwrite("m_pMainOut_R", &H2Core::Sampler::m_pMainOut_R);
	_Sampler.def_static("class_name", &H2Core::Sampler::class_name);
	_Sampler.def_static("ratioStraightPolygonalPanLaw", &H2Core::Sampler::ratioStraightPolygonalPanLaw,
		py::arg("fPan"));
	_Sampler.def_static("ratioConstPowerPanLaw", &H2Core::Sampler::ratioConstPowerPanLaw,
		py::arg("fPan"));
	_Sampler.def_static("ratioConstSumPanLaw", &H2Core::Sampler::ratioConstSumPanLaw,
		py::arg("fPan"));
	_Sampler.def_static("linearStraightPolygonalPanLaw", &H2Core::Sampler::linearStraightPolygonalPanLaw,
		py::arg("fPan"));
	_Sampler.def_static("linearConstPowerPanLaw", &H2Core::Sampler::linearConstPowerPanLaw,
		py::arg("fPan"));
	_Sampler.def_static("linearConstSumPanLaw", &H2Core::Sampler::linearConstSumPanLaw,
		py::arg("fPan"));
	_Sampler.def_static("polarStraightPolygonalPanLaw", &H2Core::Sampler::polarStraightPolygonalPanLaw,
		py::arg("fPan"));
	_Sampler.def_static("polarConstPowerPanLaw", &H2Core::Sampler::polarConstPowerPanLaw,
		py::arg("fPan"));
	_Sampler.def_static("polarConstSumPanLaw", &H2Core::Sampler::polarConstSumPanLaw,
		py::arg("fPan"));
	_Sampler.def_static("quadraticStraightPolygonalPanLaw", &H2Core::Sampler::quadraticStraightPolygonalPanLaw,
		py::arg("fPan"));
	_Sampler.def_static("quadraticConstPowerPanLaw", &H2Core::Sampler::quadraticConstPowerPanLaw,
		py::arg("fPan"));
	_Sampler.def_static("quadraticConstSumPanLaw", &H2Core::Sampler::quadraticConstSumPanLaw,
		py::arg("fPan"));
	_Sampler.def_static("linearConstKNormPanLaw", &H2Core::Sampler::linearConstKNormPanLaw,
		py::arg("fPan"),
		py::arg("k"));
	_Sampler.def_static("polarConstKNormPanLaw", &H2Core::Sampler::polarConstKNormPanLaw,
		py::arg("fPan"),
		py::arg("k"));
	_Sampler.def_static("ratioConstKNormPanLaw", &H2Core::Sampler::ratioConstKNormPanLaw,
		py::arg("fPan"),
		py::arg("k"));
	_Sampler.def_static("quadraticConstKNormPanLaw", &H2Core::Sampler::quadraticConstKNormPanLaw,
		py::arg("fPan"),
		py::arg("k"));
	_Sampler.def_static("getRatioPan", &H2Core::Sampler::getRatioPan,
		"This function is used to load old version files (v<=1.1). It returns the single pan parameter in [-1,1] from the L,R gains as it was input from the GUI (up to scale and translation, which is arbitrary). Default output is 0 (=central pan) if arguments are invalid. -----Historical Note----- Originally (version <= 1.0) pan_L,pan_R were actually gains for each channel; \"instrument\" and \"note\" pans were multiplied as in a gain CHAIN in each separate channel, so the chain killed the signal if instrument and note pans were hard-sided to opposites sides! In v1.1, pan_L and pan_R were still the members of Note/Instrument representing the pan knob position, still using the ratioStraightPolygonalPanLaw() for the correspondence (up to constant multiplication), but pan_L,pan_R were reconverted to single parameter in the Sampler, and fPan was used in the selected pan law.",
		py::arg("fPan_L"),
		py::arg("fPan_R"));
	_Sampler.def("process", &H2Core::Sampler::process,
		py::arg("nFrames"),
		py::arg("pSong"));
	_Sampler.def("noteOn", &H2Core::Sampler::noteOn,
		"Start playing a note",
		py::arg("pNote"));
	_Sampler.def("noteOff", &H2Core::Sampler::noteOff,
		"Stop playing a note.",
		py::arg("pNote"));
	_Sampler.def("midiKeyboardNoteOff", &H2Core::Sampler::midiKeyboardNoteOff,
		py::arg("key"));
	_Sampler.def("stopPlayingNotes", &H2Core::Sampler::stopPlayingNotes,
		py::arg("pInstr"));
	_Sampler.def("getPlayingNotesNumber", &H2Core::Sampler::getPlayingNotesNumber);
	_Sampler.def("preview_sample", &H2Core::Sampler::preview_sample,
		py::arg("pSample"),
		py::arg("length"));
	_Sampler.def("preview_instrument", &H2Core::Sampler::preview_instrument,
		py::arg("pInstr"));
	_Sampler.def("setPlayingNotelength", &H2Core::Sampler::setPlayingNotelength,
		py::arg("pInstrument"),
		py::arg("ticks"),
		py::arg("noteOnTick"));
	_Sampler.def("isInstrumentPlaying", &H2Core::Sampler::isInstrumentPlaying,
		py::arg("pInstr"));
	_Sampler.def("setInterpolateMode", &H2Core::Sampler::setInterpolateMode,
		py::arg("mode"));
	_Sampler.def("getPreviewInstrument", &H2Core::Sampler::getPreviewInstrument);
	_Sampler.def("getPlaybackTrackInstrument", &H2Core::Sampler::getPlaybackTrackInstrument);
	_Sampler.def("getInterpolateMode", &H2Core::Sampler::getInterpolateMode);
	_Sampler.def("reinitializePlaybackTrack", &H2Core::Sampler::reinitializePlaybackTrack,
		"Loading of the playback track.");

	py::class_<H2Core::TransportInfo, H2Core::Object, std::shared_ptr<H2Core::TransportInfo>> _TransportInfo(m, "TransportInfo");
	_TransportInfo.def(py::init<>());
	_TransportInfo.def_readwrite("m_status", &H2Core::TransportInfo::m_status);
	_TransportInfo.def_readwrite("m_nFrames", &H2Core::TransportInfo::m_nFrames);
	_TransportInfo.def_readwrite("m_fTickSize", &H2Core::TransportInfo::m_fTickSize);
	_TransportInfo.def_readwrite("m_fBPM", &H2Core::TransportInfo::m_fBPM);
	_TransportInfo.def_static("class_name", &H2Core::TransportInfo::class_name);
	_TransportInfo.def("printInfo", &H2Core::TransportInfo::printInfo,
		"Displays general information about the transport state in the #INFOLOG");

// abstract class MidiOutput

// abstract class MidiInput

// abstract class AudioOutput

	py::class_<H2Core::LadspaFX, H2Core::Object, std::shared_ptr<H2Core::LadspaFX>> _LadspaFX(m, "LadspaFX");
	_LadspaFX.def_readwrite("m_pBuffer_L", &H2Core::LadspaFX::m_pBuffer_L);
	_LadspaFX.def_readwrite("m_pBuffer_R", &H2Core::LadspaFX::m_pBuffer_R);
	_LadspaFX.def_readwrite("inputControlPorts", &H2Core::LadspaFX::inputControlPorts);
	_LadspaFX.def_readwrite("outputControlPorts", &H2Core::LadspaFX::outputControlPorts);
	_LadspaFX.def_static("class_name", &H2Core::LadspaFX::class_name);
	_LadspaFX.def("connectAudioPorts", &H2Core::LadspaFX::connectAudioPorts,
		py::arg("pIn_L"),
		py::arg("pIn_R"),
		py::arg("pOut_L"),
		py::arg("pOut_R"));
	_LadspaFX.def("activate", &H2Core::LadspaFX::activate);
	_LadspaFX.def("deactivate", &H2Core::LadspaFX::deactivate);
	_LadspaFX.def("processFX", &H2Core::LadspaFX::processFX,
		py::arg("nFrames"));
	_LadspaFX.def("getPluginLabel", &H2Core::LadspaFX::getPluginLabel);
	_LadspaFX.def("getPluginName", &H2Core::LadspaFX::getPluginName);
	_LadspaFX.def("setPluginName", &H2Core::LadspaFX::setPluginName,
		py::arg("sName"));
	_LadspaFX.def("getLibraryPath", &H2Core::LadspaFX::getLibraryPath);
	_LadspaFX.def("isEnabled", &H2Core::LadspaFX::isEnabled);
	_LadspaFX.def("setEnabled", &H2Core::LadspaFX::setEnabled,
		py::arg("value"));
	_LadspaFX.def_static("load", &H2Core::LadspaFX::load,
		py::arg("sLibraryPath"),
		py::arg("sPluginLabel"),
		py::arg("nSampleRate"));
	_LadspaFX.def("getPluginType", &H2Core::LadspaFX::getPluginType);
	_LadspaFX.def("setVolume", &H2Core::LadspaFX::setVolume,
		py::arg("fValue"));
	_LadspaFX.def("getVolume", &H2Core::LadspaFX::getVolume);

	py::class_<H2Core::Effects, H2Core::Object, std::shared_ptr<H2Core::Effects>> _Effects(m, "Effects");
	_Effects.def_static("class_name", &H2Core::Effects::class_name);
	_Effects.def_static("create_instance", &H2Core::Effects::create_instance,
		"If #__instance equals 0, a new Effects singleton will be created and stored in it.");
	_Effects.def_static("get_instance", &H2Core::Effects::get_instance,
		"Returns a pointer to the current Effects singleton stored in #__instance.");
	_Effects.def("getLadspaFX", &H2Core::Effects::getLadspaFX,
		py::arg("nFX"));
	_Effects.def("setLadspaFX", &H2Core::Effects::setLadspaFX,
		py::arg("pFX"),
		py::arg("nFX"));
	_Effects.def("getPluginList", &H2Core::Effects::getPluginList);
	_Effects.def("getLadspaFXGroup", &H2Core::Effects::getLadspaFXGroup);

	py::class_<H2Core::LadspaFXGroup, H2Core::Object, std::shared_ptr<H2Core::LadspaFXGroup>> _LadspaFXGroup(m, "LadspaFXGroup");
	_LadspaFXGroup.def(py::init<const QString &>());
	_LadspaFXGroup.def_static("class_name", &H2Core::LadspaFXGroup::class_name);
	_LadspaFXGroup.def("getName", &H2Core::LadspaFXGroup::getName);
	_LadspaFXGroup.def("addLadspaInfo", &H2Core::LadspaFXGroup::addLadspaInfo,
		py::arg("pInfo"));
	_LadspaFXGroup.def("getLadspaInfo", &H2Core::LadspaFXGroup::getLadspaInfo);
	_LadspaFXGroup.def("addChild", &H2Core::LadspaFXGroup::addChild,
		py::arg("pChild"));
	_LadspaFXGroup.def("getChildList", &H2Core::LadspaFXGroup::getChildList);
	_LadspaFXGroup.def("clear", &H2Core::LadspaFXGroup::clear);
	_LadspaFXGroup.def_static("alphabeticOrder", &H2Core::LadspaFXGroup::alphabeticOrder,
		py::arg(""),
		py::arg(""));
	_LadspaFXGroup.def("sort", &H2Core::LadspaFXGroup::sort);

	py::class_<H2Core::LadspaFXInfo, H2Core::Object, std::shared_ptr<H2Core::LadspaFXInfo>> _LadspaFXInfo(m, "LadspaFXInfo");
	_LadspaFXInfo.def(py::init<const QString &>());
	_LadspaFXInfo.def_readwrite("m_sFilename", &H2Core::LadspaFXInfo::m_sFilename);
	_LadspaFXInfo.def_readwrite("m_sID", &H2Core::LadspaFXInfo::m_sID);
	_LadspaFXInfo.def_readwrite("m_sLabel", &H2Core::LadspaFXInfo::m_sLabel);
	_LadspaFXInfo.def_readwrite("m_sName", &H2Core::LadspaFXInfo::m_sName);
	_LadspaFXInfo.def_readwrite("m_sMaker", &H2Core::LadspaFXInfo::m_sMaker);
	_LadspaFXInfo.def_readwrite("m_sCopyright", &H2Core::LadspaFXInfo::m_sCopyright);
	_LadspaFXInfo.def_readwrite("m_nICPorts", &H2Core::LadspaFXInfo::m_nICPorts);
	_LadspaFXInfo.def_readwrite("m_nOCPorts", &H2Core::LadspaFXInfo::m_nOCPorts);
	_LadspaFXInfo.def_readwrite("m_nIAPorts", &H2Core::LadspaFXInfo::m_nIAPorts);
	_LadspaFXInfo.def_readwrite("m_nOAPorts", &H2Core::LadspaFXInfo::m_nOAPorts);
	_LadspaFXInfo.def_static("class_name", &H2Core::LadspaFXInfo::class_name);
	_LadspaFXInfo.def_static("alphabeticOrder", &H2Core::LadspaFXInfo::alphabeticOrder,
		py::arg("a"),
		py::arg("b"));

	py::class_<H2Core::Timeline, H2Core::Object, std::shared_ptr<H2Core::Timeline>> _Timeline(m, "Timeline");
	_Timeline.def(py::init<>());
	_Timeline.def_static("class_name", &H2Core::Timeline::class_name);
	_Timeline.def("addTempoMarker", &H2Core::Timeline::addTempoMarker,
		py::arg("nBar"),
		py::arg("fBpm"));
	_Timeline.def("deleteTempoMarker", &H2Core::Timeline::deleteTempoMarker,
		py::arg("nBar"));
	_Timeline.def("deleteAllTempoMarkers", &H2Core::Timeline::deleteAllTempoMarkers);
	_Timeline.def("getTempoAtBar", &H2Core::Timeline::getTempoAtBar,
		"Returns the tempo of the Song at a given bar.",
		py::arg("nBar"),
		py::arg("bSticky"));
	_Timeline.def("getAllTempoMarkers", &H2Core::Timeline::getAllTempoMarkers,
		"Returns std::vector<std::shared_ptr<const TempoMarker>> Provides read-only access to m_tempoMarker.");
	_Timeline.def("addTag", &H2Core::Timeline::addTag,
		py::arg("nBar"),
		py::arg("sTag"));
	_Timeline.def("deleteTag", &H2Core::Timeline::deleteTag,
		py::arg("nBar"));
	_Timeline.def("deleteAllTags", &H2Core::Timeline::deleteAllTags);
	_Timeline.def("getTagAtBar", &H2Core::Timeline::getTagAtBar,
		"Returns the tag of the Song at a given bar.",
		py::arg("nBar"),
		py::arg("bSticky"));
	_Timeline.def("getAllTags", &H2Core::Timeline::getAllTags,
		"Returns std::vector<std::shared_ptr<const Tag>> Provides read-only access to m_tags.");
	_Timeline.def("toQString", &H2Core::Timeline::toQString,
		"Formatted string version for debugging purposes.",
		py::arg("sPrefix"),
		py::arg("bShort"));

	py::class_<MidiActionManager, H2Core::Object, std::shared_ptr<MidiActionManager>> _MidiActionManager(m, "MidiActionManager");
	_MidiActionManager.def(py::init<>());
	_MidiActionManager.def_static("class_name", &MidiActionManager::class_name);
	_MidiActionManager.def("handleAction", &MidiActionManager::handleAction,
		"The handleAction method is the heart of the MidiActionManager class. It executes the operations that are needed to carry the desired action.",
		py::arg(""));
	_MidiActionManager.def_static("create_instance", &MidiActionManager::create_instance,
		"If #__instance equals 0, a new MidiActionManager singleton will be created and stored in it.");
	_MidiActionManager.def_static("get_instance", &MidiActionManager::get_instance,
		"Returns a pointer to the current MidiActionManager singleton stored in #__instance.");
	// [<Class 'QStringList'>] _MidiActionManager.def("getActionList", &MidiActionManager::getActionList);
	// [<Class 'QStringList'>] _MidiActionManager.def("getEventList", &MidiActionManager::getEventList);

	py::class_<Action, H2Core::Object, std::shared_ptr<Action>> _Action(m, "Action");
	_Action.def(py::init<QString>());
	_Action.def_static("class_name", &Action::class_name);
	_Action.def("setParameter1", &Action::setParameter1,
		py::arg("text"));
	_Action.def("setParameter2", &Action::setParameter2,
		py::arg("text"));
	_Action.def("getParameter1", &Action::getParameter1);
	_Action.def("getParameter2", &Action::getParameter2);
	_Action.def("getType", &Action::getType);

	py::class_<H2Core::EventQueue, H2Core::Object, std::shared_ptr<H2Core::EventQueue>> _EventQueue(m, "EventQueue");
	_EventQueue.def_readwrite("m_addMidiNoteVector", &H2Core::EventQueue::m_addMidiNoteVector);
	_EventQueue.def_static("class_name", &H2Core::EventQueue::class_name);
	_EventQueue.def_static("create_instance", &H2Core::EventQueue::create_instance,
		"If #__instance equals 0, a new EventQueue singleton will be created and stored in it.");
	_EventQueue.def_static("get_instance", &H2Core::EventQueue::get_instance,
		"Returns a pointer to the current EventQueue singleton stored in #__instance.");
	_EventQueue.def("push_event", &H2Core::EventQueue::push_event,
		"Queues the next event into the EventQueue.",
		py::arg("type"),
		py::arg("nValue"));
	_EventQueue.def("pop_event", &H2Core::EventQueue::pop_event,
		"Reads out the next event of the EventQueue.");

	py::class_<H2Core::CoreActionController, H2Core::Object, std::shared_ptr<H2Core::CoreActionController>> _CoreActionController(m, "CoreActionController");
	_CoreActionController.def(py::init<>());
	_CoreActionController.def_static("class_name", &H2Core::CoreActionController::class_name);
	_CoreActionController.def("setMasterVolume", &H2Core::CoreActionController::setMasterVolume,
		py::arg("masterVolumeValue"));
	_CoreActionController.def("setStripVolume", &H2Core::CoreActionController::setStripVolume,
		py::arg("nStrip"),
		py::arg("fVolumeValue"),
		py::arg("bSelectStrip"));
	_CoreActionController.def("setStripPan", &H2Core::CoreActionController::setStripPan,
		py::arg("nStrip"),
		py::arg("fValue"),
		py::arg("bSelectStrip"));
	_CoreActionController.def("setStripPanSym", &H2Core::CoreActionController::setStripPanSym,
		py::arg("nStrip"),
		py::arg("fValue"),
		py::arg("bSelectStrip"));
	_CoreActionController.def("setMetronomeIsActive", &H2Core::CoreActionController::setMetronomeIsActive,
		py::arg("isActive"));
	_CoreActionController.def("setMasterIsMuted", &H2Core::CoreActionController::setMasterIsMuted,
		py::arg("isMuted"));
	_CoreActionController.def("setStripIsMuted", &H2Core::CoreActionController::setStripIsMuted,
		py::arg("nStrip"),
		py::arg("isMuted"));
	_CoreActionController.def("toggleStripIsMuted", &H2Core::CoreActionController::toggleStripIsMuted,
		py::arg("nStrip"));
	_CoreActionController.def("setStripIsSoloed", &H2Core::CoreActionController::setStripIsSoloed,
		py::arg("nStrip"),
		py::arg("isSoloed"));
	_CoreActionController.def("toggleStripIsSoloed", &H2Core::CoreActionController::toggleStripIsSoloed,
		py::arg("nStrip"));
	_CoreActionController.def("initExternalControlInterfaces", &H2Core::CoreActionController::initExternalControlInterfaces);
	_CoreActionController.def("handleOutgoingControlChange", &H2Core::CoreActionController::handleOutgoingControlChange,
		py::arg("param"),
		py::arg("value"));
	_CoreActionController.def("newSong", &H2Core::CoreActionController::newSong,
		"Create an empty #Song, which will be stored in songPath.",
		py::arg("songPath"));
	_CoreActionController.def("openSong", py::overload_cast<const QString &>(&H2Core::CoreActionController::openSong),
		"Opens the #Song specified in songPath.",
		py::arg("songPath"));
	_CoreActionController.def("openSong", py::overload_cast<H2Core::Song *>(&H2Core::CoreActionController::openSong),
		"Opens the #Song specified in songPath.",
		py::arg("pSong"));
	_CoreActionController.def("saveSong", &H2Core::CoreActionController::saveSong,
		"Saves the current #Song.");
	_CoreActionController.def("saveSongAs", &H2Core::CoreActionController::saveSongAs,
		"Saves the current #Song to the path provided in songPath.",
		py::arg("songPath"));
	_CoreActionController.def("savePreferences", &H2Core::CoreActionController::savePreferences,
		"Saves the current state of the #Preferences.");
	_CoreActionController.def("quit", &H2Core::CoreActionController::quit,
		"Triggers the shutdown of Hydrogen.");
	_CoreActionController.def("activateTimeline", &H2Core::CoreActionController::activateTimeline,
		"(De)activates the usage of the Timeline.",
		py::arg("bActivate"));
	_CoreActionController.def("addTempoMarker", &H2Core::CoreActionController::addTempoMarker,
		"Adds a tempo marker to the Timeline.",
		py::arg("nPosition"),
		py::arg("fBpm"));
	_CoreActionController.def("deleteTempoMarker", &H2Core::CoreActionController::deleteTempoMarker,
		"Delete a tempo marker from the Timeline.",
		py::arg("nPosition"));
	_CoreActionController.def("activateJackTransport", &H2Core::CoreActionController::activateJackTransport,
		"(De)activates the usage of Jack transport.",
		py::arg("bActivate"));
	_CoreActionController.def("activateJackTimebaseMaster", &H2Core::CoreActionController::activateJackTimebaseMaster,
		"(De)activates the usage of Jack timebase master.",
		py::arg("bActivate"));
	_CoreActionController.def("activateSongMode", &H2Core::CoreActionController::activateSongMode,
		"Switches between Song and Pattern mode of playback.",
		py::arg("bActivate"),
		py::arg("bTriggerEvent"));
	_CoreActionController.def("activateLoopMode", &H2Core::CoreActionController::activateLoopMode,
		"Toggle loop mode of playback.",
		py::arg("bActivate"),
		py::arg("bTriggerEvent"));
	_CoreActionController.def("relocate", &H2Core::CoreActionController::relocate,
		"Relocates transport to the beginning of a particular Pattern.",
		py::arg("nPatternGroup"));
	_CoreActionController.def("isSongPathValid", &H2Core::CoreActionController::isSongPathValid,
		"Checks the path of the .h2song provided via OSC.",
		py::arg("songPath"));

	py::class_<H2Core::AudioEngine, H2Core::Object, std::shared_ptr<H2Core::AudioEngine>> _AudioEngine(m, "AudioEngine");
	_AudioEngine.def(py::init<>());
	_AudioEngine.def_static("class_name", &H2Core::AudioEngine::class_name);
	_AudioEngine.def("lock", &H2Core::AudioEngine::lock,
		"Mutex locking of the AudioEngine.",
		py::arg("file"),
		py::arg("line"),
		py::arg("function"));
	_AudioEngine.def("tryLock", &H2Core::AudioEngine::tryLock,
		"Mutex locking of the AudioEngine.",
		py::arg("file"),
		py::arg("line"),
		py::arg("function"));
	// [<TemplateRef 'duration'>] _AudioEngine.def("tryLockFor", &H2Core::AudioEngine::tryLockFor,
		// [<TemplateRef 'duration'>] "Mutex locking of the AudioEngine.",
	// [<TemplateRef 'duration'>] 	py::arg("duration"),
	// [<TemplateRef 'duration'>] 	py::arg("file"),
	// [<TemplateRef 'duration'>] 	py::arg("line"),
	// [<TemplateRef 'duration'>] 	py::arg("function"));
	_AudioEngine.def("unlock", &H2Core::AudioEngine::unlock,
		"Mutex unlocking of the AudioEngine.");
	_AudioEngine.def("assertLocked", &H2Core::AudioEngine::assertLocked,
		"Assert that the calling thread is the current holder of the AudioEngine lock.");
	_AudioEngine.def("destroy", &H2Core::AudioEngine::destroy);
	_AudioEngine.def("start", &H2Core::AudioEngine::start,
		"If the audio engine is in state #m_audioEngineState #STATE_READY, this function will - sets #m_fMasterPeak_L and #m_fMasterPeak_R to 0.0f - sets TransportInfo::m_nFrames to nTotalFrames - sets m_nSongPos and m_nPatternStartTick to -1 - m_nPatternTickPosition to 0 - sets #m_audioEngineState to #STATE_PLAYING - pushes the #EVENT_STATE #STATE_PLAYING using EventQueue::push_event()",
		py::arg("bLockEngine"),
		py::arg("nTotalFrames"));
	_AudioEngine.def("stop", &H2Core::AudioEngine::stop,
		"If the audio engine is in state #m_audioEngineState #STATE_PLAYING, this function will - sets #m_fMasterPeak_L and #m_fMasterPeak_R to 0.0f - sets #m_audioEngineState to #STATE_READY - sets #m_nPatternStartTick to -1 - deletes all copied Note in song notes queue #m_songNoteQueue and MIDI notes queue #m_midiNoteQueue - calls the _clear()_ member of #m_midiNoteQueue",
		py::arg("bLockEngine"));
	_AudioEngine.def("setSong", &H2Core::AudioEngine::setSong,
		"Updates the global objects of the audioEngine according to new Song.",
		py::arg("pNewSong"));
	_AudioEngine.def("removeSong", &H2Core::AudioEngine::removeSong,
		"Does the necessary cleanup of the global objects in the audioEngine.");
	_AudioEngine.def("noteOn", &H2Core::AudioEngine::noteOn,
		py::arg("note"));
	_AudioEngine.def_static("audioEngine_process", &H2Core::AudioEngine::audioEngine_process,
		"Main audio processing function called by the audio drivers whenever there is work to do.",
		py::arg("nframes"),
		py::arg("arg"));
	_AudioEngine.def("clearNoteQueue", &H2Core::AudioEngine::clearNoteQueue);
	_AudioEngine.def("processPlayNotes", &H2Core::AudioEngine::processPlayNotes,
		py::arg("nframes"));
	_AudioEngine.def("processTransport", &H2Core::AudioEngine::processTransport,
		"Updating the TransportInfo of the audio driver.",
		py::arg("nFrames"));
	// [banned] _AudioEngine.def("renderNote", &H2Core::AudioEngine::renderNote,
	// [banned] 	py::arg("pNote"),
	// [banned] 	py::arg("nBufferSize"));
	_AudioEngine.def("updateNoteQueue", &H2Core::AudioEngine::updateNoteQueue,
		"Takes all notes from the current patterns, from the MIDI queue #m_midiNoteQueue, and those triggered by the metronome and pushes them onto #m_songNoteQueue for playback.",
		py::arg("nFrames"));
	// [banned] _AudioEngine.def("prepNoteQueue", &H2Core::AudioEngine::prepNoteQueue);
	_AudioEngine.def("findPatternInTick", &H2Core::AudioEngine::findPatternInTick,
		"Find a PatternList corresponding to the supplied tick position nTick.",
		py::arg("nTick"),
		py::arg("bLoopMode"),
		py::arg("pPatternStartTick"));
	_AudioEngine.def("seek", &H2Core::AudioEngine::seek,
		py::arg("nFrames"),
		py::arg("bLoopMode"));
	_AudioEngine.def_static("computeTickSize", &H2Core::AudioEngine::computeTickSize,
		py::arg("nSampleRate"),
		py::arg("fBpm"),
		py::arg("nResolution"));
	_AudioEngine.def("getSampler", &H2Core::AudioEngine::getSampler,
		"Returns #m_pSampler");
	_AudioEngine.def("getSynth", &H2Core::AudioEngine::getSynth,
		"Returns #m_pSynth");
	_AudioEngine.def("getElapsedTime", &H2Core::AudioEngine::getElapsedTime,
		"Returns #m_fElapsedTime");
	_AudioEngine.def("calculateElapsedTime", &H2Core::AudioEngine::calculateElapsedTime,
		"Calculates the elapsed time for an arbitrary position.",
		py::arg("sampleRate"),
		py::arg("nFrame"),
		py::arg("nResolution"));
	_AudioEngine.def("updateElapsedTime", &H2Core::AudioEngine::updateElapsedTime,
		"Increments #m_fElapsedTime at the end of a process cycle.",
		py::arg("bufferSize"),
		py::arg("sampleRate"));
	_AudioEngine.def("processCheckBPMChanged", &H2Core::AudioEngine::processCheckBPMChanged,
		"Update the tick size based on the current tempo without affecting the current transport position.",
		py::arg("pSong"));
	_AudioEngine.def("locate", &H2Core::AudioEngine::locate,
		"Relocate using the audio driver and update the #m_fElapsedTime.",
		py::arg("nFrame"));
	_AudioEngine.def("clearAudioBuffers", &H2Core::AudioEngine::clearAudioBuffers,
		"Clear all audio buffers.",
		py::arg("nFrames"));
	_AudioEngine.def("createDriver", &H2Core::AudioEngine::createDriver,
		"Create an audio driver using audioEngine_process() as its argument based on the provided choice and calling their _init()_ function to trigger their initialization.",
		py::arg("sDriver"));
	_AudioEngine.def("startAudioDrivers", &H2Core::AudioEngine::startAudioDrivers,
		"Creation and initialization of all audio and MIDI drivers called in Hydrogen::Hydrogen().");
	_AudioEngine.def("stopAudioDrivers", &H2Core::AudioEngine::stopAudioDrivers,
		"Stops all audio and MIDI drivers.");
	_AudioEngine.def("restartAudioDrivers", &H2Core::AudioEngine::restartAudioDrivers);
	_AudioEngine.def("setupLadspaFX", &H2Core::AudioEngine::setupLadspaFX);
	_AudioEngine.def("renameJackPorts", &H2Core::AudioEngine::renameJackPorts,
		"Hands the provided Song to JackAudioDriver::makeTrackOutputs() if pSong is not a null pointer and the audio driver #m_pAudioDriver is an instance of the JackAudioDriver.",
		py::arg("pSong"));
	_AudioEngine.def("setAudioDriver", &H2Core::AudioEngine::setAudioDriver,
		py::arg("pAudioDriver"));
	_AudioEngine.def("getAudioDriver", &H2Core::AudioEngine::getAudioDriver);
	_AudioEngine.def("getMidiDriver", &H2Core::AudioEngine::getMidiDriver);
	_AudioEngine.def("getMidiOutDriver", &H2Core::AudioEngine::getMidiOutDriver);
	_AudioEngine.def("raiseError", &H2Core::AudioEngine::raiseError,
		py::arg("nErrorCode"));
	_AudioEngine.def("getState", &H2Core::AudioEngine::getState);
	_AudioEngine.def("setState", &H2Core::AudioEngine::setState,
		py::arg("state"));
	_AudioEngine.def("setMasterPeak_L", &H2Core::AudioEngine::setMasterPeak_L,
		py::arg("value"));
	_AudioEngine.def("getMasterPeak_L", &H2Core::AudioEngine::getMasterPeak_L);
	_AudioEngine.def("setMasterPeak_R", &H2Core::AudioEngine::setMasterPeak_R,
		py::arg("value"));
	_AudioEngine.def("getMasterPeak_R", &H2Core::AudioEngine::getMasterPeak_R);
	_AudioEngine.def("getProcessTime", &H2Core::AudioEngine::getProcessTime);
	_AudioEngine.def("getMaxProcessTime", &H2Core::AudioEngine::getMaxProcessTime);
	_AudioEngine.def("getSelectedPatternNumber", &H2Core::AudioEngine::getSelectedPatternNumber);
	_AudioEngine.def("setSelectedPatternNumber", &H2Core::AudioEngine::setSelectedPatternNumber,
		py::arg("number"));
	_AudioEngine.def("setPatternStartTick", &H2Core::AudioEngine::setPatternStartTick,
		py::arg("tick"));
	_AudioEngine.def("setPatternTickPosition", &H2Core::AudioEngine::setPatternTickPosition,
		py::arg("tick"));
	_AudioEngine.def("getPatternTickPosition", &H2Core::AudioEngine::getPatternTickPosition);
	_AudioEngine.def("setSongPos", &H2Core::AudioEngine::setSongPos,
		py::arg("songPos"));
	_AudioEngine.def("getSongPos", &H2Core::AudioEngine::getSongPos);
	_AudioEngine.def("getNextPatterns", &H2Core::AudioEngine::getNextPatterns);
	_AudioEngine.def("getPlayingPatterns", &H2Core::AudioEngine::getPlayingPatterns);
	_AudioEngine.def("getRealtimeFrames", &H2Core::AudioEngine::getRealtimeFrames);
	_AudioEngine.def("setRealtimeFrames", &H2Core::AudioEngine::setRealtimeFrames,
		py::arg("nFrames"));
	_AudioEngine.def("getAddRealtimeNoteTickPosition", &H2Core::AudioEngine::getAddRealtimeNoteTickPosition);
	_AudioEngine.def("setAddRealtimeNoteTickPosition", &H2Core::AudioEngine::setAddRealtimeNoteTickPosition,
		py::arg("tickPosition"));
	_AudioEngine.def("getCurrentTickTime", &H2Core::AudioEngine::getCurrentTickTime);

	py::class_<H2Core::Hydrogen, H2Core::Object, std::shared_ptr<H2Core::Hydrogen>> _Hydrogen(m, "Hydrogen");
	_Hydrogen.def_readwrite("lastMidiEvent", &H2Core::Hydrogen::lastMidiEvent);
	_Hydrogen.def_readwrite("lastMidiEventParameter", &H2Core::Hydrogen::lastMidiEventParameter);
	_Hydrogen.def_readwrite("m_nMaxTimeHumanize", &H2Core::Hydrogen::m_nMaxTimeHumanize);
	_Hydrogen.def_static("class_name", &H2Core::Hydrogen::class_name);
	_Hydrogen.def_static("create_instance", &H2Core::Hydrogen::create_instance,
		"Creates all the instances used within Hydrogen in the right order.");
	_Hydrogen.def_static("get_instance", &H2Core::Hydrogen::get_instance,
		"Returns the current Hydrogen instance #__instance.",
	py::return_value_policy::reference);
	_Hydrogen.def("getAudioEngine", &H2Core::Hydrogen::getAudioEngine);
	_Hydrogen.def("sequencer_play", &H2Core::Hydrogen::sequencer_play,
		"Start the internal sequencer");
	_Hydrogen.def("sequencer_stop", &H2Core::Hydrogen::sequencer_stop,
		"Stop the internal sequencer");
	_Hydrogen.def("midi_noteOn", &H2Core::Hydrogen::midi_noteOn,
		py::arg("note"));
	_Hydrogen.def("sequencer_setNextPattern", &H2Core::Hydrogen::sequencer_setNextPattern,
		"Adding and removing a Pattern from #m_pNextPatterns.",
		py::arg("pos"));
	_Hydrogen.def("sequencer_setOnlyNextPattern", &H2Core::Hydrogen::sequencer_setOnlyNextPattern,
		"Clear #m_pNextPatterns and add one Pattern.",
		py::arg("pos"));
	_Hydrogen.def("togglePlaysSelected", &H2Core::Hydrogen::togglePlaysSelected,
		"Switches playback to focused pattern.");
	_Hydrogen.def("getSong", &H2Core::Hydrogen::getSong,
		"Get the current song.");
	_Hydrogen.def("setSong", &H2Core::Hydrogen::setSong,
		"Sets the current song #__song to newSong.",
		py::arg("newSong"));
	_Hydrogen.def("removeSong", &H2Core::Hydrogen::removeSong);
	_Hydrogen.def("addRealtimeNote", &H2Core::Hydrogen::addRealtimeNote,
		py::arg("instrument"),
		py::arg("velocity"),
		py::arg("fPan"),
		py::arg("pitch"),
		py::arg("noteoff"),
		py::arg("forcePlay"),
		py::arg("msg1"));
	_Hydrogen.def("getTickPosition", &H2Core::Hydrogen::getTickPosition,
		"Returns #m_nPatternTickPosition");
	_Hydrogen.def("getRealtimeTickPosition", &H2Core::Hydrogen::getRealtimeTickPosition,
		"Keep track of the tick position in realtime.");
	_Hydrogen.def("getTotalFrames", &H2Core::Hydrogen::getTotalFrames);
	_Hydrogen.def("setRealtimeFrames", &H2Core::Hydrogen::setRealtimeFrames,
		"Sets #m_nRealtimeFrames",
		py::arg("frames"));
	_Hydrogen.def("getRealtimeFrames", &H2Core::Hydrogen::getRealtimeFrames,
		"Returns the current realtime transport position TransportInfo::m_nFrames.");
	_Hydrogen.def("getCurrentPatternList", &H2Core::Hydrogen::getCurrentPatternList,
		"Returns #m_pPlayingPatterns");
	_Hydrogen.def("setCurrentPatternList", &H2Core::Hydrogen::setCurrentPatternList,
		"Sets #m_pPlayingPatterns.",
		py::arg("pPatternList"));
	_Hydrogen.def("getNextPatterns", &H2Core::Hydrogen::getNextPatterns,
		"Returns #m_pNextPatterns");
	_Hydrogen.def("getPatternPos", &H2Core::Hydrogen::getPatternPos,
		"Get the position of the current Pattern in the Song.");
	_Hydrogen.def("setPatternPos", &H2Core::Hydrogen::setPatternPos,
		"Relocate the position to another Pattern in the Song.",
		py::arg("pos"));
	_Hydrogen.def("getPosForTick", &H2Core::Hydrogen::getPosForTick,
		"Returns the pattern number corresponding to the tick position TickPos.",
		py::arg("TickPos"),
		py::arg("nPatternStartTick"));
	_Hydrogen.def("resetPatternStartTick", &H2Core::Hydrogen::resetPatternStartTick,
		"Move playback in Pattern mode to the beginning of the pattern.");
	_Hydrogen.def("getTickForPosition", &H2Core::Hydrogen::getTickForPosition,
		"Get the total number of ticks passed up to a Pattern at position pos.",
		py::arg("pos"));
	_Hydrogen.def("restartDrivers", &H2Core::Hydrogen::restartDrivers);
	_Hydrogen.def("getAudioOutput", &H2Core::Hydrogen::getAudioOutput);
	_Hydrogen.def("getMidiInput", &H2Core::Hydrogen::getMidiInput);
	_Hydrogen.def("getMidiOutput", &H2Core::Hydrogen::getMidiOutput);
	_Hydrogen.def("getState", &H2Core::Hydrogen::getState,
		"Returns the current state of the audio engine.");
	_Hydrogen.def("loadDrumkit", py::overload_cast<H2Core::Drumkit *>(&H2Core::Hydrogen::loadDrumkit),
		"Wrapper around loadDrumkit( Drumkit, bool ) with the conditional argument set to true.",
		py::arg("pDrumkitInfo"));
	_Hydrogen.def("loadDrumkit", py::overload_cast<H2Core::Drumkit *, bool>(&H2Core::Hydrogen::loadDrumkit),
		"Loads the H2Core::Drumkit provided in pDrumkitInfo into the current session.",
		py::arg("pDrumkitInfo"),
		py::arg("conditional"));
	_Hydrogen.def("instrumentHasNotes", &H2Core::Hydrogen::instrumentHasNotes,
		"Test if an Instrument has some Note in the Pattern (used to test before deleting an Instrument)",
		py::arg("pInst"));
	_Hydrogen.def("removeInstrument", &H2Core::Hydrogen::removeInstrument,
		"Delete an Instrument. If conditional is true, and there are some Pattern that are using this Instrument, it's not deleted anyway.",
		py::arg("instrumentnumber"),
		py::arg("conditional"));
	_Hydrogen.def("getCurrentDrumkitName", &H2Core::Hydrogen::getCurrentDrumkitName,
		"Returns m_sCurrentDrumkitName");
	_Hydrogen.def("setCurrentDrumkitName", &H2Core::Hydrogen::setCurrentDrumkitName,
		py::arg("sName"));
	_Hydrogen.def("getCurrentDrumkitLookup", &H2Core::Hydrogen::getCurrentDrumkitLookup,
		"Returns m_currentDrumkitLookup");
	_Hydrogen.def("setCurrentDrumkitLookup", &H2Core::Hydrogen::setCurrentDrumkitLookup,
		py::arg("lookup"));
	_Hydrogen.def("raiseError", &H2Core::Hydrogen::raiseError,
		py::arg("nErrorCode"));
	// [banned] _Hydrogen.def("previewSample", &H2Core::Hydrogen::previewSample,
	// [banned] 	py::arg("pSample"));
	// [banned] _Hydrogen.def("previewInstrument", &H2Core::Hydrogen::previewInstrument,
	// [banned] 	py::arg("pInstr"));
	_Hydrogen.def("onTapTempoAccelEvent", &H2Core::Hydrogen::onTapTempoAccelEvent);
	_Hydrogen.def("setTapTempo", &H2Core::Hydrogen::setTapTempo,
		py::arg("fInterval"));
	_Hydrogen.def("setBPM", &H2Core::Hydrogen::setBPM,
		"Updates the speed.",
		py::arg("fBPM"));
	_Hydrogen.def("restartLadspaFX", &H2Core::Hydrogen::restartLadspaFX);
	_Hydrogen.def("getSelectedPatternNumber", &H2Core::Hydrogen::getSelectedPatternNumber,
		"Returns #m_nSelectedPatternNumber");
	_Hydrogen.def("setSelectedPatternNumber", &H2Core::Hydrogen::setSelectedPatternNumber,
		"Sets #m_nSelectedPatternNumber.",
		py::arg("nPat"));
	_Hydrogen.def("getSelectedInstrumentNumber", &H2Core::Hydrogen::getSelectedInstrumentNumber);
	_Hydrogen.def("setSelectedInstrumentNumber", &H2Core::Hydrogen::setSelectedInstrumentNumber,
		py::arg("nInstrument"));
	_Hydrogen.def("refreshInstrumentParameters", &H2Core::Hydrogen::refreshInstrumentParameters,
		py::arg("nInstrument"));
	_Hydrogen.def("renameJackPorts", &H2Core::Hydrogen::renameJackPorts,
		"Calls audioEngine_renameJackPorts() if Preferences::m_bJackTrackOuts is set to true.",
		py::arg("pSong"));
	_Hydrogen.def("toggleOscServer", &H2Core::Hydrogen::toggleOscServer,
		"Starts/stops the OSC server",
		py::arg("bEnable"));
	_Hydrogen.def("recreateOscServer", &H2Core::Hydrogen::recreateOscServer,
		"Destroys and recreates the OscServer singleton in order to adopt a new OSC port.");
	_Hydrogen.def("startNsmClient", &H2Core::Hydrogen::startNsmClient);
	_Hydrogen.def("setbeatsToCount", &H2Core::Hydrogen::setbeatsToCount,
		py::arg("beatstocount"));
	_Hydrogen.def("getbeatsToCount", &H2Core::Hydrogen::getbeatsToCount);
	_Hydrogen.def("setNoteLength", &H2Core::Hydrogen::setNoteLength,
		py::arg("notelength"));
	_Hydrogen.def("getNoteLength", &H2Core::Hydrogen::getNoteLength);
	_Hydrogen.def("getBcStatus", &H2Core::Hydrogen::getBcStatus);
	_Hydrogen.def("handleBeatCounter", &H2Core::Hydrogen::handleBeatCounter);
	_Hydrogen.def("setBcOffsetAdjust", &H2Core::Hydrogen::setBcOffsetAdjust);
	_Hydrogen.def("offJackMaster", &H2Core::Hydrogen::offJackMaster,
		"Calling JackAudioDriver::releaseTimebaseMaster() directly from the GUI");
	_Hydrogen.def("onJackMaster", &H2Core::Hydrogen::onJackMaster,
		"Calling JackAudioDriver::initTimebaseMaster() directly from the GUI");
	_Hydrogen.def("getPatternLength", &H2Core::Hydrogen::getPatternLength,
		"Get the length (in ticks) of the nPattern th pattern.",
		py::arg("nPattern"));
	_Hydrogen.def("getNewBpmJTM", &H2Core::Hydrogen::getNewBpmJTM,
		"Returns the fallback speed.");
	_Hydrogen.def("setNewBpmJTM", &H2Core::Hydrogen::setNewBpmJTM,
		"Set the fallback speed #m_nNewBpmJTM.",
		py::arg("bpmJTM"));
	_Hydrogen.def("__panic", &H2Core::Hydrogen::__panic);
	_Hydrogen.def("setTimelineBpm", &H2Core::Hydrogen::setTimelineBpm,
		"Updates Song::m_fBpm, TransportInfo::m_fBPM, and #m_fNewBpmJTM to the local speed.");
	_Hydrogen.def("getTimelineBpm", &H2Core::Hydrogen::getTimelineBpm,
		"Returns the local speed at a specific nBar in the Timeline.",
		py::arg("nBar"));
	_Hydrogen.def("getTimeline", &H2Core::Hydrogen::getTimeline);
	_Hydrogen.def("getIsExportSessionActive", &H2Core::Hydrogen::getIsExportSessionActive);
	_Hydrogen.def("startExportSession", &H2Core::Hydrogen::startExportSession,
		py::arg("rate"),
		py::arg("depth"));
	_Hydrogen.def("stopExportSession", &H2Core::Hydrogen::stopExportSession);
	_Hydrogen.def("startExportSong", &H2Core::Hydrogen::startExportSong,
		py::arg("filename"));
	_Hydrogen.def("stopExportSong", &H2Core::Hydrogen::stopExportSong);
	_Hydrogen.def("getCoreActionController", &H2Core::Hydrogen::getCoreActionController);
	_Hydrogen.def("setPlaybackTrackState", &H2Core::Hydrogen::setPlaybackTrackState,
		"*********************************************************",
		py::arg("state"));
	_Hydrogen.def("getPlaybackTrackState", &H2Core::Hydrogen::getPlaybackTrackState,
		"Wrapper around Song::getPlaybackTrackEnabled().");
	_Hydrogen.def("loadPlaybackTrack", &H2Core::Hydrogen::loadPlaybackTrack,
		"Wrapper function for loading the playback track.",
		py::arg("filename"));
	_Hydrogen.def("getGUIState", &H2Core::Hydrogen::getGUIState,
		"Returns #m_GUIState");
	_Hydrogen.def("setGUIState", &H2Core::Hydrogen::setGUIState,
		py::arg("state"));
	_Hydrogen.def("calculateLeadLagFactor", &H2Core::Hydrogen::calculateLeadLagFactor,
		"Calculates the lookahead for a specific tick size.",
		py::arg("fTickSize"));
	_Hydrogen.def("calculateLookahead", &H2Core::Hydrogen::calculateLookahead,
		"Calculates time offset (in frames) used to determine the notes process by the audio engine.",
		py::arg("fTickSize"));
	_Hydrogen.def("haveJackAudioDriver", &H2Core::Hydrogen::haveJackAudioDriver,
		"Returns Whether JackAudioDriver is used as current audio driver.");
	_Hydrogen.def("haveJackTransport", &H2Core::Hydrogen::haveJackTransport,
		"Returns Whether JackAudioDriver is used as current audio driver and JACK transport was activated via the GUI (#Preferences::m_bJackTransportMode).");
	_Hydrogen.def("getJackTimebaseState", &H2Core::Hydrogen::getJackTimebaseState,
		"Returns Whether we haveJackTransport() and there is an external JACK timebase master broadcasting us tempo information and making use disregard Hydrogen's Timeline information (see #JackAudioDriver::m_timebaseState).");
	_Hydrogen.def("isUnderSessionManagement", &H2Core::Hydrogen::isUnderSessionManagement,
		"Returns NsmClient::m_bUnderSessionManagement if NSM is supported.");
	_Hydrogen.def("toQString", &H2Core::Hydrogen::toQString,
		"Formatted string version for debugging purposes.",
		py::arg("sPrefix"),
		py::arg("bShort"));

	py::class_<H2Core::Filesystem, H2Core::Object, std::shared_ptr<H2Core::Filesystem>> _Filesystem(m, "Filesystem");
	_Filesystem.def_static("class_name", &H2Core::Filesystem::class_name);
	_Filesystem.def_static("bootstrap", &H2Core::Filesystem::bootstrap,
		"check user and system filesystem usability",
		py::arg("logger"),
		py::arg("sys_path"));
	_Filesystem.def_static("sys_data_path", &H2Core::Filesystem::sys_data_path,
		"returns system data path");
	_Filesystem.def_static("usr_data_path", &H2Core::Filesystem::usr_data_path,
		"returns user data path");
	// [<Class 'QStringList'>] _Filesystem.def_static("ladspa_paths", &H2Core::Filesystem::ladspa_paths,
		// [<Class 'QStringList'>] "returns user ladspa paths");
	_Filesystem.def_static("sys_config_path", &H2Core::Filesystem::sys_config_path,
		"returns system config path");
	_Filesystem.def_static("usr_config_path", &H2Core::Filesystem::usr_config_path,
		"returns user config path");
	_Filesystem.def_static("empty_sample_path", &H2Core::Filesystem::empty_sample_path,
		"returns system empty sample file path");
	_Filesystem.def_static("empty_song_path", &H2Core::Filesystem::empty_song_path,
		"returns system empty song file path");
	_Filesystem.def_static("untitled_song_file_name", &H2Core::Filesystem::untitled_song_file_name,
		"returns untitled song file name");
	_Filesystem.def_static("click_file_path", &H2Core::Filesystem::click_file_path,
		"Returns a string containing the path to the _click.wav_ file used in the metronome.");
	_Filesystem.def_static("usr_click_file_path", &H2Core::Filesystem::usr_click_file_path,
		"returns click file path from user directory if exists, otherwise from system");
	_Filesystem.def_static("drumkit_xsd_path", &H2Core::Filesystem::drumkit_xsd_path,
		"returns the path to the drumkit XSD (xml schema definition) file");
	_Filesystem.def_static("pattern_xsd_path", &H2Core::Filesystem::pattern_xsd_path,
		"returns the path to the pattern XSD (xml schema definition) file");
	_Filesystem.def_static("playlist_xsd_path", &H2Core::Filesystem::playlist_xsd_path,
		"returns the path to the playlist pattern XSD (xml schema definition) file");
	_Filesystem.def_static("log_file_path", &H2Core::Filesystem::log_file_path,
		"returns the full path (including filename) of the logfile");
	_Filesystem.def_static("img_dir", &H2Core::Filesystem::img_dir,
		"returns gui image path");
	_Filesystem.def_static("doc_dir", &H2Core::Filesystem::doc_dir,
		"returns documentation path");
	_Filesystem.def_static("i18n_dir", &H2Core::Filesystem::i18n_dir,
		"returns internationalization path");
	_Filesystem.def_static("scripts_dir", &H2Core::Filesystem::scripts_dir,
		"returns user scripts path");
	_Filesystem.def_static("songs_dir", &H2Core::Filesystem::songs_dir,
		"returns user songs path");
	_Filesystem.def_static("song_path", &H2Core::Filesystem::song_path,
		"returns user song path, add file extension",
		py::arg("sg_name"));
	_Filesystem.def_static("patterns_dir_static", py::overload_cast<>(&H2Core::Filesystem::patterns_dir),
		"returns user patterns path");
	_Filesystem.def_static("patterns_dir_static", py::overload_cast<const QString &>(&H2Core::Filesystem::patterns_dir),
		"returns user patterns path for a specific drumkit",
		py::arg("dk_name"));
	_Filesystem.def_static("pattern_path", &H2Core::Filesystem::pattern_path,
		"returns user patterns path, add file extension",
		py::arg("dk_name"),
		py::arg("p_name"));
	_Filesystem.def_static("plugins_dir", &H2Core::Filesystem::plugins_dir,
		"returns user plugins path");
	_Filesystem.def_static("sys_drumkits_dir", &H2Core::Filesystem::sys_drumkits_dir,
		"returns system drumkits path");
	_Filesystem.def_static("usr_drumkits_dir", &H2Core::Filesystem::usr_drumkits_dir,
		"returns user drumkits path");
	_Filesystem.def_static("playlists_dir", &H2Core::Filesystem::playlists_dir,
		"returns user playlist path");
	_Filesystem.def_static("playlist_path", &H2Core::Filesystem::playlist_path,
		"returns user playlist path, add file extension",
		py::arg("pl_name"));
	_Filesystem.def_static("untitled_playlist_file_name", &H2Core::Filesystem::untitled_playlist_file_name,
		"returns untitled playlist file name");
	_Filesystem.def_static("cache_dir", &H2Core::Filesystem::cache_dir,
		"returns user cache path");
	_Filesystem.def_static("repositories_cache_dir", &H2Core::Filesystem::repositories_cache_dir,
		"returns user repository cache path");
	_Filesystem.def_static("demos_dir", &H2Core::Filesystem::demos_dir,
		"returns system demos path");
	_Filesystem.def_static("xsd_dir", &H2Core::Filesystem::xsd_dir,
		"returns system xsd path");
	_Filesystem.def_static("tmp_dir", &H2Core::Filesystem::tmp_dir,
		"returns temp path");
	_Filesystem.def_static("tmp_file_path", &H2Core::Filesystem::tmp_file_path,
		"touch a temporary file under tmp_dir() and return it's path. if base has a suffix it will be preserved, spaces will be replaced by underscores.",
		py::arg("base"));
	_Filesystem.def_static("prepare_sample_path", &H2Core::Filesystem::prepare_sample_path,
		"Returns the basename if the given path is under an existing user or system drumkit path, otherwise the given fname",
		py::arg("fname"));
	_Filesystem.def_static("file_is_under_drumkit", &H2Core::Filesystem::file_is_under_drumkit,
		"Checks if the given filepath is under an existing user or system drumkit path, not the existence of the file",
		py::arg("fname"));
	_Filesystem.def_static("get_basename_idx_under_drumkit", &H2Core::Filesystem::get_basename_idx_under_drumkit,
		"Returns the index of the basename if the given path is under an existing user or system drumkit path, otherwise -1",
		py::arg("fname"));
	// [<Class 'QStringList'>] _Filesystem.def_static("sys_drumkit_list", &H2Core::Filesystem::sys_drumkit_list,
		// [<Class 'QStringList'>] "returns list of usable system drumkits ( see Filesystem::drumkit_list )");
	// [<Class 'QStringList'>] _Filesystem.def_static("usr_drumkit_list", &H2Core::Filesystem::usr_drumkit_list,
		// [<Class 'QStringList'>] "returns list of usable user drumkits ( see Filesystem::drumkit_list )");
	_Filesystem.def_static("drumkit_exists", &H2Core::Filesystem::drumkit_exists,
		"returns true if the drumkit exists within usable system or user drumkits",
		py::arg("dk_name"));
	_Filesystem.def_static("drumkit_usr_path", &H2Core::Filesystem::drumkit_usr_path,
		"returns path for a drumkit within user drumkit path",
		py::arg("dk_name"));
	_Filesystem.def_static("drumkit_path_search", &H2Core::Filesystem::drumkit_path_search,
		"Returns the path to a H2Core::Drumkit folder.",
		py::arg("dk_name"),
		py::arg("lookup"),
		py::arg("bSilent"));
	_Filesystem.def_static("drumkit_dir_search", &H2Core::Filesystem::drumkit_dir_search,
		"returns the directory holding the named drumkit searching within user then system drumkits",
		py::arg("dk_name"),
		py::arg("lookup"));
	_Filesystem.def_static("drumkit_valid", &H2Core::Filesystem::drumkit_valid,
		"returns true if the path contains a usable drumkit",
		py::arg("dk_path"));
	_Filesystem.def_static("drumkit_file", &H2Core::Filesystem::drumkit_file,
		"returns the path to the xml file within a supposed drumkit path",
		py::arg("dk_path"));
	// [<Class 'QStringList'>] _Filesystem.def_static("pattern_drumkits", &H2Core::Filesystem::pattern_drumkits,
		// [<Class 'QStringList'>] "returns a list of existing drumkit sub dir into the patterns directory");
	// [<Class 'QStringList'>] _Filesystem.def_static("pattern_list_static", py::overload_cast<>(&H2Core::Filesystem::pattern_list),
		// [<Class 'QStringList'>] "returns a list of existing patterns");
	// [<Class 'QStringList'>] _Filesystem.def_static("pattern_list_static", py::overload_cast<const QString &>(&H2Core::Filesystem::pattern_list),
		// [<Class 'QStringList'>] "returns a list of existing patterns",
	// [<Class 'QStringList'>] 	py::arg("path"));
	// [<Class 'QStringList'>] _Filesystem.def_static("song_list", &H2Core::Filesystem::song_list,
		// [<Class 'QStringList'>] "returns a list of existing songs");
	// [<Class 'QStringList'>] _Filesystem.def_static("song_list_cleared", &H2Core::Filesystem::song_list_cleared,
		// [<Class 'QStringList'>] "returns a list of existing songs, excluding the autosaved one");
	_Filesystem.def_static("song_exists", &H2Core::Filesystem::song_exists,
		"returns true if the song file exists",
		py::arg("sg_name"));
	_Filesystem.def_static("info", &H2Core::Filesystem::info,
		"send current settings information to logger with INFO severity");
	// [<Class 'QStringList'>] _Filesystem.def_static("playlist_list", &H2Core::Filesystem::playlist_list,
		// [<Class 'QStringList'>] "returns a list of existing playlists");
	_Filesystem.def_static("file_exists", &H2Core::Filesystem::file_exists,
		"returns true if the given path is an existing regular file",
		py::arg("path"),
		py::arg("silent"));
	_Filesystem.def_static("file_readable", &H2Core::Filesystem::file_readable,
		"returns true if the given path is an existing readable regular file",
		py::arg("path"),
		py::arg("silent"));
	_Filesystem.def_static("file_writable", &H2Core::Filesystem::file_writable,
		"returns true if the given path is a possibly writable file (may exist or not)",
		py::arg("path"),
		py::arg("silent"));
	_Filesystem.def_static("file_executable", &H2Core::Filesystem::file_executable,
		"returns true if the given path is an existing executable regular file",
		py::arg("path"),
		py::arg("silent"));
	_Filesystem.def_static("dir_readable", &H2Core::Filesystem::dir_readable,
		"returns true if the given path is a readable regular directory",
		py::arg("path"),
		py::arg("silent"));
	_Filesystem.def_static("dir_writable", &H2Core::Filesystem::dir_writable,
		"returns true if the given path is a writable regular directory",
		py::arg("path"),
		py::arg("silent"));
	_Filesystem.def_static("path_usable", &H2Core::Filesystem::path_usable,
		"returns true if the path is a readable and writable regular directory, create if it not exists",
		py::arg("path"),
		py::arg("create"),
		py::arg("silent"));
	_Filesystem.def_static("write_to_file", &H2Core::Filesystem::write_to_file,
		"writes to a file",
		py::arg("dst"),
		py::arg("content"));
	_Filesystem.def_static("file_copy", &H2Core::Filesystem::file_copy,
		"copy a source file to a destination",
		py::arg("src"),
		py::arg("dst"),
		py::arg("overwrite"));
	_Filesystem.def_static("rm", &H2Core::Filesystem::rm,
		"remove a path",
		py::arg("path"),
		py::arg("recursive"));
	_Filesystem.def_static("mkdir", &H2Core::Filesystem::mkdir,
		"create a path",
		py::arg("path"));
	_Filesystem.def_static("getPreferencesOverwritePath", &H2Core::Filesystem::getPreferencesOverwritePath,
		"Returns m_sPreferencesOverwritePath");
	_Filesystem.def_static("setPreferencesOverwritePath", &H2Core::Filesystem::setPreferencesOverwritePath,
		py::arg("sPath"));

	py::class_<H2Core::Song, H2Core::Object, std::shared_ptr<H2Core::Song>> _Song(m, "Song");
	_Song.def(py::init<const QString &, const QString &, float, float>());
	_Song.def_static("class_name", &H2Core::Song::class_name);
	_Song.def_static("getEmptySong", &H2Core::Song::getEmptySong);
	_Song.def_static("getDefaultSong", &H2Core::Song::getDefaultSong);
	_Song.def("getIsMuted", &H2Core::Song::getIsMuted);
	_Song.def("setIsMuted", &H2Core::Song::setIsMuted,
		py::arg("bIsMuted"));
	_Song.def("getResolution", &H2Core::Song::getResolution);
	_Song.def("setResolution", &H2Core::Song::setResolution,
		py::arg("resolution"));
	_Song.def("getBpm", &H2Core::Song::getBpm);
	_Song.def("setBpm", &H2Core::Song::setBpm,
		py::arg("fBpm"));
	_Song.def("getName", &H2Core::Song::getName);
	_Song.def("setName", &H2Core::Song::setName,
		py::arg("sName"));
	_Song.def("setVolume", &H2Core::Song::setVolume,
		py::arg("fValue"));
	_Song.def("getVolume", &H2Core::Song::getVolume);
	_Song.def("setMetronomeVolume", &H2Core::Song::setMetronomeVolume,
		py::arg("fValue"));
	_Song.def("getMetronomeVolume", &H2Core::Song::getMetronomeVolume);
	_Song.def("getPatternList", &H2Core::Song::getPatternList);
	_Song.def("setPatternList", &H2Core::Song::setPatternList,
		py::arg("pList"));
	_Song.def("getPatternGroupVector", py::overload_cast<>(&H2Core::Song::getPatternGroupVector),
		"Return a pointer to a vector storing all Pattern present in the Song.");
	_Song.def("getPatternGroupVector", py::overload_cast<>(&H2Core::Song::getPatternGroupVector),
		"Return a pointer to a vector storing all Pattern present in the Song.");
	_Song.def("setPatternGroupVector", &H2Core::Song::setPatternGroupVector,
		"Sets the vector storing all Pattern present in the Song #m_pPatternGroupSequence.",
		py::arg("pGroupVector"));
	_Song.def("lengthInTicks", &H2Core::Song::lengthInTicks,
		"get the length of the song, in tick units");
	_Song.def_static("load", &H2Core::Song::load,
		py::arg("sFilename"));
	_Song.def("save", &H2Core::Song::save,
		py::arg("sFilename"));
	_Song.def("purgeInstrument", &H2Core::Song::purgeInstrument,
		"Remove all the notes in the song that play on instrument I. The function is real-time safe (it locks the audio data while deleting notes)",
		py::arg("pInstr"));
	_Song.def("getInstrumentList", &H2Core::Song::getInstrumentList);
	_Song.def("setInstrumentList", &H2Core::Song::setInstrumentList,
		py::arg("pList"));
	_Song.def("setNotes", &H2Core::Song::setNotes,
		py::arg("sNotes"));
	_Song.def("getNotes", &H2Core::Song::getNotes);
	_Song.def("setLicense", &H2Core::Song::setLicense,
		py::arg("sLicense"));
	_Song.def("getLicense", &H2Core::Song::getLicense);
	_Song.def("setAuthor", &H2Core::Song::setAuthor,
		py::arg("sAuthor"));
	_Song.def("getAuthor", &H2Core::Song::getAuthor);
	_Song.def("getFilename", &H2Core::Song::getFilename);
	_Song.def("setFilename", &H2Core::Song::setFilename,
		py::arg("sFilename"));
	_Song.def("getIsLoopEnabled", &H2Core::Song::getIsLoopEnabled);
	_Song.def("setIsLoopEnabled", &H2Core::Song::setIsLoopEnabled,
		py::arg("bEnabled"));
	_Song.def("getHumanizeTimeValue", &H2Core::Song::getHumanizeTimeValue);
	_Song.def("setHumanizeTimeValue", &H2Core::Song::setHumanizeTimeValue,
		py::arg("fValue"));
	_Song.def("getHumanizeVelocityValue", &H2Core::Song::getHumanizeVelocityValue);
	_Song.def("setHumanizeVelocityValue", &H2Core::Song::setHumanizeVelocityValue,
		py::arg("fValue"));
	_Song.def("getSwingFactor", &H2Core::Song::getSwingFactor);
	_Song.def("setSwingFactor", &H2Core::Song::setSwingFactor,
		py::arg("fFactor"));
	_Song.def("getMode", &H2Core::Song::getMode);
	_Song.def("setMode", &H2Core::Song::setMode,
		py::arg("mode"));
	_Song.def("setIsModified", &H2Core::Song::setIsModified,
		py::arg("bIsModified"));
	_Song.def("getIsModified", &H2Core::Song::getIsModified);
	_Song.def("getComponents", &H2Core::Song::getComponents);
	_Song.def("getVelocityAutomationPath", &H2Core::Song::getVelocityAutomationPath);
	_Song.def("getComponent", &H2Core::Song::getComponent,
		py::arg("nID"));
	_Song.def("readTempPatternList", &H2Core::Song::readTempPatternList,
		py::arg("sFilename"));
	_Song.def("writeTempPatternList", &H2Core::Song::writeTempPatternList,
		py::arg("sFilename"));
	_Song.def("copyInstrumentLineToString", &H2Core::Song::copyInstrumentLineToString,
		py::arg("nSelectedPattern"),
		py::arg("selectedInstrument"));
	_Song.def("pasteInstrumentLineFromString", &H2Core::Song::pasteInstrumentLineFromString,
		py::arg("sSerialized"),
		py::arg("nSelectedPattern"),
		py::arg("nSelectedInstrument"),
		py::arg("pPatterns"));
	_Song.def("getLatestRoundRobin", &H2Core::Song::getLatestRoundRobin,
		py::arg("fStartVelocity"));
	_Song.def("setLatestRoundRobin", &H2Core::Song::setLatestRoundRobin,
		py::arg("fStartVelocity"),
		py::arg("nLatestRoundRobin"));
	_Song.def("getPlaybackTrackFilename", &H2Core::Song::getPlaybackTrackFilename,
		"Returns #m_sPlaybackTrackFilename");
	_Song.def("setPlaybackTrackFilename", &H2Core::Song::setPlaybackTrackFilename,
		py::arg("sFilename"));
	_Song.def("getPlaybackTrackEnabled", &H2Core::Song::getPlaybackTrackEnabled,
		"Returns #m_bPlaybackTrackEnabled");
	_Song.def("setPlaybackTrackEnabled", &H2Core::Song::setPlaybackTrackEnabled,
		"Specifies whether a playback track should be used.",
		py::arg("bEnabled"));
	_Song.def("getPlaybackTrackVolume", &H2Core::Song::getPlaybackTrackVolume,
		"Returns #m_fPlaybackTrackVolume");
	_Song.def("setPlaybackTrackVolume", &H2Core::Song::setPlaybackTrackVolume,
		py::arg("fVolume"));
	_Song.def("getActionMode", &H2Core::Song::getActionMode);
	_Song.def("setActionMode", &H2Core::Song::setActionMode,
		py::arg("actionMode"));
	_Song.def("hasMissingSamples", &H2Core::Song::hasMissingSamples,
		"Song was incompletely loaded from file (missing samples)");
	_Song.def("clearMissingSamples", &H2Core::Song::clearMissingSamples);
	_Song.def("setPanLawType", &H2Core::Song::setPanLawType,
		py::arg("nPanLawType"));
	_Song.def("getPanLawType", &H2Core::Song::getPanLawType);
	_Song.def("setPanLawKNorm", &H2Core::Song::setPanLawKNorm,
		py::arg("fKNorm"));
	_Song.def("getPanLawKNorm", &H2Core::Song::getPanLawKNorm);
	_Song.def("toQString", &H2Core::Song::toQString,
		"Formatted string version for debugging purposes.",
		py::arg("sPrefix"),
		py::arg("bShort"));
	_Song.def("__repr__",
	[](const H2Core::Song & song) {
  return "<Song \"" + song.getName() + "\">";
}
);

	py::class_<H2Core::AutomationPath, H2Core::Object, std::shared_ptr<H2Core::AutomationPath>> _AutomationPath(m, "AutomationPath");
	_AutomationPath.def(py::init<float, float, float>());
	_AutomationPath.def_static("class_name", &H2Core::AutomationPath::class_name);
	_AutomationPath.def("empty", &H2Core::AutomationPath::empty);
	_AutomationPath.def("get_min", &H2Core::AutomationPath::get_min);
	_AutomationPath.def("get_max", &H2Core::AutomationPath::get_max);
	_AutomationPath.def("get_default", &H2Core::AutomationPath::get_default);
	_AutomationPath.def("get_value", &H2Core::AutomationPath::get_value,
		py::arg("x"));
	_AutomationPath.def("add_point", &H2Core::AutomationPath::add_point,
		py::arg("x"),
		py::arg("y"));
	_AutomationPath.def("remove_point", &H2Core::AutomationPath::remove_point,
		py::arg("x"));
	// [<TypeDef 'iterator'>] _AutomationPath.def("begin", py::overload_cast<>(&H2Core::AutomationPath::begin));
	// [<TypeDef 'const_iterator'>] _AutomationPath.def("begin", py::overload_cast<>(&H2Core::AutomationPath::begin));
	// [<TypeDef 'iterator'>] _AutomationPath.def("end", py::overload_cast<>(&H2Core::AutomationPath::end));
	// [<TypeDef 'const_iterator'>] _AutomationPath.def("end", py::overload_cast<>(&H2Core::AutomationPath::end));
	// [<TypeDef 'iterator'>] _AutomationPath.def("find", &H2Core::AutomationPath::find,
	// [<TypeDef 'iterator'>] 	py::arg("x"));
	// [<TypeDef 'iterator'>] _AutomationPath.def("move", &H2Core::AutomationPath::move,
	// [<TypeDef 'iterator'>] 	py::arg("in"),
	// [<TypeDef 'iterator'>] 	py::arg("x"),
	// [<TypeDef 'iterator'>] 	py::arg("y"));
	_AutomationPath.def("toQString", &H2Core::AutomationPath::toQString,
		"Formatted string version for debugging purposes.",
		py::arg("sPrefix"),
		py::arg("bShort"));

	py::class_<H2Core::Pattern, H2Core::Object, std::shared_ptr<H2Core::Pattern>> _Pattern(m, "Pattern");
	_Pattern.def(py::init<const QString &, const QString &, const QString &, int, int>());
	_Pattern.def(py::init<H2Core::Pattern *>());
	_Pattern.def_static("class_name", &H2Core::Pattern::class_name);
	_Pattern.def_static("load_file", &H2Core::Pattern::load_file,
		"load a pattern from a file",
		py::arg("pattern_path"),
		py::arg("instruments"));
	_Pattern.def("save_file", &H2Core::Pattern::save_file,
		"save a pattern into an xml file",
		py::arg("drumkit_name"),
		py::arg("author"),
		py::arg("license"),
		py::arg("pattern_path"),
		py::arg("overwrite"));
	_Pattern.def("set_name", &H2Core::Pattern::set_name,
		py::arg("name"));
	_Pattern.def("get_name", &H2Core::Pattern::get_name);
	_Pattern.def("set_category", &H2Core::Pattern::set_category,
		py::arg("category"));
	_Pattern.def("set_info", &H2Core::Pattern::set_info,
		py::arg("info"));
	_Pattern.def("get_info", &H2Core::Pattern::get_info);
	_Pattern.def("get_category", &H2Core::Pattern::get_category);
	_Pattern.def("set_length", &H2Core::Pattern::set_length,
		py::arg("length"));
	_Pattern.def("get_length", &H2Core::Pattern::get_length);
	_Pattern.def("set_denominator", &H2Core::Pattern::set_denominator,
		py::arg("denominator"));
	_Pattern.def("get_denominator", &H2Core::Pattern::get_denominator);
	// [<TypeDef 'notes_t'>] _Pattern.def("get_notes", &H2Core::Pattern::get_notes);
	// [<TypeDef 'virtual_patterns_t'>] _Pattern.def("get_virtual_patterns", &H2Core::Pattern::get_virtual_patterns);
	// [<TypeDef 'virtual_patterns_t'>] _Pattern.def("get_flattened_virtual_patterns", &H2Core::Pattern::get_flattened_virtual_patterns);
	_Pattern.def("insert_note", &H2Core::Pattern::insert_note,
		"insert a new note within __notes",
		py::arg("note"));
	// [banned] _Pattern.def("find_note", py::overload_cast<int, int, std::shared_ptr<Instrument>, bool>(&H2Core::Pattern::find_note),
		// [banned] "search for a note at a given index within __notes which correspond to the given arguments",
	// [banned] 	py::arg("idx_a"),
	// [banned] 	py::arg("idx_b"),
	// [banned] 	py::arg("instrument"),
	// [banned] 	py::arg("strict"));
	// [banned] _Pattern.def("find_note", py::overload_cast<int, int, std::shared_ptr<Instrument>, Note::Key, Note::Octave, bool>(&H2Core::Pattern::find_note),
		// [banned] "search for a note at a given index within __notes which correspond to the given arguments",
	// [banned] 	py::arg("idx_a"),
	// [banned] 	py::arg("idx_b"),
	// [banned] 	py::arg("instrument"),
	// [banned] 	py::arg("key"),
	// [banned] 	py::arg("octave"),
	// [banned] 	py::arg("strict"));
	_Pattern.def("remove_note", &H2Core::Pattern::remove_note,
		"removes a given note from __notes, it's not deleted",
		py::arg("note"));
	_Pattern.def("references", &H2Core::Pattern::references,
		"check if this pattern contains a note referencing the given instrument",
		py::arg("instr"));
	_Pattern.def("purge_instrument", &H2Core::Pattern::purge_instrument,
		"delete the notes referencing the given instrument The function is thread safe (it locks the audio data while deleting notes)",
		py::arg("instr"));
	_Pattern.def("set_to_old", &H2Core::Pattern::set_to_old,
		"mark all notes as old");
	_Pattern.def("virtual_patterns_empty", &H2Core::Pattern::virtual_patterns_empty);
	_Pattern.def("virtual_patterns_clear", &H2Core::Pattern::virtual_patterns_clear);
	_Pattern.def("virtual_patterns_add", &H2Core::Pattern::virtual_patterns_add,
		"add a pattern to __virtual_patterns",
		py::arg("pattern"));
	_Pattern.def("virtual_patterns_del", &H2Core::Pattern::virtual_patterns_del,
		"remove a pattern from virtual_pattern set, flattened virtual patterns have to be rebuilt",
		py::arg("pattern"));
	_Pattern.def("flattened_virtual_patterns_clear", &H2Core::Pattern::flattened_virtual_patterns_clear);
	_Pattern.def("flattened_virtual_patterns_compute", &H2Core::Pattern::flattened_virtual_patterns_compute,
		"compute virtual_pattern_transitive_closure_set based on virtual_pattern_transitive_closure_set virtual_pattern_transitive_closure_set must have been cleared before which is the case is called from PatternList::compute_flattened_virtual_patterns");
	_Pattern.def("extand_with_flattened_virtual_patterns", &H2Core::Pattern::extand_with_flattened_virtual_patterns,
		"add content of __flatteened_virtual_patterns into patterns",
		py::arg("patterns"));
	_Pattern.def("save_to", &H2Core::Pattern::save_to,
		"save the pattern within the given XMLNode",
		py::arg("node"),
		py::arg("instrumentOnly"));
	_Pattern.def("toQString", &H2Core::Pattern::toQString,
		"Formatted string version for debugging purposes.",
		py::arg("sPrefix"),
		py::arg("bShort"));

	py::class_<H2Core::Playlist, H2Core::Object, std::shared_ptr<H2Core::Playlist>> _Playlist(m, "Playlist");
	_Playlist.def_static("class_name", &H2Core::Playlist::class_name);
	_Playlist.def_static("create_instance", &H2Core::Playlist::create_instance,
		"If #__instance equals 0, a new Playlist singleton will be created and stored in it.");
	_Playlist.def_static("get_instance", &H2Core::Playlist::get_instance,
		"Returns a pointer to the current Playlist singleton stored in #__instance.");
	_Playlist.def("activateSong", &H2Core::Playlist::activateSong,
		py::arg("SongNumber"));
	_Playlist.def("size", &H2Core::Playlist::size);
	_Playlist.def("get", &H2Core::Playlist::get,
		py::arg("idx"));
	_Playlist.def("clear", &H2Core::Playlist::clear);
	_Playlist.def("add", &H2Core::Playlist::add,
		py::arg("entry"));
	_Playlist.def("setNextSongByNumber", &H2Core::Playlist::setNextSongByNumber,
		py::arg("SongNumber"));
	_Playlist.def("getSelectedSongNr", &H2Core::Playlist::getSelectedSongNr);
	_Playlist.def("setSelectedSongNr", &H2Core::Playlist::setSelectedSongNr,
		py::arg("songNumber"));
	_Playlist.def("getActiveSongNumber", &H2Core::Playlist::getActiveSongNumber);
	_Playlist.def("setActiveSongNumber", &H2Core::Playlist::setActiveSongNumber,
		py::arg("ActiveSongNumber"));
	_Playlist.def("getSongFilenameByNumber", &H2Core::Playlist::getSongFilenameByNumber,
		py::arg("songNumber"),
		py::arg("fileName"));
	_Playlist.def("getFilename", &H2Core::Playlist::getFilename);
	_Playlist.def("setFilename", &H2Core::Playlist::setFilename,
		py::arg("filename"));
	_Playlist.def("getIsModified", &H2Core::Playlist::getIsModified);
	_Playlist.def("setIsModified", &H2Core::Playlist::setIsModified,
		py::arg("IsModified"));
	_Playlist.def_static("load", &H2Core::Playlist::load,
		py::arg("filename"),
		py::arg("useRelativePaths"));
	_Playlist.def_static("load_file", &H2Core::Playlist::load_file,
		py::arg("pl_path"),
		py::arg("useRelativePaths"));
	_Playlist.def("save_file", &H2Core::Playlist::save_file,
		py::arg("pl_path"),
		py::arg("name"),
		py::arg("overwrite"),
		py::arg("useRelativePaths"));
	_Playlist.def("toQString", &H2Core::Playlist::toQString,
		"Formatted string version for debugging purposes.",
		py::arg("sPrefix"),
		py::arg("bShort"));

	py::class_<H2Core::Preferences, H2Core::Object, std::shared_ptr<H2Core::Preferences>> _Preferences(m, "Preferences");
	_Preferences.def_readwrite("__lastspatternDirectory", &H2Core::Preferences::__lastspatternDirectory);
	_Preferences.def_readwrite("__lastsampleDirectory", &H2Core::Preferences::__lastsampleDirectory);
	_Preferences.def_readwrite("__playsamplesonclicking", &H2Core::Preferences::__playsamplesonclicking);
	_Preferences.def_readwrite("__playselectedinstrument", &H2Core::Preferences::__playselectedinstrument);
	_Preferences.def_readwrite("m_bFollowPlayhead", &H2Core::Preferences::m_bFollowPlayhead);
	_Preferences.def_readwrite("m_brestartLash", &H2Core::Preferences::m_brestartLash);
	_Preferences.def_readwrite("m_bsetLash", &H2Core::Preferences::m_bsetLash);
	_Preferences.def_readwrite("__expandSongItem", &H2Core::Preferences::__expandSongItem);
	_Preferences.def_readwrite("__expandPatternItem", &H2Core::Preferences::__expandPatternItem);
	_Preferences.def_readwrite("m_bbc", &H2Core::Preferences::m_bbc);
	_Preferences.def_readwrite("m_mmcsetplay", &H2Core::Preferences::m_mmcsetplay);
	_Preferences.def_readwrite("m_countOffset", &H2Core::Preferences::m_countOffset);
	_Preferences.def_readwrite("m_startOffset", &H2Core::Preferences::m_startOffset);
	_Preferences.def_readwrite("sServerList", &H2Core::Preferences::sServerList);
	_Preferences.def_readwrite("m_patternCategories", &H2Core::Preferences::m_patternCategories);
	_Preferences.def_readwrite("m_sAudioDriver", &H2Core::Preferences::m_sAudioDriver);
	_Preferences.def_readwrite("m_bUseMetronome", &H2Core::Preferences::m_bUseMetronome);
	_Preferences.def_readwrite("m_fMetronomeVolume", &H2Core::Preferences::m_fMetronomeVolume);
	_Preferences.def_readwrite("m_nMaxNotes", &H2Core::Preferences::m_nMaxNotes);
	_Preferences.def_readwrite("m_nBufferSize", &H2Core::Preferences::m_nBufferSize);
	_Preferences.def_readwrite("m_nSampleRate", &H2Core::Preferences::m_nSampleRate);
	_Preferences.def_readwrite("m_sOSSDevice", &H2Core::Preferences::m_sOSSDevice);
	_Preferences.def_readwrite("m_sMidiDriver", &H2Core::Preferences::m_sMidiDriver);
	_Preferences.def_readwrite("m_sMidiPortName", &H2Core::Preferences::m_sMidiPortName);
	_Preferences.def_readwrite("m_sMidiOutputPortName", &H2Core::Preferences::m_sMidiOutputPortName);
	_Preferences.def_readwrite("m_nMidiChannelFilter", &H2Core::Preferences::m_nMidiChannelFilter);
	_Preferences.def_readwrite("m_bMidiNoteOffIgnore", &H2Core::Preferences::m_bMidiNoteOffIgnore);
	_Preferences.def_readwrite("m_bMidiFixedMapping", &H2Core::Preferences::m_bMidiFixedMapping);
	_Preferences.def_readwrite("m_bMidiDiscardNoteAfterAction", &H2Core::Preferences::m_bMidiDiscardNoteAfterAction);
	_Preferences.def_readwrite("m_bEnableMidiFeedback", &H2Core::Preferences::m_bEnableMidiFeedback);
	_Preferences.def_readwrite("m_bOscServerEnabled", &H2Core::Preferences::m_bOscServerEnabled);
	_Preferences.def_readwrite("m_bOscFeedbackEnabled", &H2Core::Preferences::m_bOscFeedbackEnabled);
	_Preferences.def_readwrite("m_nOscTemporaryPort", &H2Core::Preferences::m_nOscTemporaryPort);
	_Preferences.def_readwrite("m_nOscServerPort", &H2Core::Preferences::m_nOscServerPort);
	_Preferences.def_readwrite("m_sAlsaAudioDevice", &H2Core::Preferences::m_sAlsaAudioDevice);
	_Preferences.def_readwrite("m_sJackPortName1", &H2Core::Preferences::m_sJackPortName1);
	_Preferences.def_readwrite("m_sJackPortName2", &H2Core::Preferences::m_sJackPortName2);
	_Preferences.def_readwrite("m_bJackTransportMode", &H2Core::Preferences::m_bJackTransportMode);
	_Preferences.def_readwrite("m_bJackConnectDefaults", &H2Core::Preferences::m_bJackConnectDefaults);
	_Preferences.def_readwrite("m_bJackTrackOuts", &H2Core::Preferences::m_bJackTrackOuts);
	_Preferences.def_readwrite("m_JackTrackOutputMode", &H2Core::Preferences::m_JackTrackOutputMode);
	_Preferences.def_readwrite("m_bJackTimebaseEnabled", &H2Core::Preferences::m_bJackTimebaseEnabled);
	_Preferences.def_readwrite("m_JackBBTSync", &H2Core::Preferences::m_JackBBTSync);
	_Preferences.def_readwrite("m_bJackMasterMode", &H2Core::Preferences::m_bJackMasterMode);
	_Preferences.def_readwrite("m_sDefaultEditor", &H2Core::Preferences::m_sDefaultEditor);
	_Preferences.def_readwrite("m_rubberBandCLIexecutable", &H2Core::Preferences::m_rubberBandCLIexecutable);
	_Preferences.def_static("class_name", &H2Core::Preferences::class_name);
	_Preferences.def_static("create_instance", &H2Core::Preferences::create_instance,
		"If #__instance equals 0, a new Preferences singleton will be created and stored in it.");
	_Preferences.def_static("get_instance", &H2Core::Preferences::get_instance,
		"Returns a pointer to the current Preferences singleton stored in #__instance.");
	_Preferences.def("loadPreferences", &H2Core::Preferences::loadPreferences,
		"Load the preferences file",
		py::arg("bGlobal"));
	_Preferences.def("savePreferences", &H2Core::Preferences::savePreferences,
		"Save the preferences file");
	// [banned] _Preferences.def("getDataDirectory", &H2Core::Preferences::getDataDirectory);
	_Preferences.def("getDefaultEditor", &H2Core::Preferences::getDefaultEditor);
	_Preferences.def("setDefaultEditor", &H2Core::Preferences::setDefaultEditor,
		py::arg("editor"));
	_Preferences.def("getDefaultUILayout", &H2Core::Preferences::getDefaultUILayout);
	_Preferences.def("setDefaultUILayout", &H2Core::Preferences::setDefaultUILayout,
		py::arg("layout"));
	_Preferences.def("getUIScalingPolicy", &H2Core::Preferences::getUIScalingPolicy);
	_Preferences.def("setUIScalingPolicy", &H2Core::Preferences::setUIScalingPolicy,
		py::arg("nPolicy"));
	_Preferences.def("getPreferredLanguage", &H2Core::Preferences::getPreferredLanguage);
	_Preferences.def("setPreferredLanguage", &H2Core::Preferences::setPreferredLanguage,
		py::arg("sLanguage"));
	_Preferences.def("setRestoreLastSongEnabled", &H2Core::Preferences::setRestoreLastSongEnabled,
		py::arg("restore"));
	_Preferences.def("setRestoreLastPlaylistEnabled", &H2Core::Preferences::setRestoreLastPlaylistEnabled,
		py::arg("restore"));
	_Preferences.def("setUseRelativeFilenamesForPlaylists", &H2Core::Preferences::setUseRelativeFilenamesForPlaylists,
		py::arg("value"));
	_Preferences.def("setShowDevelWarning", &H2Core::Preferences::setShowDevelWarning,
		py::arg("value"));
	_Preferences.def("getShowDevelWarning", &H2Core::Preferences::getShowDevelWarning);
	_Preferences.def("getShowNoteOverwriteWarning", &H2Core::Preferences::getShowNoteOverwriteWarning);
	_Preferences.def("setShowNoteOverwriteWarning", &H2Core::Preferences::setShowNoteOverwriteWarning,
		py::arg("bValue"));
	_Preferences.def("isRestoreLastSongEnabled", &H2Core::Preferences::isRestoreLastSongEnabled);
	_Preferences.def("isRestoreLastPlaylistEnabled", &H2Core::Preferences::isRestoreLastPlaylistEnabled);
	_Preferences.def("isPlaylistUsingRelativeFilenames", &H2Core::Preferences::isPlaylistUsingRelativeFilenames);
	_Preferences.def("setLastSongFilename", &H2Core::Preferences::setLastSongFilename,
		py::arg("filename"));
	_Preferences.def("getLastSongFilename", &H2Core::Preferences::getLastSongFilename);
	_Preferences.def("setLastPlaylistFilename", &H2Core::Preferences::setLastPlaylistFilename,
		py::arg("filename"));
	_Preferences.def("getLastPlaylistFilename", &H2Core::Preferences::getLastPlaylistFilename);
	_Preferences.def("setHearNewNotes", &H2Core::Preferences::setHearNewNotes,
		py::arg("value"));
	_Preferences.def("getHearNewNotes", &H2Core::Preferences::getHearNewNotes);
	_Preferences.def("setRecordEvents", &H2Core::Preferences::setRecordEvents,
		py::arg("value"));
	_Preferences.def("getRecordEvents", &H2Core::Preferences::getRecordEvents);
	_Preferences.def("setPunchInPos", &H2Core::Preferences::setPunchInPos,
		py::arg("pos"));
	_Preferences.def("getPunchInPos", &H2Core::Preferences::getPunchInPos);
	_Preferences.def("setPunchOutPos", &H2Core::Preferences::setPunchOutPos,
		py::arg("pos"));
	_Preferences.def("getPunchOutPos", &H2Core::Preferences::getPunchOutPos);
	_Preferences.def("inPunchArea", &H2Core::Preferences::inPunchArea,
		py::arg("pos"));
	_Preferences.def("unsetPunchArea", &H2Core::Preferences::unsetPunchArea);
	_Preferences.def("setQuantizeEvents", &H2Core::Preferences::setQuantizeEvents,
		py::arg("value"));
	_Preferences.def("getQuantizeEvents", &H2Core::Preferences::getQuantizeEvents);
	_Preferences.def("getRecentFiles", &H2Core::Preferences::getRecentFiles);
	_Preferences.def("setRecentFiles", &H2Core::Preferences::setRecentFiles,
		py::arg("recentFiles"));
	_Preferences.def("insertRecentFile", &H2Core::Preferences::insertRecentFile,
		py::arg("sFilename"));
	_Preferences.def("getRecentFX", &H2Core::Preferences::getRecentFX);
	_Preferences.def("setMostRecentFX", &H2Core::Preferences::setMostRecentFX,
		py::arg(""));
	_Preferences.def("getQTStyle", &H2Core::Preferences::getQTStyle);
	_Preferences.def("setQTStyle", &H2Core::Preferences::setQTStyle,
		py::arg("sStyle"));
	_Preferences.def("getApplicationFontFamily", &H2Core::Preferences::getApplicationFontFamily);
	_Preferences.def("setApplicationFontFamily", &H2Core::Preferences::setApplicationFontFamily,
		py::arg("family"));
	_Preferences.def("getLevel2FontFamily", &H2Core::Preferences::getLevel2FontFamily);
	_Preferences.def("setLevel2FontFamily", &H2Core::Preferences::setLevel2FontFamily,
		py::arg("family"));
	_Preferences.def("getLevel3FontFamily", &H2Core::Preferences::getLevel3FontFamily);
	_Preferences.def("setLevel3FontFamily", &H2Core::Preferences::setLevel3FontFamily,
		py::arg("family"));
	_Preferences.def("getFontSize", &H2Core::Preferences::getFontSize);
	_Preferences.def("setFontSize", &H2Core::Preferences::setFontSize,
		py::arg("fontSize"));
	_Preferences.def("getMixerFalloffSpeed", &H2Core::Preferences::getMixerFalloffSpeed);
	_Preferences.def("setMixerFalloffSpeed", &H2Core::Preferences::setMixerFalloffSpeed,
		py::arg("value"));
	_Preferences.def("showInstrumentPeaks", &H2Core::Preferences::showInstrumentPeaks);
	_Preferences.def("setInstrumentPeaks", &H2Core::Preferences::setInstrumentPeaks,
		py::arg("value"));
	_Preferences.def("getPatternEditorGridResolution", &H2Core::Preferences::getPatternEditorGridResolution);
	_Preferences.def("setPatternEditorGridResolution", &H2Core::Preferences::setPatternEditorGridResolution,
		py::arg("value"));
	_Preferences.def("isPatternEditorUsingTriplets", &H2Core::Preferences::isPatternEditorUsingTriplets);
	_Preferences.def("setPatternEditorUsingTriplets", &H2Core::Preferences::setPatternEditorUsingTriplets,
		py::arg("value"));
	_Preferences.def("isFXTabVisible", &H2Core::Preferences::isFXTabVisible);
	_Preferences.def("setFXTabVisible", &H2Core::Preferences::setFXTabVisible,
		py::arg("value"));
	_Preferences.def("getShowAutomationArea", &H2Core::Preferences::getShowAutomationArea);
	_Preferences.def("setShowAutomationArea", &H2Core::Preferences::setShowAutomationArea,
		py::arg("value"));
	_Preferences.def("getPatternEditorGridHeight", &H2Core::Preferences::getPatternEditorGridHeight);
	_Preferences.def("setPatternEditorGridHeight", &H2Core::Preferences::setPatternEditorGridHeight,
		py::arg("value"));
	_Preferences.def("getPatternEditorGridWidth", &H2Core::Preferences::getPatternEditorGridWidth);
	_Preferences.def("setPatternEditorGridWidth", &H2Core::Preferences::setPatternEditorGridWidth,
		py::arg("value"));
	_Preferences.def("getSongEditorGridHeight", &H2Core::Preferences::getSongEditorGridHeight);
	_Preferences.def("setSongEditorGridHeight", &H2Core::Preferences::setSongEditorGridHeight,
		py::arg("value"));
	_Preferences.def("getSongEditorGridWidth", &H2Core::Preferences::getSongEditorGridWidth);
	_Preferences.def("setSongEditorGridWidth", &H2Core::Preferences::setSongEditorGridWidth,
		py::arg("value"));
	_Preferences.def("setColoringMethod", &H2Core::Preferences::setColoringMethod,
		py::arg("value"));
	_Preferences.def("getColoringMethod", &H2Core::Preferences::getColoringMethod);
	_Preferences.def("setPatternColors", &H2Core::Preferences::setPatternColors,
		py::arg("patternColors"));
	_Preferences.def("getPatternColors", &H2Core::Preferences::getPatternColors);
	_Preferences.def("setMaxPatternColors", &H2Core::Preferences::setMaxPatternColors,
		py::arg("nValue"));
	_Preferences.def("getMaxPatternColors", &H2Core::Preferences::getMaxPatternColors);
	_Preferences.def("setVisiblePatternColors", &H2Core::Preferences::setVisiblePatternColors,
		py::arg("nValue"));
	_Preferences.def("getVisiblePatternColors", &H2Core::Preferences::getVisiblePatternColors);
	_Preferences.def("getMainFormProperties", &H2Core::Preferences::getMainFormProperties);
	_Preferences.def("setMainFormProperties", &H2Core::Preferences::setMainFormProperties,
		py::arg("prop"));
	_Preferences.def("getMixerProperties", &H2Core::Preferences::getMixerProperties);
	_Preferences.def("setMixerProperties", &H2Core::Preferences::setMixerProperties,
		py::arg("prop"));
	_Preferences.def("getPatternEditorProperties", &H2Core::Preferences::getPatternEditorProperties);
	_Preferences.def("setPatternEditorProperties", &H2Core::Preferences::setPatternEditorProperties,
		py::arg("prop"));
	_Preferences.def("getSongEditorProperties", &H2Core::Preferences::getSongEditorProperties);
	_Preferences.def("setSongEditorProperties", &H2Core::Preferences::setSongEditorProperties,
		py::arg("prop"));
	_Preferences.def("getInstrumentRackProperties", &H2Core::Preferences::getInstrumentRackProperties);
	_Preferences.def("setInstrumentRackProperties", &H2Core::Preferences::setInstrumentRackProperties,
		py::arg("prop"));
	_Preferences.def("getAudioEngineInfoProperties", &H2Core::Preferences::getAudioEngineInfoProperties);
	_Preferences.def("setAudioEngineInfoProperties", &H2Core::Preferences::setAudioEngineInfoProperties,
		py::arg("prop"));
	_Preferences.def("getLadspaProperties", &H2Core::Preferences::getLadspaProperties,
		py::arg("nFX"));
	_Preferences.def("setLadspaProperties", &H2Core::Preferences::setLadspaProperties,
		py::arg("nFX"),
		py::arg("prop"));
	_Preferences.def("getDefaultUIStyle", &H2Core::Preferences::getDefaultUIStyle);
	_Preferences.def("patternModePlaysSelected", &H2Core::Preferences::patternModePlaysSelected,
		"Returns #m_bPatternModePlaysSelected");
	_Preferences.def("setPatternModePlaysSelected", &H2Core::Preferences::setPatternModePlaysSelected,
		py::arg("b"));
	_Preferences.def("useLash", &H2Core::Preferences::useLash);
	_Preferences.def("setUseLash", &H2Core::Preferences::setUseLash,
		py::arg("b"));
	_Preferences.def("hideKeyboardCursor", &H2Core::Preferences::hideKeyboardCursor);
	_Preferences.def("setHideKeyboardCursor", &H2Core::Preferences::setHideKeyboardCursor,
		py::arg("value"));
	_Preferences.def("setMaxBars", &H2Core::Preferences::setMaxBars,
		py::arg("bars"));
	_Preferences.def("getMaxBars", &H2Core::Preferences::getMaxBars,
		"Returns #m_nMaxBars.");
	_Preferences.def("setMaxLayers", &H2Core::Preferences::setMaxLayers,
		py::arg("layers"));
	_Preferences.def("getMaxLayers", &H2Core::Preferences::getMaxLayers,
		"Returns #m_nMaxLayers.");
	_Preferences.def("setWaitForSessionHandler", &H2Core::Preferences::setWaitForSessionHandler,
		py::arg("value"));
	_Preferences.def("getWaitForSessionHandler", &H2Core::Preferences::getWaitForSessionHandler);
	_Preferences.def("setNsmClientId", &H2Core::Preferences::setNsmClientId,
		py::arg("nsmClientId"));
	_Preferences.def("getNsmClientId", &H2Core::Preferences::getNsmClientId);
	_Preferences.def("setNsmSongName", &H2Core::Preferences::setNsmSongName,
		py::arg("nsmSongName"));
	_Preferences.def("getNsmSongName", &H2Core::Preferences::getNsmSongName);
	_Preferences.def("getOscServerEnabled", &H2Core::Preferences::getOscServerEnabled,
		"Returns #m_bOscServerEnabled");
	_Preferences.def("setOscServerEnabled", &H2Core::Preferences::setOscServerEnabled,
		py::arg("val"));
	_Preferences.def("getOscFeedbackEnabled", &H2Core::Preferences::getOscFeedbackEnabled,
		"Returns #m_bOscFeedbackEnabled");
	_Preferences.def("setOscFeedbackEnabled", &H2Core::Preferences::setOscFeedbackEnabled,
		py::arg("val"));
	_Preferences.def("getOscServerPort", &H2Core::Preferences::getOscServerPort,
		"Returns #m_nOscServerPort");
	_Preferences.def("setOscServerPort", &H2Core::Preferences::setOscServerPort,
		py::arg("oscPort"));
	_Preferences.def("getUseTimelineBpm", &H2Core::Preferences::getUseTimelineBpm,
		"Whether to use the bpm of the timeline.");
	_Preferences.def("setUseTimelineBpm", &H2Core::Preferences::setUseTimelineBpm,
		"Setting #__useTimelineBpm.",
		py::arg("val"));
	_Preferences.def("setShowPlaybackTrack", &H2Core::Preferences::setShowPlaybackTrack,
		py::arg("val"));
	_Preferences.def("getShowPlaybackTrack", &H2Core::Preferences::getShowPlaybackTrack);
	_Preferences.def("getRubberBandCalcTime", &H2Core::Preferences::getRubberBandCalcTime);
	_Preferences.def("setRubberBandCalcTime", &H2Core::Preferences::setRubberBandCalcTime,
		py::arg("val"));
	_Preferences.def("getRubberBandBatchMode", &H2Core::Preferences::getRubberBandBatchMode);
	_Preferences.def("setRubberBandBatchMode", &H2Core::Preferences::setRubberBandBatchMode,
		py::arg("val"));
	_Preferences.def("getLastOpenTab", &H2Core::Preferences::getLastOpenTab);
	_Preferences.def("setLastOpenTab", &H2Core::Preferences::setLastOpenTab,
		py::arg("n"));
	_Preferences.def("setH2ProcessName", &H2Core::Preferences::setH2ProcessName,
		py::arg("processName"));
	_Preferences.def("getH2ProcessName", &H2Core::Preferences::getH2ProcessName);
	_Preferences.def("getExportSampleDepthIdx", &H2Core::Preferences::getExportSampleDepthIdx);
	_Preferences.def("setExportSampleDepthIdx", &H2Core::Preferences::setExportSampleDepthIdx,
		py::arg("ExportSampleDepth"));
	_Preferences.def("getExportSampleRateIdx", &H2Core::Preferences::getExportSampleRateIdx);
	_Preferences.def("setExportSampleRateIdx", &H2Core::Preferences::setExportSampleRateIdx,
		py::arg("ExportSampleRate"));
	_Preferences.def("getExportModeIdx", &H2Core::Preferences::getExportModeIdx);
	_Preferences.def("setExportModeIdx", &H2Core::Preferences::setExportModeIdx,
		py::arg("ExportModeIdx"));
	_Preferences.def("getExportDirectory", &H2Core::Preferences::getExportDirectory);
	_Preferences.def("setExportDirectory", &H2Core::Preferences::setExportDirectory,
		py::arg("ExportDirectory"));
	_Preferences.def("getExportTemplateIdx", &H2Core::Preferences::getExportTemplateIdx);
	_Preferences.def("setExportTemplateIdx", &H2Core::Preferences::setExportTemplateIdx,
		py::arg("ExportTemplateIdx"));
	_Preferences.def("getMidiExportMode", &H2Core::Preferences::getMidiExportMode);
	_Preferences.def("setMidiExportMode", &H2Core::Preferences::setMidiExportMode,
		py::arg("ExportMode"));
	_Preferences.def("getMidiExportDirectory", &H2Core::Preferences::getMidiExportDirectory);
	_Preferences.def("setMidiExportDirectory", &H2Core::Preferences::setMidiExportDirectory,
		py::arg("ExportDirectory"));
	// [banned] _Preferences.def("getPreferencesOverwritePath", &H2Core::Preferences::getPreferencesOverwritePath,
		// [banned] "Returns #m_sPreferencesOverwritePath");
	// [banned] _Preferences.def("setPreferencesOverwritePath", &H2Core::Preferences::setPreferencesOverwritePath,
		// [banned] "Setting #m_sPreferencesOverwritePath.",
	// [banned] 	py::arg("newPath"));

	py::class_<H2Core::WindowProperties, H2Core::Object, std::shared_ptr<H2Core::WindowProperties>> _WindowProperties(m, "WindowProperties");
	_WindowProperties.def(py::init<>());
	_WindowProperties.def_readwrite("x", &H2Core::WindowProperties::x);
	_WindowProperties.def_readwrite("y", &H2Core::WindowProperties::y);
	_WindowProperties.def_readwrite("width", &H2Core::WindowProperties::width);
	_WindowProperties.def_readwrite("height", &H2Core::WindowProperties::height);
	_WindowProperties.def_readwrite("visible", &H2Core::WindowProperties::visible);
	_WindowProperties.def_static("class_name", &H2Core::WindowProperties::class_name);
	_WindowProperties.def("set", &H2Core::WindowProperties::set,
		py::arg("_x"),
		py::arg("_y"),
		py::arg("_width"),
		py::arg("_height"),
		py::arg("_visible"));

	py::class_<H2Core::Sample, H2Core::Object, std::shared_ptr<H2Core::Sample>> _Sample(m, "Sample");
	_Sample.def(py::init<const QString &, int, int, float *, float *>());
	_Sample.def(py::init<std::shared_ptr<Sample>>());
	_Sample.def_static("class_name", &H2Core::Sample::class_name);
	_Sample.def("write", &H2Core::Sample::write,
		"write sample to a file",
		py::arg("path"),
		py::arg("format"));
	_Sample.def_static("load_static", py::overload_cast<const QString &>(&H2Core::Sample::load),
		"Load a sample from a file.",
		py::arg("filepath"));
	// [<TemplateRef 'vector'>] _Sample.def_static("load_static", py::overload_cast<const QString &, const H2Core::Sample::Loops &, const H2Core::Sample::Rubberband &, const H2Core::Sample::VelocityEnvelope &, const H2Core::Sample::PanEnvelope &>(&H2Core::Sample::load),
		// [<TemplateRef 'vector'>] "Load a sample from a file and apply the transformations to the sample data.",
	// [<TemplateRef 'vector'>] 	py::arg("filepath"),
	// [<TemplateRef 'vector'>] 	py::arg("loops"),
	// [<TemplateRef 'vector'>] 	py::arg("rubber"),
	// [<TemplateRef 'vector'>] 	py::arg("velocity"),
	// [<TemplateRef 'vector'>] 	py::arg("pan"));
	_Sample.def("load", py::overload_cast<>(&H2Core::Sample::load),
		"Load the sample stored in #__filepath into #__data_l and #__data_r.");
	_Sample.def("unload", &H2Core::Sample::unload,
		"Flush the current content of the left and right channel and the current metadata.");
	// [<TemplateRef 'vector'>] _Sample.def("apply", &H2Core::Sample::apply,
		// [<TemplateRef 'vector'>] "Apply transformations to the sample data.",
	// [<TemplateRef 'vector'>] 	py::arg("loops"),
	// [<TemplateRef 'vector'>] 	py::arg("rubber"),
	// [<TemplateRef 'vector'>] 	py::arg("velocity"),
	// [<TemplateRef 'vector'>] 	py::arg("pan"));
	_Sample.def("apply_loops", &H2Core::Sample::apply_loops,
		"apply loop transformation to the sample",
		py::arg("lo"));
	// [<TemplateRef 'vector'>] _Sample.def("apply_velocity", &H2Core::Sample::apply_velocity,
		// [<TemplateRef 'vector'>] "apply velocity transformation to the sample",
	// [<TemplateRef 'vector'>] 	py::arg("v"));
	// [<TemplateRef 'vector'>] _Sample.def("apply_pan", &H2Core::Sample::apply_pan,
		// [<TemplateRef 'vector'>] "apply velocity transformation to the sample",
	// [<TemplateRef 'vector'>] 	py::arg("p"));
	_Sample.def("apply_rubberband", &H2Core::Sample::apply_rubberband,
		"apply rubberband transformation to the sample",
		py::arg("rb"));
	_Sample.def("exec_rubberband_cli", &H2Core::Sample::exec_rubberband_cli,
		"call rubberband cli to modify the sample",
		py::arg("rb"));
	_Sample.def("is_empty", &H2Core::Sample::is_empty,
		"Returns true if both data channels are null pointers");
	_Sample.def("get_filepath", &H2Core::Sample::get_filepath,
		"Returns #__filepath");
	_Sample.def("get_filename", &H2Core::Sample::get_filename,
		"Returns Filename part of #__filepath");
	_Sample.def("set_filename", &H2Core::Sample::set_filename,
		py::arg("filename"));
	_Sample.def("set_frames", &H2Core::Sample::set_frames,
		"#__frames setter",
		py::arg("frames"));
	_Sample.def("get_frames", &H2Core::Sample::get_frames,
		"Returns #__frames accessor");
	_Sample.def("set_sample_rate", &H2Core::Sample::set_sample_rate,
		py::arg("sampleRate"));
	_Sample.def("get_sample_rate", &H2Core::Sample::get_sample_rate,
		"Returns #__sample_rate");
	_Sample.def("get_sample_duration", &H2Core::Sample::get_sample_duration,
		"Returns sample duration in seconds");
	_Sample.def("get_size", &H2Core::Sample::get_size,
		"Returns data size, which is calculated by #__frames time sizeof( float ) * 2");
	_Sample.def("get_data_l",
	[](const H2Core::Sample & sample) {

    size_t nframes = sample.get_frames();
    auto result = py::array_t<float>(nframes);
    py::buffer_info buf = result.request();
    float *ptr = static_cast<float *>(buf.ptr);
    float *src = sample.get_data_l();
    for (size_t idx = 0; idx < nframes; idx++) {
        ptr[idx] = src[idx];
    }
    return result;
}
,
		"Returns #__data_l");
	_Sample.def("get_data_r",
	[](const H2Core::Sample & sample) {

    size_t nframes = sample.get_frames();
    auto result = py::array_t<float>(nframes);
    py::buffer_info buf = result.request();
    float *ptr = static_cast<float *>(buf.ptr);
    float *src = sample.get_data_r();
    for (size_t idx = 0; idx < nframes; idx++) {
        ptr[idx] = src[idx];
    }
    return result;
}
,
		"Returns #__data_r");
	_Sample.def("set_is_modified", &H2Core::Sample::set_is_modified,
		"#__is_modified setter",
		py::arg("is_modified"));
	_Sample.def("get_is_modified", &H2Core::Sample::get_is_modified,
		"Returns #__is_modified");
	// [<TemplateRef 'vector'>] _Sample.def("get_pan_envelope", &H2Core::Sample::get_pan_envelope,
		// [<TemplateRef 'vector'>] "Returns #__pan_envelope");
	// [<TemplateRef 'vector'>] _Sample.def("get_velocity_envelope", &H2Core::Sample::get_velocity_envelope,
		// [<TemplateRef 'vector'>] "Returns #__velocity_envelope");
	_Sample.def("get_loops", &H2Core::Sample::get_loops,
		"Returns #__loops parameters");
	_Sample.def("get_rubberband", &H2Core::Sample::get_rubberband,
		"Returns #__rubberband parameters");
	_Sample.def_static("parse_loop_mode", &H2Core::Sample::parse_loop_mode,
		"parse the given string and rturn the corresponding loop_mode",
		py::arg("string"));
	_Sample.def("get_loop_mode_string", &H2Core::Sample::get_loop_mode_string,
		"Returns mode member of #__loops as a string");
	_Sample.def("toQString", &H2Core::Sample::toQString,
		"Formatted string version for debugging purposes.",
		py::arg("sPrefix"),
		py::arg("bShort"));
	_Sample.def("__repr__",
	[](const H2Core::Sample & sample) {
  return "<Sample \"" + sample.get_filename() + "\">";
}
);

	py::class_<H2Core::InstrumentList, H2Core::Object, std::shared_ptr<H2Core::InstrumentList>> _InstrumentList(m, "InstrumentList");
	_InstrumentList.def(py::init<>());
	_InstrumentList.def(py::init<H2Core::InstrumentList *>());
	_InstrumentList.def_static("class_name", &H2Core::InstrumentList::class_name);
	_InstrumentList.def("size", &H2Core::InstrumentList::size,
		"returns the numbers of instruments");
	_InstrumentList.def("operator<<", &H2Core::InstrumentList::operator<<,
		"add an instrument to the list",
		py::arg("instrument"));
	_InstrumentList.def("operator[]", &H2Core::InstrumentList::operator[],
		"get an instrument from the list",
		py::arg("idx"));
	_InstrumentList.def("add", &H2Core::InstrumentList::add,
		"add an instrument to the list",
		py::arg("instrument"));
	_InstrumentList.def("insert", &H2Core::InstrumentList::insert,
		"insert an instrument into the list",
		py::arg("idx"),
		py::arg("instrument"));
	_InstrumentList.def("is_valid_index", &H2Core::InstrumentList::is_valid_index,
		"check if there is a idx is a valid index for this list without throwing an error messaage",
		py::arg("idx"));
	_InstrumentList.def("get", &H2Core::InstrumentList::get,
		"get an instrument from the list",
		py::arg("idx"));
	_InstrumentList.def("del", py::overload_cast<int>(&H2Core::InstrumentList::del),
		"remove the instrument at a given index, does not delete it",
		py::arg("idx"));
	_InstrumentList.def("del", py::overload_cast<std::shared_ptr<Instrument>>(&H2Core::InstrumentList::del),
		"remove an instrument from the list, does not delete it",
		py::arg("instrument"));
	_InstrumentList.def("index", &H2Core::InstrumentList::index,
		"get the index of an instrument within the instruments",
		py::arg("instrument"));
	_InstrumentList.def("find", py::overload_cast<const int>(&H2Core::InstrumentList::find),
		"find an instrument within the instruments",
		py::arg("i"));
	_InstrumentList.def("find", py::overload_cast<const QString &>(&H2Core::InstrumentList::find),
		"find an instrument within the instruments",
		py::arg("name"));
	_InstrumentList.def("findMidiNote", &H2Core::InstrumentList::findMidiNote,
		"find an instrument which play the given midi note",
		py::arg("note"));
	_InstrumentList.def("swap", &H2Core::InstrumentList::swap,
		"swap the instruments of two different indexes",
		py::arg("idx_a"),
		py::arg("idx_b"));
	_InstrumentList.def("move", &H2Core::InstrumentList::move,
		"move an instrument from a position to another",
		py::arg("idx_a"),
		py::arg("idx_b"));
	_InstrumentList.def("load_samples", &H2Core::InstrumentList::load_samples,
		"Calls the Instrument::load_samples() member function of all Instruments in #__instruments.");
	_InstrumentList.def("unload_samples", &H2Core::InstrumentList::unload_samples,
		"Calls the Instrument::unload_samples() member function of all Instruments in #__instruments.");
	_InstrumentList.def("save_to", &H2Core::InstrumentList::save_to,
		"save the instrument list within the given XMLNode",
		py::arg("node"),
		py::arg("component_id"));
	_InstrumentList.def_static("load_from", &H2Core::InstrumentList::load_from,
		"load an instrument list from an XMLNode",
		py::arg("node"),
		py::arg("dk_path"),
		py::arg("dk_name"));
	_InstrumentList.def("fix_issue_307", &H2Core::InstrumentList::fix_issue_307,
		"Fix GitHub issue #307, so called \"Hi Bongo fiasco\".");
	_InstrumentList.def("has_all_midi_notes_same", &H2Core::InstrumentList::has_all_midi_notes_same,
		"Check if all instruments have assigned the same MIDI out note");
	_InstrumentList.def("set_default_midi_out_notes", &H2Core::InstrumentList::set_default_midi_out_notes,
		"Set each instrument consecuteve MIDI out notes, starting from 36");
	_InstrumentList.def("toQString", &H2Core::InstrumentList::toQString,
		"Formatted string version for debugging purposes.",
		py::arg("sPrefix"),
		py::arg("bShort"));

	py::class_<H2Core::InstrumentLayer, H2Core::Object, std::shared_ptr<H2Core::InstrumentLayer>> _InstrumentLayer(m, "InstrumentLayer");
	_InstrumentLayer.def(py::init<std::shared_ptr<Sample>>());
	_InstrumentLayer.def(py::init<std::shared_ptr<InstrumentLayer>>());
	_InstrumentLayer.def(py::init<std::shared_ptr<InstrumentLayer>, std::shared_ptr<Sample>>());
	_InstrumentLayer.def_static("class_name", &H2Core::InstrumentLayer::class_name);
	_InstrumentLayer.def("set_gain", &H2Core::InstrumentLayer::set_gain,
		"set the gain of the layer",
		py::arg("gain"));
	_InstrumentLayer.def("get_gain", &H2Core::InstrumentLayer::get_gain,
		"get the gain of the layer");
	_InstrumentLayer.def("set_pitch", &H2Core::InstrumentLayer::set_pitch,
		"set the pitch of the layer",
		py::arg("pitch"));
	_InstrumentLayer.def("get_pitch", &H2Core::InstrumentLayer::get_pitch,
		"get the pitch of the layer");
	_InstrumentLayer.def("set_start_velocity", &H2Core::InstrumentLayer::set_start_velocity,
		"set the start ivelocity of the layer",
		py::arg("start"));
	_InstrumentLayer.def("get_start_velocity", &H2Core::InstrumentLayer::get_start_velocity,
		"get the start velocity of the layer");
	_InstrumentLayer.def("set_end_velocity", &H2Core::InstrumentLayer::set_end_velocity,
		"set the end velocity of the layer",
		py::arg("end"));
	_InstrumentLayer.def("get_end_velocity", &H2Core::InstrumentLayer::get_end_velocity,
		"get the end velocity of the layer");
	_InstrumentLayer.def("set_sample", &H2Core::InstrumentLayer::set_sample,
		"set the sample of the layer",
		py::arg("sample"));
	_InstrumentLayer.def("get_sample", &H2Core::InstrumentLayer::get_sample,
		"get the sample of the layer");
	_InstrumentLayer.def("load_sample", &H2Core::InstrumentLayer::load_sample,
		"Calls the #H2Core::Sample::load() member function of #__sample.");
	_InstrumentLayer.def("unload_sample", &H2Core::InstrumentLayer::unload_sample);
	_InstrumentLayer.def("save_to", &H2Core::InstrumentLayer::save_to,
		"save the instrument layer within the given XMLNode",
		py::arg("node"));
	_InstrumentLayer.def_static("load_from", &H2Core::InstrumentLayer::load_from,
		"load an instrument layer from an XMLNode",
		py::arg("node"),
		py::arg("dk_path"));
	_InstrumentLayer.def("toQString", &H2Core::InstrumentLayer::toQString,
		"Formatted string version for debugging purposes.",
		py::arg("sPrefix"),
		py::arg("bShort"));

	py::class_<H2Core::InstrumentComponent, H2Core::Object, std::shared_ptr<H2Core::InstrumentComponent>> _InstrumentComponent(m, "InstrumentComponent");
	_InstrumentComponent.def(py::init<int>());
	_InstrumentComponent.def(py::init<std::shared_ptr<InstrumentComponent>>());
	_InstrumentComponent.def_static("class_name", &H2Core::InstrumentComponent::class_name);
	_InstrumentComponent.def("save_to", &H2Core::InstrumentComponent::save_to,
		py::arg("node"),
		py::arg("component_id"));
	_InstrumentComponent.def_static("load_from", &H2Core::InstrumentComponent::load_from,
		py::arg("node"),
		py::arg("dk_path"));
	_InstrumentComponent.def("operator[]", &H2Core::InstrumentComponent::operator[],
		py::arg("idx"));
	_InstrumentComponent.def("get_layer", &H2Core::InstrumentComponent::get_layer,
		py::arg("idx"));
	_InstrumentComponent.def("set_layer", &H2Core::InstrumentComponent::set_layer,
		py::arg("layer"),
		py::arg("idx"));
	_InstrumentComponent.def("set_drumkit_componentID", &H2Core::InstrumentComponent::set_drumkit_componentID,
		"Sets the component ID #__related_drumkit_componentID",
		py::arg("related_drumkit_componentID"));
	_InstrumentComponent.def("get_drumkit_componentID", &H2Core::InstrumentComponent::get_drumkit_componentID,
		"Returns the component ID of the drumkit.");
	_InstrumentComponent.def("set_gain", &H2Core::InstrumentComponent::set_gain,
		py::arg("gain"));
	_InstrumentComponent.def("get_gain", &H2Core::InstrumentComponent::get_gain);
	_InstrumentComponent.def_static("getMaxLayers", &H2Core::InstrumentComponent::getMaxLayers,
		"Returns #m_nMaxLayers.");
	_InstrumentComponent.def_static("setMaxLayers", &H2Core::InstrumentComponent::setMaxLayers,
		py::arg("layers"));
	_InstrumentComponent.def("toQString", &H2Core::InstrumentComponent::toQString,
		"Formatted string version for debugging purposes.",
		py::arg("sPrefix"),
		py::arg("bShort"));

	py::class_<H2Core::Instrument, H2Core::Object, std::shared_ptr<H2Core::Instrument>> _Instrument(m, "Instrument");
	_Instrument.def(py::init<const int, const QString &, std::shared_ptr<ADSR>>());
	_Instrument.def(py::init<std::shared_ptr<Instrument>>());
	_Instrument.def_static("class_name", &H2Core::Instrument::class_name);
	_Instrument.def_static("load_instrument", &H2Core::Instrument::load_instrument,
		"creates a new Instrument, loads samples from a given instrument within a given drumkit",
		py::arg("drumkit_name"),
		py::arg("instrument_name"),
		py::arg("lookup"));
	_Instrument.def("load_from", py::overload_cast<const QString &, const QString &, bool, Filesystem::Lookup>(&H2Core::Instrument::load_from),
		"loads instrument from a given instrument within a given drumkit into a `live` Instrument object.",
		py::arg("drumkit_name"),
		py::arg("instrument_name"),
		py::arg("is_live"),
		py::arg("lookup"));
	_Instrument.def("load_from", py::overload_cast<H2Core::Drumkit *, std::shared_ptr<Instrument>, bool>(&H2Core::Instrument::load_from),
		"loads instrument from a given instrument into a `live` Instrument object.",
		py::arg("drumkit"),
		py::arg("instrument"),
		py::arg("is_live"));
	_Instrument.def_static("load_from_static", py::overload_cast<H2Core::XMLNode *, const QString &, const QString &>(&H2Core::Instrument::load_from),
		"load an instrument from an XMLNode",
		py::arg("node"),
		py::arg("dk_path"),
		py::arg("dk_name"));
	_Instrument.def("load_samples", &H2Core::Instrument::load_samples,
		"Calls the InstrumentLayer::load_sample() member function of all layers of each component of the Instrument.");
	_Instrument.def("unload_samples", &H2Core::Instrument::unload_samples,
		"Calls the InstrumentLayer::unload_sample() member function of all layers of each component of the Instrument.");
	_Instrument.def("save_to", &H2Core::Instrument::save_to,
		"save the instrument within the given XMLNode",
		py::arg("node"),
		py::arg("component_id"));
	_Instrument.def("set_name", &H2Core::Instrument::set_name,
		"Sets the name of the Instrument #__name.",
		py::arg("name"));
	_Instrument.def("get_name", &H2Core::Instrument::get_name,
		"Access the name of the Instrument.");
	_Instrument.def("set_id", &H2Core::Instrument::set_id,
		"Sets #__id to id.",
		py::arg("id"));
	_Instrument.def("get_id", &H2Core::Instrument::get_id,
		"Returns #__id.");
	_Instrument.def("set_adsr", &H2Core::Instrument::set_adsr,
		"set the ADSR of the instrument",
		py::arg("adsr"));
	_Instrument.def("get_adsr", &H2Core::Instrument::get_adsr,
		"get the ADSR of the instrument");
	_Instrument.def("copy_adsr", &H2Core::Instrument::copy_adsr,
		"get a copy of the ADSR of the instrument");
	_Instrument.def("set_mute_group", &H2Core::Instrument::set_mute_group,
		"set the mute group of the instrument",
		py::arg("group"));
	_Instrument.def("get_mute_group", &H2Core::Instrument::get_mute_group,
		"get the mute group of the instrument");
	_Instrument.def("set_midi_out_channel", &H2Core::Instrument::set_midi_out_channel,
		"set the midi out channel of the instrument",
		py::arg("channel"));
	_Instrument.def("get_midi_out_channel", &H2Core::Instrument::get_midi_out_channel,
		"get the midi out channel of the instrument");
	_Instrument.def("set_midi_out_note", &H2Core::Instrument::set_midi_out_note,
		"set the midi out note of the instrument",
		py::arg("note"));
	_Instrument.def("get_midi_out_note", &H2Core::Instrument::get_midi_out_note,
		"get the midi out note of the instrument");
	_Instrument.def("set_muted", &H2Core::Instrument::set_muted,
		"set muted status of the instrument",
		py::arg("muted"));
	_Instrument.def("is_muted", &H2Core::Instrument::is_muted,
		"get muted status of the instrument");
	_Instrument.def("setPan", &H2Core::Instrument::setPan,
		"set pan of the instrument",
		py::arg("val"));
	_Instrument.def("setPanWithRangeFrom0To1", &H2Core::Instrument::setPanWithRangeFrom0To1,
		"set pan of the instrument, assuming the input range in [0;1]",
		py::arg("fVal"));
	_Instrument.def("getPan", &H2Core::Instrument::getPan,
		"get pan of the instrument");
	_Instrument.def("getPanWithRangeFrom0To1", &H2Core::Instrument::getPanWithRangeFrom0To1,
		"get pan of the instrument scaling and translating the range from [-1;1] to [0;1]");
	_Instrument.def("set_gain", &H2Core::Instrument::set_gain,
		"set gain of the instrument",
		py::arg("gain"));
	_Instrument.def("get_gain", &H2Core::Instrument::get_gain,
		"get gain of the instrument");
	_Instrument.def("set_volume", &H2Core::Instrument::set_volume,
		"set the volume of the instrument",
		py::arg("volume"));
	_Instrument.def("get_volume", &H2Core::Instrument::get_volume,
		"get the volume of the instrument");
	_Instrument.def("set_filter_active", &H2Core::Instrument::set_filter_active,
		"activate the filter of the instrument",
		py::arg("active"));
	_Instrument.def("is_filter_active", &H2Core::Instrument::is_filter_active,
		"get the status of the filter of the instrument");
	_Instrument.def("set_filter_resonance", &H2Core::Instrument::set_filter_resonance,
		"set the filter resonance of the instrument",
		py::arg("val"));
	_Instrument.def("get_filter_resonance", &H2Core::Instrument::get_filter_resonance,
		"get the filter resonance of the instrument");
	_Instrument.def("set_filter_cutoff", &H2Core::Instrument::set_filter_cutoff,
		"set the filter cutoff of the instrument",
		py::arg("val"));
	_Instrument.def("get_filter_cutoff", &H2Core::Instrument::get_filter_cutoff,
		"get the filter cutoff of the instrument");
	_Instrument.def("set_peak_l", &H2Core::Instrument::set_peak_l,
		"set the left peak of the instrument",
		py::arg("val"));
	_Instrument.def("get_peak_l", &H2Core::Instrument::get_peak_l,
		"get the left peak of the instrument");
	_Instrument.def("set_peak_r", &H2Core::Instrument::set_peak_r,
		"set the right peak of the instrument",
		py::arg("val"));
	_Instrument.def("get_peak_r", &H2Core::Instrument::get_peak_r,
		"get the right peak of the instrument");
	_Instrument.def("set_fx_level", &H2Core::Instrument::set_fx_level,
		"set the fx level of the instrument",
		py::arg("level"),
		py::arg("index"));
	_Instrument.def("get_fx_level", &H2Core::Instrument::get_fx_level,
		"get the fx level of the instrument",
		py::arg("index"));
	_Instrument.def("set_random_pitch_factor", &H2Core::Instrument::set_random_pitch_factor,
		"set the random pitch factor of the instrument",
		py::arg("val"));
	_Instrument.def("get_random_pitch_factor", &H2Core::Instrument::get_random_pitch_factor,
		"get the random pitch factor of the instrument");
	_Instrument.def("set_pitch_offset", &H2Core::Instrument::set_pitch_offset,
		"set the pitch offset of the instrument",
		py::arg("val"));
	_Instrument.def("get_pitch_offset", &H2Core::Instrument::get_pitch_offset,
		"get the pitch offset of the instrument");
	_Instrument.def("set_active", &H2Core::Instrument::set_active,
		"set the active status of the instrument",
		py::arg("active"));
	_Instrument.def("is_active", &H2Core::Instrument::is_active,
		"get the active status of the instrument");
	_Instrument.def("set_soloed", &H2Core::Instrument::set_soloed,
		"set the soloed status of the instrument",
		py::arg("soloed"));
	_Instrument.def("is_soloed", &H2Core::Instrument::is_soloed,
		"get the soloed status of the instrument");
	_Instrument.def("enqueue", &H2Core::Instrument::enqueue,
		"enqueue the instrument");
	_Instrument.def("dequeue", &H2Core::Instrument::dequeue,
		"dequeue the instrument");
	_Instrument.def("is_queued", &H2Core::Instrument::is_queued,
		"get the queued status of the instrument");
	_Instrument.def("set_stop_notes", &H2Core::Instrument::set_stop_notes,
		"set the stop notes status of the instrument",
		py::arg("stopnotes"));
	_Instrument.def("is_stop_notes", &H2Core::Instrument::is_stop_notes,
		"get the stop notes of the instrument");
	_Instrument.def("set_sample_selection_alg", &H2Core::Instrument::set_sample_selection_alg,
		py::arg("selected_algo"));
	_Instrument.def("sample_selection_alg", &H2Core::Instrument::sample_selection_alg);
	_Instrument.def("set_hihat_grp", &H2Core::Instrument::set_hihat_grp,
		py::arg("hihat_grp"));
	_Instrument.def("get_hihat_grp", &H2Core::Instrument::get_hihat_grp);
	_Instrument.def("set_lower_cc", &H2Core::Instrument::set_lower_cc,
		py::arg("message"));
	_Instrument.def("get_lower_cc", &H2Core::Instrument::get_lower_cc);
	_Instrument.def("set_higher_cc", &H2Core::Instrument::set_higher_cc,
		py::arg("message"));
	_Instrument.def("get_higher_cc", &H2Core::Instrument::get_higher_cc);
	_Instrument.def("set_drumkit_name", &H2Core::Instrument::set_drumkit_name,
		py::arg("name"));
	_Instrument.def("get_drumkit_name", &H2Core::Instrument::get_drumkit_name);
	_Instrument.def("set_is_preview_instrument", &H2Core::Instrument::set_is_preview_instrument,
		"Mark the instrument as hydrogen's preview instrument",
		py::arg("isPreview"));
	_Instrument.def("is_preview_instrument", &H2Core::Instrument::is_preview_instrument);
	_Instrument.def("set_is_metronome_instrument", &H2Core::Instrument::set_is_metronome_instrument,
		"Mark the instrument as metronome instrument",
		py::arg("isMetronome"));
	_Instrument.def("is_metronome_instrument", &H2Core::Instrument::is_metronome_instrument);
	_Instrument.def("get_components", &H2Core::Instrument::get_components);
	_Instrument.def("get_component", &H2Core::Instrument::get_component,
		py::arg("DrumkitComponentID"));
	_Instrument.def("set_apply_velocity", &H2Core::Instrument::set_apply_velocity,
		py::arg("apply_velocity"));
	_Instrument.def("get_apply_velocity", &H2Core::Instrument::get_apply_velocity);
	_Instrument.def("is_currently_exported", &H2Core::Instrument::is_currently_exported);
	_Instrument.def("set_currently_exported", &H2Core::Instrument::set_currently_exported,
		py::arg("isCurrentlyExported"));
	_Instrument.def("has_missing_samples", &H2Core::Instrument::has_missing_samples);
	_Instrument.def("set_missing_samples", &H2Core::Instrument::set_missing_samples,
		py::arg("bHasMissingSamples"));
	_Instrument.def("toQString", &H2Core::Instrument::toQString,
		"Formatted string version for debugging purposes.",
		py::arg("sPrefix"),
		py::arg("bShort"));
	_Instrument.def("__repr__",
	[](const H2Core::Instrument & instrument) {
  return "<Instrument \"" + instrument.get_name() + "\">";
}
);

	py::class_<H2Core::ADSR, H2Core::Object, std::shared_ptr<H2Core::ADSR>> _ADSR(m, "ADSR");
	_ADSR.def(py::init<unsigned int, unsigned int, float, unsigned int>());
	_ADSR.def(py::init<const std::shared_ptr<ADSR>>());
	_ADSR.def_static("class_name", &H2Core::ADSR::class_name);
	_ADSR.def("set_attack", &H2Core::ADSR::set_attack,
		"__attack setter",
		py::arg("value"));
	_ADSR.def("get_attack", &H2Core::ADSR::get_attack,
		"__attack accessor");
	_ADSR.def("set_decay", &H2Core::ADSR::set_decay,
		"__decay setter",
		py::arg("value"));
	_ADSR.def("get_decay", &H2Core::ADSR::get_decay,
		"__decay accessor");
	_ADSR.def("set_sustain", &H2Core::ADSR::set_sustain,
		"__sustain setter",
		py::arg("value"));
	_ADSR.def("get_sustain", &H2Core::ADSR::get_sustain,
		"__sustain accessor");
	_ADSR.def("set_release", &H2Core::ADSR::set_release,
		"__release setter",
		py::arg("value"));
	_ADSR.def("get_release", &H2Core::ADSR::get_release,
		"__release accessor");
	_ADSR.def("attack", &H2Core::ADSR::attack,
		"sets state to ATTACK");
	_ADSR.def("get_value", &H2Core::ADSR::get_value,
		"compute the value and return it",
		py::arg("step"));
	_ADSR.def("release", &H2Core::ADSR::release,
		"sets state to RELEASE, returns 0 if the state is IDLE, __value if the state is RELEASE, set state to RELEASE, save __release_value and return it.");
	_ADSR.def("toQString", &H2Core::ADSR::toQString,
		"Formatted string version for debugging purposes.",
		py::arg("sPrefix"),
		py::arg("bShort"));

	py::class_<H2Core::DrumkitComponent, H2Core::Object, std::shared_ptr<H2Core::DrumkitComponent>> _DrumkitComponent(m, "DrumkitComponent");
	_DrumkitComponent.def(py::init<const int, const QString &>());
	_DrumkitComponent.def(py::init<H2Core::DrumkitComponent *>());
	_DrumkitComponent.def_static("class_name", &H2Core::DrumkitComponent::class_name);
	_DrumkitComponent.def("save_to", &H2Core::DrumkitComponent::save_to,
		py::arg("node"));
	_DrumkitComponent.def_static("load_from_static", py::overload_cast<H2Core::XMLNode *, const QString &>(&H2Core::DrumkitComponent::load_from),
		py::arg("node"),
		py::arg("dk_path"));
	_DrumkitComponent.def("load_from", py::overload_cast<H2Core::DrumkitComponent *, bool>(&H2Core::DrumkitComponent::load_from),
		py::arg("component"),
		py::arg("is_live"));
	_DrumkitComponent.def("set_name", &H2Core::DrumkitComponent::set_name,
		"Sets the name of the DrumkitComponent #__name.",
		py::arg("name"));
	_DrumkitComponent.def("get_name", &H2Core::DrumkitComponent::get_name,
		"Access the name of the DrumkitComponent.");
	_DrumkitComponent.def("set_id", &H2Core::DrumkitComponent::set_id,
		py::arg("id"));
	_DrumkitComponent.def("get_id", &H2Core::DrumkitComponent::get_id);
	_DrumkitComponent.def("set_volume", &H2Core::DrumkitComponent::set_volume,
		py::arg("volume"));
	_DrumkitComponent.def("get_volume", &H2Core::DrumkitComponent::get_volume);
	_DrumkitComponent.def("set_muted", &H2Core::DrumkitComponent::set_muted,
		py::arg("muted"));
	_DrumkitComponent.def("is_muted", &H2Core::DrumkitComponent::is_muted);
	_DrumkitComponent.def("set_soloed", &H2Core::DrumkitComponent::set_soloed,
		py::arg("soloed"));
	_DrumkitComponent.def("is_soloed", &H2Core::DrumkitComponent::is_soloed);
	_DrumkitComponent.def("set_peak_l", &H2Core::DrumkitComponent::set_peak_l,
		py::arg("val"));
	_DrumkitComponent.def("get_peak_l", &H2Core::DrumkitComponent::get_peak_l);
	_DrumkitComponent.def("set_peak_r", &H2Core::DrumkitComponent::set_peak_r,
		py::arg("val"));
	_DrumkitComponent.def("get_peak_r", &H2Core::DrumkitComponent::get_peak_r);
	_DrumkitComponent.def("reset_outs", &H2Core::DrumkitComponent::reset_outs,
		py::arg("nFrames"));
	_DrumkitComponent.def("set_outs", &H2Core::DrumkitComponent::set_outs,
		py::arg("nBufferPos"),
		py::arg("valL"),
		py::arg("valR"));
	_DrumkitComponent.def("get_out_L", &H2Core::DrumkitComponent::get_out_L,
		py::arg("nBufferPos"));
	_DrumkitComponent.def("get_out_R", &H2Core::DrumkitComponent::get_out_R,
		py::arg("nBufferPos"));
	_DrumkitComponent.def("toQString", &H2Core::DrumkitComponent::toQString,
		"Formatted string version for debugging purposes.",
		py::arg("sPrefix"),
		py::arg("bShort"));
	_DrumkitComponent.def("__repr__",
	[](const H2Core::DrumkitComponent & dkc) {
  return "<DrumkitComponent \"" + dkc.get_name() + "\">";
}
);

	py::class_<H2Core::Drumkit, H2Core::Object, std::shared_ptr<H2Core::Drumkit>> _Drumkit(m, "Drumkit");
	_Drumkit.def(py::init<>());
	_Drumkit.def(py::init<H2Core::Drumkit *>());
	_Drumkit.def_static("class_name", &H2Core::Drumkit::class_name);
	_Drumkit.def_static("load", &H2Core::Drumkit::load,
		"Load drumkit information from a directory.",
		py::arg("dk_dir"),
		py::arg("load_samples"));
	_Drumkit.def_static("load_by_name", &H2Core::Drumkit::load_by_name,
		"Simple wrapper for load() used with the drumkit's name instead of its directory.",
		py::arg("dk_name"),
		py::arg("load_samples"),
		py::arg("lookup"));
	_Drumkit.def_static("load_file", &H2Core::Drumkit::load_file,
		"Load a Drumkit from a file.",
		py::arg("dk_path"),
		py::arg("load_samples"));
	_Drumkit.def("load_samples", &H2Core::Drumkit::load_samples,
		"Calls the InstrumentList::load_samples() member function of #__instruments.");
	_Drumkit.def("unload_samples", &H2Core::Drumkit::unload_samples,
		"Calls the InstrumentList::unload_samples() member function of #__instruments.");
	_Drumkit.def_static("upgrade_drumkit", &H2Core::Drumkit::upgrade_drumkit,
		"Saves the current drumkit to dk_path, but makes a backup. This is used when the drumkit did not comply to our xml schema.",
		py::arg("pDrumkit"),
		py::arg("dk_path"));
	_Drumkit.def_static("user_drumkit_exists", &H2Core::Drumkit::user_drumkit_exists,
		"check if a user drumkit with the given name already exists",
		py::arg("dk_path"));
	_Drumkit.def("save", py::overload_cast<bool>(&H2Core::Drumkit::save),
		"save a drumkit, xml file and samples",
		py::arg("overwrite"));
	_Drumkit.def("save", py::overload_cast<const QString &, bool>(&H2Core::Drumkit::save),
		"save a drumkit, xml file and samples neither #__path nor #__name are updated",
		py::arg("dk_dir"),
		py::arg("overwrite"));
	_Drumkit.def_static("save_static", py::overload_cast<const QString &, const QString &, const QString &, const QString &, const QString &, const QString &, H2Core::InstrumentList *, std::vector<DrumkitComponent *> *, bool>(&H2Core::Drumkit::save),
		"save a drumkit using given parameters and an instrument list",
		py::arg("sName"),
		py::arg("sAuthor"),
		py::arg("sInfo"),
		py::arg("sLicense"),
		py::arg("sImage"),
		py::arg("sImageLicense"),
		py::arg("pInstruments"),
		py::arg("pComponents"),
		py::arg("bOverwrite"));
	_Drumkit.def("save_file", &H2Core::Drumkit::save_file,
		"save a drumkit into an xml file",
		py::arg("dk_path"),
		py::arg("overwrite"),
		py::arg("component_id"));
	_Drumkit.def("save_samples", &H2Core::Drumkit::save_samples,
		"save a drumkit instruments samples into a directory",
		py::arg("dk_dir"),
		py::arg("overwrite"));
	_Drumkit.def("save_image", &H2Core::Drumkit::save_image,
		"save the drumkit image into the new directory",
		py::arg("dk_dir"),
		py::arg("overwrite"));
	_Drumkit.def_static("install", &H2Core::Drumkit::install,
		"install a drumkit from a filename",
		py::arg("path"));
	_Drumkit.def_static("remove", &H2Core::Drumkit::remove,
		"remove a drumkit from the disk",
		py::arg("dk_name"),
		py::arg("lookup"));
	_Drumkit.def("set_instruments", &H2Core::Drumkit::set_instruments,
		"set __instruments, delete existing one",
		py::arg("instruments"));
	_Drumkit.def("get_instruments", &H2Core::Drumkit::get_instruments,
		"returns #__instruments",
	py::return_value_policy::reference_internal);
	_Drumkit.def("set_path", &H2Core::Drumkit::set_path,
		"#__path setter",
		py::arg("path"));
	_Drumkit.def("get_path", &H2Core::Drumkit::get_path,
		"#__path accessor");
	_Drumkit.def("set_name", &H2Core::Drumkit::set_name,
		"#__name setter",
		py::arg("name"));
	_Drumkit.def("get_name", &H2Core::Drumkit::get_name,
		"#__name accessor");
	_Drumkit.def("set_author", &H2Core::Drumkit::set_author,
		"#__author setter",
		py::arg("author"));
	_Drumkit.def("get_author", &H2Core::Drumkit::get_author,
		"#__author accessor");
	_Drumkit.def("set_info", &H2Core::Drumkit::set_info,
		"#__info setter",
		py::arg("info"));
	_Drumkit.def("get_info", &H2Core::Drumkit::get_info,
		"#__info accessor");
	_Drumkit.def("set_license", &H2Core::Drumkit::set_license,
		"#__license setter",
		py::arg("license"));
	_Drumkit.def("get_license", &H2Core::Drumkit::get_license,
		"#__license accessor");
	_Drumkit.def("set_image", &H2Core::Drumkit::set_image,
		"#__image setter",
		py::arg("image"));
	_Drumkit.def("get_image", &H2Core::Drumkit::get_image,
		"#__image accessor");
	_Drumkit.def("set_image_license", &H2Core::Drumkit::set_image_license,
		"#__imageLicense setter",
		py::arg("imageLicense"));
	_Drumkit.def("get_image_license", &H2Core::Drumkit::get_image_license,
		"#__imageLicense accessor");
	_Drumkit.def("samples_loaded", &H2Core::Drumkit::samples_loaded,
		"return true if the samples are loaded");
	_Drumkit.def("dump", &H2Core::Drumkit::dump);
	_Drumkit.def("isUserDrumkit", &H2Core::Drumkit::isUserDrumkit,
		"Returns Whether the associated files are located in the user or the systems drumkit folder.");
	_Drumkit.def("get_components", &H2Core::Drumkit::get_components);
	_Drumkit.def("set_components", &H2Core::Drumkit::set_components,
		py::arg("components"));
	_Drumkit.def("toQString", &H2Core::Drumkit::toQString,
		"Formatted string version for debugging purposes.",
		py::arg("sPrefix"),
		py::arg("bShort"));
	_Drumkit.def("__repr__",
	[](const H2Core::Drumkit & drumkit) {
  return "<Drumkit \"" + drumkit.get_name() + "\">";
}
);

	py::class_<H2Core::XMLNode, H2Core::Object, std::shared_ptr<H2Core::XMLNode>> _XMLNode(m, "XMLNode");
	_XMLNode.def(py::init<>());
	_XMLNode.def(py::init<QDomNode>());
	_XMLNode.def_static("class_name", &H2Core::XMLNode::class_name);
	_XMLNode.def("createNode", &H2Core::XMLNode::createNode,
		"create a new XMLNode that has to be appended into de XMLDoc",
		py::arg("name"));
	_XMLNode.def("read_int", &H2Core::XMLNode::read_int,
		"reads an integer stored into a child node",
		py::arg("node"),
		py::arg("default_value"),
		py::arg("inexistent_ok"),
		py::arg("empty_ok"));
	_XMLNode.def("read_bool", &H2Core::XMLNode::read_bool,
		"reads a boolean stored into a child node",
		py::arg("node"),
		py::arg("default_value"),
		py::arg("inexistent_ok"),
		py::arg("empty_ok"));
	_XMLNode.def("read_float", py::overload_cast<const QString &, float, bool, bool>(&H2Core::XMLNode::read_float),
		"reads a float stored into a child node",
		py::arg("node"),
		py::arg("default_value"),
		py::arg("inexistent_ok"),
		py::arg("empty_ok"));
	_XMLNode.def("read_float", py::overload_cast<const QString &, float, bool *, bool, bool>(&H2Core::XMLNode::read_float),
		py::arg("node"),
		py::arg("default_value"),
		py::arg("pFound"),
		py::arg("inexistent_ok"),
		py::arg("empty_ok"));
	_XMLNode.def("read_string", &H2Core::XMLNode::read_string,
		"reads a string stored into a child node",
		py::arg("node"),
		py::arg("default_value"),
		py::arg("inexistent_ok"),
		py::arg("empty_ok"));
	_XMLNode.def("read_attribute", &H2Core::XMLNode::read_attribute,
		"reads an attribute from the node",
		py::arg("attribute"),
		py::arg("default_value"),
		py::arg("inexistent_ok"),
		py::arg("empty_ok"));
	_XMLNode.def("read_text", &H2Core::XMLNode::read_text,
		"reads the text (content) from the node",
		py::arg("empty_ok"));
	_XMLNode.def("write_int", &H2Core::XMLNode::write_int,
		"write an integer into a child node",
		py::arg("node"),
		py::arg("value"));
	_XMLNode.def("write_bool", &H2Core::XMLNode::write_bool,
		"write a boolean into a child node",
		py::arg("node"),
		py::arg("value"));
	_XMLNode.def("write_float", &H2Core::XMLNode::write_float,
		"write a float into a child node",
		py::arg("node"),
		py::arg("value"));
	_XMLNode.def("write_string", &H2Core::XMLNode::write_string,
		"write a string into a child node",
		py::arg("node"),
		py::arg("value"));
	_XMLNode.def("write_attribute", &H2Core::XMLNode::write_attribute,
		"write a string as an attribute of the node",
		py::arg("attribute"),
		py::arg("value"));

	// enum log_levels
	py::enum_<H2Core::Logger::log_levels>(_Logger, "log_levels")
		.value("None", H2Core::Logger::log_levels::None)
		.value("Error", H2Core::Logger::log_levels::Error)
		.value("Warning", H2Core::Logger::log_levels::Warning)
		.value("Info", H2Core::Logger::log_levels::Info)
		.value("Debug", H2Core::Logger::log_levels::Debug)
		.value("Constructors", H2Core::Logger::log_levels::Constructors)
		.value("AELockTracing", H2Core::Logger::log_levels::AELockTracing);

	// enum MidiMessageType
	py::enum_<H2Core::MidiMessage::MidiMessageType>(_MidiMessage, "MidiMessageType")
		.value("UNKNOWN", H2Core::MidiMessage::MidiMessageType::UNKNOWN)
		.value("SYSEX", H2Core::MidiMessage::MidiMessageType::SYSEX)
		.value("NOTE_ON", H2Core::MidiMessage::MidiMessageType::NOTE_ON)
		.value("NOTE_OFF", H2Core::MidiMessage::MidiMessageType::NOTE_OFF)
		.value("POLYPHONIC_KEY_PRESSURE", H2Core::MidiMessage::MidiMessageType::POLYPHONIC_KEY_PRESSURE)
		.value("CONTROL_CHANGE", H2Core::MidiMessage::MidiMessageType::CONTROL_CHANGE)
		.value("PROGRAM_CHANGE", H2Core::MidiMessage::MidiMessageType::PROGRAM_CHANGE)
		.value("CHANNEL_PRESSURE", H2Core::MidiMessage::MidiMessageType::CHANNEL_PRESSURE)
		.value("PITCH_WHEEL", H2Core::MidiMessage::MidiMessageType::PITCH_WHEEL)
		.value("SYSTEM_EXCLUSIVE", H2Core::MidiMessage::MidiMessageType::SYSTEM_EXCLUSIVE)
		.value("START", H2Core::MidiMessage::MidiMessageType::START)
		.value("CONTINUE", H2Core::MidiMessage::MidiMessageType::CONTINUE)
		.value("STOP", H2Core::MidiMessage::MidiMessageType::STOP)
		.value("SONG_POS", H2Core::MidiMessage::MidiMessageType::SONG_POS)
		.value("QUARTER_FRAME", H2Core::MidiMessage::MidiMessageType::QUARTER_FRAME);

	py::class_<H2Core::H2Exception, std::runtime_error, std::shared_ptr<H2Core::H2Exception>> _H2Exception(m, "H2Exception");
	_H2Exception.def(py::init<const QString &>());

	py::class_<H2Core::PatternList, H2Core::Object, H2Core::AudioEngineLocking, std::shared_ptr<H2Core::PatternList>> _PatternList(m, "PatternList");
	_PatternList.def(py::init<>());
	_PatternList.def(py::init<H2Core::PatternList *>());
	_PatternList.def_static("class_name", &H2Core::PatternList::class_name);
	_PatternList.def("size", &H2Core::PatternList::size,
		"returns the numbers of patterns");
	_PatternList.def("operator<<", &H2Core::PatternList::operator<<,
		"add a pattern to the list",
		py::arg("pattern"));
	_PatternList.def("operator[]", &H2Core::PatternList::operator[],
		"get a pattern from the list",
		py::arg("idx"));
	_PatternList.def("add", &H2Core::PatternList::add,
		"add a pattern to the list",
		py::arg("pattern"));
	_PatternList.def("insert", &H2Core::PatternList::insert,
		"insert a pattern into the list",
		py::arg("idx"),
		py::arg("pattern"));
	_PatternList.def("get", py::overload_cast<int>(&H2Core::PatternList::get),
		"get a pattern from the list",
		py::arg("idx"));
	_PatternList.def("get", py::overload_cast<int>(&H2Core::PatternList::get),
		py::arg("idx"));
	_PatternList.def("del", py::overload_cast<int>(&H2Core::PatternList::del),
		"remove the pattern at a given index, does not delete it",
		py::arg("idx"));
	_PatternList.def("del", py::overload_cast<H2Core::Pattern *>(&H2Core::PatternList::del),
		"remove a pattern from the list, does not delete it",
		py::arg("pattern"));
	_PatternList.def("index", &H2Core::PatternList::index,
		"get the index of the pattern within the patterns",
		py::arg("pattern"));
	_PatternList.def("replace", &H2Core::PatternList::replace,
		"replace the pattern at a given index with a new one",
		py::arg("idx"),
		py::arg("pattern"));
	_PatternList.def("clear", &H2Core::PatternList::clear,
		"empty the pattern list");
	_PatternList.def("set_to_old", &H2Core::PatternList::set_to_old,
		"mark all patterns as old");
	_PatternList.def("find", &H2Core::PatternList::find,
		"find a pattern within the patterns",
		py::arg("name"));
	_PatternList.def("swap", &H2Core::PatternList::swap,
		"swap the patterns of two different indexes",
		py::arg("idx_a"),
		py::arg("idx_b"));
	_PatternList.def("move", &H2Core::PatternList::move,
		"move a pattern from a position to another",
		py::arg("idx_a"),
		py::arg("idx_b"));
	_PatternList.def("flattened_virtual_patterns_compute", &H2Core::PatternList::flattened_virtual_patterns_compute,
		"call compute_flattened_virtual_patterns on each pattern");
	_PatternList.def("virtual_pattern_del", &H2Core::PatternList::virtual_pattern_del,
		"call del_virtual_pattern on each pattern",
		py::arg("pattern"));
	_PatternList.def("check_name", &H2Core::PatternList::check_name,
		"check if a pattern with name patternName already exists in this list",
		py::arg("patternName"),
		py::arg("ignore"));
	_PatternList.def("find_unused_pattern_name", &H2Core::PatternList::find_unused_pattern_name,
		"find unused patternName",
		py::arg("sourceName"),
		py::arg("ignore"));
	_PatternList.def("longest_pattern_length", &H2Core::PatternList::longest_pattern_length,
		"Get the length of the longest pattern in the list");
	_PatternList.def("toQString", &H2Core::PatternList::toQString,
		"Formatted string version for debugging purposes.",
		py::arg("sPrefix"),
		py::arg("bShort"));

	// enum NameFormat
	py::enum_<QColor::NameFormat>(_QColor, "NameFormat")
		.value("HexRgb", QColor::NameFormat::HexRgb)
		.value("HexArgb", QColor::NameFormat::HexArgb);

	// enum Spec
	py::enum_<QColor::Spec>(_QColor, "Spec")
		.value("Invalid", QColor::Spec::Invalid)
		.value("Rgb", QColor::Spec::Rgb)
		.value("Hsv", QColor::Spec::Hsv)
		.value("Cmyk", QColor::Spec::Cmyk)
		.value("Hsl", QColor::Spec::Hsl)
		.value("ExtendedRgb", QColor::Spec::ExtendedRgb);

	// enum Octave
	py::enum_<H2Core::Note::Octave>(_Note, "Octave")
		.value("P8Z", H2Core::Note::Octave::P8Z)
		.value("P8Y", H2Core::Note::Octave::P8Y)
		.value("P8X", H2Core::Note::Octave::P8X)
		.value("P8", H2Core::Note::Octave::P8)
		.value("P8A", H2Core::Note::Octave::P8A)
		.value("P8B", H2Core::Note::Octave::P8B)
		.value("P8C", H2Core::Note::Octave::P8C);

	// enum Key
	py::enum_<H2Core::Note::Key>(_Note, "Key")
		.value("C", H2Core::Note::Key::C)
		.value("Cs", H2Core::Note::Key::Cs)
		.value("D", H2Core::Note::Key::D)
		.value("Ef", H2Core::Note::Key::Ef)
		.value("E", H2Core::Note::Key::E)
		.value("F", H2Core::Note::Key::F)
		.value("Fs", H2Core::Note::Key::Fs)
		.value("G", H2Core::Note::Key::G)
		.value("Af", H2Core::Note::Key::Af)
		.value("A", H2Core::Note::Key::A)
		.value("Bf", H2Core::Note::Key::Bf)
		.value("B", H2Core::Note::Key::B);

	// enum PAN_LAW_TYPES
	py::enum_<H2Core::Sampler::PAN_LAW_TYPES>(_Sampler, "PAN_LAW_TYPES")
		.value("RATIO_STRAIGHT_POLYGONAL", H2Core::Sampler::PAN_LAW_TYPES::RATIO_STRAIGHT_POLYGONAL)
		.value("RATIO_CONST_POWER", H2Core::Sampler::PAN_LAW_TYPES::RATIO_CONST_POWER)
		.value("RATIO_CONST_SUM", H2Core::Sampler::PAN_LAW_TYPES::RATIO_CONST_SUM)
		.value("LINEAR_STRAIGHT_POLYGONAL", H2Core::Sampler::PAN_LAW_TYPES::LINEAR_STRAIGHT_POLYGONAL)
		.value("LINEAR_CONST_POWER", H2Core::Sampler::PAN_LAW_TYPES::LINEAR_CONST_POWER)
		.value("LINEAR_CONST_SUM", H2Core::Sampler::PAN_LAW_TYPES::LINEAR_CONST_SUM)
		.value("POLAR_STRAIGHT_POLYGONAL", H2Core::Sampler::PAN_LAW_TYPES::POLAR_STRAIGHT_POLYGONAL)
		.value("POLAR_CONST_POWER", H2Core::Sampler::PAN_LAW_TYPES::POLAR_CONST_POWER)
		.value("POLAR_CONST_SUM", H2Core::Sampler::PAN_LAW_TYPES::POLAR_CONST_SUM)
		.value("QUADRATIC_STRAIGHT_POLYGONAL", H2Core::Sampler::PAN_LAW_TYPES::QUADRATIC_STRAIGHT_POLYGONAL)
		.value("QUADRATIC_CONST_POWER", H2Core::Sampler::PAN_LAW_TYPES::QUADRATIC_CONST_POWER)
		.value("QUADRATIC_CONST_SUM", H2Core::Sampler::PAN_LAW_TYPES::QUADRATIC_CONST_SUM)
		.value("LINEAR_CONST_K_NORM", H2Core::Sampler::PAN_LAW_TYPES::LINEAR_CONST_K_NORM)
		.value("RATIO_CONST_K_NORM", H2Core::Sampler::PAN_LAW_TYPES::RATIO_CONST_K_NORM)
		.value("POLAR_CONST_K_NORM", H2Core::Sampler::PAN_LAW_TYPES::POLAR_CONST_K_NORM)
		.value("QUADRATIC_CONST_K_NORM", H2Core::Sampler::PAN_LAW_TYPES::QUADRATIC_CONST_K_NORM);

	py::class_<H2Core::PortMidiDriver, std::shared_ptr<H2Core::PortMidiDriver>> _PortMidiDriver(m, "PortMidiDriver");
	_PortMidiDriver.def(py::init<>());
	_PortMidiDriver.def_readwrite("m_pMidiIn", &H2Core::PortMidiDriver::m_pMidiIn);
	_PortMidiDriver.def_readwrite("m_pMidiOut", &H2Core::PortMidiDriver::m_pMidiOut);
	_PortMidiDriver.def_readwrite("m_bRunning", &H2Core::PortMidiDriver::m_bRunning);
	_PortMidiDriver.def_static("class_name", &H2Core::PortMidiDriver::class_name);
	_PortMidiDriver.def("open", &H2Core::PortMidiDriver::open);
	_PortMidiDriver.def("close", &H2Core::PortMidiDriver::close);
	_PortMidiDriver.def("getInputPortList", &H2Core::PortMidiDriver::getInputPortList);
	_PortMidiDriver.def("getOutputPortList", &H2Core::PortMidiDriver::getOutputPortList);
	_PortMidiDriver.def("handleQueueNote", &H2Core::PortMidiDriver::handleQueueNote,
		py::arg("pNote"));
	_PortMidiDriver.def("handleQueueNoteOff", &H2Core::PortMidiDriver::handleQueueNoteOff,
		py::arg("channel"),
		py::arg("key"),
		py::arg("velocity"));
	_PortMidiDriver.def("handleQueueAllNoteOff", &H2Core::PortMidiDriver::handleQueueAllNoteOff);
	_PortMidiDriver.def("handleOutgoingControlChange", &H2Core::PortMidiDriver::handleOutgoingControlChange,
		py::arg("param"),
		py::arg("value"),
		py::arg("channel"));

	py::class_<H2Core::JackMidiDriver, std::shared_ptr<H2Core::JackMidiDriver>> _JackMidiDriver(m, "JackMidiDriver");
	_JackMidiDriver.def(py::init<>());
	_JackMidiDriver.def_static("class_name", &H2Core::JackMidiDriver::class_name);
	_JackMidiDriver.def("open", &H2Core::JackMidiDriver::open);
	_JackMidiDriver.def("close", &H2Core::JackMidiDriver::close);
	_JackMidiDriver.def("getInputPortList", &H2Core::JackMidiDriver::getInputPortList);
	_JackMidiDriver.def("getOutputPortList", &H2Core::JackMidiDriver::getOutputPortList);
	_JackMidiDriver.def("getPortInfo", &H2Core::JackMidiDriver::getPortInfo,
		py::arg("sPortName"),
		py::arg("nClient"),
		py::arg("nPort"));
	_JackMidiDriver.def("JackMidiWrite", &H2Core::JackMidiDriver::JackMidiWrite,
		py::arg("nframes"));
	_JackMidiDriver.def("JackMidiRead", &H2Core::JackMidiDriver::JackMidiRead,
		py::arg("nframes"));
	_JackMidiDriver.def("handleQueueNote", &H2Core::JackMidiDriver::handleQueueNote,
		py::arg("pNote"));
	_JackMidiDriver.def("handleQueueNoteOff", &H2Core::JackMidiDriver::handleQueueNoteOff,
		py::arg("channel"),
		py::arg("key"),
		py::arg("velocity"));
	_JackMidiDriver.def("handleQueueAllNoteOff", &H2Core::JackMidiDriver::handleQueueAllNoteOff);
	_JackMidiDriver.def("handleOutgoingControlChange", &H2Core::JackMidiDriver::handleOutgoingControlChange,
		py::arg("param"),
		py::arg("value"),
		py::arg("channel"));

	py::class_<H2Core::AlsaMidiDriver, std::shared_ptr<H2Core::AlsaMidiDriver>> _AlsaMidiDriver(m, "AlsaMidiDriver");
	_AlsaMidiDriver.def(py::init<>());
	_AlsaMidiDriver.def_static("class_name", &H2Core::AlsaMidiDriver::class_name);
	_AlsaMidiDriver.def("open", &H2Core::AlsaMidiDriver::open);
	_AlsaMidiDriver.def("close", &H2Core::AlsaMidiDriver::close);
	_AlsaMidiDriver.def("getInputPortList", &H2Core::AlsaMidiDriver::getInputPortList);
	_AlsaMidiDriver.def("getOutputPortList", &H2Core::AlsaMidiDriver::getOutputPortList);
	// [banned] _AlsaMidiDriver.def("midi_action", &H2Core::AlsaMidiDriver::midi_action,
	// [banned] 	py::arg("seq_handle"));
	_AlsaMidiDriver.def("getPortInfo", &H2Core::AlsaMidiDriver::getPortInfo,
		py::arg("sPortName"),
		py::arg("nClient"),
		py::arg("nPort"));
	_AlsaMidiDriver.def("handleQueueNote", &H2Core::AlsaMidiDriver::handleQueueNote,
		py::arg("pNote"));
	_AlsaMidiDriver.def("handleQueueNoteOff", &H2Core::AlsaMidiDriver::handleQueueNoteOff,
		py::arg("channel"),
		py::arg("key"),
		py::arg("velocity"));
	_AlsaMidiDriver.def("handleQueueAllNoteOff", &H2Core::AlsaMidiDriver::handleQueueAllNoteOff);
	_AlsaMidiDriver.def("handleOutgoingControlChange", &H2Core::AlsaMidiDriver::handleOutgoingControlChange,
		py::arg("param"),
		py::arg("value"),
		py::arg("channel"));

	py::class_<H2Core::PortAudioDriver, std::shared_ptr<H2Core::PortAudioDriver>> _PortAudioDriver(m, "PortAudioDriver");
	_PortAudioDriver.def(py::init<H2Core::audioProcessCallback>());
	_PortAudioDriver.def_readwrite("m_pOut_L", &H2Core::PortAudioDriver::m_pOut_L);
	_PortAudioDriver.def_readwrite("m_pOut_R", &H2Core::PortAudioDriver::m_pOut_R);
	_PortAudioDriver.def_readwrite("m_nBufferSize", &H2Core::PortAudioDriver::m_nBufferSize);
	_PortAudioDriver.def_static("class_name", &H2Core::PortAudioDriver::class_name);
	_PortAudioDriver.def("init", &H2Core::PortAudioDriver::init,
		py::arg("nBufferSize"));
	_PortAudioDriver.def("connect", &H2Core::PortAudioDriver::connect);
	_PortAudioDriver.def("disconnect", &H2Core::PortAudioDriver::disconnect);
	_PortAudioDriver.def("getBufferSize", &H2Core::PortAudioDriver::getBufferSize);
	_PortAudioDriver.def("getSampleRate", &H2Core::PortAudioDriver::getSampleRate);
	_PortAudioDriver.def("getOut_L", &H2Core::PortAudioDriver::getOut_L);
	_PortAudioDriver.def("getOut_R", &H2Core::PortAudioDriver::getOut_R);
	_PortAudioDriver.def("updateTransportInfo", &H2Core::PortAudioDriver::updateTransportInfo);
	_PortAudioDriver.def("play", &H2Core::PortAudioDriver::play);
	_PortAudioDriver.def("stop", &H2Core::PortAudioDriver::stop);
	_PortAudioDriver.def("locate", &H2Core::PortAudioDriver::locate,
		py::arg("nFrame"));
	_PortAudioDriver.def("setBpm", &H2Core::PortAudioDriver::setBpm,
		py::arg("fBPM"));

	py::class_<H2Core::NullDriver, std::shared_ptr<H2Core::NullDriver>> _NullDriver(m, "NullDriver");
	_NullDriver.def(py::init<H2Core::audioProcessCallback>());
	_NullDriver.def_static("class_name", &H2Core::NullDriver::class_name);
	_NullDriver.def("init", &H2Core::NullDriver::init,
		py::arg("nBufferSize"));
	_NullDriver.def("connect", &H2Core::NullDriver::connect);
	_NullDriver.def("disconnect", &H2Core::NullDriver::disconnect);
	_NullDriver.def("getBufferSize", &H2Core::NullDriver::getBufferSize);
	_NullDriver.def("getSampleRate", &H2Core::NullDriver::getSampleRate);
	_NullDriver.def("getOut_L", &H2Core::NullDriver::getOut_L);
	_NullDriver.def("getOut_R", &H2Core::NullDriver::getOut_R);
	_NullDriver.def("play", &H2Core::NullDriver::play);
	_NullDriver.def("stop", &H2Core::NullDriver::stop);
	_NullDriver.def("locate", &H2Core::NullDriver::locate,
		py::arg("nFrame"));
	_NullDriver.def("updateTransportInfo", &H2Core::NullDriver::updateTransportInfo);
	_NullDriver.def("setBpm", &H2Core::NullDriver::setBpm,
		py::arg("fBPM"));

	py::class_<H2Core::JackAudioDriver, std::shared_ptr<H2Core::JackAudioDriver>> _JackAudioDriver(m, "JackAudioDriver");
	_JackAudioDriver.def(py::init<JackProcessCallback>());
	_JackAudioDriver.def_readwrite("m_currentPos", &H2Core::JackAudioDriver::m_currentPos);
	_JackAudioDriver.def_static("class_name", &H2Core::JackAudioDriver::class_name);
	_JackAudioDriver.def("connect", &H2Core::JackAudioDriver::connect,
		"Connects to output ports via the JACK server.");
	_JackAudioDriver.def("disconnect", &H2Core::JackAudioDriver::disconnect,
		"Disconnects the JACK client of the Hydrogen from the JACK server.");
	_JackAudioDriver.def("deactivate", &H2Core::JackAudioDriver::deactivate,
		"Deactivates the JACK client of Hydrogen and disconnects all ports belonging to it.");
	_JackAudioDriver.def("getBufferSize", &H2Core::JackAudioDriver::getBufferSize,
		"Returns Global variable #jackServerBufferSize.");
	_JackAudioDriver.def("getSampleRate", &H2Core::JackAudioDriver::getSampleRate,
		"Returns Global variable #jackServerSampleRate.");
	_JackAudioDriver.def("clearPerTrackAudioBuffers", &H2Core::JackAudioDriver::clearPerTrackAudioBuffers,
		"Resets the buffers contained in #m_pTrackOutputPortsL and #m_pTrackOutputPortsR.",
		py::arg("nFrames"));
	_JackAudioDriver.def("makeTrackOutputs", &H2Core::JackAudioDriver::makeTrackOutputs,
		"Creates per component output ports for each instrument.",
		py::arg("pSong"));
	_JackAudioDriver.def("setConnectDefaults", &H2Core::JackAudioDriver::setConnectDefaults,
		py::arg("flag"));
	_JackAudioDriver.def("getConnectDefaults", &H2Core::JackAudioDriver::getConnectDefaults,
		"Returns #m_bConnectDefaults");
	_JackAudioDriver.def("getOut_L", &H2Core::JackAudioDriver::getOut_L,
		"Get content in the left stereo output port.");
	_JackAudioDriver.def("getOut_R", &H2Core::JackAudioDriver::getOut_R,
		"Get content in the right stereo output port.");
	_JackAudioDriver.def("getTrackOut_L", py::overload_cast<unsigned int>(&H2Core::JackAudioDriver::getTrackOut_L),
		"Get content of left output port of a specific track.",
		py::arg("nTrack"));
	_JackAudioDriver.def("getTrackOut_L", py::overload_cast<std::shared_ptr<Instrument>, std::shared_ptr<InstrumentComponent>>(&H2Core::JackAudioDriver::getTrackOut_L),
		"Convenience function looking up the track number of a component of an instrument using in #m_trackMap using their IDs Instrument::__id and InstrumentComponent::__related_drumkit_componentID. Using the track number it then calls getTrackOut_L( unsigned ) and returns its result.",
		py::arg("instr"),
		py::arg("pCompo"));
	_JackAudioDriver.def("getTrackOut_R", py::overload_cast<unsigned int>(&H2Core::JackAudioDriver::getTrackOut_R),
		"Get content of right output port of a specific track.",
		py::arg("nTrack"));
	_JackAudioDriver.def("getTrackOut_R", py::overload_cast<std::shared_ptr<Instrument>, std::shared_ptr<InstrumentComponent>>(&H2Core::JackAudioDriver::getTrackOut_R),
		"Convenience function looking up the track number of a component of an instrument using in #m_trackMap using their IDs Instrument::__id and InstrumentComponent::__related_drumkit_componentID. Using the track number it then calls getTrackOut_R( unsigned ) and returns its result.",
		py::arg("instr"),
		py::arg("pCompo"));
	_JackAudioDriver.def("init", &H2Core::JackAudioDriver::init,
		"Initializes the JACK audio driver.",
		py::arg("bufferSize"));
	_JackAudioDriver.def("play", &H2Core::JackAudioDriver::play,
		"Starts the JACK transport.");
	_JackAudioDriver.def("stop", &H2Core::JackAudioDriver::stop,
		"Stops the JACK transport.");
	_JackAudioDriver.def("locate", &H2Core::JackAudioDriver::locate,
		"Re-positions the transport position to nFrame.",
		py::arg("nFrame"));
	_JackAudioDriver.def("updateTransportInfo", &H2Core::JackAudioDriver::updateTransportInfo,
		"Updating the local instance of the TransportInfo AudioOutput::m_transport.");
	_JackAudioDriver.def("setBpm", &H2Core::JackAudioDriver::setBpm,
		"Set the tempo stored TransportInfo::m_fBPM of the local instance of the TransportInfo AudioOutput::m_transport.",
		py::arg("fBPM"));
	_JackAudioDriver.def("calculateFrameOffset", &H2Core::JackAudioDriver::calculateFrameOffset,
		"Calculates the difference between the true transport position and the internal one.",
		py::arg("oldFrame"));
	_JackAudioDriver.def("initTimebaseMaster", &H2Core::JackAudioDriver::initTimebaseMaster,
		"Registers Hydrogen as JACK timebase master.");
	_JackAudioDriver.def("releaseTimebaseMaster", &H2Core::JackAudioDriver::releaseTimebaseMaster,
		"Calls _jack_release_timebase()_ (jack/transport.h) to release Hydrogen from the JACK timebase master responsibilities. This causes the JackTimebaseCallback() callback function to not be called by the JACK server anymore.");
	_JackAudioDriver.def("getTimebaseState", &H2Core::JackAudioDriver::getTimebaseState,
		"Returns #m_timebaseState");
	_JackAudioDriver.def_static("jackDriverSampleRate", &H2Core::JackAudioDriver::jackDriverSampleRate,
		"Callback function for the JACK audio server to set the sample rate #jackServerSampleRate and prints a message to the #__INFOLOG, which has to be included via a Logger instance in the provided param.",
		py::arg("nframes"),
		py::arg("param"));
	_JackAudioDriver.def_static("jackDriverBufferSize", &H2Core::JackAudioDriver::jackDriverBufferSize,
		"Callback function for the JACK audio server to set the buffer size #jackServerBufferSize.",
		py::arg("nframes"),
		py::arg("arg"));

	py::class_<H2Core::FakeDriver, std::shared_ptr<H2Core::FakeDriver>> _FakeDriver(m, "FakeDriver");
	_FakeDriver.def(py::init<H2Core::audioProcessCallback>());
	_FakeDriver.def_static("class_name", &H2Core::FakeDriver::class_name);
	_FakeDriver.def("init", &H2Core::FakeDriver::init,
		py::arg("nBufferSize"));
	_FakeDriver.def("connect", &H2Core::FakeDriver::connect);
	_FakeDriver.def("disconnect", &H2Core::FakeDriver::disconnect);
	_FakeDriver.def("getBufferSize", &H2Core::FakeDriver::getBufferSize);
	_FakeDriver.def("getSampleRate", &H2Core::FakeDriver::getSampleRate);
	_FakeDriver.def("getOut_L", &H2Core::FakeDriver::getOut_L);
	_FakeDriver.def("getOut_R", &H2Core::FakeDriver::getOut_R);
	_FakeDriver.def("play", &H2Core::FakeDriver::play);
	_FakeDriver.def("stop", &H2Core::FakeDriver::stop);
	_FakeDriver.def("locate", &H2Core::FakeDriver::locate,
		py::arg("nFrame"));
	_FakeDriver.def("updateTransportInfo", &H2Core::FakeDriver::updateTransportInfo);
	_FakeDriver.def("setBpm", &H2Core::FakeDriver::setBpm,
		py::arg("fBPM"));

	py::class_<H2Core::DiskWriterDriver, std::shared_ptr<H2Core::DiskWriterDriver>> _DiskWriterDriver(m, "DiskWriterDriver");
	_DiskWriterDriver.def(py::init<H2Core::audioProcessCallback, unsigned int, int>());
	_DiskWriterDriver.def_readwrite("m_nSampleRate", &H2Core::DiskWriterDriver::m_nSampleRate);
	_DiskWriterDriver.def_readwrite("m_sFilename", &H2Core::DiskWriterDriver::m_sFilename);
	_DiskWriterDriver.def_readwrite("m_nBufferSize", &H2Core::DiskWriterDriver::m_nBufferSize);
	_DiskWriterDriver.def_readwrite("m_nSampleDepth", &H2Core::DiskWriterDriver::m_nSampleDepth);
	_DiskWriterDriver.def_readwrite("m_pOut_L", &H2Core::DiskWriterDriver::m_pOut_L);
	_DiskWriterDriver.def_readwrite("m_pOut_R", &H2Core::DiskWriterDriver::m_pOut_R);
	_DiskWriterDriver.def_static("class_name", &H2Core::DiskWriterDriver::class_name);
	_DiskWriterDriver.def("init", &H2Core::DiskWriterDriver::init,
		py::arg("nBufferSize"));
	_DiskWriterDriver.def("connect", &H2Core::DiskWriterDriver::connect);
	_DiskWriterDriver.def("disconnect", &H2Core::DiskWriterDriver::disconnect);
	// [banned] _DiskWriterDriver.def("write", &H2Core::DiskWriterDriver::write,
	// [banned] 	py::arg("buffer_L"),
	// [banned] 	py::arg("buffer_R"),
	// [banned] 	py::arg("bufferSize"));
	_DiskWriterDriver.def("audioEngine_process_checkBPMChanged", &H2Core::DiskWriterDriver::audioEngine_process_checkBPMChanged);
	_DiskWriterDriver.def("getBufferSize", &H2Core::DiskWriterDriver::getBufferSize);
	_DiskWriterDriver.def("getSampleRate", &H2Core::DiskWriterDriver::getSampleRate);
	_DiskWriterDriver.def("getOut_L", &H2Core::DiskWriterDriver::getOut_L);
	_DiskWriterDriver.def("getOut_R", &H2Core::DiskWriterDriver::getOut_R);
	_DiskWriterDriver.def("setFileName", &H2Core::DiskWriterDriver::setFileName,
		py::arg("sFilename"));
	_DiskWriterDriver.def("play", &H2Core::DiskWriterDriver::play);
	_DiskWriterDriver.def("stop", &H2Core::DiskWriterDriver::stop);
	_DiskWriterDriver.def("locate", &H2Core::DiskWriterDriver::locate,
		py::arg("nFrame"));
	_DiskWriterDriver.def("updateTransportInfo", &H2Core::DiskWriterDriver::updateTransportInfo);
	_DiskWriterDriver.def("setBpm", &H2Core::DiskWriterDriver::setBpm,
		py::arg("fBPM"));

	py::class_<H2Core::AlsaAudioDriver, std::shared_ptr<H2Core::AlsaAudioDriver>> _AlsaAudioDriver(m, "AlsaAudioDriver");
	_AlsaAudioDriver.def(py::init<H2Core::audioProcessCallback>());
	_AlsaAudioDriver.def_readwrite("m_bIsRunning", &H2Core::AlsaAudioDriver::m_bIsRunning);
	_AlsaAudioDriver.def_readwrite("m_nBufferSize", &H2Core::AlsaAudioDriver::m_nBufferSize);
	_AlsaAudioDriver.def_readwrite("m_pOut_L", &H2Core::AlsaAudioDriver::m_pOut_L);
	_AlsaAudioDriver.def_readwrite("m_pOut_R", &H2Core::AlsaAudioDriver::m_pOut_R);
	_AlsaAudioDriver.def_readwrite("m_nXRuns", &H2Core::AlsaAudioDriver::m_nXRuns);
	_AlsaAudioDriver.def_readwrite("m_sAlsaAudioDevice", &H2Core::AlsaAudioDriver::m_sAlsaAudioDevice);
	_AlsaAudioDriver.def_static("class_name", &H2Core::AlsaAudioDriver::class_name);
	_AlsaAudioDriver.def("init", &H2Core::AlsaAudioDriver::init,
		py::arg("nBufferSize"));
	_AlsaAudioDriver.def("connect", &H2Core::AlsaAudioDriver::connect);
	_AlsaAudioDriver.def("disconnect", &H2Core::AlsaAudioDriver::disconnect);
	_AlsaAudioDriver.def("getBufferSize", &H2Core::AlsaAudioDriver::getBufferSize);
	_AlsaAudioDriver.def("getSampleRate", &H2Core::AlsaAudioDriver::getSampleRate);
	_AlsaAudioDriver.def("getOut_L", &H2Core::AlsaAudioDriver::getOut_L);
	_AlsaAudioDriver.def("getOut_R", &H2Core::AlsaAudioDriver::getOut_R);
	_AlsaAudioDriver.def("updateTransportInfo", &H2Core::AlsaAudioDriver::updateTransportInfo);
	_AlsaAudioDriver.def("play", &H2Core::AlsaAudioDriver::play);
	_AlsaAudioDriver.def("stop", &H2Core::AlsaAudioDriver::stop);
	_AlsaAudioDriver.def("locate", &H2Core::AlsaAudioDriver::locate,
		py::arg("nFrame"));
	_AlsaAudioDriver.def("setBpm", &H2Core::AlsaAudioDriver::setBpm,
		py::arg("fBPM"));

	py::class_<H2Core::Timeline::Tag, std::shared_ptr<H2Core::Timeline::Tag>> _Tag(m, "Tag");
	_Tag.def_readwrite("nBar", &H2Core::Timeline::Tag::nBar);
	_Tag.def_readwrite("sTag", &H2Core::Timeline::Tag::sTag);

	py::class_<H2Core::Timeline::TempoMarker, std::shared_ptr<H2Core::Timeline::TempoMarker>> _TempoMarker(m, "TempoMarker");
	_TempoMarker.def_readwrite("nBar", &H2Core::Timeline::TempoMarker::nBar);
	_TempoMarker.def_readwrite("fBpm", &H2Core::Timeline::TempoMarker::fBpm);

	py::class_<H2Core::EventQueue::AddMidiNoteVector, std::shared_ptr<H2Core::EventQueue::AddMidiNoteVector>> _AddMidiNoteVector(m, "AddMidiNoteVector");
	_AddMidiNoteVector.def_readwrite("m_column", &H2Core::EventQueue::AddMidiNoteVector::m_column);
	_AddMidiNoteVector.def_readwrite("m_row", &H2Core::EventQueue::AddMidiNoteVector::m_row);
	_AddMidiNoteVector.def_readwrite("m_pattern", &H2Core::EventQueue::AddMidiNoteVector::m_pattern);
	_AddMidiNoteVector.def_readwrite("m_length", &H2Core::EventQueue::AddMidiNoteVector::m_length);
	_AddMidiNoteVector.def_readwrite("f_velocity", &H2Core::EventQueue::AddMidiNoteVector::f_velocity);
	_AddMidiNoteVector.def_readwrite("f_pan", &H2Core::EventQueue::AddMidiNoteVector::f_pan);
	_AddMidiNoteVector.def_readwrite("nk_noteKeyVal", &H2Core::EventQueue::AddMidiNoteVector::nk_noteKeyVal);
	_AddMidiNoteVector.def_readwrite("no_octaveKeyVal", &H2Core::EventQueue::AddMidiNoteVector::no_octaveKeyVal);
	_AddMidiNoteVector.def_readwrite("b_isMidi", &H2Core::EventQueue::AddMidiNoteVector::b_isMidi);
	_AddMidiNoteVector.def_readwrite("b_isInstrumentMode", &H2Core::EventQueue::AddMidiNoteVector::b_isInstrumentMode);
	_AddMidiNoteVector.def_readwrite("b_noteExist", &H2Core::EventQueue::AddMidiNoteVector::b_noteExist);

	// enum ErrorMessages
	py::enum_<H2Core::Hydrogen::ErrorMessages>(_Hydrogen, "ErrorMessages")
		.value("UNKNOWN_DRIVER", H2Core::Hydrogen::ErrorMessages::UNKNOWN_DRIVER)
		.value("ERROR_STARTING_DRIVER", H2Core::Hydrogen::ErrorMessages::ERROR_STARTING_DRIVER)
		.value("JACK_SERVER_SHUTDOWN", H2Core::Hydrogen::ErrorMessages::JACK_SERVER_SHUTDOWN)
		.value("JACK_CANNOT_ACTIVATE_CLIENT", H2Core::Hydrogen::ErrorMessages::JACK_CANNOT_ACTIVATE_CLIENT)
		.value("JACK_CANNOT_CONNECT_OUTPUT_PORT", H2Core::Hydrogen::ErrorMessages::JACK_CANNOT_CONNECT_OUTPUT_PORT)
		.value("JACK_CANNOT_CLOSE_CLIENT", H2Core::Hydrogen::ErrorMessages::JACK_CANNOT_CLOSE_CLIENT)
		.value("JACK_ERROR_IN_PORT_REGISTER", H2Core::Hydrogen::ErrorMessages::JACK_ERROR_IN_PORT_REGISTER)
		.value("OSC_CANNOT_CONNECT_TO_PORT", H2Core::Hydrogen::ErrorMessages::OSC_CANNOT_CONNECT_TO_PORT);

	// enum GUIState
	py::enum_<H2Core::Hydrogen::GUIState>(_Hydrogen, "GUIState")
		.value("notReady", H2Core::Hydrogen::GUIState::notReady)
		.value("unavailable", H2Core::Hydrogen::GUIState::unavailable)
		.value("ready", H2Core::Hydrogen::GUIState::ready);

	// enum Lookup
	py::enum_<H2Core::Filesystem::Lookup>(_Filesystem, "Lookup")
		.value("stacked", H2Core::Filesystem::Lookup::stacked)
		.value("user", H2Core::Filesystem::Lookup::user)
		.value("system", H2Core::Filesystem::Lookup::system);

	// enum file_perms
	py::enum_<H2Core::Filesystem::file_perms>(_Filesystem, "file_perms")
		.value("is_dir", H2Core::Filesystem::file_perms::is_dir)
		.value("is_file", H2Core::Filesystem::file_perms::is_file)
		.value("is_readable", H2Core::Filesystem::file_perms::is_readable)
		.value("is_writable", H2Core::Filesystem::file_perms::is_writable)
		.value("is_executable", H2Core::Filesystem::file_perms::is_executable);

	// enum ActionMode
	py::enum_<H2Core::Song::ActionMode>(_Song, "ActionMode")
		.value("selectMode", H2Core::Song::ActionMode::selectMode)
		.value("drawMode", H2Core::Song::ActionMode::drawMode);

	// enum SongMode
	py::enum_<H2Core::Song::SongMode>(_Song, "SongMode")
		.value("PATTERN_MODE", H2Core::Song::SongMode::PATTERN_MODE)
		.value("SONG_MODE", H2Core::Song::SongMode::SONG_MODE);

	py::class_<H2Core::Playlist::Entry, std::shared_ptr<H2Core::Playlist::Entry>> _Entry(m, "Entry");
	_Entry.def_readwrite("filePath", &H2Core::Playlist::Entry::filePath);
	_Entry.def_readwrite("fileExists", &H2Core::Playlist::Entry::fileExists);
	_Entry.def_readwrite("scriptPath", &H2Core::Playlist::Entry::scriptPath);
	_Entry.def_readwrite("scriptEnabled", &H2Core::Playlist::Entry::scriptEnabled);

	// enum UI_SCALING_POLICY
	py::enum_<H2Core::Preferences::UI_SCALING_POLICY>(_Preferences, "UI_SCALING_POLICY")
		.value("UI_SCALING_SMALLER", H2Core::Preferences::UI_SCALING_POLICY::UI_SCALING_SMALLER)
		.value("UI_SCALING_SYSTEM", H2Core::Preferences::UI_SCALING_POLICY::UI_SCALING_SYSTEM)
		.value("UI_SCALING_LARGER", H2Core::Preferences::UI_SCALING_POLICY::UI_SCALING_LARGER);

	// enum UI_LAYOUT_TYPES
	py::enum_<H2Core::Preferences::UI_LAYOUT_TYPES>(_Preferences, "UI_LAYOUT_TYPES")
		.value("UI_LAYOUT_SINGLE_PANE", H2Core::Preferences::UI_LAYOUT_TYPES::UI_LAYOUT_SINGLE_PANE)
		.value("UI_LAYOUT_TABBED", H2Core::Preferences::UI_LAYOUT_TYPES::UI_LAYOUT_TABBED);

	// enum FontSize
	py::enum_<H2Core::Preferences::FontSize>(_Preferences, "FontSize")
		.value("Normal", H2Core::Preferences::FontSize::Normal)
		.value("Small", H2Core::Preferences::FontSize::Small)
		.value("Large", H2Core::Preferences::FontSize::Large);

	// enum JackBBTSyncMethod
	py::enum_<H2Core::Preferences::JackBBTSyncMethod>(_Preferences, "JackBBTSyncMethod")
		.value("constMeasure", H2Core::Preferences::JackBBTSyncMethod::constMeasure)
		.value("identicalBars", H2Core::Preferences::JackBBTSyncMethod::identicalBars);

	// enum JackTrackOutputMode
	py::enum_<H2Core::Preferences::JackTrackOutputMode>(_Preferences, "JackTrackOutputMode")
		.value("postFader", H2Core::Preferences::JackTrackOutputMode::postFader)
		.value("preFader", H2Core::Preferences::JackTrackOutputMode::preFader);

	py::class_<H2Core::Sample::Rubberband, std::shared_ptr<H2Core::Sample::Rubberband>> _Rubberband(m, "Rubberband");
	_Rubberband.def(py::init<>());
	_Rubberband.def(py::init<const H2Core::Sample::Rubberband *>());
	_Rubberband.def_readwrite("use", &H2Core::Sample::Rubberband::use);
	_Rubberband.def_readwrite("divider", &H2Core::Sample::Rubberband::divider);
	_Rubberband.def_readwrite("pitch", &H2Core::Sample::Rubberband::pitch);
	_Rubberband.def_readwrite("c_settings", &H2Core::Sample::Rubberband::c_settings);
	_Rubberband.def("operator==", &H2Core::Sample::Rubberband::operator==,
		"equal to operator",
		py::arg("b"));
	_Rubberband.def("toQString", &H2Core::Sample::Rubberband::toQString,
		py::arg("sPrefix"),
		py::arg("bShort"));

	py::class_<H2Core::Sample::Loops, std::shared_ptr<H2Core::Sample::Loops>> _Loops(m, "Loops");
	_Loops.def(py::init<>());
	_Loops.def(py::init<const H2Core::Sample::Loops *>());
	_Loops.def_readwrite("start_frame", &H2Core::Sample::Loops::start_frame);
	_Loops.def_readwrite("loop_frame", &H2Core::Sample::Loops::loop_frame);
	_Loops.def_readwrite("end_frame", &H2Core::Sample::Loops::end_frame);
	_Loops.def_readwrite("count", &H2Core::Sample::Loops::count);
	_Loops.def_readwrite("mode", &H2Core::Sample::Loops::mode);
	_Loops.def("operator==", &H2Core::Sample::Loops::operator==,
		"equal to operator",
		py::arg("b"));
	_Loops.def("toQString", &H2Core::Sample::Loops::toQString,
		py::arg("sPrefix"),
		py::arg("bShort"));

	// enum SampleSelectionAlgo
	py::enum_<H2Core::Instrument::SampleSelectionAlgo>(_Instrument, "SampleSelectionAlgo")
		.value("VELOCITY", H2Core::Instrument::SampleSelectionAlgo::VELOCITY)
		.value("ROUND_ROBIN", H2Core::Instrument::SampleSelectionAlgo::ROUND_ROBIN)
		.value("RANDOM", H2Core::Instrument::SampleSelectionAlgo::RANDOM);

	// enum Timebase
	py::enum_<H2Core::JackAudioDriver::Timebase>(_JackAudioDriver, "Timebase")
		.value("Master", H2Core::JackAudioDriver::Timebase::Master)
		.value("Slave", H2Core::JackAudioDriver::Timebase::Slave)
		.value("None", H2Core::JackAudioDriver::Timebase::None);

	// enum LoopMode
	py::enum_<H2Core::Sample::Loops::LoopMode>(_Loops, "LoopMode")
		.value("FORWARD", H2Core::Sample::Loops::LoopMode::FORWARD)
		.value("REVERSE", H2Core::Sample::Loops::LoopMode::REVERSE)
		.value("PINGPONG", H2Core::Sample::Loops::LoopMode::PINGPONG);

}