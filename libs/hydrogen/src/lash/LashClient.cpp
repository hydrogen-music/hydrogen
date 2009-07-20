/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

/**
 * LASH support for Hydrogen 0.9.3.
 *
 * Created by Jaakko Sipari (jaakko.sipari@gmail.com)
 */

#include "config.h"

#ifdef LASH_SUPPORT

#include <lash-1.0/lash/lash.h>
#include <hydrogen/LashClient.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/h2_exception.h>

using namespace H2Core;

LashClient* LashClient::__instance = NULL; /// static reference of LashClient class (Singleton)

void LashClient::create_instance( const char *lashClass,
				  const char *viewName,
				  int *argc,
				  char ***argv )
{
	__instance = new LashClient(lashClass, viewName, argc, argv);
}

LashClient::LashClient(const char* lashClass, const char* viewName, int* argc, char*** argv)
{
	__instance = this;

	if ( H2Core::Preferences::get_instance()->useLash() ){
		newProject = true;
		lash_args_t *lash_args = lash_extract_args(argc, argv);
		lashClient = lash_init(lash_args, lashClass, LASH_Config_File, LASH_PROTOCOL(2, 0));
		
		if (isConnected())
		{
			sendEvent(LASH_Client_Name, viewName);
		}
	}	

}

LashClient::~LashClient()
{
	
}

bool LashClient::isConnected()
{
	return lash_enabled(lashClient);
}


void LashClient::setNewProject(bool value)
{
	newProject = value;
}

bool LashClient::isNewProject()
{
	return newProject;
}

lash_client_t* LashClient::getConnection()
{
	return lashClient;
}

lash_event_t* LashClient::getNextEvent()
{
	lash_event_t* event = lash_get_event(lashClient);
	return event;
}

void LashClient::sendEvent(LASH_Event_Type eventType)
{
	sendEvent(eventType, NULL);
}

void LashClient::sendEvent(LASH_Event_Type eventType, const char* value)
{
	lash_event_t *event = lash_event_new_with_type(eventType);
	if (value != NULL)
	{
		lash_event_set_string(event, value);
	}
	lash_send_event(lashClient, event);
}

void LashClient::setJackClientName( const std::string& name )
{
	jackClientName = name;
}

void LashClient::sendJackClientName()
{
	lash_jack_client_name(lashClient, jackClientName.c_str());
}

void LashClient::setAlsaClientId(unsigned char id)
{
	alsaClientId = id;
}

void LashClient::sendAlsaClientId()
{
	lash_alsa_client_id(lashClient, alsaClientId);
}

#endif

