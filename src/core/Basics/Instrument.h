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

#ifndef H2C_INSTRUMENT_H
#define H2C_INSTRUMENT_H

#include <cassert>
#include <memory>

#include <core/Basics/Adsr.h>
#include <core/Basics/DrumkitMap.h>
#include <core/Basics/Event.h>
#include <core/Helpers/Filesystem.h>
#include <core/License.h>
#include <core/Object.h>

#define EMPTY_INSTR_ID          -1
/** Created Instrument will be used as metronome. */
#define METRONOME_INSTR_ID      -2
#define PLAYBACK_INSTR_ID       -3

namespace H2Core
{

class ADSR;
class InstrumentLayer;
class InstrumentComponent;
class Note;
class Sample;
class XMLNode;

/**
Instrument class
*/
/** \ingroup docCore docDataStructure */
class Instrument : public H2Core::Object<Instrument>
{
		H2_OBJECT(Instrument)
	public:
		/** Maximum support pitch value */
		static constexpr float fPitchMax = 24.5;
		/** Minimum support pitch value */
		static constexpr float fPitchMin = -24.5;

		/**
		 * constructor
		 * \param id the id of this instrument
		 * \param name the name of the instrument
		 * \param adsr attack decay sustain release instance
		 */
		Instrument( const int id=EMPTY_INSTR_ID, const QString& name="", std::shared_ptr<ADSR> adsr=nullptr );
		/** copy constructor */
		Instrument( std::shared_ptr<Instrument> other );
		/** destructor */
		~Instrument();

		/**
		 * Calls the InstrumentLayer::loadSample() member
		 * function of all layers of each component of the
		 * Instrument.
		 */
		void loadSamples( float fBpm = 120 );
		/**
		 * Calls the InstrumentLayer::unloadSample() member
		 * function of all layers of each component of the
		 * Instrument.
		 */
		void unloadSamples();

		/**
		 * save the instrument within the given XMLNode
		 *
		 * \param node the XMLNode to feed
		 * \param bSongKit Whether the instrument is part of a
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
					bool bKeepMissingSamples, bool bSilent );

		/**
		 * load an instrument from an XMLNode
		 * \param pNode the XMLDode to read from
		 * \param sDrumkitPath the directory holding the drumkit
		 *   data. If empty, it will be read from @a pNode.
		 * \param sDrumkitName Name of the drumkit found in @a
		 *   sDrumkitPath.
		 * @param sSongPath If not empty, absolute path to the .h2song file the
		 *   instrument is contained in. It is used to resolve sample paths
		 *   relative to the .h2song file.
		 * \param license License assigned to all Samples that will be
		 *   loaded. If empty, the license will be read from @a
		 *   sDrumkitPath.
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
		 * \return a new Instrument instance
		 */
		static std::shared_ptr<Instrument> loadFrom( const XMLNode& pNode,
													 const QString& sDrumkitPath = "",
													 const QString& sDrumkitName = "",
													 const QString& sSongPath = "",
													 const License& license = License(),
													 bool bSongKit = false,
													 bool* pLegacyFormatEncountered = nullptr,
													 bool bSilent = false );

		///< set the name of the instrument
		void setName( const QString& name );
		///< get the name of the instrument
		const QString& getName() const;

		///< set the id of the instrument
		void setId( const int id );
		///< get the id of the instrument
		int getId() const;

		/** get the ADSR of the instrument */
		std::shared_ptr<ADSR> getAdsr() const;
		/** get a copy of the ADSR of the instrument */
		std::shared_ptr<ADSR> copyAdsr() const;

		/** set the mute group of the instrument */
		void setMuteGroup( int group );
		/** get the mute group of the instrument */
		int getMuteGroup() const;

		/** set the midi out channel of the instrument */
		void setMidiOutChannel( int channel );
		/** get the midi out channel of the instrument */
		int getMidiOutChannel() const;

		/** set the midi out note of the instrument */
		void setMidiOutNote( int note );
		/** get the midi out note of the instrument */
		int getMidiOutNote() const;

		/** set muted status of the instrument */
		void setMuted( bool muted );
		/** get muted status of the instrument */
		bool isMuted() const;

		/** set pan of the instrument */
		void setPan( float val );
		/** set pan of the instrument, assuming the input range in [0;1] */
		void setPanWithRangeFrom0To1( float fVal );
		/** get pan of the instrument */
		float getPan() const;
		/** get pan of the instrument scaling and translating the range from [-1;1] to [0;1] */
		float getPanWithRangeFrom0To1() const {
			return 0.5f * ( 1.f + m_fPan );
		}


		/** set gain of the instrument */
		void setGain( float gain );
		/** get gain of the instrument */
		float getGain() const;
		/** set the volume of the instrument */
		void setVolume( float volume );
		/** get the volume of the instrument */
		float getVolume() const;

