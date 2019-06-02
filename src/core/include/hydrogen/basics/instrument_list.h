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

#ifndef H2C_INSTRUMENT_LIST_H
#define H2C_INSTRUMENT_LIST_H

#include <vector>

#include <hydrogen/object.h>

namespace H2Core
{

class XMLNode;
class Instrument;

/**
 * InstrumentList is a collection of instruments used within a song, a drumkit, ...
 *
 * \ingroup docCore docDataStructure
*/
class InstrumentList : public H2Core::Object
{
	public:
		/** \return #m_sClassName*/
		static const char* className() { return m_sClassName; }
	
		/** constructor */
		InstrumentList();
		/** destructor */
		~InstrumentList();
		/**
		 * copy constructor
		 * \param other
		 */
		InstrumentList( InstrumentList* other );

		/** returns the numbers of instruments */
		int size() const;
		/**
		 * add an instrument to the list
		 * \param instrument a pointer to the instrument to add
		 */
		void operator<<( Instrument* instrument );
		/**
		 * get an instrument from  the list
		 * \param idx the index to get the instrument from
		 */
		Instrument* operator[]( int idx );
		/**
		 * add an instrument to the list
		 * \param instrument a pointer to the instrument to add
		 */
		void add( Instrument* instrument );
		/**
		 * insert an instrument into the list
		 * \param idx the index to insert the instrument at
		 * \param instrument a pointer to the instrument to add
		 */
		void insert( int idx, Instrument* instrument );
		/**
		 * get an instrument from  the list
		 * \param idx the index to get the instrument from
		 */
		Instrument* get( int idx );
		/**
		 * remove the instrument at a given index, does not delete it
		 * \param idx the index
		 * \return a pointer to the removed instrument
		 */
		Instrument* del( int idx );
		/**
		 * remove an instrument from the list, does not delete it
		 * \param instrument the instrument to be removed
		 * \return a pointer to the removed instrument, 0 if not found
		 */
		Instrument* del( Instrument* instrument );
		/**
		 * get the index of an instrument within the instruments
		 * \param instrument a pointer to the instrument to find
		 * \return -1 if not found
		 */
		int index( Instrument* instrument );
		/**
		 * find an instrument within the instruments
		 * \param i the id of the instrument to find
		 * \return 0 if not found
		 */
		Instrument* find( const int i );
		/**
		 * find an instrument within the instruments
		 * \param name the name of the instrument to find
		 * \return 0 if not found
		 */
		Instrument* find( const QString& name );
		/**
		 * find an intrument which play the given midi note
		 * \param note the Midi note of the instrument to find
		 * \return 0 if not found
		 */
		Instrument* findMidiNote( const int note );
		/**
		 * swap the instruments of two different indexes
		 * \param idx_a the first index
		 * \param idx_b the second index
		 */
		void swap( int idx_a, int idx_b );
		/**
		 * move an instrument from a position to another
		 * \param idx_a the start index
		 * \param idx_b the finish index
		 */
		void move( int idx_a, int idx_b );

		/** Calls the Instrument::load_samples() member
		 * function of all Instruments in #__instruments.
		 */
		void load_samples();
		/** Calls the Instrument::unload_samples() member
		 * function of all Instruments in #__instruments.
		 */
		void unload_samples();
		/**
		 * save the intrument list within the given XMLNode
		 * \param node the XMLNode to feed
		 * \param component_id Identifier of the corresponding
		 * component.
		 */
		void save_to( XMLNode* node, int component_id );
		/**
		 * load an instrument list from an XMLNode
		 * \param node the XMLDode to read from
		 * \param dk_path the directory holding the drumkit
		 * data
		 * \param dk_name
		 * \return a new InstrumentList instance
		 */
		static InstrumentList* load_from( XMLNode* node, const QString& dk_path, const QString& dk_name );

		/**
		 * Fix GitHub issue #307, so called "Hi Bongo fiasco".
		 *
		 * Check whether the same MIDI note is assignedto every
		 * instrument - that condition makes MIDI export unusable.
		 * When so, assign each instrument consecutive MIDI note
		 * starting from 36.
		 */
		void fix_issue_307();

		/**
		 * Check if all instruments have assigned the same
		 * MIDI out note
		 */
		bool has_all_midi_notes_same() const;

		/**
		 * Set each instrument consecuteve MIDI
		 * out notes, starting from 36
		 */
		void set_default_midi_out_notes();

	private:
		/** Contains the name of the class.
		 *
		 * This variable allows from more informative log messages
		 * with the name of the class the message is generated in
		 * being displayed as well. Queried using className().*/
		static const char* m_sClassName;
	
		std::vector<Instrument*> __instruments;            ///< the list of instruments
};

// DEFINITIONS

inline int InstrumentList::size() const
{
	return __instruments.size();
}

};

#endif // H2C_INSTRUMENT_LIST_H

/* vim: set softtabstop=4 noexpandtab: */
