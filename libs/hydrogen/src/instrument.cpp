/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <hydrogen/instrument.h>

#include <hydrogen/adsr.h>
#include <hydrogen/sample.h>
#include <hydrogen/Song.h>
#include <hydrogen/drumkit.h>
#include <hydrogen/filesystem.h>
#include <hydrogen/audio_engine.h>

#include <cassert>

namespace H2Core
{

const char* Instrument::__class_name = "Instrument";

Instrument::Instrument( const QString& id, const QString& name, ADSR* adsr )
		: Object( __class_name )
		, __queued( 0 )
		, __adsr( adsr )
		, __muted( false )
		, __name( name )
		, __pan_l( 1.0 )
		, __pan_r( 1.0 )
		, __gain( 1.0 )
		, __volume( 1.0 )
		, __filter_resonance( 0.0 )
		, __filter_cutoff( 1.0 )
		, __peak_l( 0.0 )
		, __peak_r( 0.0 )
		, __random_pitch_factor( 0.0 )
		, __id( id )
		, __drumkit_name( "" )
		, __filter_active( false )
		, __mute_group( -1 )
		, __active( true )
		, __soloed( false )
		, __midi_out_channel( -1 )
		, __midi_out_note( 60 )
		, __stop_notes( false )
{
	for ( unsigned nFX = 0; nFX < MAX_FX; ++nFX ) {
		__fx_level[ nFX ] = 0.0;
	}

	for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer ) {
		__layer_list[ nLayer ] = NULL;
	}
}


Instrument::Instrument( Instrument *other ) : Object( __class_name )
		, __queued( 0 )
		, __adsr( new ADSR( *( other->get_adsr() ) ) )
		, __muted( other->is_muted() )
		, __name( other->get_name() )
		, __pan_l( other->get_pan_l() )
		, __pan_r( other->get_pan_r() )
		, __gain( other->__gain )
		, __volume( other->get_volume() )
		, __filter_resonance( other->get_filter_resonance() )
		, __filter_cutoff( other->get_filter_cutoff() )
		, __peak_l( 0.0 )
		, __peak_r( 0.0 )
		, __random_pitch_factor( other->get_random_pitch_factor() )
		, __id( other->get_id() )
		, __drumkit_name( "" )
		, __filter_active( other->is_filter_active() )
		, __mute_group( other->get_mute_group() )
		, __active( true )
		, __soloed( false )
		, __midi_out_channel( -1 )
		, __midi_out_note( 60 )
		, __stop_notes( false )
{
	for ( unsigned nFX = 0; nFX < MAX_FX; ++nFX ) {
		__fx_level[ nFX ] = 0.0;
	}

	for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer ) {
		InstrumentLayer *other_layer = other->get_layer( nLayer );
		if ( other_layer ) {
		    __layer_list[ nLayer ] = new InstrumentLayer( other_layer );
        } else {
		    __layer_list[ nLayer ] = NULL;
        }
	}
}

Instrument::~Instrument()
{
	for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer ) {
		delete __layer_list[ nLayer ];
		__layer_list[ nLayer ] = NULL;
	}
	delete __adsr;
	__adsr = NULL;
}

