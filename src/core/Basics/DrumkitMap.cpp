/*
 * Hydrogen
 * Copyright(c) 2008-2023 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

namespace H2Core
{

DrumkitMap::DrumkitMap() {
}

DrumkitMap::DrumkitMap( std::shared_ptr<DrumkitMap> pOther ) {
	for ( const auto& [ nnId, ssType ] : pOther->m_mapping ) {
		m_mapping.insert( std::make_pair( nnId, ssType ) );
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
	if ( !doc.read( sPath, Filesystem::drumkit_map_xsd_path(), true ) ) {
		WARNINGLOG( QString( "Mapping file [%1] is not valid with respect to [%2]. Loading might fail." )
					.arg( sPath )
					.arg( Filesystem::drumkit_map_xsd_path() ) );
		auto ret = doc.read( sPath, nullptr, bSilent );
	}

	XMLNode rootNode = doc.firstChildElement( "drumkit_map" );
	if ( rootNode.isNull() ) {
		ERRORLOG( QString( "Mapping file [%1] does not contain 'drumkit_map'" )
					  .arg( sPath ) );
		return std::make_shared<DrumkitMap>();
	}

	return loadFrom( &rootNode, bSilent );
}

std::shared_ptr<DrumkitMap> DrumkitMap::loadFrom( XMLNode* pNode, bool bSilent ) {

	std::shared_ptr<DrumkitMap> pDrumkitMap = std::make_shared<DrumkitMap>();

	if ( pNode == nullptr ) {
		ERRORLOG( "Invalid XML node pointer" );
		return pDrumkitMap;
	}

	std::multimap<int, Type> map;

	XMLNode mappingNode = pNode->firstChildElement( "mapping" );

	while ( !mappingNode.isNull() ) {
		const QString sType =
			mappingNode.read_string( "type", "", false, false, false );
		const int nInstrumentID =
			mappingNode.read_int( "instrumentID", -1, false, false, false );

		DEBUGLOG( QString( "stype: %1, nInstrumetnID: %2" ).arg( sType ).arg( nInstrumentID ) );
		if ( !sType.isEmpty() && nInstrumentID != -1 ) {
			map.emplace(
				std::pair( nInstrumentID, static_cast<Type>( sType ) ) );
		}

		// Move on to the next entry until there is none left.
		mappingNode = mappingNode.nextSiblingElement( "mapping" );
	}

	pDrumkitMap->m_mapping = map;
	return pDrumkitMap;
}

bool DrumkitMap::save( const QString& sPath, bool bSilent ) {

	if ( ! Filesystem::dir_readable( QFileInfo( sPath ).dir().absolutePath(), false ) ) {
		ERRORLOG( QString( "Unable to write .h2map to [%1]. Dir not writable." )
					  .arg( sPath ) );
		return false;
	}

	// Save drumkit.xml
	XMLDoc doc;
	XMLNode root = doc.set_root( "drumkit_map", "drumkit_map" );
	
	saveTo( &root, bSilent );
	return doc.write( Filesystem::drumkit_file( sPath ) );
}

void DrumkitMap::saveTo( XMLNode* pNode, bool bSilent ) {

	for ( const auto& [nnId, ssType] : m_mapping ) {
		XMLNode mappingNode = pNode->createNode( "mapping" );
		mappingNode.write_int( "instrumentID", nnId );
		mappingNode.write_string( "type", static_cast<QString>( ssType ) );
	}
}

std::vector<DrumkitMap::Type> DrumkitMap::getTypes( int nId ) const {
	std::vector<Type> results;

	auto range = m_mapping.equal_range( nId );
	for ( auto ii = range.first; ii != range.second; ++ii ) {
		results.push_back( ii->second );
	}

	return std::move( results );
}

std::set<DrumkitMap::Type> DrumkitMap::getAllTypes() const {
	std::set<DrumkitMap::Type> results;

	for ( const auto& [ _, ssType ] : m_mapping ) {
		// Since we insert in a 'set', this will ensure the results are unique
		// and sorted.
		results.insert( ssType );
	}

	return std::move( results );
}

QString DrumkitMap::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[DrumkitMap]\n" ).arg( sPrefix );
		for ( const auto& [ nnId, ssType ] : m_mapping ) {
			sOutput.append( QString( "%1%2instrument ID: %3 - type: %4\n" )
							.arg( sPrefix ).arg( s ).arg( nnId ).arg( ssType ) );
		}
	}
	else {
		sOutput = QString( "[DrumkitMap]" );
		for ( const auto& [nnId, ssType] : m_mapping ) {
			sOutput.append( QString( " [ instrument ID: %1 - type: %2 ]," )
								.arg( nnId )
								.arg( ssType ) );
		}
		sOutput.append( "]\n" );
	}

	return sOutput;
}

};

/* vim: set softtabstop=4 noexpandtab: */
