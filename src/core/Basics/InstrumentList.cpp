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

#include "InstrumentList.h"

#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Note.h>
#include <core/Basics/Sample.h>
#include <core/Helpers/Xml.h>
#include <core/License.h>
#include <core/Midi/MidiMessage.h>

#include <set>

namespace H2Core
{

InstrumentList::InstrumentList()
{
}

InstrumentList::InstrumentList( std::shared_ptr<InstrumentList> other ) : Object( *other )
{
	assert( other );
	assert( m_pInstruments.size() == 0 );
	for ( int i=0; i<other->size(); i++ ) {
		add( std::make_shared<Instrument>( ( *other )[i] ) );
	}
}

InstrumentList::~InstrumentList()
{
}

void InstrumentList::loadSamples( float fBpm )
{
	for( int i=0; i<m_pInstruments.size(); i++ ) {
		m_pInstruments[i]->loadSamples( fBpm );
	}
}

void InstrumentList::unloadSamples()
{
	for( int i=0; i<m_pInstruments.size(); i++ ) {
		m_pInstruments[i]->unloadSamples();
	}
}

std::shared_ptr<InstrumentList> InstrumentList::loadFrom(
	const XMLNode& node,
	const QString& sDrumkitPath,
	const QString& sDrumkitName,
	const QString& sSongPath,
	const License& license,
	bool bSongKit,
	bool* pLegacyFormatEncountered,
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
		auto pInstrument = Instrument::loadFrom(
			instrumentNode, sDrumkitPath, sDrumkitName, sSongPath,
			license, bSongKit, pLegacyFormatEncountered, bSilent );
		if ( pInstrument != nullptr ) {
			pInstrumentList->add( pInstrument );
		}
		else {
			ERRORLOG( QString( "Unable to load instrument [%1]. The drumkit is corrupted. Skipping instrument" )
					  .arg( nCount ) );
		}
		instrumentNode = instrumentNode.nextSiblingElement( "instrument" );
	}

	if ( pInstrumentList->size() == 0 ) {
		ERRORLOG( "Newly created instrument list does not contain any instruments. Aborting." );
		return nullptr;
	}
	
	return pInstrumentList;
}

void InstrumentList::saveTo( XMLNode& node, bool bSongKit,
							bool bKeepMissingSamples, bool bSilent ) const
{
	XMLNode instruments_node = node.createNode( "instrumentList" );
	for ( const auto& pInstrument : m_pInstruments ) {
		if ( pInstrument != nullptr && pInstrument->getAdsr() != nullptr ) {
			pInstrument->saveTo( instruments_node, bSongKit,
								bKeepMissingSamples, bSilent );
		}
		else {
			ERRORLOG( "Invalid instrument!" );
		}
	}
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
	// do nothing if already in m_pInstruments
	for( int i=0; i<m_pInstruments.size(); i++ ) {
		if( m_pInstruments[i]==instrument ) return;
	}
	m_pInstruments.push_back( instrument );
}

void InstrumentList::insert( int idx, std::shared_ptr<Instrument> instrument )
{
	// do nothing if already in m_pInstruments
	for( int i=0; i<m_pInstruments.size(); i++ ) {
		if( m_pInstruments[i]==instrument ) return;
	}
	m_pInstruments.insert( m_pInstruments.begin() + idx, instrument );
}

std::shared_ptr<Instrument> InstrumentList::operator[]( int idx ) const
{
	if ( idx < 0 || idx >= m_pInstruments.size() ) {
		ERRORLOG( QString( "idx %1 out of [0;%2]" ).arg( idx ).arg( size() ) );
		return nullptr;
	}
	assert( idx >= 0 && idx < m_pInstruments.size() );
	return m_pInstruments[idx];
}

bool InstrumentList::isValidIndex( int idx ) const
{
	bool is_valid_index = true;
	
	if ( idx < 0 || idx >= m_pInstruments.size() ) {
		is_valid_index = false;
	}
	
	return is_valid_index;
}

std::shared_ptr<Instrument> InstrumentList::get( int idx ) const
{
	if ( ! isValidIndex( idx ) ) {
		ERRORLOG( QString( "idx %1 out of [0;%2]" ).arg( idx ).arg( size() ) );
		return nullptr;
	}
	assert( idx >= 0 && idx < m_pInstruments.size() );
	return m_pInstruments.at( idx );
}

int InstrumentList::index( std::shared_ptr<Instrument> instr ) const
{
	for( int i=0; i<m_pInstruments.size(); i++ ) {
		if ( m_pInstruments[i]==instr ) {
			return i;
		}
	}
	return -1;
}

std::shared_ptr<Instrument>  InstrumentList::find( const int id ) const
{
	for( int i=0; i<m_pInstruments.size(); i++ ) {
		if ( m_pInstruments[i]->getId()==id ) {
			return m_pInstruments[i];
		}
	}
	return nullptr;
}

