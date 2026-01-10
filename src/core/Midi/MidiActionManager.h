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
#ifndef MIDI_ACTION_MANAGER_H
#define MIDI_ACTION_MANAGER_H

#include <core/Helpers/Time.h>
#include <core/Midi/MidiAction.h>
#include <core/Object.h>

#include <QString>

#include <cassert>
#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <thread>
#include <vector>

namespace H2Core {

/**
* @class MidiActionManager
*
* @brief The MidiActionManager cares for the execution of MidiActions
*
*
* The MidiActionManager handles the execution of midi actions. The class
* includes the names and implementations of all possible actions.
*
*
* @author Sebastian Moors
*
* \ingroup docCore docMIDI */
class MidiActionManager : public H2Core::Object<MidiActionManager>
{
	H2_OBJECT(MidiActionManager)

	public:

		/** When calculating the tempo based on incoming MIDI clock messages, we
		 * for this amount of messages until we average. */
		static constexpr int nMidiClockIntervals = 10;
		static constexpr int nActionQueueMaxSize = 200;

		MidiActionManager();
		~MidiActionManager();

		/**
		 * Handles multiple actions at once and calls handleMidiActionAsync() on
		 * them.
		 *
		 * This variant should be called from threads which strive to for
		 * realtime handling of incoming event, like MIDI driver and OSC
		 * handler.
		 *
		 * \return true - if at least one MidiAction was queued
		 *   successfully. Calling functions should treat the event
		 *   resulting in @a actions as consumed.
		 */
		bool handleMidiActionsAsync( const std::vector<std::shared_ptr<MidiAction>>& actions );
		/**
		 * This is the heart of the MidiActionManager class. It queues the
		 * operations to be executed in its worker thread.
		 *
		 * This variant should be called from threads which strive to for
		 * realtime handling of incoming event, like MIDI driver and OSC
		 * handler.
		 *
		 * @return true - if @a MidiAction was queued successfully.
		 */
		bool handleMidiActionAsync( const std::shared_ptr<MidiAction> MidiAction );

		/**
		 * Instead of using #m_actionQueue and the worker thread to handle the
		 * provided action, it will be executed right away.
		 *
		 * This flavour of handleMidiActionAsync() should be called for actions
		 * triggered by the GUI or core itself. Those should not enter the queue
		 * in order to allow for a more realtime-ish handling of incoming MIDI
		 * and OSC events.
		 *
		 * @return true - if @a MidiAction was executed successfully.
		 */
		bool handleMidiActionSync( const std::shared_ptr<MidiAction> MidiAction );

		const std::set<MidiAction::Type>& getMidiActions() const {
			return m_midiActions;
		}
		int getActionQueueSize();

		/** \return -1 in case the @a couldn't be found. */
		int getParameterNumber( const MidiAction::Type& type ) const;

		void setPendingStart( bool bPending );

		void resetTimingClockTicks();

	private:

		/** Holds all Actions which Hydrogen is able to interpret. */
		std::set<MidiAction::Type> m_midiActions;

