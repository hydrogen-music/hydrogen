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

#include <hydrogen/IO/MidiDriverOutput.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/midi_action.h>
#include <hydrogen/midi_map.h>

namespace H2Core
{

MidiDriverOutput::MidiDriverOutput( const char* class_name )
		: Object( class_name )
{
	//INFOLOG( "INIT" );

}


MidiDriverOutput::~MidiDriverOutput()
{
	//INFOLOG( "DESTROY" );
}

};
