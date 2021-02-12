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

#include <core/Basics/Pattern.h>

#include <cassert>

#include <core/Basics/Note.h>
#include <core/Basics/PatternList.h>
#include <core/AudioEngine.h>

#include <core/Helpers/Xml.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Legacy.h>

namespace H2Core
{

const char* Pattern::__class_name = "Pattern";

Pattern::Pattern( const QString& name, const QString& info, const QString& category, int length, int denominator )
	: Object( __class_name )
	, __length( length )
	, __denominator( denominator)
	, __name( name )
	, __info( info )
	, __category( category )
{
}

Pattern::Pattern( Pattern* other )
	: Object( __class_name )
	, __length( other->get_length() )
	, __denominator( other->get_denominator() )
	, __name( other->get_name() )
	, __info( other->get_info() )
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
	INFOLOG( QString( "Load pattern %1" ).arg( pattern_path ) );
	if ( !Filesystem::file_readable( pattern_path ) ) return nullptr;
	XMLDoc doc;
	if( !doc.read( pattern_path, Filesystem::pattern_xsd_path() ) ) {
		return Legacy::load_drumkit_pattern( pattern_path, instruments );
	}
	XMLNode root = doc.firstChildElement( "drumkit_pattern" );
	if ( root.isNull() ) {
		ERRORLOG( "drumkit_pattern node not found" );
		return nullptr;
	}
	XMLNode pattern_node = root.firstChildElement( "pattern" );
	if ( pattern_node.isNull() ) {
		ERRORLOG( "pattern node not found" );
		return nullptr;
	}
	return load_from( &pattern_node, instruments );
}

Pattern* Pattern::load_from( XMLNode* node, InstrumentList* instruments )
{
	Pattern* pattern = new Pattern(
	    node->read_string( "name", nullptr, false, false ),
	    node->read_string( "info", "", false, false ),
	    node->read_string( "category", "unknown", false, false ),
	    node->read_int( "size", -1, false, false ),
	    node->read_int( "denominator", 4, false, false )
	);
	// FIXME support legacy xml element pattern_name, should once be removed
	if ( pattern->get_name().isEmpty() ) {
	    pattern->set_name( node->read_string( "pattern_name", "unknown", false, false ) );
	}
	XMLNode note_list_node = node->firstChildElement( "noteList" );
	if ( !note_list_node.isNull() ) {
		XMLNode note_node = note_list_node.firstChildElement( "note" );
		while ( !note_node.isNull() ) {
			Note* note = Note::load_from( &note_node, instruments );
			if( note ) {
				pattern->insert_note( note );
			}
			note_node = note_node.nextSiblingElement( "note" );
		}
	}
	return pattern;
}

bool Pattern::save_file( const QString& drumkit_name, const QString& author, const QString& license, const QString& pattern_path, bool overwrite ) const
{
	INFOLOG( QString( "Saving pattern into %1" ).arg( pattern_path ) );
	if( !overwrite && Filesystem::file_exists( pattern_path, true ) ) {
		ERRORLOG( QString( "pattern %1 already exists" ).arg( pattern_path ) );
		return false;
	}
	XMLDoc doc;
	XMLNode root = doc.set_root( "drumkit_pattern", "drumkit_pattern" );
	root.write_string( "drumkit_name", drumkit_name );				// FIXME loaded with LocalFileMng::getDrumkitNameForPattern(…)
	root.write_string( "author", author );							// FIXME this is never loaded back
	root.write_string( "license", license );						// FIXME this is never loaded back
	save_to( &root );
	return doc.write( pattern_path );
}

void Pattern::save_to( XMLNode* node, const Instrument* instrumentOnly ) const
{
	XMLNode pattern_node =  node->createNode( "pattern" );
	pattern_node.write_string( "name", __name );
	pattern_node.write_string( "info", __info );
	pattern_node.write_string( "category", __category );
	pattern_node.write_int( "size", __length );
	pattern_node.write_int( "denominator", __denominator );
	XMLNode note_list_node =  pattern_node.createNode( "noteList" );
	int id = ( instrumentOnly == nullptr ? -1 : instrumentOnly->get_id() );
	for( auto it=__notes.cbegin(); it!=__notes.cend(); ++it ) {
		Note* note = it->second;
		if( note && ( instrumentOnly == nullptr || note->get_instrument()->get_id() == id ) ) {
			XMLNode note_node = note_list_node.createNode( "note" );
			note->save_to( &note_node );
		}
	}
}

Note* Pattern::find_note( int idx_a, int idx_b, Instrument* instrument, Note::Key key, Note::Octave octave, bool strict ) const
{
	for( notes_cst_it_t it=__notes.lower_bound( idx_a ); it!=__notes.upper_bound( idx_a ); it++ ) {
		Note* note = it->second;
		assert( note );
		if ( note->match( instrument, key, octave ) ) return note;
	}
	if( idx_b==-1 ) return nullptr;
	for( notes_cst_it_t it=__notes.lower_bound( idx_b ); it!=__notes.upper_bound( idx_b ); it++ ) {
		Note* note = it->second;
		assert( note );
		if ( note->match( instrument, key, octave ) ) return note;
	}
	if( strict ) return nullptr;
	// TODO maybe not start from 0 but idx_b-X
	for ( int n=0; n<idx_b; n++ ) {
		for( notes_cst_it_t it=__notes.lower_bound( n ); it!=__notes.upper_bound( n ); it++ ) {
			Note* note = it->second;
			assert( note );
			if ( note->match( instrument, key, octave ) && ( ( idx_b<=note->get_position()+note->get_length() ) && idx_b>=note->get_position() ) ) return note;
		}
	}
	return nullptr;
}

