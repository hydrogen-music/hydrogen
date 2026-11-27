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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include "ObjectRegistryTest.h"

#include <core/Object.h>
#include <core/config.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

constexpr std::size_t nClasses = 256;
constexpr int nThreads = 8;

/** A distinct Object class per @a N. Registration happens once per class
 * and process, so every class must be constructed nowhere else. */
template <std::size_t N>
class RegistryProbe : public H2Core::Object<RegistryProbe<N>> {
   public:
	static const char* _class_name()
	{
		static const std::string sName =
			"ObjectRegistryTestProbe" + std::to_string( N );
		return sName.c_str();
	}
	const char* class_name() const override { return _class_name(); }
};

template <std::size_t N>
void constructProbe()
{
	RegistryProbe<N> probe;
}

template <std::size_t... N>
std::array<void ( * )(), sizeof...( N )> probeConstructors(
	std::index_sequence<N...> )
{
	return { &constructProbe<N>... };
}

template <std::size_t... N>
std::array<const char*, sizeof...( N )> probeNames( std::index_sequence<N...> )
{
	return { RegistryProbe<N>::_class_name()... };
}

}  // namespace

void ObjectRegistryTest::testConcurrentFirstConstruction()
{
	___INFOLOG( "" );

#ifdef H2CORE_HAVE_DEBUG
	CPPUNIT_ASSERT( H2Core::Base::count_active() );

	const auto constructors =
		probeConstructors( std::make_index_sequence<nClasses>() );

	std::atomic<int> nReady{ 0 };
	std::atomic<bool> bDone{ false };
	const auto waitForAll = [ &nReady ]() {
		++nReady;
		while ( nReady.load() < nThreads + 1 ) {
			std::this_thread::yield();
		}
	};

	std::vector<std::thread> workers;
	for ( int nThread = 0; nThread < nThreads; ++nThread ) {
		workers.emplace_back( [ &, nThread ]() {
			// Different orders so threads hit the same and different
			// unregistered classes at the same time.
			std::vector<std::size_t> order( nClasses );
			for ( std::size_t ii = 0; ii < nClasses; ++ii ) {
				order[ ii ] = ii;
			}
			std::mt19937 generator( static_cast<unsigned>( nThread ) );
			std::shuffle( order.begin(), order.end(), generator );

			waitForAll();
			for ( const auto nIndex : order ) {
				constructors[ nIndex ]();
			}
		} );
	}
	std::thread reader( [ & ]() {
		waitForAll();
		while ( ! bDone.load() ) {
			H2Core::Base::getObjectMap();
			H2Core::Base::getAliveObjectCount();
		}
	} );

	for ( auto& worker : workers ) {
		worker.join();
	}
	bDone = true;
	reader.join();

	// The map is keyed by the name pointer, so look the classes up by it.
	const auto map = H2Core::Base::getObjectMap();
	for ( const char* sName :
		  probeNames( std::make_index_sequence<nClasses>() ) ) {
		const auto it = map.find( sName );
		CPPUNIT_ASSERT_MESSAGE( std::string( sName ) + " is not registered",
								it != map.end() );
		CPPUNIT_ASSERT_EQUAL_MESSAGE( std::string( sName ),
									  static_cast<unsigned>( nThreads ),
									  static_cast<unsigned>(
										  it->second.constructed ) );
		CPPUNIT_ASSERT_EQUAL_MESSAGE( std::string( sName ),
									  static_cast<unsigned>( nThreads ),
									  static_cast<unsigned>(
										  it->second.destructed ) );
	}
#else
	___INFOLOG( "skipped: the class registry requires H2CORE_HAVE_DEBUG" );
#endif

	___INFOLOG( "passed" );
}
