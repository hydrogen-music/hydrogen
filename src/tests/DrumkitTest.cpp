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

#include "DrumkitTest.h"

#include "TestHelper.h"

#include <core/Basics/Event.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Sample.h>

using namespace H2Core;

void DrumkitTest::testDefaultMidiOutNotes()
{
	___INFOLOG( "" );
	InstrumentList list;

	list.add( std::make_shared<Instrument>() );
	list.add( std::make_shared<Instrument>() );
	list.add( std::make_shared<Instrument>() );

	list.setDefaultMidiOutNotes();

	CPPUNIT_ASSERT_EQUAL( 36, list.get( 0 )->getMidiOutNote() );
	CPPUNIT_ASSERT_EQUAL( 37, list.get( 1 )->getMidiOutNote() );
	CPPUNIT_ASSERT_EQUAL( 38, list.get( 2 )->getMidiOutNote() );
	___INFOLOG( "passed" );
}

void DrumkitTest::testHasAllMidiNotesSame()
{
	___INFOLOG( "" );
	// One instrument
	{
		InstrumentList list;
		auto pKick = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Kick" );
		pKick->setMidiOutNote( 42 );
		list.add( pKick );

		CPPUNIT_ASSERT( !list.hasAllMidiNotesSame() );
	}

	// All the same
	{
		InstrumentList list;

		auto pKick = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Kick" );
		pKick->setMidiOutNote( 10 );
		list.add( pKick );

		auto pSnare = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Snare" );
		pSnare->setMidiOutNote( 10 );
		list.add( pSnare );

		auto pHihat = std::make_shared<Instrument>( EMPTY_INSTR_ID, "HiHat" );
		pHihat->setMidiOutNote( 10 );
		list.add( pHihat );

		CPPUNIT_ASSERT_EQUAL( 3, list.size() );
		CPPUNIT_ASSERT( list.hasAllMidiNotesSame() );
	}

	// All different
	{
		InstrumentList list;

		auto pKick = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Kick" );
		pKick->setMidiOutNote( 36 );
		list.add( pKick );

		auto pClap = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Clap" );
		pClap->setMidiOutNote( 37 );
		list.add( pClap );

		auto pRide = std::make_shared<Instrument>( EMPTY_INSTR_ID, "Ride" );
		pRide->setMidiOutNote( 38 );
		list.add( pRide );

		auto pDummy =
			std::make_shared<Instrument>( EMPTY_INSTR_ID, "Dummy Instrument" );
		pDummy->setMidiOutNote( 36 );  // duplicate
		list.add( pDummy );

		CPPUNIT_ASSERT_EQUAL( 4, list.size() );
		CPPUNIT_ASSERT( !list.hasAllMidiNotesSame() );
	}
	___INFOLOG( "passed" );
}

void DrumkitTest::testIsValidIndex()
{
	___INFOLOG( "" );
	InstrumentList list;

	list.add( std::make_shared<Instrument>() );

	CPPUNIT_ASSERT( list.isValidIndex( 0 ) );
	CPPUNIT_ASSERT( !list.isValidIndex( 1 ) );
	CPPUNIT_ASSERT( !list.isValidIndex( -42 ) );
	___INFOLOG( "passed" );
}

