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

#include <core/Basics/Drumkit.h>
#include <core/Basics/GridPoint.h>
#include "core/Basics/InstrumentList.h"
#include "core/Basics/PatternList.h"
#include "core/Basics/Playlist.h"
#include "core/OscServer.h"
#include "core/CoreActionController.h"
#include "core/EventQueue.h"
#include "core/Hydrogen.h"
#include "core/AudioEngine/AudioEngine.h"
#include "core/Basics/Song.h"
#include "core/Midi/MidiAction.h"
#include "core/Midi/MidiActionManager.h"
#include "core/Midi/MidiMessage.h"

OscServer * OscServer::__instance = nullptr;


QString OscServer::qPrettyPrint( const lo_type& type, void* data )
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
	auto pMidiActionManager = pHydrogen->getMidiActionManager();
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return 1;
	}

	bool bMessageProcessed = false;
	
	const int nNumberOfStrips = pSong->getDrumkit()->getInstruments()->size();

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
			H2Core::CoreActionController::setStripPan(
				nStrip, argv[0]->f, false );
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
			H2Core::CoreActionController::setStripPanSym(
				nStrip, argv[0]->f, false );
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
			auto pAction = std::make_shared<MidiAction>(
				MidiAction::Type::PanRelative );
			pAction->setParameter1( QString::number( nStrip ) );
			pAction->setValue( QString::number( argv[0]->f, 'f', 0 ) );
			pMidiActionManager->handleMidiActionAsync( pAction );
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
			H2Core::CoreActionController::toggleStripIsMuted( nStrip );
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
			H2Core::CoreActionController::toggleStripIsSoloed( nStrip );
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



OscServer::OscServer( int nOscPort ) : m_bInitialized( false )
									 , m_nTemporaryPort( nOscPort )
{
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( pPref->getOscServerEnabled() ) {
		int nPort;
		// Check whether an alternative value was provided via CLI argument.
		if ( nOscPort != -1  ) {
			nPort = nOscPort;
		} else {
			nPort = pPref->getOscServerPort();
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

			m_nTemporaryPort = nTmpPort;
			
			H2Core::EventQueue::get_instance()->pushEvent(
				H2Core::Event::Type::Error, H2Core::Hydrogen::OSC_CANNOT_CONNECT_TO_PORT );
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

void OscServer::create_instance( int nOscPort )
{
	if( __instance == nullptr ) {
		__instance = new OscServer( nOscPort );
	}
}

// -------------------------------------------------------------------
// Handler functions

void OscServer::PLAY_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::Play ) );
}

void OscServer::PLAY_STOP_TOGGLE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::PlayStopToggle ) );
}

void OscServer::PLAY_PAUSE_TOGGLE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::PlayPauseToggle ) );
}

void OscServer::STOP_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::Stop ) );
}

void OscServer::PAUSE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::Pause ) );
}

void OscServer::RECORD_READY_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::RecordReady ) );
}

void OscServer::RECORD_STROBE_TOGGLE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::RecordStrobeToggle ) );
}

void OscServer::RECORD_STROBE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::RecordStrobe ) );
}

void OscServer::RECORD_EXIT_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::RecordExit ) );
}

void OscServer::MUTE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::Mute ) );
}

void OscServer::UNMUTE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::Unmute ) );
}

void OscServer::MUTE_TOGGLE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::MuteToggle ) );
}

void OscServer::NEXT_BAR_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::NextBar ) );
}

void OscServer::PREVIOUS_BAR_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::PreviousBar ) );
}

void OscServer::BPM_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	H2Core::CoreActionController::setBpm( argv[0]->f );
}

void OscServer::BPM_INCR_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::BpmIncr );
	pAction->setParameter1( QString::number( argv[0]->f, 'f', 0 ));

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::BPM_DECR_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::BpmDecr );
	pAction->setParameter1( QString::number( argv[0]->f, 'f', 0 ));
	
	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::MASTER_VOLUME_ABSOLUTE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	H2Core::CoreActionController::setMasterVolume( argv[0]->f );
}

void OscServer::MASTER_VOLUME_RELATIVE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::MasterVolumeRelative );
	pAction->setValue( QString::number( argv[0]->f, 'f', 0 ));

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::STRIP_VOLUME_ABSOLUTE_Handler(int param1, float param2)
{
	INFOLOG( "processing message" );
	H2Core::CoreActionController::setStripVolume( param1, param2, false );
}

