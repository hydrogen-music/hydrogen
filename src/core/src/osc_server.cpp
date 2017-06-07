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


#include "hydrogen/helpers/filesystem.h"
#include "hydrogen/Preferences.h"

#include <pthread.h>
#include <unistd.h>

//currently H2CORE_HAVE_OSC means: liblo is present..
#ifdef H2CORE_HAVE_OSC

#include <lo/lo.h>
#include <lo/lo_cpp.h>

#include "hydrogen/osc_server.h"
#include "hydrogen/event_queue.h"
#include "hydrogen/hydrogen.h"
#include "hydrogen/basics/song.h"
#include "hydrogen/midi_action.h"

OscServer * OscServer::__instance = 0;
const char* OscServer::__class_name = "OscServer";
std::list<lo_address> OscServer::m_pClientRegistry;

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
		lo_timetag tt;
	} h2_pcast64;


	h2_pcast32 val32;
	h2_pcast64 val64;
	int size;
	int i;

	size = lo_arg_size(type, data);
	if (size == 4 || type == LO_BLOB) {
			val32.nl = *(int32_t *)data;
	} else if (size == 8) {
			val64.nl = *(int64_t *)data;
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

		case LO_TIMETAG:
			formattedString = QString("%1.%2").arg(val64.tt.sec).arg(val64.tt.frac);
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

		default:
			formattedString = QString("Unhandled type:").arg(type);
			break;
	}

	return formattedString;

}


/* catch any incoming messages and display them. returning 1 means that the
 * message has not been fully handled and the server should try other methods */
int OscServer::generic_handler(const char *	path, 
							   const char *	types,
							   lo_arg **	argv,
							   int			argc, 
							   void *		data, 
							   void *		user_data)
{
	INFOLOG("GENERIC HANDLER");

	//First we're trying to map TouchOSC messages from multi-fader widgets
	QString oscPath( path );
	QRegExp rxStripVol( "/Hydrogen/STRIP_VOLUME_ABSOLUTE/(\\d+)" );
	int pos = rxStripVol.indexIn( oscPath );
	if ( pos > -1 ) {
		if( argc == 1 ){
			int value = rxStripVol.cap(1).toInt() -1;
			STRIP_VOLUME_ABSOLUTE_Handler( value , argv[0]->f );
		}
	}
	
	QRegExp rxStripPanAbs( "/Hydrogen/PAN_ABSOLUTE/(\\d+)" );
	pos = rxStripPanAbs.indexIn( oscPath );
	if ( pos > -1 ) {
		if( argc == 1 ){
			int value = rxStripPanAbs.cap(1).toInt() - 1;
			PAN_ABSOLUTE_Handler( QString::number( value ) , QString::number( argv[0]->f, 'f', 0) );
		}
	}
	
	QRegExp rxStripPanRel( "/Hydrogen/PAN_RELATIVE/(\\d+)" );
	pos = rxStripPanRel.indexIn( oscPath );
	if ( pos > -1 ) {
		if( argc == 1 ){
			int value = rxStripPanRel.cap(1).toInt() - 1;
			PAN_RELATIVE_Handler( QString::number( value ) , QString::number( argv[0]->f, 'f', 0) );
		}
	}
	
	QRegExp rxStripFilterCutoffAbs( "/Hydrogen/FILTER_CUTOFF_LEVEL_ABSOLUTE/(\\d+)" );
	pos = rxStripFilterCutoffAbs.indexIn( oscPath );
	if ( pos > -1 ) {
		if( argc == 1 ){
			int value = rxStripFilterCutoffAbs.cap(1).toInt() - 1;
			FILTER_CUTOFF_LEVEL_ABSOLUTE_Handler( QString::number( value ) , QString::number( argv[0]->f, 'f', 0) );
		}
	}
	

	INFOLOG( QString( "Incoming OSC Message for path %1" ).arg( path ) );
	int i;
	for (i = 0; i < argc; i++) {
		QString formattedArgument = qPrettyPrint( (lo_type)types[i], argv[i] );
		INFOLOG(QString("Argument %1: %2 %3").arg(i).arg(types[i]).arg(formattedArgument));
	}
	
	
	return 1;
}



OscServer::OscServer( H2Core::Preferences * pPreferences )
	: Object( __class_name )
{
	m_pPreferences = pPreferences;
	int port = m_pPreferences->getOscServerPort();

	m_pServerThread = new lo::ServerThread( port );
}

void OscServer::create_instance( H2Core::Preferences* pPreferences )
{
	if( __instance == 0 ) {
		__instance = new OscServer( pPreferences );
	}
}

