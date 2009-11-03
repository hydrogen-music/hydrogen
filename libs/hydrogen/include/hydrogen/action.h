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
#ifndef ACTION_H
#define ACTION_H
#include <hydrogen/Object.h>
#include <map>
#include <string>
#include <cassert>

using namespace std;


class Action : public Object {
	public:
		Action( QString );
			
		void setParameter1( QString text ){
			parameter1 = text;
		}
		
		void setParameter2( QString text ){
			parameter2 = text;
		}
		
		QString getParameter1(){
			return parameter1;
		}
		
		QString getParameter2(){
			return parameter2;
		}

		QString getType(){
			return type;
		}
		

	private:
		QString type;
		QString parameter1;
		QString parameter2;
};


bool setAbsoluteFXLevel( int nLine, int fx_channel , int fx_param);


class ActionManager : public Object
{
	private:
		static ActionManager *__instance;
		QStringList actionList;
		QStringList eventList;

	public:
		bool handleAction( Action * );
		
		static void create_instance();
		static ActionManager* get_instance() { assert(__instance); return __instance; }
		
		QStringList getActionList(){
			return actionList;
		}
		
		QStringList getEventList(){
			return eventList;
		}

		ActionManager();
		~ActionManager();
};
#endif