void OscServer::STRIP_VOLUME_RELATIVE_Handler( const QString& param1,
											  const QString& param2 )
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::StripVolumeRelative );
	pAction->setParameter1( param1 );
	pAction->setValue( param2 );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::SELECT_NEXT_PATTERN_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::SelectNextPattern );
	pAction->setParameter1(  QString::number( argv[0]->f, 'f', 0 ) );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::SELECT_ONLY_NEXT_PATTERN_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::SelectOnlyNextPattern );
	pAction->setParameter1(  QString::number( argv[0]->f, 'f', 0 ) );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::SELECT_AND_PLAY_PATTERN_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::SelectAndPlayPattern );
	pAction->setParameter1(  QString::number( argv[0]->f, 'f', 0 ) );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::FILTER_CUTOFF_LEVEL_ABSOLUTE_Handler( const QString& param1,
													  const QString& param2)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::FilterCutoffLevelAbsolute );
	pAction->setParameter1( param1 );
	pAction->setValue( param2 );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync( pAction );
}


void OscServer::INSTRUMENT_PITCH_Handler( lo_arg** argv, int )
{
	INFOLOG( "processing message" );

	H2Core::CoreActionController::setInstrumentPitch(
		static_cast<int>( argv[0]->f ), argv[1]->f );
}

void OscServer::BEATCOUNTER_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::BeatCounter ) );
}

void OscServer::TAP_TEMPO_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::TapTempo ) );
}

void OscServer::PLAYLIST_SONG_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::PlaylistSong );
	pAction->setParameter1(  QString::number( argv[0]->f, 'f', 0 ) );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::PLAYLIST_NEXT_SONG_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::PlaylistNextSong ) );
}

void OscServer::PLAYLIST_PREV_SONG_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::PlaylistPrevSong ) );
}

void OscServer::TOGGLE_METRONOME_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::ToggleMetronome ) );
}

void OscServer::SELECT_INSTRUMENT_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::SelectInstrument );
	pAction->setValue( QString::number( argv[0]->f, 'f', 0 ) );

	// Null song handling done in MidiActionManager.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::UNDO_ACTION_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::UndoAction );

	// This one does also work the current song being nullptr.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::REDO_ACTION_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::RedoAction );

	// This one does also work the current song being nullptr.
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync( pAction );
}

// -------------------------------------------------------------------
// Actions required for session management.

void OscServer::NEW_SONG_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	const auto sPath = QString::fromUtf8( &argv[0]->s );
	if ( ! H2Core::Filesystem::isPathValid(
			 H2Core::Filesystem::Type::Song, sPath ) ||
		 ! H2Core::Filesystem::file_writable( sPath ) ) {
		ERRORLOG( QString( "Unable to create new song for invalid path [%1]" )
				  .arg( sPath ) );
		return;
	}

	auto pSong = H2Core::Song::getEmptySong();
	pSong->setFileName( sPath );
	H2Core::CoreActionController::setSong( pSong );
}

void OscServer::OPEN_SONG_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pSong = H2Core::CoreActionController::loadSong(
		QString::fromUtf8( &argv[0]->s ) );
	H2Core::CoreActionController::setSong( pSong );
}

void OscServer::SAVE_SONG_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );

	// Discarding missing samples and their corresponding instrument layer is an
	// operation with information loss. This is only allowed to be performed
	// explicitly via the GUI.
	H2Core::CoreActionController::saveSong( /* bKeepMissingSamples */ true );
}

void OscServer::SAVE_SONG_AS_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );

	// Discarding missing samples and their corresponding instrument layer is an
	// operation with information loss. This is only allowed to be performed
	// explicitly via the GUI.
	H2Core::CoreActionController::saveSongAs( QString::fromUtf8( &argv[0]->s ),
											  /* bKeepMissingSamples */ true );
}

void OscServer::SAVE_PREFERENCES_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	H2Core::CoreActionController::savePreferences();
}

void OscServer::QUIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	H2Core::CoreActionController::quit();
}

// -------------------------------------------------------------------

void OscServer::TIMELINE_ACTIVATION_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	if ( argv[0]->f != 0 ) {
		H2Core::CoreActionController::activateTimeline( true );
	} else {
		H2Core::CoreActionController::activateTimeline( false );
	}
}

void OscServer::TIMELINE_ADD_MARKER_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	H2Core::CoreActionController::addTempoMarker(
		static_cast<int>(std::round( argv[0]->f )), argv[1]->f);
}

void OscServer::TIMELINE_DELETE_MARKER_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	H2Core::CoreActionController::deleteTempoMarker(
		static_cast<int>( std::round( argv[0]->f ) ) );
}

