/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include <QRegularExpression>

#include "core/Helpers/Filesystem.h"
#include "core/Preferences/Preferences.h"

#include <pthread.h>
#include <unistd.h>

//currently H2CORE_HAVE_OSC means: liblo is present..
#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_

#include <lo/lo.h>
#include <lo/lo_cpp.h>

#include "core/Basics/InstrumentList.h"
#include "core/OscServer.h"
#include "core/CoreActionController.h"
#include "core/EventQueue.h"
#include "core/Hydrogen.h"
#include "core/AudioEngine/AudioEngine.h"
#include "core/Basics/Song.h"
#include "core/MidiAction.h"
#include "core/IO/MidiCommon.h"

OscServer * OscServer::__instance = nullptr;


QString OscServer::qPrettyPrint(lo_type type,void * data)
{
	QString formattedString;

	typedef union {
	     int32_t  i;
	     float    f;
	     char     c;
	     uint32_t nl;
	 } h2_pcast32;

	typedef union {
		int64_t    i;
		double     f;
		uint64_t   nl;
	} h2_pcast64;


	h2_pcast32 val32 = {0};
	h2_pcast64 val64 = {0};
	int size;

	size = lo_arg_size(type, data);
	if (size == 4 || type == LO_BLOB) {
			val32.nl = *(int32_t *)data;
	} else if (size == 8) {
			val64.nl = *(int64_t *)data;
	} else {
		//error case
		formattedString = QString("Unhandled size: %1").arg(size);
		
		return formattedString;
	}

	switch (type) {
		case LO_INT32:
			formattedString = QString("%1").arg(val32.i);
			break;

		case LO_FLOAT:
			formattedString = QString("%1").arg(val32.f);
			break;

		case LO_STRING:
			formattedString = QString("%1").arg( (char *) data );
			break;

		case LO_BLOB:
			//not supported by Hydrogen
			formattedString = QString("BLOB");
			break;

		case LO_INT64:
			formattedString = QString("%1").arg(val64.i);
			break;

		case LO_DOUBLE:
			formattedString = QString("%1").arg(val64.f);
			break;

		case LO_SYMBOL:
			formattedString = QString("%1").arg( (char *) data );
			break;

		case LO_CHAR:
			formattedString = QString("%1").arg( QLatin1Char((char) val32.c ));
			break;

		case LO_MIDI:
			//not supported by Hydrogen
			formattedString = QString("MIDI");
			break;

		case LO_TRUE:
			formattedString = QString("#T");
			break;

		case LO_FALSE:
			formattedString = QString("#F");
			break;

		case LO_NIL:
			formattedString = QString("#NIL");
			break;

		case LO_INFINITUM:
			formattedString = QString("#INF");
			break;
			
		case LO_TIMETAG:
		default:
			formattedString = QString("Unhandled type:").arg(type);
			break;
	}

	return formattedString;

}

/* catch any incoming messages and display them. returning 1 means that the
 * message has not been fully handled and the server should try other methods */
int OscServer::incomingMessageLogging(const char *	path,
									  const char *	types,
									  lo_arg **	argv,
									  int			argc,
									  lo_message	data,
									  void *		user_data) {

	QString sSummary = QString( "Incoming OSC Message for path [%1]" ).arg( path );
	for ( int ii = 0; ii < argc; ii++) {
		QString formattedArgument = qPrettyPrint( (lo_type)types[ii], argv[ii] );
		sSummary.append( QString( ", arg. %1: [%2, %3]" )
						 .arg( ii ).arg( types[ ii ] ).arg( formattedArgument ) );
	}

	INFOLOG( sSummary );

	return 1;
}

