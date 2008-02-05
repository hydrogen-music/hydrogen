/**
 * LASH support for Hydrogen 0.9.3.                                           
 * 
 * Created by Jaakko Sipari (jaakko.sipari@gmail.com)
*/

#include "config.h"

#ifdef LASH_SUPPORT

#ifndef LASH_CLIENT
#define LASH_CLIENT

#include "config.h"

#include <lash-1.0/lash/lash.h>
#include <string>

using namespace std;

class LashClient
{
public:
	/// Return the LashClient instance
	static LashClient* getInstance();
	
	LashClient(const char* lashClass, const char* viewName, int* argc, char ***argv);
	~LashClient();

	bool isConnected();
	
	void sendEvent(LASH_Event_Type eventType, const char* value);
	void sendEvent(LASH_Event_Type eventType);

	void setJackClientName(std::string jackClientName);
	void sendJackClientName();
	void sendAlsaClientId(unsigned char id);

	lash_event_t* getNextEvent();
	lash_client_t* getConnection();

	void setNewProject(bool value);
	bool isNewProject();
private:
	bool newProject;
	lash_client_t* lashClient;
	std::string jackClientName;
	static LashClient* instance;
};

#endif // LASH_CLIENT
#endif // LASH_SUPPORT
