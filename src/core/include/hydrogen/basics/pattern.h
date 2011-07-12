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
#include <hydrogen/basics/note.h>

namespace H2Core
{

class Instrument;

/**
Pattern class is a Note container
*/
class Pattern : public H2Core::Object
{
        H2_OBJECT
    public:
        ///< \brief multimap note type
        typedef std::multimap <int, Note*> notes_t;
        ///< \brief multimap note iterator type
        typedef notes_t::iterator notes_it_t;
        ///< \brief multimap note const iterator type
        typedef notes_t::const_iterator notes_cst_it_t;

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
        ///< \brief get the note multimap
        const notes_t* get_notes() const;

        /**
         * insert a new note within __notes
         * \param note the note to be inserted
         * \param position if not -1 will be used as std::pair first element, otherwise note position will be used
         */
        void insert_note( Note* note, int position=-1 );
        /**
         * \brief search for a note at a given index within __notes wich correspond to the given arguments
         * \param idx the index to search in within __notes
         * \param instrument the instrument the note should be playing
         * \param key the key that should be set to the note
         * \param octave the octave that should be set to the note
         * \param strict if set to false, will search for a note around the given idx
         * \return the note if found, 0 otherwise
         */
        Note* find_note( int idx, Instrument* instrument, Note::Key key, Note::Octave octave, bool strict=true);
        /**
         * \brief removes a given note from __notes, it's not deleted
         * \param note the note to be removed
         */
        void remove_note( Note* note );

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
    public:
        notes_t note_map;                ///< a multimap (hash with possible multiple values for one key) of notes
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

inline const Pattern::notes_t* Pattern::get_notes() const
{
    return &note_map;
}

inline void Pattern::insert_note( Note* note, int position )
{
    note_map.insert( std::make_pair( (position==-1 ? note->get_position() : position ), note ) );
}

};

#endif // H2C_PATTERN_H

/* vim: set softtabstop=4 expandtab:  */
