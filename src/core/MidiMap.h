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

#include <memory>
#include <vector>
#include <map>
#include <cassert>
#include <core/Object.h>

#include <QtCore/QMutex>

class Action;

/** \ingroup docCore docMIDI */
class MidiMap : public H2Core::Object<MidiMap>
{
	H2_OBJECT(MidiMap)
	public:
	typedef std::map< QString, std::shared_ptr<Action>> map_t;
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

		void registerMMCEvent( QString, std::shared_ptr<Action> );
		void registerNoteEvent( int , std::shared_ptr<Action> );
		void registerCCEvent( int , std::shared_ptr<Action> );
		void registerPCEvent( std::shared_ptr<Action> );

		map_t getMMCMap();

		std::shared_ptr<Action> getMMCAction( QString );
		std::shared_ptr<Action> getNoteAction( int note );
		std::shared_ptr<Action> getCCAction( int parameter );
		std::shared_ptr<Action> getPCAction();
		
		int findCCValueByActionParam1( QString actionType, QString param1 ) const;
		int findCCValueByActionType( QString actionType ) const;

		void setupNoteArray();
	private:
		MidiMap();

	std::vector<std::shared_ptr<Action>> m_noteVector;
	std::vector<std::shared_ptr<Action>> m_ccVector;
		std::shared_ptr<Action> m_pPcAction;

		map_t mmcMap;
		QMutex __mutex;
};
#endif