void OscServer::PLAY_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("PLAY");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::PLAY_STOP_TOGGLE_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("PLAY/STOP_TOGGLE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::PLAY_PAUSE_TOGGLE_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("PLAY/PAUSE_TOGGLE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::STOP_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("STOP");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::PAUSE_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("PAUSE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::RECORD_READY_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("RECORD_READY");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::RECORD_STROBE_TOGGLE_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("RECORD/STROBE_TOGGLE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::RECORD_STROBE_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("RECORD_STROBE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::RECORD_EXIT_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("RECORD_EXIT");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::MUTE_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("MUTE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::UNMUTE_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("UNMUTE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::MUTE_TOGGLE_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("MUTE_TOGGLE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::NEXT_BAR_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action(">>_NEXT_BAR");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::PREVIOUS_BAR_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("<<_PREVIOUS_BAR");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::BPM_INCR_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("BPM_INCR");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();
	
	pAction->setParameter1( QString::number( argv[0]->f, 'f', 0));

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::BPM_DECR_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("BPM_DECR");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pAction->setParameter1( QString::number( argv[0]->f, 'f', 0));
	
	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::MASTER_VOLUME_ABSOLUTE_Handler(lo_arg **argv,int i)
{
	H2Core::Hydrogen *pEngine = H2Core::Hydrogen::get_instance();
	H2Core::CoreActionController* pController = pEngine->getCoreActionController();

	pController->setMasterVolume( argv[0]->f );
}

void OscServer::MASTER_VOLUME_RELATIVE_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("MASTER_VOLUME_RELATIVE");
	pAction->setParameter2( QString::number( argv[0]->f, 'f', 0));
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::STRIP_VOLUME_ABSOLUTE_Handler(int param1, float param2)
{
	H2Core::Hydrogen *pEngine = H2Core::Hydrogen::get_instance();
	H2Core::CoreActionController* pController = pEngine->getCoreActionController();

	pController->setStripVolume( param1, param2 );
}

void OscServer::STRIP_VOLUME_RELATIVE_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("STRIP_VOLUME_RELATIVE");
	pAction->setParameter2( QString::number( argv[0]->f, 'f', 0));
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}


void OscServer::SELECT_NEXT_PATTERN_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("SELECT_NEXT_PATTERN");
	pAction->setParameter1(  QString::number( argv[0]->f, 'f', 0 ) );
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::SELECT_NEXT_PATTERN_PROMPTLY_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("SELECT_NEXT_PATTERN_PROMPTLY");
	pAction->setParameter1(  QString::number( argv[0]->f, 'f', 0 ) );
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::SELECT_AND_PLAY_PATTERN_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("SELECT_AND_PLAY_PATTERN");
	pAction->setParameter1(  QString::number( argv[0]->f, 'f', 0 ) );
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::PAN_ABSOLUTE_Handler(QString param1, QString param2)
{
	Action* pAction = new Action("PAN_ABSOLUTE");
	pAction->setParameter1( param1 );
	pAction->setParameter2( param2 );
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::PAN_RELATIVE_Handler(QString param1, QString param2)
{
	Action* pAction = new Action("PAN_RELATIVE");
	pAction->setParameter1( param1 );
	pAction->setParameter2( param2 );
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::FILTER_CUTOFF_LEVEL_ABSOLUTE_Handler(QString param1, QString param2)
{
	Action* pAction = new Action("FILTER_CUTOFF_LEVEL_ABSOLUTE");
	pAction->setParameter1( param1 );
	pAction->setParameter2( param2 );
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::BEATCOUNTER_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("BEATCOUNTER");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}


void OscServer::TAP_TEMPO_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("TAP_TEMPO");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::PLAYLIST_SONG_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("PLAYLIST_SONG");
	pAction->setParameter1(  QString::number( argv[0]->f, 'f', 0 ) );

	MidiActionManager* pActionManager = MidiActionManager::get_instance();	

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::PLAYLIST_NEXT_SONG_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("PLAYLIST_NEXT_SONG");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::PLAYLIST_PREV_SONG_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("PLAYLIST_PREV_SONG");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::TOGGLE_METRONOME_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("TOGGLE_METRONOME");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::SELECT_INSTRUMENT_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("SELECT_INSTRUMENT");
	pAction->setParameter2(  QString::number( argv[0]->f, 'f', 0 ) );

	MidiActionManager* pActionManager = MidiActionManager::get_instance();	

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::UNDO_ACTION_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("UNDO_ACTION");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::REDO_ACTION_Handler(lo_arg **argv,int i)
{
	Action* pAction = new Action("REDO_ACTION");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}


bool IsLoAddressEqual( lo_address first, lo_address second )
{
	bool portEqual = ( strcmp( lo_address_get_port( first ), lo_address_get_port( second ) ) == 0);
	bool hostEqual = ( strcmp( lo_address_get_hostname( first ), lo_address_get_hostname( second ) ) == 0);
	bool protoEqual = ( lo_address_get_protocol( first ) == lo_address_get_protocol( second ) );
	
	return portEqual && hostEqual && protoEqual;
}


void OscServer::handleAction( Action* pAction )
{
	if( pAction->getType() == "MASTER_VOLUME_ABSOLUTE"){
		bool ok;
		float param2 = pAction->getParameter2().toFloat(&ok);
			
		lo_message reply = lo_message_new();
		lo_message_add_float(reply, param2);

		for (std::list<lo_address>::iterator it=m_pClientRegistry.begin(); it != m_pClientRegistry.end(); ++it){
			lo_address clientAddress = *it;
			lo_send_message(clientAddress, "/Hydrogen/MASTER_VOLUME_ABSOLUTE" , reply);
		}
	}
	
	if( pAction->getType() == "STRIP_VOLUME_ABSOLUTE"){
		bool ok;
		float param2 = pAction->getParameter2().toFloat(&ok);

		lo_message reply = lo_message_new();
		lo_message_add_float(reply, param2);

		QByteArray ba = QString("/Hydrogen/STRIP_VOLUME_ABSOLUTE/%1").arg(pAction->getParameter1()).toLatin1();
		const char *c_str2 = ba.data();

		for (std::list<lo_address>::iterator it=m_pClientRegistry.begin(); it != m_pClientRegistry.end(); ++it){
			lo_address clientAddress = *it;
			lo_send_message(clientAddress, c_str2, reply);
		}
	}
}


void OscServer::start()
{
	if (!m_pServerThread->is_valid()) {
		ERRORLOG("Failed to start OSC server.");
		return;
	}

	/*
	 *  Register all handler functions
	 */

	//This handler is responsible for registering clients
	m_pServerThread->add_method(NULL, NULL, [&](lo_message msg){
									INFOLOG("OSC REGISTER HANDLER");
									lo_address a = lo_message_get_source(msg);

									bool AddressRegistered = false;
									for (std::list<lo_address>::iterator it=m_pClientRegistry.begin(); it != m_pClientRegistry.end(); ++it){
										lo_address b = *it;
										if( IsLoAddressEqual(a,b) ) {
											AddressRegistered = true;
											break;
										}
									}

									if( !AddressRegistered ){
										INFOLOG("REGISTERING CLIENT");
										lo_address newAddr = lo_address_new_with_proto(	lo_address_get_protocol( a ), 
																						lo_address_get_hostname( a ),
																						lo_address_get_port( a ) );
										m_pClientRegistry.push_back( newAddr );
										
										H2Core::Hydrogen *pEngine = H2Core::Hydrogen::get_instance();
										H2Core::CoreActionController* pController = pEngine->getCoreActionController();
										
										pController->initExternalControlInterfaces();
									}

									return 1; 
								});

	m_pServerThread->add_method(NULL, NULL, generic_handler, NULL);

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
	
	m_pServerThread->add_method("/Hydrogen/NEXT_BAR", "", NEXT_BAR_Handler);
	m_pServerThread->add_method("/Hydrogen/NEXT_BAR", "f", NEXT_BAR_Handler);
	
	m_pServerThread->add_method("/Hydrogen/PREVIOUS_BAR", "", PREVIOUS_BAR_Handler);
	m_pServerThread->add_method("/Hydrogen/PREVIOUS_BAR", "f", PREVIOUS_BAR_Handler);
	
	m_pServerThread->add_method("/Hydrogen/BPM_DECR", "f", BPM_DECR_Handler);

	m_pServerThread->add_method("/Hydrogen/BPM_INCR", "f", BPM_INCR_Handler);

	m_pServerThread->add_method("/Hydrogen/MASTER_VOLUME_ABSOLUTE", "f", MASTER_VOLUME_ABSOLUTE_Handler);
	m_pServerThread->add_method("/Hydrogen/MASTER_VOLUME_RELATIVE", "f", MASTER_VOLUME_RELATIVE_Handler);
	
	m_pServerThread->add_method("/Hydrogen/STRIP_VOLUME_RELATIVE", "f", STRIP_VOLUME_RELATIVE_Handler);

	m_pServerThread->add_method("/Hydrogen/SELECT_NEXT_PATTERN", "f", SELECT_NEXT_PATTERN_Handler);
	m_pServerThread->add_method("/Hydrogen/SELECT_NEXT_PATTERN_PROMPTLY", "f", SELECT_NEXT_PATTERN_PROMPTLY_Handler);
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
	m_pServerThread->add_method("/Hydrogen/REDO_ACTION", "f", REDO_ACTION_Handler);
	
	/*
	 * Start the server.
	 */
	m_pServerThread->start();


	INFOLOG(QString("Osc server started. Listening on port %1").arg( m_pPreferences->getOscServerPort() ));
}

OscServer::~OscServer()
{
	__instance = NULL;
}


#endif /* H2CORE_HAVE_OSC */