int OscServer::generic_handler(const char *	path,
							   const char *	types,
							   lo_arg **	argv,
							   int			argc,
							   lo_message	data,
							   void *		user_data)
{
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pController = pHydrogen->getCoreActionController();
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return 1;
	}

	bool bMessageProcessed = false;
	
	const int nNumberOfStrips = pSong->getInstrumentList()->size();
	
	//First we're trying to map TouchOSC messages from multi-fader widgets
	const QString oscPath( path );

	QRegularExpression rxStripVol(
		QRegularExpression::anchoredPattern(
			"/Hydrogen/STRIP_VOLUME_ABSOLUTE/(\\d+)" ) );
	rxStripVol.setPatternOptions(
		QRegularExpression::UseUnicodePropertiesOption );
	const auto rxStripVolMatch = rxStripVol.match( oscPath );
	if ( rxStripVolMatch.hasMatch() && argc == 1 ) {
		const int nStrip = rxStripVolMatch.captured( 1 ).toInt() -1;
		if ( nStrip > -1 && nStrip < nNumberOfStrips ) {
			STRIP_VOLUME_ABSOLUTE_Handler( nStrip , argv[0]->f );
			bMessageProcessed = true;
		}
		else {
			ERRORLOG( QString( "Provided strip number [%1] out of bound [%2,%3]" )
					  .arg( nStrip + 1 ).arg( 1 )
					  .arg( nNumberOfStrips  ) );
		}
	}

	QRegularExpression rxStripVolRel(
		QRegularExpression::anchoredPattern(
			"/Hydrogen/STRIP_VOLUME_RELATIVE/(\\d+)" ) );
	rxStripVolRel.setPatternOptions(
		QRegularExpression::UseUnicodePropertiesOption );
	const auto rxStripVolRelMatch = rxStripVolRel.match( oscPath );
	if ( rxStripVolRelMatch.hasMatch() && argc == 1 ) {
		const int nStrip = rxStripVolRelMatch.captured( 1 ).toInt() - 1;
		if ( nStrip > -1 && nStrip < nNumberOfStrips ) {
			STRIP_VOLUME_RELATIVE_Handler( QString::number( nStrip ),
										   QString::number( argv[0]->f, 'f', 0 ) );
			bMessageProcessed = true;
		}
		else {
			ERRORLOG( QString( "Provided strip number [%1] out of bound [%2,%3]" )
					  .arg( nStrip + 1 ).arg( 1 )
					  .arg( nNumberOfStrips ) );
		}
	}
	
	QRegularExpression rxStripPanAbs(
		QRegularExpression::anchoredPattern(
			"/Hydrogen/PAN_ABSOLUTE/(\\d+)" ) );
	rxStripPanAbs.setPatternOptions(
		QRegularExpression::UseUnicodePropertiesOption );
	const auto rxStripPanAbsMatch = rxStripPanAbs.match( oscPath );
	if ( rxStripPanAbsMatch.hasMatch() && argc == 1 ) {
		const int nStrip = rxStripPanAbsMatch.captured( 1 ).toInt() - 1;
		if ( nStrip > -1 && nStrip < nNumberOfStrips ) {
			INFOLOG( QString( "processing message as changing pan of strip [%1] in absolute numbers" )
					 .arg( nStrip ) );
			pController->setStripPan( nStrip, argv[0]->f, false );
			bMessageProcessed = true;
		}
		else {
			ERRORLOG( QString( "Provided strip number [%1] out of bound [%2,%3]" )
					  .arg( nStrip + 1 ).arg( 1 )
					  .arg( nNumberOfStrips ) );
		}
	}

	QRegularExpression rxStripPanAbsSym(
		QRegularExpression::anchoredPattern(
			"/Hydrogen/PAN_ABSOLUTE_SYM/(\\d+)" ) );
	rxStripPanAbsSym.setPatternOptions(
		QRegularExpression::UseUnicodePropertiesOption );
	const auto rxStripPanAbsSymMatch = rxStripPanAbsSym.match( oscPath );
	if ( rxStripPanAbsSymMatch.hasMatch() && argc == 1 ) {
		const int nStrip = rxStripPanAbsSymMatch.captured( 1 ).toInt() - 1;
		if ( nStrip > -1 && nStrip < nNumberOfStrips ) {
			INFOLOG( QString( "processing message as changing pan of strip [%1] in symmetric, absolute numbers" )
					 .arg( nStrip ) );
			pController->setStripPanSym( nStrip, argv[0]->f, false );
			bMessageProcessed = true;
		}
		else {
			ERRORLOG( QString( "Provided strip number [%1] out of bound [%2,%3]" )
					  .arg( nStrip + 1 ).arg( 1 )
					  .arg( nNumberOfStrips ) );
		}
	}
	
	QRegularExpression rxStripPanRel(
		QRegularExpression::anchoredPattern(
			"/Hydrogen/PAN_RELATIVE/(\\d+)" ) );
	rxStripPanRel.setPatternOptions(
		QRegularExpression::UseUnicodePropertiesOption );
	const auto rxStripPanRelMatch = rxStripPanRel.match( oscPath );
	if ( rxStripPanRelMatch.hasMatch() && argc == 1 ) {
		const int nStrip = rxStripPanRelMatch.captured( 1 ).toInt() - 1;
		if ( nStrip > -1 && nStrip < nNumberOfStrips ) {
			INFOLOG( QString( "processing message as changing pan of strip [%1] in relative numbers" )
					 .arg( nStrip ) );
			std::shared_ptr<Action> pAction = std::make_shared<Action>("PAN_RELATIVE");
			pAction->setParameter1( QString::number( nStrip ) );
			pAction->setValue( QString::number( argv[0]->f, 'f', 0 ) );
			MidiActionManager::get_instance()->handleAction( pAction );
			bMessageProcessed = true;
		}
		else {
			ERRORLOG( QString( "Provided strip number [%1] out of bound [%2,%3]" )
					  .arg( nStrip + 1 ).arg( 1 )
					  .arg( nNumberOfStrips ) );
		}
	}

	QRegularExpression rxStripFilterCutoffAbs(
		QRegularExpression::anchoredPattern(
			"/Hydrogen/FILTER_CUTOFF_LEVEL_ABSOLUTE/(\\d+)" ) );
	rxStripFilterCutoffAbs.setPatternOptions(
		QRegularExpression::UseUnicodePropertiesOption );
	const auto rxStripFilterCutoffAbsMatch =
		rxStripFilterCutoffAbs.match( oscPath );
	if ( rxStripFilterCutoffAbsMatch.hasMatch() && argc == 1 ) {
		const int nStrip = rxStripFilterCutoffAbsMatch.captured( 1 ).toInt() - 1;
		if ( nStrip > -1 && nStrip < nNumberOfStrips ) {
			FILTER_CUTOFF_LEVEL_ABSOLUTE_Handler(
				QString::number( nStrip ), QString::number( argv[0]->f, 'f', 0 ) );
			bMessageProcessed = true;
		}
		else {
			ERRORLOG( QString( "Provided strip number [%1] out of bound [%2,%3]" )
					  .arg( nStrip + 1 ).arg( 1 )
					  .arg( nNumberOfStrips ) );
		}
	}
	
	QRegularExpression rxStripMute(
		QRegularExpression::anchoredPattern(
			"/Hydrogen/STRIP_MUTE_TOGGLE/(\\d+)" ) );
	rxStripMute.setPatternOptions(
		QRegularExpression::UseUnicodePropertiesOption );
	const auto rxStripMuteMatch = rxStripMute.match( oscPath );
	if ( rxStripMuteMatch.hasMatch() && argc <= 1 ) {
		const int nStrip = rxStripMuteMatch.captured( 1 ).toInt() - 1;
		if ( nStrip > -1 && nStrip < nNumberOfStrips ) {
			INFOLOG( QString( "processing message as toggling mute of strip [%1]" )
					 .arg( nStrip ) );
			pController->toggleStripIsMuted( nStrip );
			bMessageProcessed = true;
		}
		else {
			ERRORLOG( QString( "Provided strip number [%1] out of bound [%2,%3]" )
					  .arg( nStrip + 1 ).arg( 1 )
					  .arg( nNumberOfStrips ) );
		}
	}
	
	QRegularExpression rxStripSolo(
		QRegularExpression::anchoredPattern(
			"/Hydrogen/STRIP_SOLO_TOGGLE/(\\d+)" ) );
	rxStripSolo.setPatternOptions(
		QRegularExpression::UseUnicodePropertiesOption );
	const auto rxStripSoloMatch = rxStripSolo.match( oscPath );
	if ( rxStripSoloMatch.hasMatch() && argc <= 1 ) {
		const int nStrip = rxStripSoloMatch.captured( 1 ).toInt() - 1;
		if ( nStrip > -1 && nStrip < nNumberOfStrips ) {
			INFOLOG( QString( "processing message as toggling solo of strip [%1]" )
					 .arg( nStrip ) );
			pController->toggleStripIsSoloed( nStrip );
			bMessageProcessed = true;
		}
		else {
			ERRORLOG( QString( "Provided strip number [%1] out of bound [%2,%3]" )
					  .arg( nStrip + 1 ).arg( 1 )
					  .arg( nNumberOfStrips ) );
		}
	}

	if ( ! bMessageProcessed ) {
		ERRORLOG( "No matching handler found" );
	}
	
	// Returning 1 means that the message has not been fully handled
	// and the server should try other methods.
	return 1;
}



