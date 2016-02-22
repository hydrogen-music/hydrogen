
#include <hydrogen/helpers/legacy.h>

#include <hydrogen/version.h>
#include <hydrogen/helpers/xml.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/basics/drumkit.h>
#include <hydrogen/basics/drumkit_component.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/pattern_list.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/instrument_layer.h>
#include <hydrogen/basics/sample.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/basics/adsr.h>

namespace H2Core {

const char* Legacy::__class_name = "Legacy";

Drumkit* Legacy::load_drumkit( const QString& dk_path ) {
	if ( version_older_than( 0, 9, 8 ) ) {
		ERRORLOG( QString( "this code should not be used anymore, it belongs to 0.9.6" ) );
	} else {
		ERRORLOG( QString( "loading drumkit with legacy code" ) );
	}
	XMLDoc doc;
	if( !doc.read( dk_path ) ) {
		return 0;
	}
	XMLNode root = doc.firstChildElement( "drumkit_info" );
	if ( root.isNull() ) {
		ERRORLOG( "drumkit_info node not found" );
		return 0;
	}
	QString drumkit_name = root.read_string( "name", "", false, false );
	if ( drumkit_name.isEmpty() ) {
		ERRORLOG( "Drumkit has no name, abort" );
		return 0;
	}
	Drumkit* drumkit = new Drumkit();
	drumkit->set_path( dk_path.left( dk_path.lastIndexOf( "/" ) ) );
	drumkit->set_name( drumkit_name );
	drumkit->set_author( root.read_string( "author", "undefined author" ) );
	drumkit->set_info( root.read_string( "info", "defaultInfo" ) );
	drumkit->set_license( root.read_string( "license", "undefined license" ) );
	XMLNode instruments_node = root.firstChildElement( "instrumentList" );
	if ( instruments_node.isNull() ) {
		WARNINGLOG( "instrumentList node not found" );
		drumkit->set_instruments( new InstrumentList() );
	} else {
		InstrumentList* instruments = new InstrumentList();
		XMLNode instrument_node = instruments_node.firstChildElement( "instrument" );
		int count = 0;
		while ( !instrument_node.isNull() ) {
			count++;
			if ( count > MAX_INSTRUMENTS ) {
				ERRORLOG( QString( "instrument count >= %2, stop reading instruments" ).arg( MAX_INSTRUMENTS ) );
				break;
			}
			Instrument* instrument = 0;
			int id = instrument_node.read_int( "id", EMPTY_INSTR_ID, false, false );
			if ( id!=EMPTY_INSTR_ID ) {
				instrument = new Instrument( id, instrument_node.read_string( "name", "" ), 0 );
				instrument->set_drumkit_name( drumkit_name );
				instrument->set_volume( instrument_node.read_float( "volume", 1.0f ) );
				instrument->set_muted( instrument_node.read_bool( "isMuted", false ) );
				instrument->set_pan_l( instrument_node.read_float( "pan_L", 1.0f ) );
				instrument->set_pan_r( instrument_node.read_float( "pan_R", 1.0f ) );
				// may not exist, but can't be empty
				instrument->set_filter_active( instrument_node.read_bool( "filterActive", true, false ) );
				instrument->set_filter_cutoff( instrument_node.read_float( "filterCutoff", 1.0f, true, false ) );
				instrument->set_filter_resonance( instrument_node.read_float( "filterResonance", 0.0f, true, false ) );
				instrument->set_random_pitch_factor( instrument_node.read_float( "randomPitchFactor", 0.0f, true, false ) );
				float attack = instrument_node.read_float( "Attack", 0.0f, true, false );
				float decay = instrument_node.read_float( "Decay", 0.0f, true, false  );
				float sustain = instrument_node.read_float( "Sustain", 1.0f, true, false );
				float release = instrument_node.read_float( "Release", 1000.0f, true, false );
				instrument->set_adsr( new ADSR( attack, decay, sustain, release ) );
				instrument->set_gain( instrument_node.read_float( "gain", 1.0f, true, false ) );
				instrument->set_mute_group( instrument_node.read_int( "muteGroup", -1, true, false ) );
				instrument->set_midi_out_channel( instrument_node.read_int( "midiOutChannel", -1, true, false ) );
				instrument->set_midi_out_note( instrument_node.read_int( "midiOutNote", MIDI_MIDDLE_C, true, false ) );
				instrument->set_stop_notes( instrument_node.read_bool( "isStopNote", true ,false ) );
				instrument->set_hihat_grp( instrument_node.read_int( "isHihat", -1, true ) );
				instrument->set_lower_cc( instrument_node.read_int( "lower_cc", 0, true ) );
				instrument->set_higher_cc( instrument_node.read_int( "higher_cc", 127, true ) );
				for ( int i=0; i<MAX_FX; i++ ) {
					instrument->set_fx_level( instrument_node.read_float( QString( "FX%1Level" ).arg( i+1 ), 0.0 ), i );
				}
				QDomNode filename_node = instrument_node.firstChildElement( "filename" );
				if ( !filename_node.isNull() ) {
					DEBUGLOG( "Using back compatibility code. filename node found" );
					QString sFilename = instrument_node.read_string( "filename", "" );
					if( sFilename.isEmpty() ) {
						ERRORLOG( "filename back compability node is empty" );
					} else {

						Sample* sample = new Sample( dk_path+"/"+sFilename );

						bool p_foundMainCompo = false;
						for (std::vector<DrumkitComponent*>::iterator it = drumkit->get_components()->begin() ; it != drumkit->get_components()->end(); ++it) {
                            DrumkitComponent* existing_compo = *it;
                            if( existing_compo->get_name().compare("Main") == 0) {
                                p_foundMainCompo = true;
                                break;
                            }
                        }

						if ( !p_foundMainCompo ) {
                            DrumkitComponent* dmCompo = new DrumkitComponent( 0, "Main" );
                            drumkit->get_components()->push_back(dmCompo);
                        }

                        InstrumentComponent* component = new InstrumentComponent( 0 );
                        InstrumentLayer* layer = new InstrumentLayer( sample );
						component->set_layer( layer, 0 );
						instrument->get_components()->push_back( component );

					}
				} else {
					int n = 0;
					bool p_foundMainCompo = false;
                    for (std::vector<DrumkitComponent*>::iterator it = drumkit->get_components()->begin() ; it != drumkit->get_components()->end(); ++it) {
                        DrumkitComponent* existing_compo = *it;
                        if( existing_compo->get_name().compare("Main") == 0) {
                            p_foundMainCompo = true;
                            break;
                        }
                    }

                    if ( !p_foundMainCompo ) {
                        DrumkitComponent* dmCompo = new DrumkitComponent( 0, "Main" );
                        drumkit->get_components()->push_back(dmCompo);
                    }
					InstrumentComponent* component = new InstrumentComponent( 0 );

					XMLNode layer_node = instrument_node.firstChildElement( "layer" );
					while ( !layer_node.isNull() ) {
						if ( n >= MAX_LAYERS ) {
							ERRORLOG( QString( "n >= MAX_LAYERS (%1)" ).arg( MAX_LAYERS ) );
							break;
						}
						Sample* sample = new Sample( dk_path+"/"+layer_node.read_string( "filename", "" ) );
						InstrumentLayer* layer = new InstrumentLayer( sample );
						layer->set_start_velocity( layer_node.read_float( "min", 0.0 ) );
						layer->set_end_velocity( layer_node.read_float( "max", 1.0 ) );
						layer->set_gain( layer_node.read_float( "gain", 1.0, true, false ) );
						layer->set_pitch( layer_node.read_float( "pitch", 0.0, true, false ) );
						component->set_layer( layer, n );
						n++;
						layer_node = layer_node.nextSiblingElement( "layer" );
					}
					instrument->get_components()->push_back( component );
				}
			}
			if( instrument ) {
				( *instruments ) << instrument;
			} else {
				ERRORLOG( QString( "Empty ID for instrument %1. The drumkit is corrupted. Skipping instrument" ).arg( count ) );
				count--;
			}
			instrument_node = instrument_node.nextSiblingElement( "instrument" );
		}
		drumkit->set_instruments( instruments );
	}
	return drumkit;
}

Pattern* Legacy::load_drumkit_pattern( const QString& pattern_path ) {
	ERRORLOG( "NOT IMPLEMENTED YET !!!" );
	return 0;
}

};

/* vim: set softtabstop=4 expandtab: */