void OscServer::JACK_TRANSPORT_ACTIVATION_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	if ( argv[0]->f != 0 ) {
		H2Core::CoreActionController::activateJackTransport( true );
	} else {
		H2Core::CoreActionController::activateJackTransport( false );
	}
}

void OscServer::JACK_TIMEBASE_MASTER_ACTIVATION_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	if ( argv[0]->f != 0 ) {
		H2Core::CoreActionController::activateJackTimebaseControl( true );
	} else {
		H2Core::CoreActionController::activateJackTimebaseControl( false );
	}
}

void OscServer::SONG_MODE_ACTIVATION_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	if ( argv[0]->f != 0 ) {
		H2Core::CoreActionController::activateSongMode( true );
	} else {
		H2Core::CoreActionController::activateSongMode( false );
	}
}

void OscServer::LOOP_MODE_ACTIVATION_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	if ( argv[0]->f != 0 ) {
		H2Core::CoreActionController::activateLoopMode( true );
	} else {
		H2Core::CoreActionController::activateLoopMode( false );
	}
}

void OscServer::RELOCATE_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	H2Core::CoreActionController::locateToColumn(
		static_cast<int>(std::round( argv[0]->f ) ) );
}

void OscServer::NEW_PATTERN_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	H2Core::CoreActionController::newPattern( QString::fromUtf8( &argv[0]->s ) );
}

void OscServer::OPEN_PATTERN_Handler( lo_arg** argv, int argc )
{
	INFOLOG( "processing message" );

	const auto pSong = H2Core::Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	const auto pPattern = H2Core::CoreActionController::loadPattern(
		QString::fromUtf8( &argv[0]->s )
	);
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Unable to load pattern [%1]" )
					  .arg( QString::fromUtf8( &argv[0]->s ) ) );
		return;
	}
	H2Core::CoreActionController::setPattern(
		pPattern, pSong->getPatternList()->size(), false
	);
}

void OscServer::REMOVE_PATTERN_Handler( lo_arg** argv, int argc )
{
	INFOLOG( "processing message" );
	H2Core::CoreActionController::removePattern(
		static_cast<int>(std::round( argv[0]->f )) );
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

	H2Core::CoreActionController::clearInstrumentInPattern( nInstr );
}

void OscServer::CLEAR_INSTRUMENT_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	H2Core::CoreActionController::clearInstrumentInPattern(
		static_cast<int>(std::round( argv[0]->f )) );
}

void OscServer::CLEAR_PATTERN_Handler( lo_arg **argv, int i )
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::ClearPattern );
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::COUNT_IN_Handler( lo_arg **argv, int i ) {
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::CountIn );
	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::COUNT_IN_PAUSE_TOGGLE_Handler( lo_arg **argv, int i ) {
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::CountInPauseToggle );
		H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::COUNT_IN_STOP_TOGGLE_Handler( lo_arg **argv, int i ) {
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::CountInStopToggle );
		H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::NOTE_ON_Handler( lo_arg **argv, int i )
{
	const int nNote = static_cast<int>( std::round( argv[0]->f ) );
	if ( nNote < H2Core::MidiMessage::nInstrumentOffset || nNote > 127 ) {
		ERRORLOG( QString( "Provided note [%1] out of bound [%2,127]." )
				  .arg( nNote ).arg( H2Core::MidiMessage::nInstrumentOffset ) );
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

	H2Core::CoreActionController::handleNote(
		nNote, H2Core::MidiMessage::nChannelAll, fVelocity, false );
}

void OscServer::NOTE_OFF_Handler( lo_arg** argv, int i )
{
	const int nNote = static_cast<int>( std::round( argv[0]->f ) );
	if ( nNote < H2Core::MidiMessage::nInstrumentOffset || nNote > 127 ) {
		ERRORLOG( QString( "Provided note [%1] out of bound [%2,127]." )
				  .arg( nNote ).arg( H2Core::MidiMessage::nInstrumentOffset ) );
		return;
	}

	INFOLOG( QString( "processing message with note: [%1]" ).arg( nNote ) );

	H2Core::CoreActionController::handleNote(
		nNote, H2Core::MidiMessage::nChannelAll, 0.0, true );
}

void OscServer::SONG_EDITOR_TOGGLE_GRID_CELL_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	H2Core::CoreActionController::toggleGridCell(
		H2Core::GridPoint( static_cast<int>(std::round( argv[0]->f )),
						   static_cast<int>(std::round( argv[1]->f )) ) );
}

