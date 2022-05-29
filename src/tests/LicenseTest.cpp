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

#include "LicenseTest.h"
#include "TestHelper.h"

using namespace H2Core;

void LicenseTest::testParsing() {

	License licenseCC0_0("cc0");
	CPPUNIT_ASSERT( licenseCC0_0.getType() == License::CC_0 );
	License licenseCC0_1("CC-0");
	CPPUNIT_ASSERT( licenseCC0_1.getType() == License::CC_0 );
	
	License licenseCC_BY_0("cc-by");
	CPPUNIT_ASSERT( licenseCC_BY_0.getType() == License::CC_BY );
	License licenseCC_BY_1("CC BY");
	CPPUNIT_ASSERT( licenseCC_BY_1.getType() == License::CC_BY );

	License licenseCC_BY_NC_0("cc-by-nc");
	CPPUNIT_ASSERT( licenseCC_BY_NC_0.getType() == License::CC_BY_NC );
	License licenseCC_BY_NC_1("CC BY_NC");
	CPPUNIT_ASSERT( licenseCC_BY_NC_1.getType() == License::CC_BY_NC );

	License licenseCC_BY_SA_0("cc-by-sa");
	CPPUNIT_ASSERT( licenseCC_BY_SA_0.getType() == License::CC_BY_SA );
	License licenseCC_BY_SA_1("CC BY_SA");
	CPPUNIT_ASSERT( licenseCC_BY_SA_1.getType() == License::CC_BY_SA );

	License licenseCC_BY_ND_0("cc-by-nd");
	CPPUNIT_ASSERT( licenseCC_BY_ND_0.getType() == License::CC_BY_ND );
	License licenseCC_BY_ND_1("CC BY_ND");
	CPPUNIT_ASSERT( licenseCC_BY_ND_1.getType() == License::CC_BY_ND );

	License licenseCC_BY_NC_SA_0("cc-by-nc-sa");
	CPPUNIT_ASSERT( licenseCC_BY_NC_SA_0.getType() == License::CC_BY_NC_SA );
	License licenseCC_BY_NC_SA_1("CC BY_SA_NC");
	CPPUNIT_ASSERT( licenseCC_BY_NC_SA_1.getType() == License::CC_BY_NC_SA );

	License licenseCC_BY_NC_ND_0("cc-by-nc-nd");
	CPPUNIT_ASSERT( licenseCC_BY_NC_ND_0.getType() == License::CC_BY_NC_ND );
	License licenseCC_BY_NC_ND_1("CC BY_ND_NC");
	CPPUNIT_ASSERT( licenseCC_BY_NC_ND_1.getType() == License::CC_BY_NC_ND );

	License licenseGPL_0("gpl");
	CPPUNIT_ASSERT( licenseGPL_0.getType() == License::GPL );
	License licenseGPL_1("GPL v3");
	CPPUNIT_ASSERT( licenseGPL_1.getType() == License::GPL );

	License licenseReserved_0("all rights reserved");
	CPPUNIT_ASSERT( licenseReserved_0.getType() == License::AllRightsReserved );

	License licenseOther_0("BSD");
	CPPUNIT_ASSERT( licenseOther_0.getType() == License::Other );
	License licenseOther_1("MIT");
	CPPUNIT_ASSERT( licenseOther_1.getType() == License::Other );	

	License licenseEmpty_0("");
	CPPUNIT_ASSERT( licenseEmpty_0.getType() == License::Unspecified );

}
