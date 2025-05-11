/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef H2C_PATTERN_H
#define H2C_PATTERN_H

#include <set>
#include <memory>
#include <core/License.h>
#include <core/Object.h>
#include <core/Basics/Note.h>
#include <core/Helpers/Xml.h>

namespace H2Core
{

class XMLNode;
class Instrument;
class InstrumentList;
class PatternList;

/**
Pattern class is a Note container
*/
/** \ingroup docCore docDataStructure */
class Pattern : public H2Core::Object<Pattern>
{
		H2_OBJECT(Pattern)
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

	/** allow iteration of all contained virtual patterns.*/
	std::set<Pattern*>::iterator begin();
	std::set<Pattern*>::iterator end();
	
		/**
		 * constructor
		 * \param name the name of the pattern
		 * \param info Initialized with an empty string.
		 * \param category the category of the pattern
		 * \param length the length of the pattern
		 * \param denominator the denominator for meter representation (eg 4/4)
		 */
		Pattern( const QString& name="Pattern", const QString& info="", const QString& category="not_categorized", int length=MAX_NOTES, int denominator=4 );
		/** copy constructor */
		Pattern( Pattern* other );
		/** destructor */
		~Pattern();

		/**
		 * load a pattern from a file
		 * \param sPatternPath the path to the file to load the pattern from
		 * \param pInstruments the current instrument list to search instrument
		 *   into
		 * \param bSilent Whether infos, warnings, and errors should
		 *   be logged.
		 */
		static Pattern* load_file( const QString& sPpatternPath,
								   std::shared_ptr<InstrumentList> pInstruments,
								   bool bSilent = false );
		/**
		 * load a pattern from an XMLNode
		 * \param node the XMLDode to read from
		 * \param instruments the current instrument list to search
		 *   instrument into
		 * \param bSilent Whether infos, warnings, and errors should
		 *   be logged.
		 * \return a new Pattern instance
		 */
	static Pattern* load_from( XMLNode* node, std::shared_ptr<InstrumentList> instruments, bool bSilent = false );
		/**
		 * save a pattern into an xml file
		 * \param drumkit_name the name of the drumkit it is supposed to play with
		 * \param author the name of the author
		 * \param license the license that applies to it
		 * \param pattern_path the path to save the pattern into
		 * \param overwrite allows to write over existing pattern file
		 * \return true on success
		 */
		bool save_file( const QString& drumkit_name, const QString& author, const License& license, const QString& pattern_path, bool overwrite=false ) const;

		///< set the name of the pattern
		void set_name( const QString& name );
		///< get the name of the pattern
		const QString& get_name() const;
		///< set the category of the pattern
		void set_category( const QString& category );
		///< set the info of the pattern
		void set_info( const QString& info );
		///< get the info of the pattern
		const QString& get_info() const;
		///< get the category of the pattern
		const QString& get_category() const;
		///< set the length of the pattern
		void set_length( int length );
		///< get the length of the pattern
		int get_length() const;
		///< set the denominator of the pattern
		void set_denominator( int denominator );
		///< get the denominator of the pattern
		int get_denominator() const;
		///< get the note multimap
		const notes_t* get_notes() const;
		///< get the virtual pattern set
		const virtual_patterns_t* get_virtual_patterns() const;
		///< get the flattened virtual pattern set
		const virtual_patterns_t* get_flattened_virtual_patterns() const;

		/**
		 * insert a new note within __notes
		 * \param note the note to be inserted
		 */
		void insert_note( Note* note );
		/**
		 * search for a note at a given index within __notes which correspond to the given arguments
		 * \param idx_a the first __notes index to search in
		 * \param idx_b the second __notes index to search in, will be omitted if is -1
		 * \param instrument the instrument the note should be playing
		 * \param strict if set to false, will search for a note around the given idx
		 * \return the note if found, 0 otherwise
		 */
		Note* find_note( int idx_a, int idx_b, std::shared_ptr<Instrument> instrument, bool strict=true ) const;
		/**
		 * search for a note at a given index within __notes which correspond to the given arguments
		 * \param idx_a the first __notes index to search in
		 * \param idx_b the second __notes index to search in, will be omitted if is -1
		 * \param instrument the instrument the note should be playing
		 * \param key the key that should be set to the note
		 * \param octave the octave that should be set to the note
		 * \param strict if set to false, will search for a note around the given idx
		 * \return the note if found, 0 otherwise
		 */
		Note* find_note( int idx_a, int idx_b, std::shared_ptr<Instrument> instrument, Note::Key key, Note::Octave octave, bool strict=true) const;
		/**
		 * removes a given note from __notes, it's not deleted
		 * \param note the note to be removed
		 */
		void remove_note( Note* note );

		/**
		 * check if this pattern contains a note referencing the given instrument
		 * \param instr the instrument
		*/
		bool references( std::shared_ptr<Instrument> instr );
		/**
		 * delete the notes referencing the given instrument
		 * The function is thread safe (it locks the audio data while deleting notes)
		 * \param instr the instrument
		*/
	void purge_instrument( std::shared_ptr<Instrument> instr, bool bRequiredLock = true );
		/** Erase all notes. */
	void clear( bool bRequiredLock = true );
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
	 * Add content of __flattened_virtual_patterns into @a
	 * pPatternList.
	 *
	 * Companion function of removeFlattenedVirtualPatterns();
	 */
	void addFlattenedVirtualPatterns( PatternList* pPatternList );
	/**
	 * Add content of __flattened_virtual_patterns into @a
	 * pPatternList.
	 *
	 * Companion function of addFlattenedVirtualPatterns();
	 */
	void removeFlattenedVirtualPatterns( PatternList* pPatternList );

