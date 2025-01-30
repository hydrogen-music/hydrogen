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

#include <core/Helpers/Future.h>

#include <core/Helpers/Xml.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentList.h>

namespace H2Core {

std::shared_ptr<H2Core::Drumkit> Future::loadDrumkit( XMLNode& node,
													  const QString& sDrumkitPath,
													  bool bSilent ) {
	QString sDrumkitName = node.read_string( "name", "", false, false, bSilent );
	if ( sDrumkitName.isEmpty() ) {
		ERRORLOG( "Drumkit has no name, abort" );
		return nullptr;
	}

	std::shared_ptr<Drumkit> pDrumkit = std::make_shared<Drumkit>();

	if ( sDrumkitPath.isEmpty() ) {
		const QString sPath = node.read_string( "drumkitPath", "", false, false,
												bSilent );
		pDrumkit->set_path( sPath );
	}
	else {
		pDrumkit->set_path( sDrumkitPath );
	}
	pDrumkit->set_name( sDrumkitName );
	pDrumkit->set_author( node.read_string( "author", "undefined author",
											true, true, true ) );
	pDrumkit->set_info( node.read_string( "info", "No information available.",
										  true, true, bSilent  ) );

	License license( node.read_string( "license", "undefined license",
										true, true, bSilent  ),
					 pDrumkit->get_author() );
	pDrumkit->set_license( license );

	// As of 2022 we have no drumkits featuring an image in
	// stock. Thus, verbosity of this one will be turned of in order
	// to make to log more concise.
	pDrumkit->set_image( node.read_string( "image", "",
											true, true, true ) );
	License imageLicense( node.read_string( "imageLicense", "undefined license",
											 true, true, true  ),
						  pDrumkit->get_author() );
	pDrumkit->set_image_license( imageLicense );

	// There are no DrumkitComponents in more recent versions of the drumkit.
	// Instead, each Instrument can have several, independent
	// InstrumentComponents all holding their individual name but no component
	// ID as present in 1.2.X.
	//
	// We will load those instruments one by one, take care of mapping the
	// individual component names to DrumkitComponents - InstrumentComponents
	// holding the same name will be associated to the same DrumkitComponent -
	// and add these instrument to the drumkit one by one.
	//
	// Map uses the component name as key and the new component ID as value.
	std::map<QString, int> componentMap;
	int nNextComponentId = 0;

	// We load the instruments piece by piece and add them to the drumkit.
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

		auto pInstrument = Instrument::load_from( &instrumentNode,
												  sDrumkitPath,
												  sDrumkitName,
												  license, bSilent );
		if ( pInstrument != nullptr ) {
			auto pInstrumentComponents = pInstrument->get_components();

			// The stock loading method of Instrument does not retrieve is
			// component's name.
			XMLNode componentNode =
				instrumentNode.firstChildElement( "instrumentComponent" );
			int nnComponentIdx = 0;
			int nnComponentId;
			while ( ! componentNode.isNull() ) {
				const QString ssComponentName =
					componentNode.read_string( "name", "Main", false, false );
				if ( auto search = componentMap.find( ssComponentName );
					 search != componentMap.end() ) {
					// Component name already encountered.
					nnComponentId = componentMap[ ssComponentName ];
				}
				else {
					// The corresponding drumkit component does not exist yet.
					nnComponentId = nNextComponentId;
					auto pDrumkitComponent = std::make_shared<DrumkitComponent>(
						nnComponentId, ssComponentName );
					pDrumkit->addComponent( pDrumkitComponent );
					componentMap[ ssComponentName ] = nnComponentId;
					nNextComponentId++;
				}

				if ( nnComponentIdx < pInstrumentComponents->size() ) {
					auto pInstrumentComponent =
						pInstrumentComponents->at( nnComponentIdx );
					if ( pInstrumentComponent != nullptr ) {
						pInstrumentComponent->set_drumkit_componentID(
							nnComponentId );
					}
					else {
						ERRORLOG( QString( "Unable to access component [%1] of instrument [%2]" )
								  .arg( nnComponentIdx )
								  .arg( pInstrument->get_name() ) );
					}
				}
				else {
					ERRORLOG( QString( "Component number [%1] out of bound [0,%2] for instrument [%3]" )
							  .arg( nnComponentIdx )
							  .arg( pInstrumentComponents->size() )
							  .arg( pInstrument->get_name() ) );
				}

				++nnComponentIdx;
				componentNode = componentNode.nextSiblingElement( "instrumentComponent" );
			}

			pDrumkit->addInstrument( pInstrument );
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

	// Instead of making the *::load_from() functions more complex by
	// passing the license down to each sample, we will make the
	// drumkit assign its license to each sample in here.
	pDrumkit->propagateLicense();

	return pDrumkit;

}

std::vector<std::shared_ptr<DrumkitComponent>> Future::loadDrumkitComponentsFromKit( XMLNode* pNode ) {
	std::vector<std::shared_ptr<DrumkitComponent>> pComponents;
	XMLNode componentListNode = pNode->firstChildElement( "componentList" );
	if ( ! componentListNode.isNull() ) {
		XMLNode componentNode = componentListNode.firstChildElement( "drumkitComponent" );
		while ( ! componentNode.isNull()  ) {
			auto pDrumkitComponent = DrumkitComponent::load_from( &componentNode );
			if ( pDrumkitComponent != nullptr ) {
				pComponents.push_back(pDrumkitComponent);
			}

			componentNode = componentNode.nextSiblingElement( "drumkitComponent" );
		}
	} else {
		WARNINGLOG( "componentList node not found" );
		auto pDrumkitComponent = std::make_shared<DrumkitComponent>( 0, "Main" );
		pComponents.push_back(pDrumkitComponent);
	}

	return std::move( pComponents );
}
};
