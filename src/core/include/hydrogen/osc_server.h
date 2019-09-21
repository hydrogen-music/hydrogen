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

#ifndef OSC_SERVER_H
#define OSC_SERVER_H

#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_

#include <lo/lo.h>


#include <hydrogen/object.h>
#include <hydrogen/Preferences.h>
#include <cassert>




namespace lo
{
	class ServerThread;
}

/**
* @class OscServer
*
* @brief OSC Server implementation
*
* Open Sound Control (OSC) is a protocol for communication among
* programs, computers, and hardware, like synthesizers or multimedia
* devices, via networking protocols such as UDP or TCP. It can be
* thought of as a replacement for the MIDI protocol with rich
* benefits, like supporting symbolic and high-resolution numerical
* argument data, providing an URL-style naming scheme in combination
* with a pattern matching language, and allowing to bundle messages
* for a better handling of timing and simultaneous processing.
*
* The OscServer class provides an implementation of an OSC server
* running in a separate thread and handling all incoming and outgoing
* OSC messages send to or from Hydrogen. Its naming scheme starts with
* the prefix \e /Hydrogen/ followed by the name of one of the handler
* functions implemented as members of this class without the \e
* _Handler() prefix. Sending an OSC message to the path \e
* /Hydrogen/PAUSE will thus trigger the PAUSE_Handler() to pause the
* playback. You can play with these features using the command line
* program \b oscsend on UNIX-based systems.
*
* Internally, the OscServer is implemented as a singleton and will be
* created using create_instance() and queried using
* get_instance(). Using start() all handler functions will be
* registered to their corresponding paths and the OSC server thread
* will start to listen for incoming messages. But this will only
* happen if H2Core::Preferences::m_bOscServerEnabled is set to
* true. In addition, Hydrogen will send OSC messages about its current
* state to all clients each time its state changes if
* H2Core::Preferences::m_bOscFeedbackEnabled is set to
* true. H2Core::Preferences::m_nOscServerPort contains the port number
* the OSC server will be started at.
*
* @author Sebastian Moors
*
*/
class OscServer : public H2Core::Object
{
	H2_OBJECT
	public:
		/**
		 * Object holding the current OscServer singleton. It is
		 * initialized with nullptr, set with create_instance(), and
		 * accessed with get_instance().
		 */
		static OscServer* __instance;
		/**
		 * Destructor freeing all addresses in #m_pClientRegistry and
		 * setting #__instance to nullptr.
		 */
		~OscServer();
	
		/**		 
		 * If #__instance equals nullptr, a new OscServer singleton
		 * will be created by calling the OscServer() constructor and
		 * stored in #__instance.
		 *
		 * It is called in
		 * H2Core::Hydrogen::create_instance().
		 *
		 * \param pPreferences Pointer to the H2Core::Preferences
		 * singleton. Although it could be accessed internally using
		 * H2Core::Preferences::get_instance(), this is an appetizer
		 * for internal changes happening after the 1.0 release.
		 */
		static void create_instance( H2Core::Preferences* pPreferences );
		/**
		 * Returns a pointer to the current OscServer
		 * singleton stored in #__instance.
		 */
		static OscServer* get_instance() { assert(__instance); return __instance; }

		/**
		 * Converts a data @a data of type @a type into a printable
		 * QString.
		 * 
		 * Apart from the basic OSC types LO_INT32, LO_FLOAT, and
		 * LO_STRING the following extended OSC types are supported:
		 * - LO_INT64
		 * - LO_TIMETAG
		 * - LO_DOUBLE
		 * - LO_SYMBOL
		 * - LO_CHAR
		 * - LO_TRUE
		 * - LO_FALSE
		 * - LO_NIL
		 * - LO_INFINITUM
		 *
		 * LO_BLOB and LO_MIDI are, however, NOT supported.
		 *
		 * \param type Liblo class @a data will be cast to. 
		 * \param data Data to be converted to string.
		 *
		 * \return QString representation of @a data.
		 */
		static QString qPrettyPrint(lo_type type,void * data);
		