		typedef bool (MidiActionManager::*action_f)( std::shared_ptr<MidiAction> );
		/**
		 * Holds all MidiAction identifiers which Hydrogen is able to
		 * interpret.  
		 *
		 * It holds a pair consisting of a pointer to member function
		 * performing the desired MidiAction and an integer specifying how
		 * many additional MidiAction parameters are required to do so.
		 */
		std::map<MidiAction::Type, std::pair<action_f,int>> m_midiActionMap;
		bool beatcounter( std::shared_ptr<MidiAction> );
		bool bpmCcRelative( std::shared_ptr<MidiAction> );
		bool bpmDecrease( std::shared_ptr<MidiAction> );
		bool bpmFineCcRelative( std::shared_ptr<MidiAction> );
		bool bpmIncrease( std::shared_ptr<MidiAction> );
		bool clearPattern( std::shared_ptr<MidiAction> );
		bool clearSelectedInstrument( std::shared_ptr<MidiAction> );
		bool countIn( std::shared_ptr<MidiAction> );
		bool countInPauseToggle( std::shared_ptr<MidiAction> );
		bool countInStopToggle( std::shared_ptr<MidiAction> );
		bool effectLevelAbsolute( std::shared_ptr<MidiAction> );
		bool effectLevelRelative( std::shared_ptr<MidiAction> );
		bool filterCutoffLevelAbsolute( std::shared_ptr<MidiAction> );
		bool gainLevelAbsolute( std::shared_ptr<MidiAction> );
		bool humanizationSwingAbsolute( std::shared_ptr<MidiAction> );
		bool humanizationSwingRelative( std::shared_ptr<MidiAction> );
		bool humanizationTimingAbsolute( std::shared_ptr<MidiAction> );
		bool humanizationTimingRelative( std::shared_ptr<MidiAction> );
		bool humanizationVelocityAbsolute( std::shared_ptr<MidiAction> );
		bool humanizationVelocityRelative( std::shared_ptr<MidiAction> );
		bool instrumentPitch( std::shared_ptr<MidiAction> );
		bool loadNextDrumkit( std::shared_ptr<MidiAction> );
		bool loadPrevDrumkit( std::shared_ptr<MidiAction> );
		bool masterVolumeAbsolute( std::shared_ptr<MidiAction> );
		bool masterVolumeRelative( std::shared_ptr<MidiAction>);
		bool mute( std::shared_ptr<MidiAction> );
		bool muteToggle( std::shared_ptr<MidiAction> );
		bool nextBar( std::shared_ptr<MidiAction> );
		bool panAbsolute( std::shared_ptr<MidiAction> );
		bool panAbsoluteSym( std::shared_ptr<MidiAction> );
		bool panRelative( std::shared_ptr<MidiAction> );
		bool pause( std::shared_ptr<MidiAction> );
		bool pitchLevelAbsolute( std::shared_ptr<MidiAction> );
		bool play( std::shared_ptr<MidiAction> );
		bool playlistNextSong( std::shared_ptr<MidiAction> );
		bool playlistPreviousSong( std::shared_ptr<MidiAction> );
		bool playlistSong( std::shared_ptr<MidiAction> );
		bool playPauseToggle( std::shared_ptr<MidiAction> );
		bool playStopToggle( std::shared_ptr<MidiAction> );
		bool previousBar( std::shared_ptr<MidiAction> );
		bool recordExit( std::shared_ptr<MidiAction> );
		bool recordReady( std::shared_ptr<MidiAction> );
		bool recordStrobe( std::shared_ptr<MidiAction> );
		bool recordStrobeToggle( std::shared_ptr<MidiAction> );
		bool redoAction( std::shared_ptr<MidiAction> );
		bool selectAndPlayPattern( std::shared_ptr<MidiAction> );
		bool selectInstrument( std::shared_ptr<MidiAction> );
		bool selectNextPattern( std::shared_ptr<MidiAction> );
		bool selectNextPatternCcAbsolute( std::shared_ptr<MidiAction> );
		bool selectNextPatternRelative( std::shared_ptr<MidiAction> );
		bool selectOnlyNextPattern( std::shared_ptr<MidiAction> );
		bool selectOnlyNextPatternCcAbsolute( std::shared_ptr<MidiAction> );
		bool stop( std::shared_ptr<MidiAction> );
		bool stripMuteToggle( std::shared_ptr<MidiAction> );
		bool stripSoloToggle( std::shared_ptr<MidiAction> );
		bool stripVolumeAbsolute( std::shared_ptr<MidiAction> );
		bool stripVolumeRelative( std::shared_ptr<MidiAction> );
		bool tapTempo( std::shared_ptr<MidiAction> );
		bool timingClockTick( std::shared_ptr<MidiAction> pAction );
		bool toggleMetronome( std::shared_ptr<MidiAction> );
		bool undoAction( std::shared_ptr<MidiAction> );
		bool unmute( std::shared_ptr<MidiAction> );

		bool nextPatternSelection( int nPatternNumber );
		bool onlyNextPatternSelection( int nPatternNumber );
		bool setSongFromPlaylist( int nSongNumber );

		/** @a pInstance is expecting a pointer to the instance of the class for
		 * which the message handling thread is spawned. */
		static void workerThread( void* pInstance );


		int m_nLastBpmChangeCCParameter;

		//! Members required to handle incoming MIDI clock messages.
		//! @{
		TimePoint m_lastTick;
		std::vector<double> m_tickIntervals;
		int m_nTickIntervalIndex;
		/** Whether we already retrieved #nMidiClockTicks events in a row. */
		bool m_bMidiClockReady;

		/** In MIDI sync mode as speficied in MIDI 1.0 - if both MIDI clock and
		 * transport handling is enabled by the user - starting transport after
		 * receiving CONTINUE or START MIDI messages will only start at the next
		 * MIDI clock tick. This state indicates that we have received the
		 * former message. */
		bool m_bPendingStart;
		//! @}

		std::shared_ptr< std::thread > m_pWorkerThread;

		bool m_bWorkerShutdown;

		/** Shared data
		 * @{ */
		std::condition_variable m_workerThreadCV;
		std::mutex m_workerThreadMutex;
		/** Since MIDI driver input handler and OSC handler are both kept fast
		 * and lean, it should be more or less guaranteed that incoming
		 * #MidiAction are ordered chronologically. No need to increase
		 * complexity by e.g. using a priority_queue. */
		std::deque< std::shared_ptr<MidiAction> > m_actionQueue;
		/** @}*/
};

inline void MidiActionManager::setPendingStart( bool bPending ) {
	if ( m_bPendingStart != bPending ) {
		m_bPendingStart = bPending;
	}
}

};

#endif
