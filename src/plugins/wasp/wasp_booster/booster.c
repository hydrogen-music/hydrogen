/*

booster.c

Clipping booster (mono/stereo)

(c)2005 Artemiy Pavlov

*/

/* Includes: */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ladspa.h"

/* The port numbers for the plugin: */

#define BOOSTER_CURVE 0
#define BOOSTER_GAIN 1
#define BOOSTER_CLIP 2
#define BOOSTER_INPUT1  3
#define BOOSTER_OUTPUT1 4
#define BOOSTER_INPUT2 5
#define BOOSTER_OUTPUT2 6

#define MAX_GAIN 36

/* The structure used to hold port connection information and state  */

typedef struct {

	LADSPA_Data * m_pfCurve;
	LADSPA_Data * m_pfGain;
	LADSPA_Data * m_pfClip;
	LADSPA_Data * m_pfInputBuffer1;
	LADSPA_Data * m_pfOutputBuffer1;
	LADSPA_Data * m_pfInputBuffer2;
	LADSPA_Data * m_pfOutputBuffer2;

} Booster;

/* Construct a new plugin instance. */

LADSPA_Handle instantiateBooster(const LADSPA_Descriptor * Descriptor, unsigned long SampleRate) {

	return malloc(sizeof(Booster));
	
}

/* Connect a port to a data location. */

void connectPortToBooster(LADSPA_Handle Instance, unsigned long Port, LADSPA_Data * DataLocation) {

	Booster * psBooster;

	psBooster = (Booster *)Instance;
	
	switch (Port) {
		case BOOSTER_GAIN:
			psBooster->m_pfGain = DataLocation;
    		break;
		case BOOSTER_CLIP:
			psBooster->m_pfClip = DataLocation;
    		break;
		case BOOSTER_CURVE:
    		psBooster->m_pfCurve = DataLocation;
    		break;
  		case BOOSTER_INPUT1:
    		psBooster->m_pfInputBuffer1 = DataLocation;
    		break;
  		case BOOSTER_OUTPUT1:
    		psBooster->m_pfOutputBuffer1 = DataLocation;
    		break;
  		case BOOSTER_INPUT2:
    		psBooster->m_pfInputBuffer2 = DataLocation;
    		break;
  		case BOOSTER_OUTPUT2:
		    psBooster->m_pfOutputBuffer2 = DataLocation;
    		break;
  }
  
}

/* Booster mono */

void runMonoBooster(LADSPA_Handle Instance, unsigned long SampleCount) {
  
	LADSPA_Data * pfInput;
	LADSPA_Data * pfOutput;
	LADSPA_Data fGain;
	LADSPA_Data fClip;
	LADSPA_Data fCurve;
	LADSPA_Data inputData;
	LADSPA_Data outputData;
	LADSPA_Data boosterGain;
	LADSPA_Data asymmetryPositive;
	LADSPA_Data asymmetryNegative;
	
	Booster * psBooster;
	unsigned long lSampleIndex;

	psBooster = (Booster *)Instance;
	
	fGain = *(psBooster->m_pfGain);
	fClip = *(psBooster->m_pfClip);
	fCurve = *(psBooster->m_pfCurve);
	
	boosterGain = powf( 10, fGain/20 );
	fCurve = fCurve * 1.5f + 1;
		
	if( boosterGain < 0)
		boosterGain = 0;
		
	asymmetryPositive = 1.0f;
	asymmetryNegative = -1.0f;

	asymmetryPositive = asymmetryPositive * fClip;
	asymmetryNegative = asymmetryNegative * fClip;
	
	pfInput = psBooster->m_pfInputBuffer1;
	pfOutput = psBooster->m_pfOutputBuffer1;
	
	for(lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++){	
	
		inputData = *(pfInput++);
		
		if( inputData < 0 ){
			outputData = -inputData;
		} else {
			outputData = inputData;
		}
		
		outputData =  powf ( 1 - powf(1 - outputData , fCurve), 1/fCurve ) * boosterGain;
		
		if (outputData > asymmetryPositive) {
 			outputData = asymmetryPositive;
 		}
		  
		if( inputData >= 0 ){
			*(pfOutput++) = outputData ;
		} else {
			*(pfOutput++) = -outputData ;
		}
		
	}
	
}

