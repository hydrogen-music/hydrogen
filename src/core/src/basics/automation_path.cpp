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


/**
 * \brief Get value at given location
 * \param x Location
 *
 * If location is between points, value is computed
 **/
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


/**
 * \brief Add a point to path
 * \param x X coordinate
 * \param y Y coordinate
 **/
void AutomationPath::add_point(float x, float y)
{
	_points[x] = y;
}


/**
 * \brief Compare two paths
 *
 * Two paths are considered equal, if they have the same settings
 * (min, max, default) and points in the same places.
 */
bool operator==(const AutomationPath &lhs, const AutomationPath &rhs)
{
	return lhs._min == rhs._min
		&& lhs._max == rhs._max
		&& lhs._def == rhs._def
		&& lhs._points == rhs._points;
}


bool operator!=(const AutomationPath &lhs, const AutomationPath &rhs)
{
	return !(lhs==rhs);
}

std::ostream &operator<< (std::ostream &o, const AutomationPath &p)
{
	o << "<AutomationPath("<<p.get_min()<<","<<p.get_max()<<","<<p.get_default()<<",[";
	for (auto i = p.begin(); i != p.end(); ++i) {
		o << "(" << i->first << "," << i->second << "),";
	}
	o << "]>";
	return o;
}


/**
 * \brief Find point in specific location
 *
 * If point is faound, iterator pointing to it is returned.
 * Otherwise, AutomationPath::end() is returned.
 **/
AutomationPath::iterator AutomationPath::find(float x)
{
	const float limit = 0.5f;

	if (_points.empty())
		return _points.end();

	auto i = _points.lower_bound(x);

	if (i != _points.end()) {
		if( i->first - x <= limit)
			return i;
	}

	--i;
	if( x - i->first <= limit)
		return i;

	return _points.end();
}


/**
 * \brief Move point to other location
 * \param in Iterator pointing to point to be moved
 * \param x Destination X coordinate
 * \param y Destination Y coordinate
 **/
AutomationPath::iterator AutomationPath::move(iterator &in, float x, float y)
{
	_points.erase(in);
	auto rv = _points.insert(std::make_pair(x,y));
	return rv.first;
}


/**
 * \brief Remove point from path
 * \param x Point location
 **/
void AutomationPath::remove_point(float x)
{
	auto it = find(x);
	if (it != _points.end()) {
		_points.erase(it);
	}
}

} //namespace H2Core
