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


#ifndef MIDILEARNABLE_H
#define MIDILEARNABLE_H

#include <memory>
#include <core/MidiAction.h>



/*
  Every widget which supports MidiLearn should derive from this Class.
*/

/** \ingroup docGUI docWidgets docMIDI*/
class MidiLearnable
{
public:
    MidiLearnable(){
		m_pAction = nullptr;
	}

    void setAction( std::shared_ptr<Action> pAction ){
		m_pAction = pAction;
    }

    std::shared_ptr<Action> getAction() const {
		return m_pAction;
    }


protected:
    std::shared_ptr<Action> m_pAction;
};

#endif // MIDILEARNABLE_H