		/**
		 * Registers all handler functions defined for this class
		 * starts the OscServer.
		 *
		 * The path the handlers will be registered at always starts
		 * with \e /Hydrogen/ followed by the name of the handler
		 * function without the suffix \e _Handler. PLAY_Handler()
		 * will thus be registered to \e /Hydrogen/PLAY.
		 *
		 * Most handler will be registered for both types "" and "f"
		 * (floats). But the following handlers will be registered for
		 * floats only:
		 * - BPM_DECR_Handler()
		 * - BPM_INCR_Handler()
		 * - MASTER_VOLUME_ABSOLUTE_Handler()
		 * - MASTER_VOLUME_RELATIVE_Handler()
		 * - STRIP_VOLUME_RELATIVE_Handler()
		 * - SELECT_NEXT_PATTERN_Handler()
		 * - SELECT_NEXT_PATTERN_PROMPTLY_Handler()
		 * - SELECT_AND_PLAY_PATTERN_Handler() 
		 * - PLAYLIST_SONG_Handler()
		 * - SELECT_INSTRUMENT_Handler()
		 *
		 * The generic_handler() will be registered to match all paths
		 * and types.
		 *
		 * In addition, a lambda function will be registered to match
		 * all types and paths too. If the client has not sent any
		 * message to Hydrogen yet, it will take care of its
		 * registration to #m_pClientRegistry using the address of the
		 * received OSC message. More importantly, it also will call
		 * H2Core::CoreActionController::initExternalControlInterfaces(),
		 * which, apart from MIDI related stuff, use handleAction() to
		 * push the current state of Hydrogen to the registered OSC
		 * clients. This will happen each time the state of Hydrogen
		 * does change.
		 *
		 * This function will only be processed if the created server
		 * thread #m_pServerThread is valid.
		 */
		void start();
		/**
		 * Function called by
		 * H2Core::CoreActionController::initExternalControlInterfaces()
		 * to inform all clients about the current state of Hydrogen
		 * using OSC messages send by Hydrogen itself.
		 *
		 * The following feedback functions will be used to describe
		 * the aforementioned state:
		 * - H2Core::CoreActionController::setMasterVolume()
		 * - H2Core::CoreActionController::setMetronomeIsActive()
		 * - H2Core::CoreActionController::setMasterIsMuted()
		 * - H2Core::CoreActionController::setStripVolume() [*]
		 * - H2Core::CoreActionController::setStripPan() [*]
		 * - H2Core::CoreActionController::setStripIsMuted() [*]
		 * - H2Core::CoreActionController::setStripIsSoloed() [*]
		 *
		 * [*] Function will be called for all Instruments in
		 * H2Core::Song::__instrument_list.
		 *
		 * The constructed messages will contain the
		 * Action::parameter2 of @a pAction as float types (or
		 * Action::parameter1 for the actions @b TOGGLE_METRONOME and
		 * @b MUTE_TOGGLE) and will be associated with one of the
		 * following paths:
		 * - \e /Hydrogen/MASTER_VOLUME_ABSOLUTE
		 * - \e /Hydrogen/TOGGLE_METRONOME
		 * - \e /Hydrogen/MUTE_TOGGLE
		 * - \e /Hydrogen/STRIP_VOLUME_ABSOLUTE/[x]
		 * - \e /Hydrogen/PAN_ABSOLUTE/[x]
		 * - \e /Hydrogen/STRIP_MUTE_TOGGLE/[x]
		 * - \e /Hydrogen/STRIP_SOLO_TOGGLE/[x]
		 *
		 * [x] The last part of the URI is determined by
		 * Action::parameter1 and specifies an individual strip.
		 *
		 * Only called if H2Core::Preferences::m_bOscServerEnabled is
		 * true.
		 *
		 * \param pAction Action to be sent to all registered
		 * clients. 
		 */
		static void handleAction(Action* pAction);

