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

#ifndef OSC_SERVER_H
#define OSC_SERVER_H

#ifdef H2CORE_HAVE_OSC

#include <lo/lo.h>


#include <hydrogen/object.h>
#include <hydrogen/Preferences.h>
#include <cassert>


/**
* @class OscServer
*
* @brief Osc Server implementation
*
*
* @author Sebastian Moors
*
*/

namespace lo
{
	class ServerThread;
}

class OscServer : public H2Core::Object
{
	H2_OBJECT
	public:

		static OscServer* __instance;
		~OscServer();
	
		
		static void create_instance(H2Core::Preferences* pPreferences);
		static OscServer* get_instance() { assert(__instance); return __instance; }

		static QString qPrettyPrint(lo_type type,void * data);

		void start();

		static void PLAY_TOGGLE_Handler(lo_arg **argv, int i);
		static void PLAY_STOP_TOGGLE_Handler(lo_arg **argv, int i);
		static void PLAY_PAUSE_TOGGLE_Handler(lo_arg **argv, int i);
		static void STOP_Handler(lo_arg **argv, int i);
		static void PAUSE_Handler(lo_arg **argv, int i);
		static void RECORD_READY_Handler(lo_arg **argv, int i);
		static void RECORD_STROBE_TOGGLE_Handler(lo_arg **argv, int i);
		static void RECORD_STROBE_Handler(lo_arg **argv, int i);
		static void RECORD_EXIT_Handler(lo_arg **argv, int i);
		static void MUTE_Handler(lo_arg **argv, int i);
		static void UNMUTE_Handler(lo_arg **argv, int i);
		static void MUTE_TOGGLE_Handler(lo_arg **argv, int i);
		static void NEXT_BAR_Handler(lo_arg **argv, int i);
		static void PREVIOUS_BAR_Handler(lo_arg **argv, int i);
		static void BPM_INCR_Handler(lo_arg **argv, int i);
		static void BPM_DECR_Handler(lo_arg **argv, int i);
		static void BPM_CC_RELATIVE_Handler(lo_arg **argv, int i);
		static void BPM_FINE_CC_RELATIVE_Handler(lo_arg **argv, int i);
		static void MASTER_VOLUME_RELATIVE_Handler(lo_arg **argv, int i);
		static void MASTER_VOLUME_ABSOLUTE_Handler(lo_arg **argv, int i);
		static void STRIP_VOLUME_RELATIVE_Handler(lo_arg **argv, int i);
		static void STRIP_VOLUME_ABSOLUTE_Handler(QString param1, QString param2);
		static void SELECT_NEXT_PATTERN_Handler(lo_arg **argv, int i);
		static void SELECT_NEXT_PATTERN_CC_ABSOLUTE_Handler(lo_arg **argv, int i);
		static void SELECT_NEXT_PATTERN_PROMPTLY_Handler(lo_arg **argv, int i);
		static void SELECT_NEXT_PATTERN_RELATIVE_Handler(lo_arg **argv, int i);
		static void SELECT_AND_PLAY_PATTERN_Handler(lo_arg **argv, int i);
		static void PAN_RELATIVE_Handler(QString param1, QString param2);
		static void PAN_ABSOLUTE_Handler(QString param1, QString param2);
		static void FILTER_CUTOFF_LEVEL_ABSOLUTE_Handler(QString param1, QString param2);
		static void BEATCOUNTER_Handler(lo_arg **argv, int i);
		static void TAP_TEMPO_Handler(lo_arg **argv, int i);
		static void PLAYLIST_SONG_Handler(lo_arg **argv, int i);
		static void PLAYLIST_NEXT_SONG_Handler(lo_arg **argv, int i);
		static void PLAYLIST_PREV_SONG_Handler(lo_arg **argv, int i);
		static void TOGGLE_METRONOME_Handler(lo_arg **argv, int i);
		static void SELECT_INSTRUMENT_Handler(lo_arg **argv, int i);
		static void UNDO_ACTION_Handler(lo_arg **argv, int i);
		static void REDO_ACTION_Handler(lo_arg **argv, int i);
		static int  generic_handler(const char *path, const char *types, lo_arg ** argv,
								int argc, void *data, void *user_data);

	private:
		OscServer(H2Core::Preferences* pPreferences);

		lo::ServerThread*		m_pServerThread;
		H2Core::Preferences*	m_pPreferences;
};

#endif /* H2CORE_HAVE_OSC */

#endif // OSC_SERVER_H
