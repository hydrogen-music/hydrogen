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

#include "AdsrTest.h"

#include <core/Basics/Adsr.h>
#include <stdio.h>
#include <memory>

using namespace H2Core;

const double delta = 0.00001;

void ADSRTest::setUp()
{
	m_adsr = std::make_shared<ADSR>( 1.0, 2.0, 0.8, 256.0 );
}

float ADSRTest::getValue( float fStep ) {
	float fL = 1.0, fR = 1.0;
	m_adsr->applyADSR( &fL, &fR, 1, 2, fStep );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( fL, fR, delta );
	return fL;
}

/* Check data is monotonically increasing and convex (derivative decreasing) */
static void checkConvex( float *pfData, int nFrames )
{
	// Check data is monotonically increasing
	for ( int n = 1; n < nFrames; n++ ) {
		CPPUNIT_ASSERT_MESSAGE( "monotonically increasing", pfData[n-1] <= pfData[n] );
	}
	// Verify convexity
	for ( int n = 2; n < nFrames; n++ ) {
		float fD1 = pfData[n-1] - pfData[n-2],
			fD2 = pfData[n] - pfData[n];
		CPPUNIT_ASSERT_MESSAGE( "convexity", fD2 <= fD1 );
	}
}

/* Check data is monotonically decreasing and concave (derivative increasing) */
static void checkConcave( float *pfData, int nFrames )
{
	// Check data is monotonically decreasing
	for ( int n = 1; n < nFrames; n++ ) {
		CPPUNIT_ASSERT_MESSAGE( "monotonically decreasing", pfData[n-1] >= pfData[n] );
	}
	// Verify concavity
	for ( int n = 2; n < nFrames; n++ ) {
		float fD1 = pfData[n-1] - pfData[n-2],
			fD2 = pfData[n] - pfData[n];
		CPPUNIT_ASSERT_MESSAGE( "concavity", fD2 >= fD1 );
	}
}

static void checkEqual( float *pfA, float *pfB, int nFrames ) {
	for ( int n = 0; n < nFrames; n++ ) {
		CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE( QString( "values at index %1" ).arg( n ).toStdString(),
											 pfA[n], pfB[n], delta );
	}
}

static void checkAllEqual( float *pfA, float fValue, int nFrames ) {
	for ( int n = 0; n < nFrames; n++ ) {
		CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE( QString( "values at index %1" ).arg( n ).toStdString(),
											 fValue, pfA[n], delta );
	}
}


/* Test basic ADSR functionality: apply an ADSR envelope to DC data, and check the properties of each
   phase. */
void ADSRTest::testBasicADSR() {
	___INFOLOG( "" );
	const int N = 256;
	const float fSustain = 0.75;
	float a[5*N], b[5*N];
	for ( int n = 0; n < 5*N; n++) {
		a[n] = b[n] = 1.0;
	}
	/* 5 phases with N samples each (attack, decay, sustain, release, idle) */
	ADSR Adsr( N, N, fSustain, N );
	Adsr.applyADSR( a, b, 5 * N, 3 * N, 1.0  );

	/* Expect A and B to be identical. Verify this, so we can check only one for the rest of the checks. */
	checkEqual( a, b, 5 * N );

	/* Attack: 0..N convex, starting at 0.0 and ending at 1.0 */
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE( "attack starting at 0", 0.0, a[0], 1.0/N );
	checkConvex( a, N );
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE( "attack ending at 1", 1.0, a[N-1], 1.0/N );

	/* Decay: N..2N-1 concave, starting at 1.0 and decaying to fSustain */
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE( "decay starting at 1", 1.0, a[N], 1.0/N );
	checkConcave( &a[N], N );
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE( "decay ending at sustain level", fSustain, a[2*N-1], 1.0/N );

	/* Sustain: 2N..3N-1 all at the same level. */
	for ( int n = 2*N; n < 3*N -1; n++ ) {
		CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE( "sustain", fSustain, a[n], delta );
	}

	/* Release: 3N..4N-1 from fSustain to 0.0 */
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE( "release starting at sustain level", fSustain, a[3*N], 1.0/N );
	checkConcave( &a[3*N], N );
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE( "release ending at 0", 0.0, a[4*N-1], 1.0/N );

	/* Idle */
	for ( int n = 4*N; n < 5*N - 1; n++ ) {
		CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE( "idle", 0.0, a[n], delta );
	}
	___INFOLOG( "passed" );
}


