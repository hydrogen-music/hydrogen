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
#include <hydrogen/basics/pattern.h>

#include <vector>
#include <cassert>
namespace H2Core
{

const char* PatternList::__class_name = "PatternList";

PatternList::PatternList()
    : Object( __class_name )
{
//	infoLog("Init");
}



PatternList::~PatternList()
{
//	infoLog("destroy");

    // find single patterns. (skip duplicates)
    std::vector<Pattern*> temp;
    for ( unsigned int i = 0; i < list.size(); ++i ) {
        Pattern* pat = list[i];

        // pat exists in temp?
        bool exists = false;
        for ( unsigned int j = 0; j < temp.size(); ++j ) {
            if ( pat == temp[j] ) {
                exists = true;
                break;
            }
        }
        if ( !exists ) {
            temp.push_back( pat );
        }
    }

    // delete patterns
    for ( unsigned int i = 0; i < temp.size(); ++i ) {
        Pattern* pat = temp[i];
        if ( pat != NULL ) {
            delete pat;
            pat = NULL;
        }
    }
}



void PatternList::add( Pattern* newPattern )
{
    list.push_back( newPattern );
}



Pattern* PatternList::get( int nPos )
{
    if ( nPos >= ( int )list.size() ) {
        ERRORLOG( QString( "Pattern index out of bounds. nPos > list.size() - %1 > %2" )
                  .arg( nPos )
                  .arg( list.size() )
                );
        return NULL;
    }
//	assert( nPos < (int)list.size() );
    return list[ nPos ];
}



unsigned PatternList::get_size()
{
    return list.size();
}



void PatternList::clear()
{
    list.clear();
}



/// Replace an existent pattern with another one
void PatternList::replace( Pattern* newPattern, unsigned int pos )
{
    if ( pos >= ( unsigned )list.size() ) {
        ERRORLOG( QString( "Pattern index out of bounds in PatternList::replace. pos >= list.size() - %1 > %2" )
                  .arg( pos )
                  .arg( list.size() )
                );
        return;
    }
    list.insert( list.begin() + pos, newPattern );	// insert the new pattern
    // remove the old pattern
    list.erase( list.begin() + pos + 1 );
}



int PatternList::index_of( Pattern* pattern )
{
    if ( get_size() < 1 ) return -1;

    std::vector<Pattern*>::iterator i;

    int r = 0;
    for ( i = list.begin(); i != list.end(); ++i ) {
        if ( !*i  ) return -1;
        if ( *i == pattern ) return r;
        ++r;
    }
    return -1;
}



Pattern* PatternList::del( Pattern* p )
{
    bool did_delete = false;
    if ( get_size() < 1 ) return NULL;

    std::vector<Pattern*>::iterator i;

    for ( i = list.begin(); i != list.end(); i++ ) {
        if ( *i == p ) {
            i = list.erase( i );
            did_delete = true;
            break;
            // NOTE: Do we need to delete EVERY instance of p in the list? Better to avoid adding more than one copy of a pattern in the first place! Using the iterator i after modifying the list is dangerous, so either just delete one OR...
            // ... or roll back the iterator after deleting :
// 			i--;
        }
    }
    if ( did_delete ) return p;
    return NULL;
}


void PatternList::set_to_old()
{
    for ( int nPattern = 0 ; nPattern < ( int ) get_size() ; ++nPattern ) {
        get( nPattern )->set_to_old();
    }
}


void PatternList::del( unsigned pos )
{
    if ( pos >= ( unsigned )list.size() ) {
        ERRORLOG( QString( "Pattern index out of bounds in PatternList::del. pos >= list.size() - %1 > %2" )
                  .arg( pos )
                  .arg( list.size() )
                );
        return;
    }
    list.erase( list.begin()+pos );
}


};
