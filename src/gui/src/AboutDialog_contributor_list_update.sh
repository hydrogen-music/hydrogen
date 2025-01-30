#!/bin/bash
## Usage: ./about_dialog_contributor_list_update.sh GIT_TAG_1 GIT_TAG_2
## e.g. ./about_dialog_contributor_list_update.sh origin/releases/1.0 HEAD


# Hydrogen
# Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
# Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
#
# http://www.hydrogen-music.org
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

## Sanity checks
if [ ! -f './AboutDialogContributorList.h' ]; then
	echo "ERROR: wrong directory"
	exit 1
fi

if [ "$1" == "" ] || [ "$2" == "" ]; then
	echo "INPUT ERROR. Please provide two git tags."
	exit 1
fi

## Check the existence of the supplied git tags.
git show $1 &> /dev/null
if [ "$?" != "0" ]; then
	echo "INPUT ERROR. [$1] is not a valid git tag of this repo"
	exit 1
fi
git show $2 &> /dev/null
if [ "$?" != "0" ]; then
	echo "INPUT ERROR. [$2] is not a valid git tag of this repo"
	exit 1
fi

cat > AboutDialogContributorList.cpp <<"EOF"
// This file was automatically created by the update_contributors.sh
// script. Please don't modify it by hand. 
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
#include "AboutDialogContributorList.h"

AboutDialogContributorList::AboutDialogContributorList() {

	std::vector<QString> v{
EOF

## Query the git log to obtain an unique list of the names of all
## contributors pushing a commit between the last ($0) and the current
## ($1) tag.
CONTRIBUTOR_LIST=$(git log $1..$2 | grep '^Author:' | uniq -c | sort -n -r | sed 's/^.*Author: /,\"/' | awk '!visited[toupper($1)]++' | awk 'BEGIN { FS = " <" }; !visited[toupper($2)]++ {print $1}')

## Alter the field separator to loop over rows instead of individual words.
SAVEIFS=$IFS
IFS=$(echo -en "\n\b")

## Use ${CONTRIBUTOR_LIST:1} to get rid of the first comma.
for cc in ${CONTRIBUTOR_LIST:1}; do
	echo "$cc\"" >> AboutDialogContributorList.cpp
done

IFS=$SAVEIFS

cat >> AboutDialogContributorList.cpp << "EOF"
};
	m_pContributorList = std::make_shared<std::vector<QString>>(v);
}

AboutDialogContributorList::~AboutDialogContributorList() {}
EOF
