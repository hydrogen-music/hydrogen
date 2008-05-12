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

using namespace std;


class action : public Object {
	public:
		action( QString );
		
		QString getType();

	private:
		QString type;
};


class midiMap : public Object
{
	public:
		midiMap();
		~midiMap();

		static midiMap * instance;
		static midiMap * getInstance();

		void registerMMCEvent( QString,action * );

		action * getMMCAction( QString );
		
		map <QString , action *> getMMCMap();
		map <QString , action *> mmcMap;
};


class actionManager : public Object
{
	private:
		static actionManager *instance;
		QStringList actionList;
		QStringList eventList;

	public:
		static actionManager* getInstance();

		bool handleAction( action * );
		
		QStringList getActionList();
		QStringList getEventList();

		actionManager();
		~actionManager();
};
#endif
