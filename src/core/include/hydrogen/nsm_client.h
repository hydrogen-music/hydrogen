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

#ifndef NSM_CLIENT_H
#define NSM_CLIENT_H

#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_

#include <hydrogen/object.h>
#include <cassert>


/**
* @class NsmClient
*
* @brief Non session manager client implementation
*
*
* @author Sebastian Moors
*
*/

class NsmClient : public H2Core::Object
{
	public:
		/** \return #m_sClassName*/
		static const char* className() { return m_sClassName; }
		/**
		 * Object holding the current NsmClient singleton. It
		 * is initialized with NULL, set with
		 * create_instance(), and accessed with
		 * get_instance().
		 */
		static NsmClient* __instance;
		~NsmClient();

		pthread_t m_NsmThread;
		/**
		 * If #__instance equals 0, a new NsmClient singleton
		 * will be created and stored in it.
		 *
		 * It is called in
		 * H2Core::Hydrogen::create_instance().
		 */
		static void create_instance();
		/**
		 * Returns a pointer to the current NsmClient
		 * singleton stored in #__instance.
		 */
		static NsmClient* get_instance() { assert(__instance); return __instance; }

		void createInitialClient();

		void shutdown();

	private:
		/** Contains the name of the class.
		 *
		 * This variable allows from more informative log messages
		 * with the name of the class the message is generated in
		 * being displayed as well. Queried using className().*/
		static const char* m_sClassName;
		NsmClient();

};

#endif /* H2CORE_HAVE_OSC */

#endif // NSM_CLIENT_H
