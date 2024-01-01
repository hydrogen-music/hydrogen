/*
 * Hydrogen
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

#include <core/Helpers/Future.h>

#include <core/Helpers/Xml.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/DrumkitComponent.h>

namespace H2Core {

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
