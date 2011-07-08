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

#include <hydrogen/basics/pattern_list.h>

//#include <hydrogen/helpers/xml.h>
#include <hydrogen/basics/pattern.h>

namespace H2Core
{

const char* PatternList::__class_name = "PatternList";

PatternList::PatternList() : Object( __class_name )
{
}

PatternList::PatternList( PatternList* other ) : Object( __class_name )
{
    assert( __patterns.size() == 0 );
    for ( int i=0; i<other->size(); i++ ) {
        ( *this ) << ( new Pattern( ( *other )[i] ) );
    }
}

PatternList::~PatternList()
{
    for ( int i = 0; i < __patterns.size(); ++i ) {
        delete __patterns[i];
    }
}

void PatternList::operator<<( Pattern* pattern )
{
    // do nothing if already in __patterns
    for( int i=0; i<__patterns.size(); i++ ) {
        if( __patterns[i]==pattern ) return;
    }
    __patterns.push_back( pattern );
}

void PatternList::add( Pattern* pattern )
{
    // do nothing if already in __patterns
    for( int i=0; i<__patterns.size(); i++ ) {
        if( __patterns[i]==pattern ) return;
    }
    __patterns.push_back( pattern );
}

void PatternList::insert( int idx, Pattern* pattern )
{
    // do nothing if already in __patterns
    for( int i=0; i<__patterns.size(); i++ ) {
        if( __patterns[i]==pattern ) return;
    }
    __patterns.insert( __patterns.begin() + idx, pattern );
}

Pattern* PatternList::operator[]( int idx )
{
    if ( idx < 0 || idx >= __patterns.size() ) {
        ERRORLOG( QString( "idx %1 out of [0;%2]" ).arg( idx ).arg( size() ) );
        return 0;
    }
    assert( idx >= 0 && idx < __patterns.size() );
    return __patterns[idx];
}

Pattern* PatternList::get( int idx )
{
    if ( idx < 0 || idx >= __patterns.size() ) {
        ERRORLOG( QString( "idx %1 out of [0;%2]" ).arg( idx ).arg( size() ) );
        return 0;
    }
    assert( idx >= 0 && idx < __patterns.size() );
    return __patterns[idx];
}

int PatternList::index( Pattern* pattern )
{
    for( int i=0; i<__patterns.size(); i++ ) {
        if ( __patterns[i]==pattern ) return i;
    }
    return -1;
}

Pattern* PatternList::del( int idx )
{
    assert( idx >= 0 && idx < __patterns.size() );
    Pattern* pattern = __patterns[idx];
    __patterns.erase( __patterns.begin() + idx );
    return pattern;
}

Pattern* PatternList::del( Pattern* pattern )
{
    for( int i=0; i<__patterns.size(); i++ ) {
        if( __patterns[i]==pattern ) {
            __patterns.erase( __patterns.begin() + i );
            return pattern;
        }
    }
    return 0;
}

Pattern* PatternList::replace( int idx, Pattern* pattern )
{
    assert( idx >= 0 && idx < __patterns.size() );
    if( idx < 0 || idx >= __patterns.size() ) {
        ERRORLOG( QString( "index out of bounds %1 (size:%2)" ).arg( idx ).arg( __patterns.size() ) );
        return 0;
    }
    Pattern* ret = __patterns[idx];
    __patterns.insert( __patterns.begin() + idx, pattern );
    __patterns.erase( __patterns.begin() + idx + 1 );
    return ret;
}

void PatternList::set_to_old()
{
    for( int i=0; i<__patterns.size(); i++ ) {
        __patterns[i]->set_to_old();
    }
}

};

/* vim: set softtabstop=4 expandtab: */