		/** activate the filter of the instrument */
		void setFilterActive( bool active );
		/** get the status of the filter of the instrument */
		bool isFilterActive() const;

		/** set the filter resonance of the instrument */
		void setFilterResonance( float val );
		/** get the filter resonance of the instrument */
		float getFilterResonance() const;

		/** set the filter cutoff of the instrument */
		void setFilterCutoff( float val );
		/** get the filter cutoff of the instrument */
		float getFilterCutoff() const;

		/** set the left peak of the instrument */
		void setPeak_L( float val );
		/** get the left peak of the instrument */
		float getPeak_L() const;
		/** set the right peak of the instrument */
		void setPeak_R( float val );
		/** get the right peak of the instrument */
		float getPeak_R() const;

		/** set the fx level of the instrument */
		void setFxLevel( float level, int index );
		/** get the fx level of the instrument */
		float getFxLevel( int index ) const;

		/** set the random pitch factor of the instrument */
		void setRandomPitchFactor( float val );
		/** get the random pitch factor of the instrument */
		float getRandomPitchFactor() const;
		
		/** set the pitch offset of the instrument */
		void setPitchOffset( float val );
		/** get the pitch offset of the instrument */
		float getPitchOffset() const;

		/** set the soloed status of the instrument */
		void setSoloed( bool soloed );
		/** get the soloed status of the instrument */
		bool isSoloed() const;

		bool isAnyComponentSoloed() const;

		/** enqueue the instrument for @a pNote */
		void enqueue( std::shared_ptr<Note> pNote );
		/** dequeue the instrument for @a pNote */
		void dequeue( std::shared_ptr<Note> pNote );
		/** get the queued status of the instrument */
		bool isQueued() const;
		const QStringList& getEnqueuedBy() const;

		/** set the stop notes status of the instrument */
		void setStopNotes( bool stopnotes );
		/** get the stop notes of the instrument */
		bool isStopNotes() const;

		void setHihatGrp( int hihat_grp );
		int getHihatGrp() const;

		void setLowerCc( int message );
		int getLowerCc() const;

		void setHigherCc( int message );
		int getHigherCc() const;

		///< set the path of the related drumkit
		void setDrumkitPath( const QString& sPath );
		///< get the path of the related drumkits
		const QString& getDrumkitPath() const;
		///< set the name of the related drumkit
		void setDrumkitName( const QString& sName );

		/** Mark the instrument as hydrogen's preview instrument */
		void setIsPreviewInstrument(bool isPreview);
		bool isPreviewInstrument() const;

		std::shared_ptr<std::vector<std::shared_ptr<InstrumentComponent>>> getComponents() const;
		/** Select a component via its index in the corresponding vector. */
		std::shared_ptr<InstrumentComponent> getComponent( int nIdx ) const;
		int index( std::shared_ptr<InstrumentComponent> pComponent ) const;
		void addComponent( std::shared_ptr<InstrumentComponent> pComponent );
		void removeComponent( int nIdx );

		void setApplyVelocity( bool apply_velocity );
		bool getApplyVelocity() const;

		bool isCurrentlyExported() const;
		void setCurrentlyExported( bool isCurrentlyExported );

		bool hasMissingSamples() const { return m_bHasMissingSamples; }

		/** An @a nIndex of -1 will cause the method to append the new layer at
		   the end.*/
		void addLayer(
			std::shared_ptr<InstrumentComponent> pComponent,
			std::shared_ptr<InstrumentLayer> pLayer,
			int nIndex,
			Event::Trigger trigger
		);
		void setLayer(
			std::shared_ptr<InstrumentComponent> pComponent,
			std::shared_ptr<InstrumentLayer> pLayer,
			int nIndex,
			Event::Trigger trigger
		);
		void removeLayer(
			std::shared_ptr<InstrumentComponent> pComponent,
			int nIndex,
			Event::Trigger trigger
		);
		/** Whether the instrument contains at least one non-missing
		 * sample */
		bool hasSamples() const;
		void setSample(
			std::shared_ptr<InstrumentComponent> pComponent,
			std::shared_ptr<InstrumentLayer> pLayer,
			std::shared_ptr<Sample> pSample,
			Event::Trigger trigger
		);

		int getLongestSampleFrames() const;

		DrumkitMap::Type getType() const;
		void setType( DrumkitMap::Type type );

		/** Iteration */
		std::vector<std::shared_ptr<InstrumentComponent>>::iterator begin();
		std::vector<std::shared_ptr<InstrumentComponent>>::iterator end();

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
		void checkForMissingSamples( Event::Trigger trigger );

