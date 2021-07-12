#include <Object.h>
#include <IO/NullDriver.h>
#include <IO/JackAudioDriver.h>
#include <IO/AlsaAudioDriver.h>
#include <IO/MidiOutput.h>
#include <IO/DiskWriterDriver.h>
#include <IO/OssDriver.h>
#include <IO/TransportInfo.h>
#include <IO/JackMidiDriver.h>
#include <IO/CoreMidiDriver.h>
#include <IO/AlsaMidiDriver.h>
#include <IO/PortAudioDriver.h>
#include <IO/FakeDriver.h>
#include <IO/CoreAudioDriver.h>
#include <IO/PortMidiDriver.h>
#include <IO/PulseAudioDriver.h>
#include <IO/AudioOutput.h>
#include <MidiMap.h>
#include <NsmClient.h>
#include <MidiAction.h>
#include <Timeline.h>
#include <Helpers/Legacy.h>
#include <Basics/InstrumentList.h>
#include <Basics/Pattern.h>
#include <Basics/Adsr.h>
#include <Basics/InstrumentLayer.h>
#include <Basics/Song.h>
#include <Basics/Note.h>
#include <Basics/AutomationPath.h>
#include <Basics/Playlist.h>
#include <FX/Effects.h>
#include <FX/LadspaFX.h>
#include <Basics/Drumkit.h>
#include <Basics/InstrumentComponent.h>
#include <Basics/PatternList.h>
#include <Basics/Instrument.h>
#include <Basics/DrumkitComponent.h>
#include <Basics/Sample.h>
#include <Helpers/Files.h>
#include <Helpers/Filesystem.h>
#include <Helpers/Xml.h>
#include <Smf/SMF.h>
#include <Smf/SMFEvent.h>
#include <LocalFileMng.h>
#include <OscServer.h>
#include <Hydrogen.h>
#include <AutomationPathSerializer.h>
#include <Sampler/Sampler.h>
#include <EventQueue.h>
#include <Preferences.h>
#include <AudioEngine.h>
#include <CoreActionController.h>
#include <Synth/Synth.h>
namespace H2Core {
void init_core_object_map()
{
#ifdef H2CORE_HAVE_DEBUG
    H2_REGISTER(Object);
	H2_REGISTER(MidiMap);
	H2_REGISTER(Action);
	H2_REGISTER(MidiActionManager);
	H2_REGISTER(InstrumentList);
	H2_REGISTER(InstrumentComponent);
	H2_REGISTER(Note);
	H2_REGISTER(Song);
	H2_REGISTER(SongReader);
	H2_REGISTER(AutomationPath);
#ifdef	H2CORE_HAVE_LADSPA
	H2_REGISTER(Effects);
	H2_REGISTER(LadspaFXInfo);
	H2_REGISTER(LadspaFXGroup);
	H2_REGISTER(LadspaControlPort);
	H2_REGISTER(LadspaFX);
#endif
	H2_REGISTER(InstrumentLayer);
	H2_REGISTER(Playlist);
	H2_REGISTER(ADSR);
	H2_REGISTER(Drumkit);
	H2_REGISTER(Pattern);
	H2_REGISTER(PatternList);
	H2_REGISTER(Legacy);
	H2_REGISTER(DrumkitComponent);
	H2_REGISTER(Files);
	H2_REGISTER(Instrument);
	H2_REGISTER(Filesystem);
	H2_REGISTER(XMLNode);
	H2_REGISTER(XMLDoc);
	H2_REGISTER(EnvelopePoint);
	H2_REGISTER(Sample);
	H2_REGISTER(Timeline);
	H2_REGISTER(SMFHeader);
	H2_REGISTER(SMFTrack);
	H2_REGISTER(SMF);
	H2_REGISTER(SMFWriter);
    H2_REGISTER(SMF1Writer);
    H2_REGISTER(SMF1WriterSingle);
    H2_REGISTER(SMF1WriterMulti);
    H2_REGISTER(SMF0Writer);
	H2_REGISTER(AudioOutput);
	H2_REGISTER(SMFBuffer);
	H2_REGISTER(SMFEvent);
	H2_REGISTER(SMFTrackNameMetaEvent);
	H2_REGISTER(SMFSetTempoMetaEvent);
	H2_REGISTER(SMFCopyRightNoticeMetaEvent);
	H2_REGISTER(SMFTimeSignatureMetaEvent);
	H2_REGISTER(SMFNoteOnEvent);
	H2_REGISTER(SMFNoteOffEvent);
#ifdef H2CORE_HAVE_OSS
	H2_REGISTER(OssDriver);
	H2_REGISTER(OssDriver);
#endif
#ifdef H2CORE_HAVE_ALSA
	H2_REGISTER(AlsaAudioDriver);
	H2_REGISTER(AlsaMidiDriver);
#endif
	H2_REGISTER(DiskWriterDriver);
	H2_REGISTER(MidiOutput);
#ifdef H2CORE_HAVE_JACK
	H2_REGISTER(JackAudioDriver);
	H2_REGISTER(JackMidiDriver);
#endif
	H2_REGISTER(TransportInfo);
#ifdef H2CORE_HAVE_COREAUDIO
	H2_REGISTER(CoreAudioDriver);
	H2_REGISTER(CoreMidiDriver);
#endif
	H2_REGISTER(NullDriver);
	H2_REGISTER(FakeDriver);
#ifdef H2CORE_HAVE_PORTAUDIO
	H2_REGISTER(PortAudioDriver);
	H2_REGISTER(PortMidiDriver);
#endif
#ifdef H2CORE_HAVE_PULSEAUDIO
	H2_REGISTER(PulseAudioDriver);
#endif
	H2_REGISTER(LocalFileMng);
	H2_REGISTER(SongWriter);
#ifdef H2CORE_HAVE_OSC
	H2_REGISTER(NsmClient);
	H2_REGISTER(OscServer);
#endif
	H2_REGISTER(AutomationPathSerializer);
	H2_REGISTER(Hydrogen);
	H2_REGISTER(Sampler);
	H2_REGISTER(EventQueue);
	H2_REGISTER(CoreActionController);
	H2_REGISTER(AudioEngine);
	H2_REGISTER(Synth);
	H2_REGISTER(WindowProperties);
	H2_REGISTER(UIStyle);
	H2_REGISTER(Preferences);
#endif // H2CORE_HAVE_DEBUG
}
}; // namespace H2Core