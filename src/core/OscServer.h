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

#ifndef OSC_SERVER_H
#define OSC_SERVER_H

#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_

#include <lo/lo.h>


#include <core/Object.h>
#include <cassert>
#include <memory>

namespace lo
{
	class ServerThread;
}

class MidiAction;

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
* Please note that the way generic_handler() is implemented, the
* additional registration of commands without argument to require a
* float input, and the usage of float arguments instead of int are all
* because of the limitations of TouchOSC.
*
* @author Sebastian Moors
*
*/
/** \ingroup docCore docDebugging*/
class OscServer : public H2Core::Object<OscServer>
{
	H2_OBJECT(OscServer)
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
	
		static void create_instance( int nOscPort );
		/**
		 * Returns a pointer to the current OscServer
		 * singleton stored in #__instance.
		 */
		static OscServer* get_instance() { assert(__instance); return __instance; }

		int getTemporaryPort() const;

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
		static QString qPrettyPrint( const lo_type& type, void* data );
		
		/**
		 * Registers all handler functions.
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
		 * - SELECT_ONLY_NEXT_PATTERN_Handler()
		 * - SELECT_AND_PLAY_PATTERN_Handler() 
		 * - PLAYLIST_SONG_Handler()
		 * - SELECT_INSTRUMENT_Handler()
		 *
		 * In case of the session managing handlers the following ones
		 * only work with no argument present
		 * - SAVE_SONG_Handler()
		 * - SAVE_DRUMKIT_Handler()
		 * - SAVE_PREFERENCES_Handler()
		 * - QUIT_Handler()
		 * and others only work by supplying a string "s" type message
		 * - NEW_SONG_Handler()
		 * - OPEN_SONG_Handler()
		 * - SAVE_SONG_AS_Handler()
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
		 * which, apart from MIDI related stuff, use handleMidiAction() to
		 * push the current state of Hydrogen to the registered OSC
		 * clients. This will happen each time the state of Hydrogen
		 * does change.
		 *
		 * \return `true` on success.
		 */
		bool init();
		/** Starts the OSC server and makes it available to handle
		 * commands.
		 *
		 * If the server was not properly initialized, this function
		 * will do so.
		 *
		 * \return `true` on success*/
		bool start();
		/** Stops the OSC server and makes it unavailable.
		 * \return `true` on success*/
		bool stop();
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
		 * - \e /Hydrogen/STRIP_VOLUME_RELATIVE/[x]
		 * - \e /Hydrogen/PAN_ABSOLUTE/[x]
		 * - \e /Hydrogen/PAN_ABSOLUTE_SYM/[x]
		 * - \e /Hydrogen/PAN_RELATIVE/[x]
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
		void handleMidiAction(std::shared_ptr<MidiAction> pMidiAction);

		/** Should be only used within the integration tests! */
	lo::ServerThread* getServerThread() const;

