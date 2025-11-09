/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */
#ifndef EVENT_LISTENER
#define EVENT_LISTENER

#include <core/AudioEngine/AudioEngine.h>
#include <core/Globals.h>

#include <set>

/** \ingroup docGUI docEvent*/
class EventListener
{
	public:
		virtual void actionModeChangeEvent( int nValue ){ UNUSED( nValue ); }
		virtual void audioDriverChangedEvent(){}
		virtual void bbtChangedEvent(){}
		virtual void beatCounterEvent() {}
		virtual void drumkitLoadedEvent(){}
		virtual void effectChangedEvent(){}
		virtual void errorEvent( int nErrorCode ) { UNUSED( nErrorCode ); }
    	virtual void gridCellToggledEvent(){}
		virtual void instrumentMuteSoloChangedEvent( int nInstrumentIndex ) {
			UNUSED( nInstrumentIndex );
		}
		virtual void instrumentParametersChangedEvent( int nInstrumentIndex ) {
			UNUSED( nInstrumentIndex );
		}
		virtual void jackTransportActivationEvent(){}
		virtual void jackTimebaseStateChangedEvent( int nValue ){ UNUSED( nValue ); }
		virtual void loopModeActivationEvent(){}
		virtual void metronomeEvent( int nValue ) { UNUSED( nValue ); }
		virtual void midiClockActivationEvent(){}
		virtual void midiDriverChangedEvent(){}
		virtual void midiInputEvent() {}
		virtual void midiMapChangedEvent(){}
		virtual void midiOutputEvent() {}
		virtual void mixerSettingsChangedEvent(){}
		virtual void nextPatternsChangedEvent(){}
		virtual void nextShotEvent(){}
		virtual void noteOnEvent( int nInstrument ) { UNUSED( nInstrument ); }
		virtual void patternEditorLockedEvent(){}
		virtual void patternModifiedEvent() {}
		virtual void playbackTrackChangedEvent(){}
		virtual void playingPatternsChangedEvent() {}
		virtual void playlistChangedEvent( int nValue ){ UNUSED( nValue ); }
		virtual void playlistLoadSongEvent(){}
		virtual void progressEvent( int nValue ) { UNUSED( nValue ); }
		virtual void quitEvent( int nValue ){ UNUSED( nValue ); }
		virtual void recordingModeChangedEvent(){}
		virtual void relocationEvent(){}
		virtual void selectedPatternChangedEvent() {}
		virtual void selectedInstrumentChangedEvent() {}
		virtual void songModeActivationEvent(){}
		virtual void songModifiedEvent() {}
		virtual void songSizeChangedEvent(){}
		virtual void soundLibraryChangedEvent(){}
		virtual void stackedModeActivationEvent( int nValue ){ UNUSED( nValue ); }
		virtual void stateChangedEvent( const H2Core::AudioEngine::State& state) {}
		virtual void tempoChangedEvent( int nValue ){ UNUSED( nValue ); }
		virtual void timelineActivationEvent(){}
		virtual void timelineUpdateEvent( int nValue ){ UNUSED( nValue ); }
		virtual void undoRedoActionEvent( int nValue ){ UNUSED( nValue ); }
		virtual void updatePreferencesEvent( int nValue ){ UNUSED( nValue ); }
		virtual void updateSongEvent( int nValue ){ UNUSED( nValue ); }
		virtual void XRunEvent() {}

		virtual ~EventListener() {}

		void blacklistEventId( long nEventId ) {
			if ( m_blacklistedEventIds.find( nEventId ) ==
				 m_blacklistedEventIds.end() ) {
				m_blacklistedEventIds.insert( nEventId );
			}
		}
		bool isEventIdBlacklisted( long nEventId ) const {
			return m_blacklistedEventIds.find( nEventId ) !=
				   m_blacklistedEventIds.end();
		}
		void dropBlacklistedEventId( long nEventId ) {
			const auto it = m_blacklistedEventIds.find( nEventId );
			if ( it != m_blacklistedEventIds.end() ) {
				m_blacklistedEventIds.erase( it );
			}
		}

	private:
		std::set<long> m_blacklistedEventIds;
};


#endif

