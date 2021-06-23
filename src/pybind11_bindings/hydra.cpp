#include <map>
#include <memory>
#include <stdexcept>
#include <functional>
#include <string>

#include <pybind11/pybind11.h>

typedef std::function< pybind11::module & (std::string const &) > ModuleGetter;

void bind_bits_types_struct_timeval(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_unknown_unknown(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_QtCore_qvector(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_QtCore_qvector_1(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_QtCore_qnamespace(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_QtCore_qobjectdefs(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_QtCore_qarraydata(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_QtCore_qstringview(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_QtCore_qpair(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_QtCore_qregexp(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_QtCore_qmetatype(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_QtCore_qmetatype_1(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_QtCore_qobject(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_unknown_unknown_1(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_QtCore_qvariant(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_unknown_unknown_2(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_unknown_unknown_3(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_unknown_unknown_4(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_QtCore_qtextstream(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_QtCore_quuid(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_unknown_unknown_5(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_unknown_unknown_6(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_QtCore_qjsonvalue(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_unknown_unknown_7(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_QtCore_qpoint(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_unknown_unknown_8(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_unknown_unknown_9(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_QtCore_qsize(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_unknown_unknown_10(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_unknown_unknown_11(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_unknown_unknown_12(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_unknown_unknown_13(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_Object(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_Sampler_Interpolation(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_Sampler_Sampler(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_Helpers_Filesystem(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_Basics_Instrument(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_Basics_Note(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_AudioEngine(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_AudioEngine_1(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_unknown_unknown_14(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_unknown_unknown_15(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_unknown_unknown_16(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_unknown_unknown_17(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_AutomationPathSerializer(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_Basics_InstrumentComponent(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_Basics_Pattern(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_Helpers_Xml(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_Basics_Sample(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_Basics_Song(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_EventQueue(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_FX_LadspaFX(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_Helpers_Legacy(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_IO_NullDriver(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_IO_JackAudioDriver(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_Hydrogen(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_IO_AlsaAudioDriver(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_IO_JackMidiDriver(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_IO_PortAudioDriver(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_IO_PortMidiDriver(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_Lilipond_Lilypond(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_QtGui_qrgba64(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_LocalFileMng(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_Preferences(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_Smf_SMFEvent(std::function< pybind11::module &(std::string const &namespace_) > &M);
void bind_core_Smf_SMF(std::function< pybind11::module &(std::string const &namespace_) > &M);


PYBIND11_MODULE(hydra, root_module) {
	root_module.doc() = "hydra module";

	std::map <std::string, pybind11::module> modules;
	ModuleGetter M = [&](std::string const &namespace_) -> pybind11::module & {
		auto it = modules.find(namespace_);
		if( it == modules.end() ) throw std::runtime_error("Attempt to access pybind11::module for namespace " + namespace_ + " before it was created!!!");
		return it->second;
	};

	modules[""] = root_module;

	std::vector< std::pair<std::string, std::string> > sub_modules {
		{"", "H2Core"},
		{"H2Core", "Interpolation"},
		{"", "Qt"},
		{"", "QtMetaTypePrivate"},
	};
	for(auto &p : sub_modules ) modules[p.first.size() ? p.first+"::"+p.second : p.second] = modules[p.first].def_submodule(p.second.c_str(), ("Bindings for " + p.first + "::" + p.second + " namespace").c_str() );

	//pybind11::class_<std::shared_ptr<void>>(M(""), "_encapsulated_data_");

	bind_bits_types_struct_timeval(M);
	bind_unknown_unknown(M);
	bind_QtCore_qvector(M);
	bind_QtCore_qvector_1(M);
	bind_QtCore_qnamespace(M);
	bind_QtCore_qobjectdefs(M);
	bind_QtCore_qarraydata(M);
	bind_QtCore_qstringview(M);
	bind_QtCore_qpair(M);
	bind_QtCore_qregexp(M);
	bind_QtCore_qmetatype(M);
	bind_QtCore_qmetatype_1(M);
	bind_QtCore_qobject(M);
	bind_unknown_unknown_1(M);
	bind_QtCore_qvariant(M);
	bind_unknown_unknown_2(M);
	bind_unknown_unknown_3(M);
	bind_unknown_unknown_4(M);
	bind_QtCore_qtextstream(M);
	bind_QtCore_quuid(M);
	bind_unknown_unknown_5(M);
	bind_unknown_unknown_6(M);
	bind_QtCore_qjsonvalue(M);
	bind_unknown_unknown_7(M);
	bind_QtCore_qpoint(M);
	bind_unknown_unknown_8(M);
	bind_unknown_unknown_9(M);
	bind_QtCore_qsize(M);
	bind_unknown_unknown_10(M);
	bind_unknown_unknown_11(M);
	bind_unknown_unknown_12(M);
	bind_unknown_unknown_13(M);
	bind_core_Object(M);
	bind_core_Sampler_Interpolation(M);
	bind_core_Sampler_Sampler(M);
	bind_core_Helpers_Filesystem(M);
	bind_core_Basics_Instrument(M);
	bind_core_Basics_Note(M);
	bind_core_AudioEngine(M);
	bind_core_AudioEngine_1(M);
	bind_unknown_unknown_14(M);
	bind_unknown_unknown_15(M);
	bind_unknown_unknown_16(M);
	bind_unknown_unknown_17(M);
	bind_core_AutomationPathSerializer(M);
	bind_core_Basics_InstrumentComponent(M);
	bind_core_Basics_Pattern(M);
	bind_core_Helpers_Xml(M);
	bind_core_Basics_Sample(M);
	bind_core_Basics_Song(M);
	bind_core_EventQueue(M);
	bind_core_FX_LadspaFX(M);
	bind_core_Helpers_Legacy(M);
	bind_core_IO_NullDriver(M);
	bind_core_IO_JackAudioDriver(M);
	bind_core_Hydrogen(M);
	bind_core_IO_AlsaAudioDriver(M);
	bind_core_IO_JackMidiDriver(M);
	bind_core_IO_PortAudioDriver(M);
	bind_core_IO_PortMidiDriver(M);
	bind_core_Lilipond_Lilypond(M);
	bind_QtGui_qrgba64(M);
	bind_core_LocalFileMng(M);
	bind_core_Preferences(M);
	bind_core_Smf_SMFEvent(M);
	bind_core_Smf_SMF(M);

}
