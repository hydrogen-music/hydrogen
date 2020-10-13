/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
#ifndef ACTION_H
#define ACTION_H
#include <hydrogen/object.h>
#include <map>
#include <string>
#include <cassert>

using namespace std;


class Action : public H2Core::Object {
	H2_OBJECT
	public:
		Action( QString );

		void setParameter1( QString text ){
			m_sParameter1 = text;
		}

		void setParameter2( QString text ){
			m_sParameter2 = text;
		}

		QString getParameter1() const{
			return m_sParameter1;
		}

		QString getParameter2() const{
			return m_sParameter2;
		}

		QString getType() const{
			return m_sType;
		}

	private:
		QString m_sType;
		QString m_sParameter1;
		QString m_sParameter2;
};

namespace H2Core
{
	class Hydrogen;
}

class MidiActionManager : public H2Core::Object
{
	H2_OBJECT
	private:
		/**
		 * Object holding the current MidiActionManager
		 * singleton. It is initialized with NULL, set with
		 * create_instance(), and accessed with
		 * get_instance().
		 */
		static MidiActionManager *__instance;

		/**
		 * Holds the names of all Action identfiers which Hydrogen is
		 * able to interpret.
		 */
		QStringList actionList;

		/**
		 * Contains all information to find a particular object in a
		 * list of objects, like an effect among all LADSPA effects
		 * present or an individual sample.
		 */
		struct targeted_element {
			/**First level index, like the ID of an Effect or and InstrumentComponent.*/
			int _id;
			/**Second level index, like the ID of an InstrumentLayer.*/
			int _subId;
		};

		typedef bool (MidiActionManager::*action_f)(Action * , H2Core::Hydrogen * , targeted_element );
		/**
		 * Holds all Action identifiers which Hydrogen is able to
		 * interpret.  
		 *
		 * It holds pointer to member function.
		 */
		map<string, pair<action_f, targeted_element> > actionMap;

		bool play(Action * , H2Core::Hydrogen * , targeted_element );
		bool play_stop_pause_toggle(Action * , H2Core::Hydrogen * , targeted_element );
		bool stop(Action * , H2Core::Hydrogen * , targeted_element );
		bool pause(Action * , H2Core::Hydrogen * , targeted_element );
		bool record_ready(Action * , H2Core::Hydrogen * , targeted_element );
		bool record_strobe_toggle(Action * , H2Core::Hydrogen * , targeted_element );
		bool record_strobe(Action * , H2Core::Hydrogen * , targeted_element );
		bool record_exit(Action * , H2Core::Hydrogen * , targeted_element );
		bool mute(Action * , H2Core::Hydrogen * , targeted_element );
		bool unmute(Action * , H2Core::Hydrogen * , targeted_element );
		bool mute_toggle(Action * , H2Core::Hydrogen * , targeted_element );
		bool strip_mute_toggle(Action * , H2Core::Hydrogen * , targeted_element );
		bool strip_solo_toggle(Action * , H2Core::Hydrogen * , targeted_element );
		bool next_bar(Action * , H2Core::Hydrogen * , targeted_element );
		bool previous_bar(Action * , H2Core::Hydrogen * , targeted_element );
		bool bpm_increase(Action * , H2Core::Hydrogen * , targeted_element );
		bool bpm_decrease(Action * , H2Core::Hydrogen * , targeted_element );
		bool bpm_cc_relative(Action * , H2Core::Hydrogen * , targeted_element );
		bool bpm_fine_cc_relative(Action * , H2Core::Hydrogen * , targeted_element );
		bool master_volume_relative(Action * , H2Core::Hydrogen * , targeted_element );
		bool master_volume_absolute(Action * , H2Core::Hydrogen * , targeted_element );
		bool strip_volume_relative(Action * , H2Core::Hydrogen * , targeted_element );
		bool strip_volume_absolute(Action * , H2Core::Hydrogen * , targeted_element );
		bool effect_level_relative(Action * , H2Core::Hydrogen * , targeted_element );
		bool effect_level_absolute(Action * , H2Core::Hydrogen * , targeted_element );
		bool select_next_pattern(Action * , H2Core::Hydrogen * , targeted_element );
		bool select_only_next_pattern(Action * , H2Core::Hydrogen * , targeted_element );
		bool select_next_pattern_cc_absolute(Action * , H2Core::Hydrogen * , targeted_element );
		bool select_next_pattern_promptly(Action * , H2Core::Hydrogen * , targeted_element );
		bool select_next_pattern_relative(Action * , H2Core::Hydrogen * , targeted_element );
		bool select_and_play_pattern(Action * , H2Core::Hydrogen * , targeted_element );
		bool pan_relative(Action * , H2Core::Hydrogen * , targeted_element );
		bool pan_absolute(Action * , H2Core::Hydrogen * , targeted_element );
		bool filter_cutoff_level_absolute(Action * , H2Core::Hydrogen * , targeted_element );
		bool beatcounter(Action * , H2Core::Hydrogen * , targeted_element );
		bool tap_tempo(Action * , H2Core::Hydrogen * , targeted_element );
		bool playlist_song(Action * , H2Core::Hydrogen * , targeted_element );
		bool playlist_next_song(Action * , H2Core::Hydrogen * , targeted_element );
		bool playlist_previous_song(Action * , H2Core::Hydrogen * , targeted_element );
		bool toggle_metronome(Action * , H2Core::Hydrogen * , targeted_element );
		bool select_instrument(Action * , H2Core::Hydrogen * , targeted_element );
		bool undo_action(Action * , H2Core::Hydrogen * , targeted_element );
		bool redo_action(Action * , H2Core::Hydrogen * , targeted_element );
		bool gain_level_absolute(Action * , H2Core::Hydrogen * , targeted_element );
		bool pitch_level_absolute(Action * , H2Core::Hydrogen * , targeted_element );

