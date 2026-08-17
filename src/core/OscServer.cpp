/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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


#include <pthread.h>
#include <unistd.h>
#include <QRegularExpression>

#include "core/Midi/Midi.h"
#include "core/Helpers/Filesystem.h"
#include "core/Preferences/Preferences.h"

//currently H2CORE_HAVE_OSC means: liblo is present..
#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_

#include <lo/lo.h>
#include <lo/lo_cpp.h>

#include "core/OscServer.h"

#include "core/AudioEngine/AudioEngine.h"
#include "core/Basics/Drumkit.h"
#include "core/Basics/GridPoint.h"
#include "core/Basics/InstrumentList.h"
#include "core/Basics/PatternList.h"
#include "core/Basics/Playlist.h"
#include "core/Basics/Song.h"
#include "core/CoreActionController.h"
#include "core/EventQueue.h"
#include "core/Hydrogen.h"
#include "core/Midi/MidiAction.h"
#include "core/Midi/MidiActionManager.h"
#include "core/Midi/MidiMessage.h"
#include "core/SoundLibrary/SoundLibraryDatabase.h"

namespace H2Core {

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
	auto pServer = static_cast<OscServer*>( user_data );
	auto pHydrogen = pServer->m_pHydrogen;
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
			pServer->STRIP_VOLUME_ABSOLUTE_Handler( nStrip , argv[0]->f );
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
			pServer->STRIP_VOLUME_RELATIVE_Handler(
				static_cast<int>( argv[0]->f ), nStrip
			);
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
			pHydrogen->getCoreActionController()->setStripPan(
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
			pHydrogen->getCoreActionController()->setStripPanSym(
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
			pAction->setInstrument( nStrip );
			pAction->setValue( static_cast<int>( argv[0]->f ) );
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
			pServer->FILTER_CUTOFF_LEVEL_ABSOLUTE_Handler(
				static_cast<int>( argv[0]->f ), nStrip
			);
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
			pHydrogen->getCoreActionController()->toggleStripIsMuted( nStrip );
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
			pHydrogen->getCoreActionController()->toggleStripIsSoloed( nStrip );
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



OscServer::OscServer( H2Core::Hydrogen* pHydrogen, int nOscPort )
									 : m_pHydrogen( pHydrogen )
									 , m_bInitialized( false )
									 , m_nTemporaryPort( nOscPort )
{
	auto pPref = m_pHydrogen->getPreferences();

	// No OSC server - and so no bound port - when running as a plugin: the host
	// owns network endpoints and control surfaces (ADR 0026). The port would
	// otherwise be bound right here in the constructor.
	if ( pPref->getOscServerEnabled() &&
		 m_pHydrogen->getProcessMode() != H2Core::ProcessMode::Editor &&
		 ! m_pHydrogen->isUnderPluginHost() ) {
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
			
			m_pHydrogen->getEventQueue()->pushEvent(
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
}

void OscServer::addMethod( const char* path, const char* types,
						   void ( OscServer::*handler )( lo_arg**, int ) ) {
	// Bind the per-message member callback to this instance so it reaches this
	// OscServer's owning Hydrogen (ADR 0015). This is the single place the liblo
	// callable glue lives.
	m_pServerThread->add_method(
		path, types,
		[this, handler]( lo_arg** argv, int argc ) {
			( this->*handler )( argv, argc );
		} );
}

// -------------------------------------------------------------------
// Handler functions

void OscServer::PLAY_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::Play ) );
}

void OscServer::PLAY_STOP_TOGGLE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::PlayStopToggle ) );
}

void OscServer::PLAY_PAUSE_TOGGLE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::PlayPauseToggle ) );
}

void OscServer::STOP_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::Stop ) );
}

void OscServer::PAUSE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::Pause ) );
}

void OscServer::RECORD_READY_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::RecordReady ) );
}

void OscServer::RECORD_STROBE_TOGGLE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::RecordStrobeToggle ) );
}

void OscServer::RECORD_STROBE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::RecordStrobe ) );
}

void OscServer::RECORD_EXIT_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::RecordExit ) );
}

void OscServer::MUTE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::Mute ) );
}

void OscServer::UNMUTE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::Unmute ) );
}

void OscServer::MUTE_TOGGLE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::MuteToggle ) );
}

void OscServer::NEXT_BAR_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::NextBar ) );
}

void OscServer::PREVIOUS_BAR_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::PreviousBar ) );
}

void OscServer::BPM_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	m_pHydrogen->getCoreActionController()->setBpm( argv[0]->f );
}

void OscServer::BPM_INCR_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::BpmIncr );
	pAction->setFactor( argv[0]->f );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::BPM_DECR_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::BpmDecr );
	pAction->setFactor( argv[0]->f );
	
	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::MASTER_VOLUME_ABSOLUTE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	m_pHydrogen->getCoreActionController()->setMasterVolume( argv[0]->f );
}

void OscServer::MASTER_VOLUME_RELATIVE_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::MasterVolumeRelative );
	pAction->setValue( static_cast<int>( argv[0]->f ) );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::HUMANIZATION_SWING_ABSOLUTE_Handler( lo_arg** argv, int i )
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::HumanizationSwingAbsolute
	);
	pAction->setValue( static_cast<int>( argv[0]->f ) );

	// Null song handling done in MidiActionManager.
	m_pHydrogen
		->getMidiActionManager()
		->handleMidiActionAsync( pAction );
}

