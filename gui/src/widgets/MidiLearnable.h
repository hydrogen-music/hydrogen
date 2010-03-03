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


#ifndef MIDILEARNABLE_H
#define MIDILEARNABLE_H

#include <hydrogen/action.h>



/*
  Every widget which supports MidiLearn should derive from this Class.
*/

class MidiLearnable
{
public:
    MidiLearnable(){
	m_action = NULL;
    }

    ~MidiLearnable(){
	if( m_action != NULL) delete m_action;
    }

    void setAction( Action *action ){
	m_action = action;
    }

    Action* getAction(){
	return m_action;
    }


private:
    Action *m_action;
};

#endif // MIDILEARNABLE_H
