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

#include <hydrogen/sequencer/Sequencer.h>

namespace H2Core
{


Sequencer* Sequencer::m_pInstance = NULL;



Sequencer* Sequencer::getInstance()
{
	if ( !m_pInstance ) {
		m_pInstance = new Sequencer();
	}
	return m_pInstance;
}



Sequencer::Sequencer()
		: Object( "Sequencer" )
{
	INFOLOG( "INIT" );
}



Sequencer::~Sequencer()
{
	INFOLOG( "DESTROY" );
}



void Sequencer::start()
{
	ERRORLOG( "not implemented yet" );
}



void Sequencer::stop()
{
	ERRORLOG( "not implemented yet" );
}


}