void OscServer::LOAD_DRUMKIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );

	H2Core::CoreActionController::setDrumkit(
		QString::fromUtf8( &argv[0]->s ) );
}

void OscServer::LOAD_NEXT_DRUMKIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );

	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::LoadNextDrumkit ) );
}

void OscServer::LOAD_PREV_DRUMKIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );

	H2Core::Hydrogen::get_instance()->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::LoadPrevDrumkit ) );
}

void OscServer::UPGRADE_DRUMKIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	QString sNewPath = "";
	if ( argc > 1 ) {
		sNewPath = QString::fromUtf8( &argv[1]->s );
	}
	
	H2Core::CoreActionController::upgradeDrumkit(
		QString::fromUtf8( &argv[0]->s ), sNewPath );
}

void OscServer::VALIDATE_DRUMKIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );

	bool bValidateLegacyKits = false;
	if ( argc > 1 ) {
		bValidateLegacyKits = argv[1]->f == 0 ? false : true;
	}
	
	H2Core::CoreActionController::validateDrumkit(
		QString::fromUtf8( &argv[0]->s ), bValidateLegacyKits );
}

void OscServer::EXTRACT_DRUMKIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	QString sTargetDir = "";
	if ( argc > 1 ) {
		sTargetDir = QString::fromUtf8( &argv[1]->s );
	}
	
	H2Core::CoreActionController::extractDrumkit(
		QString::fromUtf8( &argv[0]->s ), sTargetDir );
}

void OscServer::NEW_PLAYLIST_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pPlaylist = std::make_shared<H2Core::Playlist>();
	H2Core::CoreActionController::setPlaylist( pPlaylist );
}

void OscServer::OPEN_PLAYLIST_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pPlaylist = H2Core::Playlist::load( QString::fromUtf8( &argv[0]->s ) );
	if ( pPlaylist == nullptr ) {
		ERRORLOG( QString( "Unable to load Playlist [%1]" )
				  .arg( QString::fromUtf8( &argv[0]->s ) ) );
		return;
	}

	H2Core::CoreActionController::setPlaylist( pPlaylist );
}

void OscServer::SAVE_PLAYLIST_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	H2Core::CoreActionController::savePlaylist();
}

void OscServer::SAVE_PLAYLIST_AS_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	H2Core::CoreActionController::savePlaylistAs( QString::fromUtf8( &argv[0]->s ) );
}

void OscServer::PLAYLIST_ADD_SONG_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pEntry = std::make_shared<H2Core::PlaylistEntry>();
	pEntry->setSongPath( QString::fromUtf8( &argv[0]->s ) );
	// Append at the end
	H2Core::CoreActionController::addToPlaylist( pEntry, -1 );
}

void OscServer::PLAYLIST_ADD_CURRENT_SONG_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pSong = H2Core::Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set" );
		return;
	}
	auto pEntry = std::make_shared<H2Core::PlaylistEntry>();
	pEntry->setSongPath( pSong->getFileName() );
	// Append at the end
	H2Core::CoreActionController::addToPlaylist( pEntry, -1 );
}

void OscServer::PLAYLIST_REMOVE_SONG_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pPlaylist = H2Core::Hydrogen::get_instance()->getPlaylist();
	if ( pPlaylist == nullptr ) {
		ERRORLOG( "invalid playlist" );
		return;
	}

	const int nIndex = argv[0]->f;
	if ( nIndex < 0 || nIndex >= pPlaylist->size() ) {
		ERRORLOG( QString( "Provided index [%1] out of bound [0,%2)" )
				  .arg( nIndex ).arg( pPlaylist->size() ) );
		return;
	}
	auto pEntry = pPlaylist->get( nIndex );
	H2Core::CoreActionController::removeFromPlaylist( pEntry, nIndex );
}

// -------------------------------------------------------------------
// Helper functions

bool IsLoAddressEqual( const lo_address& first, const lo_address& second )
{
	bool portEqual = ( strcmp( lo_address_get_port( first ), lo_address_get_port( second ) ) == 0);
	bool hostEqual = ( strcmp( lo_address_get_hostname( first ), lo_address_get_hostname( second ) ) == 0);
	bool protoEqual = ( lo_address_get_protocol( first ) == lo_address_get_protocol( second ) );
	
	return portEqual && hostEqual && protoEqual;
}

