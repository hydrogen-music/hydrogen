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