	int longestVirtualPatternLength() const;
	/**
	 * Whether the pattern holds at least one virtual pattern.
	 */
	bool isVirtual() const;

		/**
		 * save the pattern within the given XMLNode
		 * \param node the XMLNode to feed
		 * \param instrumentOnly export only the notes of that instrument if given
		 */
		void save_to( XMLNode* node, const std::shared_ptr<Instrument> instrumentOnly = nullptr ) const;
		/** Formatted string version for debugging purposes.
		 * \param sPrefix String prefix which will be added in front of
		 * every new line
		 * \param bShort Instead of the whole content of all classes
		 * stored as members just a single unique identifier will be
		 * displayed without line breaks.
		 *
		 * \return String presentation of current object.*/
		QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

	private:
	/**
	 * Determines the accessible range or notes within the
	 * pattern.
	 *
	 * Notes are allow to be located at positions larger than
	 * #__length. This can happen when programming a pattern and
	 * decreasing its length later on. Those notes will be stored in
	 * the associated .h2pattern and .h2song but won't be played back
	 * or exported into a MIDI file. 
	 */
		int __length;
		int __denominator;                                           ///< the meter denominator of the pattern used in meter (eg 4/4)
		QString __name;                                         ///< the name of thepattern
		QString __category;                                     ///< the category of the pattern
		QString __info;											///< a description of the pattern
		notes_t __notes;                                        ///< a multimap (hash with possible multiple values for one key) of note
		virtual_patterns_t __virtual_patterns;                  ///< a list of patterns directly referenced by this one
		virtual_patterns_t __flattened_virtual_patterns;        ///< the complete list of virtual patterns
};

/** Iterate over all provided notes in an immutable way. */
#define FOREACH_NOTE_CST_IT_BEGIN_END(_notes,_it) \
	for( Pattern::notes_cst_it_t _it=(_notes)->begin(); (_it)!=(_notes)->end(); (_it)++ )

/** Iterate over all notes in column @a _bound in an immutable way. */
#define FOREACH_NOTE_CST_IT_BOUND_END(_notes,_it,_bound) \
	for( Pattern::notes_cst_it_t _it=(_notes)->lower_bound((_bound)); (_it)!=(_notes)->end() && (_it)->first == (_bound); (_it)++ )

/** Iterate over all provided notes in a mutable way. */
#define FOREACH_NOTE_IT_BEGIN_END(_notes,_it) \
	for( Pattern::notes_it_t _it=(_notes)->begin(); (_it)!=(_notes)->end(); (_it)++ )

/** Iterate over all notes in column @a _bound in a mutable way. */
#define FOREACH_NOTE_IT_BOUND_END(_notes,_it,_bound)						\
	for( Pattern::notes_it_t _it=(_notes)->lower_bound((_bound)); (_it)!=(_notes)->end() && (_it)->first == (_bound); (_it)++ )

/** Iterate over all accessible notes between position 0 and length of
 * @a _pattern in an immutable way. */
#define FOREACH_NOTE_CST_IT_BEGIN_LENGTH(_notes,_it,_pattern)			\
	for( Pattern::notes_cst_it_t _it=(_notes)->begin(); (_it)!=(_notes)->end() && (_it)->first < (_pattern)->get_length(); (_it)++ )

/** Iterate over all notes in column @a _bound in an immutable way if
 * it is contained in @a _pattern. */
#define FOREACH_NOTE_CST_IT_BOUND_LENGTH(_notes,_it,_bound,_pattern) \
	for( Pattern::notes_cst_it_t _it=(_notes)->lower_bound((_bound)); (_it)!=(_notes)->end() && (_it)->first == (_bound) && (_it)->first < (_pattern)->get_length(); (_it)++ )

/** Iterate over all accessible notes between position 0 and length of
 * @a _pattern in a mutable way. */
#define FOREACH_NOTE_IT_BEGIN_LENGTH(_notes,_it,_pattern) \
	for( Pattern::notes_it_t _it=(_notes)->begin(); (_it)!=(_notes)->end() && (_it)->first < (_pattern)->get_length(); (_it)++ )

/** Iterate over all notes in column @a _bound in a mutable way if
 * it is contained in @a _pattern. */
#define FOREACH_NOTE_IT_BOUND_LENGTH(_notes,_it,_bound,_pattern) \
	for( Pattern::notes_it_t _it=(_notes)->lower_bound((_bound)); (_it)!=(_notes)->end() && (_it)->first == (_bound) && (_it)->first < (_pattern)->get_length(); (_it)++ )

// DEFINITIONS

inline void Pattern::set_name( const QString& name )
{
	__name = name;
}

inline const QString& Pattern::get_name() const
{
	return __name;
}

inline void Pattern::set_info( const QString& info )
{
	__info = info;
}

inline const QString& Pattern::get_info() const
{
	return __info;
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

inline void Pattern::set_denominator( int denominator )
{
	__denominator = denominator;
}

inline int Pattern::get_denominator() const
{
	return __denominator;
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

inline void Pattern::insert_note( Note* note )
{
	__notes.insert( std::make_pair( note->get_position(), note ) );
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

/* vim: set softtabstop=4 noexpandtab:  */
