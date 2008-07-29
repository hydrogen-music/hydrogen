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

#ifndef PATTERN_H
#define PATTERN_H

#include <hydrogen/globals.h>
#include <hydrogen/Object.h>

namespace H2Core
{

class Note;

///
/// The Pattern is a Note container.
///
class Pattern : public Object
{
public:
	std::multimap <int, Note*> note_map;

	Pattern( const QString& name, const QString& category, unsigned lenght = MAX_NOTES );
	~Pattern();

	static Pattern* get_empty_pattern();
	Pattern* copy();

	void dump();

	unsigned get_lenght() {
		return __lenght;
	}
	void set_lenght( unsigned lenght ) {
		__lenght = lenght;
	}

	void set_name( const QString& name ) {
		__name = name;
	}
	const QString& get_name() const {
		return __name;
	}

	void set_category( const QString& category ) {
		__category = category;
	}
	const QString& get_category() const {
		return __category;
	}

private:
	unsigned __lenght;
	QString __name;
	QString __category;
};


/// Pattern List
class PatternList : public Object
{
public:
	PatternList();
	~PatternList();

	void add( Pattern* new_pattern );
	Pattern* get( int pos );
	unsigned int get_size();
	void clear();

	void replace( Pattern* new_pattern, unsigned pos );
	int index_of( Pattern* pattern );

	Pattern * del( Pattern *pattern ); // returns NULL if the pattern is not in the list :)
	void del( unsigned index );

private:
	std::vector<Pattern*> list;
};

};

#endif