OscServer::OscServer( H2Core::Preferences* pPreferences ) : m_bInitialized( false )
{
	m_pPreferences = pPreferences;
	
	if ( m_pPreferences->getOscServerEnabled() ) {
		int nPort;
		if ( m_pPreferences->m_nOscTemporaryPort != -1  ) {
			nPort = m_pPreferences->m_nOscTemporaryPort;
		} else {
			nPort = m_pPreferences->getOscServerPort();
		}
	
		m_pServerThread = new lo::ServerThread( nPort );
		
		// If there is already another service registered to the same
		// port, the OSC server is not valid an can not be started.
		if ( ! m_pServerThread->is_valid() ) {
			delete m_pServerThread;
			
			// Instead, let the liblo library choose a working
			// port on their own (nullptr argument).
			m_pServerThread = new lo::ServerThread( nullptr );
			
			const int nTmpPort = m_pServerThread->port();
			
			ERRORLOG( QString("Could not start OSC server on port %1, using port %2 instead.")
					  .arg( nPort ).arg( nTmpPort ) );

			m_pPreferences->m_nOscTemporaryPort = nTmpPort;
			
			H2Core::EventQueue::get_instance()->push_event(
				H2Core::EVENT_ERROR, H2Core::Hydrogen::OSC_CANNOT_CONNECT_TO_PORT );
		}
	}
	else {
		m_pServerThread = nullptr;
	}
}

OscServer::~OscServer(){

	for (std::list<lo_address>::iterator it=m_pClientRegistry.begin(); it != m_pClientRegistry.end(); ++it){
		lo_address_free( *it );
	}

	delete m_pServerThread;
	
	__instance = nullptr;
}

void OscServer::create_instance( H2Core::Preferences* pPreferences )
{
	if( __instance == nullptr ) {
		__instance = new OscServer( pPreferences );
	}
}

// -------------------------------------------------------------------
// Handler functions

void OscServer::PLAY_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("PLAY");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::PLAY_STOP_TOGGLE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("PLAY/STOP_TOGGLE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::PLAY_PAUSE_TOGGLE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("PLAY/PAUSE_TOGGLE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::STOP_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("STOP");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::PAUSE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("PAUSE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::RECORD_READY_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("RECORD_READY");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::RECORD_STROBE_TOGGLE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("RECORD/STROBE_TOGGLE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::RECORD_STROBE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("RECORD_STROBE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::RECORD_EXIT_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("RECORD_EXIT");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::MUTE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("MUTE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::UNMUTE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("UNMUTE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::MUTE_TOGGLE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("MUTE_TOGGLE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::NEXT_BAR_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>(">>_NEXT_BAR");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::PREVIOUS_BAR_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("<<_PREVIOUS_BAR");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::BPM_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getTempoSource() != H2Core::Hydrogen::Tempo::Song ) {
		H2Core::EventQueue::get_instance()->push_event(
			H2Core::EVENT_TEMPO_CHANGED, -1 );
		return;
	}
	auto pAudioEngine = pHydrogen->getAudioEngine();

	float fNewBpm = argv[0]->f;
	fNewBpm = std::clamp( fNewBpm, static_cast<float>(MIN_BPM),
						  static_cast<float>(MAX_BPM) );

	pAudioEngine->lock( RIGHT_HERE );
	pAudioEngine->setNextBpm( fNewBpm );
	pAudioEngine->unlock();
		
	pHydrogen->getSong()->setBpm( fNewBpm );

	pHydrogen->setIsModified( true );
	
	H2Core::EventQueue::get_instance()->push_event( H2Core::EVENT_TEMPO_CHANGED, -1 );
}

void OscServer::BPM_INCR_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("BPM_INCR");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();
	
	pAction->setParameter1( QString::number( argv[0]->f, 'f', 0 ));

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::BPM_DECR_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("BPM_DECR");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pAction->setParameter1( QString::number( argv[0]->f, 'f', 0 ));
	
	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::MASTER_VOLUME_ABSOLUTE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	H2Core::Hydrogen *pHydrogen = H2Core::Hydrogen::get_instance();
	H2Core::CoreActionController* pController = pHydrogen->getCoreActionController();

	// Null song handling done in MidiActionManager.
	pController->setMasterVolume( argv[0]->f );
}

void OscServer::MASTER_VOLUME_RELATIVE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("MASTER_VOLUME_RELATIVE");
	pAction->setValue( QString::number( argv[0]->f, 'f', 0 ));
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::STRIP_VOLUME_ABSOLUTE_Handler(int param1, float param2)
{
	INFOLOG( "processing message" );
	H2Core::Hydrogen *pHydrogen = H2Core::Hydrogen::get_instance();
	H2Core::CoreActionController* pController = pHydrogen->getCoreActionController();

	// Null song handling done in MidiActionManager.
	pController->setStripVolume( param1, param2, false );
}

void OscServer::STRIP_VOLUME_RELATIVE_Handler(QString param1, QString param2)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("STRIP_VOLUME_RELATIVE");
	pAction->setParameter1( param1 );
	pAction->setValue( param2 );
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::SELECT_NEXT_PATTERN_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("SELECT_NEXT_PATTERN");
	pAction->setParameter1(  QString::number( argv[0]->f, 'f', 0 ) );
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::SELECT_ONLY_NEXT_PATTERN_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("SELECT_ONLY_NEXT_PATTERN");
	pAction->setParameter1(  QString::number( argv[0]->f, 'f', 0 ) );
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::SELECT_AND_PLAY_PATTERN_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("SELECT_AND_PLAY_PATTERN");
	pAction->setParameter1(  QString::number( argv[0]->f, 'f', 0 ) );
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::FILTER_CUTOFF_LEVEL_ABSOLUTE_Handler(QString param1, QString param2)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("FILTER_CUTOFF_LEVEL_ABSOLUTE");
	pAction->setParameter1( param1 );
	pAction->setValue( param2 );
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}


