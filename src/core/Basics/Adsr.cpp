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

#include <core/Basics/Adsr.h>

#include "ExponentialTables.h"

namespace H2Core
{


inline static float linear_interpolation( float fVal_A, float fVal_B, double fVal )
{
	return fVal_A * ( 1 - fVal ) + fVal_B * fVal;
	//return fVal_A + fVal * ( fVal_B - fVal_A );
	//return fVal_A + ((fVal_B - fVal_A) * fVal);
}

void ADSR::normalise()
{
	if (__attack < 0.0) {
		__attack = 0.0;
	}
	if (__decay < 0.0) {
		__decay = 0.0;
	}
	if (__sustain < 0.0) {
		__sustain = 0.0;
	}
	if (__release < 256) {
		__release = 256;
	}
	if (__attack > 100000) {
		__attack = 100000;
	}
	if (__decay > 100000) {
		__decay = 100000;
	}
	if (__sustain > 1.0) {
		__sustain = 1.0;
	}
	if (__release > 100256) {
		__release = 100256;
	}
}

ADSR::ADSR( unsigned int attack, unsigned int decay, float sustain, unsigned int release ) :
	__attack( attack ),
	__decay( decay ),
	__sustain( sustain ),
	__release( release ),
	__state( ATTACK ),
	__ticks( 0.0 ),
	__value( 0.0 ),
	__release_value( 0.0 )
{
	normalise();
}

ADSR::ADSR( const std::shared_ptr<ADSR> other ) :
	__attack( other->__attack ),
	__decay( other->__decay ),
	__sustain( other->__sustain ),
	__release( other->__release ),
	__state( other->__state ),
	__ticks( other->__ticks ),
	__value( other->__value ),
	__release_value( other->__release_value )
{
	normalise();
}

ADSR::~ADSR() { }

//#define convex_exponant
//#define concave_exponant

float ADSR::get_value( float step )
{
	switch ( __state ) {
	case ATTACK:
		if ( __attack == 0 ) {
			__value = 1.0;
		} else {
			__value = convex_exponant( linear_interpolation( 0.0, 1.0, ( __ticks * 1.0 / __attack ) ) );
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
			__value = concave_exponant( linear_interpolation( 1.0, 0.0, ( __ticks * 1.0 / __decay ) ) ) * (1 - __sustain) + __sustain;
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
		__value = concave_exponant( linear_interpolation( 1.0, 0.0, ( __ticks * 1.0 / __release ) ) ) * __release_value;
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

void ADSR::applyADSR( float *pLeft, float *pRight, int nFrames, float fStep )
{
	int n;
	for ( n = 0; n < nFrames; n++ ) {
		float fValue = get_value( fStep );
		pLeft[ n ] *= fValue;
		pRight[ n ] *= fValue;
	}
}

void ADSR::attack()
{
	__state = ATTACK;
	__ticks = 0;
}

float ADSR::release()
{
	if ( __state == IDLE ) return 0;
	if ( __state == RELEASE ) return __value;
	__release_value = __value;
	__state = RELEASE;
	__ticks = 0;
	return __release_value;
}

QString ADSR::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[ADSR]\n" ).arg( sPrefix )
			.append( QString( "%1%2attack: %3\n" ).arg( sPrefix ).arg( s ).arg( __attack ) )
			.append( QString( "%1%2decay: %3\n" ).arg( sPrefix ).arg( s ).arg( __decay ) )
			.append( QString( "%1%2sustain: %3\n" ).arg( sPrefix ).arg( s ).arg( __sustain ) )
			.append( QString( "%1%2release: %3\n" ).arg( sPrefix ).arg( s ).arg( __release ) )
			.append( QString( "%1%2state: %3\n" ).arg( sPrefix ).arg( s ).arg( __state ) )
			.append( QString( "%1%2ticks: %3\n" ).arg( sPrefix ).arg( s ).arg( __ticks ) )
			.append( QString( "%1%2value: %3\n" ).arg( sPrefix ).arg( s ).arg( __value ) )
			.append( QString( "%1%2release_value: %3\n" ).arg( sPrefix ).arg( s ).arg( __release_value ) );
	} else {
		sOutput = QString( "[ADSR]" )
			.append( QString( " attack: %1" ).arg( __attack ) )
			.append( QString( ", decay: %1" ).arg( __decay ) )
			.append( QString( ", sustain: %1" ).arg( __sustain ) )
			.append( QString( ", release: %1" ).arg( __release ) )
			.append( QString( ", state: %1" ).arg( __state ) )
			.append( QString( ", ticks: %1" ).arg( __ticks ) )
			.append( QString( ", value: %1" ).arg( __value ) )
			.append( QString( ", release_value: %1\n" ).arg( __release_value ) );
	}
	
	return sOutput;
}

};

/* vim: set softtabstop=4 noexpandtab: */
