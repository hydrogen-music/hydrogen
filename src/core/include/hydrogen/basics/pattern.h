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

class XMLNode;
class Instrument;
class InstrumentList;
class PatternList;

/**
Pattern class is a Note container
*/
class Pattern : public H2Core::Object
{
        H2_OBJECT
    public:
        ///< multimap note type
        typedef std::multimap <int, Note*> notes_t;
        ///< multimap note iterator type
        typedef notes_t::iterator notes_it_t;
        ///< multimap note const iterator type
        typedef notes_t::const_iterator notes_cst_it_t;
        ///< note set type;
        typedef std::set <Pattern*> virtual_patterns_t;
        ///< note set iterator type;
        typedef virtual_patterns_t::iterator virtual_patterns_it_t;
        ///< note set const iterator type;
        typedef virtual_patterns_t::const_iterator virtual_patterns_cst_it_t;
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

        /*
         * \brief load a pattern from a file
         * \param pattern_path the path to the file to load the pattern from
         * \param instruments the current instrument list to search instrument into
         */
        static Pattern* load_file( const QString& pattern_path, InstrumentList* instruments );
        /**
         * \brief load a pattern from an XMLNode
         * \param node the XMLDode to read from
         * \param instruments the current instrument list to search instrument into
         * \return a new Pattern instance
         */
        static Pattern* load_from( XMLNode* node, InstrumentList* instruments );

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
        ///< get the note multimap
        const notes_t* get_notes() const;
        ///< get the virtual pattern set
        const virtual_patterns_t* get_virtual_patterns() const;
        ///< get the flattened virtual pattern set
        const virtual_patterns_t* get_flattened_virtual_patterns() const;

        /**
         * insert a new note within __notes
         * \param note the note to be inserted
         * \param position if not -1 will be used as std::pair first element, otherwise note position will be used
         */
        void insert_note( Note* note, int position=-1 );
        /**
         * search for a note at a given index within __notes wich correspond to the given arguments
         * \param idx_a the first __notes index to search in
         * \param idx_b the second __notes index to search in, will be omitted if is -1
         * \param instrument the instrument the note should be playing
         * \param strict if set to false, will search for a note around the given idx
         * \return the note if found, 0 otherwise
         */
        Note* find_note( int idx_a, int idx_b, Instrument* instrument, bool strict=true );
        /**
         * search for a note at a given index within __notes wich correspond to the given arguments
         * \param idx_a the first __notes index to search in
         * \param idx_b the second __notes index to search in, will be omitted if is -1
         * \param instrument the instrument the note should be playing
         * \param key the key that should be set to the note
         * \param octave the octave that should be set to the note
         * \param strict if set to false, will search for a note around the given idx
         * \return the note if found, 0 otherwise
         */
        Note* find_note( int idx_a, int idx_b, Instrument* instrument, Note::Key key, Note::Octave octave, bool strict=true );
        /**
         * removes a given note from __notes, it's not deleted
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

        ///< return true if __virtual_patterns is empty
        bool virtual_patterns_empty() const;
        ///< clear __virtual_patterns
        void virtual_patterns_clear();
        /**
         * add a pattern to __virtual_patterns
         * \param pattern the pattern to add
         */
        void virtual_patterns_add( Pattern* pattern );
        /**
         * remove a pattern from virtual_pattern set, flattened virtual patterns have to be rebuilt
         *                   */
        void virtual_patterns_del( Pattern* pattern );
        ///< clear flattened_virtual_patterns
        void flattened_virtual_patterns_clear();
        /**
         * compute virtual_pattern_transitive_closure_set based on virtual_pattern_transitive_closure_set
         * virtual_pattern_transitive_closure_set must have been cleared before which is the case is called
         * from PatternList::compute_flattened_virtual_patterns
         */
        void flattened_virtual_patterns_compute();
        /**
         * add content of __flatteened_virtual_patterns into patterns
         * \param patterns the pattern list to feed
         */
        void extand_with_flattened_virtual_patterns( PatternList* patterns );

    private:
        int __length;                                           ///< the length of the pattern
        QString __name;                                         ///< the name of thepattern
        QString __category;                                     ///< the category of the pattern
        notes_t __notes;                                        ///< a multimap (hash with possible multiple values for one key) of note
        virtual_patterns_t __virtual_patterns;                  ///< a list of patterns directly referenced by this one
        virtual_patterns_t __flattened_virtual_patterns;        ///< the complete list of virtual patterns
};

#define FOREACH_NOTE_CST_IT_BEGIN_END(_notes,_it) \
    for( Pattern::notes_cst_it_t (_it)=(_notes)->begin(); (_it)!=(_notes)->end(); (_it)++ )

#define FOREACH_NOTE_CST_IT_BOUND(_notes,_it,_bound) \
    for( Pattern::notes_cst_it_t (_it)=(_notes)->lower_bound((_bound)); (_it)!=(_notes)->upper_bound((_bound)); (_it)++ )

#define FOREACH_NOTE_IT_BEGIN_END(_notes,_it) \
    for( Pattern::notes_it_t (_it)=(_notes)->begin(); (_it)!=(_notes)->end(); (_it)++ )

#define FOREACH_NOTE_IT_BOUND(_notes,_it,_bound) \
    for( Pattern::notes_it_t (_it)=(_notes)->lower_bound((_bound)); (_it)!=(_notes)->upper_bound((_bound)); (_it)++ )

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
    return &__notes;
}

inline const Pattern::virtual_patterns_t* Pattern::get_virtual_patterns() const
{
    return &__virtual_patterns;
}

inline const Pattern::virtual_patterns_t* Pattern::get_flattened_virtual_patterns() const
{
    return &__flattened_virtual_patterns;
}

inline void Pattern::insert_note( Note* note, int position )
{
    __notes.insert( std::make_pair( ( position==-1 ? note->get_position() : position ), note ) );
}

inline bool Pattern::virtual_patterns_empty() const
{
    return __virtual_patterns.empty();
}

inline void Pattern::virtual_patterns_clear()
{
    __virtual_patterns.clear();
}

inline void Pattern::virtual_patterns_add( Pattern* pattern )
{
    __virtual_patterns.insert( pattern );
}

inline void Pattern::virtual_patterns_del( Pattern* pattern )
{
    virtual_patterns_cst_it_t it = __virtual_patterns.find( pattern );
    if ( it!=__virtual_patterns.end() ) __virtual_patterns.erase( it );
}

inline void Pattern::flattened_virtual_patterns_clear()
{
    __flattened_virtual_patterns.clear();
}

};

#endif // H2C_PATTERN_H

/* vim: set softtabstop=4 expandtab:  */
