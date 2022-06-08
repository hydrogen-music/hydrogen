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

#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/Sample.h>

#include <core/Helpers/Xml.h>
#include <core/License.h>

#include <set>

namespace H2Core
{

InstrumentList::InstrumentList()
{
}

InstrumentList::InstrumentList( InstrumentList* other ) : Object( *other )
{
	assert( other );
	assert( __instruments.size() == 0 );
	for ( int i=0; i<other->size(); i++ ) {
		( *this ) << ( std::make_shared<Instrument>( ( *other )[i] ) );
	}
}

InstrumentList::~InstrumentList()
{
}

void InstrumentList::load_samples()
{
	for( int i=0; i<__instruments.size(); i++ ) {
		__instruments[i]->load_samples();
	}
}

void InstrumentList::unload_samples()
{
	for( int i=0; i<__instruments.size(); i++ ) {
		__instruments[i]->unload_samples();
	}
}

InstrumentList* InstrumentList::load_from( XMLNode* node, const QString& dk_path, const QString& dk_name, bool bSilent )
{
	InstrumentList* instruments = new InstrumentList();
	XMLNode instrument_node = node->firstChildElement( "instrument" );
	int count = 0;
	while ( !instrument_node.isNull() ) {
		count++;
		if ( count > MAX_INSTRUMENTS ) {
			ERRORLOG( QString( "instrument count >= %2, stop reading instruments" ).arg( MAX_INSTRUMENTS ) );
			break;
		}
		auto instrument = Instrument::load_from( &instrument_node, dk_path, dk_name, bSilent );
		if( instrument ) {
			( *instruments ) << instrument;
		} else {
			ERRORLOG( QString( "Empty ID for instrument %1. The drumkit is corrupted. Skipping instrument" ).arg( count ) );
			count--;
		}
		instrument_node = instrument_node.nextSiblingElement( "instrument" );
	}
	return instruments;
}