Instrument* Instrument::load_from( XMLNode *node ) {
    QString id = node->read_string( "id", "" );
    if ( id.isEmpty() ) return 0;
    Instrument *instrument = new Instrument( id, node->read_string( "name", "" ), 0 );
    instrument->set_volume( node->read_float( "volume", 1.0f ) );
    instrument->set_muted( node->read_bool( "isMuted", false ) );
    instrument->set_pan_l( node->read_float( "pan_L", 1.0f ) );
    instrument->set_pan_r( node->read_float( "pan_R", 1.0f ) );
    // may not exist, but can't be empty
    instrument->set_filter_active( node->read_bool( "filterActive", true, false ) );
    instrument->set_filter_cutoff( node->read_float( "filterCutoff", 1.0f, true, false ) );
    instrument->set_filter_resonance( node->read_float( "filterResonance", 0.0f, true, false ) );
    instrument->set_random_pitch_factor( node->read_float( "randomPitchFactor", 0.0f, true, false ) );
    float fAttack = node->read_float( "Attack", 0, true, false );
    float fDecay = node->read_float( "Decay", 0, true, false  );
    float fSustain = node->read_float( "Sustain", 1.0, true, false );
    float fRelease = node->read_float( "Release", 1000, true, false );
    instrument->set_adsr( new ADSR( fAttack, fDecay, fSustain, fRelease ) );
    instrument->set_gain( node->read_float( "gain", 1.0f, true, false ) );
    instrument->set_mute_group( node->read_int( "muteGroup", -1, true, false ) );
    instrument->set_midi_out_channel( node->read_int( "midiOutChannel", -1, true, false ) );
    instrument->set_midi_out_note( node->read_int( "midiOutNote", 60, true, false ) );
    instrument->set_stop_note( node->read_bool( "isStopNote", true ,false ) );

    QDomNode filename_node = node->firstChildElement( "filename" );
    if ( !filename_node.isNull() ) {
        // back compatibility code
        // TODO should be automaticaly rewritten
        DEBUGLOG( "Using back compatibility code. filename node found" );
        QString sFilename = node->read_string( "filename", "");
        if( sFilename.isEmpty() ) {
            ERRORLOG( "filename back compability node is empty" );
        } else {
            Sample *sample = new Sample( 0, sFilename, 0 );
            InstrumentLayer *layer = new InstrumentLayer( sample );
            instrument->set_layer( layer, 0 );
        }
    } else {
        int n = 0;
        XMLNode layer_node = node->firstChildElement( "layer" );
        while ( !layer_node.isNull() ) {
            if ( n >= MAX_LAYERS ) {
                ERRORLOG( QString("n >= MAX_LAYERS (%1)").arg(MAX_LAYERS) );
                break;
            }
            instrument->set_layer( InstrumentLayer::load_from( &layer_node ), n );
            n++;
            layer_node = layer_node.nextSiblingElement( "layer" );
        }
    }
    return instrument;
}


void Instrument::set_layer( InstrumentLayer* pLayer, unsigned nLayer )
{
	if ( nLayer < MAX_LAYERS ) {
		__layer_list[ nLayer ] = pLayer;
	} else {
		ERRORLOG( "nLayer > MAX_LAYER" );
	}
}

void Instrument::set_adsr( ADSR* adsr )
{
	delete __adsr;
	__adsr = adsr;
}