/* Booster stereo */

void runStereoBooster(LADSPA_Handle Instance, unsigned long SampleCount) {
  
	LADSPA_Data * pfInput;
	LADSPA_Data * pfOutput;
	LADSPA_Data fGain;
	LADSPA_Data fClip;
	LADSPA_Data fCurve;
	LADSPA_Data inputData;
	LADSPA_Data outputData;
	LADSPA_Data boosterGain;
	LADSPA_Data asymmetryPositive;
	LADSPA_Data asymmetryNegative;
	
	Booster * psBooster;
	unsigned long lSampleIndex;

	psBooster = (Booster *)Instance;

	fGain = *(psBooster->m_pfGain);
	fClip = *(psBooster->m_pfClip);
	fCurve = *(psBooster->m_pfCurve);
	
	boosterGain = powf( 10, fGain/20 );
	fCurve = fCurve * 1.5f + 1;
	
	if( boosterGain < 0)
		boosterGain = 0;
		
	asymmetryPositive = 1.0f;
	asymmetryNegative = -1.0f;
		
	asymmetryPositive = asymmetryPositive * fClip;
	asymmetryNegative = asymmetryNegative * fClip;
	
	pfInput = psBooster->m_pfInputBuffer1;
	pfOutput = psBooster->m_pfOutputBuffer1;
  
	for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++){
		
		inputData = *(pfInput++);
		
		if( inputData < 0 ){
			outputData = -inputData;
		} else {
			outputData = inputData;
		}
		
		outputData =  powf ( 1 - powf(1 - outputData , fCurve), 1/fCurve ) * boosterGain;
		
		if (outputData > asymmetryPositive) {
 			outputData = asymmetryPositive;
 		}
		  
		if( inputData >= 0 ){
			*(pfOutput++) = outputData ;
		} else {
			*(pfOutput++) = -outputData ;
		}
		
	}

	pfInput = psBooster->m_pfInputBuffer2;
	pfOutput = psBooster->m_pfOutputBuffer2;
  
	for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++){
	
    	inputData = *(pfInput++);
		
		if( inputData < 0 ){
			outputData = -inputData;
		} else {
			outputData = inputData;
		}
		
		outputData =  powf ( 1 - powf(1 - outputData , fCurve), 1/fCurve ) * boosterGain;
		
		if (outputData > asymmetryPositive) {
 			outputData = asymmetryPositive;
 		}
		  
		if( inputData >= 0 ){
			*(pfOutput++) = outputData ;
		} else {
			*(pfOutput++) = -outputData ;
		}
		
	}
}

/* Booster cleanup */

void cleanupBooster(LADSPA_Handle Instance) {

	Booster * psBooster;
	psBooster = (Booster *)Instance;

	free(psBooster);
}

LADSPA_Descriptor * g_psMonoDescriptor = NULL;
LADSPA_Descriptor * g_psStereoDescriptor = NULL;