void OscServer::HUMANIZATION_SWING_RELATIVE_Handler( lo_arg** argv, int i )
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::HumanizationSwingRelative
	);
	// Rounding to ensure we do not miss the 1.0 resulting in increases.
	pAction->setValue( static_cast<int>( argv[0]->f ) );

	// Null song handling done in MidiActionManager.
	m_pHydrogen
		->getMidiActionManager()
		->handleMidiActionAsync( pAction );
}

void OscServer::HUMANIZATION_TIMING_ABSOLUTE_Handler( lo_arg** argv, int i )
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::HumanizationTimingAbsolute
	);
	pAction->setValue( static_cast<int>( argv[0]->f ) );

	// Null song handling done in MidiActionManager.
	m_pHydrogen
		->getMidiActionManager()
		->handleMidiActionAsync( pAction );
}

void OscServer::HUMANIZATION_TIMING_RELATIVE_Handler( lo_arg** argv, int i )
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::HumanizationTimingRelative
	);
	// Rounding to ensure we do not miss the 1.0 resulting in increases.
	pAction->setValue( static_cast<int>( argv[0]->f ) );

	// Null song handling done in MidiActionManager.
	m_pHydrogen
		->getMidiActionManager()
		->handleMidiActionAsync( pAction );
}

void OscServer::HUMANIZATION_VELOCITY_ABSOLUTE_Handler( lo_arg** argv, int i )
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::HumanizationVelocityAbsolute
	);
	pAction->setValue( static_cast<int>( argv[0]->f ) );

	// Null song handling done in MidiActionManager.
	m_pHydrogen
		->getMidiActionManager()
		->handleMidiActionAsync( pAction );
}

void OscServer::HUMANIZATION_VELOCITY_RELATIVE_Handler( lo_arg** argv, int i )
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::HumanizationVelocityRelative
	);
	// Rounding to ensure we do not miss the 1.0 resulting in increases.
	pAction->setValue( static_cast<int>( argv[0]->f ) );

	// Null song handling done in MidiActionManager.
	m_pHydrogen
		->getMidiActionManager()
		->handleMidiActionAsync( pAction );
}

void OscServer::STRIP_VOLUME_ABSOLUTE_Handler(int param1, float param2)
{
	INFOLOG( "processing message" );
	m_pHydrogen->getCoreActionController()->setStripVolume( param1, param2, false );
}

void OscServer::STRIP_VOLUME_RELATIVE_Handler( int nValue, int nInstrument )
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::StripVolumeRelative );
	pAction->setValue( nValue );
	pAction->setInstrument( nInstrument );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::SELECT_NEXT_PATTERN_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::SelectNextPattern );
	pAction->setPattern(  static_cast<int>( argv[0]->f ) );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::SELECT_ONLY_NEXT_PATTERN_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::SelectOnlyNextPattern );
	pAction->setPattern(  static_cast<int>( argv[0]->f ) );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::SELECT_AND_PLAY_PATTERN_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::SelectAndPlayPattern );
	pAction->setPattern(  static_cast<int>( argv[0]->f ) );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::FILTER_CUTOFF_LEVEL_ABSOLUTE_Handler( int nValue, int nInstrument )
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::FilterCutoffLevelAbsolute );
	pAction->setInstrument( nInstrument );
	pAction->setValue( nValue );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync( pAction );
}


void OscServer::INSTRUMENT_PITCH_Handler( lo_arg** argv, int )
{
	INFOLOG( "processing message" );

	m_pHydrogen->getCoreActionController()->setInstrumentPitch(
		static_cast<int>( argv[0]->f ), argv[1]->f );
}

void OscServer::BEATCOUNTER_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::BeatCounter ) );
}

void OscServer::TAP_TEMPO_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::TapTempo ) );
}

void OscServer::PLAYLIST_SONG_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::PlaylistSong );
	pAction->setSong( static_cast<int>( argv[0]->f ) );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::PLAYLIST_NEXT_SONG_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::PlaylistNextSong ) );
}

void OscServer::PLAYLIST_PREV_SONG_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::PlaylistPrevSong ) );
}

void OscServer::TOGGLE_METRONOME_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::ToggleMetronome ) );
}

void OscServer::SELECT_INSTRUMENT_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::SelectInstrument );
	pAction->setValue( static_cast<int>( argv[0]->f ) );

	// Null song handling done in MidiActionManager.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::UNDO_ACTION_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::UndoAction );

	// This one does also work the current song being nullptr.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::REDO_ACTION_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::RedoAction );

	// This one does also work the current song being nullptr.
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync( pAction );
}

// -------------------------------------------------------------------
// Actions required for session management.

void OscServer::NEW_SONG_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pHydrogen = m_pHydrogen;
	const auto sPath = QString::fromUtf8( &argv[0]->s );
	if ( ! H2Core::Filesystem::isPathValid(
			 H2Core::Filesystem::Artifact::Song, sPath ) ||
		 ! H2Core::Filesystem::fileWritable( sPath ) ) {
		ERRORLOG( QString( "Unable to create new song for invalid path [%1]" )
				  .arg( sPath ) );
		return;
	}

	auto pSong = H2Core::Song::getEmptySong( m_pHydrogen );
	pSong->setPath( sPath );
	m_pHydrogen->getCoreActionController()->setSong( pSong );
}

