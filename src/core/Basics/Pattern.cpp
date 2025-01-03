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

Pattern::Pattern( const QString& sName, const QString& sInfo,
				  const QString& sCategory, int nLength, int nDenominator )
	: m_nVersion( 0 )
	, m_sDrumkitName( "" )
	, m_sAuthor( "" )
	, m_license( License() )
	, m_nLength( nLength )
	, m_nDenominator( nDenominator )
	, m_sName( sName )
	, m_sCategory( sCategory )
	, m_sInfo( sInfo )
{
	if ( sCategory.isEmpty() ) {
		m_sCategory = SoundLibraryDatabase::m_sPatternBaseCategory;
	}
}

Pattern::Pattern( std::shared_ptr<Pattern> pOther )
	: m_nVersion( pOther->m_nVersion )
	, m_sDrumkitName( pOther->m_sDrumkitName )
	, m_sAuthor( pOther->m_sAuthor )
	, m_license( pOther->m_license )
	, m_nLength( pOther->getLength() )
	, m_nDenominator( pOther->getDenominator() )
	, m_sName( pOther->getName() )
	, m_sCategory( pOther->getCategory() )
	, m_sInfo( pOther->getInfo() )
{
	FOREACH_NOTE_CST_IT_BEGIN_END( pOther->getNotes(),it ) {
		m_notes.insert( std::make_pair( it->first,
										std::make_shared<Note>( it->second ) ) );
	}

	for ( const auto& ppPattern : pOther->m_virtualPatterns ) {
		m_virtualPatterns.insert( std::make_shared<Pattern>( ppPattern ) );
	}
	for ( const auto& ppPattern : pOther->m_flattenedVirtualPatterns ) {
		m_flattenedVirtualPatterns.insert(
			std::make_shared<Pattern>( ppPattern ) );
	}
}