void DrumkitTest::testLayerHandling()
{
	___INFOLOG( "" );

	auto pInstrument = std::make_shared<Instrument>();

	// Check default instrument layout.
	CPPUNIT_ASSERT( pInstrument->getComponents()->size() == 1 );
	auto pComponent = pInstrument->getComponent( 0 );
	CPPUNIT_ASSERT( pComponent != nullptr );

	CPPUNIT_ASSERT( pComponent->getLayers().size() == 0 );

	const auto sFileName1 = "hh.wav";
	auto pSample1 = std::make_shared<Sample>(
		H2TEST_FILE( QString( "/drumkits/baseKit/%1" ).arg( sFileName1 ) )
	);
	auto pLayer1 = std::make_shared<InstrumentLayer>( pSample1 );
	const auto sFileName2 = "kick.wav";
	auto pSample2 = std::make_shared<Sample>(
		H2TEST_FILE( QString( "/drumkits/baseKit/%1" ).arg( sFileName2 ) )
	);
	auto pLayer2 = std::make_shared<InstrumentLayer>( pSample2 );

	////////////////////////////////////////////////////////////////////////////
	// Check layer adding and removal

	// Illegal indices should not add layer
	pInstrument->addLayer( pComponent, pLayer1, -2, Event::Trigger::Suppress );
	CPPUNIT_ASSERT( pComponent->getLayers().size() == 0 );
	pInstrument->addLayer(
		pComponent, pLayer1, pComponent->getLayers().size() + 1,
		Event::Trigger::Suppress
	);
	CPPUNIT_ASSERT( pComponent->getLayers().size() == 0 );

	pInstrument->addLayer( pComponent, pLayer1, 0, Event::Trigger::Suppress );
	CPPUNIT_ASSERT( pComponent->getLayers().size() == 1 );
	CPPUNIT_ASSERT(
		pComponent->getLayer( 0 )->getSample()->getFileName() == sFileName1
	);

	pInstrument->addLayer( pComponent, pLayer2, 0, Event::Trigger::Suppress );
	CPPUNIT_ASSERT( pComponent->getLayers().size() == 2 );
	CPPUNIT_ASSERT(
		pComponent->getLayer( 0 )->getSample()->getFileName() == sFileName2
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 1 )->getSample()->getFileName() == sFileName1
	);

	pInstrument->addLayer( pComponent, pLayer2, -1, Event::Trigger::Suppress );
	CPPUNIT_ASSERT( pComponent->getLayers().size() == 3 );
	CPPUNIT_ASSERT(
		pComponent->getLayer( 0 )->getSample()->getFileName() == sFileName2
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 1 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 2 )->getSample()->getFileName() == sFileName2
	);

	// Only valid indices should remove layers
	pInstrument->removeLayer( pComponent, -1, Event::Trigger::Suppress );
	CPPUNIT_ASSERT( pComponent->getLayers().size() == 3 );
	pInstrument->removeLayer(
		pComponent, pComponent->getLayers().size(), Event::Trigger::Suppress
	);
	CPPUNIT_ASSERT( pComponent->getLayers().size() == 3 );

	pInstrument->removeLayer( pComponent, 1, Event::Trigger::Suppress );
	CPPUNIT_ASSERT( pComponent->getLayers().size() == 2 );
	CPPUNIT_ASSERT(
		pComponent->getLayer( 0 )->getSample()->getFileName() == sFileName2
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 1 )->getSample()->getFileName() == sFileName2
	);
	pInstrument->removeLayer( pComponent, 0, Event::Trigger::Suppress );
	pInstrument->removeLayer( pComponent, 0, Event::Trigger::Suppress );
	CPPUNIT_ASSERT( pComponent->getLayers().size() == 0 );

	// Removing without layers should not be harmful.
	pInstrument->removeLayer( pComponent, 0, Event::Trigger::Suppress );

	////////////////////////////////////////////////////////////////////////////
	// set layer and sample
	pInstrument->addLayer( pComponent, pLayer1, 0, Event::Trigger::Suppress );
	CPPUNIT_ASSERT( pComponent->getLayers().size() == 1 );
	CPPUNIT_ASSERT(
		pComponent->getLayer( 0 )->getSample()->getFileName() == sFileName1
	);

	auto pLayer2Copy = std::make_shared<InstrumentLayer>( pLayer2 );
	pInstrument->setLayer(
		pComponent, pLayer2Copy, 0, Event::Trigger::Suppress
	);
	CPPUNIT_ASSERT( pComponent->getLayers().size() == 1 );
	CPPUNIT_ASSERT(
		pComponent->getLayer( 0 )->getSample()->getFileName() == sFileName2
	);

	// Using the wrong layer
	pInstrument->setSample(
		pComponent, pLayer1, pSample1, Event::Trigger::Suppress
	);
	CPPUNIT_ASSERT( pComponent->getLayers().size() == 1 );
	CPPUNIT_ASSERT(
		pComponent->getLayer( 0 )->getSample()->getFileName() == sFileName2
	);

	pInstrument->setSample(
		pComponent, pLayer2Copy, pSample1, Event::Trigger::Suppress
	);
	CPPUNIT_ASSERT( pComponent->getLayers().size() == 1 );
	CPPUNIT_ASSERT(
		pComponent->getLayer( 0 )->getSample()->getFileName() == sFileName1
	);

	pInstrument->removeLayer( pComponent, 0, Event::Trigger::Suppress );
	CPPUNIT_ASSERT( pComponent->getLayers().size() == 0 );

	////////////////////////////////////////////////////////////////////////////
	// move layers
	const auto sFileName3 = "snare.wav";
	auto pSample3 = std::make_shared<Sample>(
		H2TEST_FILE( QString( "/drumkits/baseKit/%1" ).arg( sFileName3 ) )
	);
	auto pLayer3 = std::make_shared<InstrumentLayer>( pSample3 );

	pInstrument->addLayer( pComponent, pLayer1, -1, Event::Trigger::Suppress );
	pInstrument->addLayer( pComponent, pLayer1, -1, Event::Trigger::Suppress );
	pInstrument->addLayer( pComponent, pLayer2, -1, Event::Trigger::Suppress );
	pInstrument->addLayer( pComponent, pLayer1, -1, Event::Trigger::Suppress );
	pInstrument->addLayer( pComponent, pLayer1, -1, Event::Trigger::Suppress );
	pInstrument->addLayer( pComponent, pLayer1, -1, Event::Trigger::Suppress );
	pInstrument->addLayer( pComponent, pLayer3, -1, Event::Trigger::Suppress );
	pInstrument->addLayer( pComponent, pLayer1, -1, Event::Trigger::Suppress );
	CPPUNIT_ASSERT( pComponent->getLayers().size() == 8 );
	CPPUNIT_ASSERT(
		pComponent->getLayer( 0 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 1 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 2 )->getSample()->getFileName() == sFileName2
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 3 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 4 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 5 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 6 )->getSample()->getFileName() == sFileName3
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 7 )->getSample()->getFileName() == sFileName1
	);

	pInstrument->moveLayer( pComponent, 2, 6, Event::Trigger::Suppress );
	CPPUNIT_ASSERT( pComponent->getLayers().size() == 8 );
	CPPUNIT_ASSERT(
		pComponent->getLayer( 0 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 1 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 2 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 3 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 4 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 5 )->getSample()->getFileName() == sFileName3
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 6 )->getSample()->getFileName() == sFileName2
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 7 )->getSample()->getFileName() == sFileName1
	);

	pInstrument->moveLayer( pComponent, 5, 0, Event::Trigger::Suppress );
	CPPUNIT_ASSERT( pComponent->getLayers().size() == 8 );
	CPPUNIT_ASSERT(
		pComponent->getLayer( 0 )->getSample()->getFileName() == sFileName3
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 1 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 2 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 3 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 4 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 5 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 6 )->getSample()->getFileName() == sFileName2
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 7 )->getSample()->getFileName() == sFileName1
	);

	pInstrument->moveLayer(
		pComponent, 0, pComponent->getLayers().size() - 1,
		Event::Trigger::Suppress
	);
	CPPUNIT_ASSERT( pComponent->getLayers().size() == 8 );
	CPPUNIT_ASSERT(
		pComponent->getLayer( 0 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 1 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 2 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 3 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 4 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 5 )->getSample()->getFileName() == sFileName2
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 6 )->getSample()->getFileName() == sFileName1
	);
	CPPUNIT_ASSERT(
		pComponent->getLayer( 7 )->getSample()->getFileName() == sFileName3
	);

	___INFOLOG( "passed" );
}

