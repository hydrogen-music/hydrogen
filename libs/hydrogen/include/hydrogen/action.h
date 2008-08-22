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


class Action : public Object {
	public:
		Action( QString );
		
		QString getType();
		QStringList getParameterList();
		void addParameter( QString );

	private:
		QString type;
		QStringList parameterList;
};





class ActionManager : public Object
{
	public:
		static ActionManager* getInstance();

		bool handleAction( Action * );
		
		QStringList getActionList();
		QStringList getEventList();

		ActionManager();
		~ActionManager();

	private:
		static ActionManager *instance;
		QStringList actionList;
		QStringList eventList;
};
#endif
