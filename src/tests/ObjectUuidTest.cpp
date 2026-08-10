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

#include "ObjectUuidTest.h"

#include <core/Object.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>

#include <QHash>
#include <QSet>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace {
/** Minimal concrete Object subclass used to exercise the base-class identity. */
class UuidProbe : public H2Core::Object<UuidProbe> {
	H2_OBJECT( UuidProbe )
   public:
	UuidProbe() = default;
	UuidProbe( const UuidProbe& other ) : H2Core::Object<UuidProbe>( other ) {}
};
}

using H2Core::Uuid;

void ObjectUuidTest::testUniqueness() {
	// Many freshly constructed objects all carry distinct, non-null ids,
	// sharing the single per-process epoch.
	std::vector<std::shared_ptr<UuidProbe>> probes;
	QSet<Uuid> seen;
	const int nCount = 1000;
	for ( int i = 0; i < nCount; ++i ) {
		auto p = std::make_shared<UuidProbe>();
		CPPUNIT_ASSERT( ! p->getUuid().isNull() );
		CPPUNIT_ASSERT( ! seen.contains( p->getUuid() ) );
		seen.insert( p->getUuid() );
		probes.push_back( p );
	}
	CPPUNIT_ASSERT_EQUAL( nCount, static_cast<int>( seen.size() ) );

	// All ids minted in this process share one epoch.
	const auto epoch = probes.front()->getUuid().epoch;
	for ( const auto& p : probes ) {
		CPPUNIT_ASSERT_EQUAL( epoch, p->getUuid().epoch );
	}
}

void ObjectUuidTest::testFreshOnCopy() {
	// Copy-construction mints a brand-new identity ("pointer identity as a
	// value"): the copy is a distinct object, so it gets a distinct id.
	UuidProbe original;
	UuidProbe copy( original );
	CPPUNIT_ASSERT( original.getUuid() != copy.getUuid() );
	CPPUNIT_ASSERT( ! copy.getUuid().isNull() );

	// The same holds through shared_ptr cloning of a copy.
	auto a = std::make_shared<UuidProbe>();
	auto b = std::make_shared<UuidProbe>( *a );
	CPPUNIT_ASSERT( a->getUuid() != b->getUuid() );
}

void ObjectUuidTest::testStableUnderAssignment() {
	// Copy-assignment transfers data, not identity: an object keeps the id it
	// was born with (immutable thereafter).
	UuidProbe a;
	UuidProbe b;
	const Uuid idA = a.getUuid();
	const Uuid idB = b.getUuid();
	CPPUNIT_ASSERT( idA != idB );

	a = b;
	CPPUNIT_ASSERT( idA == a.getUuid() );
	CPPUNIT_ASSERT( idB == b.getUuid() );
}

void ObjectUuidTest::testValueSemantics() {
	// Equality is reflexive and structural; equal ids hash equally so the value
	// works as a key in Qt associative containers.
	Uuid x( 42, 7 );
	Uuid y( 42, 7 );
	Uuid z( 42, 8 );
	CPPUNIT_ASSERT( x == y );
	CPPUNIT_ASSERT( x != z );
	CPPUNIT_ASSERT_EQUAL( qHash( x ), qHash( y ) );

	QHash<Uuid, int> map;
	map.insert( x, 1 );
	CPPUNIT_ASSERT( map.contains( y ) );
	CPPUNIT_ASSERT_EQUAL( 1, map.value( z, 1 ) );

	CPPUNIT_ASSERT( ! x.toQString().isEmpty() );
	CPPUNIT_ASSERT( x.toQString() != z.toQString() );

	CPPUNIT_ASSERT( Uuid().isNull() );
}

void ObjectUuidTest::testCrossProcessEpoch() {
	// A foreign-process id (same counter, different epoch) must never spuriously
	// match a local one — the epoch is what makes ids split-safe.
	Uuid local( 0xAAAAAAAAAAAAAAAAull, 5 );
	Uuid foreign( 0xBBBBBBBBBBBBBBBBull, 5 );
	CPPUNIT_ASSERT( local != foreign );
	CPPUNIT_ASSERT( local.counter == foreign.counter );
	CPPUNIT_ASSERT( local.epoch != foreign.epoch );
}

void ObjectUuidTest::testInstrumentIdentityVsId() {
	using namespace H2Core;
	// The death-row / kit-switch hazard (ADR 0028): two instruments can legally
	// share an Instrument::Id (it is only kit-unique), so identity must NOT be
	// decided by Id. Distinct objects always have distinct identities.
	const auto sharedId = Instrument::Id( 3 );
	auto a = std::make_shared<Instrument>( sharedId, "A" );
	auto b = std::make_shared<Instrument>( sharedId, "B" );

	CPPUNIT_ASSERT( a->getId() == b->getId() );        // identical Id ...
	CPPUNIT_ASSERT( a->getUuid() != b->getUuid() );    // ... distinct identity
	CPPUNIT_ASSERT( ! sameObject( a, b ) );
	CPPUNIT_ASSERT( sameObject( a, a ) );

	// A list locates each by identity, not by the shared Id, returning the
	// correct distinct positions.
	InstrumentList list;
	list.add( a );
	list.add( b );
	CPPUNIT_ASSERT_EQUAL( 2, list.size() );
	CPPUNIT_ASSERT_EQUAL( 0, list.index( a ) );
	CPPUNIT_ASSERT_EQUAL( 1, list.index( b ) );
	CPPUNIT_ASSERT_EQUAL( 0, list.index( a->getUuid() ) );
	CPPUNIT_ASSERT_EQUAL( 1, list.index( b->getUuid() ) );

	// del() by identity removes exactly the intended instrument.
	CPPUNIT_ASSERT( list.del( a->getUuid() ) == a );
	CPPUNIT_ASSERT_EQUAL( 1, list.size() );
	CPPUNIT_ASSERT_EQUAL( 0, list.index( b ) );
	CPPUNIT_ASSERT_EQUAL( -1, list.index( a ) );
}
