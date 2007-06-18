/*

noisifier.c

Noisifier (mono/stereo)

(c)2005 Artemiy Pavlov

*/

/* Includes: */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include "ladspa.h"

/* The port numbers for the plugin: */

#define NOISIFIER_TYPE 0
#define NOISIFIER_DENSITY 1
#define NOISIFIER_BALANCE 2
#define NOISIFIER_INPUT1  3
#define NOISIFIER_OUTPUT1 4
#define NOISIFIER_INPUT2 5
#define NOISIFIER_OUTPUT2 6

/* The structure used to hold port connection information and state  */

typedef struct {

	LADSPA_Data * m_pfType;
	LADSPA_Data * m_pfDensity;
	LADSPA_Data * m_pfBalance;
	LADSPA_Data * m_pfInputBuffer1;
	LADSPA_Data * m_pfOutputBuffer1;
	LADSPA_Data * m_pfInputBuffer2;
	LADSPA_Data * m_pfOutputBuffer2;
  
	int m_fStep;
	LADSPA_Data m_fNoise1;
	LADSPA_Data m_fNoise2;

} Noisifier;

/* Construct a new plugin instance. */

LADSPA_Handle instantiateNoisifier(const LADSPA_Descriptor * Descriptor, unsigned long SampleRate) {

	return malloc(sizeof(Noisifier));
	
}

/* Connect a port to a data location. */

void connectPortToNoisifier(LADSPA_Handle Instance, unsigned long Port, LADSPA_Data * DataLocation) {

	Noisifier * psNoisifier;

	psNoisifier = (Noisifier *)Instance;
	
	switch (Port) {
		case NOISIFIER_TYPE:
			psNoisifier->m_pfType = DataLocation;
    		break;
		case NOISIFIER_DENSITY:
			psNoisifier->m_pfDensity = DataLocation;
    		break;
		case NOISIFIER_BALANCE:
			psNoisifier->m_pfBalance = DataLocation;
    		break;
  		case NOISIFIER_INPUT1:
    		psNoisifier->m_pfInputBuffer1 = DataLocation;
    		break;
  		case NOISIFIER_OUTPUT1:
    		psNoisifier->m_pfOutputBuffer1 = DataLocation;
    		break;
  		case NOISIFIER_INPUT2:
    		psNoisifier->m_pfInputBuffer2 = DataLocation;
    		break;
  		case NOISIFIER_OUTPUT2:
		    psNoisifier->m_pfOutputBuffer2 = DataLocation;
    		break;
  }
  
}

/* Noisifier mono */

void runMonoNoisifier(LADSPA_Handle Instance, unsigned long SampleCount) {
  
	LADSPA_Data * pfInput;
	LADSPA_Data * pfOutput;
	LADSPA_Data fType;
	LADSPA_Data fDensity;
	LADSPA_Data fBalance;
	
	LADSPA_Data fNoise = 0;
	int fStep = 0;
	
	LADSPA_Data inputData;
	LADSPA_Data outputData;
	
	Noisifier * psNoisifier;
	unsigned long lSampleIndex;

	psNoisifier = (Noisifier *)Instance;
	
	fType = *(psNoisifier->m_pfType);
	fDensity = *(psNoisifier->m_pfDensity);
	fBalance = *(psNoisifier->m_pfBalance);
	
	if(fType >= 1.0f && fType < 2.0f){
	
		fType = 1;
		
		fDensity= 100.0f * (1 - fDensity);
		
	}
	
	if(fType >= 2.0f && fType < 3.0f){
	
		fType = 2;
		
		fDensity = 1.0f * pow ((1 - fDensity), 0.1f);
		
	}
	
	if(fType == 0){
	
		fStep = psNoisifier->m_fStep;
		fNoise = psNoisifier->m_fNoise1;
		
		if(fNoise == 0){
			fNoise = 2.0f * rand()/RAND_MAX - 1.0f;
		}
		
	}
	
	pfInput = psNoisifier->m_pfInputBuffer1;
	pfOutput = psNoisifier->m_pfOutputBuffer1;
		
	for(lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++){	
	
		inputData = *(pfInput++);
		
		if(fType == 1){
	
			fStep = fStep + 1;
			
			if(fStep >= fDensity){
				fNoise = 2.0f * rand()/RAND_MAX - 1.0f;
				fStep = 0;
			}
			
		}
		
		if(fType == 2){
	
			fNoise = 1.0f * rand()/RAND_MAX;
			
			if(fNoise >= fDensity){
				fNoise = 2.0f * rand()/RAND_MAX - 1.0f;
			} else {
				fNoise = 0.0f;
			}
			
		}
		
		outputData = inputData * fNoise * fBalance + inputData * ( 1 - fBalance );
		
		*(pfOutput++) = outputData;
	}
	
	if(fType == 1){
		psNoisifier->m_fStep = fStep;
		psNoisifier->m_fNoise1 = fNoise;
	}
	
}

