/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include <core/Basics/DrumkitMap.h>
#include <core/Basics/Note.h>
#include <core/Helpers/Xml.h>

namespace H2Core
{

class Drumkit;
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
		typedef std::multimap<int, std::shared_ptr<Note>> notes_t;
		///< multimap note iterator type
		typedef notes_t::iterator notes_it_t;
		///< multimap note const iterator type
		typedef notes_t::const_iterator notes_cst_it_t;
		///< note set type;
		typedef std::set<std::shared_ptr<Pattern>> virtual_patterns_t;
		///< note set iterator type;
		typedef virtual_patterns_t::iterator virtual_patterns_it_t;
		///< note set const iterator type;
		typedef virtual_patterns_t::const_iterator virtual_patterns_cst_it_t;

	/** allow iteration of all contained virtual patterns.*/
	std::set<std::shared_ptr<Pattern>>::iterator begin();
	std::set<std::shared_ptr<Pattern>>::iterator end();
	
		/**
		 * constructor
		 * \param name the name of the pattern
		 * \param info Initialized with an empty string.
		 * \param category the category of the pattern
		 * \param length the length of the pattern
		 * \param denominator the denominator for meter representation (eg 4/4)
		 */
		Pattern( const QString& name="Pattern", const QString& info="",
				 const QString& sCategory = "", int length=MAX_NOTES,
				 int denominator=4 );
		/** copy constructor */
		Pattern( std::shared_ptr<Pattern> pOther );
		/** destructor */
		~Pattern();

		/**
		 * load a pattern from a file
		 * \param sPatternPath the path to the file to load the pattern from
		 */
		static std::shared_ptr<Pattern> load( const QString& sPatternPath );
		/**
		 * load a pattern from an XMLNode
		 * \param node the XMLDode to read from
		 * \param sDrumkitName kit the pattern was created for (only used as
		 *   fallback).
		 * \param bSilent Whether infos, warnings, and errors should
		 * be logged.
		 * \return a new Pattern instance
		 */
	static std::shared_ptr<Pattern> loadFrom( const XMLNode& node,
											   const QString& sDrumkitName,
											   bool bSilent = false );
		/**
		 * save a pattern into an xml file
		 * \param sPatternPath the path to save the pattern into
		 * \param bSilent whever to log info and debug messages.
		 * \return true on success
		 */
		bool save( const QString& sPatternPath, bool bSilent = false ) const;

		/** Stores a serialized version of the instance to the XML note @a
		 * pNote.
		 *
		 * @param node the XMLNode to feed
		 * @param nInstrumentId If set to a value other than #EMPTY_INSTR_ID, it
		 *   is used to filter serialized notes by requiring a matching id.
		 * @param sType If set to a non-empty value, it is used to filter
		 *   serialized notess by requiring a matching type.
		 * @param bSilent whever to log info and debug messages. */
		void saveTo( XMLNode& node, int nInstrumentId = EMPTY_INSTR_ID,
					 const QString& sType = "", bool bSilent = false ) const;

		void setVersion( int nVersion );
		int getVersion() const;
		void setDrumkitName( const QString& sDrumkitName );
		const QString& getDrumkitName() const;
		void setAuthor( const QString& sAuthor );
		const QString& getAuthor() const;
		void setLicense( const License& sLicense );
		const License& getLicense() const;
		///< set the name of the pattern
		void setName( const QString& sName );
		///< get the name of the pattern
		const QString& getName() const;
		///< set the category of the pattern
		void setCategory( const QString& sCategory );
		///< get the category of the pattern
		const QString& getCategory() const;
		///< set the info of the pattern
		void setInfo( const QString& sInfo );
		///< get the info of the pattern
		const QString& getInfo() const;
		///< set the length of the pattern
		void setLength( int nLength );
		///< get the length of the pattern
		int getLength() const;
		///< set the denominator of the pattern
		void setDenominator( int nDenominator );
		///< get the denominator of the pattern
		int getDenominator() const;
		///< get the note multimap
		const notes_t* getNotes() const;
		///< get the virtual pattern set
		const virtual_patterns_t* getVirtualPatterns() const;
		///< get the flattened virtual pattern set
		const virtual_patterns_t* getFlattenedVirtualPatterns() const;

