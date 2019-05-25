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
#ifndef H2C_AUTOMATION_PATH_H
#define H2C_AUTOMATION_PATH_H

#include <hydrogen/object.h>

#include <map>

#if __cplusplus <= 199711L
#  define noexcept
#endif

namespace H2Core
{

class AutomationPath : private Object
{
public:
	/** \return #m_sClassName*/
	static const char* className() { return m_sClassName; }

	typedef std::map<float,float>::iterator iterator;
	typedef std::map<float,float>::const_iterator const_iterator;
	
	AutomationPath(float min, float max, float def);

	bool empty() const noexcept { return _points.empty(); }
	float get_min() const noexcept { return _min; }
	float get_max() const noexcept { return _max; }
	float get_default() const noexcept { return _def; }

	float get_value(float x) const noexcept;

	void add_point(float x, float y);
	void remove_point(float x);

	friend bool operator==(const AutomationPath &lhs, const AutomationPath &rhs);
	friend bool operator!=(const AutomationPath &lhs, const AutomationPath &rhs);

	iterator begin() { return _points.begin(); }
	iterator end() { return _points.end(); }
	const_iterator begin() const { return _points.begin(); }
	const_iterator end() const { return _points.end(); }

	iterator find(float x);
	iterator move(iterator &in, float x, float y);

private:
	/** Contains the name of the class.
	 *
	 * This variable allows from more informative log messages
	 * with the name of the class the message is generated in
	 * being displayed as well. Queried using className().*/
	static const char* m_sClassName;
	
	float _min;
	float _max;
	float _def;

	std::map<float,float> _points;
};

std::ostream &operator<< (std::ostream &o, const AutomationPath &p);

};

#endif
