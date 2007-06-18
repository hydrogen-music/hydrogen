/*

x-shaper.c

Multi-mode wave shaper (mono/stereo)

(c)2005 Artemiy Pavlov

*/

/* Includes: */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "ladspa.h"
#include "waveshaper.h"
#include "LFO.h"

/* The port numbers for the plugin: */

#define XSHAPER_GAIN 0
#define XSHAPER_TYPE 1
#define XSHAPER_CURVE 2
#define XSHAPER_AMOUNT 3
#define XSHAPER_LFO1FORM 4
#define XSHAPER_LFO1RATE 5
#define XSHAPER_LFO1DEPTH 6
#define XSHAPER_LFO2FORM 7
#define XSHAPER_LFO2RATE 8
#define XSHAPER_LFO2DEPTH 9
#define XSHAPER_INPUT1  10
#define XSHAPER_OUTPUT1 11
#define XSHAPER_INPUT2 12
#define XSHAPER_OUTPUT2 13

#define XSHAPER_TYPES 9
#define XSHAPER_LFOTYPES 5

/* The structure used to hold port connection information and state  */

typedef struct {
	
	unsigned long SampleRate;

	LADSPA_Data * m_pfGain;
	LADSPA_Data * m_pfCurve;
	LADSPA_Data * m_pfType;
	LADSPA_Data * m_pfAmount;
	LADSPA_Data * m_pfLFO1Form;
	LADSPA_Data * m_pfLFO1Rate;
	LADSPA_Data * m_pfLFO1Depth;
	LADSPA_Data * m_pfLFO2Form;
	LADSPA_Data * m_pfLFO2Rate;
	LADSPA_Data * m_pfLFO2Depth;
	LADSPA_Data * m_pfInputBuffer1;
	LADSPA_Data * m_pfOutputBuffer1;
	LADSPA_Data * m_pfInputBuffer2;
	LADSPA_Data * m_pfOutputBuffer2;

	LADSPA_Data m_fGainLast;
	LADSPA_Data m_fCurveLast;
	LADSPA_Data m_fAmountLast;
	LADSPA_Data m_fLFO1RateLast;
	LADSPA_Data m_fLFO2RateLast;
	LADSPA_Data m_fLFO1DepthLast;
	LADSPA_Data m_fLFO2DepthLast;
	
	float m_fLFO1Step;
	float m_fLFO2Step;

} XShaper;

/* Construct a new plugin instance. */

LADSPA_Handle instantiateXShaper(const LADSPA_Descriptor * Descriptor, unsigned long SampleRate) {

	XShaper *mp = calloc(sizeof(XShaper),1);

	mp->SampleRate = SampleRate;
	
	return mp;
	
}

/* Connect a port to a data location. */

void connectPortToXShaper(LADSPA_Handle Instance, unsigned long Port, LADSPA_Data * DataLocation) {

	XShaper * psXShaper;

	psXShaper = (XShaper *)Instance;
	
	switch (Port) {
		case XSHAPER_GAIN:
			psXShaper->m_pfGain = DataLocation;
    		break;
		case XSHAPER_TYPE:
			psXShaper->m_pfType = DataLocation;
    		break;
		case XSHAPER_CURVE:
			psXShaper->m_pfCurve = DataLocation;
    		break;
		case XSHAPER_AMOUNT:
			psXShaper->m_pfAmount = DataLocation;
    		break;
		case XSHAPER_LFO1FORM:
			psXShaper->m_pfLFO1Form = DataLocation;
    		break;
		case XSHAPER_LFO1RATE:
			psXShaper->m_pfLFO1Rate = DataLocation;
    		break;
		case XSHAPER_LFO1DEPTH:
			psXShaper->m_pfLFO1Depth = DataLocation;
    		break;
		case XSHAPER_LFO2FORM:
			psXShaper->m_pfLFO2Form = DataLocation;
    		break;
		case XSHAPER_LFO2RATE:
			psXShaper->m_pfLFO2Rate = DataLocation;
    		break;
		case XSHAPER_LFO2DEPTH:
			psXShaper->m_pfLFO2Depth = DataLocation;
    		break;
  		case XSHAPER_INPUT1:
    		psXShaper->m_pfInputBuffer1 = DataLocation;
    		break;
  		case XSHAPER_OUTPUT1:
    		psXShaper->m_pfOutputBuffer1 = DataLocation;
    		break;
  		case XSHAPER_INPUT2:
    		psXShaper->m_pfInputBuffer2 = DataLocation;
    		break;
  		case XSHAPER_OUTPUT2:
		    psXShaper->m_pfOutputBuffer2 = DataLocation;
    		break;
  }
  
}

/* XShaper mono */

