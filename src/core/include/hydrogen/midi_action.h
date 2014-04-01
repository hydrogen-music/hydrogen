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


class MidiAction : public H2Core::Object {
	H2_OBJECT
	public:
		MidiAction( QString );

		void setParameter1( QString text ){
			parameter1 = text;
		}

		void setParameter2( QString text ){
			parameter2 = text;
		}

		QString getParameter1(){
			return parameter1;
		}

		QString getParameter2(){
			return parameter2;
		}

		QString getType(){
			return type;
		}

	private:
		QString type;
		QString parameter1;
		QString parameter2;
};

namespace H2Core
{
	class Hydrogen;
}

class MidiActionManager : public H2Core::Object
{
	H2_OBJECT
	private:
		static MidiActionManager *__instance;

		QStringList actionList;
		typedef bool (MidiActionManager::*action_f)(MidiAction * , H2Core::Hydrogen * , int );
		map<string, pair<action_f, int> > actionMap;

		bool play(MidiAction * , H2Core::Hydrogen * , int );
		bool play_stop_pause_toggle(MidiAction * , H2Core::Hydrogen * , int );
		bool stop(MidiAction * , H2Core::Hydrogen * , int );
		bool pause(MidiAction * , H2Core::Hydrogen * , int );
		bool record_ready(MidiAction * , H2Core::Hydrogen * , int );
		bool record_strobe_toggle(MidiAction * , H2Core::Hydrogen * , int );
		bool record_strobe(MidiAction * , H2Core::Hydrogen * , int );
		bool record_exit(MidiAction * , H2Core::Hydrogen * , int );
		bool mute(MidiAction * , H2Core::Hydrogen * , int );
		bool unmute(MidiAction * , H2Core::Hydrogen * , int );
		bool mute_toggle(MidiAction * , H2Core::Hydrogen * , int );
		bool next_bar(MidiAction * , H2Core::Hydrogen * , int );
		bool previous_bar(MidiAction * , H2Core::Hydrogen * , int );
		bool bpm_increase(MidiAction * , H2Core::Hydrogen * , int );
		bool bpm_decrease(MidiAction * , H2Core::Hydrogen * , int );
		bool bpm_cc_relative(MidiAction * , H2Core::Hydrogen * , int );
		bool bpm_fine_cc_relative(MidiAction * , H2Core::Hydrogen * , int );
		bool master_volume_relative(MidiAction * , H2Core::Hydrogen * , int );
		bool master_volume_absolute(MidiAction * , H2Core::Hydrogen * , int );
		bool strip_volume_relative(MidiAction * , H2Core::Hydrogen * , int );
		bool strip_volume_absolute(MidiAction * , H2Core::Hydrogen * , int );
		bool effect_level_relative(MidiAction * , H2Core::Hydrogen * , int );
		bool effect_level_absolute(MidiAction * , H2Core::Hydrogen * , int );
		bool select_next_pattern(MidiAction * , H2Core::Hydrogen * , int );
		bool select_next_pattern_cc_absolute(MidiAction * , H2Core::Hydrogen * , int );
		bool select_next_pattern_promptly(MidiAction * , H2Core::Hydrogen * , int );
		bool select_next_pattern_relative(MidiAction * , H2Core::Hydrogen * , int );
		bool select_and_play_pattern(MidiAction * , H2Core::Hydrogen * , int );
		bool pan_relative(MidiAction * , H2Core::Hydrogen * , int );
		bool pan_absolute(MidiAction * , H2Core::Hydrogen * , int );
		bool filter_cutoff_level_absolute(MidiAction * , H2Core::Hydrogen * , int );
		bool beatcounter(MidiAction * , H2Core::Hydrogen * , int );
		bool tap_tempo(MidiAction * , H2Core::Hydrogen * , int );
		bool playlist_song(MidiAction * , H2Core::Hydrogen * , int );
		bool playlist_next_song(MidiAction * , H2Core::Hydrogen * , int );
		bool playlist_previous_song(MidiAction * , H2Core::Hydrogen * , int );
		bool toggle_metronome(MidiAction * , H2Core::Hydrogen * , int );
		bool select_instrument(MidiAction * , H2Core::Hydrogen * , int );
		bool undo_action(MidiAction * , H2Core::Hydrogen * , int );
		bool redo_action(MidiAction * , H2Core::Hydrogen * , int );
		bool gain_level_absolute(MidiAction * , H2Core::Hydrogen * , int );
		bool pitch_level_absolute(MidiAction * , H2Core::Hydrogen * , int );

		QStringList eventList;

		int m_nLastBpmChangeCCParameter;

	public:
		bool handleAction( MidiAction * );

		static void create_instance();
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
