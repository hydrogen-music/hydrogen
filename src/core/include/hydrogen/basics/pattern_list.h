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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef PATTERN_LIST_H
#define PATTERN_LIST_H

#include <hydrogen/globals.h>
#include <hydrogen/object.h>
#include <set>

namespace H2Core
{

class Pattern;

/// Pattern List
class PatternList : public H2Core::Object
{
        H2_OBJECT
    public:
        PatternList();
        ~PatternList();

        void add( Pattern* new_pattern );
        Pattern* get( int pos );
        unsigned int get_size();
        void clear();

        void replace( Pattern* new_pattern, unsigned pos );
        int index_of( Pattern* pattern );

        /// Remove a pattern from the list (every instance in the list), the pattern is not deleted!!!
        /// Returns NULL if the pattern is not in the list
        Pattern* del( Pattern* pattern );

        void set_to_old();

        /// Remove one pattern from the list, the pattern is not deleted!!!
        void del( unsigned index );

    private:
        std::vector<Pattern*> list;
};

};

#endif