void runMonoXShaper(LADSPA_Handle Instance, unsigned long SampleCount) {
  
	/* audio i/o */
	
	unsigned long fSampleRate;
	
	LADSPA_Data * pfInput;
	LADSPA_Data * pfOutput;
	LADSPA_Data inputData;
	LADSPA_Data outputData;
	
	/* input params */
	
	LADSPA_Data fGain;
	LADSPA_Data fType;
	LADSPA_Data fCurve;
	LADSPA_Data fAmount;
	
	LADSPA_Data fLFO1Form;
	LADSPA_Data fLFO1Rate;
	LADSPA_Data fLFO1Depth;
	float fLFO1Step;
	LADSPA_Data fLFO1Period;
	LADSPA_Data fLFO1Value = 0.0f;
	
	LADSPA_Data fLFO2Form;
	LADSPA_Data fLFO2Rate;
	LADSPA_Data fLFO2Depth;
	float fLFO2Step;
	LADSPA_Data fLFO2Period;
	LADSPA_Data fLFO2Value = 0.0f;
	
	/* interpolation */
	
	LADSPA_Data rCurve;
	LADSPA_Data fGainLast;
	LADSPA_Data fCurveLast;
	LADSPA_Data fAmountLast;
	LADSPA_Data fLFO1RateLast;
	LADSPA_Data fLFO2RateLast;
	LADSPA_Data fLFO1DepthLast;
	LADSPA_Data fLFO2DepthLast;
	LADSPA_Data dGain;
	LADSPA_Data dCurve;
	LADSPA_Data dAmount;
	LADSPA_Data dLFO1Rate;
	LADSPA_Data dLFO2Rate;
	LADSPA_Data dLFO1Depth;
	LADSPA_Data dLFO2Depth;

	XShaper * psXShaper;
	unsigned long lSampleIndex;

	psXShaper = (XShaper *)Instance;
	
	fSampleRate = psXShaper->SampleRate;
	
	fGain = *(psXShaper->m_pfGain);
	fType = *(psXShaper->m_pfType);
	fCurve = *(psXShaper->m_pfCurve);
	fAmount = *(psXShaper->m_pfAmount);
	fLFO1Form = *(psXShaper->m_pfLFO1Form);
	fLFO1Rate = *(psXShaper->m_pfLFO1Rate);
	fLFO1Depth = *(psXShaper->m_pfLFO1Depth);
	fLFO2Form = *(psXShaper->m_pfLFO2Form);
	fLFO2Rate = *(psXShaper->m_pfLFO2Rate);
	fLFO2Depth = *(psXShaper->m_pfLFO2Depth);
	
	if(fLFO1Rate < 0.001f)
		fLFO1Rate = 0.001f;
	
	if(fLFO2Rate < 0.001f)
		fLFO2Rate = 0.001f;
	
	if(fType >= 0.0f && fType <= 1.0f){
		fType = 1;
	} else if(fType > 1.0f && fType <= 2.0f){
		fType = 2;
	} else if(fType > 2.0f && fType <= 3.0f){
		fType = 3;
	} else if(fType > 3.0f && fType <= 4.0f){
		fType = 4;
	} else if(fType > 4.0f && fType <= 5.0f){
		fType = 5;
	} else if(fType > 5.0f && fType <= 6.0f){
		fType = 6;
	} else if(fType > 6.0f && fType <= 7.0f){
		fType = 7;
	} else if(fType > 7.0f && fType <= 8.0f){
		fType = 8;
	} else if(fType > 8.0f && fType <= 9.0f){
		fType = 9;
	}
	
	if(fLFO1Form >= 0.0f && fLFO1Form <= 1.0f){
		fLFO1Form = 1;
	} else if(fLFO1Form > 1.0f && fLFO1Form <= 2.0f){
		fLFO1Form = 2;
	} else if(fLFO1Form > 2.0f && fLFO1Form <= 3.0f){
		fLFO1Form = 3;
	} else if(fLFO1Form > 3.0f && fLFO1Form <= 4.0f){
		fLFO1Form = 4;
	} else if(fLFO1Form > 4.0f && fLFO1Form <= 5.0f){
		fLFO1Form = 5;
	} else {
		fLFO1Form = 1;
	}
	
	if(fLFO2Form >= 0.0f && fLFO2Form <= 1.0f){
		fLFO2Form = 1;
	} else if(fLFO2Form > 1.0f && fLFO2Form <= 2.0f){
		fLFO2Form = 2;
	} else if(fLFO2Form > 2.0f && fLFO2Form <= 3.0f){
		fLFO2Form = 3;
	} else if(fLFO2Form > 3.0f && fLFO2Form <= 4.0f){
		fLFO2Form = 4;
	} else if(fLFO2Form > 4.0f && fLFO2Form <= 5.0f){
		fLFO2Form = 5;
	} else {
		fLFO2Form = 1;
	}
	
	fLFO1Step = psXShaper->m_fLFO1Step;
	fLFO2Step = psXShaper->m_fLFO2Step;
	
	if(!fLFO1Step)
		fLFO1Step = 0;
	if(!fLFO2Step)
		fLFO2Step = 0;
	
	fGainLast = psXShaper->m_fGainLast;
	fCurveLast = psXShaper->m_fCurveLast;
	fAmountLast = psXShaper->m_fAmountLast;
	fLFO1RateLast = psXShaper->m_fLFO1RateLast;
	fLFO2RateLast = psXShaper->m_fLFO2RateLast;
	fLFO1DepthLast = psXShaper->m_fLFO1DepthLast;
	fLFO2DepthLast = psXShaper->m_fLFO2DepthLast;
	
	if(!fGainLast)
		fGainLast = fGain;
		
	if(!fCurveLast)
		fCurveLast = fCurve;
	
	if(!fAmountLast)
		fAmountLast = fAmount;
	
	if(!fLFO1RateLast)
		fLFO1RateLast = fLFO1Rate;
	
	if(!fLFO2RateLast)
		fLFO2RateLast = fLFO2Rate;
	
	if(!fLFO1DepthLast)
		fLFO1DepthLast = fLFO1Depth;
	
	if(!fLFO2DepthLast)
		fLFO2DepthLast = fLFO2Depth;
	
	dGain = (fGain - fGainLast) / SampleCount;
	dCurve = (fCurve - fCurveLast) / SampleCount;
	dAmount = (fAmount - fAmountLast) / SampleCount;
	dLFO1Rate = (fLFO1Rate - fLFO1RateLast) / SampleCount;
	dLFO2Rate = (fLFO2Rate - fLFO2RateLast) / SampleCount;
	dLFO1Depth = (fLFO1Depth - fLFO1DepthLast) / SampleCount;
	dLFO2Depth = (fLFO2Depth - fLFO2DepthLast) / SampleCount;
	
	fGain = fGainLast;
	fCurve = fCurveLast;
	fAmount = fAmountLast;
	fLFO1Rate = fLFO1RateLast;
	fLFO2Rate = fLFO2RateLast;
	fLFO1Depth = fLFO1DepthLast;
	fLFO2Depth = fLFO2DepthLast;
	
	pfInput = psXShaper->m_pfInputBuffer1;
	pfOutput = psXShaper->m_pfOutputBuffer1;
	
	for(lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++){	
	
		fGain = fGain + dGain;
		fCurve = fCurve + dCurve;
		fAmount = fAmount + dAmount;
		fLFO1Rate = fLFO1Rate + dLFO1Rate;
		fLFO2Rate = fLFO2Rate + dLFO2Rate;
		fLFO1Depth = fLFO1Depth + dLFO1Depth;
		fLFO2Depth = fLFO2Depth + dLFO2Depth;
		
		fLFO1Period = fSampleRate/fLFO1Rate;
		fLFO2Period = fSampleRate/fLFO2Rate;
		
		fLFO1Step++;
		fLFO2Step++;
		
		if(fLFO1Step >= fLFO1Period){
			fLFO1Step = 0;
		}
		
		if(fLFO2Step >= fLFO2Period){
			fLFO2Step = 0;
		}
		
		switch ((int)fLFO1Form){
			case 1:
				fLFO1Value = LFOtri(fLFO1Step,fLFO1Period);
				break;
			case 2:
				fLFO1Value = LFOsin(fLFO1Step,fLFO1Period);
				break;
			case 3:
				fLFO1Value = LFOsaw(fLFO1Step,fLFO1Period,0.05f);
				break;
			case 4:
				fLFO1Value = LFOtrp(fLFO1Step,fLFO1Period,0.02f);
				break;
			case 5:
				fLFO1Value = LFOtrp(fLFO1Step,fLFO1Period,0.25f);
				break;
		}
		
		switch ((int)fLFO2Form){
			case 1:
				fLFO2Value = LFOtri(fLFO2Step,fLFO2Period);
				break;
			case 2:
				fLFO2Value = LFOsin(fLFO2Step,fLFO2Period);
				break;
			case 3:
				fLFO2Value = LFOsaw(fLFO2Step,fLFO2Period,0.02f);
				break;
			case 4:
				fLFO2Value = LFOtrp(fLFO2Step,fLFO2Period,0.02f);
				break;
			case 5:
				fLFO2Value = LFOtrp(fLFO2Step,fLFO2Period,0.25f);
				break;
		}
		
		rCurve = 3.0f * (fCurve + 0.5f * (fLFO2Value * fLFO2Depth));
	
		fLFO1Value = (fLFO1Value + 1.0f)/2.0f;
		
		inputData = *(pfInput++) * (fGain * (1-fLFO1Depth) +  fLFO1Value * fLFO1Depth);
		
		if( inputData < 0.0f ){
			outputData = -inputData;
		} else {
			outputData = inputData;
		}
		
		if(fType == 1){
			outputData = waveshaper_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 2){
			outputData = waveshaper_double_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 3){
			outputData = waveshaper_quadruple_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 4){
			outputData = waveshaper_triple_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 5){
			outputData = waveshaper_morph_double_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 6){
			outputData = waveshaper_morph_triple_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 7){
			outputData = waveshaper_morph_quadruple_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 8){
			outputData = waveshaper_rect_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 9){
			outputData = waveshaper_nonlin_rect_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		}
		
		if(outputData < 0.0f){
			outputData = -outputData;
		}
		
		if(inputData >= 0.0f){
			*(pfOutput++) = outputData ;
		} else {
			*(pfOutput++) = -outputData ;
		}
		
	}
		
	psXShaper->m_fGainLast = fGain;
	psXShaper->m_fCurveLast = fCurve;
	psXShaper->m_fAmountLast = fAmount;
	psXShaper->m_fLFO1RateLast = fLFO1Rate;
	psXShaper->m_fLFO2RateLast = fLFO2Rate;
	psXShaper->m_fLFO1DepthLast = fLFO1Depth;
	psXShaper->m_fLFO2DepthLast = fLFO2Depth;
	
	psXShaper->m_fLFO1Step = fLFO1Step;
	psXShaper->m_fLFO2Step = fLFO2Step;
	rCurve = fCurve + 0.5f * (fLFO2Value * fLFO2Depth);
		
		if(fType == 1){
			rCurve = - rCurve + 1.01f;
		} else {
			rCurve = rCurve + 1.01f;
		}
		
		rCurve = rCurve * 1.5f;
}