/* Noisifier stereo */

void runStereoNoisifier(LADSPA_Handle Instance, unsigned long SampleCount) {
  
	LADSPA_Data * pfInput;
	LADSPA_Data * pfOutput;
	LADSPA_Data fType;
	LADSPA_Data fDensity;
	LADSPA_Data fBalance;
	
	LADSPA_Data fNoise = 0;
	int fStep = 0;
	
	LADSPA_Data inputData;
	LADSPA_Data outputData;
	
	Noisifier * psNoisifier;
	unsigned long lSampleIndex;

	psNoisifier = (Noisifier *)Instance;

	fType = *(psNoisifier->m_pfType);
	fDensity = *(psNoisifier->m_pfDensity);
	fBalance = *(psNoisifier->m_pfBalance);
	
	if(fType >= 1.0f && fType < 2.0f){
	
		fType = 1;
		
		fDensity= 100.0f * (1 - fDensity);
		
	}
	
	if(fType >= 2.0f && fType < 3.0f){
	
		fType = 2;
		
		fDensity = 1.0f * pow ((1 - fDensity), 0.1f);
		
	}
	
	if(fType == 0){
	
		fStep = psNoisifier->m_fStep;
		fNoise = psNoisifier->m_fNoise1;
		
		if(fNoise == 0){
			fNoise = 2.0f * rand()/RAND_MAX - 1.0f;
		}
		
	}
		
	pfInput = psNoisifier->m_pfInputBuffer1;
	pfOutput = psNoisifier->m_pfOutputBuffer1;
  
	for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++){
		
		inputData = *(pfInput++);
		
		if(fType == 1){
	
			fStep = fStep + 1;
			
			if(fStep >= fDensity){
				fNoise = 2.0f * rand()/RAND_MAX - 1.0f;
				fStep = 0;
			}
			
		}
		
		if(fType == 2){
	
			fNoise = 1.0f * rand()/RAND_MAX;
			
			if(fNoise >= fDensity){
				fNoise = 2.0f * rand()/RAND_MAX - 1.0f;
			} else {
				fNoise = 0.0f;
			}
			
		}
		
		outputData = inputData * fNoise * fBalance + inputData * ( 1 - fBalance );
		
		*(pfOutput++) = outputData;
		
	}

	if(fType == 1){
		psNoisifier->m_fNoise2 = fNoise;
	}
	
	if(fNoise==0){
		fNoise = 1.0f * rand()/RAND_MAX;
	}
	
	pfInput = psNoisifier->m_pfInputBuffer2;
	pfOutput = psNoisifier->m_pfOutputBuffer2;
  
	for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++){
	
		inputData = *(pfInput++);
		
		if(fType == 1){
	
			fStep = fStep + 1;
			
			if(fStep >= fDensity){
				fNoise = 2.0f * rand()/RAND_MAX - 1.0f;
				fStep = 0;
			}
			
		}
		
		if(fType == 2){
	
			fNoise = 1.0f * rand()/RAND_MAX;
			
			if(fNoise >= fDensity){
				fNoise = 2.0f * rand()/RAND_MAX - 1.0f;
			} else {
				fNoise = 0.0f;
			}
			
		}
		
		outputData = inputData * fNoise * fBalance + inputData * ( 1 - fBalance );
		
		*(pfOutput++) = outputData;
		
	}
	
	if(fType == 1){
		psNoisifier->m_fStep = fStep;
		psNoisifier->m_fNoise2 = fNoise;
	}
}

