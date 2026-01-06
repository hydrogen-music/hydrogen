/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include <core/Basics/Adsr.h>

namespace H2Core
{

/* Attack parameters */
const float fAttackExponent = 0.038515241777294117,
	fAttackInit = 1.039835771720117430;

const float fDecayExponent = 0.044796211247505179,
	fDecayInit = 1.046934808452493870,
	fDecayYOffset = -0.046934663351557632;

ADSR::ADSR( unsigned int attack, unsigned int decay, float sustain, unsigned int release ) :
	m_nAttack( attack ),
	m_nDecay( decay ),
	m_fSustain( sustain ),
	m_nRelease( release ),
	m_state( State::Attack ),
	m_fFramesInState( 0.0 ),
	m_fValue( 0.0 ),
	m_fReleaseValue( 0.0 ),
	m_fQ( fAttackInit )
{
	normalise();
}

ADSR::ADSR( const std::shared_ptr<ADSR> other ) :
	m_nAttack( other->m_nAttack ),
	m_nDecay( other->m_nDecay ),
	m_fSustain( other->m_fSustain ),
	m_nRelease( other->m_nRelease ),
	m_state( other->m_state ),
	m_fFramesInState( other->m_fFramesInState ),
	m_fValue( other->m_fValue ),
	m_fReleaseValue( other->m_fReleaseValue )
{
	normalise();
}

ADSR::~ADSR() { }

void ADSR::normalise()
{
	if (m_nAttack < 0.0) {
		m_nAttack = 0.0;
	}
	if (m_nDecay < 0.0) {
		m_nDecay = 0.0;
	}
	if (m_fSustain < 0.0) {
		m_fSustain = 0.0;
	}
	if (m_nRelease < 256) {
		m_nRelease = 256;
	}
	if (m_nAttack > 100000) {
		m_nAttack = 100000;
	}
	if (m_nDecay > 100000) {
		m_nDecay = 100000;
	}
	if (m_fSustain > 1.0) {
		m_fSustain = 1.0;
	}
	if (m_nRelease > 100256) {
		m_nRelease = 100256;
	}
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
 * Because some parameters will take on trivial values depending on use, it's desirable to inline this
 * to allow constant propagation to remove redundant operations.
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
bool ADSR::applyADSR( float *pLeft, float *pRight, int nFinalBufferPos, int nReleaseFrame, float fStep )
{
	int nBufferPos = 0;

	// If the release point is somehow in the past, move direcly to Release
	if ( nReleaseFrame <= 0 && m_state != State::Release && m_state != State::Idle ) {
		WARNINGLOG( QString( "Impossibly early release for ADSR: [%1]" )
						.arg( this->toQString() ) );
		nReleaseFrame = 0;
		m_state = State::Release;
	}

	if ( m_state == State::Attack ) {
		int nAttackFrames = std::min( nFinalBufferPos, nReleaseFrame );
		if ( nAttackFrames * fStep > m_nAttack ) {
			// Attack must end before nFinalBufferPos, so trim it
			nAttackFrames = ceil( m_nAttack / fStep );
		}

		m_fQ = applyExponential( fAttackExponent, fAttackInit, 0.0, -1.0,
								  pLeft, pRight, m_fQ, nAttackFrames, m_nAttack,
								 fStep, &m_fValue );

		nBufferPos += nAttackFrames;

		m_fFramesInState += nAttackFrames * fStep;

		if ( m_fFramesInState >= m_nAttack ) {
			m_fFramesInState = 0;
			m_state = State::Decay;
			m_fQ = fDecayInit;
		}
	}

	if ( m_state == State::Decay ) {
		int nDecayFrames = std::min( nFinalBufferPos, nReleaseFrame ) - nBufferPos;
		if ( nDecayFrames * fStep > m_nDecay ) {
			nDecayFrames = ceil( m_nDecay / fStep );
		}

		m_fQ = applyExponential( fDecayExponent, -fDecayYOffset, m_fSustain, (1.0-m_fSustain),
								 &pLeft[nBufferPos], &pRight[nBufferPos], m_fQ, nDecayFrames, m_nDecay, fStep, &m_fValue );

		nBufferPos += nDecayFrames;
		m_fFramesInState += nDecayFrames * fStep;

		if ( m_fFramesInState >= m_nDecay ) {
			m_fFramesInState = 0;
			m_state = State::Sustain;
		}
	}

	if ( m_state == State::Sustain ) {

		int nSustainFrames = std::min( nFinalBufferPos, nReleaseFrame ) - nBufferPos;
		if ( nSustainFrames != 0 ) {
			m_fValue = m_fSustain;
			if ( m_fSustain != 1.0 ) {
				for ( int i = 0; i < nSustainFrames; i++ ) {
					pLeft[ nBufferPos + i ] *= m_fSustain;
					pRight[ nBufferPos + i ] *= m_fSustain;
				}
			}
			nBufferPos += nSustainFrames;
		}
	}

	if ( m_state != State::Release && m_state != State::Idle && nBufferPos >= nReleaseFrame ) {
		m_fReleaseValue = m_fValue;
		m_state = State::Release;
		m_fFramesInState = 0;
		m_fQ = fDecayInit;
	}

	if ( m_state == State::Release ) {

		int nReleaseFrames = nFinalBufferPos - nBufferPos;
		if ( nReleaseFrames * fStep > m_nRelease ) {
			nReleaseFrames = ceil( m_nRelease / fStep );
		}

		m_fQ = applyExponential( fDecayExponent, -fDecayYOffset, 0.0, m_fReleaseValue,
								 &pLeft[nBufferPos], &pRight[nBufferPos], m_fQ, nReleaseFrames, m_nRelease, fStep, &m_fValue );

		nBufferPos += nReleaseFrames;
		m_fFramesInState += nReleaseFrames * fStep;
		
		if ( m_fFramesInState >= m_nRelease ) {
			m_state = State::Idle;
		}
	}

	if ( m_state == State::Idle ) {
		for ( ; nBufferPos < nFinalBufferPos; nBufferPos++ ) {
			pLeft[ nBufferPos ] = pRight[ nBufferPos ] = 0.0;
		}
		return true;
	}
	return false;
}

void ADSR::attack()
{
	m_state = State::Attack;
	m_fFramesInState = 0;
	m_fQ = fAttackInit;
}

float ADSR::release()
{
	if ( m_state == State::Idle ) {
		return 0;
	} 
	else if ( m_state == State::Release ) {
		return m_fValue;
	}
	
	m_fReleaseValue = m_fValue;
	m_state = State::Release;
	m_fFramesInState = 0;
	m_fQ = fDecayInit;
	return m_fReleaseValue;
}

QString ADSR::StateToQString( const State& state ) {
	switch( state ) {
	case State::Attack:
		return std::move( "Attack" );
	case State::Decay:
		return std::move( "Decay" );
	case State::Sustain:
		return std::move( "Sustain" );
	case State::Release:
		return std::move( "Release" );
	case State::Idle:
		return std::move( "Idle" );
	}

	return std::move( "Attack" );
}

QString ADSR::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[ADSR]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_nAttack: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nAttack ) )
			.append( QString( "%1%2m_nDecay: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nDecay ) )
			.append( QString( "%1%2m_fSustain: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fSustain ) )
			.append( QString( "%1%2m_nRelease: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nRelease ) )
			.append( QString( "%1%2m_state: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( StateToQString( m_state ) ) )
			.append( QString( "%1%2m_fFramesInState: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fFramesInState ) )
			.append( QString( "%1%2m_fValue: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fValue ) )
			.append( QString( "%1%2m_fReleaseValue: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fReleaseValue ) )
			.append( QString( "%1%2m_fQ: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fQ ) );
	} else {
		sOutput = QString( "[ADSR]" )
			.append( QString( " m_nAttack: %1" ).arg( m_nAttack ) )
			.append( QString( ", m_nDecay: %1" ).arg( m_nDecay ) )
			.append( QString( ", m_fSustain: %1" ).arg( m_fSustain ) )
			.append( QString( ", m_nRelease: %1" ).arg( m_nRelease ) )
			.append( QString( ", m_state: %1" ).arg( StateToQString( m_state ) ) )
			.append( QString( ", m_fFramesInState: %1" ).arg( m_fFramesInState ) )
			.append( QString( ", m_fValue: %1" ).arg( m_fValue ) )
			.append( QString( ", m_fReleaseValue: %1\n" ).arg( m_fReleaseValue ) )
			.append( QString( ", m_fQ: %1\n" ).arg( m_fQ ) );
	}
	
	return sOutput;
}

};

/* vim: set softtabstop=4 noexpandtab: */
