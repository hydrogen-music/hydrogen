#include "math.h"

/*

DSPE: digital signal processing essentials

LFO.h
A library of functions to produce various LFO forms

(c)2005 Artemiy Pavlov
http://artemiolabs.com

This program is free software, it may be distributed under the terms
and conditions of the GNU General Public License version 2 or later.

*/

/*

Simple triangle LFO

*/

float LFOtri(float step, float period){

	if(step < period/2.0f) {
		return 1.0f - 2.0f * (step/period);
	} else {
		return 2.0f * (step/period) - 1.0f;
	}

}

/*

Simple sine LFO

*/

float LFOsin(float step, float period){

	return sinf (2.0f * M_PI * (step/period));

}

/*

Saw LFO with variable knee

*/

float LFOsaw(float step, float period, float knee){
	
	if(step <= knee*period) {
		return 1.0f/knee * (1.0f - knee) * step/period;
	} else {
		return 1.0f - step/period + knee;
	}

}

/*

Trapezoid LFO with variable knee

[knee value]	[form]

0.0				square
0.25			trapezoid
0.5				triangle

*/

float LFOtrp(float step, float period, float knee){

	if(step < (knee*period)/2.0f) {
		return (step/period) * 2.0f/knee;
	} else if(step >= (knee*period)/2.0f && step < period*(1.0f-knee)/2.0f) {
		return 1.0f;
	} else if(step >= period*(1.0f-knee)/2.0f && step < period*(1.0f+knee)/2.0f) {
		return (1.0f - step/(period/2.0f)) * 1.0f/knee;
	} else if(step >= period*(1.0f+knee)/2.0f && step < period-(knee*period)/2.0f) {
		return -1.0f;
	} else {
		return (step/period - 1.0f) * 2.0f/knee;
	}
	
}
