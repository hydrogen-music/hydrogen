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

#include <cassert>

#include <hydrogen/basics/note.h>
#include <hydrogen/audio_engine.h>

namespace H2Core
{

const char* Pattern::__class_name = "Pattern";

Pattern::Pattern( const QString& name, const QString& category, int length )
    : Object( __class_name )
    , __length( length )
    , __name( name )
    , __category( category )
{
}

Pattern::Pattern( Pattern* other)
    : Object( __class_name )
    , __length( other->get_length() )
    , __name( other->get_name() )
    , __category( other->get_category() )
{
    for( notes_cst_it_t it=other->get_notes()->begin(); it!=other->get_notes()->end(); it++ ) {
        note_map.insert( std::make_pair( it->first, new Note( it->second ) ) );
    }
}

Pattern::~Pattern()
{
    for( notes_cst_it_t it=note_map.begin(); it!=note_map.end(); it++ ) {
        delete it->second;
    }
}

bool Pattern::references( Instrument* instr )
{
    for( notes_cst_it_t it=note_map.begin(); it!=note_map.end(); it++ ) {
        Note *note = it->second;
        assert( note );
        if ( note->get_instrument() == instr ) {
            return true;
        }
    }
    return false;
}

void Pattern::purge_instrument( Instrument* instr )
{
    bool locked = false;
    std::list< Note* > slate;
    for( notes_it_t it=note_map.begin(); it!=note_map.end(); it++ ) {
        Note* note = it->second;
        assert( note );
        if ( note->get_instrument() == instr ) {
            if ( !locked ) {
                H2Core::AudioEngine::get_instance()->lock( RIGHT_HERE );
                locked = true;
            }
            slate.push_back( note );
            note_map.erase( it );
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

void Pattern::set_to_old() {
    for( notes_cst_it_t it=note_map.begin(); it!=note_map.end(); it++ ) {
        Note *note = it->second;
        assert( note );
        note->set_just_recorded( false );
    }
}

};

/* vim: set softtabstop=4 expandtab: */
