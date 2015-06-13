#include <hydrogen/basics/automation_path.h>

#include <iostream>

namespace H2Core
{

const char* AutomationPath::__class_name = "AutomationPath";

AutomationPath::AutomationPath(float min, float max, float def)
	: Object(__class_name),
	  _min(min),
	  _max(max),
	  _def(def)
{
}

float AutomationPath::get_value(float x) const noexcept
{
	if (_points.empty())
		return _def;

	auto f = _points.begin();
	if(x <= f->first)
		return f->second;

	auto l = _points.rbegin();
	if(x >= l->first)
		return l->second;

	auto i = _points.lower_bound(x);
	auto p1 = *i;
	auto p0 = *(--i);
	float x1 = p0.first;
	float y1 = p0.second;
	float x2 = p1.first;
	float y2 = p1.second;

	float d = (x-x1)/(x2 - x1);

	return y1 + (y2-y1)*d;
}

void AutomationPath::add_point(float x, float y)
{
	_points[x] = y;
}

}
