#include "stdlib.h"
#include "math.h"

/*

DSPE: digital signal processing essentials

Gd.h
Random number generator with Gaussian distribution

(c)2005 Artemiy Pavlov
http://artemiolabs.com

This program is free software, it may be distributed under the terms
and conditions of the GNU General Public License version 2 or later.

*/

float Gdrand(float deviation){

	return powf (10, - deviation * powf( (float)(rand()/RAND_MAX) - 1, 2));

}
