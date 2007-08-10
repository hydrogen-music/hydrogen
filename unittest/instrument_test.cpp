#include <cppunit/TestCase.h>

#include "hydrogen/note.h"
#include "hydrogen/instrument.h"

using namespace H2Core;

class InstrumentTest : public CppUnit::TestCase
{
public:
	InstrumentTest()
			: CppUnit::TestCase( "InstrumentTest" )
	{}

	InstrumentLayer* instrument_layer_test()
	{
		Sample* sample = NULL;

		InstrumentLayer* layer = new InstrumentLayer(sample);

		float start_vel = 0;
		layer->set_start_velocity(start_vel);
		CPPUNIT_ASSERT(layer->get_start_velocity() == start_vel);

		float end_vel = 1.0;
		layer->set_end_velocity(end_vel);
		CPPUNIT_ASSERT(layer->get_end_velocity() == end_vel);

		float pitch = 0.7;
		layer->set_pitch(pitch);
		CPPUNIT_ASSERT(layer->get_pitch() == pitch);

		float gain = 0.4;
		layer->set_gain(gain);
		CPPUNIT_ASSERT(layer->get_gain() == gain);

		layer->set_sample(sample);
		CPPUNIT_ASSERT(layer->get_sample() == sample);

		return layer;
	}



	void instrument_test()
	{
		Instrument* instr = new Instrument(
				"dummy instrument",
				"dummy instrument",
				new ADSR()
		);
		instr->set_filter_active( false );
		instr->set_filter_cutoff( 0.4 );
		instr->set_filter_resonance( 0.9 );
		instr->set_random_pitch_factor(0.5);
		instr->set_active( true );
		instr->set_volume( 0.4 );
		instr->set_muted( false );
		instr->set_soloed( false );
		instr->set_peak_l( 0.2 );
		instr->set_peak_r( 0.4 );
		instr->set_pan_l( 0.3 );
		instr->set_pan_r( 0.2 );
		instr->set_drumkit_name( "dummy drumkit" );
		instr->set_gain( 0.4 );
		instr->set_mute_group( 1 );


		//Instrument* copy = new Instrument(instr);	// copy constructor




		delete instr;
	}


	void runTest()
	{
		InstrumentLayer* layer = instrument_layer_test();
		instrument_test();
	}
};