		/**
		 * insert a new note within m_notes
		 * \param pNote the note to be inserted
		 */
		void insertNote( std::shared_ptr<Note> pNote );
		/**
		 * search for a note at a given index within m_notes which correspond to
		 * the given arguments
		 *
		 * \param nIdx_a the first m_notes index to search in
		 * \param nIdx_b the second m_notes index to search in, will be omitted if is -1
		 * \param nInstrumentId the instrument ID the note should be associated
		 *   with
		 * \param sInstrumentType the instrument type the note should be
		 *   associated with
		 * \param bStrict if set to false, will search for a note around the given idx
		 * \return the note if found, 0 otherwise
		 */
		std::shared_ptr<Note> findNote( int nIdx_a, int nIdx_b, int nInstrumentId,
										const QString& sInstrumentType,
										bool bStrict = true ) const;
		/**
		 * search for a note at a given index within m_notes which correspond to the given arguments
		 * \param nIdx_a the first m_notes index to search in
		 * \param nIdx_b the second m_notes index to search in, will be omitted if is -1
		 * \param nInstrumentId the instrument ID the note should be associated
		 *   with
		 * \param sInstrumentType the instrument type the note should be
		 *   associated with
		 * \param key the key that should be set to the note
		 * \param octave the octave that should be set to the note
		 * \param bStrict if set to false, will search for a note around the given idx
		 * \return the note if found, 0 otherwise
		 */
		std::shared_ptr<Note> findNote( int nIdx_a, int nIdx_b, int nInstrumentId,
										const QString& sInstrumentType,
										Note::Key key, Note::Octave octave,
										bool bStrict = true ) const;
		/**
		 * removes a given note from m_notes, it's not deleted
		 * \param pNote the note to be removed
		 */
		void removeNote( std::shared_ptr<Note> pNote );

		/**
		 * check if this pattern contains a note referencing the given instrument
		 * \param pInstr the instrument
		*/
		bool references( std::shared_ptr<Instrument> pInstr ) const;
		/**
		 * delete the notes referencing the given instrument
		 * The function is thread safe (it locks the audio data while deleting notes)
		 * \param pInstr the instrument
		*/
		void purgeInstrument( std::shared_ptr<Instrument> pInstr,
							  bool bRequiredLock = true );
		/** Erase all notes. */
		void clear( bool bRequiredLock = true );
		/**
		 * mark all notes as old
		 */
		void setToOld();

		///< return true if m_virtualPatterns is empty
		bool virtualPatternsEmpty() const;
		///< clear m_virtualPatterns
		void virtualPatternsClear();
		/**
		 * add a pattern to m_virtualPatterns
		 * \param pattern the pattern to add
		 */
		void virtualPatternsAdd( std::shared_ptr<Pattern> pPattern );
		/**
		 * remove a pattern from virtual_pattern set, flattened virtual patterns have to be rebuilt
		 *                   */
		void virtualPatternsDel( std::shared_ptr<Pattern> pPattern );
		///< clear flattened_virtual_patterns
		void flattenedVirtualPatternsClear();
		/**
		 * compute virtual_pattern_transitive_closure_set based on virtual_pattern_transitive_closure_set
		 * virtual_pattern_transitive_closure_set must have been cleared before which is the case is called
		 * from PatternList::compute_flattened_virtual_patterns
		 */
		void flattenedVirtualPatternsCompute();
	/**
	 * Add content of m_flattenedVirtualPatterns into @a
	 * pPatternList.
	 *
	 * Companion function of removeFlattenedVirtualPatterns();
	 */
	void addFlattenedVirtualPatterns( PatternList* pPatternList );
	/**
	 * Add content of m_flattenedVirtualPatterns into @a
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


		void mapTo( std::shared_ptr<Drumkit> pDrumkit,
					std::shared_ptr<Drumkit> pOldDrumkit = nullptr );

		/** Aggregates all types of the contained notes. */
		std::set<DrumkitMap::Type> getAllTypes() const;
		std::vector<std::shared_ptr<Note>> getAllNotesOfType(
			const DrumkitMap::Type& sType ) const;

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
		int m_nVersion;
		/** Name of the kit using which the pattern was written. This is mainly
		 * used for backward compatibility. */
		QString m_sDrumkitName;

		QString m_sAuthor;
		License m_license;

	/**
	 * Determines the accessible range or notes within the
	 * pattern.
	 *
	 * Notes are allow to be located at positions larger than
	 * #m_nLength. This can happen when programming a pattern and
	 * decreasing its length later on. Those notes will be stored in
	 * the associated .h2pattern and .h2song but won't be played back
	 * or exported into a MIDI file. 
	 */
		int m_nLength;
		/** meter denominator of the pattern used in meter (eg 4/4) */
		int m_nDenominator;
		/** name of the pattern */
		QString m_sName;
		/** category of the pattern */
		QString m_sCategory;
		/** a description of the pattern */
		QString m_sInfo;
		/** multimap (hash with possible multiple values for one key) of note */
		notes_t m_notes;
		/** list of patterns directly referenced by this one */
		virtual_patterns_t m_virtualPatterns;
		/** complete list of virtual patterns */
		virtual_patterns_t m_flattenedVirtualPatterns;
	/**
	 * Loads the pattern stored in @a sPatternPath into @a pDoc and
	 * takes care of all the error handling.
	 *
	 * \return true on success.
	 */
	static bool loadDoc( const QString& sPatternPath, XMLDoc* pDoc,
						 bool bSilent = false );

