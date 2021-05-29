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

/*: Text displayed on the button for soloing an instrument strip in
  the mixer. Its size is designed for a single character.*/
QString CommonStrings::m_sSmallSoloButton( tr( "S" ) );
/*: Text displayed on the button for muting an instrument strip in
  the mixer. Its size is designed for a single character.*/
QString CommonStrings::m_sSmallMuteButton( tr( "M" ) );
/*: Text displayed on the button for muting the master strip. Its
  size is designed for a four characters.*/
QString CommonStrings::m_sBigMuteButton( tr( "MUTE" ) );
/*: Text displayed on the button for bypassing an element. Its
  size is designed for a three characters.*/
QString CommonStrings::m_sBypassButton( tr( "BYP" ) );
/*: Text displayed on the button for editin an element. Its
  size is designed for a four characters.*/
QString CommonStrings::m_sEditButton( tr( "EDIT" ) );
/*: Text displayed on the button to clear all patterns in the
  SongEditor. Its size is designed to hold five characters.*/
QString CommonStrings::m_sClearButton( tr( "CLEAR" ) );
/*: Text displayed on the button to show the Playback track. Its size
  is designed to hold a single character.*/
QString CommonStrings::m_sPlaybackTrackButton( tr( "P" ) );
/*: Text displayed on the button to show the Timeline. Its size
  is designed to hold a single character.*/
QString CommonStrings::m_sTimelineButton( tr( "T" ) );
/*: Text displayed on the button to activate the Timeline. Its size
  is designed to hold three characters.*/
QString CommonStrings::m_sTimelineBigButton( tr( "BPM" ) );
/*: Text displayed on the button to enable the LADSPA effect strips. Its size
  is designed to hold two characters.*/
QString CommonStrings::m_sFXButton( tr( "FX" ) );
/*: Text displayed on the button to show the instrument peaks. Its size
  is designed to hold four characters.*/
QString CommonStrings::m_sPeakButton( tr( "PEAK" ) );
/*: Text displayed on the button to show the Instrument Rack. Its size
  is designed to hold seven characters but is quite flexible.*/
QString CommonStrings::m_sGeneralButton( tr( "General" ) );
/*: Text displayed on the button to show the Instrument Editor in the Instrument Rack. Its size
  is designed to hold ten characters but is quite flexible.*/
QString CommonStrings::m_sInstrumentButton( tr( "Instrument" ) );
/*: Text displayed on the button to show the Sound Library in the Instrument Rack. Its size
  is designed to hold ten characters but is quite flexible.*/
QString CommonStrings::m_sSoundLibraryButton( tr( "Sound Library" ) );
/*: Text displayed on the button to show the Layer view of the Instrument Rack. Its size
  is designed to hold six characters but is quite flexible.*/
QString CommonStrings::m_sLayersButton( tr( "Layers" ) );
/*: Text displayed on the button to load a layer into an
  instrument. Its size is designed to hold ten characters but is quite
  flexible.*/
QString CommonStrings::m_sLoadLayerButton( tr( "Load Layer" ) );

/*: Text displayed on the button to delete a layer into an
  instrument. Its size is designed to hold twelve characters but is quite
  flexible.*/
QString CommonStrings::m_sDeleteLayerButton( tr( "Delete Layer" ) );

/*: Text displayed on the button to edit a layer into an
  instrument. Its size is designed to hold ten characters but is quite
  flexible.*/
QString CommonStrings::m_sEditLayerButton( tr( "Edit Layer" ) );

/*: Text displayed on the button to activate the Beat Counter. Its
  size is designed to hold two characters in two separate rows.*/
QString CommonStrings::m_sBeatCounterButton( tr( "B\nC" ) );

/*: Text displayed on the button indicating that the Beat Counter will
  only set tempo. Its size is designed to hold one character.*/
QString CommonStrings::m_sBeatCounterSetPlayButtonOff( tr( "S" ) );
/*: Text displayed on the button indicating that the Beat Counter will
  start playing after setting the tempo. Its size is designed to hold one character.*/
QString CommonStrings::m_sBeatCounterSetPlayButtonOn( tr( "P" ) );

/*: Text displayed on the button to activate the resampling using 
  Rubberband. Its
  size is designed to hold three characters in two separate rows.*/
QString CommonStrings::m_sRubberbandButton( tr( "R\nU\nB" ) );

/*: Text displayed on the button to activate the JACK transport control. Its
  size is designed to hold seven characters and is moderately flexible.*/
QString CommonStrings::m_sJackTransportButton( tr( "J.TRANS" ) );
/*: Text displayed on the button to activate the JACK Timebase master control. Its
  size is designed to hold eight characters and is moderately flexible.*/
QString CommonStrings::m_sJackMasterButton( tr( "J.MASTER" ) );
/*: Text displayed on the button to show the Mixer window. Its
  size is designed to hold five characters and is flexible.*/
QString CommonStrings::m_sMixerButton( tr( "Mixer" ) );
/*: Text displayed on the button to show the Instrument Rack. Its
  size is designed to hold 15 characters and is flexible.*/
QString CommonStrings::m_sInstrumentRackButton( tr( "Instrument Rack" ) );

/*: Text displayed on the button activating Pattern Mode for playback. Its
  size is designed to hold seven characters and is slightly flexible.*/
QString CommonStrings::m_sPatternModeButton( tr( "PATTERN" ) );
/*: Text displayed on the button activating Song Mode for playback. Its
  size is designed to hold four characters and is slightly flexible.*/
QString CommonStrings::m_sSongModeButton( tr( "SONG" ) );
