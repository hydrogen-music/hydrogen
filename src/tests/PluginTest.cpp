/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "PluginTest.h"
#include "TestHelper.h"

#include <QStringList>
#include <unistd.h>
#include <vector>
#include <iostream>

#include <core/config.h>
#include <core/FX/Effects.h>

CPPUNIT_TEST_SUITE_REGISTRATION( PluginTest );

void PluginTest::testCompiledWindowsLadspaBundle()
{
#ifdef WIN32
	QStringList ladspaDataPath = { TestHelper::get_instance()->getDataDir() + "plugins" };
  std::vector<H2Core::LadspaFXInfo*> pluginList =
	  H2Core::Effects::get_instance()->getPluginList( ladspaDataPath );

  if ( pluginList.size() != 31 ) {
	  std::cout << "Expected number of plugins: 31 - actual number: "
				<< pluginList.size() << std::endl;
  }

  // Number of plugins installed with Hydrogen on Windows
  CPPUNIT_ASSERT( pluginList.size() == 31 );
#endif
}
