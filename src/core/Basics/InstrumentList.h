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
		 * get an instrument from  the list
		 * \param idx the index to get the instrument from
		 */
		std::shared_ptr<Instrument> operator[]( int idx ) const;
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
		bool isValidIndex( int idx ) const;
		/**
		 * get an instrument from  the list
		 * \param idx the index to get the instrument from
		 */
		std::shared_ptr<Instrument> get( int idx ) const;
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
		int index( std::shared_ptr<Instrument> instrument ) const;
		/**
		 * find an instrument within the instruments
		 * \param i the id of the instrument to find
		 * \return 0 if not found
		 */
		std::shared_ptr<Instrument> find( const int i ) const;
		/**
		 * find an instrument within the instruments
		 * \param name the name of the instrument to find
		 * \return 0 if not found
		 */
		std::shared_ptr<Instrument> find( const QString& name ) const;
		/**
		 * find all instruments which play the given midi note
		 * \param nNote the Midi note of the instruments to find
		 * \return 0 if not found
		 */
		std::vector< std::shared_ptr<Instrument> > findByMidiNote( const int nNote ) const;
		/**
		 * move an instrument from a position to another
		 * \param idx_a the start index
		 * \param idx_b the finish index
		 */
		void move( int idx_a, int idx_b );

		/** Calls the Instrument::loadSamples() member
		 * function of all Instruments in #m_pInstruments.
		 */
		void loadSamples( float fBpm = 120 );
		/** Calls the Instrument::unloadSamples() member
		 * function of all Instruments in #m_pInstruments.
		 */
		void unloadSamples();
		/**
		 * save the instrument list within the given XMLNode
		 *
		 * \param node the XMLNode to feed
		 * \param bSongKit Whether the instruments are part of a
		 *   stand-alone kit or part of a song. In the latter case all samples
		 *   located in the corresponding drumkit folder and are referenced by
		 *   filenames. In the former case, each instrument might be
		 *   associated with a different kit and the lookup folder for the
		 *   samples are stored on a per-instrument basis.
		 * @param bKeepMissingSamples Whether layers containing a missing sample
		 *   should be kept or discarded.
		 * \param bSilent if set to true, all log messages except of errors and
		 *   warnings are suppressed.
		 */
		void saveTo( XMLNode& node, bool bSongKit,
					bool bKeepMissingSamples, bool bSilent ) const;

		/**
		 * load an instrument list from an XMLNode
		 *
		 * \param node the XMLDode to read from
		 * \param sDrumkitPath the directory holding the #Drumkit
		 * \param sDrumkitName name of the #Drumkit found in @a sDrumkitPath
		 * @param sSongPath If not empty, absolute path to the .h2song file the
		 *   instrument list is contained in. It is used to resolve sample
		 *   paths relative to the .h2song file.
		 * \param license License assigned to all Samples that will be
		 *   loaded. If empty, the license will be read from @a dk_path.
		 * @param bSongKit If true samples are loaded on a
		 *   per-instrument basis. If the filename of the sample is a plain
		 *   filename, it will be searched for in the folder associated with the
		 *   drumkit named in "drumkit" (name for portability) and "drumkitPath"
		 *   (unique identifier locally). If it is an absolute path, it will be
		 *   loaded directly.
		 * \param pLegacyFormatEncountered will be set to `true` is any of the
		 *   XML elements requires legacy format support and left untouched
		 *   otherwise.
		 * \param bSilent if set to true, all log messages except of
		 *   errors and warnings are suppressed.
		 *
		 * \return a new InstrumentList instance
		 */
	static std::shared_ptr<InstrumentList> loadFrom(
		const XMLNode& node,
		const QString& sDrumkitPath,
		const QString& sDrumkitName,
		const QString& sSongPath = "",
		const License& license = License(),
		bool bSongKit = false,
		bool* pLegacyFormatEncountered = nullptr,
		bool bSilent = false );
	/**
	 * Returns vector of lists containing instrument name, component
	 * name, file name, the license of all associated samples.
	 */
	std::vector<std::shared_ptr<Content>> summarizeContent() const;

		/**
		 * Check if all instruments have assigned the same
		 * MIDI out note
		 */
		bool hasAllMidiNotesSame() const;

		/**
		 * Set each instrument consecuteve MIDI
		 * out notes, starting from #MIDI_DEFAULT_OFFSET
		 */
		void setDefaultMidiOutNotes();
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
		bool isAnyInstrumentSoloed() const;

		bool isAnyInstrumentSampleLoaded() const;

	private:
		std::vector<std::shared_ptr<Instrument>> m_pInstruments;            ///< the list of instruments
};

// DEFINITIONS

inline int InstrumentList::size() const
{
	return m_pInstruments.size();
}

};

#endif // H2C_INSTRUMENT_LIST_H

/* vim: set softtabstop=4 noexpandtab: */