		// Actions required for session management.
		/**
		 * Create an empty H2Core::Song which will be stored at to the
		 * path provided in @a pAction.
		 *
		 * This will be done without immediately and without saving
		 * the current H2Core::Song. All unsaved changes will be lost!
		 * In addition, the new H2Core::Song won't be saved by this
		 * function. You can do so using save_song().
		 *
		 * The intended use of this function for session
		 * management. Therefore the function will *not* store the
		 * provided in H2Core::Preferences::m_lastSongFilename and
		 * thus H2Core::Hydrogen does not resumes with the particular
		 * H2Core::Song upon restarting.
		 *
		 * Right now the function is only able to handle the provided
		 * path as is. Therefore it is important to provide an absolute
		 * path to a .h2song file.
		 *
		 * \param pAction Action "NEW_SONG" uniquely triggering this function.
		 * \param pHydrogen Pointer to the instance of the H2Core::Hydrogen singleton.
		 * \param element Unused.
		 * \return true on success
		 */
		bool new_song(Action* pAction, H2Core::Hydrogen* pHydrogen, targeted_element element);
		/**
		 * Opens the H2Core::Song specified in the path provided in @a
		 * pAction.
		 *
		 * This will be done without immediately and without saving
		 * the current H2Core::Song. All unsaved changes will be lost!
		 *
		 * The intended use of this function for session
		 * management. Therefore the function will *not* store the
		 * provided in H2Core::Preferences::m_lastSongFilename and
		 * thus H2Core::Hydrogen does not resumes with the particular
		 * H2Core::Song upon restarting.
		 *
		 * Right now the function is only able to handle the provided
		 * path as is. Therefore it is important to provide an absolute
		 * path to a .h2song file.
		 *
		 * \param pAction Action "OPEN_SONG" uniquely triggering this function.
		 * \param pHydrogen Pointer to the instance of the H2Core::Hydrogen singleton.
		 * \param element Unused.
		 * \return true on success
		 */
		bool open_song(Action* pAction, H2Core::Hydrogen* pHydrogen, targeted_element element);
		/**
		 * Saves the current H2Core::Song and the local
		 * H2Core::Preferences.
		 *
		 * Wrapper around save_preferences() and save_song().
		 *
		 * \param pAction Action "SAVE_ALL" uniquely triggering this function.
		 * \param pHydrogen Pointer to the instance of the H2Core::Hydrogen singleton.
		 * \param element Unused.
		 * \return true on success of all three wrapped functions.
		 */
		bool save_all(Action* pAction, H2Core::Hydrogen* pHydrogen, targeted_element element);
		/**
		 * Saves the H2Core::Preferences.
		 *
		 * If the GUI is present - and thus H2Core::Hydrogen::m_iActiveGUI is
		 * not 0 -, the H2Core::EVENT_UPDATE_PREFERENCES event will be
		 * pushed onto the H2Core::EventQueue. It no GUI is present,
		 * H2Core::Preferences::savePreferences() will be called directly.
		 *
		 * \param pAction Action "SAVE_PREFERENCES" uniquely triggering this function.
		 * \param pHydrogen Pointer to the instance of the H2Core::Hydrogen singleton.
		 * \param element Unused.
		 * \return true on success
		 */
		bool save_preferences(Action* pAction, H2Core::Hydrogen* pHydrogen, targeted_element element);
		bool save_drumkit(Action* pAction, H2Core::Hydrogen* pHydrogen, targeted_element element);
		/**
		 * Saves the current H2Core::Song.
		 *
		 * The intended use of this function for session
		 * management. Therefore the function will *not* store the
		 * provided in H2Core::Preferences::m_lastSongFilename and
		 * thus H2Core::Hydrogen does not resumes with the particular
		 * H2Core::Song upon restarting.
		 *
		 * \param pAction Action "SAVE_SONG" uniquely triggering this function.
		 * \param pHydrogen Pointer to the instance of the H2Core::Hydrogen singleton.
		 * \param element Unused.
		 * \return true on success
		 */
		bool save_song(Action* pAction, H2Core::Hydrogen* pHydrogen, targeted_element element);
		/**
		 * Saves the current H2Core::Song to the path provided in @a pAction.
		 *
		 * The intended use of this function for session
		 * management. Therefore the function will *not* store the
		 * provided in H2Core::Preferences::m_lastSongFilename and
		 * thus H2Core::Hydrogen does not resumes with the particular
		 * H2Core::Song upon restarting.
		 *
		 * \param pAction Action "SAVE_SONG_AS" uniquely triggering this function.
		 * \param pHydrogen Pointer to the instance of the H2Core::Hydrogen singleton.
		 * \param element Unused.
		 * \return true on success
		 */
		bool save_song_as(Action* pAction, H2Core::Hydrogen* pHydrogen, targeted_element element);
		/**
		 * Triggers the shutdown of H2Core::Hydrogen.
		 *
		 * This will be done without immediately and without saving
		 * the current H2Core::Song. All unsaved changes will be lost!
		 *
		 * The shutdown will only be triggered if
		 * H2Core::Hydrogen::m_iActiveGUI is true and the Qt5 GUI is
		 * present.
		 *
		 * \param pAction Action "QUIT" uniquely triggering this function.
		 * \param pHydrogen Pointer to the instance of the H2Core::Hydrogen singleton.
		 * \param element Unused.
		 * \return true on success
		 */
		bool quit(Action* pAction, H2Core::Hydrogen* pHydrogen, targeted_element element);

		QStringList eventList;

		int m_nLastBpmChangeCCParameter;

		/**
		 * Checks the path of the .h2song provided via OSC.
		 *
		 * It will be checked whether @a songPath
		 * - is absolute
		 * - has the '.h2song' suffix
		 * - is writable (if it exists)
		 *
		 * \param songPath Absolute path to an .h2song file.
		 * \return true - if valid.
		 */
		bool isSongPathValid( const QString& songPath );

	public:

		/**
		 * The handleAction method is the heart of the
		 * MidiActionManager class. It executes the operations that
		 * are needed to carry the desired action.
		 */
		bool handleAction( Action * );
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

		QStringList getActionList(){
			return actionList;
		}

		QStringList getEventList(){
			return eventList;
		}

		MidiActionManager();
		~MidiActionManager();
};
#endif
