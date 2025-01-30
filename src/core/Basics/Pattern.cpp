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

#include <core/Basics/Pattern.h>

#include <cassert>

#include <core/Basics/Note.h>
#include <core/Basics/PatternList.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Hydrogen.h>

#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Legacy.h>

namespace H2Core
{

Pattern::Pattern( const QString& name, const QString& info, const QString& category, int length, int denominator )
	: __length( length )
	, __denominator( denominator)
	, __name( name )
	, __info( info )
	, __category( category )
{
}

Pattern::Pattern( Pattern* other )
	: __length( other->get_length() )
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

bool Pattern::loadDoc( const QString& sPatternPath, std::shared_ptr<InstrumentList> pInstrumentList, XMLDoc* pDoc, bool bSilent )
{
	if ( ! Filesystem::file_readable( sPatternPath, bSilent ) ) {
		return false;
	}

	bool bReadingSuccessful = true;

	if ( ! pDoc->read( sPatternPath, Filesystem::pattern_xsd_path() ) ) {
		if ( ! pDoc->read( sPatternPath, nullptr ) ) {
			ERRORLOG( QString( "Unable to read pattern [%1]" )
					  .arg( sPatternPath ) );
			return false;
		}
		else {
			if ( ! bSilent ) {
				WARNINGLOG( QString( "Pattern [%1] does not validate the current pattern schema. Loading might fail." )
							.arg( sPatternPath ) );
			}
			bReadingSuccessful = false;
		}
	}
	
	XMLNode root = pDoc->firstChildElement( "drumkit_pattern" );
	if ( root.isNull() ) {
		ERRORLOG( QString( "'drumkit_pattern' node not found in [%1]" )
				  .arg( sPatternPath ) );
		return false;
	}
	
	XMLNode pattern_node = root.firstChildElement( "pattern" );
	if ( pattern_node.isNull() ) {
		ERRORLOG( QString( "'pattern' node not found in [%1]" )
				  .arg( sPatternPath ) );
		return false;
	}

	return bReadingSuccessful;
}

Pattern* Pattern::load_file( const QString& sPatternPath, std::shared_ptr<InstrumentList> pInstrumentList )
{
	INFOLOG( QString( "Load pattern %1" ).arg( sPatternPath ) );

	XMLDoc doc;
	const bool bLoadSuccessful = loadDoc(
		sPatternPath, pInstrumentList, &doc, false );

	const XMLNode root = doc.firstChildElement( "drumkit_pattern" );
	XMLNode pattern_node = root.firstChildElement( "pattern" );

	// Check whether the file was created using a newer version of Hydrogen.
	auto formatVersionNode = pattern_node.firstChildElement( "formatVersion" );
	if ( ! formatVersionNode.isNull() ) {
		WARNINGLOG( QString( "Pattern file [%1] was created with a more recent version of Hydrogen than the current one!" )
					.arg( sPatternPath ) );
		// Even in case the future version is invalid with respect to the XSD
		// file, the most recent version of the format will be the most
		// successful one.
	}
	else if ( ! bLoadSuccessful ) {
		// Try former pattern version
		return Legacy::load_drumkit_pattern( sPatternPath, pInstrumentList );
	}
	return load_from( &pattern_node, pInstrumentList );
}

Pattern* Pattern::load_from( XMLNode* node, std::shared_ptr<InstrumentList> pInstrumentList, bool bSilent )
{
	Pattern* pPattern = new Pattern(
	    node->read_string( "name", nullptr, false, false ),
	    node->read_string( "info", "", false, true ),
	    node->read_string( "category", "unknown", false, true, true ),
	    node->read_int( "size", -1, false, false ),
	    node->read_int( "denominator", 4, false, false )
	);

	if ( pInstrumentList == nullptr ) {
		ERRORLOG( "Invalid instrument list provided" );
		return pPattern;
	}
	
	XMLNode note_list_node = node->firstChildElement( "noteList" );
	if ( !note_list_node.isNull() ) {
		XMLNode note_node = note_list_node.firstChildElement( "note" );
		while ( !note_node.isNull() ) {
			Note* pNote = Note::load_from( &note_node, pInstrumentList, bSilent );
			assert( pNote );
			if ( pNote != nullptr ) {
				pPattern->insert_note( pNote );
			}
			note_node = note_node.nextSiblingElement( "note" );
		}
	}
	
	return pPattern;
}

bool Pattern::save_file( const QString& drumkit_name, const QString& author, const License& license, const QString& pattern_path, bool overwrite ) const
{
	INFOLOG( QString( "Saving pattern into %1" ).arg( pattern_path ) );
	if( !overwrite && Filesystem::file_exists( pattern_path, true ) ) {
		ERRORLOG( QString( "pattern %1 already exists" ).arg( pattern_path ) );
		return false;
	}
	XMLDoc doc;
	XMLNode root = doc.set_root( "drumkit_pattern", "drumkit_pattern" );
	root.write_string( "drumkit_name", drumkit_name );
	root.write_string( "author", author );							// FIXME this is never loaded back
	root.write_string( "license", license.getLicenseString() );
	// FIXME this is never loaded back
	save_to( &root );
	return doc.write( pattern_path );
}

void Pattern::save_to( XMLNode* node, const std::shared_ptr<Instrument> pInstrumentOnly ) const
{
	XMLNode pattern_node =  node->createNode( "pattern" );
	pattern_node.write_string( "name", __name );
	pattern_node.write_string( "info", __info );
	pattern_node.write_string( "category", __category );
	pattern_node.write_int( "size", __length );
	pattern_node.write_int( "denominator", __denominator );
	
	int nId = ( pInstrumentOnly == nullptr ? -1 : pInstrumentOnly->get_id() );
	
	XMLNode note_list_node =  pattern_node.createNode( "noteList" );
	for( auto it = __notes.cbegin(); it != __notes.cend(); ++it ) {
		auto pNote = it->second;
		if ( pNote != nullptr &&
			 ( pInstrumentOnly == nullptr ||
			   pNote->get_instrument()->get_id() == nId ) ) {
			XMLNode note_node = note_list_node.createNode( "note" );
			pNote->save_to( &note_node );
		}
	}
}

Note* Pattern::find_note( int idx_a, int idx_b, std::shared_ptr<Instrument> instrument, Note::Key key, Note::Octave octave, bool strict ) const
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

Note* Pattern::find_note( int idx_a, int idx_b, std::shared_ptr<Instrument> instrument, bool strict ) const
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

bool Pattern::references( std::shared_ptr<Instrument> instr )
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

void Pattern::purge_instrument( std::shared_ptr<Instrument> instr, bool bRequiresLock )
{
	bool locked = false;
	std::list< Note* > slate;
	for( notes_it_t it=__notes.begin(); it!=__notes.end(); ) {
		Note* note = it->second;
		assert( note );
		if ( note->get_instrument() == instr ) {
			if ( !locked && bRequiresLock ) {
				Hydrogen::get_instance()->getAudioEngine()->lock( RIGHT_HERE );
				locked = true;
			}
			slate.push_back( note );
			__notes.erase( it++ );
		} else {
			++it;
		}
	}
	if ( locked ) {
		Hydrogen::get_instance()->getAudioEngine()->unlock();
	}
	while ( slate.size() ) {
		delete slate.front();
		slate.pop_front();
	}
}

void Pattern::clear( bool bRequiresLock )
{
	auto pAudioEngine = Hydrogen::get_instance()->getAudioEngine();
	if ( bRequiresLock ){
		pAudioEngine->lock( RIGHT_HERE );
	}
	std::list< Note* > slate;
	for ( notes_it_t it=__notes.begin(); it!=__notes.end(); ) {
		Note* note = it->second;
		assert( note );
		slate.push_back( note );
		__notes.erase( it++ );
	}
	if ( bRequiresLock ) {
		pAudioEngine->unlock();
	}

	while ( slate.size() ) {
		delete slate.front();
		slate.pop_front();
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

void Pattern::addFlattenedVirtualPatterns( PatternList* pPatternList ) {
	for( virtual_patterns_cst_it_t it=__flattened_virtual_patterns.begin();
		 it!=__flattened_virtual_patterns.end(); ++it ) {
		pPatternList->add( *it, true );
	}
}

void Pattern::removeFlattenedVirtualPatterns( PatternList* pPatternList ) {
	for( virtual_patterns_cst_it_t it=__flattened_virtual_patterns.begin();
		 it!=__flattened_virtual_patterns.end(); ++it ) {
		pPatternList->del( *it );
	}
}

int Pattern::longestVirtualPatternLength() const {
	int nMax = __length;
	for ( virtual_patterns_cst_it_t it=__flattened_virtual_patterns.begin();
		 it!=__flattened_virtual_patterns.end(); ++it ) {
		if ( (*it)->__length > nMax ) {
			nMax = (*it)->__length;
		}
	}

	return nMax;
}

bool Pattern::isVirtual() const {
	return __flattened_virtual_patterns.size() > 0;
}

std::set<Pattern*>::iterator Pattern::begin() {
	return __flattened_virtual_patterns.begin();
}

std::set<Pattern*>::iterator Pattern::end() {
	return __flattened_virtual_patterns.end();
}

QString Pattern::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
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
			sOutput.append( ", Virtual_patterns: {" );
		}
		for ( auto ii : __virtual_patterns ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s + s, bShort ) ) );
			}
		}
		if ( __flattened_virtual_patterns.size() != 0 ) {
			sOutput.append( "}, Flattened_virtual_patterns: {" );
		}
		for ( auto ii : __flattened_virtual_patterns ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s + s, bShort ) ) );
			}
		}
		if ( __flattened_virtual_patterns.size() != 0 ) {
			sOutput.append( "}" );
		}
	}	
	return sOutput;
}

};

/* vim: set softtabstop=4 noexpandtab: */
