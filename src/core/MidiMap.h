/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#ifndef MIDIMAP_H
#define MIDIMAP_H


#include <map>
#include <cassert>
#include <core/Object.h>

#include <QtCore/QMutex>

class Action;

class MidiMap : public H2Core::Object
{
	H2_OBJECT
	public:
		typedef std::map< QString, Action* > map_t;
		/**
		 * Object holding the current MidiMap singleton. It is
		 * initialized with NULL, set with create_instance(),
		 * and accessed with get_instance().
		 */
		static MidiMap* __instance;
		~MidiMap();
		
		/**
		 * If #__instance equals 0, a new MidiMap singleton will
		 * be created and stored in it.
		 *
		 * It is called in Hydrogen::create_instance().
		 */
		static void create_instance();
		/**
		 * Convenience function calling reset() on the current
		 * MidiMap #__instance.
		 */
		static void reset_instance();
		/**
		 * Returns a pointer to the current MidiMap singleton
		 * stored in #__instance.
		 */
		static MidiMap* get_instance() { assert(__instance); return __instance; }

		void reset();  ///< Reinitializes the object.

		void registerMMCEvent( QString, Action* );
		void registerNoteEvent( int , Action* );
		void registerCCEvent( int , Action* );
		void registerPCEvent( Action* );

		map_t getMMCMap();

		Action* getMMCAction( QString );
		Action* getNoteAction( int note );
		Action* getCCAction( int parameter );
		Action* getPCAction();
		
		int findCCValueByActionParam1( QString actionType, QString param1 ) const;
		int findCCValueByActionType( QString actionType ) const;

		void setupNoteArray();
	private:
		MidiMap();

		Action* __note_array[ 128 ];
		Action* __cc_array[ 128 ];
		Action* __pc_action;

		map_t mmcMap;
		QMutex __mutex;
};
#endif
