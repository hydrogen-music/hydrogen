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

#ifndef H2C_PATTERN_LIST_H
#define H2C_PATTERN_LIST_H

#include <memory>
#include <vector>

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/DrumkitMap.h>
#include <core/Object.h>

namespace H2Core
{

class Drumkit;
class InstrumentList;
class Note;
class Pattern;
class XMLNode;

/**
 * PatternList is a collection of patterns
*/
/** \ingroup docCore docDataStructure */
  class PatternList : public H2Core::Object<PatternList>,
					  public H2Core::AudioEngineLocking,
					  public std::enable_shared_from_this<PatternList> {
		H2_OBJECT(PatternList)
	public:
		PatternList();
		~PatternList();

		PatternList( std::shared_ptr<PatternList> pOther );
	
		/**
		 * load a #PatternList from an XMLNode
		 * \param pNode the XMLDode to read from
		 * \param sDrumkitName kit the pattern was created for (only used as
		 *   fallback).
		 * \param pDrumkit In case the PatternList is loaded as part of a song,
		 *   the current drumkit used to retrieve the types of all contained
		 *   notes.
		 * \param bSilent Whether infos, warnings, and errors should
		 * be logged.
		 * \return a new Pattern instance
		 */
	static std::shared_ptr<PatternList> loadFrom( const XMLNode& pNode,
												  const QString& sDrumkitName,
												  std::shared_ptr<Drumkit> pDrumkit = nullptr,
												  bool bSilent = false );

		/** Stores a serialized version of the instance to the XML note @a
		 * pNote.
		 *
		 * @param pNode the XMLNode to feed
		 * @param nInstrumentId If set to a value other than #EMPTY_INSTR_ID, it
		 *   is used to filter serialized notes by requiring a matching id.
		 * @param sType If set to a non-empty value, it is used to filter
		 *   serialized notess by requiring a matching type.
		 * @param nPitch If a valid one is provided, one those notes matching
		 *   this particular pitch will be stored. */
		void saveTo( XMLNode& pNode, int nInstrumentId = EMPTY_INSTR_ID,
					  const QString& sType = "",
					  int nPitch = PITCH_INVALID ) const;

		/** returns the numbers of patterns */
		int size() const;

		/**
		 * get a pattern from  the list
		 * \param idx the index to get the pattern from
		 */
		std::shared_ptr<Pattern> operator[]( int idx ) const;
		/**
		 * add a pattern to the list
		 * \param pattern a pointer to the pattern to add
		 * \param bAddVirtuals Whether virtual patterns contained in
		 * @a pattern should be added too.
		 */
	void add( std::shared_ptr<Pattern> pPattern, bool bAddVirtuals = false );
		/**
		 * insert a pattern into the list
		 * \param idx the index to insert the pattern at
		 * \param pPattern a pointer to the pattern to add
		 */
		void insert( int idx, std::shared_ptr<Pattern> pPattern );
		/**
		 * get a pattern from  the list
		 * \param idx the index to get the pattern from
		 */
		std::shared_ptr<Pattern> get( int idx ) const;
		/**
		 * remove the pattern at a given index, does not delete it
		 * \param idx the index
		 * \return a pointer to the removed pattern
		 */
		std::shared_ptr<Pattern> del( int idx );
		/**
		 * remove a pPattern from the list, does not delete it
		 * \param pattern the pattern to be removed
		 * \return a pointer to the removed pattern, 0 if not found
		 */
		std::shared_ptr<Pattern> del( std::shared_ptr<Pattern> pPattern );
		/**
		 * get the index of the pattern within the patterns
		 * \param pPattern a pointer to the pattern to find
		 * \return -1 if not found
		 */
		int index( const std::shared_ptr<Pattern> pPattern ) const;
		/**
		 * replace the pattern at a given index with a new one
		 * \param idx the index
		 * \param pPattern the new pattern to insert
		 * \return a pointer to the removed pattern, 0 if index out of bounds
		 */
		std::shared_ptr<Pattern> replace( int idx,
										  std::shared_ptr<Pattern> pPattern );
		/**
		 * empty the pattern list
		 */
		void clear();
		/**
		 * call compute_flattened_virtual_patterns on each pattern
		 */
		void flattenedVirtualPatternsCompute();
		/**
		 * call del_virtual_pattern on each pattern
		 * \param pPattern the pattern to remove where it's found
		 */
		void virtualPatternDel( std::shared_ptr<Pattern> pPattern );
		/**
		 * check if a pattern with name patternName already exists in this list
		 * \param patternName name of a pattern to check
		 * \param pIgnore optional pattern in the list to ignore
		 */
		bool checkName( const QString& patternName,
						std::shared_ptr<Pattern> pIgnore = nullptr ) const;
		/**
		 * find unused patternName
		 * \param sourceName base name to start with
		 * \param pIgnore optional pattern in the list to ignore
		 */
		QString findUnusedPatternName( const QString& sourceName,
									   std::shared_ptr<Pattern> pIgnore = nullptr ) const;

		/**
		 * Get the length of the longest pattern in the list
		 *
		 * \param bIncludeVirtuals In case there are virtual patterns
		 * present this argument specifies whether to include their
		 * contained patterns as well.
		 *
		 * \return pattern length in ticks, -1 if list is empty
		 */
		int longestPatternLength( bool bIncludeVirtuals = true ) const;
		std::shared_ptr<Pattern> getLongestPattern( bool bIncludeVirtuals = true ) const;

		void mapToDrumkit( std::shared_ptr<Drumkit> pDrumkit,
						   std::shared_ptr<Drumkit> pOldDrumkit = nullptr );
		std::set<DrumkitMap::Type> getAllTypes() const;
		std::vector<std::shared_ptr<Note>> getAllNotesOfType(
			const DrumkitMap::Type& sType ) const;

		friend bool operator==( const PatternList& lhs, const PatternList& rhs );
		friend bool operator!=( const PatternList& lhs, const PatternList& rhs );


		/** Formatted string version for debugging purposes.
		 * \param sPrefix String prefix which will be added in front of
		 * every new line
		 * \param bShort Instead of the whole content of all classes
		 * stored as members just a single unique identifier will be
		 * displayed without line breaks.
		 *
		 * \return String presentation of current object.*/
		QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

		/** Iteration */
		std::vector<std::shared_ptr<Pattern>>::iterator begin();
		std::vector<std::shared_ptr<Pattern>>::iterator end();
		std::vector<std::shared_ptr<Pattern>>::const_iterator cbegin() const;
		std::vector<std::shared_ptr<Pattern>>::const_iterator cend() const;

	private:
		std::vector<std::shared_ptr<Pattern>> m_pPatterns;            ///< the list of patterns

};

// DEFINITIONS

inline int PatternList::size() const
{
	return m_pPatterns.size();
}

inline void PatternList::clear()
{
	m_pPatterns.clear();
}

inline std::shared_ptr<Pattern> PatternList::operator[]( int idx ) const {
	return get( idx );
}

};

#endif // H2C_PATTERN_LIST_H

/* vim: set softtabstop=4 noexpandtab: */