void Instrument::load_from_placeholder( Instrument* placeholder, bool is_live )
{
	QString path = Filesystem::drumkit_path( placeholder->get_drumkit_name() )+ "/";
	for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer ) {
		InstrumentLayer *pNewLayer = placeholder->get_layer( nLayer );
		if ( pNewLayer != NULL ) {
			// this is a 'placeholder sample:
			Sample *pNewSample = pNewLayer->get_sample();
			
			// now we load the actal data:
			Sample *pSample = Sample::load( path + pNewSample->get_filename() );
			InstrumentLayer *pOldLayer = this->get_layer( nLayer );

			if ( pSample == NULL ) {
				_ERRORLOG( QString("Error loading sample %1. Creating a new empty layer.").arg(path+pNewSample->get_filename() ) );
				if ( is_live )
					AudioEngine::get_instance()->lock( RIGHT_HERE );
				
				this->set_layer( NULL, nLayer );
				
				if ( is_live )
					AudioEngine::get_instance()->unlock();
				delete pOldLayer;
				continue;
			}
			InstrumentLayer *pLayer = new InstrumentLayer( pSample );
			pLayer->set_start_velocity( pNewLayer->get_start_velocity() );
			pLayer->set_end_velocity( pNewLayer->get_end_velocity() );
			pLayer->set_gain( pNewLayer->get_gain() );
			 pLayer->set_pitch(pNewLayer->get_pitch()); 

			if ( is_live )
				AudioEngine::get_instance()->lock( RIGHT_HERE );
			
			this->set_layer( pLayer, nLayer );	// set the new layer
			
			if ( is_live )
				AudioEngine::get_instance()->unlock();
			delete pOldLayer;		// delete the old layer

		} else {
			InstrumentLayer *pOldLayer = this->get_layer( nLayer );
			if ( is_live )
				AudioEngine::get_instance()->lock( RIGHT_HERE );
			
			this->set_layer( NULL, nLayer );
			
			if ( is_live )
				AudioEngine::get_instance()->unlock();
			delete pOldLayer;		// delete the old layer
		}

	}
	if ( is_live )
		AudioEngine::get_instance()->lock( RIGHT_HERE );
	
	// update instrument properties
	this->set_gain( placeholder->get_gain() );
	this->set_id( placeholder->get_id() );
	this->set_name( placeholder->get_name() );
	this->set_pan_l( placeholder->get_pan_l() );
	this->set_pan_r( placeholder->get_pan_r() );
	this->set_volume( placeholder->get_volume() );
	this->set_drumkit_name( placeholder->get_drumkit_name() );
	this->set_muted( placeholder->is_muted() );
	this->set_random_pitch_factor( placeholder->get_random_pitch_factor() );
	this->set_adsr( new ADSR( *( placeholder->get_adsr() ) ) );
	this->set_filter_active( placeholder->is_filter_active() );
	this->set_filter_cutoff( placeholder->get_filter_cutoff() );
	this->set_filter_resonance( placeholder->get_filter_resonance() );
	this->set_mute_group( placeholder->get_mute_group() );
	
	if ( is_live )
		AudioEngine::get_instance()->unlock();
}

Instrument * Instrument::create_empty()
{
	return new Instrument( "", "Empty Instrument", new ADSR() );
}

Instrument * Instrument::load_instrument(
    const QString& drumkit_name,
    const QString& instrument_name
)
{
	Instrument * I = create_empty();
	I->load_from_name( drumkit_name, instrument_name, false );
	return I;
}

void Instrument::load_from_name(
    const QString& drumkit_name,
    const QString& instrument_name,
    bool is_live
)
{
	Instrument * pInstr = NULL;
	
	// find the drumkit
	 QString dir = Filesystem::drumkit_path( drumkit_name );
	if ( dir.isEmpty() ) return;
	Drumkit *pDrumkitInfo = Drumkit::load( dir );
	assert( pDrumkitInfo );

	// find the instrument
	InstrumentList *pInstrList = pDrumkitInfo->getInstrumentList();
	for ( unsigned nInstr = 0; nInstr < pInstrList->get_size(); ++nInstr ) {
		pInstr = pInstrList->get( nInstr );
		if ( pInstr->get_name() == instrument_name ) {
			break;
		}
	}
	
	if ( pInstr != NULL ) {
		load_from_placeholder( pInstr, is_live );
	}
	delete pDrumkitInfo;
}



// ::::

const char* InstrumentList::__class_name = "InstrumentList";

InstrumentList::InstrumentList()
		: Object( __class_name )
{
//	infoLog("INIT");
}

InstrumentList::InstrumentList( InstrumentList *other) : Object( __class_name ) {
    for ( int i=0; i<other->get_size(); i++ ) {
		add( new Instrument( other->get( i ) ) );
	}
}


InstrumentList::~InstrumentList()
{
//	infoLog("DESTROY");
	for ( unsigned int i = 0; i < m_list.size(); ++i ) {
		delete m_list[i];
	}
}

