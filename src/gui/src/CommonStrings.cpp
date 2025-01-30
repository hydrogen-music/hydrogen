/*
 * Hydrogen
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
	/*: Text displayed on the button to show the Instrument Editor in the Instrument Rack. Its size
	  is designed to hold ten characters but is quite flexible.*/
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

	/*: Text displayed on a Ok button of a dialog. The character after
	  the '&' symbol can be used as a hotkey and the '&' symbol itself
	  will not be displayed.*/
	m_sButtonOk = tr( "&Ok" );
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

	m_sMutableDialog = tr( "Don't show this message again" );
	
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
	
	m_sSoundLibraryFailedPreDrumkitLoad = tr( "Drumkit registered in the current song can not be found on disk.\nPlease load an existing drumkit first.\nCurrent kit:" );
	
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

CommonStrings::~CommonStrings(){
}