		/**
		 * Creates an MidiAction of type @b PLAY and passes its
		 * references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void PLAY_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b PLAY/STOP_TOGGLE and passes
		 * its references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void PLAY_STOP_TOGGLE_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b PLAY/PAUSE_TOGGLE and passes
		 * its references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void PLAY_PAUSE_TOGGLE_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b STOP and passes its
		 * references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void STOP_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b PAUSE and passes its
		 * references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void PAUSE_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b RECORD_READY and passes its
		 * references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void RECORD_READY_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b RECORD/STROBE_TOGGLE and passes its
		 * references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void RECORD_STROBE_TOGGLE_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b RECORD_STROBE and passes its
		 * references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void RECORD_STROBE_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b EXIT and passes its
		 * references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void RECORD_EXIT_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b MUTE and passes its
		 * references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void MUTE_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b UNMUTE and passes its
		 * references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void UNMUTE_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b MUTE_TOGGLE and passes its
		 * references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void MUTE_TOGGLE_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b >>_NEXT_BAR and passes its
		 * references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void NEXT_BAR_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b <<_PREVIOUS_BAR and passes its
		 * references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void PREVIOUS_BAR_Handler(lo_arg **argv, int i);
		/**
		 * Creates sets the current tempo of Hydrogen to the provided
		 * value (first argument in @a argv).
		 *
		 * \param argv Pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void BPM_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b BPM_INCR and passes its
		 * references to MidiActionManager::handleMidiAction().
		 *
		 * The first argument in @a argv will be used to set
		 * MidiAction::parameter1.
		 *
		 * \param argv Pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void BPM_INCR_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b BPM_DECR and passes its
		 * references to MidiActionManager::handleMidiAction().
		 *
		 * The first argument in @a argv will be used to set
		 * MidiAction::parameter1.
		 *
		 * \param argv Pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void BPM_DECR_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b MASTER_VOLUME_RELATIVE and
		 * passes its references to MidiActionManager::handleMidiAction().
		 *
		 * The first argument in @a argv will be used to set
		 * MidiAction::parameter2.
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
		 * Creates an MidiAction of type @b STRIP_VOLUME_RELATIVE and
		 * passes its references to MidiActionManager::handleMidiAction().
		 *
		 * \param param1 Sets MidiAction::parameter1 of the newly created
		 * MidiAction.
		 * \param param2 Sets MidiAction::parameter2 of the newly created
		 * MidiAction.*/
		static void STRIP_VOLUME_RELATIVE_Handler( const QString& param1,
												   const QString& param2);
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
		 * Creates an MidiAction of type @b SELECT_NEXT_PATTERN and
		 * passes its references to MidiActionManager::handleMidiAction().
		 *
		 * The first argument in @a argv will be used to set
		 * MidiAction::parameter1.
		 *
		 * \param argv Pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void SELECT_NEXT_PATTERN_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b SELECT_ONLY_NEXT_PATTERN and
		 * passes its references to MidiActionManager::handleMidiAction().
		 *
		 * The first argument in @a argv will be used to set
		 * MidiAction::parameter1.
		 *
		 * \param argv Pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void SELECT_ONLY_NEXT_PATTERN_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b SELECT_AND_PLAY_PATTERN and
		 * passes its references to MidiActionManager::handleMidiAction().
		 *
		 * The first argument in @a argv will be used to set
		 * MidiAction::parameter1.
		 *
		 * \param argv Pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void SELECT_AND_PLAY_PATTERN_Handler(lo_arg **argv, int i);
		static void INSTRUMENT_PITCH_Handler( lo_arg **argv, int t);
		/**
		 * Creates an MidiAction of type @b FILTER_CUTOFF_LEVEL_ABSOLUTE
		 * and passes its references to
		 * MidiActionManager::handleMidiAction().
		 *
		 * \param param1 Sets MidiAction::parameter1 of the newly created
		 * MidiAction.
		 * \param param2 Sets MidiAction::parameter2 of the newly created
		 * MidiAction.*/
		static void FILTER_CUTOFF_LEVEL_ABSOLUTE_Handler( const QString& param1,
														  const QString& param2);
		/**
		 * Creates an MidiAction of type @b BEATCOUNTER and passes its
		 * references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void BEATCOUNTER_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b TAP_TEMPO and passes its
		 * references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void TAP_TEMPO_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b PLAYLIST_SONG and
		 * passes its references to MidiActionManager::handleMidiAction().
		 *
		 * The first argument in @a argv will be used to set
		 * MidiAction::parameter1.
		 *
		 * \param argv Pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void PLAYLIST_SONG_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b PLAYLIST_NEXT_SONG and passes
		 * its references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void PLAYLIST_NEXT_SONG_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b PLAYLIST_PREV_SONG and passes
		 * its references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void PLAYLIST_PREV_SONG_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b TOGGLE_METRONOME and passes
		 * its references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void TOGGLE_METRONOME_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b SELECT_INSTRUMENT and
		 * passes its references to MidiActionManager::handleMidiAction().
		 *
		 * The first argument in @a argv will be used to set
		 * MidiAction::parameter2.
		 *
		 * \param argv Pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void SELECT_INSTRUMENT_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b UNDO_ACTION and passes its
		 * references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param i Unused number of arguments passed by the OSC
		 * message.*/
		static void UNDO_ACTION_Handler(lo_arg **argv, int i);
		/**
		 * Creates an MidiAction of type @b REDO_ACTION and passes its
		 * references to MidiActionManager::handleMidiAction().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param argc Number of arguments passed by the OSC
		 * message.*/
		static void REDO_ACTION_Handler(lo_arg **argv, int argc);
		/**
		 * The handler expects the user to provide an absolute path to
		 * a .h2song file. If another file already exists with the
		 * same name, it will be overwritten.
		 *
		 * \param argv The "s" field does contain the absolute path.
		 * \param argc Number of arguments passed by the OSC message.
		 */
		static void NEW_SONG_Handler(lo_arg **argv, int argc);
		/**
		 * The handler expects the user to provide an absolute path for
		 * a .h2song file.
		 *
		 * \param argv The "s" field does contain the absolute path.
		 * \param argc Number of arguments passed by the OSC message.
		 */
		static void OPEN_SONG_Handler(lo_arg **argv, int argc);
		/**
		 * Triggers CoreActionController::saveSong().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param argc Unused number of arguments passed by the OSC
		 * message.*/
		static void SAVE_SONG_Handler(lo_arg **argv, int argc);
		/**
		 * Triggers CoreActionController::saveSongAs().
		 *
		 * The handler expects the user to provide an absolute path to
		 * a .h2song file. If another file already exists with the
		 * same name, it will be overwritten.
		 *
		 * \param argv The "s" field does contain the absolute path.
		 * \param argc Number of arguments passed by the OSC
		 * message.*/
		static void SAVE_SONG_AS_Handler(lo_arg **argv, int argc);
		/**
		 * Triggers CoreActionController::savePreferences().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param argc Unused number of arguments passed by the OSC
		 * message.*/
		static void SAVE_PREFERENCES_Handler(lo_arg **argv, int argc);
		/**
		 * Triggers CoreActionController::quit().
		 *
		 * \param argv Unused pointer to a vector of arguments passed
		 * by the OSC message.
		 * \param argc Unused number of arguments passed by the OSC
		 * message.*/
		static void QUIT_Handler(lo_arg **argv, int argc);
		/**
		 * Triggers CoreActionController::activateTimeline().
		 *
		 * \param argv The "f" field does contain the value supplied
		 * by the user. If it is 0, the Timeline will be
		 * deactivated. Else, it will be activated instead.
		 * \param argc Unused number of arguments passed by the OSC
		 * message.*/
		static void TIMELINE_ACTIVATION_Handler(lo_arg **argv, int argc);
		/**
		 * Triggers CoreActionController::addTempoMarker().
		 *
		 * \param argv The first field "f" does contain the bar at
		 * which to place the new Timeline::TempoMarker while the
		 * second one "f" specifies its tempo in bpm.
		 * \param argc Unused number of arguments passed by the OSC
		 * message.*/
		static void TIMELINE_ADD_MARKER_Handler(lo_arg **argv, int argc);
		/**
		 * Triggers CoreActionController::deleteTempoMarker().
		 *
		 * \param argv The first field "f" does contain the bar at
		 * which to delete a Timeline::TempoMarker.
		 * \param argc Unused number of arguments passed by the OSC
		 * message.*/
		static void TIMELINE_DELETE_MARKER_Handler(lo_arg **argv, int argc);
		/**
		 * Triggers CoreActionController::activatedJackTransport().
		 *
		 * \param argv The "f" field does contain the value supplied
		 * by the user. If it is 0, the Jack transport will be
		 * deactivated. Else, it will be activated instead.
		 * \param argc Unused number of arguments passed by the OSC
		 * message.*/
		static void JACK_TRANSPORT_ACTIVATION_Handler(lo_arg **argv, int argc);
		/**
		 * Triggers CoreActionController::activateJackTimebaseControl().
		 *
		 * \param argv The "f" field does contain the value supplied by the
		 *   user. If it is `0`, Hydrogen will drop JACK Timebase control. Else,
		 *   it tries to register as Timebase controller instead.
		 * \param argc Unused number of arguments passed by the OSC
		 *   message.*/
		static void JACK_TIMEBASE_MASTER_ACTIVATION_Handler(lo_arg **argv, int argc);
		/**
		 * Triggers CoreActionController::activateSongMode().
		 *
		 * \param argv The "f" field does contain the value supplied
		 * by the user. If it is 0, Pattern mode of the playback will
		 * be activated. Else, Song mode will be activated instead.
		 * \param argc Unused number of arguments passed by the OSC
		 * message.*/
		static void SONG_MODE_ACTIVATION_Handler(lo_arg **argv, int argc);
	/**
		 * Triggers CoreActionController::activateLoopMode().
		 *
		 * \param argv The "f" field does contain the value supplied
		 * by the user. If it is 0, loop mode will
		 * be deactivated. Else, it will be activated instead.
		 * \param argc Unused number of arguments passed by the OSC
		 * message.*/
		static void LOOP_MODE_ACTIVATION_Handler(lo_arg **argv, int argc);
		/**
		 * \param argv The "f" field does contain the desired
		 * position / number of the pattern group (starting with
		 * 0).
		 * \param argc Unused number of arguments passed by the OSC
		 * message.*/
		static void RELOCATE_Handler(lo_arg **argv, int argc);
		/**
		 * The handler expects the user to provide an absolute path for
		 * a .h2pattern file. If another file already exists with the
		 * same name, it will be overwritten.
		 *
		 * \param argv The "s" field does contain the name for the new
		 * pattern.
		 * \param argc Number of arguments passed by the OSC message.
		 */
		static void NEW_PATTERN_Handler(lo_arg **argv, int argc);
		/**
		 * Triggers CoreActionController::openPattern().
		 *
		 * The handler expects the user to provide an absolute path to
		 * a .h2pattern file.
		 *
		 * \param argv The "s" field does contain the absolute path.
		 * \param argc Number of arguments passed by the OSC message.
		 */
		static void OPEN_PATTERN_Handler(lo_arg **argv, int argc);
		/**
		 * Triggers CoreActionController::removePattern().
		 *
		 * The handler expects the user to provide the pattern number
		 * (row the pattern resides in within the SongEditor).
		 *
		 * \param argv The "f" field does contain the pattern number
		 * (caution: it starts at 0).
		 * \param argc Number of arguments passed by the OSC message.
		 */
		static void REMOVE_PATTERN_Handler(lo_arg **argv, int argc);
		static void CLEAR_SELECTED_INSTRUMENT_Handler(lo_arg **argv, int argc);
		/**
		 * The handler expects the user to provide the number of the instrument
		 * for which all notes should be removed from the currently selected
		 * pattern.
		 *
		 * \param argv The "f" field does contain the instrument number
		 * (caution: it starts at 0).
		 * \param argc Number of arguments passed by the OSC message.
		 */
		static void CLEAR_INSTRUMENT_Handler(lo_arg **argv, int argc);
		/**
		 * The handler removes all notes from the the currently selected
		 * pattern.
		 *
		 * \param argv The "f" field does contain the instrument number
		 * (caution: it starts at 0).
		 * \param argc Number of arguments passed by the OSC message.
		 */
		static void CLEAR_PATTERN_Handler(lo_arg **argv, int argc);

