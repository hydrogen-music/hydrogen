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

#include <hydrogen/IO/MidiOutput.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/instrument.h>
#include <hydrogen/note.h>
#include <hydrogen/action.h>
#include <hydrogen/midiMap.h>

namespace H2Core
{

MidiOutput::MidiOutput( const QString class_name )
		: Object( class_name )
{
	//INFOLOG( "INIT" );
	
}


MidiOutput::~MidiOutput()
{
	//INFOLOG( "DESTROY" );
}

/*
void MidiOutput::queueMidiMessage( const MidiMessage& msg )
{
	m_pendingMessages.push_back(msg);
}

bool MidiOutput::hasQueuedMessages()
{
	return !m_pendingMessages.empty();
}
	
void MidiOutput::processQueuedMessages()
{
	/// Do something...
}
*/
};
