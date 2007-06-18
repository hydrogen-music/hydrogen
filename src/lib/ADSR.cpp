/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include "ADSR.h"
#include "synth/Curve.h"

inline static float linear_interpolation( float fVal_A, float fVal_B, float fVal)
{
	return fVal_A * ( 1 - fVal ) + fVal_B * fVal;
//	return fVal_A + fVal * ( fVal_B - fVal_A );
//	return fVal_A + ((fVal_B - fVal_A) * fVal);
}


ADSR::ADSR(
		float fAttack,
		float fDecay,
		float fSustain,
		float fRelease
)
 : m_fAttack( fAttack )
 , m_fDecay( fDecay )
 , m_fSustain( fSustain )
 , m_fRelease( fRelease )
 , m_state( ATTACK )
 , m_fTicks( 0.0 )
 , m_fValue( 0.0 )
{
}

ADSR::ADSR( const ADSR& orig )
 : m_fAttack( orig.m_fAttack )
 , m_fDecay( orig.m_fDecay )
 , m_fSustain( orig.m_fSustain )
 , m_fRelease( orig.m_fRelease )
 , m_state( orig.m_state )
 , m_fTicks( orig.m_fTicks )
 , m_fValue( orig.m_fValue )
{
}

float ADSR::getValue( float fStep )
{
	switch ( m_state ) {
		case ATTACK:
			if ( m_fAttack == 0 ) {
				m_fValue = 1.0;
			}
			else {
				m_fValue = linear2exponential ( linear_interpolation( 0.0, 1.0, ( m_fTicks * 1.0 / m_fAttack ) ), 1 );
			}

			m_fTicks += fStep;
			if ( m_fTicks > m_fAttack ) {
				m_state = DECAY;
				m_fTicks = 0;
			}
			break;

		case DECAY:
			if ( m_fDecay == 0) {
				m_fValue = m_fSustain;
			}
			else {
				m_fValue = linear2exponential ( linear_interpolation( 1.0, m_fSustain, ( m_fTicks * 1.0 / m_fDecay ) ), 2 );
			}

			m_fTicks += fStep;
			if ( m_fTicks > m_fDecay ) {
				m_state = SUSTAIN;
				m_fTicks = 0;
			}
			break;

		case SUSTAIN:
			m_fValue = m_fSustain;
			break;

		case RELEASE:
			if ( m_fRelease == 0) {
				m_fValue = 0.0;
			}
			else {
				m_fValue = linear2exponential ( linear_interpolation( m_fReleaseValue, 0.0, ( m_fTicks * 1.0 / m_fRelease ) ), 2 );
			}

			m_fTicks += fStep;
			if ( m_fTicks > m_fRelease ) {
				m_state = IDLE;
				m_fTicks = 0;
			}
			break;

		case IDLE:
		default:
			m_fValue = 0;
	};

	return m_fValue;
}


/// Restituisce il valore corrente, se e' zero significa che la nota e' finita
float ADSR::release()
{
	if ( m_state == IDLE ) {
		return 0;
	}

	if ( m_state != RELEASE ) {
		m_fReleaseValue = m_fValue;
		m_state = RELEASE;
		return m_fReleaseValue;
	}

	return 1;
}

