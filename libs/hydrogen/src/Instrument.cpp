/*
 * Hydrogen
 * Copyright(c) 2002-2006 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <hydrogen/Instrument.h>
#include <hydrogen/adsr.h>
#include <hydrogen/Sample.h>
#include <hydrogen/Song.h>
#include <hydrogen/LocalFileMng.h>
#include <hydrogen/SoundLibrary.h>

#include <cassert>

namespace H2Core {

Instrument::Instrument(
		const string& sId,
		const string& sName,
		float fVolume,
		bool bMuted,
		float fPan_L,
		float fPan_R,
		const string& sDrumkitName
)
 : Object( "Instrument" )
 , m_pADSR( NULL )
 , m_sId( sId )
 , m_sName( sName )
 , m_bFilterActive( false )
 , m_fCutoff( 1.0 )
 , m_fResonance( 0.0 )
 , m_fRandomPitchFactor( 0.0 )
 , m_bActive( true )
 , m_fVolume( fVolume )
 , m_bIsMuted( bMuted )
 , m_bIsSoloed( false )
 , m_fPeak_L( 0.0 )
 , m_fPeak_R( 0.0 )
 , m_fPan_L( fPan_L )
 , m_fPan_R( fPan_R )
 , m_sDrumkitName( sDrumkitName )
 , m_fGain( 1.0 )
 , m_nMuteGroup( -1 )
{
	for (unsigned nFX = 0; nFX < MAX_FX; ++nFX) {
		m_fFXLevel[ nFX ] = 0.0;
	}

	for (unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer) {
		m_layers[ nLayer ] = NULL;
	}
}



Instrument::~Instrument()
{
	for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer ) {
		delete m_layers[ nLayer ];
		m_layers[ nLayer ] = NULL;
	}
	delete m_pADSR;
	m_pADSR = NULL;
}



InstrumentLayer* Instrument::getLayer( int nLayer )
{
	if (nLayer < 0 ) {
		ERRORLOG( "nLayer < 0 (nLayer=" + toString(nLayer) + ")" );
		return NULL;
	}
	if (nLayer >= MAX_LAYERS ) {
		ERRORLOG( "nLayer > MAX_LAYERS (nLayer=" + toString(nLayer) + ")" );
		return NULL;
	}

	return m_layers[ nLayer ];
}



void Instrument::setLayer( InstrumentLayer* pLayer, unsigned nLayer )
{
	if (nLayer < MAX_LAYERS) {
		m_layers[ nLayer ] = pLayer;
	}
	else {
		ERRORLOG( "nLayer > MAX_LAYER" );
	}
}



Instrument* Instrument::loadInstrument( const std::string& sDrumkitName, const std::string& sInstrumentName )
{
	Instrument *pInstrument = NULL;
	LocalFileMng mgr;

	// find the drumkit
	Drumkit *pDrumkitInfo = mgr.loadDrumkit( mgr.getDrumkitDirectory( sDrumkitName ) + sDrumkitName );
	assert( pDrumkitInfo );

	InstrumentList *pInstrList = pDrumkitInfo->getInstrumentList();
	for ( unsigned nInstr = 0; nInstr < pInstrList->getSize(); ++nInstr ) {
		Instrument *pInstr = pInstrList->get( nInstr );
		if ( pInstr->m_sName == sInstrumentName) {
			// creo un nuovo strumento
			pInstrument = new Instrument();

			// copio tutte le proprieta' dello strumento
			pInstrument->m_sName = pInstr->m_sName;
			pInstrument->m_fPan_L = pInstr->m_fPan_L;
			pInstrument->m_fPan_R = pInstr->m_fPan_R;
			pInstrument->m_fVolume = pInstr->m_fVolume;
			pInstrument->m_sDrumkitName = pInstr->m_sDrumkitName;
			pInstrument->m_bIsMuted = pInstr->m_bIsMuted;
			pInstrument->m_fRandomPitchFactor = pInstr->m_fRandomPitchFactor;
			pInstrument->m_pADSR = new ADSR( *( pInstr->m_pADSR ) );
			pInstrument->m_bFilterActive = pInstr->m_bFilterActive;
			pInstrument->m_fCutoff = pInstr->m_fCutoff;
			pInstrument->m_fResonance = pInstr->m_fResonance;
			pInstrument->m_nMuteGroup = pInstr->m_nMuteGroup;

			for ( int nLayer = 0; nLayer < MAX_LAYERS; ++nLayer ) {
				InstrumentLayer *pOrigLayer = pInstr->getLayer( nLayer );
				if ( pOrigLayer ) {
					string sDrumkitPath = mgr.getDrumkitDirectory( sDrumkitName );
					string sSampleFilename = sDrumkitPath + sDrumkitName + "/" + pOrigLayer->m_pSample->m_sFilename;
					Sample* pSample = Sample::load( sSampleFilename );
					InstrumentLayer *pLayer = new InstrumentLayer( pSample );
					pLayer->m_fStartVelocity = pOrigLayer->m_fStartVelocity;
					pLayer->m_fEndVelocity = pOrigLayer->m_fEndVelocity;
					pLayer->m_fGain = pOrigLayer->m_fGain;
					pInstrument->setLayer( pLayer, nLayer );
				}
				else {
					pInstrument->setLayer( NULL, nLayer );
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
		ERRORLOG( "pos > list.size(). pos = " + toString(pos) );
		return NULL;
	}
/*	else if ( pos < 0 ) {
		ERRORLOG( "pos < 0. pos = " + toString(pos) );
		return NULL;
	}*/
	return m_list[pos];
}



/// Returns index of instrument in list, if instrument not found, returns -1
int InstrumentList::getPos( Instrument *pInstr )
{
	if ( m_posmap.find( pInstr ) == m_posmap.end() )
		return -1;
	return m_posmap[ pInstr ];
}



unsigned int InstrumentList::getSize()
{
	return m_list.size();
}


void InstrumentList::replace( Instrument* pNewInstr, unsigned nPos )
{
	if ( nPos >= m_list.size() ) {
		ERRORLOG( "Instrument index out of bounds in InstrumentList::replace. pos >= list.size() - " + toString( nPos ) + " > " + toString( m_list.size() ) );
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




InstrumentLayer::InstrumentLayer( Sample *pSample )
 : Object( "InstrumentLayer" )
 , m_fStartVelocity( 0.0 )
 , m_fEndVelocity( 1.0 )
 , m_fPitch( 0.0 )
 , m_fGain( 1.0 )
 , m_pSample( pSample )
{
	//infoLog( "INIT" );
}



InstrumentLayer::~InstrumentLayer()
{
	delete m_pSample;
	//infoLog( "DESTROY" );
}

};


