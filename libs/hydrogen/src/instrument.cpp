/*
 * Hydrogen
 * Copyright(c) 2002-2007 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
#include <hydrogen/Sample.h>
#include <hydrogen/Song.h>
#include <hydrogen/LocalFileMng.h>
#include <hydrogen/SoundLibrary.h>

#include <cassert>

namespace H2Core {


Instrument::Instrument(
		const string& id,
		const string& name,
		ADSR* adsr
)
 : Object( "Instrument" )
 , __adsr( adsr )
 , __id( id )
 , __name( name )
 , __filter_active( false )
 , __filter_cutoff( 1.0 )
 , __filter_resonance( 0.0 )
 , __random_pitch_factor( 0.0 )
 , __active( true )
 , __volume( 1.0 )
 , __muted( false )
 , __soloed( false )
 , __peak_l( 0.0 )
 , __peak_r( 0.0 )
 , __pan_l( 1.0 )
 , __pan_r( 1.0 )
 , __drumkit_name( "" )
 , __gain( 1.0 )
 , __mute_group( -1 )
{
	for (unsigned nFX = 0; nFX < MAX_FX; ++nFX) {
		__fx_level[ nFX ] = 0.0;
	}

	for (unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer) {
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



InstrumentLayer* Instrument::get_layer( int nLayer )
{
	if (nLayer < 0 ) {
		ERRORLOG( "nLayer < 0 (nLayer=" + to_string(nLayer) + ")" );
		return NULL;
	}
	if (nLayer >= MAX_LAYERS ) {
		ERRORLOG( "nLayer > MAX_LAYERS (nLayer=" + to_string(nLayer) + ")" );
		return NULL;
	}

	return __layer_list[ nLayer ];
}



void Instrument::set_layer( InstrumentLayer* pLayer, unsigned nLayer )
{
	if (nLayer < MAX_LAYERS) {
		__layer_list[ nLayer ] = pLayer;
	}
	else {
		ERRORLOG( "nLayer > MAX_LAYER" );
	}
}

void Instrument::set_adsr( ADSR* adsr )
{
	delete __adsr;
	__adsr = adsr;
}


Instrument* Instrument::load_instrument(
		const std::string& drumkit_name,
		const std::string& instrument_name
)
{
	Instrument *pInstrument = NULL;
	LocalFileMng mgr;

	// find the drumkit
	Drumkit *pDrumkitInfo = mgr.loadDrumkit( mgr.getDrumkitDirectory( drumkit_name ) + drumkit_name );
	assert( pDrumkitInfo );

	InstrumentList *pInstrList = pDrumkitInfo->getInstrumentList();
	for ( unsigned nInstr = 0; nInstr < pInstrList->get_size(); ++nInstr ) {
		Instrument *pInstr = pInstrList->get( nInstr );
		if ( pInstr->get_name() == instrument_name) {
			// creo un nuovo strumento

			pInstrument = new Instrument("", "", new ADSR( *( pInstr->get_adsr() ) ));

			// copio tutte le proprieta' dello strumento
			pInstrument->set_name( pInstr->get_name());
			pInstrument->set_pan_l( pInstr->get_pan_l() );
			pInstrument->set_pan_r( pInstr->get_pan_r() );
			pInstrument->set_volume( pInstr->get_volume() );
			pInstrument->set_drumkit_name( pInstr->get_drumkit_name() );
			pInstrument->set_muted( pInstr->is_muted() );
			pInstrument->set_random_pitch_factor( pInstr->get_random_pitch_factor() );
			pInstrument->set_filter_active( pInstr->is_filter_active() );
			pInstrument->set_filter_cutoff( pInstr->get_filter_cutoff() );
			pInstrument->set_filter_resonance( pInstr->get_filter_resonance() );
			pInstrument->set_mute_group( pInstr->get_mute_group() );

			for ( int nLayer = 0; nLayer < MAX_LAYERS; ++nLayer ) {
				InstrumentLayer *pOrigLayer = pInstr->get_layer( nLayer );
				if ( pOrigLayer ) {
					string sDrumkitPath = mgr.getDrumkitDirectory( drumkit_name );
					string sSampleFilename = sDrumkitPath + drumkit_name + "/" + pOrigLayer->get_sample()->get_filename();
					Sample* pSample = Sample::load( sSampleFilename );
					InstrumentLayer *pLayer = new InstrumentLayer( pSample );
					pLayer->set_start_velocity( pOrigLayer->get_start_velocity() );
					pLayer->set_end_velocity( pOrigLayer->get_end_velocity() );
					pLayer->set_gain( pOrigLayer->get_gain() );
					pInstrument->set_layer( pLayer, nLayer );
				}
				else {
					pInstrument->set_layer( NULL, nLayer );
				}
			}
			break;
		}
	}

	delete pDrumkitInfo;

	return pInstrument;
}



// ::::


InstrumentList::InstrumentList()
 : Object( "InstrumentList" )
{
//	infoLog("INIT");
}



InstrumentList::~InstrumentList() {
//	infoLog("DESTROY");
	for (unsigned int i = 0; i < m_list.size(); ++i) {
		delete m_list[i];
	}
}



void InstrumentList::add(Instrument* newInstrument) {
	m_list.push_back(newInstrument);
	m_posmap[newInstrument] = m_list.size() - 1;
}



Instrument* InstrumentList::get(unsigned int pos)
{
	if ( pos >= m_list.size() ) {
		ERRORLOG( "pos > list.size(). pos = " + to_string(pos) );
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
		ERRORLOG( "Instrument index out of bounds in InstrumentList::replace. pos >= list.size() - " + to_string( nPos ) + " > " + to_string( m_list.size() ) );
		return;
	}
	m_list.insert( m_list.begin() + nPos, pNewInstr );	// insert the new Instrument
	// remove the old Instrument
	m_list.erase( m_list.begin() + nPos + 1 );
}


void InstrumentList::del( int pos )
{
	assert( pos < (int)m_list.size() );
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
	delete __sample; __sample = NULL;
	//infoLog( "DESTROY" );
}

};


