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

#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/Note.h>
#include <core/Basics/Sample.h>

#include <core/Helpers/Xml.h>
#include <core/IO/MidiCommon.h>
#include <core/License.h>

#include <set>

namespace H2Core
{

InstrumentList::InstrumentList()
{
}

InstrumentList::InstrumentList( std::shared_ptr<InstrumentList> other ) : Object( *other )
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

void InstrumentList::load_samples( float fBpm )
{
	for( int i=0; i<__instruments.size(); i++ ) {
		__instruments[i]->load_samples( fBpm );
	}
}

void InstrumentList::unload_samples()
{
	for( int i=0; i<__instruments.size(); i++ ) {
		__instruments[i]->unload_samples();
	}
}

std::shared_ptr<InstrumentList> InstrumentList::load_from(
	const XMLNode& node,
	const QString& sDrumkitPath,
	const QString& sDrumkitName,
	const QString& sSongPath,
	const License& license,
	bool bSongKit,
	bool bSilent )
{

	XMLNode instrumentListNode = node.firstChildElement( "instrumentList" );
	if ( instrumentListNode.isNull() ) {
		ERRORLOG( "'instrumentList' node not found. Unable to load instrument list." );
		return nullptr;
	}

	auto pInstrumentList = std::make_shared<InstrumentList>();
	XMLNode instrumentNode = instrumentListNode.firstChildElement( "instrument" );
	int nCount = 0;
	while ( !instrumentNode.isNull() ) {
		nCount++;
		if ( nCount > MAX_INSTRUMENTS ) {
			ERRORLOG( QString( "instrument nCount >= %1 (MAX_INSTRUMENTS), stop reading instruments" )
					  .arg( MAX_INSTRUMENTS ) );
			break;
		}

		auto pInstrument = Instrument::load_from(
			instrumentNode, sDrumkitPath, sDrumkitName, sSongPath,
			license, bSongKit, bSilent );
		if ( pInstrument != nullptr ) {
			( *pInstrumentList ) << pInstrument;
		}
		else {
			ERRORLOG( QString( "Unable to load instrument [%1]. The drumkit is corrupted. Skipping instrument" )
					  .arg( nCount ) );
			nCount--;
		}
		instrumentNode = instrumentNode.nextSiblingElement( "instrument" );
	}

	if ( nCount == 0 ) {
		ERRORLOG( "Newly created instrument list does not contain any instruments. Aborting." );
		return nullptr;
	}
	
	return pInstrumentList;
}

void InstrumentList::save_to( XMLNode& node, bool bSongKit ) const
{
	XMLNode instruments_node = node.createNode( "instrumentList" );
	for ( const auto& pInstrument : __instruments ) {
		if ( pInstrument != nullptr && pInstrument->get_adsr() != nullptr ) {
			pInstrument->save_to( instruments_node, bSongKit );
		}
		else {
			ERRORLOG( "Invalid instrument!" );
		}
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
	
bool InstrumentList::operator==( std::shared_ptr<InstrumentList> pOther ) const {
	if ( pOther != nullptr && size() == pOther->size() ) {
		for ( int ii = 0; ii < size(); ++ii ) {
			if ( get( ii ).get() != pOther->get( ii ).get() ) {
				return false;
			}
		}

		return true;
	}

	return false;
}

bool InstrumentList::operator!=( std::shared_ptr<InstrumentList> pOther ) const {
	if ( pOther != nullptr && size() == pOther->size() ) {
		for ( int ii = 0; ii < size(); ++ii ) {
			if ( get( ii ).get() != pOther->get( ii ).get() ) {
				return true;
			}
		}

		return false;
	}

	return true;
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

std::shared_ptr<Instrument> InstrumentList::operator[]( int idx ) const
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

std::shared_ptr<Instrument> InstrumentList::get( int idx ) const
{
	if ( ! is_valid_index( idx ) ) {
		ERRORLOG( QString( "idx %1 out of [0;%2]" ).arg( idx ).arg( size() ) );
		return nullptr;
	}
	assert( idx >= 0 && idx < __instruments.size() );
	return __instruments.at( idx );
}

int InstrumentList::index( std::shared_ptr<Instrument> instr ) const
{
	for( int i=0; i<__instruments.size(); i++ ) {
		if ( __instruments[i]==instr ) {
			return i;
		}
	}
	return -1;
}

std::shared_ptr<Instrument>  InstrumentList::find( const int id ) const
{
	for( int i=0; i<__instruments.size(); i++ ) {
		if ( __instruments[i]->get_id()==id ) {
			return __instruments[i];
		}
	}
	return nullptr;
}

std::shared_ptr<Instrument>  InstrumentList::find( const QString& name ) const
{
	for( int i=0; i<__instruments.size(); i++ ) {
		if ( __instruments[i]->get_name()==name ) {
			return __instruments[i];
		}
	}
	return nullptr;
}

std::shared_ptr<Instrument>  InstrumentList::findMidiNote( const int note ) const
{
	for( int i=0; i<__instruments.size(); i++ ) {
		if ( __instruments[i]->get_midi_out_note()==note ) {
			return __instruments[i];
		}
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

std::vector<std::shared_ptr<InstrumentList::Content>> InstrumentList::summarizeContent() const {
	std::vector<std::shared_ptr<InstrumentList::Content>> results;

	for ( const auto& ppInstrument : __instruments ) {
		if ( ppInstrument != nullptr ) {
			for ( const auto& ppInstrumentComponent : *ppInstrument->get_components() ) {
				if ( ppInstrumentComponent != nullptr ) {
					for ( const auto& ppInstrumentLayer : *ppInstrumentComponent ) {
						if ( ppInstrumentLayer != nullptr ) {
							auto pSample = ppInstrumentLayer->get_sample();
							if ( pSample != nullptr ) {
								results.push_back( std::make_shared<Content>(
									ppInstrument->get_name(), // m_sInstrumentName
									ppInstrumentComponent->getName(),
									pSample->get_filename(), // m_sSampleName
									pSample->get_filepath(), // m_sFullSamplePath
									pSample->getLicense() // m_license
								    ) );
							}
						}
					}
				}
			}
		}
	}

	return results;
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
		__instruments[i]->set_midi_out_note( i + MidiMessage::instrumentOffset );
	}
}

QString InstrumentList::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[InstrumentList]\n" ).arg( sPrefix );
		for ( const auto& ii : __instruments ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s, bShort ) ) );
			}
		}
	} else {
		sOutput = QString( "[InstrumentList] " );
		for ( const auto& ii : __instruments ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "(%1: %2 [%3]) " ).arg( ii->get_id() )
								.arg( ii->get_name() ).arg( ii->getType() ) );
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

