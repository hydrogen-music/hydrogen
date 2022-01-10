// Reverb model implementation
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain

#include <stdio.h>
#include "revmodel.h"

int revmodel::calcbufferlength(int tuning, float ratio)
{
	int result;
	
	result = (int)(tuning*ratio + 0.5f);
	if (result == 0)
		result = 1;
	
	return result;
}

revmodel::revmodel(float samplerate)
{
	// Jezar's original comment on the comb/allpass tuning values
	// is: "These values assume 44.1KHz sample rate. They will
	// probably be OK for 48KHz sample rate but would need scaling
	// for 96KHz (or other) sample rates. The values were obtained
	// by listening tests." He also used fixed-size comb/allpass
	// buffers regardless of sample rate.
	//
	// As higher sample rates are more common these days, the
	// buffers are now sized dynamically (with a single memory
	// allocation) and are scaled with the following ratio, which
	// is 1 for both 44.1kHz and 48kHz (for consistency with
	// previous versions).
	float ratio = samplerate / 44100.0f;
	if (ratio > 1)
		ratio = (float)(int)(ratio + 0.5f);
  
	// Potentially modify the buffers' lengths to handle the
	// sample rate.
	int comblengthL1 = calcbufferlength(combtuningL1,ratio);
	int comblengthR1 = calcbufferlength(combtuningR1,ratio);
	int comblengthL2 = calcbufferlength(combtuningL2,ratio);
	int comblengthR2 = calcbufferlength(combtuningR2,ratio);
	int comblengthL3 = calcbufferlength(combtuningL3,ratio);
	int comblengthR3 = calcbufferlength(combtuningR3,ratio);
	int comblengthL4 = calcbufferlength(combtuningL4,ratio);
	int comblengthR4 = calcbufferlength(combtuningR4,ratio);
	int comblengthL5 = calcbufferlength(combtuningL5,ratio);
	int comblengthR5 = calcbufferlength(combtuningR5,ratio);
	int comblengthL6 = calcbufferlength(combtuningL6,ratio);
	int comblengthR6 = calcbufferlength(combtuningR6,ratio);
	int comblengthL7 = calcbufferlength(combtuningL7,ratio);
	int comblengthR7 = calcbufferlength(combtuningR7,ratio);
	int comblengthL8 = calcbufferlength(combtuningL8,ratio);
	int comblengthR8 = calcbufferlength(combtuningR8,ratio);
	int allpasslengthL1 = calcbufferlength(allpasstuningL1,ratio);
	int allpasslengthR1 = calcbufferlength(allpasstuningR1,ratio);
	int allpasslengthL2 = calcbufferlength(allpasstuningL2,ratio);
	int allpasslengthR2 = calcbufferlength(allpasstuningR2,ratio);
	int allpasslengthL3 = calcbufferlength(allpasstuningL3,ratio);
	int allpasslengthR3 = calcbufferlength(allpasstuningR3,ratio);
	int allpasslengthL4 = calcbufferlength(allpasstuningL4,ratio);
	int allpasslengthR4 = calcbufferlength(allpasstuningR4,ratio);
	// Create buffer space
	buffers = new float[comblengthL1+comblengthR1
			    +comblengthL2+comblengthR2
			    +comblengthL3+comblengthR3
			    +comblengthL4+comblengthR4
			    +comblengthL5+comblengthR5
			    +comblengthL6+comblengthR6
			    +comblengthL7+comblengthR7
			    +comblengthL8+comblengthR8
			    +allpasslengthL1+allpasslengthR1
			    +allpasslengthL2+allpasslengthR2
			    +allpasslengthL3+allpasslengthR3
			    +allpasslengthL4+allpasslengthR4];
	// Tie the components to their buffers
	float * bufferat = buffers;
	combL[0].setbuffer(bufferat,comblengthL1);
	bufferat += comblengthL1;
	combR[0].setbuffer(bufferat,comblengthR1);
	bufferat += comblengthR1;
	combL[1].setbuffer(bufferat,comblengthL2);
	bufferat += comblengthL2;
	combR[1].setbuffer(bufferat,comblengthR2);
	bufferat += comblengthR2;
	combL[2].setbuffer(bufferat,comblengthL3);
	bufferat += comblengthL3;
	combR[2].setbuffer(bufferat,comblengthR3);
	bufferat += comblengthR3;
	combL[3].setbuffer(bufferat,comblengthL4);
	bufferat += comblengthL4;
	combR[3].setbuffer(bufferat,comblengthR4);
	bufferat += comblengthR4;
	combL[4].setbuffer(bufferat,comblengthL5);
	bufferat += comblengthL5;
	combR[4].setbuffer(bufferat,comblengthR5);
	bufferat += comblengthR5;
	combL[5].setbuffer(bufferat,comblengthL6);
	bufferat += comblengthL6;
	combR[5].setbuffer(bufferat,comblengthR6);
	bufferat += comblengthR6;
	combL[6].setbuffer(bufferat,comblengthL7);
	bufferat += comblengthL7;
	combR[6].setbuffer(bufferat,comblengthR7);
	bufferat += comblengthR7;
	combL[7].setbuffer(bufferat,comblengthL8);
	bufferat += comblengthL8;
	combR[7].setbuffer(bufferat,comblengthR8);
	bufferat += comblengthR8;
	allpassL[0].setbuffer(bufferat,allpasslengthL1);
	bufferat += allpasslengthL1;
	allpassR[0].setbuffer(bufferat,allpasslengthR1);
	bufferat += allpasslengthR1;
	allpassL[1].setbuffer(bufferat,allpasslengthL2);
	bufferat += allpasslengthL2;
	allpassR[1].setbuffer(bufferat,allpasslengthR2);
	bufferat += allpasslengthR2;
	allpassL[2].setbuffer(bufferat,allpasslengthL3);
	bufferat += allpasslengthL3;
	allpassR[2].setbuffer(bufferat,allpasslengthR3);
	bufferat += allpasslengthR3;
	allpassL[3].setbuffer(bufferat,allpasslengthL4);
	bufferat += allpasslengthL4;
	allpassR[3].setbuffer(bufferat,allpasslengthR4);
	
	// Set default values
	allpassL[0].setfeedback(0.5f);
	allpassR[0].setfeedback(0.5f);
	allpassL[1].setfeedback(0.5f);
	allpassR[1].setfeedback(0.5f);
	allpassL[2].setfeedback(0.5f);
	allpassR[2].setfeedback(0.5f);
	allpassL[3].setfeedback(0.5f);
	allpassR[3].setfeedback(0.5f);
	setwet(initialwet);
	setroomsize(initialroom);
	setdry(initialdry);
	setdamp(initialdamp);
	setwidth(initialwidth);
	setmode(initialmode);

	// Buffer will be full of rubbish - so we MUST mute them
	mute();
}

