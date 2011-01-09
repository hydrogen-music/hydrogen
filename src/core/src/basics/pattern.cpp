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

#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/audio_engine.h>

#include <vector>
#include <cassert>
namespace H2Core
{

const char* Pattern::__class_name = "Pattern";

Pattern::Pattern( const QString& name, const QString& category, unsigned length )
		: Object( __class_name )
{
//	INFOLOG( "INIT: " + m_sName );
	set_name( name );
	set_category( category );
	set_length( length );
}



Pattern::~Pattern()
{
//	INFOLOG( "DESTROY: " + m_sName );

	// delete all Notes
	std::multimap <int, Note*>::iterator pos;
	for ( pos = note_map.begin(); pos != note_map.end(); ++pos ) {
		Note *pNote = pos->second;
		delete pNote;
	}
}


void Pattern::purge_instrument( Instrument * I )
{
	bool locked = false;
	std::list< Note* > slate;
	
	std::multimap <int, Note*>::iterator pos = note_map.begin();
	while ( pos != note_map.end() ) {
		Note *pNote = pos->second;
		assert( pNote );
		
		if ( pNote->get_instrument() == I ) {
			if ( !locked ) {
				H2Core::AudioEngine::get_instance()->lock( RIGHT_HERE );
				locked = true;
			}
			slate.push_back( pNote );
			
			note_map.erase( pos++ );
		} else {
			++pos;
		}
	}
	
	if ( locked ) {
		H2Core::AudioEngine::get_instance()->unlock();
		while ( slate.size() ) {
			delete slate.front();
			slate.pop_front();
		}
	}
}


bool Pattern::references_instrument( Instrument * I )
{
	std::multimap <int, Note*>::const_iterator pos;
	for ( pos = note_map.begin(); pos != note_map.end(); ++pos ) {
		Note *pNote = pos->second;
		assert( pNote );
		if ( pNote->get_instrument() == I ) {
			return true;
		}
	}
	return false;
}


void Pattern::set_to_old()
{
 	std::multimap <int, Note*>::const_iterator pos;
	for ( pos = note_map.begin(); pos != note_map.end(); ++pos ) {
		Note *pNote = pos->second;
		assert( pNote );
		pNote->set_just_recorded( false );
	}
}


/// Returns an empty Pattern
Pattern* Pattern::get_empty_pattern()
{
	Pattern *pat = new Pattern( "Pattern", "not_categorized" );
	return pat;
}



Pattern* Pattern::copy()
{
//	ERRORLOG( "not implemented yet!!!" );

	Pattern *newPat = new Pattern( __name, __category );
	newPat->set_length( get_length() );

	std::multimap <int, Note*>::iterator pos;
	for ( pos = note_map.begin(); pos != note_map.end(); ++pos ) {
		Note *pNote = new Note( pos->second );
		newPat->note_map.insert( std::make_pair( pos->first, pNote ) );
	}

	return newPat;
}



void Pattern::debug_dump()
{
	INFOLOG( "Pattern dump" );
	INFOLOG( "Pattern name: " + __name );
	INFOLOG( "Pattern category: " + __category );
	INFOLOG( QString("Pattern length: %1").arg( get_length() ) );
}

};
