/*
 * Hydrogen
 * Copyright(c) 2017 by Sebastian Moors
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

#include <hydrogen/core_action_controller.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/osc_server.h>
#include <hydrogen/midi_action.h>
#include <hydrogen/midi_map.h>

#include <hydrogen/IO/AlsaMidiDriver.h>
#include <hydrogen/IO/MidiOutput.h>

namespace H2Core
{

const char* CoreActionController::__class_name = "CoreActionController";


CoreActionController::CoreActionController() : Object( __class_name ) {
	//nothing
}

CoreActionController::~CoreActionController() {
	//nothing
}

void CoreActionController::setMasterVolume( float masterVolumeValue )
{
	Hydrogen* pEngine = Hydrogen::get_instance();
	pEngine->getSong()->set_volume( masterVolumeValue );
	
#ifdef H2CORE_HAVE_OSC
	Action* pFeedbackAction = new Action( "MASTER_VOLUME_ABSOLUTE" );
	pFeedbackAction->setParameter2( QString("%1").arg( masterVolumeValue ) );
	OscServer::handleAction( pFeedbackAction );
	delete pFeedbackAction;
#endif
	
	MidiOutput *pMidiDriver = pEngine->getMidiOutput();
	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionType( QString("MASTER_VOLUME_ABSOLUTE"));
	
	if( pMidiDriver && ccParamValue >= 0 ){
		pMidiDriver->handleOutgoingControlChange( ccParamValue, (masterVolumeValue / 1.5) * 127, 0);
	}
}

void CoreActionController::setStripVolume( int nStrip, float masterVolumeValue )
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	pEngine->setSelectedInstrumentNumber( nStrip );
	
	Song *pSong = pEngine->getSong();
	InstrumentList *instrList = pSong->get_instrument_list();

	Instrument *pInstr = instrList->get( nStrip );
	pInstr->set_volume( masterVolumeValue );
	
#ifdef H2CORE_HAVE_OSC
	Action* pFeedbackAction = new Action( "STRIP_VOLUME_ABSOLUTE" );
	
	pFeedbackAction->setParameter1( QString("%1").arg( nStrip + 1 ) );
	pFeedbackAction->setParameter2( QString("%1").arg( masterVolumeValue ) );
	OscServer::handleAction( pFeedbackAction );
	
	delete pFeedbackAction;
#endif

	MidiOutput *pMidiDriver = pEngine->getMidiOutput();
	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionParam1( QString("STRIP_VOLUME_ABSOLUTE"), QString("%1").arg( nStrip ) );
	
	if( pMidiDriver && ccParamValue >= 0 ){
		pMidiDriver->handleOutgoingControlChange( ccParamValue, (masterVolumeValue / 1.5) * 127, 0);
	}
}

void CoreActionController::setMetronomeIsActive( bool isActive ){
	Preferences::get_instance()->m_bUseMetronome = isActive;
	Hydrogen *pEngine = Hydrogen::get_instance();
	
#ifdef H2CORE_HAVE_OSC
	Action* pFeedbackAction = new Action( "TOGGLE_METRONOME" );
	
	pFeedbackAction->setParameter1( QString("%1").arg( (int) isActive ) );
	OscServer::handleAction( pFeedbackAction );
	
	delete pFeedbackAction;
#endif
	
	MidiOutput *pMidiDriver = pEngine->getMidiOutput();
	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionType( QString("TOGGLE_METRONOME"));
	
	if( pMidiDriver && ccParamValue >= 0 ){
		pMidiDriver->handleOutgoingControlChange( ccParamValue, (int) isActive * 127 , 0);
	}
}

void CoreActionController::setMasterIsMuted( bool isMuted ){
	Hydrogen *pEngine = Hydrogen::get_instance();
	pEngine->getSong()->__is_muted = isMuted;
	
#ifdef H2CORE_HAVE_OSC
	Action* pFeedbackAction = new Action( "MUTE_TOGGLE" );
	
	pFeedbackAction->setParameter1( QString("%1").arg( (int) isMuted ) );
	OscServer::handleAction( pFeedbackAction );
	
	delete pFeedbackAction;
#endif
	
	MidiOutput *pMidiDriver = pEngine->getMidiOutput();
	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionType( QString("MUTE_TOGGLE"));
	
	if( pMidiDriver && ccParamValue >= 0 ){
		pMidiDriver->handleOutgoingControlChange( ccParamValue, (int) isMuted * 127 , 0);
	}
}

void CoreActionController::setStripIsMuted( int nStrip, bool isMuted ){
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	InstrumentList *pInstrList = pSong->get_instrument_list();

	Instrument *pInstr = pInstrList->get( nStrip );
	pInstr->set_muted( isMuted );
	
#ifdef H2CORE_HAVE_OSC
	Action* pFeedbackAction = new Action( "STRIP_MUTE_TOGGLE" );
	
	pFeedbackAction->setParameter1( QString("%1").arg( nStrip + 1 ) );
	pFeedbackAction->setParameter2( QString("%1").arg( (int) isMuted ) );
	OscServer::handleAction( pFeedbackAction );
	
	delete pFeedbackAction;
#endif
	
	MidiOutput *pMidiDriver = pEngine->getMidiOutput();
	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionParam1( QString("STRIP_MUTE_TOGGLE"), QString("%1").arg( nStrip ) );
	
	if( pMidiDriver && ccParamValue >= 0 ){
		pMidiDriver->handleOutgoingControlChange( ccParamValue, ((int) isMuted) * 127, 0);
	}
}

void CoreActionController::setStripIsSoloed( int nStrip, bool isSoloed ){
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	InstrumentList *pInstrList = pSong->get_instrument_list();
	
	if ( isSoloed ) {
		for ( int i = 0; i < pInstrList->size(); ++i ) {
			setStripIsMuted( i, true );
		}

		setStripIsMuted( nStrip, false );
	} else {
		for ( int i = 0; i < pInstrList->size(); ++i ) {
			setStripIsMuted( i, false );
		}
	}
	
#ifdef H2CORE_HAVE_OSC
	Action* pFeedbackAction = new Action( "STRIP_SOLO_TOGGLE" );
	
	pFeedbackAction->setParameter1( QString("%1").arg( nStrip + 1 ) );
	pFeedbackAction->setParameter2( QString("%1").arg( (int) isSoloed ) );
	OscServer::handleAction( pFeedbackAction );
	
	delete pFeedbackAction;
#endif
	
	MidiOutput *pMidiDriver = pEngine->getMidiOutput();
	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionParam1( QString("STRIP_SOLO_TOGGLE"), QString("%1").arg( nStrip ) );
	
	if( pMidiDriver && ccParamValue >= 0 ){
		pMidiDriver->handleOutgoingControlChange( ccParamValue, ((int) isSoloed) * 127, 0);
	}
}



void CoreActionController::setStripPan( int nStrip, float panValue )
{
	float	pan_L;
	float	pan_R;

	if (panValue >= 0.5) {
		pan_L = (1.0 - panValue) * 2;
		pan_R = 1.0;
	}
	else {
		pan_L = 1.0;
		pan_R = panValue * 2;
	}

	Hydrogen *pEngine = Hydrogen::get_instance();
	pEngine->setSelectedInstrumentNumber( nStrip );
	
	
	Song *pSong = pEngine->getSong();
	InstrumentList *pInstrList = pSong->get_instrument_list();

	Instrument *pInstr = pInstrList->get( nStrip );
	pInstr->set_pan_l( pan_L ); 
	pInstr->set_pan_r( pan_R );

	pEngine->setSelectedInstrumentNumber( nStrip );
	
#ifdef H2CORE_HAVE_OSC
	Action* pFeedbackAction = new Action( "PAN_ABSOLUTE" );
	
	pFeedbackAction->setParameter1( QString("%1").arg( nStrip + 1 ) );
	pFeedbackAction->setParameter2( QString("%1").arg( panValue ) );
	OscServer::handleAction( pFeedbackAction );
	
	delete pFeedbackAction;
#endif
	
	MidiOutput *pMidiDriver = pEngine->getMidiOutput();
	MidiMap*	pMidiMap = MidiMap::get_instance();
	
	int ccParamValue = pMidiMap->findCCValueByActionParam1( QString("PAN_ABSOLUTE"), QString("%1").arg( nStrip ) );
	
	if( pMidiDriver && ccParamValue >= 0 ){
		pMidiDriver->handleOutgoingControlChange( ccParamValue, panValue * 127, 0);
	}
}

void CoreActionController::initExternalControlInterfaces()
{
	/*
	 * Push the current state of Hydrogen to the attached control interfaces (e.g. OSC clients)
	 */
	
	//MASTER_VOLUME_ABSOLUTE
	Hydrogen* pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	setMasterVolume( pSong->get_volume() );
	
	//PER-INSTRUMENT/STRIP STATES
	InstrumentList *instrList = pSong->get_instrument_list();
	for(int i=0; i < instrList->size(); i++){
		
			//STRIP_VOLUME_ABSOLUTE
			Instrument *pInstr = instrList->get( i );
			setStripVolume( i, pInstr->get_volume() );
			
			float fPan_L = pInstr->get_pan_l();
			float fPan_R = pInstr->get_pan_r();

			//PAN_ABSOLUTE
			float fPanValue = 0.0;
			if (fPan_R == 1.0) {
				fPanValue = 1.0 - (fPan_L / 2.0);
			}
			else {
				fPanValue = fPan_R / 2.0;
			}
		
			setStripPan( i, fPanValue );
			
			//STRIP_MUTE_TOGGLE
			setStripIsMuted( i, pInstr->is_muted() );
			
			//SOLO
			setStripIsSoloed( i, pInstr->is_soloed() );
	}
	
	//TOGGLE_METRONOME
	setMetronomeIsActive( Preferences::get_instance()->m_bUseMetronome );
	
	//MUTE_TOGGLE
	setMasterIsMuted( Hydrogen::get_instance()->getSong()->__is_muted );
}

}
