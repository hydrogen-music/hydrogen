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

#ifndef H2C_PATTERN_H
#define H2C_PATTERN_H

#include <set>

#include <hydrogen/object.h>

namespace H2Core
{

class Note;
class Instrument;

/**
Pattern class is a Note container
*/
class Pattern : public H2Core::Object
{
        H2_OBJECT
    public:
        std::multimap <int, Note*> note_map;
        std::set<Pattern*> virtual_pattern_set;
        std::set<Pattern*> virtual_pattern_transitive_closure_set;
        /**
         * constructor
         * \param name the name of the pattern
         * \param category the name of the pattern
         * \param length the length of the pattern
         */
        Pattern( const QString& name="Pattern", const QString& category="not_categorized", int length=MAX_NOTES );
        /** copy constructor */
        Pattern( Pattern* other );
        /** destructor */
        ~Pattern();

        ///< set the name of the pattern
        void set_name( const QString& name );
        ///< get the name of the pattern
        const QString& get_name() const;
        ///< set the category of the pattern
        void set_category( const QString& category );
        ///< get the category of the pattern
        const QString& get_category() const;
        ///< set the length of the pattern
        void set_length( int length );
        ///< get the length of the pattern
        int get_length() const;

        /**
         * check if this pattern contains a note referencing the given instrument
         * \param instr the instrument
        */
        bool references( Instrument* instr );
        /**
         * delete the notes referencing the given instrument
         * The function is thread safe (it locks the audio data while deleting notes)
         * \param instr the instrument
        */
        void purge_instrument( Instrument* instr );
        /**
         * mark all notes as old
         */
        void set_to_old();

    private:
        int __length;                   ///< the length of the pattern
        QString __name;                 ///< the name of thepattern
        QString __category;             ///< the category of the pattern
};

// DEFINITIONS

inline void Pattern::set_name( const QString& name )
{
    __name = name;
}

inline const QString& Pattern::get_name() const
{
    return __name;
}

inline void Pattern::set_category( const QString& category )
{
    __category = category;
}

inline const QString& Pattern::get_category() const
{
    return __category;
}

inline void Pattern::set_length( int length )
{
    __length = length;
}

inline int Pattern::get_length() const
{
    return __length;
}

};

#endif // H2C_PATTERN_H

/* vim: set softtabstop=4 expandtab:  */
