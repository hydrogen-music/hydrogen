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

#include <core/config.h>

#if defined(H2CORE_HAVE_LASH) || _DOXYGEN_

#ifndef LASH_CLIENT
#define LASH_CLIENT

#include <lash/lash.h>

#include <string>
#include <cassert>

/** \ingroup docCore docAutomation*/
class LashClient
{
public:
	static LashClient* get_instance() { assert(__instance); return __instance;}
	static void create_instance(
		const char *lashClass,
		const char *viewName,
		int *argc,
		char ***argv
		);

	LashClient(const char* lashClass, const char* viewName, int* argc, char ***argv);
	~LashClient();

	bool isConnected();

	void sendEvent(LASH_Event_Type eventType, const char* value);
	void sendEvent(LASH_Event_Type eventType);

	/**
	 * Set the name of the JACK client by modifying
	 * #jackClientName. 
	 *
	 * Called in H2Core::JackAudioDriver::init() after
	 * successfully opening a client session.
	 */
	void setJackClientName( const std::string& jackClientName );
	void sendJackClientName();
	void setAlsaClientId(unsigned char id);
	void sendAlsaClientId();

	lash_event_t* getNextEvent();
	lash_client_t* getConnection();

	void setNewProject(bool value);
	bool isNewProject();
private:
	bool newProject;
	lash_client_t* lashClient;
	/**
	 * Name of the external JACK client Hydrogen is using to
	 * communicate with the JACK server. It can be modified via
	 * setJackClientName().
	 */
	std::string jackClientName;
	unsigned char alsaClientId;
	static LashClient* __instance;
};

#endif // LASH_CLIENT
#endif // H2CORE_HAVE_LASH