/* XShaper stereo */

void runStereoXShaper(LADSPA_Handle Instance, unsigned long SampleCount) {
  
	/* audio i/o */
	
	unsigned long fSampleRate;
	
	LADSPA_Data * pfInput;
	LADSPA_Data * pfOutput;
	LADSPA_Data inputData;
	LADSPA_Data outputData;
	
	/* input params */
	
	LADSPA_Data fGain;
	LADSPA_Data fType;
	LADSPA_Data fCurve;
	LADSPA_Data fAmount;
	
	LADSPA_Data fLFO1Form;
	LADSPA_Data fLFO1Rate;
	LADSPA_Data fLFO1Depth;
	float fLFO1Step;
	LADSPA_Data fLFO1Period;
	LADSPA_Data fLFO1Value = 0.0f;
	
	LADSPA_Data fLFO2Form;
	LADSPA_Data fLFO2Rate;
	LADSPA_Data fLFO2Depth;
	float fLFO2Step;
	LADSPA_Data fLFO2Period;
	LADSPA_Data fLFO2Value = 0.0f;
	
	/* interpolation */
	
	LADSPA_Data rCurve;
	LADSPA_Data fGainLast;
	LADSPA_Data fCurveLast;
	LADSPA_Data fAmountLast;
	LADSPA_Data fLFO1RateLast;
	LADSPA_Data fLFO2RateLast;
	LADSPA_Data fLFO1DepthLast;
	LADSPA_Data fLFO2DepthLast;
	LADSPA_Data dGain;
	LADSPA_Data dCurve;
	LADSPA_Data dAmount;
	LADSPA_Data dLFO1Rate;
	LADSPA_Data dLFO2Rate;
	LADSPA_Data dLFO1Depth;
	LADSPA_Data dLFO2Depth;
	
	XShaper * psXShaper;
	unsigned long lSampleIndex;

	psXShaper = (XShaper *)Instance;
	
	fSampleRate = psXShaper->SampleRate;
	
	fGain = *(psXShaper->m_pfGain);
	fType = *(psXShaper->m_pfType);
	fCurve = *(psXShaper->m_pfCurve);
	fAmount = *(psXShaper->m_pfAmount);
	fLFO1Form = *(psXShaper->m_pfLFO1Form);
	fLFO1Rate = *(psXShaper->m_pfLFO1Rate);
	fLFO1Depth = *(psXShaper->m_pfLFO1Depth);
	fLFO2Form = *(psXShaper->m_pfLFO2Form);
	fLFO2Rate = *(psXShaper->m_pfLFO2Rate);
	fLFO2Depth = *(psXShaper->m_pfLFO2Depth);
	
	if(fLFO1Rate < 0.001f)
		fLFO1Rate = 0.001f;
	
	if(fLFO2Rate < 0.001f)
		fLFO2Rate = 0.001f;
	
	if(fType >= 0.0f && fType <= 1.0f){
		fType = 1;
	} else if(fType > 1.0f && fType <= 2.0f){
		fType = 2;
	} else if(fType > 2.0f && fType <= 3.0f){
		fType = 3;
	} else if(fType > 3.0f && fType <= 4.0f){
		fType = 4;
	} else if(fType > 4.0f && fType <= 5.0f){
		fType = 5;
	} else if(fType > 5.0f && fType <= 6.0f){
		fType = 6;
	} else if(fType > 6.0f && fType <= 7.0f){
		fType = 7;
	} else if(fType > 7.0f && fType <= 8.0f){
		fType = 8;
	} else if(fType > 8.0f && fType <= 9.0f){
		fType = 9;
	}

	if(fLFO1Form >= 0.0f && fLFO1Form <= 1.0f){
		fLFO1Form = 1;
	} else if(fLFO1Form > 1.0f && fLFO1Form <= 2.0f){
		fLFO1Form = 2;
	} else if(fLFO1Form > 2.0f && fLFO1Form <= 3.0f){
		fLFO1Form = 3;
	} else if(fLFO1Form > 3.0f && fLFO1Form <= 4.0f){
		fLFO1Form = 4;
	} else if(fLFO1Form > 4.0f && fLFO1Form <= 5.0f){
		fLFO1Form = 5;
	} else {
		fLFO1Form = 1;
	}
	
	if(fLFO2Form >= 0.0f && fLFO2Form <= 1.0f){
		fLFO2Form = 1;
	} else if(fLFO2Form > 1.0f && fLFO2Form <= 2.0f){
		fLFO2Form = 2;
	} else if(fLFO2Form > 2.0f && fLFO2Form <= 3.0f){
		fLFO2Form = 3;
	} else if(fLFO2Form > 3.0f && fLFO2Form <= 4.0f){
		fLFO2Form = 4;
	} else if(fLFO2Form > 4.0f && fLFO2Form <= 5.0f){
		fLFO2Form = 5;
	} else {
		fLFO2Form = 1;
	}
	
	fLFO1Step = psXShaper->m_fLFO1Step;
	fLFO2Step = psXShaper->m_fLFO2Step;
	
	if(!fLFO1Step)
		fLFO1Step = 0;
	if(!fLFO2Step)
		fLFO2Step = 0;
	
	fGainLast = psXShaper->m_fGainLast;
	fCurveLast = psXShaper->m_fCurveLast;
	fAmountLast = psXShaper->m_fAmountLast;
	fLFO1RateLast = psXShaper->m_fLFO1RateLast;
	fLFO2RateLast = psXShaper->m_fLFO2RateLast;
	fLFO1DepthLast = psXShaper->m_fLFO1DepthLast;
	fLFO2DepthLast = psXShaper->m_fLFO2DepthLast;
	
	if(!fGainLast)
		fGainLast = fGain;
		
	if(!fCurveLast)
		fCurveLast = fCurve;
	
	if(!fAmountLast)
		fAmountLast = fAmount;
	
	if(!fLFO1RateLast)
		fLFO1RateLast = fLFO1Rate;
	
	if(!fLFO2RateLast)
		fLFO2RateLast = fLFO2Rate;
	
	if(!fLFO1DepthLast)
		fLFO1DepthLast = fLFO1Depth;
	
	if(!fLFO2DepthLast)
		fLFO2DepthLast = fLFO2Depth;
	
	dGain = (fGain - fGainLast) / SampleCount;
	dCurve = (fCurve - fCurveLast) / SampleCount;
	dAmount = (fAmount - fAmountLast) / SampleCount;
	dLFO1Rate = (fLFO1Rate - fLFO1RateLast) / SampleCount;
	dLFO2Rate = (fLFO2Rate - fLFO2RateLast) / SampleCount;
	dLFO1Depth = (fLFO1Depth - fLFO1DepthLast) / SampleCount;
	dLFO2Depth = (fLFO2Depth - fLFO2DepthLast) / SampleCount;
	
	fGain = fGainLast;
	fCurve = fCurveLast;
	fAmount = fAmountLast;
	fLFO1Rate = fLFO1RateLast;
	fLFO2Rate = fLFO2RateLast;
	fLFO1Depth = fLFO1DepthLast;
	fLFO2Depth = fLFO2DepthLast;
	
	pfInput = psXShaper->m_pfInputBuffer1;
	pfOutput = psXShaper->m_pfOutputBuffer1;
  
	for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++){
		
		fGain = fGain + dGain;
		fCurve = fCurve + dCurve;
		fAmount = fAmount + dAmount;
		fLFO1Rate = fLFO1Rate + dLFO1Rate;
		fLFO2Rate = fLFO2Rate + dLFO2Rate;
		fLFO1Depth = fLFO1Depth + dLFO1Depth;
		fLFO2Depth = fLFO2Depth + dLFO2Depth;
		
		fLFO1Period = fSampleRate/fLFO1Rate;
		fLFO2Period = fSampleRate/fLFO2Rate;
		
		fLFO1Step++;
		fLFO2Step++;
		
		if(fLFO1Step >= fLFO1Period){
			fLFO1Step = 0;
		}
		
		if(fLFO2Step >= fLFO2Period){
			fLFO2Step = 0;
		}
		
		switch ((int)fLFO1Form){
			case 1:
				fLFO1Value = LFOtri(fLFO1Step,fLFO1Period);
				break;
			case 2:
				fLFO1Value = LFOsin(fLFO1Step,fLFO1Period);
				break;
			case 3:
				fLFO1Value = LFOsaw(fLFO1Step,fLFO1Period,0.05f);
				break;
			case 4:
				fLFO1Value = LFOtrp(fLFO1Step,fLFO1Period,0.02f);
				break;
			case 5:
				fLFO1Value = LFOtrp(fLFO1Step,fLFO1Period,0.25f);
				break;
		}
		
		switch ((int)fLFO2Form){
			case 1:
				fLFO2Value = LFOtri(fLFO2Step,fLFO2Period);
				break;
			case 2:
				fLFO2Value = LFOsin(fLFO2Step,fLFO2Period);
				break;
			case 3:
				fLFO2Value = LFOsaw(fLFO2Step,fLFO2Period,0.02f);
				break;
			case 4:
				fLFO2Value = LFOtrp(fLFO2Step,fLFO2Period,0.02f);
				break;
			case 5:
				fLFO2Value = LFOtrp(fLFO2Step,fLFO2Period,0.25f);
				break;
		}
		
		rCurve = 3.0f * (fCurve + 0.5f * (fLFO2Value * fLFO2Depth));
		
		fLFO1Value = (fLFO1Value + 1.0f)/2.0f;
		
		inputData = *(pfInput++) * (fGain * (1-fLFO1Depth) +  fLFO1Value * fLFO1Depth);
		
		if( inputData < 0.0f ){
			outputData = -inputData;
		} else {
			outputData = inputData;
		}
		
		if(fType == 1){
			outputData = waveshaper_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 2){
			outputData = waveshaper_double_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 3){
			outputData = waveshaper_quadruple_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 4){
			outputData = waveshaper_triple_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 5){
			outputData = waveshaper_morph_double_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 6){
			outputData = waveshaper_morph_triple_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 7){
			outputData = waveshaper_morph_quadruple_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 8){
			outputData = waveshaper_rect_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 9){
			outputData = waveshaper_nonlin_rect_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		}
		
		if(fType >= 3 && fType <= 8){
			if(outputData < 0.0f)
				outputData = -outputData;
		}
		
		if( inputData >= 0.0f ){
			*(pfOutput++) = outputData ;
		} else {
			*(pfOutput++) = -outputData ;
		}
		
	}
	
	fLFO1Step = psXShaper->m_fLFO1Step;
	fLFO2Step = psXShaper->m_fLFO2Step;
	
	if(!fLFO1Step)
		fLFO1Step = 0;
	if(!fLFO2Step)
		fLFO2Step = 0;

	fGain = fGainLast;
	fCurve = fCurveLast;
	fAmount = fAmountLast;
	fLFO1Rate = fLFO1RateLast;
	fLFO2Rate = fLFO2RateLast;
	fLFO1Depth = fLFO1DepthLast;
	fLFO2Depth = fLFO2DepthLast;

	pfInput = psXShaper->m_pfInputBuffer2;
	pfOutput = psXShaper->m_pfOutputBuffer2;
	
	for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++){
		
		fGain = fGain + dGain;
		fCurve = fCurve + dCurve;
		fAmount = fAmount + dAmount;
		fLFO1Rate = fLFO1Rate + dLFO1Rate;
		fLFO2Rate = fLFO2Rate + dLFO2Rate;
		fLFO1Depth = fLFO1Depth + dLFO1Depth;
		fLFO2Depth = fLFO2Depth + dLFO2Depth;
		
		fLFO1Period = fSampleRate/fLFO1Rate;
		fLFO2Period = fSampleRate/fLFO2Rate;
		
		fLFO1Step++;
		fLFO2Step++;
		
		if(fLFO1Step >= fLFO1Period){
			fLFO1Step = 0;
		}
		
		if(fLFO2Step >= fLFO2Period){
			fLFO2Step = 0;
		}
		
		switch ((int)fLFO1Form){
			case 1:
				fLFO1Value = LFOtri(fLFO1Step,fLFO1Period);
				break;
			case 2:
				fLFO1Value = LFOsin(fLFO1Step,fLFO1Period);
				break;
			case 3:
				fLFO1Value = LFOsaw(fLFO1Step,fLFO1Period,0.05f);
				break;
			case 4:
				fLFO1Value = LFOtrp(fLFO1Step,fLFO1Period,0.02f);
				break;
			case 5:
				fLFO1Value = LFOtrp(fLFO1Step,fLFO1Period,0.25f);
				break;
		}
		
		switch ((int)fLFO2Form){
			case 1:
				fLFO2Value = LFOtri(fLFO2Step,fLFO2Period);
				break;
			case 2:
				fLFO2Value = LFOsin(fLFO2Step,fLFO2Period);
				break;
			case 3:
				fLFO2Value = LFOsaw(fLFO2Step,fLFO2Period,0.02f);
				break;
			case 4:
				fLFO2Value = LFOtrp(fLFO2Step,fLFO2Period,0.02f);
				break;
			case 5:
				fLFO2Value = LFOtrp(fLFO2Step,fLFO2Period,0.25f);
				break;
		}
		
		rCurve = 3.0f * (fCurve + 0.5f * (fLFO2Value * fLFO2Depth));
	
		fLFO1Value = (fLFO1Value + 1.0f)/2.0f;
		
		inputData = *(pfInput++) * (fGain * (1-fLFO1Depth) +  fLFO1Value * fLFO1Depth);
		
		if( inputData < 0.0f ){
			outputData = -inputData;
		} else {
			outputData = inputData;
		}
		
		if(fType == 1){
			outputData = waveshaper_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 2){
			outputData = waveshaper_double_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 3){
			outputData = waveshaper_quadruple_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 4){
			outputData = waveshaper_triple_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 5){
			outputData = waveshaper_morph_double_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 6){
			outputData = waveshaper_morph_triple_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 7){
			outputData = waveshaper_morph_quadruple_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 8){
			outputData = waveshaper_rect_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		} else if(fType == 9){
			outputData = waveshaper_nonlin_rect_sine(outputData,rCurve) * fAmount + outputData * (1 - fAmount);
		}
		
		if(fType >= 3 && fType <= 8){
			if(outputData < 0.0f)
				outputData = -outputData;
		}
		
		if( inputData >= 0.0f ){
			*(pfOutput++) = outputData ;
		} else {
			*(pfOutput++) = -outputData ;
		}
		
	}
	
	psXShaper->m_fGainLast = fGain;
	psXShaper->m_fCurveLast = fCurve;
	psXShaper->m_fAmountLast = fAmount;
	psXShaper->m_fLFO1RateLast = fLFO1Rate;
	psXShaper->m_fLFO2RateLast = fLFO2Rate;
	psXShaper->m_fLFO1DepthLast = fLFO1Depth;
	psXShaper->m_fLFO2DepthLast = fLFO2Depth;
	
	psXShaper->m_fLFO1Step = fLFO1Step;
	psXShaper->m_fLFO2Step = fLFO2Step;
}

