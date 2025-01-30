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

#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <QString>
#include <cassert>

class TestHelper {
	static TestHelper*	m_pInstance;
	QString m_sDataDir;
	QString m_sTestDataDir;
	
	public:
		TestHelper();
	
		QString getDataDir() const;
		QString getTestDataDir() const;
		QString getTestFile(const QString& file) const;

	static void			createInstance();
	static TestHelper*	get_instance();
};

inline TestHelper*	TestHelper::get_instance() 
{ 
	assert(m_pInstance); return m_pInstance; 
}

inline QString TestHelper::getDataDir() const 
{ 
	return m_sDataDir; 
}

inline QString TestHelper::getTestDataDir() const 
{ 
	return m_sTestDataDir;
}

inline QString TestHelper::getTestFile(const QString& file) const
{
	return m_sTestDataDir + file; 
}

#define H2TEST_FILE(name) TestHelper::get_instance()->getTestFile(name)

#endif