		/** Used to indicate changes in the underlying XSD file. */
		static constexpr int nCurrentFormatVersion = 2;
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
	for( Pattern::notes_cst_it_t _it=(_notes)->begin(); (_it)!=(_notes)->end() && (_it)->first < (_pattern)->getLength(); (_it)++ )

/** Iterate over all notes in column @a _bound in an immutable way if
 * it is contained in @a _pattern. */
#define FOREACH_NOTE_CST_IT_BOUND_LENGTH(_notes,_it,_bound,_pattern) \
	for( Pattern::notes_cst_it_t _it=(_notes)->lower_bound((_bound)); (_it)!=(_notes)->end() && (_it)->first == (_bound) && (_it)->first < (_pattern)->getLength(); (_it)++ )

/** Iterate over all accessible notes between position 0 and length of
 * @a _pattern in a mutable way. */
#define FOREACH_NOTE_IT_BEGIN_LENGTH(_notes,_it,_pattern) \
	for( Pattern::notes_it_t _it=(_notes)->begin(); (_it)!=(_notes)->end() && (_it)->first < (_pattern)->getLength(); (_it)++ )

/** Iterate over all notes in column @a _bound in a mutable way if
 * it is contained in @a _pattern. */
#define FOREACH_NOTE_IT_BOUND_LENGTH(_notes,_it,_bound,_pattern) \
	for( Pattern::notes_it_t _it=(_notes)->lower_bound((_bound)); (_it)!=(_notes)->end() && (_it)->first == (_bound) && (_it)->first < (_pattern)->getLength(); (_it)++ )

// DEFINITIONS
inline void Pattern::setVersion( int nVersion ) {
	m_nVersion = nVersion;
}
inline int Pattern::getVersion() const {
	return m_nVersion;
}
inline void Pattern::setDrumkitName( const QString& sDrumkitName ) {
	m_sDrumkitName = sDrumkitName;
}
inline const QString& Pattern::getDrumkitName() const {
	return m_sDrumkitName;
}
inline void Pattern::setAuthor( const QString& sAuthor ) {
	m_sAuthor = sAuthor;
}
inline const QString& Pattern::getAuthor() const {
	return m_sAuthor;
}
inline void Pattern::setLicense( const License& license ) {
	m_license = license;
}
inline const License& Pattern::getLicense() const {
	return m_license;
}
inline void Pattern::setName( const QString& name )
{
	m_sName = name;
}

inline const QString& Pattern::getName() const
{
	return m_sName;
}

inline void Pattern::setInfo( const QString& info )
{
	m_sInfo = info;
}

inline const QString& Pattern::getInfo() const
{
	return m_sInfo;
}

inline void Pattern::setCategory( const QString& category )
{
	m_sCategory = category;
}

inline const QString& Pattern::getCategory() const
{
	return m_sCategory;
}

inline void Pattern::setLength( int length )
{
	m_nLength = length;
}

inline int Pattern::getLength() const
{
	return m_nLength;
}

inline void Pattern::setDenominator( int denominator )
{
	m_nDenominator = denominator;
}

inline int Pattern::getDenominator() const
{
	return m_nDenominator;
}

inline const Pattern::notes_t* Pattern::getNotes() const
{
	return &m_notes;
}

inline const Pattern::virtual_patterns_t* Pattern::getVirtualPatterns() const
{
	return &m_virtualPatterns;
}

inline const Pattern::virtual_patterns_t* Pattern::getFlattenedVirtualPatterns() const
{
	return &m_flattenedVirtualPatterns;
}

inline void Pattern::insertNote( std::shared_ptr<Note> pNote )
{
	if ( pNote != nullptr ) {
		m_notes.insert( std::make_pair( pNote->get_position(), pNote ) );
	}
}

inline bool Pattern::virtualPatternsEmpty() const
{
	return m_virtualPatterns.empty();
}

inline void Pattern::virtualPatternsClear()
{
	m_virtualPatterns.clear();
}

inline void Pattern::virtualPatternsAdd( std::shared_ptr<Pattern> pPattern )
{
	m_virtualPatterns.insert( pPattern );
}

inline void Pattern::virtualPatternsDel( std::shared_ptr<Pattern> pPattern )
{
	virtual_patterns_cst_it_t it = m_virtualPatterns.find( pPattern );
	if ( it!=m_virtualPatterns.end() ) m_virtualPatterns.erase( it );
}

inline void Pattern::flattenedVirtualPatternsClear()
{
	m_flattenedVirtualPatterns.clear();
}

};

#endif // H2C_PATTERN_H

/* vim: set softtabstop=4 noexpandtab:  */