		static void COUNT_IN_Handler(lo_arg **argv, int argc);
		static void COUNT_IN_PAUSE_TOGGLE_Handler(lo_arg **argv, int argc);
		static void COUNT_IN_STOP_TOGGLE_Handler(lo_arg **argv, int argc);

		/**
		 * Provides a similar behavior as a NOTE_ON MIDI message.
		 *
		 * \param argv The first "f" holds the note. It is designed after the
		 *   MIDI NOTE_ON handling and expects an integer between 36 and 127
		 *   (inspired by the General MIDI standard).
		 *   The second "f" field contains the velocity of the new note within
		 *   the range of [0, 1.0]
		 * \param argc Number of arguments passed by the OSC message.
		 */
		static void NOTE_ON_Handler(lo_arg **argv, int argc);

		/**
		 * Provides a similar behavior as a NOTE_OFF MIDI message.
		 *
		 * \param argv The "f" field holds the note. It is designed after the
		 *   MIDI NOTE_ON handling and expects an integer between 36 and 127
		 *   (inspired by the General MIDI standard).
		 * \param argc Number of arguments passed by the OSC message.
		 */
		static void NOTE_OFF_Handler(lo_arg **argv, int argc);

		/**
		 * Triggers CoreActionController::songEditorToggleGridCell().
		 *
		 * The handler expects the user to provide the pattern number
		 * (row the pattern resides in within the SongEditor).
		 *
		 * \param argv The first two "f" fields do contain the column
		 * and row number of the particular grid cell.
		 * \param argc Number of arguments passed by the OSC message.
		 */
		static void SONG_EDITOR_TOGGLE_GRID_CELL_Handler(lo_arg **argv, int argc);
		/**
		 * Triggers CoreActionController::setDrumkit().
		 *
		 * The handler expects the user to provide the drumkit name. 
		 * (row the pattern resides in within the SongEditor). The
		 * second argument, whether or not superfluous instrument
		 * should be removed even if there is a pattern in which they
		 * contain notes, is optional. The default choice will be true.
		 */
	static void LOAD_DRUMKIT_Handler( lo_arg **argv, int argc );