		/**
		 * Creates an Action of type @b PLAY and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void PLAY_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b PLAY/STOP_TOGGLE and passes
		 * its references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void PLAY_STOP_TOGGLE_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b PLAY/PAUSE_TOGGLE and passes
		 * its references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void PLAY_PAUSE_TOGGLE_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b STOP and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void STOP_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b PAUSE and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void PAUSE_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b RECORD_READY and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void RECORD_READY_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b RECORD/STROBE_TOGGLE and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void RECORD_STROBE_TOGGLE_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b RECORD_STROBE and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void RECORD_STROBE_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b EXIT and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void RECORD_EXIT_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b MUTE and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void MUTE_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b UNMUTE and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void UNMUTE_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b MUTE_TOGGLE and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void MUTE_TOGGLE_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b >>_NEXT_BAR and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void NEXT_BAR_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b <<_PREVIOUS_BAR and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void PREVIOUS_BAR_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b BPM_INCR and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * The first argument in @a argv will be used to set
		 * Action::parameter1.
		 *
		 * \param argv Pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void BPM_INCR_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b BPM_DECR and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * The first argument in @a argv will be used to set
		 * Action::parameter1.
		 *
		 * \param argv Pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void BPM_DECR_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b MASTER_VOLUME_RELATIVE and
		 * passes its references to MidiActionManager::handleAction().
		 *
		 * The first argument in @a argv will be used to set
		 * Action::parameter2.
		 *
		 * \param argv Pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void MASTER_VOLUME_RELATIVE_Handler(lo_arg **argv, int i);
		/**
		 * Calls H2Core::CoreActionController::setMasterVolume() with
		 * the first argument in @a argv.
		 *
		 * \param argv Pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void MASTER_VOLUME_ABSOLUTE_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b STRIP_VOLUME_RELATIVE and
		 * passes its references to MidiActionManager::handleAction().
		 *
		 * The first argument in @a argv will be used to set
		 * Action::parameter2.
		 *
		 * \param argv Pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void STRIP_VOLUME_RELATIVE_Handler(lo_arg **argv, int i);
		/**
		 * Calls H2Core::CoreActionController::setStripVolume() with
		 * both @a param1 and @a param2.
		 *
		 * \param param1 Passed as first argument to
		 * H2Core::CoreActionController::setStripVolume().
		 * \param param2 Passed as second argument to
		 * H2Core::CoreActionController::setStripVolume().*/
		static void STRIP_VOLUME_ABSOLUTE_Handler(int param1, float param2);
		/**
		 * Creates an Action of type @b SELECT_NEXT_PATTERN and
		 * passes its references to MidiActionManager::handleAction().
		 *
		 * The first argument in @a argv will be used to set
		 * Action::parameter1.
		 *
		 * \param argv Pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void SELECT_NEXT_PATTERN_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b SELECT_NEXT_PATTERN_PROMPTLY
		 * and passes its references to
		 * MidiActionManager::handleAction().
		 *
		 * The first argument in @a argv will be used to set
		 * Action::parameter1.
		 *
		 * \param argv Pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void SELECT_NEXT_PATTERN_PROMPTLY_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b SELECT_AND_PLAY_PATTERN and
		 * passes its references to MidiActionManager::handleAction().
		 *
		 * The first argument in @a argv will be used to set
		 * Action::parameter1.
		 *
		 * \param argv Pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void SELECT_AND_PLAY_PATTERN_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b PAN_RELATIVE and
		 * passes its references to MidiActionManager::handleAction().
		 *
		 * \param param1 Sets Action::parameter1 of the newly created
		 * Action.
		 * \param param2 Sets Action::parameter2 of the newly created
		 * Action.*/
		static void PAN_RELATIVE_Handler(QString param1, QString param2);
		/**
		 * Creates an Action of type @b PAN_ABSOLTUE and
		 * passes its references to MidiActionManager::handleAction().
		 *
		 * \param param1 Sets Action::parameter1 of the newly created
		 * Action.
		 * \param param2 Sets Action::parameter2 of the newly created
		 * Action.*/
		static void PAN_ABSOLUTE_Handler(QString param1, QString param2);
		/**
		 * Creates an Action of type @b FILTER_CUTOFF_LEVEL_ABSOLUTE
		 * and passes its references to
		 * MidiActionManager::handleAction().
		 *
		 * \param param1 Sets Action::parameter1 of the newly created
		 * Action.
		 * \param param2 Sets Action::parameter2 of the newly created
		 * Action.*/
		static void FILTER_CUTOFF_LEVEL_ABSOLUTE_Handler(QString param1, QString param2);
		/**
		 * Creates an Action of type @b BEATCOUNTER and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void BEATCOUNTER_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b TAP_TEMPO and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void TAP_TEMPO_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b PLAYLIST_SONG and
		 * passes its references to MidiActionManager::handleAction().
		 *
		 * The first argument in @a argv will be used to set
		 * Action::parameter1.
		 *
		 * \param argv Pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void PLAYLIST_SONG_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b PLAYLIST_NEXT_SONG and passes
		 * its references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void PLAYLIST_NEXT_SONG_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b PLAYLIST_PREV_SONG and passes
		 * its references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void PLAYLIST_PREV_SONG_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b TOGGLE_METRONOME and passes
		 * its references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void TOGGLE_METRONOME_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b SELECT_INSTRUMENT and
		 * passes its references to MidiActionManager::handleAction().
		 *
		 * The first argument in @a argv will be used to set
		 * Action::parameter2.
		 *
		 * \param argv Pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void SELECT_INSTRUMENT_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b UNDO_ACTION and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void UNDO_ACTION_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b REDO_ACTION and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void REDO_ACTION_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b NEW_SONG and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void NEW_SONG_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b OPEN_SONG and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void OPEN_SONG_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b SAVE_SONG and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void SAVE_SONG_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b SAVE_SONG_AS and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void SAVE_SONG_AS_Handler(lo_arg **argv, int i);
		/**
		 * Creates an Action of type @b QUIT and passes its
		 * references to MidiActionManager::handleAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void QUIT_Handler(lo_arg **argv, int i);
		/** 
		 * Catches any incoming messages and display them. 
		 *
		 * It is also responsible for catching OSC messages at the
		 * following paths and invoking the corresponding functions
		 * (if only a single argument is present.)
		 * - \e /Hydrogen/STRIP_VOLUME_ABSOLUTE/[x]
		 * - \e /Hydrogen/PAN_ABSOLUTE/[x]
		 * - \e /Hydrogen/PAN_RELATIVE/[x]
		 * - \e /Hydrogen/FILTER_CUTOFF_LEVEL_ABSOLUTE/[x]
		 * - \e /Hydrogen/STRIP_MUTE_TOGGLE/[x]
		 * - \e /Hydrogen/STRIP_SOLO_TOGGLE/[x]
		 *
		 * [x] Digit specifying a particular instrument.
		 *
		 * \param path The OSC path to register the method to. If NULL
		 * is passed the method will match all paths.
		 * \param types The typespec the method accepts. In Hydrogen
		 * handler functions are registered to listen for floats.
		 * \param argv Pointer to a vector of arguments passed by the
		 * OSC message.
		 * \param argc Number of arguments passed by the OSC message.
		 * \param data Unused.
		 * \param user_data Unused.
		 *
		 * \return 1 - means that the message has not been fully
		 * handled and the server should try other methods */
		static int  generic_handler(const char *path, const char *types, lo_arg ** argv,
								int argc, void *data, void *user_data);

