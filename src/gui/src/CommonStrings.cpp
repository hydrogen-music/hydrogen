/*
 * Hydrogen
 * Copyright (C) 2021 The hydrogen development team <hydrogen-devel@lists.sourceforge.net>
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

const char* CommonStrings::__class_name = "CommonStrings";

CommonStrings::CommonStrings() : Object( __class_name ) {
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
	m_sBigMuteButton = tr( "MUTE" );
	/*: Text displayed on the button for bypassing an element. Its
	  size is designed for a three characters.*/
	m_sBypassButton = tr( "BYP" );
	/*: Text displayed on the button for editin an element. Its
	  size is designed for a four characters.*/
	m_sEditButton = tr( "EDIT" );
	/*: Text displayed on the button to clear all patterns in the
	  SongEditor. Its size is designed to hold five characters.*/
	m_sClearButton = tr( "CLEAR" );
	/*: Text displayed on the button to show the Timeline. Its size
	  is designed to hold a single character.*/
	m_sTimelineButton = tr( "T" );
	/*: Text displayed on the button to activate the Timeline. Its size
	  is designed to hold three characters.*/
	m_sTimelineBigButton = tr( "BPM" );
	/*: Text displayed on the button to enable the LADSPA effect strips. Its size
	  is designed to hold two characters.*/
	m_sFXButton = tr( "FX" );
	/*: Text displayed on the button to show the instrument peaks. Its size
	  is designed to hold four characters.*/
	m_sPeakButton = tr( "PEAK" );
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
	m_sJackTransportButton = tr( "J.TRANS" );
	/*: Text displayed on the button to activate the JACK Timebase master control. Its
	  size is designed to hold eight characters and is moderately flexible.*/
	m_sJackMasterButton = tr( "J.MASTER" );
	/*: Text displayed on the button to show the Mixer window. Its
	  size is designed to hold five characters and is flexible.*/
	m_sMixerButton = tr( "Mixer" );
	/*: Text displayed on the button to show the Instrument Rack. Its
	  size is designed to hold 15 characters and is flexible.*/
	m_sInstrumentRackButton = tr( "Instrument Rack" );

	/*: Text displayed on the button activating Pattern Mode for playback. Its
	  size is designed to hold seven characters and is slightly flexible.*/
	m_sPatternModeButton = tr( "PATTERN" );
	/*: Text displayed on the button activating Song Mode for playback. Its
	  size is designed to hold four characters and is slightly flexible.*/
	m_sSongModeButton = tr( "SONG" );

	/*: Text displayed below the rotary to adjust the attack of the
	  ADSR in the Instrument Editor. Designed to hold six characters
	  but flexible.*/
	m_sAttackLabel = tr( "ATTACK" );
	/*: Text displayed below the rotary to adjust the decay of the
	  ADSR in the Instrument Editor. Designed to hold five characters
	  but flexible.*/
	m_sDecayLabel = tr( "DECAY" );
	/*: Text displayed below the rotary to adjust the sustain of the
	  ADSR in the Instrument Editor. Designed to hold seven characters
	  but flexible.*/
	m_sSustainLabel = tr( "SUSTAIN" );
	/*: Text displayed below the rotary to adjust the release of the
	  ADSR in the Instrument Editor. Designed to hold seven characters
	  but flexible.*/
	m_sReleaseLabel = tr( "RELEASE" );
	/*: Text displayed below the LCD to set the output MIDI channel
	  in the Instrument Editor. Designed to hold seven characters but
	  flexible.*/
	m_sMidiOutChannelLabel = tr( "CHANNEL" );
	/*: Text displayed below the LCD to set the output MIDI note
	  in the Instrument Editor. Designed to hold four characters but
	  flexible.*/
	m_sMidiOutNoteLabel = tr( "NOTE" );
	/*: Text displayed in the Instrument Editor in the row of the
	  pitch widget. Designed to hold five characters but flexible.*/
	m_sPitchLabel = tr( "PITCH" );
	/*: Text displayed below the rotary to adjust the deterministic
	  part of the instrument pitch in front of decimal point in the
	  Instrument Editor. Designed to hold six characters but
	  flexible.*/
	m_sPitchCoarseLabel = tr( "COARSE" );
	/*: Text displayed below the rotary to adjust the deterministic
	  part of the instrument pitch after decimal point in the
	  Instrument Editor. Designed to hold four characters but
	  flexible.*/
	m_sPitchFineLabel = tr( "FINE" );
	/*: Text displayed below the rotary to adjust the random part of
	  the instrument pitch in the Instrument Editor. Designed to hold
	 six characters but flexible.*/
	m_sPitchRandomLabel = tr( "RANDOM" );
	/*: Text displayed below the rotary to adjust the instrument gain
	 in the Instrument Editor. Designed to hold four characters but
	 flexible.*/
	m_sGainLabel = tr( "GAIN" );
	/*: Text displayed below the LCD to set the mute group in the
	 Instrument Editor. Designed to hold ten characters but
	 flexible.*/
	m_sMuteGroupLabel = tr( "MUTE GROUP" );
	/*: Text displayed next to the checkbox to activate the auto stop
	 note feature in the Instrument Editor. Designed to hold 14
	 characters but flexible.*/
	m_sIsStopNoteLabel = tr( "AUTO-STOP-NOTE" );
	/*: Text displayed next to the checkbox to activate the apply
	 velocity feature in the Instrument Editor. Designed to hold 14
	 characters but flexible.*/
	m_sApplyVelocityLabel = tr( "APPLY VELOCITY" );
	/*: Text displayed below the LCD to set the hihat pressure group
	 in the Instrument Editor. Designed to hold 13 characters but
	 is only moderately flexible.*/
	m_sHihatGroupLabel = tr( "HH PRESS. GRP" );
	/*: Text displayed below the LCD to set the maximum range of the
	 hihat pressure group in the Instrument Editor. Designed to hold
	 ten characters but flexible.*/
	m_sHihatMaxRangeLabel = tr( "MAX. RANGE" );
	/*: Text displayed below the LCD to set the minimum range of the
	 hihat pressure group in the Instrument Editor. Designed to hold
	 ten characters but flexible.*/
	m_sHihatMinRangeLabel = tr( "MIN. RANGE" );
	/*: Text displayed below the rotary to adjust the cutoff frequency
	 of the lowpass filter applied to the instrument in the Instrument
	 Editor. Designed to hold six characters but flexible.*/
	m_sCutoffLabel = tr( "CUTOFF" );
	/*: Text displayed below the rotary to adjust the resonance frequency
	 of the lowpass filter applied to the instrument in the Instrument
	 Editor. Designed to hold ten characters but flexible.*/
	m_sResonanceLabel = tr( "RESONANCE" );
	/*: Text displayed below the rotary to adjust the layer gain
	 in the Instrument Editor. Designed to hold six characters but
	 flexible.*/
	m_sLayerGainLabel = tr( "L. GAIN" );
	/*: Text displayed below the rotary to adjust the component gain
	 in the Instrument Editor. Designed to hold six characters but
	 flexible.*/
	m_sComponentGainLabel = tr( "C. GAIN" );
	/*: Text displayed left of the sample selection LCD combo in the
	 Instrument Editor. Designed to hold eleven characters but not
	 that flexible.*/
	m_sSampleSelectionLabel = tr( "SAMPLE SEL." );
	/*: Text displayed left of the pattern size LCD combo in the panel
	 of the Pattern Editor. Designed to hold four characters but not
	 that flexible.*/
	m_sPatternSizeLabel = tr( "SIZE" );
	/*: Text displayed left of the resolution LCD combo in the panel
	 of the Pattern Editor. Designed to hold three characters but not
	 that flexible.*/
	m_sResolutionLabel = tr( "RES" );
	/*: Text displayed left of the button to activate the playback of
	 inserted notes in the panel of the Pattern Editor. Designed to
	 hold four characters but not that flexible.*/
	m_sHearNotesLabel = tr( "HEAR" );
	/*: Text displayed left of the button to toggle the quantization
	 in the panel of the Pattern Editor. Designed to hold five
	 characters but not that flexible.*/
	m_sQuantizeEventsLabel = tr( "QUANT" );
	/*: Text displayed left of the button to switch between the
	 Drum Pattern Editor and the Piano Roll Editor in the panel
	 of the Pattern Editor. Designed to hold five characters but not
	 that flexible.*/
	m_sShowPianoLabel = tr( "INPUT" );
	/*: Text displayed left of the button to switch between the
	 Drum Pattern Editor and the Piano Roll Editor in the panel
	 of the Pattern Editor. Designed to hold five characters but not
	 that flexible.*/
	m_sShowPianoLabel = tr( "INPUT" );
	/*: Text displayed in the Player Control to indicate incoming MIDI
	  events. Designed to hold seven characters but not that
	  flexible.*/
	m_sMidiInLabel = tr( "MIDI-IN" );
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
	m_sTimeHoursLabel = tr( "HRS" );
	/*: Text displayed in the Player Control to indicate the number of
	 minutes passed since playback started. Designed to hold three
	 characters but not that flexible.*/
	m_sTimeMinutesLabel = tr( "MIN" );
	/*: Text displayed in the Player Control to indicate the number of
	 seconds passed since playback started. Designed to hold three
	 characters but not that flexible.*/
	m_sTimeSecondsLabel = tr( "SEC" );
	/*: Text displayed in the Player Control to indicate the number of
	 milliseconds passed since playback started. Designed to hold three
	 characters but not that flexible.*/
	m_sTimeMilliSecondsLabel = tr( "1/1000" );

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
}

CommonStrings::~CommonStrings(){
}
