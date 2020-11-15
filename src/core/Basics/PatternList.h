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

#ifndef H2C_PATTERN_LIST_H
#define H2C_PATTERN_LIST_H

#include <vector>

#include <core/Object.h>

namespace H2Core
{

class Pattern;

/**
 * PatternList is a collection of patterns
*/
class PatternList : public H2Core::Object
{
		H2_OBJECT
	public:
		/** constructor */
		PatternList();
		/** destructor */
		~PatternList();
		/**
		 * copy constructor
		 * \param other
		 */
		PatternList( PatternList* other );

		/** returns the numbers of patterns */
		int size() const;

		/**
		 * add a pattern to the list
		 * \param new_pattern a pointer to the pattern to add
		 */
		void operator<<( Pattern* new_pattern );
		/**
		 * get a pattern from  the list
		 * \param idx the index to get the pattern from
		 */
		Pattern* operator[]( int idx );
		/**
		 * add a pattern to the list
		 * \param pattern a pointer to the pattern to add
		 */
		void add( Pattern* pattern );
		/**
		 * insert a pattern into the list
		 * \param idx the index to insert the pattern at
		 * \param pattern a pointer to the pattern to add
		 */
		void insert( int idx, Pattern* pattern );
		/**
		 * get a pattern from  the list
		 * \param idx the index to get the pattern from
		 */
		Pattern* get( int idx );
		const Pattern* get( int idx ) const;
		/**
		 * remove the pattern at a given index, does not delete it
		 * \param idx the index
		 * \return a pointer to the removed pattern
		 */
		Pattern* del( int idx );
		/**
		 * remove a pattern from the list, does not delete it
		 * \param pattern the pattern to be removed
		 * \return a pointer to the removed pattern, 0 if not found
		 */
		Pattern* del( Pattern* pattern );
		/**
		 * get the index of the pattern within the patterns
		 * \param pattern a pointer to the pattern to find
		 * \return -1 if not found
		 */
		int index( Pattern* pattern );
		/**
		 * replace the pattern at a given index with a new one
		 * \param idx the index
		 * \param pattern the new pattern to insert
		 * \return a pointer to the removed pattern, 0 if index out of bounds
		 */
		Pattern* replace( int idx, Pattern* pattern );
		/**
		 * empty the pattern list
		 */
		void clear();
		/**
		 * mark all patterns as old
		 */
		void set_to_old();
		/**
		 * find a pattern within the patterns
		 * \param name the name of the pattern to find
		 * \return 0 if not found
		 */
		Pattern* find( const QString& name );
		/**
		 * swap the patterns of two different indexes
		 * \param idx_a the first index
		 * \param idx_b the second index
		 */
		void swap( int idx_a, int idx_b );
		/**
		 * move a pattern from a position to another
		 * \param idx_a the start index
		 * \param idx_b the finish index
		 */
		void move( int idx_a, int idx_b );
		/**
		 * call compute_flattened_virtual_patterns on each pattern
		 */
		void flattened_virtual_patterns_compute();
		/**
		 * call del_virtual_pattern on each pattern
		 * \param pattern the pattern to remove where it's found
		 */
		void virtual_pattern_del( Pattern* pattern );
		/**
		 * check if a pattern with name patternName already exists in this list
		 * \param patternName name of a pattern to check
		 */
		bool check_name( QString patternName );
		/**
		 * find unused patternName
		 * \param sourceName base name to start with
		 */
		QString find_unused_pattern_name( QString sourceName );

	private:
		std::vector<Pattern*> __patterns;            ///< the list of patterns
};

// DEFINITIONS

inline int PatternList::size() const
{
	return __patterns.size();
}

inline void PatternList::clear()
{
	__patterns.clear();
}

};

#endif // H2C_PATTERN_LIST_H

/* vim: set softtabstop=4 noexpandtab: */
