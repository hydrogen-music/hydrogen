/*
 * Hydrogen
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

#include <core/Basics/DrumkitMap.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>
#include "Basics/Instrument.h"

namespace H2Core
{

DrumkitMap::DrumkitMap() {
}

DrumkitMap::DrumkitMap( std::shared_ptr<DrumkitMap> pOther ) {
	for ( const auto& [ iid, ssType ] : pOther->m_mapping ) {
		m_mapping.insert( std::make_pair( iid, ssType ) );
	}
}

DrumkitMap::~DrumkitMap() {
}

std::shared_ptr<DrumkitMap> DrumkitMap::load( const QString& sPath, bool bSilent ) {

	if ( ! Filesystem::file_exists( sPath, true ) ) {
		ERRORLOG( QString( "Unable to find mapping file [%1]" ).arg( sPath ) );
		return std::make_shared<DrumkitMap>();
	}

	XMLDoc doc;
	doc.read( sPath, bSilent );

	XMLNode rootNode = doc.firstChildElement( "drumkit_map" );
	if ( rootNode.isNull() ) {
		ERRORLOG( QString( "Mapping file [%1] does not contain 'drumkit_map'" )
					  .arg( sPath ) );
		return std::make_shared<DrumkitMap>();
	}

	return loadFrom( rootNode, bSilent );
}

std::shared_ptr<DrumkitMap> DrumkitMap::loadFrom( const XMLNode& node, bool bSilent ) {

	std::shared_ptr<DrumkitMap> pDrumkitMap = std::make_shared<DrumkitMap>();

	XMLNode mappingNode = node.firstChildElement( "mapping" );

	while ( !mappingNode.isNull() ) {
		const auto instrumentID =
			static_cast<Instrument::Id>( mappingNode.read_int(
				"instrumentID", static_cast<int>( Instrument::EmptyId ), false,
				false, false
			) );
		const QString sType =
			mappingNode.read_string( "type", "", false, false, false );

		if ( !sType.isEmpty() && instrumentID != Instrument::EmptyId ) {
			// Ensure types to be unique and takes care of error logging.
			pDrumkitMap->addMapping(
				instrumentID, static_cast<Instrument::Type>( sType )
			);
		}

		// Move on to the next entry until there is none left.
		mappingNode = mappingNode.nextSiblingElement( "mapping" );
	}

	return pDrumkitMap;
}

XMLDoc DrumkitMap::toXml( bool bSilent ) const {
	XMLDoc doc;
	XMLNode root = doc.set_root( "drumkit_map", "drumkit_map" );
	saveTo( root, bSilent );

	return std::move( doc );
}

bool DrumkitMap::save( const QString& sPath, bool bSilent ) const {

	if ( ! Filesystem::dir_readable( QFileInfo( sPath ).dir().absolutePath(), false ) ) {
		ERRORLOG( QString( "Unable to write .h2map to [%1]. Dir not writable." )
					  .arg( sPath ) );
		return false;
	}

	INFOLOG( QString( "Saving drumkit mappings in [%1]" ).arg( sPath ) );

	XMLDoc doc;
	XMLNode root = doc.set_root( "drumkit_map", "drumkit_map" );
	
	saveTo( root, bSilent );
	return doc.write( sPath );
}

void DrumkitMap::saveTo( XMLNode& node, bool bSilent ) const {

	node.write_int( "formatVersion", nCurrentFormatVersion );

	for ( const auto& [iid, ssType] : m_mapping ) {
		XMLNode mappingNode = node.createNode( "mapping" );
		mappingNode.write_int( "instrumentID", static_cast<int>(iid) );
		mappingNode.write_string( "type", static_cast<QString>( ssType ) );
	}
}

bool DrumkitMap::addMapping( Instrument::Id id, const Instrument::Type& sType ) {
	// Since .h2map files are intended to be read-only and shipped by us, we
	// just fill them carefully and do not support modification.
	if ( m_mapping.find( id ) != m_mapping.end() ) {
		ERRORLOG( QString( "Unable to assign type [%1]. There is already one "
						   "present for instrument [%2]: %3" )
					  .arg( sType )
					  .arg( static_cast<int>( id ) )
					  .arg( m_mapping[id] ) );
		return false;
	}

	// Ensure uniqueness of type
	std::set<Instrument::Type> types;
	for ( const auto& [ _, ssType ] : m_mapping ) {
		types.insert( ssType );
	}
	const auto [ _, bUnique ] = types.insert( sType );
	if ( ! bUnique ) {
		ERRORLOG( QString( "Type [%1] is already present and those have to be unique!" )
				  .arg( sType ) );
		return false;
	}

	const auto [ __, bSuccess] = m_mapping.insert( { id, sType } );
	return bSuccess;
}

Instrument::Id DrumkitMap::getId( const Instrument::Type& sType, bool* pOk ) const {
	for ( const auto& [ iid, ssType ] : m_mapping ) {
		if ( ssType == sType ) {
			*pOk = true;
			return iid;
		}
	}

	*pOk = false;
	return Instrument::EmptyId;
}

Instrument::Type DrumkitMap::getType( Instrument::Id id ) const
{
	const auto it = m_mapping.find( id );
	if ( it == m_mapping.end() ) {
		WARNINGLOG(
			QString( "No type found for id [%1]" ).arg( static_cast<int>( id ) )
		);
		return "";
	}

	return it->second;
}

std::set<Instrument::Type> DrumkitMap::getAllTypes() const {
	std::set<Instrument::Type> results;

	for ( const auto& [ _, ssType ] : m_mapping ) {
		// Since we insert in a 'set', this will ensure the results are unique
		// and sorted.
		results.insert( ssType );
	}

	return std::move( results );
}

QString DrumkitMap::toQString( const QString& sPrefix, bool bShort ) const
{
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( !bShort ) {
		sOutput = QString( "%1[DrumkitMap]\n" ).arg( sPrefix );
		for ( const auto& [iid, ssType] : m_mapping ) {
			sOutput.append( QString( "%1%2instrument ID: %3 - type: %4\n" )
								.arg( sPrefix )
								.arg( s )
								.arg( static_cast<int>( iid ) )
								.arg( ssType ) );
		}
	}
	else {
		sOutput = QString( "[DrumkitMap]" );
		for ( const auto& [iid, ssType] : m_mapping ) {
			sOutput.append( QString( " [ instrument ID: %1 - type: %2 ]," )
								.arg( static_cast<int>( iid ) )
								.arg( ssType ) );
		}
		sOutput.append( "]\n" );
	}

	return sOutput;
}

};	// namespace H2Core

/* vim: set softtabstop=4 noexpandtab: */