/* XShaper cleanup */

void cleanupXShaper(LADSPA_Handle Instance) {

	XShaper * psXShaper;
	psXShaper = (XShaper *)Instance;

	free(psXShaper);
}

LADSPA_Descriptor * g_psMonoDescriptor = NULL;
LADSPA_Descriptor * g_psStereoDescriptor = NULL;

/* XShaper _init() function */

void _init() {

  char ** pcPortNames;
  LADSPA_PortDescriptor * piPortDescriptors;
  LADSPA_PortRangeHint * psPortRangeHints;

  g_psMonoDescriptor
    = (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));
  g_psStereoDescriptor 
    = (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

  if (g_psMonoDescriptor) {
  
    g_psMonoDescriptor->UniqueID
      = 2547;
    g_psMonoDescriptor->Label
      = strdup("XShaperM");
    g_psMonoDescriptor->Properties
      = LADSPA_PROPERTY_HARD_RT_CAPABLE;
    g_psMonoDescriptor->Name 
      = strdup("X-Shaper (mono)");
    g_psMonoDescriptor->Maker
      = strdup("Artemiy Pavlov");
    g_psMonoDescriptor->Copyright
      = strdup("(c)2005 GPL");
    g_psMonoDescriptor->PortCount
      = 12;
    piPortDescriptors
      = (LADSPA_PortDescriptor *)calloc(12, sizeof(LADSPA_PortDescriptor));
    g_psMonoDescriptor->PortDescriptors
      = (const LADSPA_PortDescriptor *)piPortDescriptors;
	  
	piPortDescriptors[XSHAPER_GAIN]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_TYPE]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_CURVE]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_AMOUNT]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_LFO1FORM]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_LFO1RATE]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_LFO1DEPTH]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_LFO2FORM]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_LFO2RATE]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_LFO2DEPTH]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_INPUT1]
	= LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	piPortDescriptors[XSHAPER_OUTPUT1]
	= LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	  
	pcPortNames
	= (char **)calloc(12, sizeof(char *));
	g_psMonoDescriptor->PortNames 
	= (const char **)pcPortNames;
	  
	pcPortNames[XSHAPER_GAIN]
	= strdup("Gain");
	pcPortNames[XSHAPER_TYPE]
	= strdup("Type");
	pcPortNames[XSHAPER_CURVE]
	= strdup("Curve");
	pcPortNames[XSHAPER_AMOUNT]
	= strdup("Amount");
	pcPortNames[XSHAPER_LFO1FORM]
	= strdup("LFO1 Form");
	pcPortNames[XSHAPER_LFO1RATE]
	= strdup("LFO1 Rate");
	pcPortNames[XSHAPER_LFO1DEPTH]
	= strdup("LFO1 Gain Depth");
	pcPortNames[XSHAPER_LFO2FORM]
	= strdup("LFO2 Form");
	pcPortNames[XSHAPER_LFO2RATE]
	= strdup("LFO2 Rate");
	pcPortNames[XSHAPER_LFO2DEPTH]
	= strdup("LFO2 Curve Depth");
	pcPortNames[XSHAPER_INPUT1]
	= strdup("Input");
	pcPortNames[XSHAPER_OUTPUT1]
	= strdup("Output");
	  
    psPortRangeHints = ((LADSPA_PortRangeHint *)
		calloc(12, sizeof(LADSPA_PortRangeHint)));
    g_psMonoDescriptor->PortRangeHints
      = (const LADSPA_PortRangeHint *)psPortRangeHints;
	
	psPortRangeHints[XSHAPER_GAIN].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_DEFAULT_1);
	psPortRangeHints[XSHAPER_GAIN].LowerBound
	= 0;
	psPortRangeHints[XSHAPER_GAIN].UpperBound
	= 1;
	    
	psPortRangeHints[XSHAPER_TYPE].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_INTEGER
	| LADSPA_HINT_DEFAULT_1);
	psPortRangeHints[XSHAPER_TYPE].LowerBound
	= 1;
	psPortRangeHints[XSHAPER_TYPE].UpperBound
	= XSHAPER_TYPES;
	 
	psPortRangeHints[XSHAPER_CURVE].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_DEFAULT_0);
	psPortRangeHints[XSHAPER_CURVE].LowerBound
	= 0;
	psPortRangeHints[XSHAPER_CURVE].UpperBound
	= 1;
	 
	psPortRangeHints[XSHAPER_AMOUNT].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_DEFAULT_1);
	psPortRangeHints[XSHAPER_AMOUNT].LowerBound
	= 0;
	psPortRangeHints[XSHAPER_AMOUNT].UpperBound
	= 1;
	
	psPortRangeHints[XSHAPER_LFO1FORM].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_INTEGER
	| LADSPA_HINT_DEFAULT_1);
	psPortRangeHints[XSHAPER_LFO1FORM].LowerBound
	= 1;
	psPortRangeHints[XSHAPER_LFO1FORM].UpperBound
	= XSHAPER_LFOTYPES;
	
	psPortRangeHints[XSHAPER_LFO1RATE].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_DEFAULT_1);
	psPortRangeHints[XSHAPER_LFO1RATE].LowerBound
	= 0;
	psPortRangeHints[XSHAPER_LFO1RATE].UpperBound
	= 10;
	
	psPortRangeHints[XSHAPER_LFO1DEPTH].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_DEFAULT_0);
	psPortRangeHints[XSHAPER_LFO1DEPTH].LowerBound
	= -1;
	psPortRangeHints[XSHAPER_LFO1DEPTH].UpperBound
	= 1;
	
	psPortRangeHints[XSHAPER_LFO2FORM].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_INTEGER
	| LADSPA_HINT_DEFAULT_1);
	psPortRangeHints[XSHAPER_LFO2FORM].LowerBound
	= 1;
	psPortRangeHints[XSHAPER_LFO2FORM].UpperBound
	= XSHAPER_LFOTYPES;
	
	psPortRangeHints[XSHAPER_LFO2RATE].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_DEFAULT_1);
	psPortRangeHints[XSHAPER_LFO2RATE].LowerBound
	= 0;
	psPortRangeHints[XSHAPER_LFO2RATE].UpperBound
	= 10;
	
	psPortRangeHints[XSHAPER_LFO2DEPTH].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_DEFAULT_0);
	psPortRangeHints[XSHAPER_LFO2DEPTH].LowerBound
	= -1;
	psPortRangeHints[XSHAPER_LFO2DEPTH].UpperBound
	= 1;
	 
	psPortRangeHints[XSHAPER_INPUT1].HintDescriptor
	 = 0;
    psPortRangeHints[XSHAPER_OUTPUT1].HintDescriptor
	 = 0;
	  
    g_psMonoDescriptor->instantiate 
      = instantiateXShaper;
    g_psMonoDescriptor->connect_port 
      = connectPortToXShaper;
    g_psMonoDescriptor->activate
      = NULL;
    g_psMonoDescriptor->run
      = runMonoXShaper;
    g_psMonoDescriptor->run_adding
      = NULL;
    g_psMonoDescriptor->set_run_adding_gain
      = NULL;
    g_psMonoDescriptor->deactivate
      = NULL;
    g_psMonoDescriptor->cleanup
      = cleanupXShaper;
  }
  
  if (g_psStereoDescriptor) {
    
    g_psStereoDescriptor->UniqueID
      = 2548;
    g_psStereoDescriptor->Label
      = strdup("XShaperS");
    g_psStereoDescriptor->Properties
      = LADSPA_PROPERTY_HARD_RT_CAPABLE;
    g_psStereoDescriptor->Name 
      = strdup("X-Shaper (stereo)");
    g_psStereoDescriptor->Maker
      = strdup("Artemiy Pavlov");
    g_psStereoDescriptor->Copyright
      = strdup("(c)2005 GPL");
    g_psStereoDescriptor->PortCount
      = 14;
    piPortDescriptors
      = (LADSPA_PortDescriptor *)calloc(14, sizeof(LADSPA_PortDescriptor));
    g_psStereoDescriptor->PortDescriptors
      = (const LADSPA_PortDescriptor *)piPortDescriptors;
	 
	piPortDescriptors[XSHAPER_GAIN]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_TYPE]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_CURVE]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_AMOUNT]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_LFO1FORM]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_LFO1RATE]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_LFO1DEPTH]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_LFO2FORM]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_LFO2RATE]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_LFO2DEPTH]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[XSHAPER_INPUT1]
	= LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	piPortDescriptors[XSHAPER_OUTPUT1]
	= LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	piPortDescriptors[XSHAPER_INPUT2]
	= LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	piPortDescriptors[XSHAPER_OUTPUT2]
	= LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	  
	pcPortNames
	= (char **)calloc(14, sizeof(char *));
	g_psStereoDescriptor->PortNames 
	= (const char **)pcPortNames;
	
	pcPortNames[XSHAPER_GAIN]
	= strdup("Gain");
	pcPortNames[XSHAPER_TYPE]
	= strdup("Type");
	pcPortNames[XSHAPER_CURVE]
	= strdup("Curve");
	pcPortNames[XSHAPER_AMOUNT]
	= strdup("Amount");
	pcPortNames[XSHAPER_LFO1FORM]
	= strdup("LFO1 Form");
	pcPortNames[XSHAPER_LFO1RATE]
	= strdup("LFO1 Rate");
	pcPortNames[XSHAPER_LFO1DEPTH]
	= strdup("LFO1 Gain Depth");
	pcPortNames[XSHAPER_LFO2FORM]
	= strdup("LFO2 Form");
	pcPortNames[XSHAPER_LFO2RATE]
	= strdup("LFO2 Rate");
	pcPortNames[XSHAPER_LFO2DEPTH]
	= strdup("LFO2 Curve Depth");
	pcPortNames[XSHAPER_INPUT1]
	= strdup("Input L");
	pcPortNames[XSHAPER_OUTPUT1]
	= strdup("Output L");
	pcPortNames[XSHAPER_INPUT2]
	= strdup("Input R");
	pcPortNames[XSHAPER_OUTPUT2]
	= strdup("Output R");
	  
    psPortRangeHints = ((LADSPA_PortRangeHint *)
			calloc(14, sizeof(LADSPA_PortRangeHint)));
    g_psStereoDescriptor->PortRangeHints
      = (const LADSPA_PortRangeHint *)psPortRangeHints;
	
	psPortRangeHints[XSHAPER_GAIN].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_DEFAULT_1);
	psPortRangeHints[XSHAPER_GAIN].LowerBound
	= 0;
	psPortRangeHints[XSHAPER_GAIN].UpperBound
	= 1;
	  
   psPortRangeHints[XSHAPER_TYPE].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_INTEGER
	| LADSPA_HINT_DEFAULT_1);
	psPortRangeHints[XSHAPER_TYPE].LowerBound
	= 1;
	psPortRangeHints[XSHAPER_TYPE].UpperBound
	= XSHAPER_TYPES;
	 
	psPortRangeHints[XSHAPER_CURVE].HintDescriptor
      = (LADSPA_HINT_BOUNDED_BELOW 
	 | LADSPA_HINT_BOUNDED_ABOVE
	 | LADSPA_HINT_DEFAULT_0);    
	psPortRangeHints[XSHAPER_CURVE].LowerBound
	 = 0;
	psPortRangeHints[XSHAPER_CURVE].UpperBound
	 = 1;
	 
	psPortRangeHints[XSHAPER_AMOUNT].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_DEFAULT_1);
	psPortRangeHints[XSHAPER_AMOUNT].LowerBound
	= 0;
	psPortRangeHints[XSHAPER_AMOUNT].UpperBound
	= 1;
	
	psPortRangeHints[XSHAPER_LFO1FORM].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_INTEGER
	| LADSPA_HINT_DEFAULT_1);
	psPortRangeHints[XSHAPER_LFO1FORM].LowerBound
	= 1;
	psPortRangeHints[XSHAPER_LFO1FORM].UpperBound
	= XSHAPER_LFOTYPES;
	
	psPortRangeHints[XSHAPER_LFO1RATE].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_DEFAULT_1);
	psPortRangeHints[XSHAPER_LFO1RATE].LowerBound
	= 0;
	psPortRangeHints[XSHAPER_LFO1RATE].UpperBound
	= 10;
	
	psPortRangeHints[XSHAPER_LFO1DEPTH].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_DEFAULT_0);
	psPortRangeHints[XSHAPER_LFO1DEPTH].LowerBound
	= -1;
	psPortRangeHints[XSHAPER_LFO1DEPTH].UpperBound
	= 1;
	
	psPortRangeHints[XSHAPER_LFO2FORM].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_INTEGER
	| LADSPA_HINT_DEFAULT_1);
	psPortRangeHints[XSHAPER_LFO2FORM].LowerBound
	= 1;
	psPortRangeHints[XSHAPER_LFO2FORM].UpperBound
	= XSHAPER_LFOTYPES;
	
	psPortRangeHints[XSHAPER_LFO2RATE].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_DEFAULT_1);
	psPortRangeHints[XSHAPER_LFO2RATE].LowerBound
	= 0;
	psPortRangeHints[XSHAPER_LFO2RATE].UpperBound
	= 10;
	
	psPortRangeHints[XSHAPER_LFO2DEPTH].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_DEFAULT_0);
	psPortRangeHints[XSHAPER_LFO2DEPTH].LowerBound
	= -1;
	psPortRangeHints[XSHAPER_LFO2DEPTH].UpperBound
	= 1;
	  
	psPortRangeHints[XSHAPER_INPUT1].HintDescriptor
	 = 0;
    psPortRangeHints[XSHAPER_OUTPUT1].HintDescriptor
	 = 0;
    psPortRangeHints[XSHAPER_INPUT2].HintDescriptor
	 = 0;
    psPortRangeHints[XSHAPER_OUTPUT2].HintDescriptor
	 = 0;
	  
    g_psStereoDescriptor->instantiate 
      = instantiateXShaper;
    g_psStereoDescriptor->connect_port 
      = connectPortToXShaper;
    g_psStereoDescriptor->activate
      = NULL;
    g_psStereoDescriptor->run
      = runStereoXShaper;
    g_psStereoDescriptor->run_adding
      = NULL;
    g_psStereoDescriptor->set_run_adding_gain
      = NULL;
    g_psStereoDescriptor->deactivate
      = NULL;
    g_psStereoDescriptor->cleanup
      = cleanupXShaper;
  }
}

