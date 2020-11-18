/*
 * Hydrogen
 * Copyright(c) 2015-2016 by Przemysław Sitek
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
#ifndef AUTOMATION_PATH_SERIALIZER_H
#define AUTOMATION_PATH_SERIALIZER_H

#include <core/Object.h>
#include <core/Basics/AutomationPath.h>

#include <QDomDocument>

namespace H2Core
{

class AutomationPathSerializer : private Object
{
	H2_OBJECT

public:
	AutomationPathSerializer();

	void read_automation_path(const QDomNode &node, AutomationPath &path);
	void write_automation_path(QDomNode &node, const AutomationPath &path);

};


}

#endif
