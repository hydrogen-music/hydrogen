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
#include <core/AudioEngine/TransportInfo.h>
#include <core/config.h>

namespace H2Core {

TransportInfo::TransportInfo( const char* __class_name )
	: Object( __class_name )
	, m_status( Status::Stopped )
	, m_nFrames( 0 )
	, m_nExternalFrames( 0 )
	, m_fTickSize( 1 )
	, m_fBpm( 120 ) {
}


TransportInfo::~TransportInfo() {
}

void TransportInfo::setBpm( float fNewBpm ) {
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
}
 
void TransportInfo::setFrames( long long nNewFrames ) {
	if ( nNewFrames < 0 ) {
		ERRORLOG( QString( "Provided frame [%1] is negative. Setting frame 0 instead." )
				  .arg( nNewFrames ) );
		nNewFrames = 0;
	}
	
	m_nFrames = nNewFrames;
}
	
void TransportInfo::setTickSize( float fNewTickSize ) {
	if ( fNewTickSize <= 0 ) {
		ERRORLOG( QString( "Provided tick size [%1] is too small. Using 400 as a fallback instead." )
				  .arg( fNewTickSize ) );
		fNewTickSize = 400;
	}

	m_fTickSize = fNewTickSize;
}
	 
void TransportInfo::setExternalFrames( long long nNewExternalFrames ) {
	m_nExternalFrames = nNewExternalFrames;
}

};
