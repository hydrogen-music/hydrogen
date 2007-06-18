#include "math.h"

/*

Wave shaper curve functions

(c)2005 Artemiy Pavlov
Sineshine sonic innovations - http://sineshine.com

This program is free software, it may be distributed under the terms
and conditions of the GNU General Public License version 2 or later.

*/

float waveshaper_sine( float signal , float curve ){

	signal = (0.5f * sin ( 2 * M_PI * curve * signal) + 0.5f) * signal;
	
	return signal;

}

float waveshaper_double_sine( float signal , float curve ){

	signal = (0.5f * sin ( 2 * M_PI * curve * signal) * sin ( 4 * M_PI * curve * signal) + 0.5f) * signal;
	
	return signal;

}

float waveshaper_triple_sine( float signal , float curve ){

	signal = (0.5f * sin ( 2 * M_PI * curve * signal) * sin ( 4 * M_PI * curve * signal) * sin ( 8 * M_PI * curve * signal) + 0.5f) * signal;
	
	return signal;

}

float waveshaper_quadruple_sine( float signal , float curve ){

	signal = (0.5f * sin ( 2 * M_PI * curve * signal) * sin ( 4 * M_PI * curve * signal) * sin ( 6 * M_PI * curve * signal) * sin ( 8 * M_PI * curve * signal) + 0.5f) * signal;
	
	return signal;

}

float waveshaper_morph_double_sine( float signal , float curve ){

	signal = (0.5f * sin ( 2 * M_PI * curve * signal) * sin ( 4 * M_PI * signal) + 0.5f) * signal;
	
	return signal;

}

float waveshaper_morph_triple_sine( float signal , float curve ){

	signal = (0.5f * sin ( 2 * M_PI * curve * signal) * sin ( 4 * M_PI * curve * signal) * sin ( 6 * M_PI * signal) + 0.5f) * signal;
	
	return signal;

}

float waveshaper_morph_quadruple_sine( float signal , float curve ){

	signal = (0.5f * sin ( 2 * M_PI * curve * signal) * sin ( 4 * M_PI * signal) * sin ( 6 * M_PI * curve * signal) * sin ( 8 * M_PI * signal) + 0.5f) * signal;
	
	return signal;

}

float waveshaper_rect_sine( float signal , float curve ){

	float value = sin ( 2 * M_PI * curve * signal);
	
	if( value < 0.0f )
		value = -value;

	signal = (0.5f * value + 0.5f) * signal;

	return signal;

}

float waveshaper_nonlin_rect_sine( float signal , float curve ){

	float value = sin ( 2 * M_PI * curve * pow (signal, 1/(curve+1)));
	
	if( value < 0.0f )
		value = -value;

	signal = (0.5f * value + 0.5f) * signal;
	
	return signal;

}
