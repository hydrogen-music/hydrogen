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

#ifndef H2C_FUTURE_H
#define H2C_FUTURE_H

#include <core/Object.h>
#include <memory>
#include <vector>

namespace H2Core {

class DrumkitComponent;
class XMLNode;

/** Ensures compatibility with the song format introduced in Hydrogen v1.3.0.
 *
 * This code is meant only for the 1.2.X release branch! */
class Future : public H2Core::Object<Future> {
		H2_OBJECT(Future)
public:
	static std::vector<std::shared_ptr<DrumkitComponent>> loadDrumkitComponentsFromKit(
		XMLNode* pNode );
};

};

#endif  // H2C_FUTURE_H
