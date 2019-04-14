/*
 * Hydrogen
 * Copyright(c) 2017 by Sebastian Moors
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

#ifndef CORE_ACTION_CONTROLLER_H
#define CORE_ACTION_CONTROLLER_H

#include <hydrogen/object.h>

namespace H2Core
{

class CoreActionController : public H2Core::Object {
	H2_OBJECT
	
	public:
		CoreActionController();
		~CoreActionController();
	
		void setMasterVolume( float masterVolumeValue );
		void setStripVolume( int nStrip, float masterVolumeValue );
		void setStripPan( int nStrip, float panValue );
		void setMetronomeIsActive( bool isActive );
		void setMasterIsMuted( bool isMuted );
		void setStripIsMuted( int nStrip, bool isMuted );
		void setStripIsSoloed( int nStrip, bool isSoloed );
		
		void initExternalControlInterfaces();
		void handleOutgoingControlChange( int param, int value);
		
	private:
		
		const int m_nDefaultMidiFeedbackChannel;
};

}
#endif
