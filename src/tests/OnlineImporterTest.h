/*
 * Hydrogen
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

#ifndef ONLINE_IMPORTER_TEST_H
#define ONLINE_IMPORTER_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class OnlineImporterTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( OnlineImporterTest );
	CPPUNIT_TEST( testParseValidIndex );
	CPPUNIT_TEST( testParseMalformedEntry );
	CPPUNIT_TEST( testParseEmptyIndex );
	CPPUNIT_TEST( testTopLevelHashValidation );
	CPPUNIT_TEST( testCountMismatchWarning );
	CPPUNIT_TEST( testHashVerificationPass );
	CPPUNIT_TEST( testHashVerificationFail );
	CPPUNIT_TEST( testResolveLocalStatusNotInstalled );
	CPPUNIT_TEST( testResolveLocalStatusInstalled );
	CPPUNIT_TEST( testResolveLocalStatusModified );
	CPPUNIT_TEST( testDownloadArtifactsEmptyList );
	CPPUNIT_TEST( testDownloadArtifactsAbort );
	CPPUNIT_TEST( testDownloadBlockingSuccess );
	CPPUNIT_TEST( testDownloadBlockingHashMismatch );
	CPPUNIT_TEST_SUITE_END();

public:
	void testParseValidIndex();
	void testParseMalformedEntry();
	void testParseEmptyIndex();
	void testTopLevelHashValidation();
	void testCountMismatchWarning();
	void testHashVerificationPass();
	void testHashVerificationFail();
	void testResolveLocalStatusNotInstalled();
	void testResolveLocalStatusInstalled();
	void testResolveLocalStatusModified();
	void testDownloadArtifactsEmptyList();
	void testDownloadArtifactsAbort();
	void testDownloadBlockingSuccess();
	void testDownloadBlockingHashMismatch();
};

#endif
