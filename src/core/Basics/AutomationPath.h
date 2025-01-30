/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef H2C_AUTOMATION_PATH_H
#define H2C_AUTOMATION_PATH_H

#include <core/Object.h>
#include <map>

#if __cplusplus <= 199711L
#  define noexcept
#endif

namespace H2Core
{

/** \ingroup docCore docDataStructure docAutomation*/
class AutomationPath : public Object<AutomationPath>
{
	H2_OBJECT(AutomationPath)

	public:
	typedef std::map<float,float>::iterator iterator;
	typedef std::map<float,float>::const_iterator const_iterator;

	private:
	
	float _min;
	float _max;
	float _def;

	std::map<float,float> _points;

	public:
	
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
	
	/** Formatted string version for debugging purposes.
	 * \param sPrefix String prefix which will be added in front of
	 * every new line
	 * \param bShort Instead of the whole content of all classes
	 * stored as members just a single unique identifier will be
	 * displayed without line breaks.
	 *
	 * \return String presentation of current object.*/
	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;
};
};

#endif
