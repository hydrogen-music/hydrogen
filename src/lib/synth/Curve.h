/*

(c)2005 Artemiy Pavlov

These functions are needed to be able to turn parameter changes from
linear to exponetial and logarithmic.

*/

#include "math.h"

float linear2exponential( float input, int type ){

	if (type == 2){

		return powf(input, M_E);
		
	} else {
	
		return powf(input, 1/M_E);
	
	}

}