void OscServer::OPEN_SONG_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pSong = m_pHydrogen->getCoreActionController()->loadSong(
		QString::fromUtf8( &argv[0]->s ) );
	m_pHydrogen->getCoreActionController()->setSong( pSong );
}

void OscServer::SAVE_SONG_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );

	// Discarding missing samples and their corresponding instrument layer is an
	// operation with information loss. This is only allowed to be performed
	// explicitly via the GUI.
	m_pHydrogen->getCoreActionController()->saveSong( /* bKeepMissingSamples */ true );
}

void OscServer::SAVE_SONG_AS_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );

	// Discarding missing samples and their corresponding instrument layer is an
	// operation with information loss. This is only allowed to be performed
	// explicitly via the GUI.
	m_pHydrogen->getCoreActionController()->saveSongAs( QString::fromUtf8( &argv[0]->s ),
											  /* bKeepMissingSamples */ true );
}

void OscServer::SAVE_PREFERENCES_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	m_pHydrogen->getCoreActionController()->savePreferences();
}

void OscServer::QUIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	m_pHydrogen->getCoreActionController()->quit();
}

// -------------------------------------------------------------------

void OscServer::TIMELINE_ACTIVATION_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	if ( argv[0]->f != 0 ) {
		m_pHydrogen->getCoreActionController()->activateTimeline( true );
	} else {
		m_pHydrogen->getCoreActionController()->activateTimeline( false );
	}
}

void OscServer::TIMELINE_ADD_MARKER_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	m_pHydrogen->getCoreActionController()->addTempoMarker(
		static_cast<int>(std::round( argv[0]->f )), argv[1]->f);
}

void OscServer::TIMELINE_DELETE_MARKER_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	m_pHydrogen->getCoreActionController()->deleteTempoMarker(
		static_cast<int>( std::round( argv[0]->f ) ) );
}

void OscServer::JACK_TRANSPORT_ACTIVATION_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	if ( argv[0]->f != 0 ) {
		m_pHydrogen->getCoreActionController()->activateJackTransport( true );
	} else {
		m_pHydrogen->getCoreActionController()->activateJackTransport( false );
	}
}

void OscServer::JACK_TIMEBASE_MASTER_ACTIVATION_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	if ( argv[0]->f != 0 ) {
		m_pHydrogen->getCoreActionController()->activateJackTimebaseControl( true );
	} else {
		m_pHydrogen->getCoreActionController()->activateJackTimebaseControl( false );
	}
}

void OscServer::SONG_MODE_ACTIVATION_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	if ( argv[0]->f != 0 ) {
		m_pHydrogen->getCoreActionController()->activateSongMode( true );
	} else {
		m_pHydrogen->getCoreActionController()->activateSongMode( false );
	}
}

void OscServer::LOOP_MODE_ACTIVATION_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	if ( argv[0]->f != 0 ) {
		m_pHydrogen->getCoreActionController()->activateLoopMode( true );
	} else {
		m_pHydrogen->getCoreActionController()->activateLoopMode( false );
	}
}

void OscServer::RELOCATE_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	m_pHydrogen->getCoreActionController()->locateToColumn(
		static_cast<int>(std::round( argv[0]->f ) ) );
}

void OscServer::NEW_PATTERN_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	m_pHydrogen->getCoreActionController()->newPattern( QString::fromUtf8( &argv[0]->s ) );
}

void OscServer::OPEN_PATTERN_Handler( lo_arg** argv, int argc )
{
	INFOLOG( "processing message" );

	const auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	const auto pPattern = m_pHydrogen->getCoreActionController()->loadPattern(
		QString::fromUtf8( &argv[0]->s )
	);
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Unable to load pattern [%1]" )
					  .arg( QString::fromUtf8( &argv[0]->s ) ) );
		return;
	}
	m_pHydrogen->getCoreActionController()->setPattern(
		pPattern, pSong->getPatternList()->size(), false
	);
}

void OscServer::REMOVE_PATTERN_Handler( lo_arg** argv, int argc )
{
	INFOLOG( "processing message" );
	m_pHydrogen->getCoreActionController()->removePattern(
		static_cast<int>(std::round( argv[0]->f )) );
}

void OscServer::CLEAR_SELECTED_INSTRUMENT_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	auto pHydrogen = m_pHydrogen;
	const int nInstr = pHydrogen->getSelectedInstrumentNumber();
	if ( nInstr == -1 ) {
		WARNINGLOG( "No instrument selected" );
		return;
	}

	m_pHydrogen->getCoreActionController()->clearInstrumentInPattern( nInstr );
}

void OscServer::CLEAR_INSTRUMENT_Handler(lo_arg **argv,int i)
{
	INFOLOG( "processing message" );
	m_pHydrogen->getCoreActionController()->clearInstrumentInPattern(
		static_cast<int>(std::round( argv[0]->f )) );
}

