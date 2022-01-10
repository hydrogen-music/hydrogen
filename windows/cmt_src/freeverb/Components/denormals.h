// Macro for killing denormalled numbers
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// Originally based on IS_DENORMAL macro by Jon Watte, updated to use C99 isnormal().
// This code is public domain

#ifndef _denormals_
#define _denormals_

//#define undenormalise(sample) if(((*(unsigned int*)&sample)&0x7f800000)==0) sample=0.0f

#include <cmath>
#define undenormalise(sample) if(!std::isnormal(sample)) sample=0.0f

#endif//_denormals_

//ends