revmodel::~revmodel()
{
	delete [] buffers;
}

void revmodel::mute()
{
	int i;

	if (getmode() >= freezemode)
		return;

	for (i=0;i<numcombs;i++)
	{
		combL[i].mute();
		combR[i].mute();
	}
	for (i=0;i<numallpasses;i++)
	{
		allpassL[i].mute();
		allpassR[i].mute();
	}
}

void revmodel::processreplace(float *inputL, float *inputR, float *outputL, float *outputR, long numsamples, int skip)
{
	float outL,outR,input;
	int i;

	while(numsamples-- > 0)
	{
		outL = outR = 0;
		input = (*inputL + *inputR) * gain;

		// Accumulate comb filters in parallel
		for(i=0; i<numcombs; i++)
		{
			outL += combL[i].process(input);
			outR += combR[i].process(input);
		}

		// Feed through allpasses in series
		for(i=0; i<numallpasses; i++)
		{
			outL = allpassL[i].process(outL);
			outR = allpassR[i].process(outR);
		}

		// Calculate output REPLACING anything already there
		*outputL = outL*wet1 + outR*wet2 + *inputL*dry;
		*outputR = outR*wet1 + outL*wet2 + *inputR*dry;

		// Increment sample pointers, allowing for interleave (if any)
		inputL += skip;
		inputR += skip;
		outputL += skip;
		outputR += skip;
	}
}

void revmodel::processmix(float *inputL, float *inputR, float *outputL, float *outputR, long numsamples, int skip)
{
	float outL,outR,input;
	int i;

	while(numsamples-- > 0)
	{
		outL = outR = 0;
		input = (*inputL + *inputR) * gain;

		// Accumulate comb filters in parallel
		for(i=0; i<numcombs; i++)
		{
			outL += combL[i].process(input);
			outR += combR[i].process(input);
		}

		// Feed through allpasses in series
		for(i=0; i<numallpasses; i++)
		{
			outL = allpassL[i].process(outL);
			outR = allpassR[i].process(outR);
		}

		// Calculate output MIXING with anything already there
		*outputL += outL*wet1 + outR*wet2 + *inputL*dry;
		*outputR += outR*wet1 + outL*wet2 + *inputR*dry;

		// Increment sample pointers, allowing for interleave (if any)
		inputL += skip;
		inputR += skip;
		outputL += skip;
		outputR += skip;
	}
}

void revmodel::update()
{
// Recalculate internal values after parameter change

	int i;

	wet1 = wet*(width/2 + 0.5f);
	wet2 = wet*((1-width)/2);

	if (mode >= freezemode)
	{
		roomsize1 = 1;
		damp1 = 0;
		gain = muted;
	}
	else
	{
		roomsize1 = roomsize;
		damp1 = damp;
		gain = fixedgain;
	}

	for(i=0; i<numcombs; i++)
	{
		combL[i].setfeedback(roomsize1);
		combR[i].setfeedback(roomsize1);
	}

	for(i=0; i<numcombs; i++)
	{
		combL[i].setdamp(damp1);
		combR[i].setdamp(damp1);
	}
}

// The following get/set functions are not inlined, because
// speed is never an issue when calling them, and also
// because as you develop the reverb model, you may
// wish to take dynamic action when they are called.

void revmodel::setroomsize(float value)
{
	roomsize = (value*scaleroom) + offsetroom;
	update();
}

float revmodel::getroomsize()
{
	return (roomsize-offsetroom)/scaleroom;
}

void revmodel::setdamp(float value)
{
	damp = value*scaledamp;
	update();
}

float revmodel::getdamp()
{
	return damp/scaledamp;
}

void revmodel::setwet(float value)
{
	wet = value*scalewet;
	update();
}

float revmodel::getwet()
{
	return wet/scalewet;
}

void revmodel::setdry(float value)
{
	dry = value*scaledry;
}

float revmodel::getdry()
{
	return dry/scaledry;
}

void revmodel::setwidth(float value)
{
	width = value;
	update();
}

float revmodel::getwidth()
{
	return width;
}

void revmodel::setmode(float value)
{
	mode = value;
	update();
}

float revmodel::getmode()
{
	if (mode >= freezemode)
		return 1;
	else
		return 0;
}

//ends
