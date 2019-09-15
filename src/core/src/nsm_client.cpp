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

#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_

#include "hydrogen/nsm_client.h"
#include "hydrogen/nsm.h"
#include "hydrogen/event_queue.h"
#include "hydrogen/hydrogen.h"
#include "hydrogen/basics/song.h"



NsmClient * NsmClient::__instance = nullptr;
const char* NsmClient::__class_name = "NsmClient";
bool NsmShutdown = false;


static int nsm_open_cb (const char *name,
						const char *display_name,
						const char *client_id,
						char **out_msg,
						void *userdata )
{

	H2Core::Preferences *pPref = H2Core::Preferences::get_instance();

	if(pPref){
		if(client_id){
			// setup JACK here, client_id gets the JACK client name
			pPref->setNsmClientId(QString(client_id));
		}

		if(name){

			/*
			 * the hydrogen core is not responsible for managing
			 * song loading on startup. Therefore we use store
			 * the desired song name and let the GUIs do the actual work.
			 */

			pPref->setNsmSongName(QString(name));
		}
	}

	return ERR_OK;
}

static int nsm_save_cb ( char **out_msg, void *userdata )
{
	H2Core::Song *pSong = H2Core::Hydrogen::get_instance()->getSong();
	QString fileName = pSong->get_filename();

	pSong->save( fileName );

	return ERR_OK;
}

void* nsm_processEvent(void* data)
{
	nsm_client_t* nsm = (nsm_client_t*) data;

	while(!NsmShutdown && nsm){
		nsm_check_wait( nsm, 1000);
	}

	return nullptr;
}

NsmClient::NsmClient()
	: Object( __class_name )
{
	m_NsmThread = 0;
	m_bUnderSessionManagement = false;
}

void NsmClient::create_instance()
{
	if( __instance == nullptr ) {
		__instance = new NsmClient;
	}
}

NsmClient::~NsmClient()
{
	__instance = nullptr;
}

void NsmClient::shutdown()
{
	NsmShutdown = true;
}

void NsmClient::createInitialClient()
{
	/*
	 * Make first contact with NSM server.
	 */

	nsm_client_t* nsm = nullptr;

	H2Core::Preferences *pPref = H2Core::Preferences::get_instance();
	QString H2ProcessName = pPref->getH2ProcessName();
	QByteArray byteArray = H2ProcessName.toLatin1();

	const char *nsm_url = getenv( "NSM_URL" );

	if ( nsm_url )
	{
		nsm = nsm_new();

		if ( nsm )
		{
			nsm_set_open_callback( nsm, nsm_open_cb, (void*) nullptr );
			nsm_set_save_callback( nsm, nsm_save_cb, (void*) nullptr );

			if ( nsm_init( nsm, nsm_url ) == 0 )
			{
				nsm_send_announce( nsm, "Hydrogen", "", byteArray.data() );
				nsm_check_wait( nsm, 10000 );

				if(pthread_create(&m_NsmThread, nullptr, nsm_processEvent, nsm)) {
					___ERRORLOG("Error creating NSM thread\n	");
					return;
				}
				
				// Everything worked fine and H2 can now be considered
				// under session management.
				m_bUnderSessionManagement = true;

			}
			else
			{
				___ERRORLOG("failed, freeing NSM client");
				nsm_free( nsm );
				nsm = nullptr;
			}
		}
	}
	else
	{
		___WARNINGLOG("No NSM URL available: no NSM management\n");
	}
}
#endif /* H2CORE_HAVE_OSC */

