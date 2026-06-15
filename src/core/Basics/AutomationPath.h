/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

class Hydrogen;
class XMLNode;

/** \ingroup docCore docDataStructure docAutomation*/
class AutomationPath : public Object<AutomationPath> {
	H2_OBJECT( AutomationPath )

   public:
	typedef std::map<float, float>::iterator iterator;
	typedef std::map<float, float>::const_iterator const_iterator;

   public:
	AutomationPath( float min = 0.0f, float max = 1.5f, float def = 1.0f );

	static std::shared_ptr<AutomationPath>
	loadFrom( const XMLNode& node, bool bSilent );
	void saveTo( XMLNode& node, bool bSilent ) const;

	bool empty() const noexcept { return m_points.empty(); }
	float getMin() const noexcept { return m_fMin; }
	float getMax() const noexcept { return m_fMax; }
	float getDefault() const noexcept { return m_fDef; }

	float getValue( float x ) const noexcept;

	void addPoint( float x, float y, Hydrogen* pHydrogen );
	void removePoint( float x, Hydrogen* pHydrogen );

	friend bool
	operator==( const AutomationPath& lhs, const AutomationPath& rhs );
	friend bool
	operator!=( const AutomationPath& lhs, const AutomationPath& rhs );

	iterator begin() { return m_points.begin(); }
	iterator end() { return m_points.end(); }
	const_iterator begin() const { return m_points.begin(); }
	const_iterator end() const { return m_points.end(); }

	iterator find( float x );
	iterator move( iterator& in, float x, float y, Hydrogen* pHydrogen );

	/** Formatted string version for debugging purposes.
	 * \param sPrefix String prefix which will be added in front of
	 * every new line
	 * \param bShort Instead of the whole content of all classes
	 * stored as members just a single unique identifier will be
	 * displayed without line breaks.
	 *
	 * \return String presentation of current object.*/
	QString toQString( const QString& sPrefix = "", bool bShort = true )
		const override;

   private:
	float m_fMin;
	float m_fMax;
	float m_fDef;

	std::map<float, float> m_points;
};
};	// namespace H2Core

#endif
