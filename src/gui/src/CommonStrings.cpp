/*
 * Hydrogen
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "CommonStrings.h"

CommonStrings::CommonStrings(){
	/*: Text displayed on the button to show the Playback track. Its size
	  is designed to hold a single character.*/
	m_sPlaybackTrackButton = tr( "P" );

	/*: Text displayed on the button for soloing an instrument strip in
	  the mixer. Its size is designed for a single character.*/
	m_sSmallSoloButton = tr( "S" );
	/*: Text displayed on the button for muting an instrument strip in
	  the mixer. Its size is designed for a single character.*/
	m_sSmallMuteButton = tr( "M" );
	/*: Text displayed on the button for muting the master strip. Its
	  size is designed for a four characters.*/
	m_sBigMuteButton = tr( "Mute" );
	/*: Text displayed on the button for bypassing an element. Its
	  size is designed for a three characters.*/
	m_sBypassButton = tr( "BYP" );
	/*: Text displayed on the button for editing an element. Its
	  size is designed for a four characters.*/
	m_sEditButton = tr( "Edit" );
	/*: Text displayed on the button to clear all patterns in the
	  SongEditor. Its size is designed to hold five characters.*/
	m_sClearButton = tr( "Clear" );
	/*: Text displayed on the button to show the Timeline. Its size
	  is designed to hold a single character.*/
	m_sTimelineButton = tr( "T" );
	/*: Text displayed on the button to activate the Timeline. Its size
	  is designed to hold eight characters.*/
	m_sTimelineBigButton = tr( "Timeline" );
	/*: Text displayed on the button to enable the LADSPA effect strips. Its size
	  is designed to hold two characters.*/
	m_sFXButton = tr( "FX" );
	/*: Text displayed on the button to show the instrument peaks. Its size
	  is designed to hold four characters.*/
	m_sPeakButton = tr( "Peak" );
	/*: Text displayed on the button to show the Instrument Rack. Its size
	  is designed to hold seven characters but is quite flexible.*/
	m_sGeneralButton = tr( "General" );
	/*: Text displayed on the button to show the Instrument Editor in the
	  * Instrument Rack. Its size is designed to hold ten characters but is
	  * quite flexible.
	  *
	  * It is also used in table headers corresponding to the instrument's name
	  * or id. */
	m_sInstrumentButton = tr( "Instrument" );
	/*: Text displayed on the button to show the Sound Library in the Instrument Rack. Its size
	  is designed to hold ten characters but is quite flexible.*/
	m_sSoundLibraryButton = tr( "Sound Library" );
	/*: Text displayed on the button to show the Layer view of the Instrument Rack. Its size
	  is designed to hold six characters but is quite flexible.*/
	m_sLayersButton = tr( "Layers" );
	/*: Text displayed on the button to load a layer into an
	  instrument. Its size is designed to hold ten characters but is quite
	  flexible.*/
	m_sLoadLayerButton = tr( "Load Layer" );

	/*: Text displayed on the button to delete a layer into an
	  instrument. Its size is designed to hold twelve characters but is quite
	  flexible.*/
	m_sDeleteLayerButton = tr( "Delete Layer" );

	/*: Text displayed on the button to edit a layer into an
	  instrument. Its size is designed to hold ten characters but is quite
	  flexible.*/
	m_sEditLayerButton = tr( "Edit Layer" );

	/*: Text displayed on the button to activate the Beat Counter. Its
	  size is designed to hold two characters in two separate rows.*/
	m_sBeatCounterButton = tr( "B\nC" );

	/*: Text displayed on the button indicating that the Beat Counter will
	  only set tempo. Its size is designed to hold one character.*/
	m_sBeatCounterSetPlayButtonOff = tr( "S" );
	/*: Text displayed on the button indicating that the Beat Counter will
	  start playing after setting the tempo. Its size is designed to hold one character.*/
	m_sBeatCounterSetPlayButtonOn = tr( "P" );

	/*: Text displayed on the button to activate the resampling using 
	  Rubberband. Its
	  size is designed to hold three characters in two separate rows.*/
	m_sRubberbandButton = tr( "R\nU\nB" );

	/*: Text displayed on the button to activate the JACK transport control. Its
	  size is designed to hold seven characters and is moderately flexible.*/
	m_sJackTransportButton = tr( "J.Trans" );
	/*: Text displayed on the button to register Hydrogen to be in the JACK
	  Timebase control. Its size is designed to hold eight characters and is
	  moderately flexible.*/
	m_sJackTimebaseButton = tr( "Timebase" );
	/*: Text displayed on the button to show the Mixer window. Its
	  size is designed to hold five characters and is flexible.*/
	m_sMixerButton = tr( "Mixer" );
	/*: Text displayed on the button to show the Instrument Rack. Its
	  size is designed to hold 15 characters and is flexible.*/
	m_sInstrumentRackButton = tr( "Instrument Rack" );

	/*: Text displayed on the button activating Pattern Mode for playback. Its
	  size is designed to hold seven characters and is slightly flexible.*/
	m_sPatternModeButton = tr( "Pattern" );
	/*: Text displayed on the button activating Song Mode for playback. Its
	  size is designed to hold four characters and is slightly flexible.*/
	m_sSongModeButton = tr( "Song" );

	/*: Text displayed below the rotary to adjust the attack of the
	  ADSR in the Instrument Editor. Designed to hold six characters
	  but flexible.*/
	m_sAttackLabel = tr( "Attack" );
	/*: Text displayed below the rotary to adjust the decay of the
	  ADSR in the Instrument Editor. Designed to hold five characters
	  but flexible.*/
	m_sDecayLabel = tr( "Decay" );
	/*: Text displayed below the rotary to adjust the sustain of the
	  ADSR in the Instrument Editor. Designed to hold seven characters
	  but flexible.*/
	m_sSustainLabel = tr( "Sustain" );
	/*: Text displayed below the rotary to adjust the release of the
	  ADSR in the Instrument Editor. Designed to hold seven characters
	  but flexible.*/
	m_sReleaseLabel = tr( "Release" );
	/*: Text displayed below the LCD to set the output MIDI channel
	  in the Instrument Editor. Designed to hold seven characters but
	  flexible.*/
	m_sMidiOutChannelLabel = tr( "Channel" );
	/*: Text displayed below the LCD to set the output MIDI note
	  in the Instrument Editor. Designed to hold four characters but
	  flexible.*/
	m_sMidiOutNoteLabel = tr( "Note" );
	/*: Text displayed in the left part of the row of the Instrument
	  Editor concerned with MIDI output parameters. Designed to hold
	  eleven characters but flexible.*/
	m_sMidiOutLabel = tr( "MIDI Output" );
	/*: Text displayed in the Instrument Editor in the row of the
	  pitch widget. Designed to hold five characters but flexible.*/
	m_sPitchLabel = tr( "Pitch" );
	/*: Text displayed below the rotary to adjust the deterministic
	  part of the instrument pitch in front of decimal point in the
	  Instrument Editor. Designed to hold six characters but
	  flexible.*/
	m_sPitchCoarseLabel = tr( "Coarse" );
	/*: Text displayed below the rotary to adjust the deterministic
	  part of the instrument pitch after decimal point in the
	  Instrument Editor. Designed to hold four characters but
	  flexible.*/
	m_sPitchFineLabel = tr( "Fine" );
	/*: Text displayed below the rotary to adjust the random part of
	  the instrument pitch in the Instrument Editor. Designed to hold
	 six characters but flexible.*/
	m_sPitchRandomLabel = tr( "Random" );
	/*: Text displayed below the rotary to adjust the instrument gain
	 in the Instrument Editor. Designed to hold four characters but
	 flexible.*/
	m_sGainLabel = tr( "Gain" );
	/*: Text displayed below the LCD to set the mute group in the
	 Instrument Editor. Designed to hold ten characters but
	 flexible.*/
	m_sMuteGroupLabel = tr( "Mute Group" );
	/*: Text displayed next to the checkbox to activate the auto stop
	 note feature in the Instrument Editor. Designed to hold 14
	 characters but flexible.*/
	m_sIsStopNoteLabel = tr( "Auto-Stop Note" );
	/*: Text displayed next to the checkbox to activate the apply
	 velocity feature in the Instrument Editor. Designed to hold 14
	 characters but flexible.*/
	m_sApplyVelocityLabel = tr( "Apply Velocity" );
	/*: Text displayed below the LCD to set the hihat pressure group
	 in the Instrument Editor. Designed to hold 13 characters but
	 is only moderately flexible.*/
	m_sHihatGroupLabel = tr( "HH Press. Grp" );
	/*: Text displayed below the LCD to set the maximum range of the
	 hihat pressure group in the Instrument Editor. Designed to hold
	 nine characters but flexible.*/
	m_sHihatMaxRangeLabel = tr( "Max Range" );
	/*: Text displayed below the LCD to set the minimum range of the
	 hihat pressure group in the Instrument Editor. Designed to hold
	 nine characters but flexible.*/
	m_sHihatMinRangeLabel = tr( "Min Range" );
	/*: Text displayed below the rotary to adjust the cutoff frequency
	 of the lowpass filter applied to the instrument in the Instrument
	 Editor. Designed to hold six characters but flexible.*/
	m_sCutoffLabel = tr( "Cutoff" );
	/*: Text displayed below the rotary to adjust the resonance frequency
	 of the lowpass filter applied to the instrument in the Instrument
	 Editor. Designed to hold ten characters but flexible.*/
	m_sResonanceLabel = tr( "Resonance" );
	/*: Text displayed below the rotary to adjust the layer gain
	 in the Instrument Editor. Designed to hold six characters but
	 flexible.*/
	m_sLayerGainLabel = tr( "L. Gain" );
	/*: Text displayed below the rotary to adjust the component gain
	 in the Instrument Editor. Designed to hold six characters but
	 flexible.*/
	m_sComponentGainLabel = tr( "C. Gain" );
	/*: Text displayed left of the sample selection LCD combo in the
	 Instrument Editor. Designed to hold eleven characters but not
	 that flexible.*/
	m_sSampleSelectionLabel = tr( "Sample Sel." );
	/*: Text displayed left of the pattern size LCD combo in the panel
	 of the Pattern Editor.*/
	m_sPatternSizeLabel = tr( "Size" );
	/*: Text displayed left of the resolution LCD combo in the panel
	 of the Pattern Editor.*/
	m_sResolutionLabel = tr( "Res" );
	/*: Text displayed left of the button to activate the playback of
	 inserted notes in the panel of the Pattern Editor.*/
	m_sHearNotesLabel = tr( "Hear" );
	/*: Text displayed left of the button to toggle the quantization
	 in the panel of the Pattern Editor.*/
	m_sQuantizeEventsLabel = tr( "Quant" );
	/*: Text displayed left of the button to switch between the
	 Drum Pattern Editor and the Piano Roll Editor in the panel
	 of the Pattern Editor.*/
	m_sShowPianoLabel = tr( "Input" );
	/*: Text displayed in the Player Control to indicate incoming MIDI
	  events. Designed to hold seven characters but not that
	  flexible.*/
	m_sMidiInLabel = tr( "MIDI-In" );
	/*: Text displayed in the Player Control to indicate the CPU
	 load. Designed to hold three characters but not that flexible.*/
	m_sCpuLabel = tr( "CPU" );
	/*: Text displayed in the Player Control to indicate where the set
	 the tempo of the song. Designed to hold three characters but not
	 that flexible.*/
	m_sBPMLabel = tr( "BPM" );
	/*: Text displayed in the Player Control to indicate the number of
	 hours passed since playback started. Designed to hold three
	 characters but not that flexible.*/
	m_sTimeHoursLabel = tr( "Hrs" );
	/*: Text displayed in the Player Control to indicate the number of
	 minutes passed since playback started. Designed to hold three
	 characters but not that flexible.*/
	m_sTimeMinutesLabel = tr( "Min" );
	/*: Text displayed in the Player Control to indicate the number of
	 seconds passed since playback started. Designed to hold three
	 characters but not that flexible.*/
	m_sTimeSecondsLabel = tr( "Sec" );
	/*: Text displayed in the Player Control to indicate the number of
	 milliseconds passed since playback started. Designed to hold three
	 characters but not that flexible.*/
	m_sTimeMilliSecondsLabel = tr( "1/1000" );
	/*: Text displayed in the Master Mixer Strip as a heading for the
	  humanization rotaries. Designed to hold eight characters but not
	  that flexible.*/
	m_sHumanizeLabel = tr( "Humanize" );
	/*: Text displayed in the Master Mixer Strip as a heading for the
	 swing humanization rotary. Designed to hold five characters but
	  flexible.*/
	m_sSwingLabel = tr( "Swing" );
	/*: Text displayed in the Master Mixer Strip as a heading for the
	 timing humanization rotary. Designed to hold six characters but
	  flexible.*/
	m_sTimingLabel = tr( "Timing" );
	/*: Text displayed in the Master Mixer Strip as a heading for the
	 velocity humanization rotary. Designed to hold eight characters
	 flexible.*/
	m_sVelocityLabel = tr( "Velocity" );
	/*: Text displayed as the title of the Master Mixer
	 Strip. Designed to hold six characters but flexible.*/
	m_sMasterLabel = tr( "Master" );
	/*: Text displayed below the rotary in the FX Mixerline. Designed
	  to hold six characters but flexible.*/
	m_sReturnLabel = tr( "Return" );

	/*: Displayed in the tooltip of input widgets. Indicates the
	  allowed values from minimum to maximum.*/
	m_sRangeTooltip = tr( "Range" );
	/*: Displayed in the tooltip of input widgets. General heading of
	  the part associating the Action of the widget with the MIDI
	  event and parameter it is bound to.*/
	m_sMidiTooltipHeading = tr( "MIDI" );
	/*: Displayed in the tooltip of input widgets. Body of the part
	  associating the Action of the widget with the MIDI event and
	  parameter it is bound to. It's full context is "ACTION bound to
	  [EVENT : PARAMETER]".*/
	m_sMidiTooltipBound = tr( "bound to" );
	/*: Displayed in the tooltip of input widgets. Body of the part
	  displaying the Action that is not associate to a MIDI event
	  yet. It's full context is "ACTION not bound".*/
	m_sMidiTooltipUnbound = tr( "not bound" );
	/*: Displayed on both LCDSpinBoxes used for the pattern size while
	  playback is rolling.*/
	m_sPatternSizeDisabledTooltip = tr( "It's not possible to change the pattern size when playing." );

	/*: Displayed when hovering over the button in the
	PatternEditorPanel to activate the DrumkitEditor.*/
	m_sShowDrumkitEditorTooltip = tr( "Show drumkit editor" );
	/*: Displayed when hovering over the button in the
	PatternEditorPanel to activate the PianoRollEditor.*/
	m_sShowPianoRollEditorTooltip = tr( "Show piano roll editor" );
	
	m_sAudioDriverStartError = tr( "Unable to start audio driver!" );
	m_sAudioDriverErrorHint = tr( "Please use the Preferences to select a different one." );
	m_sAudioDriverNotPresent = tr( "No audio driver set!" );

	m_sJackTimebaseTooltip = tr("No external JACK Timebase controller. Press to make Hydrogen in control.");
	m_sJackTimebaseListenerTooltip = tr("Hydrogen is listening to tempo and position info. Press to make Hydrogen in control instead.");
	m_sJackTimebaseDisabledTooltip = tr( "JACK timebase support is disabled in the Preferences" );
	
	/*: Title of the window displayed when using the MIDI learning
	  capabilities of Hydrogen.*/
	m_sMidiSenseWindowTitle = tr( "Waiting..." );
	/*: Text displayed when using the MIDI learning capabilities of
	  Hydrogen. Only displayed if the widget has an associated
	  action.*/
	m_sMidiSenseInput = tr( "Waiting for MIDI input..." );
	/*: Displayed in the  popup window when  using the  MIDI learning
	  capabilities of  Hydrogen. Indicating  that there is  not Action
	  which could be associated to a MIDI event.*/
	m_sMidiSenseUnavailable = tr( "This element is not MIDI operable." );

	m_sPatternLoadError = tr( "Unable to load pattern" );
	m_sInstrumentLoadError = tr( "Unable to load instrument" );

	/*: Error message shown when attempt to export a song, pattern,
	  drumkit, MIDI etc. into a read-only folder.*/
	m_sFileDialogMissingWritePermissions = tr( "You do not have permissions to write to the selected folder. Please select another one." );

	/*: Displayed within a status message when activating a widget.*/
	m_sStatusOn = tr( "on" );
	/*: Displayed within a status message when deactivating a widget.*/
	m_sStatusOff = tr( "off" );
	/*: Displayed within a status message when enabling a widget.*/
	m_sStatusEnabled = tr( "enabled" );
	/*: Displayed within a status message when disabling a widget.*/
	m_sStatusDisabled = tr( "disabled" );
		
	m_sTimelineEnabled = tr( "Enable the Timeline for custom tempo changes" );
	m_sTimelineDisabledPatternMode = tr( "The Timeline is only available in Song Mode" );
	m_sTimelineDisabledTimebaseListener = tr( "In the presence of an external JACK Timebase controller the tempo can not be altered from within Hydrogen" );
	m_sPatternEditorLocked = tr( "Lock the Pattern Editor to only show and follow the pattern recorded notes will be inserted into while in Song Mode." );
	
	/*: Displayed in the Preferences dialog in the info section for a
	  particular driver in case it is not properly supported on the
	  system.*/
	m_sPreferencesNotCompiled = tr( "Not compiled" );
	/*: Displayed in the Preferences dialog within a driver combobox
	  in case no driver was selected.*/
	m_sPreferencesNone = tr( "None" );
	/*: Displayed in the Preferences dialog as a tooltip for both the
	  sample rate combobox and buffer size spinbox.*/
	m_sPreferencesJackTooltip = tr( "Both buffer size and sample rate can only be altered in the configuration of the JACK server itself." );
	/*: Displayed both as tooltip in the Preferences dialog >
	  Shortcuts tab as well as window title.*/
	m_sPreferencesShortcutCapture = tr( "Define a keybinding for the selected shortcut" );

	/*: Text displayed on a Ok button of a dialog. The character after
	  the '&' symbol can be used as a hotkey and the '&' symbol itself
	  will not be displayed.*/
	m_sButtonOk = tr( "&Ok" );
	/*: Text displayed on an Apply button of a dialog. The character after the
	  '&' symbol can be used as a hotkey and the '&' symbol itself will not be
	  displayed.*/
	m_sButtonApply = tr( "&Apply" );
	/*: Text displayed on a Save button of a dialog. The character after
	  the '&' symbol can be used as a hotkey and the '&' symbol itself
	  will not be displayed.*/
	m_sButtonSave = tr( "&Save" );
	/*: Text displayed on a Cancel button of a dialog. The character
	  after the '&' symbol can be used as a hotkey and the '&' symbol
	  itself will not be displayed.*/
	m_sButtonCancel = tr( "&Cancel" );
	/*: Text displayed on a Discard button of a dialog. The character
	  after the '&' symbol can be used as a hotkey and the '&' symbol
	  itself will not be displayed.*/
	m_sButtonDiscard = tr( "&Discard" );
	/*: Text displayed on a Play button which will start playback. The
	  character after the '&' symbol can be used as a hotkey and the
	  '&' symbol itself will not be displayed.*/
	m_sButtonPlay = tr( "&Play" );
	/*: Text displayed on a Play button in the SampleEditor which will
	  start playback of the original file. The character after the '&'
	  symbol can be used as a hotkey and the '&' symbol itself will
	  not be displayed.*/
	m_sButtonPlayOriginalSample = tr( "Play &original sample" );
	/*: Displayed in popup dialogs in case the user attempts to close
	  a window which still contains unsaved changes. The '\n'
	  character introduces a linebreak and must not be translated*/
	m_sUnsavedChanges = tr( "Unsaved changes left. These changes will be lost. \nAre you sure?" );
	m_sSavingChanges = tr( "Do you want to save the changes?" );

	m_sMutableDialog = tr( "Don't show this message again" );
	/*: Label of the tab in pattern/song/drumkit properties dialog containing
	 *  artifact parameters, like name or author. */
	m_sTabGeneralDialog = tr( "General" );
	/*: Label of the tab in pattern/song/drumkit properties dialog holding a
	 *  table of all contained licenses. */
	m_sTabLicensesDialog = tr( "Licenses" );
	/*: Label of the text input in pattern/song/drumkit properties dialog to set
	 *  the name of the particular artifact. */
	m_sNameDialog = tr( "Name" );
	/*: Label of the spin box in pattern/song/drumkit properties dialog to set
	 *  the version of the particular artifact. */
	m_sVersionDialog = tr( "Version" );
	/*: Label of the text input in pattern/song/drumkit properties dialog to set
	 *  the license of the particular artifact. */
	m_sLicenseDialog = tr( "License" );
	/*: Label of the text input in pattern/song/drumkit properties dialog to set
	 *  the author of the particular artifact. */
	m_sAuthorDialog = tr( "Author" );
	/*: Label of the text input in pattern/song/drumkit properties dialog to
	 *  fill in notes about the particular artifact. */
	m_sNotesDialog = tr( "Notes" );
	
	// Not used yet.
	/*: Displayed in the Open dialog window if the selected song could
	  not be loaded.*/
	// m_sDialogSongLoadError = tr( "Error loading song." );
	/*: Heading displayed in the info box asking the user to recover
	  unsaved changes from an earlier session.*/
	// m_sDialogUnsavedChangedH1 = tr( "There are unsaved changes." );
	/*: Additional text displayed in the info box asking the user to
	  recover unsaved changes from an earlier session.*/
	// m_sDialogUnsavedChangedH2 = tr( "Do you want to recover them?" );

	/*: Label corresponding to the line edit in the drumkit and song
	  properties dialog used to enter the license*/
	m_sLicenseStringLbl = tr( "License String" );
	/*: Tool tip used for the combo boxes in both the drumkit and song
	  property dialog to set a predefined license type.*/
	m_sLicenseComboToolTip = tr( "License parsed from License String. You can use this combo box to overwrite the current license with a predefined one" );
	m_sLicenseStringToolTip = tr( "License string written to disk. You can customize it to e.g. include an attribution other then the author. But be aware that it will be overwritten once you select a different license" );
	
	m_sLicenseCopyleftWarning = tr( "You used drumkit samples holding a <b>copyleft license</b>. Be aware that <b>you are legally obliged to make a copy publicly available and can not prevent its redistribution by others.</b>" );
	m_sLicenseAttributionWarning = tr( "All license containing the letters 'CC BY' <b>require you to give an attribution</b> by naming drumkit, author, as well as the license itself." );
	/*: Shown as title in dialogs used to inform the user about
	  license issues and information.*/
	m_sLicenseWarningWindowTitle = tr( "License Warning" );
	/*: Shown in a warning dialog in case the user inserted a license string
	 *  which does not comply with her selected license (in the combo box). */
	m_sLicenseMismatchingUserInput = tr( "Specified drumkit License String does not comply with the license selected in the combo box." );
	
	/*: Label shown in the input capture dialog for querying a new
	  tempo value. */
	m_sInputCaptureBpm = tr( "BPM" );
	/*: Label shown in the input capture dialog for querying a new
	  volume value. */
	m_sInputCaptureVolume = tr( "Volume" );
	/*: Label shown in the input capture dialog for querying a
	  column number of the song editor grid value. */
	m_sInputCaptureColumn = tr( "Column Number" );
	/*: Label shown in the input capture dialog for querying a
	  pattern number. */
	m_sInputCapturePattern = tr( "Pattern Number" );
	/*: Label shown in the input capture dialog for querying a
	  song number of the current playlist. */
	m_sInputCaptureSong = tr( "Song Number" );
	/*: Label shown in the input capture dialog for querying an
	  instrument number of the current drumkit. */
	m_sInputCaptureInstrument = tr( "Instrument Number" );
	/*: Label shown in the input capture dialog for querying a
	  component number of the specified instrument. */
	m_sInputCaptureComponent = tr( "Component Number" );
	/*: Label shown in the input capture dialog for querying a
	  layer number of the specified instrument component. */
	m_sInputCaptureLayer = tr( "Layer Number" );
	/*: Label shown in the input capture dialog for querying a
	  FX level of the specified FX. */
	m_sInputCaptureFXLevel = tr( "FX Level" );
	/*: Label shown in the input capture dialog for querying a
	  FX number of the specified instrument. */
	m_sInputCaptureFXNumber = tr( "FX Number" );
	/*: Label shown in the input capture dialog for querying a
	  new pan value for a specified instrument. */
	m_sInputCapturePan = tr( "Pan" );
	/*: Label shown in the input capture dialog for querying a
	  new filter cutoff value for a specified instrument. */
	m_sInputCaptureFilterCutoff = tr( "Filter Cutoff" );
	/*: Label shown in the input capture dialog for querying
	  text content for a new tag. */
	m_sInputCaptureTag = tr( "Tag Text" );
	
	/*: Shown in a dialog on export failure. */
	m_sExportSongFailure = tr( "Unable to export song" );
	m_sExportDrumkitFailure = tr( "Unable to export drumkit" );
	/*: Shown in a dialog on successful drumkit import. The path imported kit
	 *  will be appended to the translated string.*/
	m_sImportDrumkitSuccess = tr( "Drumkit imported in" );
	m_sImportDrumkitFailure = tr( "Unable to import drumkit" );
	m_sImportDrumkitEncodingFailure =
		tr( "\nBut there were encoding issues.\n\nPlease set your system's locale to UTF-8!" );
	m_sPlaylistSaveFailure = tr( "Unable to save playlist" );
	/*: Shown e.g. as suffix in a window title in case an underlying file was
	 *  modified */
	m_sIsModified = tr( "modified" );
	m_sReadOnlyAdvice = tr( "Use 'Save as' to enable autosave." );

	/*: Shown in table headers when referring to an instrument's id.*/
	m_sInstrumentId = tr( "Id" );
	/*: Shown in table headers when referring to an instrument's type (as part
	 *  of a Drumkit Map .h2map).*/
	m_sInstrumentType = tr( "Type" );
		/*: Shown in table headers when referring to a component's name.*/
	m_sComponent = tr( "Component" );
		/*: Shown in table headers when referring to a sample's name.*/
	m_sSample = tr( "Sample" );
		/*: Shown in table headers when referring to a license of an object.*/
	m_sLicense = tr( "License" );

	/*: Names an action in a drop down or pop up menu. (with no further text)*/
	m_sMenuActionAdd = tr( "Add" );
	/*: Names an action in a drop down or pop up menu. (with no further text)*/
	m_sMenuActionDelete = tr( "Delete" );
	/*: Names an action in a drop down or pop up menu. (with no further text)*/
	m_sMenuActionRename = tr( "Rename" );
	/*: Names an action in a drop down or pop up menu. (with no further text)*/
	m_sMenuActionLoad = tr( "Load" );
	/*: Names an action in a drop down or pop up menu. (with no further text)*/
	m_sMenuActionExport = tr( "Export" );
	/*: Names an action in a drop down or pop up menu. (with no further text)*/
	m_sMenuActionProperties = tr( "Properties" );
	/*: Names an action in a drop down or pop up menu. (with no further text)*/
	m_sMenuActionDuplicate = tr( "Duplicate" );
	/*: Names an action in a drop down or pop up menu. (with no further text)*/
	m_sMenuActionImport = tr( "Import" );
	/*: Names an action in a drop down or pop up menu. (with no further text)*/
	m_sMenuActionOnlineImport = tr( "Online Import" );

	/*: Used both as name for the undo menu in the main and playlist menu bar as
	 *  well as for the undo action itself. Mind the & symbol. The character
	 *  right after it will be used as default shortcut (Alt + character) for
	 *  this action. You can place it somewhere else or even just drop it. But
	 *  please mind possible conflicts (double assignments) with other
	 *  shortcuts.*/
	m_sUndoMenuUndo = tr( "&Undo" );
	/*: Used name for the redo action in undo menus. Mind the & symbol. The
	 *  character right after it will be used as default shortcut (Alt +
	 *  character) for this action. You can place it somewhere else or even just
	 *  drop it. But please mind possible conflicts (double assignments) with
	 *  other shortcuts.*/
	m_sUndoMenuRedo = tr( "&Redo" );
	/*: Used name for the action in undo menus opening a context menu showing
	 *  the particular undo history. Mind the & symbol. The character right
	 *  after it will be used as default shortcut (Alt + character) for this
	 *  action. You can place it somewhere else or even just drop it. But please
	 *  mind possible conflicts (double assignments) with other shortcuts.*/
	m_sUndoMenuHistory = tr( "Undo &History" );
	/*: Window title of the dialog showing the undo history. */
	m_sUndoHistoryTitle = tr( "Undo history" );

	m_sActionEditDrumkitProperties = tr( "Edit Drumkit Properties of Current Song" );
	m_sActionIrreversible = tr( "This action can not be undone!" );

	/** Representing adding a new instrument in the undo history as well as
	 * names the action presenting when right-clicking the instrument list in
	 * the pattern editor. */
	m_sActionAddInstrument = tr( "Add instrument" );
	/** Representing an instrument deletion in the undo history */
	m_sActionDeleteInstrument = tr( "Delete instrument" );
	/** Representing a drag&drop event for an instrument in the undo history */
	m_sActionDropInstrument = tr( "Drop instrument" );
	/** Representing a drumkit loading in the undo history */
	m_sActionSwitchDrumkit = tr( "Switch drumkit" );
	/** Representing the creation of a new drumkit in the undo history */
	m_sActionNewDrumkit = tr( "Replace song drumkit with new and empty one" );
	/** Representing adding a new component to the currently selected instrument
	 * in the undo history */
	m_sActionAddComponent = tr( "Adding component" );
	/** Representing deletion of a component from the currently selected
	 * instrument in the undo history */
	m_sActionDeleteComponent = tr( "Delete component" );
	/** Representing renaming a component of the currently selected instrument
	 * in the undo history */
	m_sActionRenameComponent = tr( "Rename component" );

	m_sErrorNotFound = tr( "File could not be found!" );
	/*: Shorter version of missing file warning. E.g. used as a prefix for the
	 *  song path in the Playlist editor.*/
	m_sErrorNotFoundShort = tr( "File not found" );

	/*: Suffix appended to a drumkit, song, or pattern name in case it
	 * is found on system-level and is read-only. */
	m_sSoundLibrarySystemSuffix = tr( "system" );
	/*: Suffix appended to a drumkit that are loaded non-persistently
	 *  into the current Hydrogen session. */
	m_sSoundLibrarySessionSuffix = tr( "session" );

	/*: Displayed in a warning message in case the user tries to read
	 * or write data to a file/path Hydrogen can not handle in the
	 * current encoding.*/
	m_sEncodingError = tr( "The provided filename can not be handled by your current encoding" );
}

CommonStrings::~CommonStrings(){}
