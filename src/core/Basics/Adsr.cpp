/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2022 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

/* Attack parameters */
const float fAttackExponent = 0.038515241777294117,
	fAttackInit = 1.039835771720117430;

const float fDecayExponent = 0.044796211247505179,
	fDecayInit = 1.046934808452493870,
	fDecayYOffset = -0.046934663351557632;


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
	__release_value( 0.0 ),
	m_fQ( fAttackInit )
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
		m_fQ = fAttackInit;
	};

	return __value;
}


/**
 * Apply an exponential envelope to a stereo pair of sample fragments.
 *
 * The exponential is generalised by parameters:
 *   - fXOffset -- x offset
 *   - fYOffset -- y offset
 *   - fExponent -- base of the exponential
 *   - fScale -- scale the curve (including reflection)
 *
 * These parameters allow suitable curves for attack, decay and release to be formed.
 *
 * Because some parameters will take on trivial valuesbdepending on use, its desitable to inline this to allow
 * constant propagation to remove redundant operations.
 *
 * The exponential loop isn't naturally vectorisable since there is a loop carried dependency for the
 * exponential variable. However, we can manually unroll the loop and replace the single Q with multiple
 * copies, which allows the SLP vectoriser to piece together a nice loop. On AArch64 we get loops of ~11
 * instructions for 4 frames. 
 *
 * Even if the code is not SLP-vectorised, the unrolled loop should still have better performance
 * characteristics due to more flexible scheduling and reduced loop overhead.
 *
 */
inline double applyExponential( const float fExponent, const float fXOffset, const float fYOffset,
								const float fScale,
								float * __restrict__ pA, float * __restrict__ pB,
								float fQ, int nFrames, int nFramesTotal, float fStep,
								float * __restrict__ pfADSRVal ) {

	int i = 0;
	float fVal = *pfADSRVal;

	float fFactor = pow( fExponent, (double)fStep / nFramesTotal );

	if ( nFrames > 4) {
		float fFactor4 = fFactor * fFactor * fFactor * fFactor;
		float fQ0 = fQ,
			fQ1 = fQ0 * fFactor,
			fQ2 = fQ1 * fFactor,
			fQ3 = fQ2 * fFactor;
		for (; i < nFrames - 4; i += 4) {
			float fVal0 = ( fQ0 - fXOffset ) * fScale + fYOffset,
				fVal1 = ( fQ1 - fXOffset ) * fScale + fYOffset,
				fVal2 = ( fQ2 - fXOffset ) * fScale + fYOffset,
				fVal3 = ( fQ3 - fXOffset ) * fScale + fYOffset;

			pA[i] *= fVal0;
			pA[i+1] *= fVal1;
			pA[i+2] *= fVal2;
			pA[i+3] *= fVal3;

			pB[i] *= fVal0;
			pB[i+1] *= fVal1;
			pB[i+2] *= fVal2;
			pB[i+3] *= fVal3;

			fQ0 *= fFactor4;
			fQ1 *= fFactor4;
		    fQ2 *= fFactor4;
			fQ3 *= fFactor4;

			fVal = fVal0;
		}
		fQ = fQ0;
	}

	for (; i < nFrames; i++) {
		fVal = ( fQ - fXOffset ) * fScale + fYOffset;
		pA[i] *= fVal;
		pB[i] *= fVal;
		fQ *= fFactor;
	}
	*pfADSRVal = fVal;
	return fQ;
}

/**
 * Apply ADSR envelope to stereo pair sample fragments.
 * 
 * This function manages the current state of the ADSR state machine, and applies envelope calculations
 * appropriate to each phase.
 */
bool ADSR::applyADSR( float *pLeft, float *pRight, int nFrames, int nReleaseFrame, float fStep )
{
	int n = 0;

	if ( __state == ATTACK ) {
		int nAttackFrames = std::min( nFrames, nReleaseFrame );
		if ( nAttackFrames * fStep > __attack ) {
			// Attack must end before nFrames, so trim it
			nAttackFrames = ceil( __attack / fStep );
		}

		m_fQ =  applyExponential( fAttackExponent, fAttackInit, 0.0, -1.0,
								  pLeft, pRight, m_fQ, nAttackFrames, __attack, fStep, &__value );

		n += nAttackFrames;

		__ticks += nAttackFrames * fStep;

		if ( __ticks >= __attack ) {
			__ticks = 0;
			__state = DECAY;
			m_fQ = fDecayInit;
		}
	}

	if ( __state == DECAY ) {
		int nDecayFrames = std::min( nFrames, nReleaseFrame ) - n;
		if ( nDecayFrames * fStep > __decay ) {
			nDecayFrames = ceil( __decay / fStep );
		}

		m_fQ = applyExponential( fDecayExponent, -fDecayYOffset, __sustain, (1.0-__sustain),
								 &pLeft[n], &pRight[n], m_fQ, nDecayFrames, __decay, fStep, &__value );

		n += nDecayFrames;
		__ticks += nDecayFrames * fStep;

		if ( __ticks >= __decay ) {
			__ticks = 0;
			__state = SUSTAIN;
		}
	}

	if ( __state == SUSTAIN ) {

		int nSustainFrames = std::min( nFrames, nReleaseFrame ) - n;
		if ( nSustainFrames != 0 ) {
			__value = __sustain;
			if ( __sustain != 1.0 ) {
				for ( int i = 0; i < nSustainFrames; i++ ) {
					pLeft[ n + i ] *= __sustain;
					pRight[ n + i ] *= __sustain;
				}
			}
			n += nSustainFrames;
		}
	}

	if ( __state != RELEASE && __state != IDLE && n >= nReleaseFrame ) {
		__release_value = __value;
		__state = RELEASE;
		__ticks = 0;
		m_fQ = fDecayInit;
	}

	if ( __state == RELEASE ) {

		int nReleaseFrames = nFrames - n;
		if ( nReleaseFrames * fStep > __decay ) {
			nReleaseFrames = ceil( __decay / fStep );
		}

		m_fQ = applyExponential( fDecayExponent, -fDecayYOffset, 0.0, __release_value,
								 &pLeft[n], &pRight[n], m_fQ, nReleaseFrames, __release, fStep, &__value );

		n += nReleaseFrames;
		__ticks += nReleaseFrames * fStep;
		if ( __ticks >= __release ) {
			__state = IDLE;
		}
	}

	if ( __state == IDLE ) {
		for ( ; n < nFrames; n++ ) {
			pLeft[ n ] = pRight[ n ] = 0.0;
		}
		return true;
	}
	return false;
}

void ADSR::attack()
{
	__state = ATTACK;
	__ticks = 0;
	m_fQ = fAttackInit;
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
