/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */
#ifndef EVENT_LISTENER
#define EVENT_LISTENER

#include <core/Globals.h>
#include <core/AudioEngine/AudioEngine.h>
/** \ingroup docGUI docEvent*/
class EventListener
{
	public:
		virtual void stateChangedEvent( const H2Core::AudioEngine::State& state) {}
	virtual void playingPatternsChangedEvent() {}
	virtual void nextPatternsChangedEvent(){}
		virtual void patternModifiedEvent() {}
		virtual void songModifiedEvent() {}
		virtual void selectedPatternChangedEvent() {}
		virtual void selectedInstrumentChangedEvent() {}
	virtual void instrumentParametersChangedEvent( int nInstrumentNumber ) { UNUSED( nInstrumentNumber ); }
		virtual void midiActivityEvent() {}
		virtual void noteOnEvent( int nInstrument ) { UNUSED( nInstrument ); }
		virtual void XRunEvent() {}
		virtual void errorEvent( int nErrorCode ) { UNUSED( nErrorCode ); }
		virtual void metronomeEvent( int nValue ) { UNUSED( nValue ); }
		virtual void progressEvent( int nValue ) { UNUSED( nValue ); }
		virtual void jacksessionEvent( int nValue) { UNUSED( nValue ); }
		virtual void playlistLoadSongEvent(){}
		virtual void undoRedoActionEvent( int nValue ){ UNUSED( nValue ); }
		virtual void tempoChangedEvent( int nValue ){ UNUSED( nValue ); }
		virtual void updateSongEvent( int nValue ){ UNUSED( nValue ); }
		virtual void quitEvent( int nValue ){ UNUSED( nValue ); }
		virtual void timelineActivationEvent(){}
		virtual void timelineUpdateEvent( int nValue ){ UNUSED( nValue ); }
		virtual void jackTransportActivationEvent(){}
		virtual void jackTimebaseStateChangedEvent( int nValue ){ UNUSED( nValue ); }
		virtual void songModeActivationEvent(){}
		virtual void stackedModeActivationEvent( int nValue ){ UNUSED( nValue ); }
		virtual void loopModeActivationEvent(){}
		virtual void updatePreferencesEvent( int nValue ){ UNUSED( nValue ); }
		virtual void actionModeChangeEvent( int nValue ){ UNUSED( nValue ); }
    	virtual void gridCellToggledEvent(){}
	virtual void drumkitLoadedEvent(){}
	virtual void patternEditorLockedEvent(){}
	virtual void relocationEvent(){}
	virtual void bbtChangedEvent(){}
	virtual void songSizeChangedEvent(){}
	virtual void driverChangedEvent(){}
	virtual void playbackTrackChangedEvent(){}
	virtual void soundLibraryChangedEvent(){}
	virtual void nextShotEvent(){}
	virtual void midiMapChangedEvent(){}
	virtual void playlistChangedEvent( int nValue ){ UNUSED( nValue ); }

		virtual ~EventListener() {}
};


#endif

