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

#include <hydrogen/adsr.h>
#include "table/getTableValue.h"
#include "table/TableExponential.h"

namespace H2Core
{

inline static float linear_interpolation( float fVal_A, float fVal_B, double fVal )
{
	return fVal_A * ( 1 - fVal ) + fVal_B * fVal;
//	return fVal_A + fVal * ( fVal_B - fVal_A );
//	return fVal_A + ((fVal_B - fVal_A) * fVal);
}


ADSR::ADSR(
    float attack,
    float decay,
    float sustain,
    float release
)
		: Object( "ADSR" )
		, __attack( attack )
		, __decay( decay )
		, __sustain( sustain )
		, __release( release )
		, __state( ATTACK )
		, __ticks( 0.0 )
		, __value( 0.0 )
{
	//INFOLOG( "INIT" );
}



ADSR::ADSR( const ADSR& orig )
		: Object( "ADSR" )
		, __attack( orig.__attack )
		, __decay( orig.__decay )
		, __sustain( orig.__sustain )
		, __release( orig.__release )
		, __state( orig.__state )
		, __ticks( orig.__ticks )
		, __value( orig.__value )
{
	//INFOLOG( "INIT - copy ctr" );
}



ADSR::~ADSR()
{
	//INFOLOG( "DESTROY" );
}


float ADSR::get_value( float step )
{
	switch ( __state ) {
	case ATTACK:
		if ( __attack == 0 ) {
			__value = 1.0;
		} else {
			__value = getTableValue ( linear_interpolation( 0.0, 1.0, ( __ticks * 1.0 / __attack ) ), tableExponentConvex, tableExponentConvexSize );
		}

		__ticks += step;
		if ( __ticks > __attack ) {
			__state = DECAY;
			__ticks = 0;
		}
		break;

	case DECAY:
		if ( __decay == 0 ) {
			__value = __sustain;
		} else {
			__value = getTableValue ( linear_interpolation( 1.0, __sustain, ( __ticks * 1.0 / __decay ) ), tableExponentConcave, tableExponentConcaveSize );
		}

		__ticks += step;
		if ( __ticks > __decay ) {
			__state = SUSTAIN;
			__ticks = 0;
		}
		break;

	case SUSTAIN:
		__value = __sustain;
		break;

	case RELEASE:
		if ( __release < 256 ) {
			__release = 256;
		}
		__value = getTableValue ( linear_interpolation( __release_value, 0.0, ( __ticks * 1.0 / __release ) ), tableExponentConcave, tableExponentConcaveSize );

		__ticks += step;
		if ( __ticks > __release ) {
			__state = IDLE;
			__ticks = 0;
		}
		break;

	case IDLE:
	default:
		__value = 0;
	};

	return __value;
}


///
/// Retuns the current value. Returns 0 if the note is ended.
///
float ADSR::release()
{
	if ( __state == IDLE ) {
		return 0;
	}

	if ( __state != RELEASE ) {
		__release_value = __value;
		__state = RELEASE;
		__ticks = 0;
		return __release_value;
	}

	return 1;
}

};

