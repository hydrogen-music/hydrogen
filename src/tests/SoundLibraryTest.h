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

#include <core/config.h>

#include <cppunit/extensions/HelperMacros.h>
#include <core/License.h>

class SoundLibraryTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( SoundLibraryTest );
	CPPUNIT_TEST( testContextValidity );
	CPPUNIT_TEST( testFindArtifactPatternDoesNotReturnSong );
	CPPUNIT_TEST( testFindArtifactStackedFindsLaterContext );
	CPPUNIT_TEST( testFindArtifactStackedSkipsNonMatching );
	CPPUNIT_TEST( testKitRetrievalCopy );
	CPPUNIT_TEST( testKitRetrievalDirect );
	CPPUNIT_TEST( testSnapshotStableAcrossUpdate );
	CPPUNIT_TEST( testCustomDrumkitPathSurvivesUpdate );
	CPPUNIT_TEST( testGetDrumkitPublishesSnapshot );
	CPPUNIT_TEST( testGetDrumkitCanonicalizesPath );
	CPPUNIT_TEST( testGetDrumkitUpgradeParameter );
	CPPUNIT_TEST_SUITE_END();
	
public:
		/** No kit in the sound library must be of Context::Song. */
		void testContextValidity();
		/** A Pattern lookup must not fall through into the Song branch
		 * of findArtifact() and return the path of a song of the same
		 * name. */
		void testFindArtifactPatternDoesNotReturnSong();
		/** A stacked findArtifact() lookup has to find artifacts of a
		 * later context through the cached first pass. */
		void testFindArtifactStackedFindsLaterContext();
		/** In a stacked findArtifact() lookup only artifacts matching by
		 * name but not by context may be cached for a later pass. */
		void testFindArtifactStackedSkipsNonMatching();
	void testKitRetrievalCopy();
	void testKitRetrievalDirect();
	/** A snapshot held via getSnapshot() must remain valid and
	 * unchanged across a full update() of the database. */
	void testSnapshotStableAcrossUpdate();
	/** A drumkit path registered via registerCustomDrumkitPath() must
	 * survive a full update() of the database. */
	void testCustomDrumkitPathSurvivesUpdate();
	/** A lazy getDrumkit() load of a kit outside the scanned contexts
	 * must publish a new snapshot without affecting previously held
	 * ones. */
	void testGetDrumkitPublishesSnapshot();
	/** getDrumkit() must address a kit by a canonical path: folder
	 * spellings and redundant separators hit the cached kit instead
	 * of missing it and registering duplicates of it. */
	void testGetDrumkitCanonicalizesPath();
	/** getDrumkit() must honor its bUpgrade argument: an opt-out must
	 * not rewrite a legacy kit on disk. */
	void testGetDrumkitUpgradeParameter();
};