/* Booster _init() function */

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
      = 2545;
    g_psMonoDescriptor->Label
      = strdup("BoosterM");
    g_psMonoDescriptor->Properties
      = LADSPA_PROPERTY_HARD_RT_CAPABLE;
    g_psMonoDescriptor->Name 
      = strdup("Clipping Booster (mono)");
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
	  
    piPortDescriptors[BOOSTER_CURVE]
      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
    piPortDescriptors[BOOSTER_GAIN]
      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[BOOSTER_CLIP]
      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
    piPortDescriptors[BOOSTER_INPUT1]
      = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors[BOOSTER_OUTPUT1]
      = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	  
    pcPortNames
      = (char **)calloc(5, sizeof(char *));
	g_psMonoDescriptor->PortNames 
      = (const char **)pcPortNames;
	  
    pcPortNames[BOOSTER_CURVE]
      = strdup("Curve");
	pcPortNames[BOOSTER_GAIN]
      = strdup("Gain (dB)");
	 pcPortNames[BOOSTER_CLIP]
      = strdup("Clip");
	pcPortNames[BOOSTER_INPUT1]
      = strdup("Input");
    pcPortNames[BOOSTER_OUTPUT1]
      = strdup("Output");
	  
    psPortRangeHints = ((LADSPA_PortRangeHint *)
			calloc(5, sizeof(LADSPA_PortRangeHint)));
    g_psMonoDescriptor->PortRangeHints
      = (const LADSPA_PortRangeHint *)psPortRangeHints;
	
	psPortRangeHints[BOOSTER_CURVE].HintDescriptor
      = (LADSPA_HINT_BOUNDED_BELOW 
	 | LADSPA_HINT_BOUNDED_ABOVE
	 | LADSPA_HINT_DEFAULT_0);
	 
    psPortRangeHints[BOOSTER_CURVE].LowerBound
	 = 0;
	psPortRangeHints[BOOSTER_CURVE].UpperBound
	 = 1; 
	  
    psPortRangeHints[BOOSTER_GAIN].HintDescriptor
      = (LADSPA_HINT_BOUNDED_BELOW 
	 | LADSPA_HINT_BOUNDED_ABOVE
	 | LADSPA_HINT_INTEGER
	 | LADSPA_HINT_DEFAULT_0);
    psPortRangeHints[BOOSTER_GAIN].LowerBound
	 = 0;
	psPortRangeHints[BOOSTER_GAIN].UpperBound
	 = MAX_GAIN;
	 
	 psPortRangeHints[BOOSTER_CLIP].HintDescriptor
      = (LADSPA_HINT_BOUNDED_BELOW 
	 | LADSPA_HINT_BOUNDED_ABOVE
	 | LADSPA_HINT_DEFAULT_1);
    psPortRangeHints[BOOSTER_CLIP].LowerBound
	 = 0;
	psPortRangeHints[BOOSTER_CLIP].UpperBound
	 = 1;
	  
	psPortRangeHints[BOOSTER_INPUT1].HintDescriptor
	 = 0;
    psPortRangeHints[BOOSTER_OUTPUT1].HintDescriptor
	 = 0;
	  
    g_psMonoDescriptor->instantiate 
      = instantiateBooster;
    g_psMonoDescriptor->connect_port 
      = connectPortToBooster;
    g_psMonoDescriptor->activate
      = NULL;
    g_psMonoDescriptor->run
      = runMonoBooster;
    g_psMonoDescriptor->run_adding
      = NULL;
    g_psMonoDescriptor->set_run_adding_gain
      = NULL;
    g_psMonoDescriptor->deactivate
      = NULL;
    g_psMonoDescriptor->cleanup
      = cleanupBooster;
  }
  
  if (g_psStereoDescriptor) {
    
    g_psStereoDescriptor->UniqueID
      = 2546;
    g_psStereoDescriptor->Label
      = strdup("BoosterS");
    g_psStereoDescriptor->Properties
      = LADSPA_PROPERTY_HARD_RT_CAPABLE;
    g_psStereoDescriptor->Name 
      = strdup("Clipping Booster (stereo)");
    g_psStereoDescriptor->Maker
      = strdup("Artemiy Pavlov");
    g_psStereoDescriptor->Copyright
      = strdup("(c)2005 GPL");
    g_psStereoDescriptor->PortCount
      = 7;
    piPortDescriptors
      = (LADSPA_PortDescriptor *)calloc(7, sizeof(LADSPA_PortDescriptor));
    g_psStereoDescriptor->PortDescriptors
      = (const LADSPA_PortDescriptor *)piPortDescriptors;
    piPortDescriptors[BOOSTER_CURVE]
      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[BOOSTER_GAIN]
      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[BOOSTER_CLIP]
      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
    piPortDescriptors[BOOSTER_INPUT1]
      = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors[BOOSTER_OUTPUT1]
      = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors[BOOSTER_INPUT2]
      = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors[BOOSTER_OUTPUT2]
      = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	  
    pcPortNames
      = (char **)calloc(7, sizeof(char *));
    g_psStereoDescriptor->PortNames 
      = (const char **)pcPortNames;
	  
    pcPortNames[BOOSTER_CURVE]
      = strdup("Curve");
	pcPortNames[BOOSTER_GAIN]
      = strdup("Gain (dB)");
	pcPortNames[BOOSTER_CLIP]
      = strdup("Clip");
    pcPortNames[BOOSTER_INPUT1]
      = strdup("Input L");
    pcPortNames[BOOSTER_OUTPUT1]
      = strdup("Output L");
    pcPortNames[BOOSTER_INPUT2]
      = strdup("Input R");
    pcPortNames[BOOSTER_OUTPUT2]
      = strdup("Output R");
	  
    psPortRangeHints = ((LADSPA_PortRangeHint *)
			calloc(7, sizeof(LADSPA_PortRangeHint)));
    g_psStereoDescriptor->PortRangeHints
      = (const LADSPA_PortRangeHint *)psPortRangeHints;
	  
    
	psPortRangeHints[BOOSTER_CURVE].HintDescriptor
      = (LADSPA_HINT_BOUNDED_BELOW 
	 | LADSPA_HINT_BOUNDED_ABOVE
	 | LADSPA_HINT_DEFAULT_0);
	 
    psPortRangeHints[BOOSTER_CURVE].LowerBound
	 = 0;
	psPortRangeHints[BOOSTER_CURVE].UpperBound
	 = 1; 
	
	psPortRangeHints[BOOSTER_GAIN].HintDescriptor
      = (LADSPA_HINT_BOUNDED_BELOW 
	 | LADSPA_HINT_BOUNDED_ABOVE
	 | LADSPA_HINT_INTEGER
	 | LADSPA_HINT_DEFAULT_0);
    
	psPortRangeHints[BOOSTER_GAIN].LowerBound
	 = 0;
	psPortRangeHints[BOOSTER_GAIN].UpperBound
	 = MAX_GAIN;
	  
	psPortRangeHints[BOOSTER_CLIP].HintDescriptor
      = (LADSPA_HINT_BOUNDED_BELOW 
	 | LADSPA_HINT_BOUNDED_ABOVE
	 | LADSPA_HINT_DEFAULT_1);
    psPortRangeHints[BOOSTER_CLIP].LowerBound
	 = 0;
	psPortRangeHints[BOOSTER_CLIP].UpperBound
	 = 1;
	 
	psPortRangeHints[BOOSTER_INPUT1].HintDescriptor
	 = 0;
    psPortRangeHints[BOOSTER_OUTPUT1].HintDescriptor
	 = 0;
    psPortRangeHints[BOOSTER_INPUT2].HintDescriptor
	 = 0;
    psPortRangeHints[BOOSTER_OUTPUT2].HintDescriptor
	 = 0;
	  
    g_psStereoDescriptor->instantiate 
      = instantiateBooster;
    g_psStereoDescriptor->connect_port 
      = connectPortToBooster;
    g_psStereoDescriptor->activate
      = NULL;
    g_psStereoDescriptor->run
      = runStereoBooster;
    g_psStereoDescriptor->run_adding
      = NULL;
    g_psStereoDescriptor->set_run_adding_gain
      = NULL;
    g_psStereoDescriptor->deactivate
      = NULL;
    g_psStereoDescriptor->cleanup
      = cleanupBooster;
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