		/** Triggers #H2Core::MidiActionManager::loadNextDrumkit(). */
		static void LOAD_NEXT_DRUMKIT_Handler( lo_arg **argv, int argc );
		/** Triggers #H2Core::MidiActionManager::loadPrevDrumkit(). */
		static void LOAD_PREV_DRUMKIT_Handler( lo_arg **argv, int argc );

		/**
		 * Triggers CoreActionController::upgradeDrumkit().
		 *
		 * The handler expects the user to provide as first argument
		 * the absolute path to a folder containing a drumkit, the
		 * absolute path to a drumkit file (drumkit.xml) itself, or an
		 * absolute path to a compressed drumkit ( *.h2drumkit). The
		 * second argument is optional and contains the absolute path
		 * to a directory where the upgraded kit will be stored. If
		 * the second path is missing, the drumkit will be upgraded in
		 * place and a backup file will be created in order to not
		 * overwrite the existing state. If a compressed drumkit is
		 * provided as first argument, the upgraded drumkit will be
		 * compressed as well.
		 */
	static void UPGRADE_DRUMKIT_Handler( lo_arg **argv, int argc );
		/**
		 * Triggers CoreActionController::validateDrumkit().
		 *
		 * The handler expects the user to provide the absolute path
		 * to a folder containing a drumkit, the absolute path to a
		 * drumkit file (drumkit.xml) itself, or an absolute path to a
		 * compressed drumkit ( *.h2drumkit). The second argument is
		 * optional and contains the absolute path to a directory
		 * where the upgraded kit will be stored.
		 */
	static void VALIDATE_DRUMKIT_Handler( lo_arg **argv, int argc );
		/**
		 * Triggers CoreActionController::extractDrumkit().
		 *
		 * The handler expects the user to provide as first argument
		 * the absolute path to a compressed drumkit ( *.h2drumkit). The
		 * second argument is optional and contains the absolute path
		 * to a directory where the kit will be extracted to. If
		 * the second path is missing, the drumkit will be installed
		 * in the user's drumkit data folder.
		 */
	static void EXTRACT_DRUMKIT_Handler( lo_arg **argv, int argc );

