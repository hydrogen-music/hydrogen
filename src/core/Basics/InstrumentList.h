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

#ifndef H2C_INSTRUMENT_LIST_H
#define H2C_INSTRUMENT_LIST_H

#include <vector>
#include <memory>
#include <core/License.h>
#include <core/Object.h>

namespace H2Core
{

class XMLNode;
class Instrument;
class DrumkitComponent;

/**
 * InstrumentList is a collection of instruments used within a song, a drumkit, ...
*/
/** \ingroup docCore docDataStructure */
class InstrumentList : public H2Core::Object<InstrumentList>
{
		H2_OBJECT(InstrumentList)
	public:

		struct Content {
			QString m_sInstrumentName;
			QString m_sComponentName;
			QString m_sSampleName;
			QString m_sFullSamplePath;
			License m_license;

			Content( const QString& sInstrumentName,
					 const QString& sComponentName,
					 const QString& sSampleName,
					 const QString& sFullSamplePath,
					 const License& license ) :
				m_sInstrumentName( sInstrumentName ),
				m_sComponentName( sComponentName ),
				m_sSampleName( sSampleName ),
				m_sFullSamplePath( sFullSamplePath ),
				m_license( license ) {
			};
			
			QString toQString( const QString& sPrefix = "", bool bShort = true ) const;
		};
		
		/** constructor */
		InstrumentList();
		/** destructor */
		~InstrumentList();
		/**
		 * copy constructor
		 * \param other
		 */
		InstrumentList( std::shared_ptr<InstrumentList> other );

		/** returns the numbers of instruments */
		int size() const;
		/**
		 * add an instrument to the list
		 * \param instrument a pointer to the instrument to add
		 */
		void operator<<( std::shared_ptr<Instrument> instrument );
		/**
		 * get an instrument from  the list
		 * \param idx the index to get the instrument from
		 */
		std::shared_ptr<Instrument> operator[]( int idx );
	/**
	 * Superficial comparison check. If it succeeds, both objects are
	 * identical. If it fails, however, they are not guarantued to be
	 * not identical.
	 */
	bool operator==( std::shared_ptr<InstrumentList> pOther ) const;
	bool operator!=( std::shared_ptr<InstrumentList> pOther ) const;
		/**
		 * add an instrument to the list
		 * \param instrument a pointer to the instrument to add
		 */
		void add( std::shared_ptr<Instrument> instrument );
		/**
		 * insert an instrument into the list
		 * \param idx the index to insert the instrument at
		 * \param instrument a pointer to the instrument to add
		 */
		void insert( int idx, std::shared_ptr<Instrument> instrument );
		
		/**
		 * check if there is a idx is a valid index for this list
		 * without throwing an error messaage
		 * \param idx the index of the instrument
		 */
		bool is_valid_index( int idx ) const;
		/**
		 * get an instrument from  the list
		 * \param idx the index to get the instrument from
		 */
		std::shared_ptr<Instrument> get( int idx ) const;
		/**
		 * remove the instrument at a given index, does not delete it
		 * \param idx the index
		 * \return a pointer to the removed instrument
		 */
		std::shared_ptr<Instrument> del( int idx );
		/**
		 * remove an instrument from the list, does not delete it
		 * \param instrument the instrument to be removed
		 * \return a pointer to the removed instrument, 0 if not found
		 */
		std::shared_ptr<Instrument> del( std::shared_ptr<Instrument> instrument );
		/**
		 * get the index of an instrument within the instruments
		 * \param instrument a pointer to the instrument to find
		 * \return -1 if not found
		 */
		int index( std::shared_ptr<Instrument> instrument );
		/**
		 * find an instrument within the instruments
		 * \param i the id of the instrument to find
		 * \return 0 if not found
		 */
		std::shared_ptr<Instrument> find( const int i );
		/**
		 * find an instrument within the instruments
		 * \param name the name of the instrument to find
		 * \return 0 if not found
		 */
		std::shared_ptr<Instrument> find( const QString& name );
		/**
		 * find an instrument which play the given midi note
		 * \param note the Midi note of the instrument to find
		 * \return 0 if not found
		 */
		std::shared_ptr<Instrument> findMidiNote( const int note );
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
		void load_samples( float fBpm = 120 );
		/** Calls the Instrument::unload_samples() member
		 * function of all Instruments in #__instruments.
		 */
		void unload_samples();
		/**
		 * save the instrument list within the given XMLNode
		 * \param node the XMLNode to feed
		 * \param component_id Identifier of the corresponding
		 * component.
		 * \param bRecentVersion Whether the drumkit format should be
		 * supported by Hydrogen 0.9.7 or higher (whether it should be
		 * composed of DrumkitComponents).
		 * \param bFull Whether to write all parameters of the
		 * contained #Sample as well. This will be done when storing
		 * an #Instrument as part of a #Song but not when storing
		 * as part of a #Drumkit.
		 */
	void save_to( XMLNode* node, int component_id, bool bRecentVersion = true, bool bFull = false );
		/**
		 * load an instrument list from an XMLNode
		 * \param node the XMLDode to read from
		 * \param sDrumkitPath the directory holding the #Drumkit
		 * \param sDrumkitName name of the #Drumkit found in @a sDrumkitPath
		 * \param license License assigned to all Samples that will be
		 * loaded. If empty, the license will be read from @a dk_path.
		 * \param bSilent if set to true, all log messages except of
		 * errors and warnings are suppressed.
		 *
		 * \return a new InstrumentList instance
		 */
	static std::shared_ptr<InstrumentList> load_from( XMLNode* node,
									  const QString& sDrumkitPath,
									  const QString& sDrumkitName,
									  const License& license = License(),
									  bool bSilent = false );
	/**
	 * Returns vector of lists containing instrument name, component
	 * name, file name, the license of all associated samples.
	 */
	std::vector<std::shared_ptr<Content>> summarizeContent( const std::shared_ptr<std::vector<std::shared_ptr<DrumkitComponent>>> pDrumkitComponents ) const;

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
	std::vector<std::shared_ptr<Instrument>>::iterator begin();
	std::vector<std::shared_ptr<Instrument>>::iterator end();

		/** Check if any instrument in the list is solo'd */
		bool isAnyInstrumentSoloed();

	private:
		std::vector<std::shared_ptr<Instrument>> __instruments;            ///< the list of instruments
};

// DEFINITIONS

inline int InstrumentList::size() const
{
	return __instruments.size();
}

};

#endif // H2C_INSTRUMENT_LIST_H

/* vim: set softtabstop=4 noexpandtab: */
