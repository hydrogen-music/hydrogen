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

#include <hydrogen/util.h>

#include <ctype.h>
#include <cassert>

namespace H2Core
{

    /* Convert a single hex digit to an integer.  Digit may be upper
     * or lower case.
     *
     * If not a hex digit, returns -1.
     *
     */
    static char hex_word_to_char(char c)
    {
	char rv = -1;  // Default:  Non-hex digit.
	
	/* I've seen a few implementations that utilize
	 * the ascii character value...  but since
	 * 0-9 isn't followed immediately by a-f or A-F,
	 * it makes the code hard to follow.
	 */
	c = toupper(c);
	switch(c) {
	case '0':
	    rv = 0;
	    break;
	case '1':
	    rv = 1;
	    break;
	case '2':
	    rv = 2;
	    break;
	case '3':
	    rv = 3;
	    break;
	case '4':
	    rv = 4;
	    break;
	case '5':
	    rv = 5;
	    break;
	case '6':
	    rv = 6;
	    break;
	case '7':
	    rv = 7;
	    break;
	case '8':
	    rv = 8;
	    break;
	case '9':
	    rv = 9;
	    break;
	case 'A':
	    rv = 10;
	    break;
	case 'B':
	    rv = 11;
	    break;
	case 'C':
	    rv = 12;
	    break;
	case 'D':
	    rv = 13;
	    break;
	case 'E':
	    rv = 14;
	    break;
	case 'F':
	    rv = 15;
	    break;
	}

	return rv;
    }

    int hextoi(const char* str, long len) {
	long pos = 0;
	bool leading_zero = false; // For 0x detection
	bool done;
	char c = 0;
	int rv = 0;

	if( len == -1 ) {
	    done = false;
	} else {
	    done = pos < len;
	}

	while( !done && str[pos] ) {
	    c = hex_word_to_char( str[pos] );

	    if( (c == -1) && (pos == 1) && (str[pos] == 'x') && leading_zero ) {
		// String starts with 0x.  Start over.
		assert( rv == 0 );
		++pos;
		continue;
	    }

	    if ( (c == 0) && (pos == 0) ) {
		leading_zero = true;
	    }

	    if( c == -1 ) {
		done = true;
	    } else {
		assert( c == (c & 0xF) );
		rv = (rv << 4) | c;
		assert( (rv & 0xF) == (c & 0xF) );
	    }

	    ++pos;
	    // Are we done, yet?
	    if( !done && (len != -1) ) {
		done = (pos < len);
	    }
	}
	return rv;
    }

}; // namespace H2Core