void OscServer::CLEAR_PATTERN_Handler( lo_arg **argv, int i )
{
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::ClearPattern );
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::COUNT_IN_Handler( lo_arg **argv, int i ) {
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>( MidiAction::Type::CountIn );
	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::COUNT_IN_PAUSE_TOGGLE_Handler( lo_arg **argv, int i ) {
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::CountInPauseToggle );
		m_pHydrogen->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::COUNT_IN_STOP_TOGGLE_Handler( lo_arg **argv, int i ) {
	INFOLOG( "processing message" );
	auto pAction = std::make_shared<MidiAction>(
		MidiAction::Type::CountInStopToggle );
		m_pHydrogen->getMidiActionManager()->handleMidiActionAsync( pAction );
}

void OscServer::NOTE_ON_Handler( lo_arg **argv, int i )
{
	const int nNote = static_cast<int>( std::round( argv[0]->f ) );
	if ( nNote < static_cast<int>( H2Core::Midi::NoteMinimum ) ||
		 nNote > static_cast<int>( H2Core::Midi::NoteMaximum ) ) {
		ERRORLOG( QString( "Provided note [%1] out of bound [%2,%3]." )
					  .arg( nNote )
					  .arg( static_cast<int>( H2Core::Midi::NoteMinimum ) )
					  .arg( static_cast<int>( H2Core::Midi::NoteMaximum ) ) );
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

	m_pHydrogen->getCoreActionController()->handleNote(
		H2Core::Midi::noteFromInt( nNote ), H2Core::Midi::ChannelAll, fVelocity,
		false
	);
}

void OscServer::NOTE_OFF_Handler( lo_arg** argv, int i )
{
	const int nNote = static_cast<int>( std::round( argv[0]->f ) );
	if ( nNote < static_cast<int>( H2Core::Midi::NoteMinimum ) ||
		 nNote > static_cast<int>( H2Core::Midi::NoteMaximum ) ) {
		ERRORLOG( QString( "Provided note [%1] out of bound [%2,%3]." )
					  .arg( nNote )
					  .arg( static_cast<int>( H2Core::Midi::NoteMinimum ) )
					  .arg( static_cast<int>( H2Core::Midi::NoteMaximum ) ) );
		return;
	}

	INFOLOG( QString( "processing message with note: [%1]" ).arg( nNote ) );

	m_pHydrogen->getCoreActionController()->handleNote(
		H2Core::Midi::noteFromInt( nNote ), H2Core::Midi::ChannelAll, 0.0, true
	);
}

void OscServer::SONG_EDITOR_TOGGLE_GRID_CELL_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	m_pHydrogen->getCoreActionController()->toggleGridCell(
		H2Core::GridPoint( static_cast<int>(std::round( argv[0]->f )),
						   static_cast<int>(std::round( argv[1]->f )) ) );
}

void OscServer::LOAD_DRUMKIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );

	const QString sDrumkitName = QString::fromUtf8( &argv[0]->s );

	auto pDB = m_pHydrogen->getSoundLibraryDatabase();
	auto pDrumkit = pDB->getDrumkit( pDB->findArtifact(
		H2Core::Filesystem::Artifact::DrumkitExtracted,
		H2Core::Filesystem::Context::User, sDrumkitName, true
	) );
	if ( pDrumkit == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve drumkit called [%1]" )
					  .arg( sDrumkitName ) );
		return;
	}

	m_pHydrogen->getCoreActionController()->setDrumkit( pDrumkit );
}

void OscServer::LOAD_NEXT_DRUMKIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );

	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::LoadNextDrumkit ) );
}

void OscServer::LOAD_PREV_DRUMKIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );

	m_pHydrogen->getMidiActionManager()->handleMidiActionAsync(
		std::make_shared<MidiAction>( MidiAction::Type::LoadPrevDrumkit ) );
}

void OscServer::UPGRADE_DRUMKIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	QString sNewPath = "";
	if ( argc > 1 ) {
		sNewPath = QString::fromUtf8( &argv[1]->s );
	}
	
	m_pHydrogen->getCoreActionController()->upgradeDrumkit(
		QString::fromUtf8( &argv[0]->s ), sNewPath );
}

void OscServer::VALIDATE_DRUMKIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );

	bool bValidateLegacyKits = false;
	if ( argc > 1 ) {
		bValidateLegacyKits = argv[1]->f == 0 ? false : true;
	}
	
	m_pHydrogen->getCoreActionController()->validateDrumkit(
		QString::fromUtf8( &argv[0]->s ), bValidateLegacyKits );
}

void OscServer::EXTRACT_DRUMKIT_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	QString sTargetDir = "";
	if ( argc > 1 ) {
		sTargetDir = QString::fromUtf8( &argv[1]->s );
	}
	
	m_pHydrogen->getCoreActionController()->extractDrumkit(
		QString::fromUtf8( &argv[0]->s ), sTargetDir );
}

void OscServer::NEW_PLAYLIST_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pPlaylist = std::make_shared<H2Core::Playlist>();
	m_pHydrogen->getCoreActionController()->setPlaylist( pPlaylist );
}

