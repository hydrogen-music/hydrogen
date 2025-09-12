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

#include <core/Midi/MidiAction.h>
#include <core/Object.h>

#include <QString>

#include <cassert>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace H2Core
{
	class Hydrogen;
}

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

		MidiActionManager();
		~MidiActionManager();
		/**
		 * If #__instance equals 0, a new MidiActionManager
		 * singleton will be created and stored in it.
		 *
		 * It is called in H2Core::Hydrogen::create_instance().
		 */
		static void create_instance();
		/**
		 * Returns a pointer to the current MidiActionManager
		 * singleton stored in #__instance.
		 */
		static MidiActionManager* get_instance() { assert(__instance); return __instance; }

		/**
		 * Handles multiple actions at once and calls handleAction()
		 * on them.
		 *
		 * \return true - if at least one MidiAction was handled
		 *   successfully. Calling functions should treat the event
		 *   resulting in @a actions as consumed.
		 */
		bool handleMidiActions( const std::vector<std::shared_ptr<MidiAction>>& actions );
		/**
		 * The handleAction method is the heart of the
		 * MidiActionManager class. It executes the operations that
		 * are needed to carry the desired MidiAction.
		 *
		 * @return true - if @a MidiAction was handled successfully.
		 */
		bool handleMidiAction( const std::shared_ptr<MidiAction> MidiAction );

		const std::set<MidiAction::Type>& getMidiActions() const {
			return m_midiActions;
		}

		/** \return -1 in case the @a couldn't be found. */
		int getParameterNumber( const MidiAction::Type& type ) const;

	private:
		/**
		 * Object holding the current MidiActionManager
		 * singleton. It is initialized with NULL, set with
		 * create_instance(), and accessed with
		 * get_instance().
		 */
		static MidiActionManager *__instance;

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
		bool effectLevelAbsolute( std::shared_ptr<MidiAction> );
		bool effectLevelRelative( std::shared_ptr<MidiAction> );
		bool filterCutoffLevelAbsolute( std::shared_ptr<MidiAction> );
		bool gainLevelAbsolute( std::shared_ptr<MidiAction> );
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
		bool toggleMetronome( std::shared_ptr<MidiAction> );
		bool undoAction( std::shared_ptr<MidiAction> );
		bool unmute( std::shared_ptr<MidiAction> );

		bool nextPatternSelection( int nPatternNumber );
		bool onlyNextPatternSelection( int nPatternNumber );
		bool setSongFromPlaylist( int nSongNumber );

		int m_nLastBpmChangeCCParameter;
};
#endif
