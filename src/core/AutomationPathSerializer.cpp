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
#include <hydrogen/automation_path_serializer.h>

namespace H2Core
{

const char* AutomationPathSerializer::__class_name = "AutomationPathSerializer";


AutomationPathSerializer::AutomationPathSerializer()
	: Object(__class_name)
{
}

void AutomationPathSerializer::read_automation_path(const QDomNode &node, AutomationPath &path)
{
	auto point = node.firstChildElement();
	while (! point.isNull()) {
		if (point.tagName() == "point") {
			bool hasX = false;
			bool hasY = false;
			float x = point.attribute("x").toFloat(&hasX);
			float y = point.attribute("y").toFloat(&hasY);

			if (hasX && hasY) {
				path.add_point(x, y);
			}

		}
		point = point.nextSiblingElement();
	}
}


void AutomationPathSerializer::write_automation_path(QDomNode &node, const AutomationPath &path)
{
	for (auto point : path) {
		auto element = node.ownerDocument().createElement("point");
		element.setAttribute("x", point.first);
		element.setAttribute("y", point.second);
		node.appendChild(element);
	}
}


}