/* Test that we get an equivalent envelope when it's computed in chunks rather than all at once. */
void ADSRTest::testBufferChunks() {
	___INFOLOG( "" );
	const int N = 256;
	const int nChunk = 16;
	const float fSustain = 0.75;
	float a[5*N], b[5*N];
	float c[5*N], d[5*N];

	/* Test sanity check */
	assert( N % nChunk == 0 );

	for ( int nReleasePoint = 0.5 * N; nReleasePoint < 5 * N; nReleasePoint += N/2 ) {

		for ( int n = 0; n < 5*N; n++) {
			a[n] = b[n] = c[n] = d[n] = 1.0;
		}

		/* Reference: single-pass */
		ADSR AdsrRef( N, N, fSustain, N ), AdsrTest( N, N, fSustain, N );
		AdsrRef.applyADSR( a, b, 5 * N, nReleasePoint, 1.0 );

		for ( int n = 0; n < 5 * N; n += nChunk) {
			AdsrTest.applyADSR( c + n, d + n, nChunk, nReleasePoint - n, 1.0 );
			checkEqual( a + n, c + n, nChunk );
		}
	}
	___INFOLOG( "passed" );

}


void ADSRTest::testEarlyRelease() {
	___INFOLOG( "" );
	const int N = 256;
	const float fSustain = 0.75;
	float a[5*N], b[5*N];

	ADSR Adsr( N, N, fSustain, N );

	/* Release in the middle of the Decay phase */

	for ( int n = 0; n < 5*N; n++) {
		a[n] = b[n] = 1.0;
	}
	int nDecayStart = N,
		nReleaseStart = 1.5 * N,
		nReleaseEnd = 2.5 * N;

	Adsr.applyADSR( a, b, 5 * N, nReleaseStart, 1.0 );

	/* Expect A and B to be identical. Verify this, so we can check only one for the rest of the checks. */
	checkEqual( a, b, 5 * N );

	/* Attack: 0..N convex, starting at 0.0 and ending at 1.0 */
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE( "attack starting at 0", 0.0, a[0], 1.0/N );
	checkConvex( a, N );
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE( "attack ending at 1", 1.0, a[N-1], 1.0/N );

	/* Decay: N..1.5*N concave, starting at 1.0 and decaying to fSustain */
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE( "decay starting at 1", 1.0, a[N], 1.0/N );
	checkConcave( a + nDecayStart, nReleaseStart - nDecayStart );
	/* Concave release */
	checkConcave( a + nReleaseStart, N );
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE( "release ending at 0", 0.0, a[ nReleaseEnd ], 1.0/N );

	/* Idle */
	checkAllEqual( a + nReleaseEnd, 0.0, 5 * N - nReleaseEnd );


	/* Release in the middle of the Attack phase */

	for ( int n = 0; n < 5*N; n++) {
		a[n] = b[n] = 1.0;
	}

	nReleaseStart = 0.5 * N;
	nReleaseEnd = 1.5 * N;

	Adsr.applyADSR( a, b, 5 * N, nReleaseStart, 1.0 );

	/* Expect A and B to be identical. Verify this, so we can check only one for the rest of the checks. */
	checkEqual( a, b, 5 * N );

	/* Attack: 0..N/2 convex, starting at 0.0 */
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE( "attack starting at 0", 0.0, a[0], 1.0/N );
	checkConvex( a, nReleaseStart );

	/* Release */
	checkConcave( a + nReleaseStart, N );
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE( "release ending at 0", 0.0, a[ nReleaseEnd ], 1.0/N );

	/* Idle */
	checkAllEqual( a + nReleaseEnd, 0.0, 5 * N - nReleaseEnd );
	___INFOLOG( "passed" );

}

void ADSRTest::testAttack()
{
	___INFOLOG( "" );
	m_adsr->attack();

	/* Attack */
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, getValue( 0.5 ), delta );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.83576488494873, getValue( 0.5 ), delta );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 1.0, getValue( 0.1 ), delta );

	/* Decay */
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.969884753227234, getValue( 1.0 ), delta );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.82855612039566, getValue( 1.0 ), delta );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.8, getValue( 1.0 ), delta );

	/* Sustain */
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.8, getValue( 4.0 ), delta );
	___INFOLOG( "passed" );
}


void ADSRTest::testRelease()
{
	___INFOLOG( "" );
	getValue( 1.1 ); // move past Attack
	getValue( 2.1 ); // move past Decay
	getValue( 0.1 ); // calculate and store sustain

	/* Release note, and check if it was on sustain value */
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.8, m_adsr->release(), delta );

	/* Release */
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.8, getValue( 128.0 ), delta );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.139720246195793, getValue( 128.0 ), delta );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, getValue( 128.0 ), delta );

	/* Idle */
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, getValue( 2.0 ), delta );
	___INFOLOG( "passed" );
}
