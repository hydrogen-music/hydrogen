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

#include <core/Basics/PatternList.h>

//#include <core/Helpers/Xml.h>
#include <core/Basics/Pattern.h>

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
		assert ( __patterns[i] );
		delete __patterns[i];
	}
}

void PatternList::add( Pattern* pattern )
{
	// do nothing if already in __patterns
	if ( index( pattern) != -1) {
		return;
	}
	__patterns.push_back( pattern );
}

void PatternList::insert( int idx, Pattern* pattern )
{
	// do nothing if already in __patterns
	if ( index( pattern) != -1) {
		return;
	}
	__patterns.insert( __patterns.begin() + idx, pattern );
}

Pattern* PatternList::get( int idx )
{
	if ( idx < 0 || idx >= __patterns.size() ) {
		ERRORLOG( QString( "idx %1 out of [0;%2]" ).arg( idx ).arg( size() ) );
		return nullptr;
	}
	assert( idx >= 0 && idx < __patterns.size() );
	return __patterns[idx];
}

const Pattern* PatternList::get( int idx ) const
{
	if ( idx < 0 || idx >= __patterns.size() ) {
		ERRORLOG( QString( "idx %1 out of [0;%2]" ).arg( idx ).arg( size() ) );
		return nullptr;
	}
	assert( idx >= 0 && idx < __patterns.size() );
	return __patterns[idx];
}

int PatternList::index( const Pattern* pattern )
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
	return nullptr;
}

Pattern* PatternList::replace( int idx, Pattern* pattern )
{
	/*
	 * if we insert a new pattern (copy, add new pattern, undo delete pattern and so on will do this)
	 * idx is > __pattern.size(). that's why i add +1 to assert expression
	 */

	assert( idx >= 0 && idx <= __patterns.size() +1 );
	if( idx < 0 || idx >= __patterns.size() ) {
		ERRORLOG( QString( "index out of bounds %1 (size:%2)" ).arg( idx ).arg( __patterns.size() ) );
		return nullptr;
	}

	__patterns.insert( __patterns.begin() + idx, pattern );
	__patterns.erase( __patterns.begin() + idx + 1 );

	//create return pattern after patternlist tätatä to return the right one
	Pattern* ret = __patterns[idx];
	return ret;
}

void PatternList::set_to_old()
{
	for( int i=0; i<__patterns.size(); i++ ) {
		__patterns[i]->set_to_old();
	}
}

Pattern*  PatternList::find( const QString& name )
{
	for( int i=0; i<__patterns.size(); i++ ) {
		if ( __patterns[i]->get_name()==name ) return __patterns[i];
	}
	return nullptr;
}

void PatternList::swap( int idx_a, int idx_b )
{
	assert( idx_a >= 0 && idx_a < __patterns.size() );
	assert( idx_b >= 0 && idx_b < __patterns.size() );
	if( idx_a == idx_b ) return;
	//DEBUGLOG(QString("===>> SWAP  %1 %2").arg(idx_a).arg(idx_b) );
	Pattern* tmp = __patterns[idx_a];
	__patterns[idx_a] = __patterns[idx_b];
	__patterns[idx_b] = tmp;
}

void PatternList::move( int idx_a, int idx_b )
{
	assert( idx_a >= 0 && idx_a < __patterns.size() );
	assert( idx_b >= 0 && idx_b < __patterns.size() );
	if( idx_a == idx_b ) return;
	//DEBUGLOG(QString("===>> MOVE  %1 %2").arg(idx_a).arg(idx_b) );
	Pattern* tmp = __patterns[idx_a];
	__patterns.erase( __patterns.begin() + idx_a );
	__patterns.insert( __patterns.begin() + idx_b, tmp );
}

void PatternList::flattened_virtual_patterns_compute()
{
	for ( int i=0 ; i<__patterns.size() ; i++ ) __patterns[i]->flattened_virtual_patterns_clear();
	for ( int i=0 ; i<__patterns.size() ; i++ ) __patterns[i]->flattened_virtual_patterns_compute();
}

void PatternList::virtual_pattern_del( Pattern* pattern )
{
	for( int i=0; i<__patterns.size(); i++ ) __patterns[i]->virtual_patterns_del( pattern );
}

bool PatternList::check_name( QString patternName )
{
	if (patternName == "") {
		return false;
	}

	for (uint i = 0; i < __patterns.size(); i++) {
		if ( __patterns[i]->get_name() == patternName) {
			return false;
		}
	}
	return true;
}

QString PatternList::find_unused_pattern_name( QString sourceName )
{
	QString unusedPatternNameCandidate;

	if( sourceName.isEmpty() ) {
		sourceName = "Pattern 11";
	}

	int i = 1;
	QString suffix = "";
	unusedPatternNameCandidate = sourceName;

	while( !check_name( unusedPatternNameCandidate + suffix ) ) {
		suffix = " #" + QString::number(i);
		i++;
	}

	unusedPatternNameCandidate += suffix;

	return unusedPatternNameCandidate;
}

}



/* vim: set softtabstop=4 noexpandtab: */