void OscServer::INSTRUMENT_PITCH_Handler( lo_arg** argv, int )
{
	INFOLOG( "processing message" );

	H2Core::Hydrogen::get_instance()->getCoreActionController()->
		setInstrumentPitch( static_cast<int>( argv[0]->f ), argv[1]->f );
}

void OscServer::BEATCOUNTER_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("BEATCOUNTER");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::TAP_TEMPO_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("TAP_TEMPO");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::PLAYLIST_SONG_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("PLAYLIST_SONG");
	pAction->setParameter1(  QString::number( argv[0]->f, 'f', 0 ) );

	MidiActionManager* pActionManager = MidiActionManager::get_instance();	

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::PLAYLIST_NEXT_SONG_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("PLAYLIST_NEXT_SONG");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::PLAYLIST_PREV_SONG_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("PLAYLIST_PREV_SONG");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::TOGGLE_METRONOME_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("TOGGLE_METRONOME");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::SELECT_INSTRUMENT_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("SELECT_INSTRUMENT");
	pAction->setValue( QString::number( argv[0]->f, 'f', 0 ) );

	MidiActionManager* pActionManager = MidiActionManager::get_instance();	

	// Null song handling done in MidiActionManager.
	pActionManager->handleAction( pAction );
}

void OscServer::UNDO_ACTION_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("UNDO_ACTION");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// This one does also work the current song being nullptr.
	pActionManager->handleAction( pAction );
}

void OscServer::REDO_ACTION_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>("REDO_ACTION");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	// This one does also work the current song being nullptr.
	pActionManager->handleAction( pAction );
}

// -------------------------------------------------------------------
// Actions required for session management.

void OscServer::NEW_SONG_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pController = pHydrogen->getCoreActionController();
	pController->newSong( QString::fromUtf8( &argv[0]->s ) );
}

void OscServer::OPEN_SONG_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pController = pHydrogen->getCoreActionController();
	pController->openSong( QString::fromUtf8( &argv[0]->s ) );
}

void OscServer::SAVE_SONG_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	auto pController = pHydrogen->getCoreActionController();
	pController->saveSong();
}

void OscServer::SAVE_SONG_AS_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	auto pController = pHydrogen->getCoreActionController();
	pController->saveSongAs( QString::fromUtf8( &argv[0]->s ) );
}

void OscServer::SAVE_PREFERENCES_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}
	
	auto pController = pHydrogen->getCoreActionController();
	pController->savePreferences();
}

void OscServer::QUIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pController = pHydrogen->getCoreActionController();
	pController->quit();
}

// -------------------------------------------------------------------

void OscServer::TIMELINE_ACTIVATION_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	auto pController = pHydrogen->getCoreActionController();

	if ( argv[0]->f != 0 ) { 
		pController->activateTimeline( true );
	} else {
		pController->activateTimeline( false );
	}
}

void OscServer::TIMELINE_ADD_MARKER_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	auto pController = pHydrogen->getCoreActionController();
	pController->addTempoMarker( static_cast<int>(std::round( argv[0]->f )),
								 argv[1]->f);
}

void OscServer::TIMELINE_DELETE_MARKER_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	auto pController = pHydrogen->getCoreActionController();
	pController->deleteTempoMarker( static_cast<int>( std::round( argv[0]->f ) ) );
}

void OscServer::JACK_TRANSPORT_ACTIVATION_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}
	
	auto pController = pHydrogen->getCoreActionController();

	if ( argv[0]->f != 0 ) {
		pController->activateJackTransport( true );
	} else {
		pController->activateJackTransport( false );
	}
}

void OscServer::JACK_TIMEBASE_MASTER_ACTIVATION_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	auto pController = pHydrogen->getCoreActionController();
	if ( argv[0]->f != 0 ) {
		pController->activateJackTimebaseControl( true );
	} else {
		pController->activateJackTimebaseControl( false );
	}
}

void OscServer::SONG_MODE_ACTIVATION_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	auto pController = pHydrogen->getCoreActionController();
	if ( argv[0]->f != 0 ) {
		pController->activateSongMode( true );
	} else {
		pController->activateSongMode( false );
	}
}

void OscServer::LOOP_MODE_ACTIVATION_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	auto pController = pHydrogen->getCoreActionController();
	if ( argv[0]->f != 0 ) {
		pController->activateLoopMode( true );
	} else {
		pController->activateLoopMode( false );
	}
}

void OscServer::RELOCATE_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	pHydrogen->getCoreActionController()->locateToColumn( static_cast<int>(std::round( argv[0]->f ) ) );
}

void OscServer::NEW_PATTERN_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}
	
	auto pController = pHydrogen->getCoreActionController();
	pController->newPattern( QString::fromUtf8( &argv[0]->s ) );
}

void OscServer::OPEN_PATTERN_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	auto pController = pHydrogen->getCoreActionController();
	pController->openPattern( QString::fromUtf8( &argv[0]->s ) );
}

void OscServer::REMOVE_PATTERN_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	auto pController = pHydrogen->getCoreActionController();
	pController->removePattern( static_cast<int>(std::round( argv[0]->f )) );
}