Note* Pattern::find_note( int idx_a, int idx_b, Instrument* instrument, bool strict ) const
{
	notes_cst_it_t it;
	for( it=__notes.lower_bound( idx_a ); it!=__notes.upper_bound( idx_a ); it++ ) {
		Note* note = it->second;
		assert( note );
		if ( note->get_instrument() == instrument ) return note;
	}
	if( idx_b==-1 ) return nullptr;
	for( it=__notes.lower_bound( idx_b ); it!=__notes.upper_bound( idx_b ); it++ ) {
		Note* note = it->second;
		assert( note );
		if ( note->get_instrument() == instrument ) return note;
	}
	if ( strict ) return nullptr;
	// TODO maybe not start from 0 but idx_b-X
	for ( int n=0; n<idx_b; n++ ) {
		for( it=__notes.lower_bound( n ); it!=__notes.upper_bound( n ); it++ ) {
			Note* note = it->second;
			assert( note );
			if ( note->get_instrument() == instrument && ( ( idx_b<=note->get_position()+note->get_length() ) && idx_b>=note->get_position() ) ) return note;
		}
	}

	return nullptr;
}

void Pattern::remove_note( Note* note )
{
	int pos = note->get_position();
	for( notes_it_t it=__notes.lower_bound( pos ); it!=__notes.end() && it->first == pos; ++it ) {
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
	for( notes_it_t it=__notes.begin(); it!=__notes.end(); ) {
		Note* note = it->second;
		assert( note );
		if ( note->get_instrument() == instr ) {
			if ( !locked ) {
				H2Core::AudioEngine::get_instance()->lock( RIGHT_HERE );
				locked = true;
			}
			slate.push_back( note );
			__notes.erase( it++ );
		} else {
			++it;
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
		// for each pattern of it's flattened virtual pattern set
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

QString Pattern::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Logger::printIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Pattern]\n" ).arg( sPrefix )
			.append( QString( "%1%2length: %3\n" ).arg( sPrefix ).arg( s ).arg( __length ) )
			.append( QString( "%1%2denominator: %3\n" ).arg( sPrefix ).arg( s ).arg( __denominator ) )
			.append( QString( "%1%2name: %3\n" ).arg( sPrefix ).arg( s ).arg( __name ) )
			.append( QString( "%1%2category: %3\n" ).arg( sPrefix ).arg( s ).arg( __category ) )
			.append( QString( "%1%2info: %3\n" ).arg( sPrefix ).arg( s ).arg( __info ) )
			.append( QString( "%1%2Notes:\n" ).arg( sPrefix ).arg( s ) );
				 
		for ( auto it = __notes.begin(); it != __notes.end(); it++ ) {
			if ( it->second != nullptr ) {
				sOutput.append( QString( "%1" ).arg( it->second->toQString( sPrefix + s + s, bShort ) ) );
			}
		}

		sOutput.append( QString( "%1%2Virtual_patterns:\n" ).arg( sPrefix ).arg( s ) );
		for ( auto ii : __virtual_patterns ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s + s, bShort ) ) );
			}
		}

		sOutput.append( QString( "%1%2Flattened_virtual_patterns:\n" ).arg( sPrefix ).arg( s ) );
		for ( auto ii : __flattened_virtual_patterns ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s + s, bShort ) ) );
			}
		}
	} else {
		
		sOutput = QString( "[Pattern]" )
			.append( QString( " length: %1" ).arg( __length ) )
			.append( QString( ", denominator: %1" ).arg( __denominator ) )
			.append( QString( ", name: %1" ).arg( __name ) )
			.append( QString( ", category: %1" ).arg( __category ) )
			.append( QString( ", info: %1" ).arg( __info ) )
			.append( QString( ", [Notes: " ) );
		for ( auto it = __notes.begin(); it != __notes.end(); it++ ) {
			if ( it->second != nullptr ) {
				sOutput.append( QString( "[%2, %3] " )
								.arg( it->second->get_instrument()->get_name() )
								.arg( it->second->get_position() ) );
			}
		}
		sOutput.append( "]" );
		if ( __virtual_patterns.size() != 0 ) {
			sOutput.append( QString( ", Virtual_patterns:" ) );
		}
		for ( auto ii : __virtual_patterns ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s + s, bShort ) ) );
			}
		}
		if ( __flattened_virtual_patterns.size() != 0 ) {
			sOutput.append( QString( ", Flattened_virtual_patterns:" ) );
		}
		for ( auto ii : __flattened_virtual_patterns ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s + s, bShort ) ) );
			}
		}
	}	
	return sOutput;
}
 
void Pattern::Print( bool bShort ) const {
	DEBUGLOG( toQString( "", bShort ) );
}
std::ostream& operator<<( std::ostream& os, const Pattern& pattern ) {
	return os << pattern.toQString( "", true ).toLocal8Bit().data() << std::endl;
}
std::ostream& operator<<( std::ostream& os, const Pattern* pattern ) {
	return os << pattern->toQString( "", true ).toLocal8Bit().data() << std::endl;
}

};

/* vim: set softtabstop=4 noexpandtab: */