InstrumentList* InstrumentList::load_from( XMLNode *node ) {
    XMLNode instrument_node = node->firstChildElement( "instrument" );
    InstrumentList *instruments = new InstrumentList();
    int count = 0;
    while ( !instrument_node.isNull() ) {
        count++;
        if ( count > MAX_INSTRUMENTS ) {
            ERRORLOG( QString("instrument count >= %2, stop reading instruments").arg(MAX_INSTRUMENTS) );
            break;
        }
        Instrument* instrument = Instrument::load_from( &instrument_node );
        if(instrument) {
            instruments->add( instrument );
        } else {
            ERRORLOG( QString("Empty ID for instrument %1. The drumkit is corrupted. Skipping instrument").arg(count) );
            count--;
        }
        instrument_node = instrument_node.nextSiblingElement( "instrument" );
    }
    return instruments;
}

void InstrumentList::add( Instrument* newInstrument )
{
	m_list.push_back( newInstrument );
	m_posmap[newInstrument] = m_list.size() - 1;
}



Instrument* InstrumentList::get( unsigned int pos )
{
	if ( pos >= m_list.size() ) {
		ERRORLOG( QString( "pos > list.size(). pos = %1" ).arg( pos ) );
		return NULL;
	}
	/*	else if ( pos < 0 ) {
			ERRORLOG( "pos < 0. pos = " + to_string(pos) );
			return NULL;
		}*/
	return m_list[pos];
}



/// Returns index of instrument in list, if instrument not found, returns -1
int InstrumentList::get_pos( Instrument *pInstr )
{
	if ( m_posmap.find( pInstr ) == m_posmap.end() )
		return -1;
	return m_posmap[ pInstr ];
}



unsigned int InstrumentList::get_size()
{
	return m_list.size();
}


void InstrumentList::replace( Instrument* pNewInstr, unsigned nPos )
{
	if ( nPos >= m_list.size() ) {
		ERRORLOG( QString( "Instrument index out of bounds in InstrumentList::replace. pos >= list.size() - %1 > %2" ).arg( nPos ).arg( m_list.size() ) );
		return;
	}
	m_list.insert( m_list.begin() + nPos, pNewInstr );	// insert the new Instrument
	// remove the old Instrument
	m_list.erase( m_list.begin() + nPos + 1 );
}


void InstrumentList::del( int pos )
{
	assert( pos < ( int )m_list.size() );
	assert( pos >= 0 );
	m_list.erase( m_list.begin() + pos );
}


const char* InstrumentLayer::__class_name = "InstrumentLayer";

InstrumentLayer::InstrumentLayer( Sample *sample )
		: Object( __class_name )
		, __start_velocity( 0.0 )
		, __end_velocity( 1.0 )
		, __pitch( 0.0 )
		, __gain( 1.0 )
		, __sample( sample )
{
	//infoLog( "INIT" );
}


InstrumentLayer::InstrumentLayer( InstrumentLayer *other )
		: Object( __class_name )
		, __start_velocity( other->get_start_velocity() )
		, __end_velocity( other->get_end_velocity() )
		, __pitch( other->get_pitch() )
		, __gain( other->get_gain() )
		, __sample( new Sample( 0, other->get_sample()->get_filename(), 0 ) )       // is not a real sample, it contains only the filename information
{
}

InstrumentLayer* InstrumentLayer::load_from( XMLNode *node ) {
    Sample* sample = new Sample( 0, node->read_string( "filename", "" ), 0 );
    InstrumentLayer* layer = new InstrumentLayer( sample );
    layer->set_start_velocity( node->read_float( "min", 0.0 ) );
    layer->set_end_velocity( node->read_float( "max", 1.0 ) );
    layer->set_gain( node->read_float( "gain", 1.0, true, false ) );
    layer->set_pitch( node->read_float( "pitch", 0.0, true, false ) );
    return layer;
}

InstrumentLayer::~InstrumentLayer()
{
	delete __sample;
	__sample = NULL;
	//infoLog( "DESTROY" );
}

};


