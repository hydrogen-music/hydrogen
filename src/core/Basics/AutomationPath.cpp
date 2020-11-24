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
#include <core/Basics/AutomationPath.h>

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
	if (_points.empty()) {
		return _def;
	}

	auto f = _points.begin();
	if(x <= f->first) {
		return f->second;
	}

	auto l = _points.rbegin();
	if(x >= l->first) {
		return l->second;
	}

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
 * \brief Find point near specific location
 *
 * If point is faound, iterator pointing to it is returned.
 * Otherwise, AutomationPath::end() is returned.
 **/
AutomationPath::iterator AutomationPath::find(float x)
{
	const float limit = 0.5f;

	if (_points.empty()) {
		return _points.end();
	}

	auto i = _points.lower_bound(x);

	if (i != _points.end()) {
		if( i->first - x <= limit) {
			return i;
		}
	}

	/* If there is a point before, check whether
	 * it is a close match */
	if (i != _points.begin()) {
		--i;
		if( x - i->first <= limit) {
			return i;
		}
	}

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