/* Noisifier cleanup */

void cleanupNoisifier(LADSPA_Handle Instance) {

	Noisifier * psNoisifier;
	psNoisifier = (Noisifier *)Instance;

	free(psNoisifier);
}

LADSPA_Descriptor * g_psMonoDescriptor = NULL;
LADSPA_Descriptor * g_psStereoDescriptor = NULL;

/* Noisifier _init() function */

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
      = 2543;
    g_psMonoDescriptor->Label
      = strdup("NoisifierM");
    g_psMonoDescriptor->Properties
      = LADSPA_PROPERTY_HARD_RT_CAPABLE;
    g_psMonoDescriptor->Name 
      = strdup("Noisifier (mono)");
    g_psMonoDescriptor->Maker
      = strdup("Artemiy Pavlov");
    g_psMonoDescriptor->Copyright
      = strdup("(c)2005 GPL");
    g_psMonoDescriptor->PortCount
      = 5;
    piPortDescriptors
      = (LADSPA_PortDescriptor *)calloc(5, sizeof(LADSPA_PortDescriptor));
    g_psMonoDescriptor->PortDescriptors
      = (const LADSPA_PortDescriptor *)piPortDescriptors;
	  
	piPortDescriptors[NOISIFIER_TYPE]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[NOISIFIER_DENSITY]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[NOISIFIER_BALANCE]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[NOISIFIER_INPUT1]
	= LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	piPortDescriptors[NOISIFIER_OUTPUT1]
	= LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	  
	pcPortNames
	= (char **)calloc(5, sizeof(char *));
	g_psMonoDescriptor->PortNames 
	= (const char **)pcPortNames;
	  
	pcPortNames[NOISIFIER_TYPE]
	= strdup("Noise Type");
	pcPortNames[NOISIFIER_DENSITY]
	= strdup("Noise Density");
	pcPortNames[NOISIFIER_BALANCE]
	= strdup("Balance");
	pcPortNames[NOISIFIER_INPUT1]
	= strdup("Input");
	pcPortNames[NOISIFIER_OUTPUT1]
	= strdup("Output");
	  
    psPortRangeHints = ((LADSPA_PortRangeHint *)
		calloc(5, sizeof(LADSPA_PortRangeHint)));
    g_psMonoDescriptor->PortRangeHints
	= (const LADSPA_PortRangeHint *)psPortRangeHints;
	  
	psPortRangeHints[NOISIFIER_TYPE].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_INTEGER
	| LADSPA_HINT_DEFAULT_1);
	psPortRangeHints[NOISIFIER_TYPE].LowerBound
	= 1;
	psPortRangeHints[NOISIFIER_TYPE].UpperBound
	= 2;
	  
	psPortRangeHints[NOISIFIER_DENSITY].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_DEFAULT_1);
	psPortRangeHints[NOISIFIER_DENSITY].LowerBound
	= 0;
	psPortRangeHints[NOISIFIER_DENSITY].UpperBound
	= 1;
	 
	psPortRangeHints[NOISIFIER_BALANCE].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_DEFAULT_0);
	psPortRangeHints[NOISIFIER_BALANCE].LowerBound
	= 0;
	psPortRangeHints[NOISIFIER_BALANCE].UpperBound
	= 1;
	 
	psPortRangeHints[NOISIFIER_INPUT1].HintDescriptor
	= 0;
	psPortRangeHints[NOISIFIER_OUTPUT1].HintDescriptor
	= 0;
	  
	g_psMonoDescriptor->instantiate 
	= instantiateNoisifier;
	g_psMonoDescriptor->connect_port 
	= connectPortToNoisifier;
	g_psMonoDescriptor->activate
	= NULL;
	g_psMonoDescriptor->run
	= runMonoNoisifier;
	g_psMonoDescriptor->run_adding
	= NULL;
	g_psMonoDescriptor->set_run_adding_gain
	= NULL;
	g_psMonoDescriptor->deactivate
	= NULL;
	g_psMonoDescriptor->cleanup
	= cleanupNoisifier;
  }
  
  if (g_psStereoDescriptor) {

	g_psStereoDescriptor->UniqueID
	= 2544;
	g_psStereoDescriptor->Label
	= strdup("NoisifierS");
	g_psStereoDescriptor->Properties
	= LADSPA_PROPERTY_HARD_RT_CAPABLE;
	g_psStereoDescriptor->Name 
	= strdup("Noisifier (stereo)");
	g_psStereoDescriptor->Maker
	= strdup("Artemiy Pavlov");
	g_psStereoDescriptor->Copyright
	= strdup("(c)2005 SineShine");
	g_psStereoDescriptor->PortCount
	= 7;
	piPortDescriptors
	= (LADSPA_PortDescriptor *)calloc(7, sizeof(LADSPA_PortDescriptor));
	g_psStereoDescriptor->PortDescriptors
	= (const LADSPA_PortDescriptor *)piPortDescriptors;
	  
	piPortDescriptors[NOISIFIER_TYPE]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[NOISIFIER_DENSITY]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[NOISIFIER_BALANCE]
	= LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[NOISIFIER_INPUT1]
	= LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	piPortDescriptors[NOISIFIER_OUTPUT1]
	= LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	piPortDescriptors[NOISIFIER_INPUT2]
	= LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	piPortDescriptors[NOISIFIER_OUTPUT2]
	= LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	  
    pcPortNames
      = (char **)calloc(7, sizeof(char *));
    g_psStereoDescriptor->PortNames 
      = (const char **)pcPortNames;
	  
    pcPortNames[NOISIFIER_TYPE]
      = strdup("Noise Type");
	pcPortNames[NOISIFIER_DENSITY]
      = strdup("Noise Density");
	pcPortNames[NOISIFIER_BALANCE]
      = strdup("Balance");
    pcPortNames[NOISIFIER_INPUT1]
      = strdup("Input L");
    pcPortNames[NOISIFIER_OUTPUT1]
      = strdup("Output L");
    pcPortNames[NOISIFIER_INPUT2]
      = strdup("Input R");
    pcPortNames[NOISIFIER_OUTPUT2]
      = strdup("Output R");
	  
    psPortRangeHints = ((LADSPA_PortRangeHint *) 
		calloc(7, sizeof(LADSPA_PortRangeHint)));
    g_psStereoDescriptor->PortRangeHints
	= (const LADSPA_PortRangeHint *)psPortRangeHints;
	  
	psPortRangeHints[NOISIFIER_TYPE].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_INTEGER
	| LADSPA_HINT_DEFAULT_1);
	psPortRangeHints[NOISIFIER_TYPE].LowerBound
	= 1;
	psPortRangeHints[NOISIFIER_TYPE].UpperBound
	= 2;
	  
	psPortRangeHints[NOISIFIER_DENSITY].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_DEFAULT_1);
	psPortRangeHints[NOISIFIER_DENSITY].LowerBound
	= 0;
	psPortRangeHints[NOISIFIER_DENSITY].UpperBound
	= 1;
	  
	psPortRangeHints[NOISIFIER_BALANCE].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	| LADSPA_HINT_BOUNDED_ABOVE
	| LADSPA_HINT_DEFAULT_0);
	psPortRangeHints[NOISIFIER_BALANCE].LowerBound
	= 0;
	psPortRangeHints[NOISIFIER_BALANCE].UpperBound
	= 1;
	  
	psPortRangeHints[NOISIFIER_INPUT1].HintDescriptor
	 = 0;
	psPortRangeHints[NOISIFIER_OUTPUT1].HintDescriptor
	= 0;
	psPortRangeHints[NOISIFIER_INPUT2].HintDescriptor
	= 0;
	psPortRangeHints[NOISIFIER_OUTPUT2].HintDescriptor
	= 0;
	  
	g_psStereoDescriptor->instantiate 
	= instantiateNoisifier;
	g_psStereoDescriptor->connect_port 
	= connectPortToNoisifier;
	g_psStereoDescriptor->activate
	= NULL;
	g_psStereoDescriptor->run
	= runStereoNoisifier;
	g_psStereoDescriptor->run_adding
	= NULL;
	g_psStereoDescriptor->set_run_adding_gain
	= NULL;
	g_psStereoDescriptor->deactivate
	= NULL;
	g_psStereoDescriptor->cleanup
	= cleanupNoisifier;
	
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