	void InstrumentList::save_to( XMLNode* node, int component_id, bool bRecentVersion )
{
	XMLNode instruments_node = node->createNode( "instrumentList" );
	for ( int i = 0; i < size(); i++ ) {
		( *this )[i]->save_to( &instruments_node, component_id, bRecentVersion );
	}
}

void InstrumentList::operator<<( std::shared_ptr<Instrument> instrument )
{
	// do nothing if already in __instruments
	for( int i=0; i<__instruments.size(); i++ ) {
		if( __instruments[i]==instrument ) return;
	}
	__instruments.push_back( instrument );
}

void InstrumentList::add( std::shared_ptr<Instrument> instrument )
{
	// do nothing if already in __instruments
	for( int i=0; i<__instruments.size(); i++ ) {
		if( __instruments[i]==instrument ) return;
	}
	__instruments.push_back( instrument );
}

void InstrumentList::insert( int idx, std::shared_ptr<Instrument> instrument )
{
	// do nothing if already in __instruments
	for( int i=0; i<__instruments.size(); i++ ) {
		if( __instruments[i]==instrument ) return;
	}
	__instruments.insert( __instruments.begin() + idx, instrument );
}

std::shared_ptr<Instrument> InstrumentList::operator[]( int idx )
{
	if ( idx < 0 || idx >= __instruments.size() ) {
		ERRORLOG( QString( "idx %1 out of [0;%2]" ).arg( idx ).arg( size() ) );
		return nullptr;
	}
	assert( idx >= 0 && idx < __instruments.size() );
	return __instruments[idx];
}

bool InstrumentList::is_valid_index( int idx ) const
{
	bool is_valid_index = true;
	
	if ( idx < 0 || idx >= __instruments.size() ) {
		is_valid_index = false;
	}
	
	return is_valid_index;
}

std::shared_ptr<Instrument> InstrumentList::get( int idx )
{
	if ( !is_valid_index( idx ) ) {
		ERRORLOG( QString( "idx %1 out of [0;%2]" ).arg( idx ).arg( size() ) );
		return nullptr;
	}
	assert( idx >= 0 && idx < __instruments.size() );
	return __instruments[idx];
}

int InstrumentList::index( std::shared_ptr<Instrument> instr )
{
	for( int i=0; i<__instruments.size(); i++ ) {
		if ( __instruments[i]==instr ) return i;
	}
	return -1;
}

std::shared_ptr<Instrument>  InstrumentList::find( const int id )
{
	for( int i=0; i<__instruments.size(); i++ ) {
		if ( __instruments[i]->get_id()==id ) return __instruments[i];
	}
	return nullptr;
}

std::shared_ptr<Instrument>  InstrumentList::find( const QString& name )
{
	for( int i=0; i<__instruments.size(); i++ ) {
		if ( __instruments[i]->get_name()==name ) return __instruments[i];
	}
	return nullptr;
}

std::shared_ptr<Instrument>  InstrumentList::findMidiNote( const int note )
{
	for( int i=0; i<__instruments.size(); i++ ) {
		if ( __instruments[i]->get_midi_out_note()==note ) return __instruments[i];
	}
	return nullptr;
}

std::shared_ptr<Instrument> InstrumentList::del( int idx )
{
	assert( idx >= 0 && idx < __instruments.size() );
	auto instrument = __instruments[idx];
	__instruments.erase( __instruments.begin() + idx );
	return instrument;
}

std::shared_ptr<Instrument> InstrumentList::del( std::shared_ptr<Instrument> instrument )
{
	for( int i=0; i<__instruments.size(); i++ ) {
		if( __instruments[i]==instrument ) {
			__instruments.erase( __instruments.begin() + i );
			return instrument;
		}
	}
	return nullptr;
}

void InstrumentList::swap( int idx_a, int idx_b )
{
	assert( idx_a >= 0 && idx_a < __instruments.size() );
	assert( idx_b >= 0 && idx_b < __instruments.size() );
	if( idx_a == idx_b ) return;
	//DEBUGLOG(QString("===>> SWAP  %1 %2").arg(idx_a).arg(idx_b) );
	auto tmp = __instruments[idx_a];
	__instruments[idx_a] = __instruments[idx_b];
	__instruments[idx_b] = tmp;
}

void InstrumentList::move( int idx_a, int idx_b )
{
	assert( idx_a >= 0 && idx_a < __instruments.size() );
	assert( idx_b >= 0 && idx_b < __instruments.size() );
	if( idx_a == idx_b ) return;
	//DEBUGLOG(QString("===>> MOVE  %1 %2").arg(idx_a).arg(idx_b) );
	auto tmp = __instruments[idx_a];
	__instruments.erase( __instruments.begin() + idx_a );
	__instruments.insert( __instruments.begin() + idx_b, tmp );
}

std::vector<QStringList> InstrumentList::summarizeContent( const std::vector<DrumkitComponent*>* pDrumkitComponents ) const {
	std::vector<QStringList> results;

	for ( const auto& ppInstrument : __instruments ) {
		if ( ppInstrument != nullptr ) {
			for ( const auto& ppInstrumentComponent : *ppInstrument->get_components() ) {
				if ( ppInstrumentComponent != nullptr ) {
					for ( const auto& ppInstrumentLayer : *ppInstrumentComponent ) {
						if ( ppInstrumentLayer != nullptr ) {
							auto pSample = ppInstrumentLayer->get_sample();
							if ( pSample != nullptr ) {
								// Map component ID to component
								// name.
								bool bFound = false;
								QString sComponentName;
								for ( const auto& ppDrumkitComponent : *pDrumkitComponents ) {
									if ( ppInstrumentComponent->get_drumkit_componentID() ==
										 ppDrumkitComponent->get_id() ) {
										bFound = true;
										sComponentName = ppDrumkitComponent->get_name();
										break;
									}
								}

								if ( ! bFound ) {
									sComponentName = pDrumkitComponents->front()->get_name();
								}
									
								results.push_back( QStringList() << 
												   ppInstrument->get_name() <<
												   sComponentName <<
												   pSample->get_filename() <<
												   License::LicenseTypeToQString( pSample->getLicense().getType() ) );
							}
						}
					}
				}
			}
		}
	}

	return std::move( results );
}

void InstrumentList::fix_issue_307()
{
	if ( has_all_midi_notes_same() ) {
		WARNINGLOG( "Same MIDI note assigned to every instrument. Assigning default values." );
		set_default_midi_out_notes();
	}
}

bool InstrumentList::has_all_midi_notes_same() const
{
	if (__instruments.size() < 2) {
		return false;
	}

	std::set<int> notes;
	for( int i=0; i<__instruments.size(); i++ ) {
		auto instr = __instruments[i];
		notes.insert( instr->get_midi_out_note() );
	}
	return notes.size() == 1;
}

void InstrumentList::set_default_midi_out_notes()
{
	for( int i=0; i<__instruments.size(); i++ ) {
		__instruments[i]->set_midi_out_note( i + 36 );
	}
}

QString InstrumentList::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[InstrumentList]\n" ).arg( sPrefix );
		for ( auto ii : __instruments ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s, bShort ) ) );
			}
		}
	} else {
		sOutput = QString( "[InstrumentList] " );
		for ( auto ii : __instruments ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "(%1: %2) " ).arg( ii->get_id() ).arg( ii->get_name() ) );
			}
		}
	}
	
	return sOutput;
}


std::vector<std::shared_ptr<Instrument>>::iterator InstrumentList::begin() {
	return __instruments.begin();
}

std::vector<std::shared_ptr<Instrument>>::iterator InstrumentList::end() {
	return __instruments.end();
}

};

/* vim: set softtabstop=4 noexpandtab: */
