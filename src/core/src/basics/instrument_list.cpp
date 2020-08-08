/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <hydrogen/basics/instrument_list.h>

#include <hydrogen/helpers/xml.h>
#include <hydrogen/basics/instrument.h>

#include <set>

namespace H2Core
{

const char* InstrumentList::__class_name = "InstrumentList";

InstrumentList::InstrumentList() : Object( __class_name )
{
}

InstrumentList::InstrumentList( InstrumentList* other ) : Object( __class_name )
{
	assert( __instruments.size() == 0 );
	for ( int i=0; i<other->size(); i++ ) {
		( *this ) << ( new Instrument( ( *other )[i] ) );
	}
}

InstrumentList::~InstrumentList()
{
	for ( int i = 0; i < __instruments.size(); ++i ) {
		delete __instruments[i];
	}
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

InstrumentList* InstrumentList::load_from( XMLNode* node, const QString& dk_path, const QString& dk_name )
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
		Instrument* instrument = Instrument::load_from( &instrument_node, dk_path, dk_name );
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

void InstrumentList::save_to( XMLNode* node, int component_id )
{
	XMLNode instruments_node = node->createNode( "instrumentList" );
	for ( int i = 0; i < size(); i++ ) {
		( *this )[i]->save_to( &instruments_node, component_id );
	}
}

void InstrumentList::operator<<( Instrument* instrument )
{
	// do nothing if already in __instruments
	for( int i=0; i<__instruments.size(); i++ ) {
		if( __instruments[i]==instrument ) return;
	}
	__instruments.push_back( instrument );
}

void InstrumentList::add( Instrument* instrument )
{
	// do nothing if already in __instruments
	for( int i=0; i<__instruments.size(); i++ ) {
		if( __instruments[i]==instrument ) return;
	}
	__instruments.push_back( instrument );
}

void InstrumentList::insert( int idx, Instrument* instrument )
{
	// do nothing if already in __instruments
	for( int i=0; i<__instruments.size(); i++ ) {
		if( __instruments[i]==instrument ) return;
	}
	__instruments.insert( __instruments.begin() + idx, instrument );
}

Instrument* InstrumentList::operator[]( int idx )
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

Instrument* InstrumentList::get( int idx )
{
	if ( !is_valid_index( idx ) ) {
		ERRORLOG( QString( "idx %1 out of [0;%2]" ).arg( idx ).arg( size() ) );
		return nullptr;
	}
	assert( idx >= 0 && idx < __instruments.size() );
	return __instruments[idx];
}

int InstrumentList::index( Instrument* instr )
{
	for( int i=0; i<__instruments.size(); i++ ) {
		if ( __instruments[i]==instr ) return i;
	}
	return -1;
}

Instrument*  InstrumentList::find( const int id )
{
	for( int i=0; i<__instruments.size(); i++ ) {
		if ( __instruments[i]->get_id()==id ) return __instruments[i];
	}
	return nullptr;
}

Instrument*  InstrumentList::find( const QString& name )
{
	for( int i=0; i<__instruments.size(); i++ ) {
		if ( __instruments[i]->get_name()==name ) return __instruments[i];
	}
	return nullptr;
}

Instrument*  InstrumentList::findMidiNote( const int note )
{
	for( int i=0; i<__instruments.size(); i++ ) {
		if ( __instruments[i]->get_midi_out_note()==note ) return __instruments[i];
	}
	return nullptr;
}

Instrument* InstrumentList::del( int idx )
{
	assert( idx >= 0 && idx < __instruments.size() );
	Instrument* instrument = __instruments[idx];
	__instruments.erase( __instruments.begin() + idx );
	return instrument;
}

Instrument* InstrumentList::del( Instrument* instrument )
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
	Instrument* tmp = __instruments[idx_a];
	__instruments[idx_a] = __instruments[idx_b];
	__instruments[idx_b] = tmp;
}

void InstrumentList::move( int idx_a, int idx_b )
{
	assert( idx_a >= 0 && idx_a < __instruments.size() );
	assert( idx_b >= 0 && idx_b < __instruments.size() );
	if( idx_a == idx_b ) return;
	//DEBUGLOG(QString("===>> MOVE  %1 %2").arg(idx_a).arg(idx_b) );
	Instrument* tmp = __instruments[idx_a];
	__instruments.erase( __instruments.begin() + idx_a );
	__instruments.insert( __instruments.begin() + idx_b, tmp );
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

};

/* vim: set softtabstop=4 noexpandtab: */
