/*

(c)2005 Artemiy Pavlov
http://sineshine.com

This function takes a precomputed table, finds the value nearest to inputValue,
and calculates an interpolated return value.

*/

float getTableValue( float fInputValue, float* pLookupTable, int nLookupTableSize )
{

	float fOutputValue;

	/* Determine table element position closest to what we need */

	int nPosition = ( int )( fInputValue * nLookupTableSize + 1 );

	if ( nPosition < 1 ) {
		nPosition = 1;
	}

	/* Make sure we do not run out of the table limits */

	if ( nPosition > nLookupTableSize ) {
		nPosition = nLookupTableSize;
	}

	/* Get the nearest value */
	float fNearestValue = pLookupTable[ nPosition - 1 ];

	/* Calculate and return the output value */

	fOutputValue = ( fNearestValue * fInputValue ) / ( ( float )nPosition / ( float )nLookupTableSize );

	return fOutputValue;
}