QString InstrumentList::Content::toQString( const QString& sPrefix, bool bShort ) const {
	
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "\n" ).arg( sPrefix )
			.append( QString( "%1%2m_sInstrumentName: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sInstrumentName ) )
			.append( QString( "%1%2m_sComponentName: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sComponentName ) )
			.append( QString( "%1%2m_sSampleName: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sSampleName ) )
			.append( QString( "%1%2m_sFullSamplePath: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sFullSamplePath ) )
			.append( QString( "%1%2m_license: %3\n" ).arg( m_license.toQString( sPrefix + s, bShort ) ) );
	} else {
		sOutput = QString( "m_sInstrumentName: %1\n" ).arg( m_sInstrumentName )
			.append( QString( ", m_sComponentName: %1\n" ).arg( m_sComponentName ) )
			.append( QString( ", m_sSampleName: %1\n" ).arg( m_sSampleName ) )
			.append( QString( ", m_sFullSamplePath: %1\n" ).arg( m_sFullSamplePath ) )
			.append( QString( ", m_license: %1\n" ).arg( m_license.toQString( "", bShort ) ) );
	}

	return sOutput;
}


bool InstrumentList::isAnyInstrumentSoloed() const
{
	for ( const auto& pInstrument : __instruments ) {
		if ( pInstrument != nullptr && pInstrument->is_soloed() ) {
			return true;
		}
	}
	return false;
}

bool InstrumentList::isAnyInstrumentSampleLoaded() const {
	for ( const auto& pInstrument : __instruments ) {
		if ( pInstrument != nullptr ) {
			for ( const auto& pCompo : *pInstrument->get_components() ) {
				if ( pCompo != nullptr ) {
					for ( const auto& pLayer : pCompo->getLayers() ) {
						if ( pLayer != nullptr &&
							 pLayer->get_sample() != nullptr &&
							 pLayer->get_sample()->isLoaded() ) {
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

};

/* vim: set softtabstop=4 noexpandtab: */