		static void NEW_PLAYLIST_Handler(lo_arg **argv, int argc);
		static void OPEN_PLAYLIST_Handler(lo_arg **argv, int argc);
		static void SAVE_PLAYLIST_Handler(lo_arg **argv, int argc);
		static void SAVE_PLAYLIST_AS_Handler(lo_arg **argv, int argc);
		static void PLAYLIST_ADD_SONG_Handler(lo_arg **argv, int argc);
		static void PLAYLIST_ADD_CURRENT_SONG_Handler(lo_arg **argv, int argc);
		static void PLAYLIST_REMOVE_SONG_Handler(lo_arg **argv, int argc);

		/** 
		 * Catches any incoming messages and display them. 
		 *
		 * It is also responsible for catching OSC messages at the
		 * following paths and invoking the corresponding functions
		 * (if only a single argument is present.)
		 * - \e /Hydrogen/STRIP_VOLUME_ABSOLUTE/[x]
		 * - \e /Hydrogen/PAN_ABSOLUTE/[x]
		 * - \e /Hydrogen/PAN_ABSOLUTE_SYM/[x]
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
								int argc, lo_message data, void *user_data);
	static int incomingMessageLogging(const char *path, const char *types, lo_arg ** argv,
								int argc, lo_message data, void *user_data);

	private:
		OscServer( int nOscPort );
		
		/** Helper function which sends a message with msgText to all 
		 * connected clients. **/
		void broadcastMessage( const char* msgText, const lo_message& message);

		/**
		 * Used to determine whether the callback methods were already
		 * added to #m_pServerThread.
		 */
		bool m_bInitialized;
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
		std::list<lo_address> m_pClientRegistry;

		/**
		 * In case #Preferences::m_nOscServerPort is already occupied by another
		 * client, the alternative - random - port number provided by the OSC
		 * server will be stored in this variable. If the connection using the
		 * default/CLI port succeeded, the variable will be set to -1.
		 */
		int					m_nTemporaryPort;
};

inline lo::ServerThread* OscServer::getServerThread() const {
	return m_pServerThread;
}
inline int OscServer::getTemporaryPort() const {
	return m_nTemporaryPort;
}

#endif /* H2CORE_HAVE_OSC */

#endif // OSC_SERVER_H
