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
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/osc_server.h>
#include <hydrogen/midi_action.h>


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
}

void CoreActionController::setMetronomeIsActive( bool isActive ){
	Preferences::get_instance()->m_bUseMetronome = isActive;
	
#ifdef H2CORE_HAVE_OSC
	Action* pFeedbackAction = new Action( "TOGGLE_METRONOME" );
	
	pFeedbackAction->setParameter1( QString("%1").arg( (int) isActive ) );
	OscServer::handleAction( pFeedbackAction );
	
	delete pFeedbackAction;
#endif
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
	
	//STRIP_VOLUME_ABSOLUTE
	InstrumentList *instrList = pSong->get_instrument_list();
	for(int i=0; i < instrList->size(); i++){
			Instrument *pInstr = instrList->get( i );
			setStripVolume( i, pInstr->get_volume() );
	}
	
	//TOGGLE_METRONOME
	setMetronomeIsActive( Preferences::get_instance()->m_bUseMetronome );
}

}