std::shared_ptr<Instrument>  InstrumentList::find( const QString& name ) const
{
	for( int i=0; i<m_pInstruments.size(); i++ ) {
		if ( m_pInstruments[i]->getName()==name ) {
			return m_pInstruments[i];
		}
	}
	return nullptr;
}

std::vector< std::shared_ptr<Instrument> > InstrumentList::findByMidiNote(
	const int nNote ) const
{
	std::vector< std::shared_ptr<Instrument> > instrumentsFound;

	for ( const auto& ppInstrument : m_pInstruments ) {
		if ( ppInstrument != nullptr && ppInstrument->getMidiOutNote() == nNote ) {
			instrumentsFound.push_back( ppInstrument );
		}
	}

	return std::move( instrumentsFound );
}

std::shared_ptr<Instrument> InstrumentList::del( std::shared_ptr<Instrument> instrument )
{
	for( int i=0; i<m_pInstruments.size(); i++ ) {
		if( m_pInstruments[i]==instrument ) {
			m_pInstruments.erase( m_pInstruments.begin() + i );
			return instrument;
		}
	}
	return nullptr;
}

void InstrumentList::move( int idx_a, int idx_b )
{
	assert( idx_a >= 0 && idx_a < m_pInstruments.size() );
	assert( idx_b >= 0 && idx_b < m_pInstruments.size() );
	if( idx_a == idx_b ) return;
	//DEBUGLOG(QString("===>> MOVE  %1 %2").arg(idx_a).arg(idx_b) );
	auto tmp = m_pInstruments[idx_a];
	m_pInstruments.erase( m_pInstruments.begin() + idx_a );
	m_pInstruments.insert( m_pInstruments.begin() + idx_b, tmp );
}

std::vector<std::shared_ptr<InstrumentList::Content>> InstrumentList::summarizeContent() const {
	std::vector<std::shared_ptr<InstrumentList::Content>> results;

	for ( const auto& ppInstrument : m_pInstruments ) {
		if ( ppInstrument != nullptr ) {
			for ( const auto& ppInstrumentComponent : *ppInstrument->getComponents() ) {
				if ( ppInstrumentComponent != nullptr ) {
					for ( const auto& ppInstrumentLayer : *ppInstrumentComponent ) {
						if ( ppInstrumentLayer != nullptr ) {
							auto pSample = ppInstrumentLayer->getSample();
							if ( pSample != nullptr ) {
								results.push_back( std::make_shared<Content>(
									ppInstrument->getName(), // m_sInstrumentName
									ppInstrumentComponent->getName(),
									pSample->getFileName(), // m_sSampleName
									pSample->getFilePath(), // m_sFullSamplePath
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

bool InstrumentList::hasAllMidiNotesSame() const
{
	if (m_pInstruments.size() < 2) {
		return false;
	}

	std::set<int> notes;
	for( int i=0; i<m_pInstruments.size(); i++ ) {
		auto instr = m_pInstruments[i];
		notes.insert( instr->getMidiOutNote() );
	}
	return notes.size() == 1;
}

void InstrumentList::setDefaultMidiOutNotes()
{
	for( int i=0; i<m_pInstruments.size(); i++ ) {
		m_pInstruments[i]->setMidiOutNote( i + MidiMessage::nInstrumentOffset );
	}
}

QString InstrumentList::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[InstrumentList]\n" ).arg( sPrefix );
		for ( const auto& ii : m_pInstruments ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "%1" ).arg( ii->toQString( sPrefix + s, bShort ) ) );
			}
		}
	} else {
		sOutput = QString( "[InstrumentList] " );
		for ( const auto& ii : m_pInstruments ) {
			if ( ii != nullptr ) {
				sOutput.append( QString( "(%1: %2 [%3]) " ).arg( ii->getId() )
								.arg( ii->getName() ).arg( ii->getType() ) );
			}
		}
	}
	
	return sOutput;
}


std::vector<std::shared_ptr<Instrument>>::iterator InstrumentList::begin() {
	return m_pInstruments.begin();
}

std::vector<std::shared_ptr<Instrument>>::iterator InstrumentList::end() {
	return m_pInstruments.end();
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
	for ( const auto& pInstrument : m_pInstruments ) {
		if ( pInstrument != nullptr && pInstrument->isSoloed() ) {
			return true;
		}
	}
	return false;
}

bool InstrumentList::isAnyInstrumentSampleLoaded() const {
	for ( const auto& pInstrument : m_pInstruments ) {
		if ( pInstrument != nullptr ) {
			for ( const auto& pCompo : *pInstrument->getComponents() ) {
				if ( pCompo != nullptr ) {
					for ( const auto& pLayer : pCompo->getLayers() ) {
						if ( pLayer != nullptr &&
							 pLayer->getSample() != nullptr &&
							 pLayer->getSample()->isLoaded() ) {
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