void OscServer::CLEAR_SELECTED_INSTRUMENT_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	const int nInstr = pHydrogen->getSelectedInstrumentNumber();
	if ( nInstr == -1 ) {
		WARNINGLOG( "No instrument selected" );
		return;
	}

	pHydrogen->getCoreActionController()->clearInstrumentInPattern( nInstr );
}

void OscServer::CLEAR_INSTRUMENT_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	H2Core::Hydrogen::get_instance()->getCoreActionController()
		->clearInstrumentInPattern( static_cast<int>(std::round( argv[0]->f )) );
}

void OscServer::CLEAR_PATTERN_Handler( lo_arg **argv, int i )
{
	INFOLOG( "processing message" );
	std::shared_ptr<Action> pAction = std::make_shared<Action>( "CLEAR_PATTERN" );
	MidiActionManager::get_instance()->handleAction( pAction );
}

void OscServer::NOTE_ON_Handler( lo_arg **argv, int i )
{
	const int nNote = static_cast<int>( std::round( argv[0]->f ) );
	if ( nNote < H2Core::MidiMessage::instrumentOffset || nNote > 127 ) {
		ERRORLOG( QString( "Provided note [%1] out of bound [%2,127]." )
				  .arg( nNote ).arg( H2Core::MidiMessage::instrumentOffset ) );
		return;
	}

	float fVelocity = argv[1]->f;
	if ( fVelocity < 0 ) {
		WARNINGLOG( QString( "Provided velocity [%1] out of bound. Using minimum value [0] instead." )
					.arg( fVelocity ) );
		fVelocity = 0;
	}
	else if ( fVelocity > 1.0 ) {
		WARNINGLOG( QString( "Provided velocity [%1] out of bound. Using maximum value [1.0] instead." )
					.arg( fVelocity ) );
		fVelocity = 1.0;
	}

	INFOLOG( QString( "processing message with note: [%1] and velocity: [%2]" )
			 .arg( nNote ).arg( fVelocity ) );

	H2Core::Hydrogen::get_instance()->getCoreActionController()
		->handleNote( nNote, fVelocity, false );
}

void OscServer::NOTE_OFF_Handler( lo_arg** argv, int i )
{
	const int nNote = static_cast<int>( std::round( argv[0]->f ) );
	if ( nNote < H2Core::MidiMessage::instrumentOffset || nNote > 127 ) {
		ERRORLOG( QString( "Provided note [%1] out of bound [%2,127]." )
				  .arg( nNote ).arg( H2Core::MidiMessage::instrumentOffset ) );
		return;
	}

	INFOLOG( QString( "processing message with note: [%1]" ).arg( nNote ) );

	H2Core::Hydrogen::get_instance()->getCoreActionController()
		->handleNote( nNote, 0.0, true );
}

void OscServer::SONG_EDITOR_TOGGLE_GRID_CELL_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	auto pController = pHydrogen->getCoreActionController();
	pController->toggleGridCell( static_cast<int>(std::round( argv[0]->f )),
								 static_cast<int>(std::round( argv[1]->f )) );
}

void OscServer::LOAD_DRUMKIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}
	
	auto pController = pHydrogen->getCoreActionController();

	bool bConditionalLoad = true;
	if ( argc > 1 ) {
		bConditionalLoad = argv[1]->f == 0 ? false : true;
	}
	
	pController->setDrumkit( QString::fromUtf8( &argv[0]->s ),
							 bConditionalLoad );
}

void OscServer::UPGRADE_DRUMKIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	
	auto pController = H2Core::Hydrogen::get_instance()->getCoreActionController();

	QString sNewPath = "";
	if ( argc > 1 ) {
		sNewPath = QString::fromUtf8( &argv[1]->s );
	}
	
	pController->upgradeDrumkit( QString::fromUtf8( &argv[0]->s ),
								 sNewPath );
}

void OscServer::VALIDATE_DRUMKIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );

	bool bValidateLegacyKits = false;
	if ( argc > 1 ) {
		bValidateLegacyKits = argv[1]->f == 0 ? false : true;
	}
	
	auto pController = H2Core::Hydrogen::get_instance()->getCoreActionController();
	pController->validateDrumkit( QString::fromUtf8( &argv[0]->s ),
								  bValidateLegacyKits );
}

void OscServer::EXTRACT_DRUMKIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	
	auto pController = H2Core::Hydrogen::get_instance()->getCoreActionController();

	QString sTargetDir = "";
	if ( argc > 1 ) {
		sTargetDir = QString::fromUtf8( &argv[1]->s );
	}
	
	pController->extractDrumkit( QString::fromUtf8( &argv[0]->s ), sTargetDir );
}

// -------------------------------------------------------------------
// Helper functions

bool IsLoAddressEqual( lo_address first, lo_address second )
{
	bool portEqual = ( strcmp( lo_address_get_port( first ), lo_address_get_port( second ) ) == 0);
	bool hostEqual = ( strcmp( lo_address_get_hostname( first ), lo_address_get_hostname( second ) ) == 0);
	bool protoEqual = ( lo_address_get_protocol( first ) == lo_address_get_protocol( second ) );
	
	return portEqual && hostEqual && protoEqual;
}

void OscServer::broadcastMessage( const char* msgText, lo_message message ) {
	for ( const auto& clientAddress: m_pClientRegistry ){
		
		INFOLOG( QString( "Outgoing OSC broadcast message %1" ).arg( msgText ));
		
		int i;
		for (i = 0; i < lo_message_get_argc( message ); i++) {
			QString formattedArgument = qPrettyPrint( (lo_type)lo_message_get_types(message)[i], lo_message_get_argv(message)[i] );
			INFOLOG(QString("Argument %1: %2 %3").arg(i).arg(lo_message_get_types(message)[i]).arg(formattedArgument));
		}
		
		lo_send_message(clientAddress, msgText, message);
	}
}

// -------------------------------------------------------------------
// Main action handler

