/**
 * LASH support for Hydrogen 0.9.3.
 *
 * Created by Jaakko Sipari (jaakko.sipari@gmail.com)
 */

#include "config.h"

#ifdef LASH_SUPPORT

#include <lash-1.0/lash/lash.h>
#include "lib/lash/LashClient.h"
#include <string>

LashClient* LashClient::instance = NULL; /// static reference of LashClient class (Singleton)

LashClient::LashClient(const char* lashClass, const char* viewName, int* argc, char*** argv)
{
	newProject = true;
	lash_args_t *lash_args = lash_extract_args(argc, argv);
	lashClient = lash_init(lash_args, lashClass, LASH_Config_File, LASH_PROTOCOL(2, 0));
	
	if (isConnected())
	{
		sendEvent(LASH_Client_Name, viewName);
	}
	
	instance = this;
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

void LashClient::setJackClientName(string name)
{
	jackClientName = name;
}

void LashClient::sendJackClientName()
{
	lash_jack_client_name(lashClient, jackClientName.c_str());
}

void LashClient::sendAlsaClientId(unsigned char id)
{
	lash_alsa_client_id(lashClient, id);
}


/// Return the LashClient instance
LashClient* LashClient::getInstance() {
	return instance;
}

#endif
