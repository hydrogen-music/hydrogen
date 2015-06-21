#ifndef AUTOMATION_PATH_SERIALIZER_H
#define AUTOMATION_PATH_SERIALIZER_H

#include <hydrogen/object.h>
#include <hydrogen/basics/automation_path.h>

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