void OscServer::handleAction( std::shared_ptr<Action> pAction )
{
	H2Core::Preferences *pPref = H2Core::Preferences::get_instance();
	
	if( !pPref->getOscFeedbackEnabled() ){
		return;
	}
	
	if( pAction->getType() == "MASTER_VOLUME_ABSOLUTE"){
		bool ok;
		float fValue = pAction->getValue().toFloat(&ok);
			
		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		broadcastMessage("/Hydrogen/MASTER_VOLUME_ABSOLUTE", reply);
		
		lo_message_free( reply );
	}
	
	if( pAction->getType() == "STRIP_VOLUME_ABSOLUTE"){
		bool ok;
		float fValue = pAction->getValue().toFloat(&ok);

		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		QByteArray ba = QString("/Hydrogen/STRIP_VOLUME_ABSOLUTE/%1").arg(pAction->getParameter1()).toLatin1();
		const char *c_str2 = ba.data();

		broadcastMessage( c_str2, reply);
		
		lo_message_free( reply );
	}
	
	if( pAction->getType() == "TOGGLE_METRONOME"){
		bool ok;
		float param1 = pAction->getParameter1().toFloat(&ok);

		lo_message reply = lo_message_new();
		lo_message_add_float(reply, param1);

		broadcastMessage("/Hydrogen/TOGGLE_METRONOME", reply);
		
		lo_message_free( reply );
	}
	
	if( pAction->getType() == "MUTE_TOGGLE"){
		bool ok;
		float param1 = pAction->getParameter1().toFloat(&ok);

		lo_message reply = lo_message_new();
		lo_message_add_float(reply, param1);
		
		broadcastMessage("/Hydrogen/MUTE_TOGGLE", reply);
		
		lo_message_free( reply );
	}
	
	if( pAction->getType() == "STRIP_MUTE_TOGGLE"){
		bool ok;
		float fValue = pAction->getValue().toFloat(&ok);

		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		QByteArray ba = QString("/Hydrogen/STRIP_MUTE_TOGGLE/%1").arg(pAction->getParameter1()).toLatin1();
		const char *c_str2 = ba.data();

		broadcastMessage( c_str2, reply);
		
		lo_message_free( reply );
	}
	
	if( pAction->getType() == "STRIP_SOLO_TOGGLE"){
		bool ok;
		float fValue = pAction->getValue().toFloat(&ok);

		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		QByteArray ba = QString("/Hydrogen/STRIP_SOLO_TOGGLE/%1").arg(pAction->getParameter1()).toLatin1();
		const char *c_str2 = ba.data();

		broadcastMessage( c_str2, reply);
		
		lo_message_free( reply );
	}
	
	if( pAction->getType() == "PAN_ABSOLUTE"){
		bool ok;
		float fValue = pAction->getValue().toFloat(&ok);

		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		QByteArray ba = QString("/Hydrogen/PAN_ABSOLUTE/%1").arg(pAction->getParameter1()).toLatin1();
		const char *c_str2 = ba.data();

		broadcastMessage( c_str2, reply);
		
		lo_message_free( reply );
	}

	if( pAction->getType() == "PAN_ABSOLUTE_SYM"){
		bool ok;
		float fValue = pAction->getValue().toFloat(&ok);

		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		QByteArray ba = QString("/Hydrogen/PAN_ABSOLUTE_SYM/%1").arg(pAction->getParameter1()).toLatin1();
		const char *c_str2 = ba.data();

		broadcastMessage( c_str2, reply);
		
		lo_message_free( reply );
	}
}