void OscServer::broadcastMessage( const char* msgText, const lo_message& message ) {
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
// Main Midiaction handler

void OscServer::handleMidiAction( std::shared_ptr<MidiAction> pAction )
{
	if ( ! H2Core::Preferences::get_instance()->getOscFeedbackEnabled() ) {
		return;
	}
	
	if ( pAction->getType() == MidiAction::Type::MasterVolumeAbsolute ) {
		bool ok;
		float fValue = pAction->getValue().toFloat(&ok);
			
		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		broadcastMessage("/Hydrogen/MASTER_VOLUME_ABSOLUTE", reply);
		
		lo_message_free( reply );
	}
	
	if ( pAction->getType() == MidiAction::Type::StripVolumeAbsolute ) {
		bool ok;
		float fValue = pAction->getValue().toFloat(&ok);

		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		QByteArray ba = QString("/Hydrogen/STRIP_VOLUME_ABSOLUTE/%1").arg(pAction->getParameter1()).toLatin1();
		const char *c_str2 = ba.data();

		broadcastMessage( c_str2, reply);
		
		lo_message_free( reply );
	}
	
	if ( pAction->getType() == MidiAction::Type::ToggleMetronome ) {
		bool ok;
		float param1 = pAction->getParameter1().toFloat(&ok);

		lo_message reply = lo_message_new();
		lo_message_add_float(reply, param1);

		broadcastMessage("/Hydrogen/TOGGLE_METRONOME", reply);
		
		lo_message_free( reply );
	}
	
	if ( pAction->getType() == MidiAction::Type::MuteToggle ) {
		bool ok;
		float param1 = pAction->getParameter1().toFloat(&ok);

		lo_message reply = lo_message_new();
		lo_message_add_float(reply, param1);
		
		broadcastMessage("/Hydrogen/MUTE_TOGGLE", reply);
		
		lo_message_free( reply );
	}
	
	if ( pAction->getType() == MidiAction::Type::StripMuteToggle ) {
		bool ok;
		float fValue = pAction->getValue().toFloat(&ok);

		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		QByteArray ba = QString("/Hydrogen/STRIP_MUTE_TOGGLE/%1")
			.arg(pAction->getParameter1()).toLatin1();
		const char *c_str2 = ba.data();

		broadcastMessage( c_str2, reply);
		
		lo_message_free( reply );
	}
	
	if ( pAction->getType() == MidiAction::Type::StripSoloToggle ) {
		bool ok;
		float fValue = pAction->getValue().toFloat(&ok);

		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		QByteArray ba = QString("/Hydrogen/STRIP_SOLO_TOGGLE/%1").arg(pAction->getParameter1()).toLatin1();
		const char *c_str2 = ba.data();

		broadcastMessage( c_str2, reply);
		
		lo_message_free( reply );
	}
	
	if ( pAction->getType() == MidiAction::Type::PanAbsolute ) {
		bool ok;
		float fValue = pAction->getValue().toFloat(&ok);

		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		QByteArray ba = QString("/Hydrogen/PAN_ABSOLUTE/%1").arg(pAction->getParameter1()).toLatin1();
		const char *c_str2 = ba.data();

		broadcastMessage( c_str2, reply);
		
		lo_message_free( reply );
	}

	if ( pAction->getType() == MidiAction::Type::PanAbsoluteSym ) {
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
	m_pServerThread->add_method(nullptr, nullptr, [&](lo_message msg) {
		lo_address address = lo_message_get_source(msg);

		bool AddressRegistered = false;
		for ( const auto& cclientAddress : m_pClientRegistry ) {
			if ( IsLoAddressEqual( address, cclientAddress ) ) {
				AddressRegistered = true;
				break;
			}
		}

		if ( ! AddressRegistered ) {
			lo_address newAddress =
				lo_address_new_with_proto( lo_address_get_protocol( address ),
										   lo_address_get_hostname( address ),
										   lo_address_get_port( address ) );
			m_pClientRegistry.push_back( newAddress );
			INFOLOG( QString( "New OSC client registered. Hostname: %1, port: %2, protocol: %3" )
					 .arg( lo_address_get_hostname( address ) )
					 .arg( lo_address_get_port( address ) )
					 .arg( lo_address_get_protocol( address ) ) );
										
			H2Core::CoreActionController::initExternalControlInterfaces();
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
	m_pServerThread->add_method("/Hydrogen/COUNT_IN", "", COUNT_IN_Handler);
	m_pServerThread->add_method("/Hydrogen/COUNT_IN", "f", COUNT_IN_Handler);
	m_pServerThread->add_method("/Hydrogen/COUNT_IN_PAUSE_TOGGLE", "",
								COUNT_IN_PAUSE_TOGGLE_Handler);
	m_pServerThread->add_method("/Hydrogen/COUNT_IN_PAUSE_TOGGLE", "f",
								COUNT_IN_PAUSE_TOGGLE_Handler);
	m_pServerThread->add_method("/Hydrogen/COUNT_IN_STOP_TOGGLE", "",
								COUNT_IN_STOP_TOGGLE_Handler);
	m_pServerThread->add_method("/Hydrogen/COUNT_IN_STOP_TOGGLE", "f",
								COUNT_IN_STOP_TOGGLE_Handler);

	m_pServerThread->add_method("/Hydrogen/NOTE_ON", "ff", NOTE_ON_Handler);
	m_pServerThread->add_method("/Hydrogen/NOTE_OFF", "f", NOTE_OFF_Handler);

	m_pServerThread->add_method("/Hydrogen/SONG_EDITOR_TOGGLE_GRID_CELL", "ff", SONG_EDITOR_TOGGLE_GRID_CELL_Handler);
	m_pServerThread->add_method("/Hydrogen/LOAD_DRUMKIT", "s", LOAD_DRUMKIT_Handler);
	m_pServerThread->add_method("/Hydrogen/LOAD_PREV_DRUMKIT", "", LOAD_PREV_DRUMKIT_Handler);
	m_pServerThread->add_method("/Hydrogen/LOAD_PREV_DRUMKIT", "f", LOAD_PREV_DRUMKIT_Handler);
	m_pServerThread->add_method("/Hydrogen/LOAD_NEXT_DRUMKIT", "", LOAD_NEXT_DRUMKIT_Handler);
	m_pServerThread->add_method("/Hydrogen/LOAD_NEXT_DRUMKIT", "f", LOAD_NEXT_DRUMKIT_Handler);
	m_pServerThread->add_method("/Hydrogen/UPGRADE_DRUMKIT", "s", UPGRADE_DRUMKIT_Handler);
	m_pServerThread->add_method("/Hydrogen/UPGRADE_DRUMKIT", "ss", UPGRADE_DRUMKIT_Handler);
	m_pServerThread->add_method("/Hydrogen/VALIDATE_DRUMKIT", "s", VALIDATE_DRUMKIT_Handler);
	m_pServerThread->add_method("/Hydrogen/VALIDATE_DRUMKIT", "sf", VALIDATE_DRUMKIT_Handler);
	m_pServerThread->add_method("/Hydrogen/EXTRACT_DRUMKIT", "s", EXTRACT_DRUMKIT_Handler);
	m_pServerThread->add_method("/Hydrogen/EXTRACT_DRUMKIT", "ss", EXTRACT_DRUMKIT_Handler);

	m_pServerThread->add_method("/Hydrogen/NEW_PLAYLIST", "", NEW_PLAYLIST_Handler);
	m_pServerThread->add_method("/Hydrogen/NEW_PLAYLIST", "f", NEW_PLAYLIST_Handler);
	m_pServerThread->add_method("/Hydrogen/OPEN_PLAYLIST", "s", OPEN_PLAYLIST_Handler);
	m_pServerThread->add_method("/Hydrogen/SAVE_PLAYLIST", "", SAVE_PLAYLIST_Handler);
	m_pServerThread->add_method("/Hydrogen/SAVE_PLAYLIST", "f", SAVE_PLAYLIST_Handler);
	m_pServerThread->add_method("/Hydrogen/SAVE_PLAYLIST_AS", "s", SAVE_PLAYLIST_AS_Handler);
	m_pServerThread->add_method("/Hydrogen/PLAYLIST_ADD_SONG", "s",
								PLAYLIST_ADD_SONG_Handler);
	m_pServerThread->add_method("/Hydrogen/PLAYLIST_ADD_CURRENT_SONG", "",
								PLAYLIST_ADD_CURRENT_SONG_Handler);
	m_pServerThread->add_method("/Hydrogen/PLAYLIST_ADD_CURRENT_SONG", "f",
								PLAYLIST_ADD_CURRENT_SONG_Handler);
	m_pServerThread->add_method("/Hydrogen/PLAYLIST_REMOVE_SONG", "f",
								PLAYLIST_REMOVE_SONG_Handler);

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
	const auto pPref = H2Core::Preferences::get_instance();
	if ( m_nTemporaryPort != -1 ) {
		nOscPortUsed = m_nTemporaryPort;
	} else {
		nOscPortUsed = pPref->getOscServerPort();
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

