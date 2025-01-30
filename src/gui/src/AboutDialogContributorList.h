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

#ifndef ABOUT__DIALOG_CONTRIBUTOR_LIST_H
#define ABOUT__DIALOG_CONTRIBUTOR_LIST_H

#include <vector>
#include <memory>
#include <QString>
#include <core/Object.h>

/** \ingroup docGUI*/
class AboutDialogContributorList :   public H2Core::Object<AboutDialogContributorList>
{
H2_OBJECT(AboutDialogContributorList)
public:
	AboutDialogContributorList();
	~AboutDialogContributorList();

	std::shared_ptr<std::vector<QString>> getContributorList() const;

private:
	std::shared_ptr<std::vector<QString>> m_pContributorList;
};

inline std::shared_ptr<std::vector<QString>> AboutDialogContributorList::getContributorList() const {
	return m_pContributorList;
}
#endif