bool OscServer::init()
{
	if ( m_pServerThread == nullptr || !m_pServerThread->is_valid() ) {
		ERRORLOG("Failed to initialize OSC server. No valid server thread.");
		return false;
	}

	/*
	 *  Register all handler functions
	 */

	//This handler is responsible for registering clients
	m_pServerThread->add_method(nullptr, nullptr, [&](lo_message msg){
		lo_address address = lo_message_get_source(msg);

		bool AddressRegistered = false;
		for ( const auto& cclientAddress : m_pClientRegistry ){
			if ( IsLoAddressEqual( address, cclientAddress ) ) {
				AddressRegistered = true;
				break;
			}
		}

		if( ! AddressRegistered ){
			lo_address newAddress =
				lo_address_new_with_proto( lo_address_get_protocol( address ),
										   lo_address_get_hostname( address ),
										   lo_address_get_port( address ) );
			m_pClientRegistry.push_back( newAddress );
			INFOLOG( QString( "New OSC client registered. Hostname: %1, port: %2, protocol: %3" )
					 .arg( lo_address_get_hostname( address ) )
					 .arg( lo_address_get_port( address ) )
					 .arg( lo_address_get_protocol( address ) ) );
										
			H2Core::Hydrogen::get_instance()->getCoreActionController()
				->initExternalControlInterfaces();
		}
									
		// Returning 1 means that the
		// message has not been fully
		// handled and the server should
		// try other methods.
		return 1;
	});

	m_pServerThread->add_method(nullptr, nullptr, incomingMessageLogging, nullptr);

	m_pServerThread->add_method("/Hydrogen/PLAY", "", PLAY_Handler);
	m_pServerThread->add_method("/Hydrogen/PLAY", "f", PLAY_Handler);
	m_pServerThread->add_method("/Hydrogen/PLAY_STOP_TOGGLE", "", PLAY_STOP_TOGGLE_Handler);
	m_pServerThread->add_method("/Hydrogen/PLAY_STOP_TOGGLE", "f", PLAY_STOP_TOGGLE_Handler);
	m_pServerThread->add_method("/Hydrogen/PLAY_PAUSE_TOGGLE", "", PLAY_PAUSE_TOGGLE_Handler);
	m_pServerThread->add_method("/Hydrogen/PLAY_PAUSE_TOGGLE", "f", PLAY_PAUSE_TOGGLE_Handler);
	m_pServerThread->add_method("/Hydrogen/STOP", "", STOP_Handler);
	m_pServerThread->add_method("/Hydrogen/STOP", "f", STOP_Handler);
	m_pServerThread->add_method("/Hydrogen/PAUSE", "", PAUSE_Handler);
	m_pServerThread->add_method("/Hydrogen/PAUSE", "f", PAUSE_Handler);
	
	m_pServerThread->add_method("/Hydrogen/RECORD_READY", "", RECORD_READY_Handler);
	m_pServerThread->add_method("/Hydrogen/RECORD_READY", "f", RECORD_READY_Handler);
	m_pServerThread->add_method("/Hydrogen/RECORD_STROBE_TOGGLE", "", RECORD_STROBE_TOGGLE_Handler);
	m_pServerThread->add_method("/Hydrogen/RECORD_STROBE_TOGGLE", "f", RECORD_STROBE_TOGGLE_Handler);
	m_pServerThread->add_method("/Hydrogen/RECORD_STROBE", "", RECORD_STROBE_Handler);
	m_pServerThread->add_method("/Hydrogen/RECORD_STROBE", "f", RECORD_STROBE_Handler);
	m_pServerThread->add_method("/Hydrogen/RECORD_EXIT", "", RECORD_EXIT_Handler);
	m_pServerThread->add_method("/Hydrogen/RECORD_EXIT", "f", RECORD_EXIT_Handler);
	
	m_pServerThread->add_method("/Hydrogen/MUTE", "", MUTE_Handler);
	m_pServerThread->add_method("/Hydrogen/MUTE", "f", MUTE_Handler);
	m_pServerThread->add_method("/Hydrogen/UNMUTE", "", UNMUTE_Handler);
	m_pServerThread->add_method("/Hydrogen/UNMUTE", "f", UNMUTE_Handler);
	m_pServerThread->add_method("/Hydrogen/MUTE_TOGGLE", "", MUTE_TOGGLE_Handler);
	m_pServerThread->add_method("/Hydrogen/MUTE_TOGGLE", "f", MUTE_TOGGLE_Handler);

	m_pServerThread->add_method("/Hydrogen/INSTRUMENT_PITCH", "ff",
								INSTRUMENT_PITCH_Handler);
	
	m_pServerThread->add_method("/Hydrogen/NEXT_BAR", "", NEXT_BAR_Handler);
	m_pServerThread->add_method("/Hydrogen/NEXT_BAR", "f", NEXT_BAR_Handler);
	m_pServerThread->add_method("/Hydrogen/PREVIOUS_BAR", "", PREVIOUS_BAR_Handler);
	m_pServerThread->add_method("/Hydrogen/PREVIOUS_BAR", "f", PREVIOUS_BAR_Handler);
	
	m_pServerThread->add_method("/Hydrogen/BPM", "f", BPM_Handler);
	m_pServerThread->add_method("/Hydrogen/BPM_DECR", "f", BPM_DECR_Handler);
	m_pServerThread->add_method("/Hydrogen/BPM_INCR", "f", BPM_INCR_Handler);

	m_pServerThread->add_method("/Hydrogen/MASTER_VOLUME_ABSOLUTE", "f", MASTER_VOLUME_ABSOLUTE_Handler);
	m_pServerThread->add_method("/Hydrogen/MASTER_VOLUME_RELATIVE", "f", MASTER_VOLUME_RELATIVE_Handler);
	
	m_pServerThread->add_method("/Hydrogen/SELECT_NEXT_PATTERN", "f", SELECT_NEXT_PATTERN_Handler);
	m_pServerThread->add_method("/Hydrogen/SELECT_ONLY_NEXT_PATTERN", "f", SELECT_ONLY_NEXT_PATTERN_Handler);
	m_pServerThread->add_method("/Hydrogen/SELECT_AND_PLAY_PATTERN", "f", SELECT_AND_PLAY_PATTERN_Handler);
	
	m_pServerThread->add_method("/Hydrogen/BEATCOUNTER", "", BEATCOUNTER_Handler);
	m_pServerThread->add_method("/Hydrogen/BEATCOUNTER", "f", BEATCOUNTER_Handler);
	
	m_pServerThread->add_method("/Hydrogen/TAP_TEMPO", "", TAP_TEMPO_Handler);
	m_pServerThread->add_method("/Hydrogen/TAP_TEMPO", "f", TAP_TEMPO_Handler);
	
	m_pServerThread->add_method("/Hydrogen/PLAYLIST_SONG", "f", PLAYLIST_SONG_Handler);
	m_pServerThread->add_method("/Hydrogen/PLAYLIST_NEXT_SONG", "", PLAYLIST_NEXT_SONG_Handler);
	m_pServerThread->add_method("/Hydrogen/PLAYLIST_NEXT_SONG", "f", PLAYLIST_NEXT_SONG_Handler);
	m_pServerThread->add_method("/Hydrogen/PLAYLIST_PREV_SONG", "", PLAYLIST_PREV_SONG_Handler);
	m_pServerThread->add_method("/Hydrogen/PLAYLIST_PREV_SONG", "f", PLAYLIST_PREV_SONG_Handler);
	
	m_pServerThread->add_method("/Hydrogen/TOGGLE_METRONOME", "", TOGGLE_METRONOME_Handler);
	m_pServerThread->add_method("/Hydrogen/TOGGLE_METRONOME", "f", TOGGLE_METRONOME_Handler);
	
	m_pServerThread->add_method("/Hydrogen/SELECT_INSTRUMENT", "f", SELECT_INSTRUMENT_Handler);
	
	m_pServerThread->add_method("/Hydrogen/UNDO_ACTION", "", UNDO_ACTION_Handler);
	m_pServerThread->add_method("/Hydrogen/UNDO_ACTION", "f", UNDO_ACTION_Handler);
	m_pServerThread->add_method("/Hydrogen/REDO_ACTION", "", REDO_ACTION_Handler);
	m_pServerThread->add_method("/Hydrogen/REDO_ACTION", "f", REDO_ACTION_Handler);

	m_pServerThread->add_method("/Hydrogen/NEW_SONG", "s", NEW_SONG_Handler);
	m_pServerThread->add_method("/Hydrogen/OPEN_SONG", "s", OPEN_SONG_Handler);
	m_pServerThread->add_method("/Hydrogen/SAVE_SONG", "", SAVE_SONG_Handler);
	m_pServerThread->add_method("/Hydrogen/SAVE_SONG", "f", SAVE_SONG_Handler);
	m_pServerThread->add_method("/Hydrogen/SAVE_SONG_AS", "s", SAVE_SONG_AS_Handler);
	m_pServerThread->add_method("/Hydrogen/SAVE_PREFERENCES", "", SAVE_PREFERENCES_Handler);
	m_pServerThread->add_method("/Hydrogen/SAVE_PREFERENCES", "f", SAVE_PREFERENCES_Handler);
	m_pServerThread->add_method("/Hydrogen/QUIT", "", QUIT_Handler);
	m_pServerThread->add_method("/Hydrogen/QUIT", "f", QUIT_Handler);

	m_pServerThread->add_method("/Hydrogen/TIMELINE_ACTIVATION", "f", TIMELINE_ACTIVATION_Handler);
	m_pServerThread->add_method("/Hydrogen/TIMELINE_ADD_MARKER", "ff", TIMELINE_ADD_MARKER_Handler);
	m_pServerThread->add_method("/Hydrogen/TIMELINE_DELETE_MARKER", "f", TIMELINE_DELETE_MARKER_Handler);

	m_pServerThread->add_method("/Hydrogen/JACK_TRANSPORT_ACTIVATION", "f", JACK_TRANSPORT_ACTIVATION_Handler);
	m_pServerThread->add_method("/Hydrogen/JACK_TIMEBASE_MASTER_ACTIVATION", "f", JACK_TIMEBASE_MASTER_ACTIVATION_Handler);
	m_pServerThread->add_method("/Hydrogen/SONG_MODE_ACTIVATION", "f", SONG_MODE_ACTIVATION_Handler);
	m_pServerThread->add_method("/Hydrogen/LOOP_MODE_ACTIVATION", "f", LOOP_MODE_ACTIVATION_Handler);
	m_pServerThread->add_method("/Hydrogen/RELOCATE", "f", RELOCATE_Handler);
	m_pServerThread->add_method("/Hydrogen/NEW_PATTERN", "s", NEW_PATTERN_Handler);
	m_pServerThread->add_method("/Hydrogen/OPEN_PATTERN", "s", OPEN_PATTERN_Handler);
	m_pServerThread->add_method("/Hydrogen/REMOVE_PATTERN", "f", REMOVE_PATTERN_Handler);
	m_pServerThread->add_method("/Hydrogen/CLEAR_INSTRUMENT", "f", CLEAR_INSTRUMENT_Handler);
	m_pServerThread->add_method("/Hydrogen/CLEAR_SELECTED_INSTRUMENT", "",
								CLEAR_SELECTED_INSTRUMENT_Handler);
	m_pServerThread->add_method("/Hydrogen/CLEAR_SELECTED_INSTRUMENT", "f",
								CLEAR_SELECTED_INSTRUMENT_Handler);
	m_pServerThread->add_method("/Hydrogen/CLEAR_PATTERN", "", CLEAR_PATTERN_Handler);
	m_pServerThread->add_method("/Hydrogen/CLEAR_PATTERN", "f", CLEAR_PATTERN_Handler);

	m_pServerThread->add_method("/Hydrogen/NOTE_ON", "ff", NOTE_ON_Handler);
	m_pServerThread->add_method("/Hydrogen/NOTE_OFF", "f", NOTE_OFF_Handler);

	m_pServerThread->add_method("/Hydrogen/SONG_EDITOR_TOGGLE_GRID_CELL", "ff", SONG_EDITOR_TOGGLE_GRID_CELL_Handler);
	m_pServerThread->add_method("/Hydrogen/LOAD_DRUMKIT", "s", LOAD_DRUMKIT_Handler);
	m_pServerThread->add_method("/Hydrogen/LOAD_DRUMKIT", "sf", LOAD_DRUMKIT_Handler);
	m_pServerThread->add_method("/Hydrogen/UPGRADE_DRUMKIT", "s", UPGRADE_DRUMKIT_Handler);
	m_pServerThread->add_method("/Hydrogen/UPGRADE_DRUMKIT", "ss", UPGRADE_DRUMKIT_Handler);
	m_pServerThread->add_method("/Hydrogen/VALIDATE_DRUMKIT", "s", VALIDATE_DRUMKIT_Handler);
	m_pServerThread->add_method("/Hydrogen/VALIDATE_DRUMKIT", "sf", VALIDATE_DRUMKIT_Handler);
	m_pServerThread->add_method("/Hydrogen/EXTRACT_DRUMKIT", "s", EXTRACT_DRUMKIT_Handler);
	m_pServerThread->add_method("/Hydrogen/EXTRACT_DRUMKIT", "ss", EXTRACT_DRUMKIT_Handler);

	m_pServerThread->add_method(nullptr, nullptr, generic_handler, nullptr);

	m_bInitialized = true;
	
	return true;
}

bool OscServer::start() {
	if ( m_pServerThread == nullptr || !m_pServerThread->is_valid() ) {
		ERRORLOG("Failed to start OSC server. No valid server thread.");
		return false;
	}

	if ( ! m_bInitialized ) {
		if ( ! init() ) {
			return false;
		}
	}

	m_pServerThread->start();

	int nOscPortUsed;
	if ( m_pPreferences->m_nOscTemporaryPort != -1 ) {
		nOscPortUsed = m_pPreferences->m_nOscTemporaryPort;
	} else {
		nOscPortUsed = m_pPreferences->getOscServerPort();
	}
	
	INFOLOG( QString( "Osc server started. Listening on port %1" )
			 .arg( nOscPortUsed ) );

	return true;
}

bool OscServer::stop() {
	if ( m_pServerThread == nullptr || !m_pServerThread->is_valid() ) {
		ERRORLOG("Failed to stop OSC server. No valid server thread.");
		return false;
	}

	m_pServerThread->stop();
	INFOLOG(QString("Osc server stopped" ));

	return true;
}

#endif /* H2CORE_HAVE_OSC */

