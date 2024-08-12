/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Note.h>
#include <core/Basics/PatternList.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Legacy.h>
#include <core/Hydrogen.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

namespace H2Core
{

Pattern::Pattern( const QString& name, const QString& info, const QString& sCategory, int length, int denominator )
	: m_nVersion( 0 )
	, __length( length )
	, __denominator( denominator)
	, __name( name )
	, __info( info )
	, __category( sCategory )
	  , m_sDrumkitName( "" )
	  , m_sAuthor( "" )
	  , m_license( License() )
{
	if ( sCategory.isEmpty() ) {
		__category = SoundLibraryDatabase::m_sPatternBaseCategory;
	}
}

Pattern::Pattern( Pattern* other )
	: m_nVersion( other->m_nVersion )
	, __length( other->get_length() )
	, __denominator( other->get_denominator() )
	, __name( other->get_name() )
	, __info( other->get_info() )
	, __category( other->get_category() )
	, m_sDrumkitName( other->m_sDrumkitName )
	, m_sAuthor( other->m_sAuthor )
	, m_license( other->m_license )
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

bool Pattern::loadDoc( const QString& sPatternPath, XMLDoc* pDoc, bool bSilent )
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

Pattern* Pattern::load_file( const QString& sPatternPath )
{
	INFOLOG( QString( "Load pattern %1" ).arg( sPatternPath ) );

	XMLDoc doc;
	if ( ! loadDoc( sPatternPath, &doc, false ) ) {
		// Try former pattern version
		return Legacy::load_drumkit_pattern( sPatternPath );
	}

	XMLNode root = doc.firstChildElement( "drumkit_pattern" );
	const QString sDrumkitName =
		root.read_string( "drumkit_name", "", false, false, false );
	XMLNode pattern_node = root.firstChildElement( "pattern" );
	return load_from( pattern_node, sDrumkitName );
}

Pattern* Pattern::load_from( const XMLNode& node, const QString& sDrumkitName,
							 bool bSilent )
{
	Pattern* pPattern = new Pattern(
	    node.read_string( "name", nullptr, false, false ),
	    node.read_string( "info", "", false, true ),
	    node.read_string( "category", "unknown", false, true, true ),
	    node.read_int( "size", -1, false, false ),
	    node.read_int( "denominator", 4, false, false )
	);

	pPattern->setDrumkitName( sDrumkitName );
	pPattern->m_nVersion = node.read_int(
		"userVersion", pPattern->m_nVersion, false, false, bSilent );
	pPattern->m_sAuthor = node.read_string(
		"author", pPattern->m_sAuthor, false, false, bSilent );
	const License license( node.read_string(
							   "license", pPattern->m_license.getLicenseString(),
							   false, false, bSilent ) );
	pPattern->setLicense( license );

	XMLNode note_list_node = node.firstChildElement( "noteList" );
	if ( !note_list_node.isNull() ) {
		XMLNode note_node = note_list_node.firstChildElement( "note" );
		while ( !note_node.isNull() ) {
			Note* pNote = Note::load_from( note_node, bSilent );
			assert( pNote );
			if ( pNote != nullptr ) {
				pPattern->insert_note( pNote );
			}
			note_node = note_node.nextSiblingElement( "note" );
		}
	}

	// Sanity checks
	//
	// In case no instrument type is assigned to any of the notes contained, we
	// check whether there is a .h2map file shipped with the application
	// corresponding to the name of the kit contained in the pattern. Via
	// instrument id -> instrument type mapping in there we can use it as a
	// fallback to obtain types.
	bool bMissingType = false;
	for ( const auto& [ _ , ppNote ] : pPattern->__notes ) {
		if ( ppNote != nullptr && ppNote->getType().isEmpty() ) {
			bMissingType = true;
			break;
		}
	}

	if ( bMissingType ) {
		const QString sMapFile =
			Filesystem::getDrumkitMap( pPattern->getDrumkitName(), bSilent );

		if ( ! sMapFile.isEmpty() ) {
			const auto pDrumkitMap = DrumkitMap::load( sMapFile, bSilent );
			if ( pDrumkitMap != nullptr ) {
				// We do not replace any type but only set those not defined
				// yet.
				for ( const auto& [ _, ppNote ] : pPattern->__notes ) {
					if ( ppNote != nullptr && ppNote->getType().isEmpty() &&
						 ! pDrumkitMap->getType(
							 ppNote->get_instrument_id() ).isEmpty() ) {
						ppNote->setType(
							pDrumkitMap->getType( ppNote->get_instrument_id() ) );
					}
				}
			}
			else {
				ERRORLOG( QString( "Unable to load .h2map file [%1] to replace missing Types in notes for pattern [%2]" )
						  .arg( sMapFile ).arg( pPattern->get_name() ) );
			}
		}
		else if ( ! bSilent ) {
			INFOLOG( QString( "There are missing Types for notes in pattern [%1] and no corresponding .h2map file for registered drumkit [%2]." )
					 .arg( pPattern->get_name() )
					 .arg( pPattern->getDrumkitName() ) );
		}
	}

	return pPattern;
}

bool Pattern::save_file( const QString& drumkit_name, const QString& pattern_path, bool overwrite ) const
{
	INFOLOG( QString( "Saving pattern into %1" ).arg( pattern_path ) );
	if( !overwrite && Filesystem::file_exists( pattern_path, true ) ) {
		ERRORLOG( QString( "pattern %1 already exists" ).arg( pattern_path ) );
		return false;
	}
	XMLDoc doc;
	XMLNode root = doc.set_root( "drumkit_pattern", "drumkit_pattern" );
	root.write_string( "drumkit_name", drumkit_name );
	save_to( root );
	return doc.write( pattern_path );
}

void Pattern::save_to( XMLNode& node, const std::shared_ptr<Instrument> pInstrumentOnly ) const
{
	XMLNode pattern_node =  node.createNode( "pattern" );
	pattern_node.write_int( "formatVersion", nCurrentFormatVersion );
	pattern_node.write_int( "userVersion", m_nVersion );
	pattern_node.write_string( "name", __name );
	pattern_node.write_string( "author", m_sAuthor );
	pattern_node.write_string( "info", __info );
	pattern_node.write_string( "license", m_license.getLicenseString() );
	pattern_node.write_string( "category", __category );
	pattern_node.write_int( "size", __length );
	pattern_node.write_int( "denominator", __denominator );
	
	int nId = ( pInstrumentOnly == nullptr ? -1 : pInstrumentOnly->get_id() );
	
	XMLNode note_list_node =  pattern_node.createNode( "noteList" );
	for( auto it = __notes.cbegin(); it != __notes.cend(); ++it ) {
		auto pNote = it->second;
		if ( pNote != nullptr &&
			 ( pInstrumentOnly == nullptr ||
			   pNote->get_instrument_id() == nId ) ) {
			XMLNode note_node = note_list_node.createNode( "note" );
			pNote->save_to( note_node );
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

Note* Pattern::find_note( int idx_a, int idx_b,
						  std::shared_ptr<Instrument> pInstrument,
						  bool strict ) const
{
	if ( pInstrument == nullptr ) {
		return nullptr;
	}

	notes_cst_it_t it;
	for ( it = __notes.lower_bound( idx_a );
		  it != __notes.upper_bound( idx_a ); it++ ) {
		Note* pNote = it->second;
		assert( pNote );
		if ( pNote->get_instrument() == pInstrument ) {
			return pNote;
		}
	}
	if ( idx_b == -1 ) {
		return nullptr;
	}
	for ( it = __notes.lower_bound( idx_b );
		  it != __notes.upper_bound( idx_b ); it++ ) {
		Note* pNote = it->second;
		assert( pNote );
		if ( pNote->get_instrument() == pInstrument ) {
			return pNote;
		}
	}
	if ( strict ) {
		return nullptr;
	}

	// TODO maybe not start from 0 but idx_b-X
	for ( int n=0; n<idx_b; n++ ) {
		for ( it = __notes.lower_bound( n );
			  it != __notes.upper_bound( n ); it++ ) {
			Note* pNote = it->second;
			assert( pNote );
			if ( pNote->get_instrument() == pInstrument &&
				 ( ( idx_b <= pNote->get_position() + pNote->get_length() )
				   && idx_b >= pNote->get_position() ) ) {
				return pNote;
			}
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

bool Pattern::references( std::shared_ptr<Instrument> pInstrument ) const
{
	if ( pInstrument == nullptr ) {
		return false;
	}

	for ( const auto& [ _, ppNote ] : __notes ) {
		if ( ppNote != nullptr && ppNote->get_instrument() == pInstrument ) {
			return true;
		}
	}
	return false;
}

void Pattern::purge_instrument( std::shared_ptr<Instrument> pInstrument,
								bool bRequiresLock )
{
	if ( pInstrument == nullptr ) {
		return;
	}

	bool locked = false;
	std::list< Note* > slate;
	for ( notes_it_t it=__notes.begin(); it!=__notes.end(); ) {
		Note* note = it->second;
		assert( note );
		if ( note->get_instrument() == pInstrument ) {
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

void Pattern::mapTo( std::shared_ptr<Drumkit> pDrumkit ) {
	if ( pDrumkit == nullptr ) {
		ERRORLOG( "Invalid drumkit" );
		return;
	}

	for ( auto& [ _, ppNote ] : __notes ) {
		ppNote->mapTo( pDrumkit );
	}
}

std::set<DrumkitMap::Type> Pattern::getAllTypes() const {
	std::set<DrumkitMap::Type> types;

	for ( const auto& [ _, ppNote ] : __notes ) {
		if ( ppNote != nullptr && ! ppNote->getType().isEmpty() ) {
			if ( auto search = types.find( ppNote->getType() );
				 search == types.end() ) {
				const auto [ _, bSuccess ] = types.insert( ppNote->getType() );
				if ( ! bSuccess ) {
					WARNINGLOG( QString( "Unable to insert note type [%1]" ) );
				}
			}
		}
	}

	return types;
}

std::vector<H2Core::Note*> Pattern::getAllNotesOfType(
	const DrumkitMap::Type& sType ) const
{
	std::vector<H2Core::Note*> notes;

	for ( const auto& [ _, ppNote ] : __notes ) {
		if ( ppNote != nullptr && ppNote->getType() == sType ) {
			notes.push_back( ppNote );
		}
	}

	return notes;
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
			.append( QString( "%1%2name: %3\n" ).arg( sPrefix ).arg( s ).arg( __name ) )
			.append( QString( "%1%2m_nVersion: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nVersion ) )
			.append( QString( "%1%2m_sDrumkitName: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_sDrumkitName ) )
			.append( QString( "%1%2m_sAuthor: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sAuthor ) )
			.append( QString( "%1%2m_license: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_license.toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2length: %3\n" ).arg( sPrefix ).arg( s ).arg( __length ) )
			.append( QString( "%1%2denominator: %3\n" ).arg( sPrefix ).arg( s ).arg( __denominator ) )
			.append( QString( "%1%2category: %3\n" ).arg( sPrefix ).arg( s ).arg( __category ) )
			.append( QString( "%1%2info: %3\n" ).arg( sPrefix ).arg( s ).arg( __info ) )
			.append( QString( "%1%2Notes:\n" ).arg( sPrefix ).arg( s ) );
				 
		for ( const auto& [ _, ppNote ] : __notes ) {
			if ( ppNote != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ppNote->toQString(
					sPrefix + s + s, bShort ) ) );
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
	}
	else {

		sOutput = QString( "[Pattern]" )
			.append( QString( " name: %1" ).arg( __name ) )
			.append( QString( ", m_nVersion: %1" ).arg( m_nVersion ) )
			.append( QString( ", m_sDrumkitName: %1" ).arg( m_sDrumkitName ) )
			.append( QString( ", m_sAuthor: %1" ).arg( m_sAuthor ) )
			.append( QString( ", m_license: %1" )
					 .arg( m_license.toQString( sPrefix, bShort ) ) )
			.append( QString( ", length: %1" ).arg( __length ) )
			.append( QString( ", denominator: %1" ).arg( __denominator ) )
			.append( QString( ", category: %1" ).arg( __category ) )
			.append( QString( ", info: %1" ).arg( __info ) )
			.append( QString( ", [Notes: " ) );
		for ( const auto& [ _, ppNote ] : __notes ) {
			if ( ppNote != nullptr ) {
				sOutput.append( QString( "[type: %1, pos: %2, instrument: %3] " )
								.arg( ppNote->getType() )
								.arg( ppNote->get_position() )
								.arg( ppNote->get_instrument() != nullptr ?
									  ppNote->get_instrument()->get_name() :
									  "nullptr" ) );
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
