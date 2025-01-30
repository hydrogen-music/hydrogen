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

#include "MimeTest.h"
#include "TestHelper.h"

#include <core/Logger.h>
#include <core/Object.h>
#include <core/Basics/Playlist.h>

void MimeTest::testPlaylistSerialization(){
	___INFOLOG( "" );

	const auto pPlaylist =
		H2Core::Playlist::load( H2TEST_FILE( "/playlist/test.h2playlist" ) );
	CPPUNIT_ASSERT( pPlaylist != nullptr );
	CPPUNIT_ASSERT( pPlaylist->size() > 0 );

	for ( const auto& ppEntry : *pPlaylist ) {
		CPPUNIT_ASSERT( ppEntry != nullptr );

		const auto sMimeText = ppEntry->toMimeText();
		CPPUNIT_ASSERT( ! sMimeText.isEmpty() );

		const auto pNewEntry = H2Core::PlaylistEntry::fromMimeText( sMimeText );
		CPPUNIT_ASSERT( pNewEntry != nullptr );

		CPPUNIT_ASSERT( pNewEntry == ppEntry );
	}

	___INFOLOG( "passed" );
}
