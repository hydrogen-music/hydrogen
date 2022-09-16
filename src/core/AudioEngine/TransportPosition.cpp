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
#include <core/AudioEngine/TransportPosition.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/config.h>

namespace H2Core {

TransportPosition::TransportPosition()
	: m_nFrames( 0 )
	, m_fTick( 0 )
	, m_fTickSize( 400 )
	, m_fBpm( 120 ) {
}


TransportPosition::~TransportPosition() {
}

void TransportPosition::setBpm( float fNewBpm ) {
	if ( fNewBpm > MAX_BPM ) {
		ERRORLOG( QString( "Provided bpm [%1] is too high. Assigning upper bound %2 instead" )
					.arg( fNewBpm ).arg( MAX_BPM ) );
		fNewBpm = MAX_BPM;
	} else if ( fNewBpm < MIN_BPM ) {
		ERRORLOG( QString( "Provided bpm [%1] is too low. Assigning lower bound %2 instead" )
					.arg( fNewBpm ).arg( MIN_BPM ) );
		fNewBpm = MIN_BPM;
	}
	
	m_fBpm = fNewBpm;

	if ( Preferences::get_instance()->getRubberBandBatchMode() ) {
		Hydrogen::get_instance()->recalculateRubberband( getBpm() );
	}
}
 
void TransportPosition::setFrames( long long nNewFrames ) {
	if ( nNewFrames < 0 ) {
		ERRORLOG( QString( "Provided frame [%1] is negative. Setting frame 0 instead." )
				  .arg( nNewFrames ) );
		nNewFrames = 0;
	}
	
	m_nFrames = nNewFrames;
}

void TransportPosition::setTick( double fNewTick ) {
	if ( fNewTick < 0 ) {
		ERRORLOG( QString( "Provided tick [%1] is negative. Setting frame 0 instead." )
				  .arg( fNewTick ) );
		fNewTick = 0;
	}
	
	m_fTick = fNewTick;
}

void TransportPosition::setTickSize( float fNewTickSize ) {
	if ( fNewTickSize <= 0 ) {
		ERRORLOG( QString( "Provided tick size [%1] is too small. Using 400 as a fallback instead." )
				  .arg( fNewTickSize ) );
		fNewTickSize = 400;
	}

	m_fTickSize = fNewTickSize;
}	 

};
