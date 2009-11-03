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
#ifndef MIDIMAP_H
#define MIDIMAP_H


#include <map>
#include <cassert>
#include <hydrogen/Object.h>

#include <QtCore/QMutex>

class Action;

class MidiMap : public Object
{
	public:
		typedef std::map< QString, Action* > map_t;
		static MidiMap* __instance;
		~MidiMap();

		static void create_instance();
		static void reset_instance();  // convenience accessor to reset()
		static MidiMap* get_instance() { assert(__instance); return __instance; }

		void reset();  // Reinitializes the object.

		void registerMMCEvent( QString, Action* );
		void registerNoteEvent( int , Action* );
		void registerCCEvent( int , Action * );

		map_t getMMCMap();

		Action* getMMCAction( QString );
		Action* getNoteAction( int note );
		Action * getCCAction( int parameter );

		void setupNoteArray();

	private:
		MidiMap();

		Action* __note_array[ 128 ];
		Action* __cc_array[ 128 ];

		map_t mmcMap;
		QMutex __mutex;
};
#endif