	private:
		/**
		 * Private constructor creating a new OSC server thread using
		 * the port H2Core::Preferences::m_nOscServerPort and
		 * assigning the object to #m_pServerThread.
		 *
		 * \param pPreferences Pointer to the H2Core::Preferences
		 * singleton. Although it could be accessed internally using
		 * H2Core::Preferences::get_instance(), this is an appetizer
		 * for internal changes happening after the 1.0 release.
		 */
		OscServer( H2Core::Preferences* pPreferences );
	
		/** Pointer to the H2Core::Preferences singleton. Although it
		 * could be accessed internally using
		 * H2Core::Preferences::get_instance(), this is an appetizer
		 * for internal changes happening after the 1.0 release.*/
		H2Core::Preferences*			m_pPreferences;
		
		/**
		 * Object containing the actual thread with an OSC server
		 * running in.
		 *
		 * It is created in OscServer() and both assigned handlers and
		 * started in start().
		 */
		lo::ServerThread*				m_pServerThread;
		/**
		 * List of all OSC clients known to Hydrogen.
		 *
		 * Whenever an OSC client sends a message to the started OSC
		 * server of Hydrogen, a lambda handler registered in start()
		 * will check whether the address of this client is already
		 * present in #m_pClientRegistry. If this is not the case it
		 * will be added to it and the current state Hydrogen will be
		 * propagated to all registered clients.
		 */
		static std::list<lo_address>	m_pClientRegistry;
};

#endif /* H2CORE_HAVE_OSC */

#endif // OSC_SERVER_H