void OscServer::OPEN_PLAYLIST_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pPlaylist = H2Core::Playlist::load( QString::fromUtf8( &argv[0]->s ),
											 m_pHydrogen->getPreferences() );
	if ( pPlaylist == nullptr ) {
		ERRORLOG( QString( "Unable to load Playlist [%1]" )
				  .arg( QString::fromUtf8( &argv[0]->s ) ) );
		return;
	}

	m_pHydrogen->getCoreActionController()->setPlaylist( pPlaylist );
}

void OscServer::SAVE_PLAYLIST_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	m_pHydrogen->getCoreActionController()->savePlaylist();
}

void OscServer::SAVE_PLAYLIST_AS_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	m_pHydrogen->getCoreActionController()->savePlaylistAs( QString::fromUtf8( &argv[0]->s ) );
}

void OscServer::PLAYLIST_ADD_SONG_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pEntry = std::make_shared<H2Core::PlaylistEntry>();
	pEntry->setSongPath( QString::fromUtf8( &argv[0]->s ) );
	// Append at the end
	m_pHydrogen->getCoreActionController()->addToPlaylist( pEntry, -1 );
}

void OscServer::PLAYLIST_ADD_CURRENT_SONG_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pSong = m_pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set" );
		return;
	}
	auto pEntry = std::make_shared<H2Core::PlaylistEntry>();
	pEntry->setSongPath( pSong->getPath() );
	// Append at the end
	m_pHydrogen->getCoreActionController()->addToPlaylist( pEntry, -1 );
}