Pattern::~Pattern() {
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

std::shared_ptr<Pattern> Pattern::load( const QString& sPatternPath )
{
	INFOLOG( QString( "Load pattern %1" ).arg( sPatternPath ) );

	XMLDoc doc;
	if ( ! loadDoc( sPatternPath, &doc, false ) ) {
		// Try former pattern version
		return Legacy::loadPattern( sPatternPath );
	}

	XMLNode root = doc.firstChildElement( "drumkit_pattern" );
	const QString sDrumkitName =
		root.read_string( "drumkit_name", "", false, false, false );
	XMLNode pattern_node = root.firstChildElement( "pattern" );
	return loadFrom( pattern_node, sDrumkitName );
}

std::shared_ptr<Pattern> Pattern::loadFrom( const XMLNode& node,
											 const QString& sDrumkitName,
											 bool bSilent )
{
	auto pPattern = std::make_shared<Pattern>(
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
		"author", pPattern->m_sAuthor, false, true, bSilent );
	const License license( node.read_string(
							   "license", pPattern->m_license.getLicenseString(),
							   false, false, bSilent ) );
	pPattern->setLicense( license );

	XMLNode note_list_node = node.firstChildElement( "noteList" );
	if ( !note_list_node.isNull() ) {
		XMLNode note_node = note_list_node.firstChildElement( "note" );
		while ( !note_node.isNull() ) {
			auto pNote = Note::loadFrom( note_node, bSilent );
			assert( pNote );
			if ( pNote != nullptr &&
				 ( pNote->getInstrumentId() != EMPTY_INSTR_ID ||
				   ! pNote->getType().isEmpty() ) ) {
				pPattern->insertNote( pNote );
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
	for ( const auto& [ _ , ppNote ] : pPattern->m_notes ) {
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
				for ( const auto& [ _, ppNote ] : pPattern->m_notes ) {
					if ( ppNote != nullptr && ppNote->getType().isEmpty() &&
						 ! pDrumkitMap->getType(
							 ppNote->getInstrumentId() ).isEmpty() ) {
						ppNote->setType(
							pDrumkitMap->getType( ppNote->getInstrumentId() ) );
					}
				}
			}
			else {
				ERRORLOG( QString( "Unable to load .h2map file [%1] to replace missing Types in notes for pattern [%2]" )
						  .arg( sMapFile ).arg( pPattern->getName() ) );
			}
		}
		else if ( ! bSilent ) {
			INFOLOG( QString( "There are missing Types for notes in pattern [%1] and no corresponding .h2map file for registered drumkit [%2]." )
					 .arg( pPattern->getName() )
					 .arg( pPattern->getDrumkitName() ) );
		}
	}

	return pPattern;
}

bool Pattern::save( const QString& sPatternPath, bool bSilent ) const
{
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return false;
	}

	if ( ! bSilent ) {
		INFOLOG( QString( "Saving pattern into [%1]" ).arg( sPatternPath ) );
	}

	XMLDoc doc;
	XMLNode root = doc.set_root( "drumkit_pattern", "drumkit_pattern" );

	// For backward compatibility we will also add the name of the current
	// drumkit.
	root.write_string( "drumkit_name", pSong->getDrumkit()->getExportName() );
	saveTo( root, EMPTY_INSTR_ID, "", bSilent );
	return doc.write( sPatternPath );
}

void Pattern::saveTo( XMLNode& node, int nInstrumentId, const QString& sType,
					  bool bSilent ) const
{
	XMLNode pattern_node =  node.createNode( "pattern" );
	pattern_node.write_int( "formatVersion", nCurrentFormatVersion );
	pattern_node.write_int( "userVersion", m_nVersion );
	pattern_node.write_string( "name", m_sName );
	pattern_node.write_string( "author", m_sAuthor );
	pattern_node.write_string( "info", m_sInfo );
	pattern_node.write_string( "license", m_license.getLicenseString() );
	pattern_node.write_string( "category", m_sCategory );
	pattern_node.write_int( "size", m_nLength );
	pattern_node.write_int( "denominator", m_nDenominator );
	
	XMLNode note_list_node =  pattern_node.createNode( "noteList" );
	for ( auto it = m_notes.cbegin(); it != m_notes.cend(); ++it ) {
		auto pNote = it->second;
		if ( pNote != nullptr &&
			 // Optionally filter note
			 ( ( nInstrumentId == EMPTY_INSTR_ID ||
				 nInstrumentId == pNote->getInstrumentId() ) &&
			   ( sType.isEmpty() || sType == pNote->getType() ) ) &&
			 // Check whether the note corresponds to an instrument of the
			 // current kit at all.
			 ( pNote->getInstrumentId() != EMPTY_INSTR_ID ||
			   ! pNote->getType().isEmpty() ) ) {
			XMLNode note_node = note_list_node.createNode( "note" );
			pNote->saveTo( note_node );
		}
	}
}

std::shared_ptr<Note> Pattern::findNote( int nPosition, int nInstrumentId,
										 const QString& sInstrumentType,
										 Note::Key key, Note::Octave octave ) const
{
	for ( notes_cst_it_t it = m_notes.lower_bound( nPosition );
		  it != m_notes.upper_bound( nPosition ); it++ ) {
		auto ppNote = it->second;
		assert( ppNote );
		if ( ppNote != nullptr &&
			 ppNote->match( nInstrumentId, sInstrumentType, key, octave ) ) {
			return ppNote;
		}
	}

	return nullptr;
}

std::vector< std::shared_ptr<Note>> Pattern::findNotes(
	int nPosition, int nInstrumentId, const QString& sInstrumentType ) const
{
	std::vector< std::shared_ptr<Note> > notes;
	notes_cst_it_t it;
	for ( it = m_notes.lower_bound( nPosition );
		  it != m_notes.upper_bound( nPosition ); it++ ) {
		auto ppNote = it->second;
		if ( ppNote == nullptr ) {
			assert( ppNote );
			ERRORLOG( "Invalid note" );
			continue;
		}
		if ( ppNote->getInstrumentId() == nInstrumentId &&
			 ppNote->getType() == sInstrumentType ) {
			notes.push_back( ppNote );
		}
	}

	return notes;
}

void Pattern::removeNote( std::shared_ptr<Note> pNote )
{
	int nPos = pNote->getPosition();
	for ( notes_it_t it = m_notes.lower_bound( nPos );
		  it != m_notes.end() && it->first == nPos; ++it ) {
		if ( it->second == pNote ) {
			m_notes.erase( it );
			break;
		}
	}
}

bool Pattern::references( std::shared_ptr<Instrument> pInstrument ) const
{
	if ( pInstrument == nullptr ) {
		return false;
	}

	for ( const auto& [ _, ppNote ] : m_notes ) {
		if ( ppNote != nullptr && ppNote->getInstrument() == pInstrument ) {
			return true;
		}
	}
	return false;
}

void Pattern::purgeInstrument( std::shared_ptr<Instrument> pInstrument,
								bool bRequiresLock )
{
	if ( pInstrument == nullptr ) {
		return;
	}

	bool bLocked = false;
	for ( notes_it_t it = m_notes.begin(); it != m_notes.end(); ) {
		auto pNote = it->second;
		assert( pNote );
		if ( pNote != nullptr && pNote->getInstrument() == pInstrument ) {
			if ( ! bLocked && bRequiresLock ) {
				Hydrogen::get_instance()->getAudioEngine()->lock( RIGHT_HERE );
				bLocked = true;
			}
			m_notes.erase( it++ );
		} else {
			++it;
		}
	}
	if ( bLocked ) {
		Hydrogen::get_instance()->getAudioEngine()->unlock();
	}
}

void Pattern::clear( bool bRequiresLock )
{
	auto pAudioEngine = Hydrogen::get_instance()->getAudioEngine();
	if ( bRequiresLock ){
		pAudioEngine->lock( RIGHT_HERE );
	}

	m_notes.clear();

	if ( bRequiresLock ) {
		pAudioEngine->unlock();
	}
}

void Pattern::flattenedVirtualPatternsCompute()
{
	// m_flattenedVirtualPatterns must have been cleared before
	if ( m_flattenedVirtualPatterns.size() >= m_virtualPatterns.size() ) {
		return;
	}

	// for each virtual pattern
	for ( virtual_patterns_cst_it_t it0 = m_virtualPatterns.begin();
		  it0 != m_virtualPatterns.end(); ++it0 ) {
		// add it
		m_flattenedVirtualPatterns.insert( *it0 );
		// build it's flattened virtual patterns set
		( *it0 )->flattenedVirtualPatternsCompute();
		// for each pattern of it's flattened virtual pattern set
		for ( virtual_patterns_cst_it_t it1 =
				  ( *it0 )->getFlattenedVirtualPatterns()->begin();
			  it1 != ( *it0 )->getFlattenedVirtualPatterns()->end(); ++it1 ) {
			// add the pattern
			m_flattenedVirtualPatterns.insert( *it1 );
		}
	}
}

void Pattern::addFlattenedVirtualPatterns( PatternList* pPatternList ) {
	for ( virtual_patterns_cst_it_t it = m_flattenedVirtualPatterns.begin();
		 it != m_flattenedVirtualPatterns.end(); ++it ) {
		pPatternList->add( *it, true );
	}
}

void Pattern::removeFlattenedVirtualPatterns( PatternList* pPatternList ) {
	for ( virtual_patterns_cst_it_t it = m_flattenedVirtualPatterns.begin();
		 it != m_flattenedVirtualPatterns.end(); ++it ) {
		pPatternList->del( *it );
	}
}

int Pattern::longestVirtualPatternLength() const {
	int nMax = m_nLength;
	for ( virtual_patterns_cst_it_t it = m_flattenedVirtualPatterns.begin();
		 it != m_flattenedVirtualPatterns.end(); ++it ) {
		if ( (*it)->m_nLength > nMax ) {
			nMax = (*it)->m_nLength;
		}
	}

	return nMax;
}

bool Pattern::isVirtual() const {
	return m_flattenedVirtualPatterns.size() > 0;
}

void Pattern::mapTo( std::shared_ptr<Drumkit> pDrumkit,
					 std::shared_ptr<Drumkit> pOldDrumkit ) {
	if ( pDrumkit == nullptr ) {
		ERRORLOG( "Invalid drumkit" );
		return;
	}

	for ( auto& [ _, ppNote ] : m_notes ) {
		if ( ppNote != nullptr ) {
			ppNote->mapTo( pDrumkit, pOldDrumkit );
		}
	}
}

std::set<DrumkitMap::Type> Pattern::getAllTypes() const {
	std::set<DrumkitMap::Type> types;

	for ( const auto& [ _, ppNote ] : m_notes ) {
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

std::vector<std::shared_ptr<Note>> Pattern::getAllNotesOfType(
	const DrumkitMap::Type& sType ) const
{
	std::vector<std::shared_ptr<Note>> notes;

	for ( const auto& [ _, ppNote ] : m_notes ) {
		if ( ppNote != nullptr && ppNote->getType() == sType ) {
			notes.push_back( ppNote );
		}
	}

	return notes;
}

std::set<std::shared_ptr<Pattern>>::iterator Pattern::begin() {
	return m_flattenedVirtualPatterns.begin();
}

std::set<std::shared_ptr<Pattern>>::iterator Pattern::end() {
	return m_flattenedVirtualPatterns.end();
}

QString Pattern::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Pattern]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_sName: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sName ) )
			.append( QString( "%1%2m_nVersion: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nVersion ) )
			.append( QString( "%1%2m_sDrumkitName: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_sDrumkitName ) )
			.append( QString( "%1%2m_sAuthor: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sAuthor ) )
			.append( QString( "%1%2m_license: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_license.toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2m_nLength: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nLength ) )
			.append( QString( "%1%2m_nDenominator: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nDenominator ) )
			.append( QString( "%1%2m_sCategory: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sCategory ) )
			.append( QString( "%1%2m_sInfo: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sInfo ) )
			.append( QString( "%1%2m_notes:\n" ).arg( sPrefix ).arg( s ) );
				 
		for ( const auto& [ _, ppNote ] : m_notes ) {
			if ( ppNote != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ppNote->toQString(
					sPrefix + s + s, bShort ) ) );
			}
		}

		sOutput.append( QString( "%1%2m_virtualPatterns:\n" ).arg( sPrefix ).arg( s ) );
		for ( auto ii : m_virtualPatterns ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s + s, bShort ) ) );
			}
		}

		sOutput.append( QString( "%1%2m_flattenedVirtualPatterns:\n" ).arg( sPrefix ).arg( s ) );
		for ( auto ii : m_flattenedVirtualPatterns ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s + s, bShort ) ) );
			}
		}
	}
	else {

		sOutput = QString( "[Pattern]" )
			.append( QString( " m_sName: %1" ).arg( m_sName ) )
			.append( QString( ", m_nVersion: %1" ).arg( m_nVersion ) )
			.append( QString( ", m_sDrumkitName: %1" ).arg( m_sDrumkitName ) )
			.append( QString( ", m_sAuthor: %1" ).arg( m_sAuthor ) )
			.append( QString( ", m_license: %1" )
					 .arg( m_license.toQString( sPrefix, bShort ) ) )
			.append( QString( ", m_nLength: %1" ).arg( m_nLength ) )
			.append( QString( ", m_nDenominator: %1" ).arg( m_nDenominator ) )
			.append( QString( ", m_sCategory: %1" ).arg( m_sCategory ) )
			.append( QString( ", m_sInfo: %1" ).arg( m_sInfo ) )
			.append( QString( ", m_notes: [" ) );
		for ( const auto& [ _, ppNote ] : m_notes ) {
			if ( ppNote != nullptr ) {
				sOutput.append( QString( "[%1], " ).arg( ppNote->prettyName() ) );
			}
		}
		sOutput.append( "]" );
		if ( m_virtualPatterns.size() != 0 ) {
			sOutput.append( ", m_virtualPatterns: {" );
		}
		for ( auto ii : m_virtualPatterns ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s + s, bShort ) ) );
			}
		}
		if ( m_flattenedVirtualPatterns.size() != 0 ) {
			sOutput.append( "}, m_flattenedVirtualPatterns: {" );
		}
		for ( auto ii : m_flattenedVirtualPatterns ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s + s, bShort ) ) );
			}
		}
		if ( m_flattenedVirtualPatterns.size() != 0 ) {
			sOutput.append( "}" );
		}
	}	
	return sOutput;
}

};

/* vim: set softtabstop=4 noexpandtab: */
