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
#ifndef EVENT_LISTENER
#define EVENT_LISTENER

#include <hydrogen/globals.h>

class EventListener
{
	public:
		virtual void stateChangedEvent(int nState) { UNUSED( nState ); }
		virtual void patternChangedEvent() {}
		virtual void patternModifiedEvent() {}
		virtual void selectedPatternChangedEvent() {}
		virtual void selectedInstrumentChangedEvent() {}
		virtual void midiActivityEvent() {}
		virtual void noteOnEvent( int nInstrument ) { UNUSED( nInstrument ); }
		virtual void XRunEvent() {}
		virtual void errorEvent( int nErrorCode ) { UNUSED( nErrorCode ); }
		virtual void metronomeEvent( int nValue ) { UNUSED( nValue ); }
		virtual void rubberbandbpmchangeEvent() {}
		virtual void progressEvent( int nValue ) { UNUSED( nValue ); }

		virtual ~EventListener() {}
};


#endif

