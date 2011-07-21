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
#include <hydrogen/basics/pattern_list.h>
#include <hydrogen/audio_engine.h>

#include <hydrogen/helpers/xml.h>
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/helpers/legacy.h>

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

Pattern::Pattern( Pattern* other )
    : Object( __class_name )
    , __length( other->get_length() )
    , __name( other->get_name() )
    , __category( other->get_category() )
{
    FOREACH_NOTE_CST_IT_BEGIN_END( other->get_notes(),it ) {
        __notes.insert( std::make_pair( it->first, new Note( it->second ) ) );
    }
}

Pattern::~Pattern()
{
    for( notes_cst_it_t it=__notes.begin(); it!=__notes.end(); it++ ) {
        delete it->second;
    }
}

Pattern* Pattern::load_file( const QString& pattern_path, InstrumentList* instruments )
{
    INFOLOG( QString("Load pattern %1").arg(pattern_path) );
    if ( !Filesystem::file_readable( pattern_path ) ) return 0;
    XMLDoc doc;
    if( !doc.read( pattern_path, Filesystem::drumkit_pattern_xsd() ) ) {
        return Legacy::load_drumkit_pattern( pattern_path );
    }
    XMLNode root = doc.firstChildElement( "drumkit_pattern" );
    if ( root.isNull() ) {
        ERRORLOG( "drumkit_pattern node not found" );
        return 0;
    }
    XMLNode pattern_node = root.firstChildElement( "pattern" );
    if ( pattern_node.isNull() ) {
        ERRORLOG( "pattern node not found" );
        return 0;
    }
    return load_from( &pattern_node, instruments );
}

Pattern* Pattern::load_from( XMLNode* node, InstrumentList* instruments )
{
    Pattern *pattern = new Pattern(
            node->read_string( "name", "unknown", false, false ),
            node->read_string( "category", "unknown", false, false ),
            node->read_int( "size", -1, false, false )
            );
    XMLNode note_list_node = node->firstChildElement( "noteList" );
	if ( !note_list_node.isNull() ) {
        XMLNode note_node = note_list_node.firstChildElement( "note" );
        while ( !note_node.isNull() ) {
            Note *note = Note::load_from( &note_node, instruments );
            if(note) {
                pattern->insert_note( note);
            }
            note_node = note_node.nextSiblingElement( "note" );
        }
    }
    return pattern;
}

Note* Pattern::find_note( int idx_a, int idx_b, Instrument* instrument, Note::Key key, Note::Octave octave, bool strict )
{
    for( notes_cst_it_t it=__notes.lower_bound( idx_a ); it!=__notes.upper_bound( idx_a ); it++ ) {
        Note* note = it->second;
        assert( note );
        if ( note->match( instrument, key, octave ) ) return note;
    }
    if( idx_b==-1 ) return 0;
    for( notes_cst_it_t it=__notes.lower_bound( idx_b ); it!=__notes.upper_bound( idx_b ); it++ ) {
        Note* note = it->second;
        assert( note );
        if ( note->match( instrument, key, octave ) ) return note;
    }
    if( strict ) return 0;
    // TODO maybe not start from 0 but idx_b-X
    for ( int n=0; n<idx_b; n++ ) {
        for( notes_cst_it_t it=__notes.lower_bound( n ); it!=__notes.upper_bound( n ); it++ ) {
            Note* note = it->second;
            assert( note );
            if ( note->match( instrument, key, octave ) && ( ( idx_b<=note->get_position()+note->get_length() ) && idx_b>=note->get_position() ) ) return note;
        }
    }
    return 0;
}

Note* Pattern::find_note( int idx_a, int idx_b, Instrument* instrument, bool strict )
{
    notes_cst_it_t it;
    for( it=__notes.lower_bound( idx_a ); it!=__notes.upper_bound( idx_a ); it++ ) {
        Note* note = it->second;
        assert( note );
        if ( note->get_instrument() == instrument ) return note;
    }
    if( idx_b==-1 ) return 0;
    for( it=__notes.lower_bound( idx_b ); it!=__notes.upper_bound( idx_b ); it++ ) {
        Note* note = it->second;
        assert( note );
        if ( note->get_instrument() == instrument ) return note;
    }
    if ( strict ) return 0;
    // TODO maybe not start from 0 but idx_b-X
    for ( int n=0; n<idx_b; n++ ) {
        for( it=__notes.lower_bound( n ); it!=__notes.upper_bound( n ); it++ ) {
            Note* note = it->second;
            assert( note );
            if ( note->get_instrument() == instrument && ( ( idx_b<=note->get_position()+note->get_length() ) && idx_b>=note->get_position() ) ) return note;
        }
        return 0;
    }
}

void Pattern::remove_note( Note* note )
{
    for( notes_it_t it=__notes.begin(); it!=__notes.end(); ++it ) {
        if( it->second==note ) {
            __notes.erase( it );
            break;
        }
    }
}

bool Pattern::references( Instrument* instr )
{
    for( notes_cst_it_t it=__notes.begin(); it!=__notes.end(); it++ ) {
        Note* note = it->second;
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
    for( notes_it_t it=__notes.begin(); it!=__notes.end(); it++ ) {
        Note* note = it->second;
        assert( note );
        if ( note->get_instrument() == instr ) {
            if ( !locked ) {
                H2Core::AudioEngine::get_instance()->lock( RIGHT_HERE );
                locked = true;
            }
            slate.push_back( note );
            __notes.erase( it );
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

void Pattern::set_to_old()
{
    for( notes_cst_it_t it=__notes.begin(); it!=__notes.end(); it++ ) {
        Note* note = it->second;
        assert( note );
        note->set_just_recorded( false );
    }
}

void Pattern::flattened_virtual_patterns_compute()
{
    // __flattened_virtual_patterns must have been cleared before
    if( __flattened_virtual_patterns.size() >= __virtual_patterns.size() ) return;
    // for each virtual pattern
    for( virtual_patterns_cst_it_t it0=__virtual_patterns.begin(); it0!=__virtual_patterns.end(); ++it0 ) {
        __flattened_virtual_patterns.insert( *it0 );        // add it
        ( *it0 )->flattened_virtual_patterns_compute();     // build it's flattened virtual patterns set
        // for each pattern of it's flattened virtual patern set
        for( virtual_patterns_cst_it_t it1=( *it0 )->get_flattened_virtual_patterns()->begin(); it1!=( *it0 )->get_flattened_virtual_patterns()->end(); ++it1 ) {
            // add the pattern
            __flattened_virtual_patterns.insert( *it1 );
        }
    }
}

void Pattern::extand_with_flattened_virtual_patterns( PatternList* patterns )
{
    for( virtual_patterns_cst_it_t it=__flattened_virtual_patterns.begin(); it!=__flattened_virtual_patterns.end(); ++it ) {
        patterns->add( *it );
    }
}

};

/* vim: set softtabstop=4 expandtab: */
