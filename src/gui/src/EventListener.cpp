/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "EventListener.h"

#include "HydrogenApp.h"

#include <core/IPC/IpcEngineAccess.h>
#include <core/Hydrogen.h>

void EventListener::blacklistEventId( long nEventId )
{
	if ( m_blacklistedEventIds.find( nEventId ) ==
		 m_blacklistedEventIds.end() ) {
		m_blacklistedEventIds.insert( nEventId );
		// In case we are connected with an authoritative engine via IPC, we
		// have to blacklist an event twice. First, for the local
		// CoreActionController of the mirror engine. This one is present for
		// "instant" feedback. And, second, for the remote one which will fire
		// later and whose event will be propagated back to the GUI via IPC.
		if ( HydrogenApp::pHydrogen()->getProcessMode() ==
			 H2Core::Hydrogen::ProcessMode::Editor ) {
			m_blacklistedEventIds.insert( nEventId );
		}
	}
}
