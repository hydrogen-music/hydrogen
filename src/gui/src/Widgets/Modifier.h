/*
 * Hydrogen
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef MODIFIER_H
#define MODIFIER_H

#include <QString>

#include <core/Hydrogen.h>
#include <core/Object.h>

#include "../HydrogenApp.h"
#include "../PatternEditor/PatternEditorPanel.h"

/** Derived classes will have the capability to mark a corresponding artifact as
 * changed. */
class Modifier {
   public:
	// Not an enum class as we use in so many other spaces because of the
	// more concise calling.
	enum { None, Drumkit, Pattern, Song };

	void setModifierTarget( int nModifierTarget )
	{
		if ( nModifierTarget != Drumkit && nModifierTarget != Pattern &&
			 nModifierTarget != Song ) {
			___ERRORLOG(
				QString( "Invalid artifact [%1]" ).arg( nModifierTarget )
			);
			m_nModifierTarget = None;
		}
		else {
			m_nModifierTarget = nModifierTarget;
		}
	};

	void modify()
	{
		if ( m_nModifierTarget == Modifier::None ) {
			return;
		}

		auto pHydrogen = H2Core::Hydrogen::get_instance();
		switch ( m_nModifierTarget ) {
			case Drumkit:
				pHydrogen->setDrumkitModified( true );
				break;
			case Pattern:
				pHydrogen->setPatternModified(
					true, HydrogenApp::get_instance()
							  ->getPatternEditorPanel()
							  ->getPatternNumber()
				);
				break;
			case Song:
				pHydrogen->setSongModified( true );
				break;
			default:
				___ERRORLOG(
					QString( "Invalid artifact [%1]" ).arg( m_nModifierTarget )
				);
		}
	};

   protected:
	int m_nModifierTarget;
};

#endif
