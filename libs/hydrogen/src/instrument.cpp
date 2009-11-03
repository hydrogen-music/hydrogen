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
#include <hydrogen/LocalFileMng.h>
#include <hydrogen/SoundLibrary.h>
#include <hydrogen/audio_engine.h>

#include <cassert>

namespace H2Core
{


Instrument::Instrument( const QString& id, const QString& name, ADSR* adsr )
		: Object( "Instrument" )
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



Instrument::~Instrument()
{
	for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer ) {
		delete __layer_list[ nLayer ];
		__layer_list[ nLayer ] = NULL;
	}
	delete __adsr;
	__adsr = NULL;
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
	LocalFileMng mgr;
	QString path = mgr.getDrumkitDirectory( placeholder->get_drumkit_name() ) + placeholder->get_drumkit_name() + "/";
	for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer ) {
		InstrumentLayer *pNewLayer = placeholder->get_layer( nLayer );
		if ( pNewLayer != NULL ) {
			// this is a 'placeholder sample:
			Sample *pNewSample = pNewLayer->get_sample();
			
			// now we load the actal data:
			Sample *pSample = Sample::load( path + pNewSample->get_filename() );
			InstrumentLayer *pOldLayer = this->get_layer( nLayer );

			if ( pSample == NULL ) {
				_ERRORLOG( "Error loading sample. Creating a new empty layer." );
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
	
	LocalFileMng mgr;
	QString sDrumkitPath = mgr.getDrumkitDirectory( drumkit_name );

	// find the drumkit
	QString dir = mgr.getDrumkitDirectory( drumkit_name ) + drumkit_name;
	if ( QDir( dir ).exists() == false )
		return;
	Drumkit *pDrumkitInfo = mgr.loadDrumkit( dir );
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


InstrumentList::InstrumentList()
		: Object( "InstrumentList" )
{
//	infoLog("INIT");
}



InstrumentList::~InstrumentList()
{
//	infoLog("DESTROY");
	for ( unsigned int i = 0; i < m_list.size(); ++i ) {
		delete m_list[i];
	}
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




InstrumentLayer::InstrumentLayer( Sample *sample )
		: Object( "InstrumentLayer" )
		, __start_velocity( 0.0 )
		, __end_velocity( 1.0 )
		, __pitch( 0.0 )
		, __gain( 1.0 )
		, __sample( sample )
{
	//infoLog( "INIT" );
}



InstrumentLayer::~InstrumentLayer()
{
	delete __sample;
	__sample = NULL;
	//infoLog( "DESTROY" );
}

};