void OscServer::PLAYLIST_REMOVE_SONG_Handler(lo_arg **argv, int argc) {
	INFOLOG( "processing message" );
	auto pPlaylist = m_pHydrogen->getPlaylist();
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
	m_pHydrogen->getCoreActionController()->removeFromPlaylist( pEntry, nIndex );
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

void OscServer::sendFeedbackMessage(
	MidiAction::Type type,
	float fValue,
	int nInstrumentIndex
)
{
	if ( !m_pHydrogen->getPreferences()->getOscFeedbackEnabled() ) {
		return;
	}

	if ( type == MidiAction::Type::MasterVolumeAbsolute ) {
		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		broadcastMessage( "/Hydrogen/MASTER_VOLUME_ABSOLUTE", reply );

		lo_message_free( reply );
	}

	if ( type == MidiAction::Type::StripVolumeAbsolute ) {
		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		QByteArray ba = QString( "/Hydrogen/STRIP_VOLUME_ABSOLUTE/%1" )
							.arg( nInstrumentIndex )
							.toLatin1();
		const char* c_str2 = ba.data();

		broadcastMessage( c_str2, reply );

		lo_message_free( reply );
	}

	if ( type == MidiAction::Type::ToggleMetronome ) {
		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		broadcastMessage( "/Hydrogen/TOGGLE_METRONOME", reply );

		lo_message_free( reply );
	}

	if ( type == MidiAction::Type::MuteToggle ) {
		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		broadcastMessage( "/Hydrogen/MUTE_TOGGLE", reply );

		lo_message_free( reply );
	}

	if ( type == MidiAction::Type::StripMuteToggle ) {
		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		QByteArray ba = QString( "/Hydrogen/STRIP_MUTE_TOGGLE/%1" )
							.arg( nInstrumentIndex )
							.toLatin1();
		const char* c_str2 = ba.data();

		broadcastMessage( c_str2, reply );

		lo_message_free( reply );
	}

	if ( type == MidiAction::Type::StripSoloToggle ) {
		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		QByteArray ba = QString( "/Hydrogen/STRIP_SOLO_TOGGLE/%1" )
							.arg( nInstrumentIndex )
							.toLatin1();
		const char* c_str2 = ba.data();

		broadcastMessage( c_str2, reply );

		lo_message_free( reply );
	}

	if ( type == MidiAction::Type::PanAbsolute ) {
		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		QByteArray ba = QString( "/Hydrogen/PAN_ABSOLUTE/%1" )
							.arg( nInstrumentIndex )
							.toLatin1();
		const char* c_str2 = ba.data();

		broadcastMessage( c_str2, reply );

		lo_message_free( reply );
	}

	if ( type == MidiAction::Type::PanAbsoluteSym ) {
		lo_message reply = lo_message_new();
		lo_message_add_float( reply, fValue );

		QByteArray ba = QString( "/Hydrogen/PAN_ABSOLUTE_SYM/%1" )
							.arg( nInstrumentIndex )
							.toLatin1();
		const char* c_str2 = ba.data();

		broadcastMessage( c_str2, reply );

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
										
			m_pHydrogen->getCoreActionController()->initExternalControlInterfaces();
		}
									
		// Returning 1 means that the
		// message has not been fully
		// handled and the server should
		// try other methods.
		return 1;
	});

	m_pServerThread->add_method(nullptr, nullptr, incomingMessageLogging, nullptr);

	addMethod("/Hydrogen/PLAY", "", &OscServer::PLAY_Handler);
	addMethod("/Hydrogen/PLAY", "f", &OscServer::PLAY_Handler);
	addMethod("/Hydrogen/PLAY_STOP_TOGGLE", "", &OscServer::PLAY_STOP_TOGGLE_Handler);
	addMethod("/Hydrogen/PLAY_STOP_TOGGLE", "f", &OscServer::PLAY_STOP_TOGGLE_Handler);
	addMethod("/Hydrogen/PLAY_PAUSE_TOGGLE", "", &OscServer::PLAY_PAUSE_TOGGLE_Handler);
	addMethod("/Hydrogen/PLAY_PAUSE_TOGGLE", "f", &OscServer::PLAY_PAUSE_TOGGLE_Handler);
	addMethod("/Hydrogen/STOP", "", &OscServer::STOP_Handler);
	addMethod("/Hydrogen/STOP", "f", &OscServer::STOP_Handler);
	addMethod("/Hydrogen/PAUSE", "", &OscServer::PAUSE_Handler);
	addMethod("/Hydrogen/PAUSE", "f", &OscServer::PAUSE_Handler);
	
	addMethod("/Hydrogen/RECORD_READY", "", &OscServer::RECORD_READY_Handler);
	addMethod("/Hydrogen/RECORD_READY", "f", &OscServer::RECORD_READY_Handler);
	addMethod("/Hydrogen/RECORD_STROBE_TOGGLE", "", &OscServer::RECORD_STROBE_TOGGLE_Handler);
	addMethod("/Hydrogen/RECORD_STROBE_TOGGLE", "f", &OscServer::RECORD_STROBE_TOGGLE_Handler);
	addMethod("/Hydrogen/RECORD_STROBE", "", &OscServer::RECORD_STROBE_Handler);
	addMethod("/Hydrogen/RECORD_STROBE", "f", &OscServer::RECORD_STROBE_Handler);
	addMethod("/Hydrogen/RECORD_EXIT", "", &OscServer::RECORD_EXIT_Handler);
	addMethod("/Hydrogen/RECORD_EXIT", "f", &OscServer::RECORD_EXIT_Handler);
	
	addMethod("/Hydrogen/MUTE", "", &OscServer::MUTE_Handler);
	addMethod("/Hydrogen/MUTE", "f", &OscServer::MUTE_Handler);
	addMethod("/Hydrogen/UNMUTE", "", &OscServer::UNMUTE_Handler);
	addMethod("/Hydrogen/UNMUTE", "f", &OscServer::UNMUTE_Handler);
	addMethod("/Hydrogen/MUTE_TOGGLE", "", &OscServer::MUTE_TOGGLE_Handler);
	addMethod("/Hydrogen/MUTE_TOGGLE", "f", &OscServer::MUTE_TOGGLE_Handler);

	addMethod("/Hydrogen/INSTRUMENT_PITCH", "ff", &OscServer::INSTRUMENT_PITCH_Handler);
	
	addMethod("/Hydrogen/NEXT_BAR", "", &OscServer::NEXT_BAR_Handler);
	addMethod("/Hydrogen/NEXT_BAR", "f", &OscServer::NEXT_BAR_Handler);
	addMethod("/Hydrogen/PREVIOUS_BAR", "", &OscServer::PREVIOUS_BAR_Handler);
	addMethod("/Hydrogen/PREVIOUS_BAR", "f", &OscServer::PREVIOUS_BAR_Handler);
	
	addMethod("/Hydrogen/BPM", "f", &OscServer::BPM_Handler);
	addMethod("/Hydrogen/BPM_DECR", "f", &OscServer::BPM_DECR_Handler);
	addMethod("/Hydrogen/BPM_INCR", "f", &OscServer::BPM_INCR_Handler);

	addMethod("/Hydrogen/MASTER_VOLUME_ABSOLUTE", "f", &OscServer::MASTER_VOLUME_ABSOLUTE_Handler);
	addMethod("/Hydrogen/MASTER_VOLUME_RELATIVE", "f", &OscServer::MASTER_VOLUME_RELATIVE_Handler);
	addMethod("/Hydrogen/HUMANIZATION_SWING_ABSOLUTE", "f", &OscServer::HUMANIZATION_SWING_ABSOLUTE_Handler);
	addMethod("/Hydrogen/HUMANIZATION_SWING_RELATIVE", "f", &OscServer::HUMANIZATION_SWING_RELATIVE_Handler);
	addMethod("/Hydrogen/HUMANIZATION_TIMING_ABSOLUTE", "f", &OscServer::HUMANIZATION_TIMING_ABSOLUTE_Handler);
	addMethod("/Hydrogen/HUMANIZATION_TIMING_RELATIVE", "f", &OscServer::HUMANIZATION_TIMING_RELATIVE_Handler);
	addMethod("/Hydrogen/HUMANIZATION_VELOCITY_ABSOLUTE", "f", &OscServer::HUMANIZATION_VELOCITY_ABSOLUTE_Handler);
	addMethod("/Hydrogen/HUMANIZATION_VELOCITY_RELATIVE", "f", &OscServer::HUMANIZATION_VELOCITY_RELATIVE_Handler);

	addMethod("/Hydrogen/SELECT_NEXT_PATTERN", "f", &OscServer::SELECT_NEXT_PATTERN_Handler);
	addMethod("/Hydrogen/SELECT_ONLY_NEXT_PATTERN", "f", &OscServer::SELECT_ONLY_NEXT_PATTERN_Handler);
	addMethod("/Hydrogen/SELECT_AND_PLAY_PATTERN", "f", &OscServer::SELECT_AND_PLAY_PATTERN_Handler);
	
	addMethod("/Hydrogen/BEATCOUNTER", "", &OscServer::BEATCOUNTER_Handler);
	addMethod("/Hydrogen/BEATCOUNTER", "f", &OscServer::BEATCOUNTER_Handler);
	
	addMethod("/Hydrogen/TAP_TEMPO", "", &OscServer::TAP_TEMPO_Handler);
	addMethod("/Hydrogen/TAP_TEMPO", "f", &OscServer::TAP_TEMPO_Handler);
	
	addMethod("/Hydrogen/PLAYLIST_SONG", "f", &OscServer::PLAYLIST_SONG_Handler);
	addMethod("/Hydrogen/PLAYLIST_NEXT_SONG", "", &OscServer::PLAYLIST_NEXT_SONG_Handler);
	addMethod("/Hydrogen/PLAYLIST_NEXT_SONG", "f", &OscServer::PLAYLIST_NEXT_SONG_Handler);
	addMethod("/Hydrogen/PLAYLIST_PREV_SONG", "", &OscServer::PLAYLIST_PREV_SONG_Handler);
	addMethod("/Hydrogen/PLAYLIST_PREV_SONG", "f", &OscServer::PLAYLIST_PREV_SONG_Handler);
	
	addMethod("/Hydrogen/TOGGLE_METRONOME", "", &OscServer::TOGGLE_METRONOME_Handler);
	addMethod("/Hydrogen/TOGGLE_METRONOME", "f", &OscServer::TOGGLE_METRONOME_Handler);
	
	addMethod("/Hydrogen/SELECT_INSTRUMENT", "f", &OscServer::SELECT_INSTRUMENT_Handler);
	
	addMethod("/Hydrogen/UNDO_ACTION", "", &OscServer::UNDO_ACTION_Handler);
	addMethod("/Hydrogen/UNDO_ACTION", "f", &OscServer::UNDO_ACTION_Handler);
	addMethod("/Hydrogen/REDO_ACTION", "", &OscServer::REDO_ACTION_Handler);
	addMethod("/Hydrogen/REDO_ACTION", "f", &OscServer::REDO_ACTION_Handler);

	addMethod("/Hydrogen/NEW_SONG", "s", &OscServer::NEW_SONG_Handler);
	addMethod("/Hydrogen/OPEN_SONG", "s", &OscServer::OPEN_SONG_Handler);
	addMethod("/Hydrogen/SAVE_SONG", "", &OscServer::SAVE_SONG_Handler);
	addMethod("/Hydrogen/SAVE_SONG", "f", &OscServer::SAVE_SONG_Handler);
	addMethod("/Hydrogen/SAVE_SONG_AS", "s", &OscServer::SAVE_SONG_AS_Handler);
	addMethod("/Hydrogen/SAVE_PREFERENCES", "", &OscServer::SAVE_PREFERENCES_Handler);
	addMethod("/Hydrogen/SAVE_PREFERENCES", "f", &OscServer::SAVE_PREFERENCES_Handler);
	addMethod("/Hydrogen/QUIT", "", &OscServer::QUIT_Handler);
	addMethod("/Hydrogen/QUIT", "f", &OscServer::QUIT_Handler);

	addMethod("/Hydrogen/TIMELINE_ACTIVATION", "f", &OscServer::TIMELINE_ACTIVATION_Handler);
	addMethod("/Hydrogen/TIMELINE_ADD_MARKER", "ff", &OscServer::TIMELINE_ADD_MARKER_Handler);
	addMethod("/Hydrogen/TIMELINE_DELETE_MARKER", "f", &OscServer::TIMELINE_DELETE_MARKER_Handler);

	addMethod("/Hydrogen/JACK_TRANSPORT_ACTIVATION", "f", &OscServer::JACK_TRANSPORT_ACTIVATION_Handler);
	addMethod("/Hydrogen/JACK_TIMEBASE_MASTER_ACTIVATION", "f", &OscServer::JACK_TIMEBASE_MASTER_ACTIVATION_Handler);
	addMethod("/Hydrogen/SONG_MODE_ACTIVATION", "f", &OscServer::SONG_MODE_ACTIVATION_Handler);
	addMethod("/Hydrogen/LOOP_MODE_ACTIVATION", "f", &OscServer::LOOP_MODE_ACTIVATION_Handler);
	addMethod("/Hydrogen/RELOCATE", "f", &OscServer::RELOCATE_Handler);
	addMethod("/Hydrogen/NEW_PATTERN", "s", &OscServer::NEW_PATTERN_Handler);
	addMethod("/Hydrogen/OPEN_PATTERN", "s", &OscServer::OPEN_PATTERN_Handler);
	addMethod("/Hydrogen/REMOVE_PATTERN", "f", &OscServer::REMOVE_PATTERN_Handler);
	addMethod("/Hydrogen/CLEAR_INSTRUMENT", "f", &OscServer::CLEAR_INSTRUMENT_Handler);
	addMethod("/Hydrogen/CLEAR_SELECTED_INSTRUMENT", "", &OscServer::CLEAR_SELECTED_INSTRUMENT_Handler);
	addMethod("/Hydrogen/CLEAR_SELECTED_INSTRUMENT", "f", &OscServer::CLEAR_SELECTED_INSTRUMENT_Handler);
	addMethod("/Hydrogen/CLEAR_PATTERN", "", &OscServer::CLEAR_PATTERN_Handler);
	addMethod("/Hydrogen/CLEAR_PATTERN", "f", &OscServer::CLEAR_PATTERN_Handler);
	addMethod("/Hydrogen/COUNT_IN", "", &OscServer::COUNT_IN_Handler);
	addMethod("/Hydrogen/COUNT_IN", "f", &OscServer::COUNT_IN_Handler);
	addMethod("/Hydrogen/COUNT_IN_PAUSE_TOGGLE", "", &OscServer::COUNT_IN_PAUSE_TOGGLE_Handler);
	addMethod("/Hydrogen/COUNT_IN_PAUSE_TOGGLE", "f", &OscServer::COUNT_IN_PAUSE_TOGGLE_Handler);
	addMethod("/Hydrogen/COUNT_IN_STOP_TOGGLE", "", &OscServer::COUNT_IN_STOP_TOGGLE_Handler);
	addMethod("/Hydrogen/COUNT_IN_STOP_TOGGLE", "f", &OscServer::COUNT_IN_STOP_TOGGLE_Handler);

	addMethod("/Hydrogen/NOTE_ON", "ff", &OscServer::NOTE_ON_Handler);
	addMethod("/Hydrogen/NOTE_OFF", "f", &OscServer::NOTE_OFF_Handler);

	addMethod("/Hydrogen/SONG_EDITOR_TOGGLE_GRID_CELL", "ff", &OscServer::SONG_EDITOR_TOGGLE_GRID_CELL_Handler);
	addMethod("/Hydrogen/LOAD_DRUMKIT", "s", &OscServer::LOAD_DRUMKIT_Handler);
	addMethod("/Hydrogen/LOAD_PREV_DRUMKIT", "", &OscServer::LOAD_PREV_DRUMKIT_Handler);
	addMethod("/Hydrogen/LOAD_PREV_DRUMKIT", "f", &OscServer::LOAD_PREV_DRUMKIT_Handler);
	addMethod("/Hydrogen/LOAD_NEXT_DRUMKIT", "", &OscServer::LOAD_NEXT_DRUMKIT_Handler);
	addMethod("/Hydrogen/LOAD_NEXT_DRUMKIT", "f", &OscServer::LOAD_NEXT_DRUMKIT_Handler);
	addMethod("/Hydrogen/UPGRADE_DRUMKIT", "s", &OscServer::UPGRADE_DRUMKIT_Handler);
	addMethod("/Hydrogen/UPGRADE_DRUMKIT", "ss", &OscServer::UPGRADE_DRUMKIT_Handler);
	addMethod("/Hydrogen/VALIDATE_DRUMKIT", "s", &OscServer::VALIDATE_DRUMKIT_Handler);
	addMethod("/Hydrogen/VALIDATE_DRUMKIT", "sf", &OscServer::VALIDATE_DRUMKIT_Handler);
	addMethod("/Hydrogen/EXTRACT_DRUMKIT", "s", &OscServer::EXTRACT_DRUMKIT_Handler);
	addMethod("/Hydrogen/EXTRACT_DRUMKIT", "ss", &OscServer::EXTRACT_DRUMKIT_Handler);

	addMethod("/Hydrogen/NEW_PLAYLIST", "", &OscServer::NEW_PLAYLIST_Handler);
	addMethod("/Hydrogen/NEW_PLAYLIST", "f", &OscServer::NEW_PLAYLIST_Handler);
	addMethod("/Hydrogen/OPEN_PLAYLIST", "s", &OscServer::OPEN_PLAYLIST_Handler);
	addMethod("/Hydrogen/SAVE_PLAYLIST", "", &OscServer::SAVE_PLAYLIST_Handler);
	addMethod("/Hydrogen/SAVE_PLAYLIST", "f", &OscServer::SAVE_PLAYLIST_Handler);
	addMethod("/Hydrogen/SAVE_PLAYLIST_AS", "s", &OscServer::SAVE_PLAYLIST_AS_Handler);
	addMethod("/Hydrogen/PLAYLIST_ADD_SONG", "s", &OscServer::PLAYLIST_ADD_SONG_Handler);
	addMethod("/Hydrogen/PLAYLIST_ADD_CURRENT_SONG", "", &OscServer::PLAYLIST_ADD_CURRENT_SONG_Handler);
	addMethod("/Hydrogen/PLAYLIST_ADD_CURRENT_SONG", "f", &OscServer::PLAYLIST_ADD_CURRENT_SONG_Handler);
	addMethod("/Hydrogen/PLAYLIST_REMOVE_SONG", "f", &OscServer::PLAYLIST_REMOVE_SONG_Handler);

	// generic_handler is a static liblo C-callback; it reaches its owning
	// OscServer (and thus Hydrogen) through the user_data we bind here (ADR 0015).
	m_pServerThread->add_method(nullptr, nullptr, generic_handler, this);

	m_bInitialized = true;
	
	return true;
}

bool OscServer::start() {
	ASSERT_NO_EDITOR_MODE( m_pHydrogen );
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
	const auto pPref = m_pHydrogen->getPreferences();
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

} // namespace H2Core

#endif /* H2CORE_HAVE_OSC */