void deleteDescriptor(LADSPA_Descriptor * psDescriptor) {
  unsigned long lIndex;
  if (psDescriptor) {
    free((char *)psDescriptor->Label);
    free((char *)psDescriptor->Name);
    free((char *)psDescriptor->Maker);
    free((char *)psDescriptor->Copyright);
    free((LADSPA_PortDescriptor *)psDescriptor->PortDescriptors);
    for (lIndex = 0; lIndex < psDescriptor->PortCount; lIndex++)
      free((char *)(psDescriptor->PortNames[lIndex]));
    free((char **)psDescriptor->PortNames);
    free((LADSPA_PortRangeHint *)psDescriptor->PortRangeHints);
    free(psDescriptor);
  }
}

void _fini() {
  deleteDescriptor(g_psMonoDescriptor);
  deleteDescriptor(g_psStereoDescriptor);
}


#ifdef WIN32
	#define _DLL_EXPORT_ __declspec(dllexport)
	int bIsFirstTime = 1; 
	void _init(); // forward declaration
#else
	#define _DLL_EXPORT_ 
#endif



_DLL_EXPORT_ const LADSPA_Descriptor * ladspa_descriptor(unsigned long Index) {
#ifdef WIN32
	if (bIsFirstTime) {
		_init();
		bIsFirstTime = 0;
	}
#endif
  switch (Index) {
  case 0:
    return g_psMonoDescriptor;
  case 1:
    return g_psStereoDescriptor;
  default:
    return NULL;
  }
}