		/** Identifier of an instrument, which should be
		unique. It is set by setId() and accessed via
		getId().*/
		int					m_nId;
	        /** Name of the Instrument. It is set by setName()
		    and accessed via getName().*/
		QString					m_sName;
		DrumkitMap::Type m_type;
	/** Path of the #Drumkit this #Instrument belongs to.
	 *
	 * An instrument belonging to a #Drumkit uses relative paths for
	 * its #Sample. Therefore we have to take care of mapping them to
	 * absolute paths ourselves in case instruments of several
	 * drumkits are mixed in one #Song.
	 */
	QString					m_sDrumkitPath;
	/** Name of the #Drumkit found at @a m_sDrumkitPath.
	 *
	 * This helper variable should only be used during #Instrument
	 * loading. It ensures portability of songs as absolute paths only
	 * serve for unique identifiers locally and also ensures backward
	 * compatibility.
	 */
	QString					m_sDrumkitName;
	float					m_fGain;					///< gain of the instrument
		float					m_fVolume;				///< volume of the instrument
		float					m_fPan;	///< pan of the instrument, [-1;1] from left to right, as requested by Sampler PanLaws
		float					m_fPeak_L;				///< left current peak value
		float					m_fPeak_R;				///< right current peak value
		std::shared_ptr<ADSR>					m_pAdsr;					///< attack delay sustain release instance
		bool					m_bFilterActive;		///< is filter active?
		float					m_fFilterCutoff;		///< filter cutoff (0..1)
		float					m_fFilterResonance;		///< filter resonant frequency (0..1)
	/**
	 * Factor to scale the random contribution when humanizing pitch
	 * between 0 and #AudioEngine::fHumanizePitchSD.
	 *
	 * Supported range [0,1].
	 */
		float					m_fRandomPitchFactor;
		float					m_fPitchOffset;	///< instrument main pitch offset
		int						m_nMidiOutNote;		///< midi out note
		int						m_nMidiOutChannel;		///< midi out channel
		bool					m_bStopNotes;			///< will the note automatically generate a note off after being on
		bool					m_bSoloed;				///< is the instrument in solo mode?
		bool					m_bMuted;				///< is the instrument muted?
		int						m_nMuteGroup;			///< mute group of the instrument
		int						m_nQueued;				///< count the number of notes queued within Sampler::m_playingNotesQueue or std::priority_queue m_songNoteQueue
		/** List of short string representations of notes for which this
		 * instrument was enqueued. */
		QStringList				m_enqueuedBy;
		float					m_fxLevel[MAX_FX];		///< Ladspa FX level array
		int						m_nHihatGrp;			///< the instrument is part of a hihat
		int						m_nLowerCc;				///< lower cc level
		int						m_nHigherCc;			///< higher cc level
		bool					m_bIsPreviewInstrument;		///< is the instrument an hydrogen preview instrument?
		bool					m_bApplyVelocity;				///< change the sample gain based on velocity
		bool					m_bCurrentInstrForExport;		///< is the instrument currently being exported?
		bool 					m_bHasMissingSamples;	///< does the instrument have missing sample files?
		std::shared_ptr<std::vector<std::shared_ptr<InstrumentComponent>>> m_pComponents;
};

inline void Instrument::setName( const QString& name )
{
	m_sName = name;
}
inline const QString& Instrument::getName() const
{
	return m_sName;
}
inline void Instrument::setId( const int id )
{
	m_nId = id;
}
inline int Instrument::getId() const
{
	return m_nId;
}

inline std::shared_ptr<ADSR> Instrument::getAdsr() const
{
	return m_pAdsr;
}

inline std::shared_ptr<ADSR> Instrument::copyAdsr() const
{
	return std::make_shared<ADSR>( m_pAdsr );
}

inline void Instrument::setMuteGroup( int group )
{
	m_nMuteGroup = ( group<-1 ? -1 : group );
}

inline int Instrument::getMuteGroup() const
{
	return m_nMuteGroup;
}

inline int Instrument::getMidiOutChannel() const
{
	return m_nMidiOutChannel;
}

inline void Instrument::setMidiOutChannel( int nChannel )
{
	if ( ( nChannel >= MIDI_OUT_CHANNEL_MIN ) &&
		 ( nChannel <= MIDI_OUT_CHANNEL_MAX ) ) {
		m_nMidiOutChannel = nChannel;
	} else {
		ERRORLOG( QString( "midi out channel [%1] out of bounds [%2,%3]" )
				  .arg( nChannel )
				  .arg( MIDI_OUT_CHANNEL_MIN )
				  .arg( MIDI_OUT_CHANNEL_MAX ) );
	}
}

inline int Instrument::getMidiOutNote() const
{
	return m_nMidiOutNote;
}

inline void Instrument::setMidiOutNote( int note )
{
	if ( ( note >= MIDI_OUT_NOTE_MIN ) && ( note <= MIDI_OUT_NOTE_MAX ) ) {
		m_nMidiOutNote = note;
	} else {
		ERRORLOG( QString( "midi out note %1 out of bounds" ).arg( note ) );
	}
}

inline void Instrument::setMuted( bool muted )
{
	m_bMuted = muted;
}

inline bool Instrument::isMuted() const
{
	return m_bMuted;
}

inline void Instrument::setPan( float val ) //TODO check boundary factorize function?
{
	if ( val > 1.0 ) {
		m_fPan = 1.0;
	} else if ( val < -1.0 ) {
		m_fPan = -1.0;
	} else {
		m_fPan = val;
	}
}

inline float Instrument::getPan() const
{
	return m_fPan;
}

inline void Instrument::setGain( float gain )
{
	m_fGain = gain;
}

inline float Instrument::getGain() const
{
	return m_fGain;
}

inline void Instrument::setVolume( float volume )
{
	m_fVolume = volume;
}

inline float Instrument::getVolume() const
{
	return m_fVolume;
}

inline void Instrument::setFilterActive( bool active )
{
	m_bFilterActive = active;
}

inline bool Instrument::isFilterActive() const
{
	return m_bFilterActive;
}

inline void Instrument::setFilterResonance( float val )
{
	m_fFilterResonance = val;
}

inline float Instrument::getFilterResonance() const
{
	return m_fFilterResonance;
}

inline void Instrument::setFilterCutoff( float val )
{
	m_fFilterCutoff = val;
}

inline float Instrument::getFilterCutoff() const
{
	return m_fFilterCutoff;
}

inline void Instrument::setPeak_L( float val )
{
	m_fPeak_L = val;
}

inline float Instrument::getPeak_L() const
{
	return m_fPeak_L;
}

inline void Instrument::setPeak_R( float val )
{
	m_fPeak_R = val;
}

inline float Instrument::getPeak_R() const
{
	return m_fPeak_R;
}

inline void Instrument::setFxLevel( float level, int index )
{
	m_fxLevel[index] = level;
}

inline float Instrument::getFxLevel( int index ) const
{
	return m_fxLevel[index];
}

inline void Instrument::setRandomPitchFactor( float val )
{
	m_fRandomPitchFactor = val;
}

inline float Instrument::getRandomPitchFactor() const
{
	return m_fRandomPitchFactor;
}

inline float Instrument::getPitchOffset() const
{
	return m_fPitchOffset;
}

inline void Instrument::setSoloed( bool soloed )
{
	m_bSoloed = soloed;
}

inline bool Instrument::isSoloed() const
{
	return m_bSoloed;
}

inline bool Instrument::isQueued() const
{
	return ( m_nQueued > 0 );
}

inline const QStringList& Instrument::getEnqueuedBy() const {
	return m_enqueuedBy;
}

inline void Instrument::setStopNotes( bool stopnotes )
{
	m_bStopNotes = stopnotes;
}

inline bool Instrument::isStopNotes() const
{
	return m_bStopNotes;
}

inline void Instrument::setHihatGrp( int hihat_grp )
{
	m_nHihatGrp = hihat_grp;
}

inline int Instrument::getHihatGrp() const
{
	return m_nHihatGrp;
}

inline void Instrument::setLowerCc( int message )
{
	m_nLowerCc = message;
}

inline int Instrument::getLowerCc() const
{
	return m_nLowerCc;
}

inline void Instrument::setHigherCc( int message )
{
	m_nHigherCc = message;
}

inline int Instrument::getHigherCc() const
{
	return m_nHigherCc;
}

inline void Instrument::setDrumkitPath( const QString& sPath )
{
	m_sDrumkitPath = sPath;
}

inline void Instrument::setDrumkitName( const QString& sName )
{
	m_sDrumkitName = sName;
}

inline bool Instrument::isPreviewInstrument() const
{
	return m_bIsPreviewInstrument;
}

inline void Instrument::setIsPreviewInstrument(bool isPreview)
{
	m_bIsPreviewInstrument = isPreview;
}

inline std::shared_ptr<std::vector<std::shared_ptr<InstrumentComponent>>> Instrument::getComponents() const
{
	return m_pComponents;
}

inline void Instrument::setApplyVelocity( bool apply_velocity )
{
	m_bApplyVelocity = apply_velocity;
}

inline bool Instrument::getApplyVelocity() const
{
	return m_bApplyVelocity;
}

inline bool Instrument::isCurrentlyExported() const
{
	return m_bCurrentInstrForExport;
}

inline void Instrument::setCurrentlyExported( bool isCurrentlyExported )
{
	m_bCurrentInstrForExport = isCurrentlyExported;
}

inline DrumkitMap::Type Instrument::getType() const {
	return m_type;
}

inline void Instrument::setType( DrumkitMap::Type type ) {
	m_type = type;
}

};



#endif // H2C_INSTRUMENT_H

/* vim: set softtabstop=4 noexpandtab:  */
