#include <FilesystemInfoForm.h>
#include <AboutDialogContributorList.h>
#include <PreferencesDialog.h>
#include <SongEditor/VirtualPatternDialog.h>
#include <SongEditor/SongEditorPanel.h>
#include <SongEditor/SongEditorPanelTagWidget.h>
#include <SongEditor/SongEditorPanelBpmWidget.h>
#include <SampleEditor/MainSampleWaveDisplay.h>
#include <SampleEditor/SampleEditor.h>
#include <SampleEditor/DetailWaveDisplay.h>
#include <SampleEditor/TargetWaveDisplay.h>
#include <SongEditor/PlaybackTrackWaveDisplay.h>
#include <SongEditor/SongEditor.h>
#include <SongEditor/PatternFillDialog.h>
#include <ExportSongDialog.h>
#include <PlaylistEditor/PlaylistDialog.h>
#include <ExportMidiDialog.h>
#include <InstrumentEditor/LayerPreview.h>
#include <InstrumentEditor/InstrumentEditor.h>
#include <SoundLibrary/SoundLibraryDatastructures.h>
#include <SoundLibrary/SoundLibraryExportDialog.h>
#include <SoundLibrary/SoundLibraryPanel.h>
#include <InstrumentEditor/InstrumentEditorPanel.h>
#include <InstrumentEditor/WaveDisplay.h>
#include <SoundLibrary/SoundLibraryRepositoryDialog.h>
#include <SoundLibrary/SoundLibraryOpenDialog.h>
#include <SoundLibrary/SoundLibraryImportDialog.h>
#include <SoundLibrary/SoundLibraryPropertiesDialog.h>
#include <SoundLibrary/FileBrowser.h>
#include <SoundLibrary/SoundLibraryTree.h>
#include <SplashScreen.h>
#include <AudioEngineInfoForm.h>
#include <Director.h>
#include <SoundLibrary/SoundLibrarySaveDialog.h>
#include <Mixer/MixerLine.h>
#include <HydrogenApp.h>
#include <PatternEditor/PatternEditor.h>
#include <PatternEditor/PatternEditorInstrumentList.h>
#include <PatternEditor/PianoRollEditor.h>
#include <PatternEditor/PatternEditorRuler.h>
#include <PatternEditor/PatternEditorPanel.h>
#include <AudioFileBrowser/SampleWaveDisplay.h>
#include <PatternEditor/NotePropertiesRuler.h>
#include <AudioFileBrowser/AudioFileBrowser.h>
#include <Mixer/Mixer.h>
#include <PatternEditor/DrumPatternEditor.h>
#include <Mixer/MixerSettingsDialog.h>
#include <MainForm.h>
#include <LadspaFXSelector.h>
#include <LadspaFXProperties.h>
#include <PlayerControl.h>
#include <InstrumentRack.h>
#include <Widgets/LCDCombo.h>
#include <Widgets/ColorSelectionButton.h>
#include <Widgets/Fader.h>
#include <Widgets/CpuLoadWidget.h>
#include <Widgets/PixmapWidget.h>
#include <Widgets/MidiSenseWidget.h>
#include <Widgets/LCD.h>
#include <Widgets/DownloadWidget.h>
#include <Widgets/Button.h>
#include <Widgets/MidiActivityWidget.h>
#include <Widgets/Rotary.h>
#include <Widgets/MidiTable.h>
#include <Widgets/AutomationPathView.h>
namespace H2Core {
void init_gui_object_map()
{
	H2_REGISTER(AboutDialogContributorList);
	H2_REGISTER(PreferencesDialog);
	H2_REGISTER(SongEditorPanel);
    H2_REGISTER(SongEditorPanelTagWidget);
    H2_REGISTER(SongEditorPanelBpmWidget);
    H2_REGISTER(PlaybackTrackWaveDisplay);
    H2_REGISTER(VirtualPatternDialog);
	H2_REGISTER(SampleEditor);
    H2_REGISTER(SongEditor);
    H2_REGISTER(SongEditorPatternList);
    H2_REGISTER(SongEditorPositionRuler);
    H2_REGISTER(MainSampleWaveDisplay);
    H2_REGISTER(DetailWaveDisplay);
	H2_REGISTER(TargetWaveDisplay);
	H2_REGISTER(PatternFillDialog);
	H2_REGISTER(ExportSongDialog);
	H2_REGISTER(PlaylistDialog);
	H2_REGISTER(ExportMidiDialog);
    H2_REGISTER(WaveDisplay);
	H2_REGISTER(SoundLibraryPanel);
	H2_REGISTER(SoundLibraryDatabase);
	H2_REGISTER(SoundLibraryInfo);
    H2_REGISTER(LayerPreview);
    H2_REGISTER(FileBrowser);
	H2_REGISTER(SoundLibraryPropertiesDialog);
    H2_REGISTER(SoundLibraryImportDialog);
    H2_REGISTER(SoundLibraryTree);
	H2_REGISTER(SoundLibrarySaveDialog);
	H2_REGISTER(SoundLibraryOpenDialog);
	H2_REGISTER(SoundLibraryRepositoryDialog);
	H2_REGISTER(SoundLibraryExportDialog);
	H2_REGISTER(InstrumentEditor);
	H2_REGISTER(FilesystemInfoForm);
    H2_REGISTER(InstrumentEditorPanel);
    H2_REGISTER(SplashScreen);
    H2_REGISTER(AudioEngineInfoForm);
	H2_REGISTER(Director);
	H2_REGISTER(Mixer);
	H2_REGISTER(AudioFileBrowser);
	H2_REGISTER(MixerSettingsDialog);
	H2_REGISTER(HydrogenApp);
	H2_REGISTER(SampleWaveDisplay);
	H2_REGISTER(MainForm);
    H2_REGISTER(LadspaFXProperties);
    H2_REGISTER(MetronomeWidget);
    H2_REGISTER(PlayerControl);
	H2_REGISTER(PatternEditorPanel);
	H2_REGISTER(PatternEditor);
    H2_REGISTER(PianoRollEditor);
    H2_REGISTER(DrumPatternEditor);
    H2_REGISTER(InstrumentLine);
	H2_REGISTER(PatternEditorInstrumentList);
    H2_REGISTER(InstrumentRack);
    H2_REGISTER(NotePropertiesRuler);
    H2_REGISTER(LadspaFXSelector);
	H2_REGISTER(InstrumentNameWidget);
	H2_REGISTER(MixerLine);
	H2_REGISTER(ComponentMixerLine);
	H2_REGISTER(MasterMixerLine);
	H2_REGISTER(FxMixerLine);
	H2_REGISTER(LadspaFXMixerLine);
    H2_REGISTER(PatternEditorRuler);
    H2_REGISTER(LCDDigit);
    H2_REGISTER(LCDDisplay);
    H2_REGISTER(LCDSpinBox);
	H2_REGISTER(LCDCombo);
    H2_REGISTER(ColorSelectionButton);
    H2_REGISTER(CpuLoadWidget);
    H2_REGISTER(Fader);
    H2_REGISTER(MasterFader);
    H2_REGISTER(PixmapWidget);
	H2_REGISTER(MidiSenseWidget);
	H2_REGISTER(Download);
	H2_REGISTER(DownloadWidget);
    H2_REGISTER(Button);
    H2_REGISTER(MidiActivityWidget);
    H2_REGISTER(Rotary);
    H2_REGISTER(MidiTable);
	H2_REGISTER(AutomationPathView);
}

}; // namespace H2Core