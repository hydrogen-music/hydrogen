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
		bool play( std::shared_ptr<MidiAction> );
		bool play_pause_toggle( std::shared_ptr<MidiAction> );
		bool play_stop_toggle( std::shared_ptr<MidiAction> );
		bool stop( std::shared_ptr<MidiAction> );
		bool pause( std::shared_ptr<MidiAction> );
		bool record_ready( std::shared_ptr<MidiAction> );
		bool record_strobe_toggle( std::shared_ptr<MidiAction> );
		bool record_strobe( std::shared_ptr<MidiAction> );
		bool record_exit( std::shared_ptr<MidiAction> );
		bool mute( std::shared_ptr<MidiAction> );
		bool unmute( std::shared_ptr<MidiAction> );
		bool mute_toggle( std::shared_ptr<MidiAction> );
		bool strip_mute_toggle( std::shared_ptr<MidiAction> );
		bool strip_solo_toggle( std::shared_ptr<MidiAction> );
		bool next_bar( std::shared_ptr<MidiAction> );
		bool previous_bar( std::shared_ptr<MidiAction> );
		bool bpm_increase( std::shared_ptr<MidiAction> );
		bool bpm_decrease( std::shared_ptr<MidiAction> );
		bool bpm_cc_relative( std::shared_ptr<MidiAction> );
		bool bpm_fine_cc_relative( std::shared_ptr<MidiAction> );
		bool master_volume_relative( std::shared_ptr<MidiAction>);
		bool master_volume_absolute( std::shared_ptr<MidiAction> );
		bool strip_volume_relative( std::shared_ptr<MidiAction> );
		bool strip_volume_absolute( std::shared_ptr<MidiAction> );
		bool effect_level_relative( std::shared_ptr<MidiAction> );
		bool effect_level_absolute( std::shared_ptr<MidiAction> );
		bool select_next_pattern( std::shared_ptr<MidiAction> );
	bool select_only_next_pattern( std::shared_ptr<MidiAction> );
	bool select_only_next_pattern_cc_absolute( std::shared_ptr<MidiAction> );
		bool select_next_pattern_cc_absolute( std::shared_ptr<MidiAction> );
		bool select_next_pattern_relative( std::shared_ptr<MidiAction> );
		bool select_and_play_pattern( std::shared_ptr<MidiAction> );
		bool pan_relative( std::shared_ptr<MidiAction> );
		bool pan_absolute( std::shared_ptr<MidiAction> );
	bool pan_absolute_sym( std::shared_ptr<MidiAction> );
		bool instrument_pitch( std::shared_ptr<MidiAction> );
		bool filter_cutoff_level_absolute( std::shared_ptr<MidiAction> );
		bool beatcounter( std::shared_ptr<MidiAction> );
		bool tap_tempo( std::shared_ptr<MidiAction> );
		bool playlist_song( std::shared_ptr<MidiAction> );
		bool playlist_next_song( std::shared_ptr<MidiAction> );
		bool playlist_previous_song( std::shared_ptr<MidiAction> );
		bool toggle_metronome( std::shared_ptr<MidiAction> );
		bool select_instrument( std::shared_ptr<MidiAction> );
		bool undo_action( std::shared_ptr<MidiAction> );
		bool redo_action( std::shared_ptr<MidiAction> );
		bool gain_level_absolute( std::shared_ptr<MidiAction> );
		bool pitch_level_absolute( std::shared_ptr<MidiAction> );
		bool clear_selected_instrument( std::shared_ptr<MidiAction> );
		bool clear_pattern( std::shared_ptr<MidiAction> );
		bool loadNextDrumkit( std::shared_ptr<MidiAction> );
		bool loadPrevDrumkit( std::shared_ptr<MidiAction> );

		int m_nLastBpmChangeCCParameter;

	bool setSongFromPlaylist( int nSongNumber );
	bool nextPatternSelection( int nPatternNumber );
	bool onlyNextPatternSelection( int nPatternNumber );

	public:

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

		const std::set<MidiAction::Type>& getMidiActions() const {
			return m_midiActions;
		}

		/** \return -1 in case the @a couldn't be found. */
		int getParameterNumber( const MidiAction::Type& type ) const;

		MidiActionManager();
		~MidiActionManager();
};
#endif