void DrumkitTest::testInstrumentMove()
{
	___INFOLOG( "" );

	auto pInstrumentList = std::make_shared<InstrumentList>();
	auto pInstrument1 = std::make_shared<Instrument>();
	const QString sInstrument1( "instrument1" );
	pInstrument1->setName( sInstrument1 );
	auto pInstrument2 = std::make_shared<Instrument>();
	const QString sInstrument2( "instrument2" );
	pInstrument2->setName( sInstrument2 );
	auto pInstrument3 = std::make_shared<Instrument>();
	const QString sInstrument3( "instrument3" );
	pInstrument3->setName( sInstrument3 );

	pInstrumentList->add( pInstrument1 );
	pInstrumentList->add( std::make_shared<Instrument>(pInstrument1) );
	pInstrumentList->add( pInstrument2 );
	pInstrumentList->add( std::make_shared<Instrument>(pInstrument1) );
	pInstrumentList->add( std::make_shared<Instrument>(pInstrument1) );
	pInstrumentList->add( std::make_shared<Instrument>(pInstrument1) );
	pInstrumentList->add( pInstrument3 );
	pInstrumentList->add( std::make_shared<Instrument>(pInstrument1) );
	CPPUNIT_ASSERT( pInstrumentList->size() == 8 );
	CPPUNIT_ASSERT( pInstrumentList->get( 0 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 1 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 2 )->getName() == sInstrument2 );
	CPPUNIT_ASSERT( pInstrumentList->get( 3 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 4 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 5 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 6 )->getName() == sInstrument3 );
	CPPUNIT_ASSERT( pInstrumentList->get( 7 )->getName() == sInstrument1 );

	pInstrumentList->move( 2, 6 );
	CPPUNIT_ASSERT( pInstrumentList->size() == 8 );
	CPPUNIT_ASSERT( pInstrumentList->get( 0 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 1 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 2 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 3 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 4 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 5 )->getName() == sInstrument3 );
	CPPUNIT_ASSERT( pInstrumentList->get( 6 )->getName() == sInstrument2 );
	CPPUNIT_ASSERT( pInstrumentList->get( 7 )->getName() == sInstrument1 );

	pInstrumentList->move( 5, 0 );
	CPPUNIT_ASSERT( pInstrumentList->size() == 8 );
	CPPUNIT_ASSERT( pInstrumentList->get( 0 )->getName() == sInstrument3 );
	CPPUNIT_ASSERT( pInstrumentList->get( 1 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 2 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 3 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 4 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 5 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 6 )->getName() == sInstrument2 );
	CPPUNIT_ASSERT( pInstrumentList->get( 7 )->getName() == sInstrument1 );

	pInstrumentList->move( 0, pInstrumentList->size() - 1 );
	CPPUNIT_ASSERT( pInstrumentList->size() == 8 );
	CPPUNIT_ASSERT( pInstrumentList->get( 0 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 1 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 2 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 3 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 4 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 5 )->getName() == sInstrument2 );
	CPPUNIT_ASSERT( pInstrumentList->get( 6 )->getName() == sInstrument1 );
	CPPUNIT_ASSERT( pInstrumentList->get( 7 )->getName() == sInstrument3 );

	___INFOLOG( "passed" );
}
