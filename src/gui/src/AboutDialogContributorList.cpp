// This file was automatically created by the update_contributors.sh
// script. Please don't modify it by hand. 
/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
#include "AboutDialogContributorList.h"

AboutDialogContributorList::AboutDialogContributorList() {

	std::vector<QString> v{
"theGreatWhiteShark"
,"Colin McEwan"
,"Sebastian Moors"
,"oddtime"
,"Varik Valefor"
,"Olivier Humbert"
,"Clara Hobbs"
,"Przemysław Sitek"
,"Charbel Jacquin"
,"freddii"
,"daryl"
,"Jérémy Zurcher"
,"Guocheng"
,"David Runge"
,"Darío Hereñú"
,"Aaron"
};
	m_pContributorList = std::make_shared<std::vector<QString>>(v);
}

AboutDialogContributorList::~AboutDialogContributorList() {}
