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

//currently H2CORE_HAVE_NSMSESSION means: liblo is present..
#ifdef H2CORE_HAVE_NSMSESSION

#include <lo/lo.h>
#include <lo/lo_cpp.h>

#include "hydrogen/osc_server.h"
#include "hydrogen/event_queue.h"
#include "hydrogen/hydrogen.h"
#include "hydrogen/basics/song.h"
#include "hydrogen/midi_action.h"

OscServer * OscServer::__instance = 0;
const char* OscServer::__class_name = "OscServer";

/* catch any incoming messages and display them. returning 1 means that the
 * message has not been fully handled and the server should try other methods */
int OscServer::generic_handler(const char *path, const char *types, lo_arg ** argv,
							   int argc, void *data, void *user_data)
{
	//First we're trying to map TouchOSC messages from multi-fader widgets
	QString oscPath(path);
	QRegExp rxlen("/Hydrogen/STRIP_VOLUME_ABSOLUTE/(\\d+)");
	int pos = rxlen.indexIn(oscPath);
	if (pos > -1) {
		if(argc == 1){
			int value = rxlen.cap(1).toInt();
			
			//Those fader groups are 1-based, where as we adress faders 0-based.
			value -= 1;
			
			STRIP_VOLUME_ABSOLUTE_Handler( QString::number( value ) , QString::number( argv[0]->f, 'f', 0) );
		}
	}
	
	//Second, do some debugging output..
	printf("path: <%s>\n", path);
	int i;
	for (i = 0; i < argc; i++) {
		printf("arg %d '%c' ", i, types[i]);
		lo_arg_pp((lo_type)types[i], argv[i]);
		printf("\n");
	}
	printf("\n");
	fflush(stdout);
	
	return 1;
}



OscServer::OscServer()
	: Object( __class_name )
{
	m_pServerThread = new lo::ServerThread(9000);
}

void OscServer::create_instance()
{
	if( __instance == 0 ) {
		__instance = new OscServer;
	}
}

void OscServer::PLAY_TOGGLE_Handler(lo_arg **argv,int i)
{
	MidiAction* pAction = new MidiAction("PLAY_TOGGLE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::PLAY_STOP_TOGGLE_Handler(lo_arg **argv,int i)
{
	MidiAction* pAction = new MidiAction("PLAY/STOP_TOGGLE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::PLAY_PAUSE_TOGGLE_Handler(lo_arg **argv,int i)
{
	MidiAction* pAction = new MidiAction("PLAY/PAUSE_TOGGLE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::STOP_Handler(lo_arg **argv,int i)
{
	MidiAction* pAction = new MidiAction("STOP");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::PAUSE_Handler(lo_arg **argv,int i)
{
	MidiAction* pAction = new MidiAction("PAUSE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::RECORD_READY_Handler(lo_arg **argv,int i)
{
	MidiAction* pAction = new MidiAction("PAUSE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::MUTE_Handler(lo_arg **argv,int i)
{
	MidiAction* pAction = new MidiAction("MUTE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::UNMUTE_Handler(lo_arg **argv,int i)
{
	MidiAction* pAction = new MidiAction("UNMUTE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::MUTE_TOGGLE_Handler(lo_arg **argv,int i)
{
	MidiAction* pAction = new MidiAction("MUTE_TOGGLE");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::NEXT_BAR_Handler(lo_arg **argv,int i)
{
	MidiAction* pAction = new MidiAction(">>_NEXT_BAR");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::PREVIOUS_BAR_Handler(lo_arg **argv,int i)
{
	MidiAction* pAction = new MidiAction("<<_PREVIOUS_BAR");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::BPM_INCR_Handler(lo_arg **argv,int i)
{
	MidiAction* pAction = new MidiAction("BPM_INCR");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();
	
	pAction->setParameter1( QString::number( argv[0]->f, 'f', 0));

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::BPM_DECR_Handler(lo_arg **argv,int i)
{
	MidiAction* pAction = new MidiAction("BPM_DECR");
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pAction->setParameter1( QString::number( argv[0]->f, 'f', 0));
	
	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::MASTER_VOLUME_ABSOLUTE_Handler(lo_arg **argv,int i)
{
	MidiAction* pAction = new MidiAction("MASTER_VOLUME_ABSOLUTE");
	pAction->setParameter2( QString::number( argv[0]->f, 'f', 0));
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::STRIP_VOLUME_ABSOLUTE_Handler(QString param1, QString param2)
{
	MidiAction* pAction = new MidiAction("STRIP_VOLUME_ABSOLUTE");
	pAction->setParameter1( param1 );
	pAction->setParameter2( param2 );
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}


void OscServer::SELECT_NEXT_PATTERN_Handler(lo_arg **argv,int i)
{
	MidiAction* pAction = new MidiAction("SELECT_NEXT_PATTERN");
	pAction->setParameter1(  QString::number( argv[0]->f, 'f', 0 ) );
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::SELECT_NEXT_PATTERN_PROMPTLY_Handler(lo_arg **argv,int i)
{
	MidiAction* pAction = new MidiAction("SELECT_NEXT_PATTERN_PROMPTLY");
	pAction->setParameter1(  QString::number( argv[0]->f, 'f', 0 ) );
	MidiActionManager* pActionManager = MidiActionManager::get_instance();

	pActionManager->handleAction(pAction);
	delete pAction;
}

void OscServer::start()
{
	if (!m_pServerThread->is_valid()) {
		ERRORLOG("Failed to start OSC server.");
		return ;
	}

	
	/*
	 *  Register all handler functions
	 */
	m_pServerThread->add_method(NULL, NULL, generic_handler, NULL);
	//lo_server_add_method(m_pServerThread, NULL, NULL, generic_handler, NULL);
	
	m_pServerThread->add_method("/Hydrogen/PLAY_TOGGLE", "", PLAY_TOGGLE_Handler);
	m_pServerThread->add_method("/Hydrogen/PLAY_TOGGLE", "f", PLAY_TOGGLE_Handler);

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

/*	
	m_pServerThread->add_method("/Hydrogen/RECORD_STROBE_TOGGLE", "", RECORD_STROBE_TOGGLE_Handler);
	m_pServerThread->add_method("/Hydrogen/RECORD_STROBE_TOGGLE", "f", RECORD_STROBE_TOGGLE_Handler);

	m_pServerThread->add_method("/Hydrogen/RECORD_STROBE", "", RECORD_STROBE_Handler);
	m_pServerThread->add_method("/Hydrogen/RECORD_STROBE", "f", RECORD_STROBE_Handler);
*/
	m_pServerThread->add_method("/Hydrogen/MASTER_VOLUME_ABSOLUTE", "f", MASTER_VOLUME_ABSOLUTE_Handler);
	
	//This is handled by the generic handler.
	//m_pServerThread->add_method("/Hydrogen/STRIP_VOLUME_ABSOLUTE", "f", STRIP_VOLUME_ABSOLUTE_Handler);

	m_pServerThread->add_method("/Hydrogen/SELECT_NEXT_PATTERN", "f", SELECT_NEXT_PATTERN_Handler);
	m_pServerThread->add_method("/Hydrogen/SELECT_NEXT_PATTERN_PROMPTLY", "f", SELECT_NEXT_PATTERN_PROMPTLY_Handler);
	m_pServerThread->add_method("/Hydrogen/SELECT_AND_PLAY_PATTERN", "f", SELECT_AND_PLAY_PATTERN_Handler);

	
	/*
	 * Start the server.
	 */
	m_pServerThread->start();
}

OscServer::~OscServer()
{
	__instance = NULL;
}


#endif /* H2CORE_HAVE_NSMSESSION */

