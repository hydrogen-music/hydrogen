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

#include <core/Object.h>
#include <core/Basics/Adsr.h>
#include <core/Helpers/Filesystem.h>
#include <core/License.h>

#define EMPTY_INSTR_ID          -1
/** Created Instrument will be used as metronome. */
#define METRONOME_INSTR_ID      -2
#define PLAYBACK_INSTR_ID       -3

namespace H2Core
{

class XMLNode;
class ADSR;
class Drumkit;
class DrumkitComponent;
class InstrumentLayer;
class InstrumentComponent;


/**
Instrument class
*/
/** \ingroup docCore docDataStructure */
class Instrument : public H2Core::Object<Instrument>
{
		H2_OBJECT(Instrument)
	public:
		enum SampleSelectionAlgo {
			VELOCITY,
			ROUND_ROBIN,
			RANDOM
		};

		/**
		 * constructor
		 * \param id the id of this instrument
		 * \param name the name of the instrument
		 * \param adsr attack decay sustain release instance
		 */
		Instrument( const int id=EMPTY_INSTR_ID, const QString& name="Empty Instrument", std::shared_ptr<ADSR> adsr=nullptr );
		/** copy constructor */
		Instrument( std::shared_ptr<Instrument> other );
		/** destructor */
		~Instrument();

		/**
		 * creates a new Instrument, loads samples from a given instrument within a given drumkit
		 * \param drumkit_name the drumkit to search the instrument in
		 * \param instrument_name the instrument within the drumkit to load samples from
		 * \return a new Instrument instance
		 */
		static std::shared_ptr<Instrument> load_instrument( const QString& drumkit_path, const QString& instrument_name );

		/**
		 * loads instrument from a given instrument within a given drumkit into a `live` Instrument object.
		 * \param drumkit_path the drumkit to search the instrument in
		 * \param instrument_name the instrument within the drumkit to load samples from
		 * for the drumkit.
		 */
		void load_from( const QString& drumkit_path, const QString& instrument_name );

		/**
		 * loads instrument from a given instrument into a `live` Instrument object.
		 * \param drumkit the drumkit the instrument belongs to
		 * \param instrument to load samples and members from
		 * \param lookup Where to search (system/user folder or both)
		 * for the drumkit.
		 */
		void load_from( std::shared_ptr<Drumkit> drumkit, std::shared_ptr<Instrument> instrument );

		/**
		 * Calls the InstrumentLayer::load_sample() member
		 * function of all layers of each component of the
		 * Instrument.
		 */
		void load_samples( float fBpm = 120 );
		/**
		 * Calls the InstrumentLayer::unload_sample() member
		 * function of all layers of each component of the
		 * Instrument.
		 */
		void unload_samples();

		/**
		 * save the instrument within the given XMLNode
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
		 * load an instrument from an XMLNode
		 * \param pNode the XMLDode to read from
		 * \param sDrumkitPath the directory holding the drumkit
		 * data. If empty, it will be read from @a pNode.
		 * \param sDrumkitName Name of the drumkit found in @a
		 * sDrumkitPath.
		 * \param license License assigned to all Samples that will be
		 * loaded. If empty, the license will be read from @a
		 * sDrumkitPath.
		 * \param bSilent if set to true, all log messages except of
		 * errors and warnings are suppressed.
		 *
		 * \return a new Instrument instance
		 */
		static std::shared_ptr<Instrument> load_from( XMLNode* pNode,
													  const QString& sDrumkitPath = "",
													  const QString& sDrumkitName = "",
													  const License& license = License(),
													  bool bSilent = false );

		///< set the name of the instrument
		void set_name( const QString& name );
		///< get the name of the instrument
		const QString& get_name() const;

		///< set the id of the instrument
		void set_id( const int id );
		///< get the id of the instrument
		int get_id() const;

		/** set the ADSR of the instrument */
		void set_adsr( std::shared_ptr<ADSR> adsr );
		/** get the ADSR of the instrument */
		std::shared_ptr<ADSR> get_adsr() const;
		/** get a copy of the ADSR of the instrument */
		std::shared_ptr<ADSR> copy_adsr() const;

		/** set the mute group of the instrument */
		void set_mute_group( int group );
		/** get the mute group of the instrument */
		int get_mute_group() const;

		/** set the midi out channel of the instrument */
		void set_midi_out_channel( int channel );
		/** get the midi out channel of the instrument */
		int get_midi_out_channel() const;

		/** set the midi out note of the instrument */
		void set_midi_out_note( int note );
		/** get the midi out note of the instrument */
		int get_midi_out_note() const;

		/** set muted status of the instrument */
		void set_muted( bool muted );
		/** get muted status of the instrument */
		bool is_muted() const;

		/** set pan of the instrument */
		void setPan( float val );
		/** set pan of the instrument, assuming the input range in [0;1] */
		void setPanWithRangeFrom0To1( float fVal ) {
			this->setPan( -1.f + 2.f * fVal ); // scale and translate into [-1;1]
		};
		/** get pan of the instrument */
		float getPan() const;
		/** get pan of the instrument scaling and translating the range from [-1;1] to [0;1] */
		float getPanWithRangeFrom0To1() const {
			return 0.5f * ( 1.f + m_fPan );
		}


		/** set gain of the instrument */
		void set_gain( float gain );
		/** get gain of the instrument */
		float get_gain() const;
		/** set the volume of the instrument */
		void set_volume( float volume );
		/** get the volume of the instrument */
		float get_volume() const;

		/** activate the filter of the instrument */
		void set_filter_active( bool active );
		/** get the status of the filter of the instrument */
		bool is_filter_active() const;

		/** set the filter resonance of the instrument */
		void set_filter_resonance( float val );
		/** get the filter resonance of the instrument */
		float get_filter_resonance() const;

		/** set the filter cutoff of the instrument */
		void set_filter_cutoff( float val );
		/** get the filter cutoff of the instrument */
		float get_filter_cutoff() const;

		/** set the left peak of the instrument */
		void set_peak_l( float val );
		/** get the left peak of the instrument */
		float get_peak_l() const;
		/** set the right peak of the instrument */
		void set_peak_r( float val );
		/** get the right peak of the instrument */
		float get_peak_r() const;

		/** set the fx level of the instrument */
		void set_fx_level( float level, int index );
		/** get the fx level of the instrument */
		float get_fx_level( int index ) const;

		/** set the random pitch factor of the instrument */
		void set_random_pitch_factor( float val );
		/** get the random pitch factor of the instrument */
		float get_random_pitch_factor() const;
		
		/** set the pitch offset of the instrument */
		void set_pitch_offset( float val );
		/** get the pitch offset of the instrument */
		float get_pitch_offset() const;

		/** set the active status of the instrument */
		void set_active( bool active );
		/** get the active status of the instrument */
		bool is_active() const;

		/** set the soloed status of the instrument */
		void set_soloed( bool soloed );
		/** get the soloed status of the instrument */
		bool is_soloed() const;

		/** enqueue the instrument */
		void enqueue();
		/** dequeue the instrument */
		void dequeue();
		/** get the queued status of the instrument */
		bool is_queued() const;

		/** set the stop notes status of the instrument */
		void set_stop_notes( bool stopnotes );
		/** get the stop notes of the instrument */
		bool is_stop_notes() const;

		void set_sample_selection_alg( SampleSelectionAlgo selected_algo);
		SampleSelectionAlgo sample_selection_alg() const;

		void set_hihat_grp( int hihat_grp );
		int get_hihat_grp() const;

		void set_lower_cc( int message );
		int get_lower_cc() const;

		void set_higher_cc( int message );
		int get_higher_cc() const;

		///< set the path of the related drumkit
		void set_drumkit_path( const QString& sPath );
		///< get the path of the related drumkits
		QString get_drumkit_path() const;
		///< set the name of the related drumkit
		void set_drumkit_name( const QString& sName );
		///< get the name of the related drumkits
		const QString& get_drumkit_name() const;

		/** Mark the instrument as hydrogen's preview instrument */
		void set_is_preview_instrument(bool isPreview);
		bool is_preview_instrument() const;

		/** Mark the instrument as metronome instrument */
		void set_is_metronome_instrument(bool isMetronome);
		bool is_metronome_instrument() const;

		std::vector<std::shared_ptr<InstrumentComponent>>* get_components();
		std::shared_ptr<InstrumentComponent> get_component( int DrumkitComponentID );

		void set_apply_velocity( bool apply_velocity );
		bool get_apply_velocity() const;

		bool is_currently_exported() const;
		void set_currently_exported( bool isCurrentlyExported );

		bool has_missing_samples() const { return m_bHasMissingSamples; }
		void set_missing_samples( bool bHasMissingSamples ) { m_bHasMissingSamples = bHasMissingSamples; }

	/** Whether the instrument contains at least one non-missing
	 * sample */
	bool hasSamples() const;

		/** Maximum support pitch value */
		static constexpr float fPitchMax = 24.5;
		/** Minimum support pitch value */
		static constexpr float fPitchMin = -24.5;

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
	        /** Identifier of an instrument, which should be
		    unique. It is set by set_id() and accessed via
	        get_id().*/
		int					__id;
	        /** Name of the Instrument. It is set by set_name()
		    and accessed via get_name().*/
		QString					__name;
	/** Path of the #Drumkit this #Instrument belongs to.
	 *
	 * An instrument belonging to a #Drumkit uses relative paths for
	 * its #Sample. Therefore we have to take care of mapping them to
	 * absolute paths ourselves in case instruments of several
	 * drumkits are mixed in one #Song.
	 */
	QString					__drumkit_path;
	/** Name of the #Drumkit found at @a __drumkit_path.
	 *
	 * This helper variable should only be used during #Instrument
	 * loading. It ensures portability of songs as absolute paths only
	 * serve for unique identifiers locally and also ensures backward
	 * compatibility.
	 */
	QString					__drumkit_name;
	float					__gain;					///< gain of the instrument
		float					__volume;				///< volume of the instrument
		float					m_fPan;	///< pan of the instrument, [-1;1] from left to right, as requested by Sampler PanLaws
		float					__peak_l;				///< left current peak value
		float					__peak_r;				///< right current peak value
		std::shared_ptr<ADSR>					__adsr;					///< attack delay sustain release instance
		bool					__filter_active;		///< is filter active?
		float					__filter_cutoff;		///< filter cutoff (0..1)
		float					__filter_resonance;		///< filter resonant frequency (0..1)
	/**
	 * Factor to scale the random contribution when humanizing pitch
	 * between 0 and #AudioEngine::fHumanizePitchSD.
	 *
	 * Supported range [0,1].
	 */
		float					__random_pitch_factor;
		float					__pitch_offset;	///< instrument main pitch offset
		int						__midi_out_note;		///< midi out note
		int						__midi_out_channel;		///< midi out channel
		bool					__stop_notes;			///< will the note automatically generate a note off after being on
		SampleSelectionAlgo		__sample_selection_alg;	///< how Hydrogen will chose the sample to use
		bool					__active;				///< is the instrument active?
		bool					__soloed;				///< is the instrument in solo mode?
		bool					__muted;				///< is the instrument muted?
		int						__mute_group;			///< mute group of the instrument
		int						__queued;				///< count the number of notes queued within Sampler::__playing_notes_queue or std::priority_queue m_songNoteQueue
		float					__fx_level[MAX_FX];		///< Ladspa FX level array
		int						__hihat_grp;			///< the instrument is part of a hihat
		int						__lower_cc;				///< lower cc level
		int						__higher_cc;			///< higher cc level
		bool					__is_preview_instrument;		///< is the instrument an hydrogen preview instrument?
		bool					__is_metronome_instrument;		///< is the instrument an metronome instrument?
		std::vector<std::shared_ptr<InstrumentComponent>>* __components;		///< InstrumentLayer array
		bool					__apply_velocity;				///< change the sample gain based on velocity
		bool					__current_instr_for_export;		///< is the instrument currently being exported?
		bool 					m_bHasMissingSamples;	///< does the instrument have missing sample files?
};

// DEFINITIONS
/** Sets the name of the Instrument #__name.
 * \param name New name. */
inline void Instrument::set_name( const QString& name )
{
	__name = name;
}
/** Access the name of the Instrument.
 * \return #__name */
inline const QString& Instrument::get_name() const
{
	return __name;
}
/** Sets #__id to @a id.
 * \param id Unique identifier of the instrument. */
inline void Instrument::set_id( const int id )
{
	__id = id;
}
/** Returns #__id. 
* \return #__id. */
inline int Instrument::get_id() const
{
	return __id;
}

inline std::shared_ptr<ADSR> Instrument::get_adsr() const
{
	return __adsr;
}

inline std::shared_ptr<ADSR> Instrument::copy_adsr() const
{
	return std::make_shared<ADSR>( __adsr );
}

inline void Instrument::set_mute_group( int group )
{
	__mute_group = ( group<-1 ? -1 : group );
}

inline int Instrument::get_mute_group() const
{
	return __mute_group;
}

inline int Instrument::get_midi_out_channel() const
{
	return __midi_out_channel;
}

inline void Instrument::set_midi_out_channel( int nChannel )
{
	if ( ( nChannel >= MIDI_OUT_CHANNEL_MIN ) &&
		 ( nChannel <= MIDI_OUT_CHANNEL_MAX ) ) {
		__midi_out_channel = nChannel;
	} else {
		ERRORLOG( QString( "midi out channel [%1] out of bounds [%2,%3]" )
				  .arg( nChannel )
				  .arg( MIDI_OUT_CHANNEL_MIN )
				  .arg( MIDI_OUT_CHANNEL_MAX ) );
	}
}

inline int Instrument::get_midi_out_note() const
{
	return __midi_out_note;
}

inline void Instrument::set_midi_out_note( int note )
{
	if ( ( note >= MIDI_OUT_NOTE_MIN ) && ( note <= MIDI_OUT_NOTE_MAX ) ) {
		__midi_out_note = note;
	} else {
		ERRORLOG( QString( "midi out note %1 out of bounds" ).arg( note ) );
	}
}

inline void Instrument::set_muted( bool muted )
{
	__muted = muted;
}

inline bool Instrument::is_muted() const
{
	return __muted;
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

inline void Instrument::set_gain( float gain )
{
	__gain = gain;
}

inline float Instrument::get_gain() const
{
	return __gain;
}

inline void Instrument::set_volume( float volume )
{
	__volume = volume;
}

inline float Instrument::get_volume() const
{
	return __volume;
}

inline void Instrument::set_filter_active( bool active )
{
	__filter_active = active;
}

inline bool Instrument::is_filter_active() const
{
	return __filter_active;
}

inline void Instrument::set_filter_resonance( float val )
{
	__filter_resonance = val;
}

inline float Instrument::get_filter_resonance() const
{
	return __filter_resonance;
}

inline void Instrument::set_filter_cutoff( float val )
{
	__filter_cutoff = val;
}

inline float Instrument::get_filter_cutoff() const
{
	return __filter_cutoff;
}

inline void Instrument::set_peak_l( float val )
{
	__peak_l = val;
}

inline float Instrument::get_peak_l() const
{
	return __peak_l;
}

inline void Instrument::set_peak_r( float val )
{
	__peak_r = val;
}

inline float Instrument::get_peak_r() const
{
	return __peak_r;
}

inline void Instrument::set_fx_level( float level, int index )
{
	__fx_level[index] = level;
}

inline float Instrument::get_fx_level( int index ) const
{
	return __fx_level[index];
}

inline void Instrument::set_random_pitch_factor( float val )
{
	__random_pitch_factor = val;
}

inline float Instrument::get_random_pitch_factor() const
{
	return __random_pitch_factor;
}

inline float Instrument::get_pitch_offset() const
{
	return __pitch_offset;
}

inline void Instrument::set_active( bool active )
{
	__active = active;
}

inline bool Instrument::is_active() const
{
	return __active;
}

inline void Instrument::set_soloed( bool soloed )
{
	__soloed = soloed;
}

inline bool Instrument::is_soloed() const
{
	return __soloed;
}

inline void Instrument::enqueue()
{
	__queued++;
}

inline void Instrument::dequeue()
{
	assert( __queued > 0 );
	__queued--;
}

inline bool Instrument::is_queued() const
{
	return ( __queued > 0 );
}

inline void Instrument::set_stop_notes( bool stopnotes )
{
	__stop_notes = stopnotes;
}

inline bool Instrument::is_stop_notes() const
{
	return __stop_notes;
}

inline void Instrument::set_sample_selection_alg( SampleSelectionAlgo selected_algo)
{
	__sample_selection_alg = selected_algo;
}

inline Instrument::SampleSelectionAlgo Instrument::sample_selection_alg() const
{
	return __sample_selection_alg;
}

inline void Instrument::set_hihat_grp( int hihat_grp )
{
	__hihat_grp = hihat_grp;
}

inline int Instrument::get_hihat_grp() const
{
	return __hihat_grp;
}

inline void Instrument::set_lower_cc( int message )
{
	__lower_cc = message;
}

inline int Instrument::get_lower_cc() const
{
	return __lower_cc;
}

inline void Instrument::set_higher_cc( int message )
{
	__higher_cc = message;
}

inline int Instrument::get_higher_cc() const
{
	return __higher_cc;
}

inline void Instrument::set_drumkit_path( const QString& sPath )
{
	__drumkit_path = sPath;
}

inline void Instrument::set_drumkit_name( const QString& sName )
{
	__drumkit_name = sName;
}

inline const QString& Instrument::get_drumkit_name() const
{
	return __drumkit_name;
}

inline bool Instrument::is_preview_instrument() const
{
	return __is_preview_instrument;
}

inline void Instrument::set_is_preview_instrument(bool isPreview)
{
	__is_preview_instrument = isPreview;
}

inline bool Instrument::is_metronome_instrument() const
{
	return __is_metronome_instrument;
}

inline void Instrument::set_is_metronome_instrument(bool isMetronome)
{
	__is_metronome_instrument = isMetronome;
}

inline std::vector<std::shared_ptr<InstrumentComponent>>* Instrument::get_components()
{
	return __components;
}

inline void Instrument::set_apply_velocity( bool apply_velocity )
{
	__apply_velocity = apply_velocity;
}

inline bool Instrument::get_apply_velocity() const
{
	return __apply_velocity;
}

inline bool Instrument::is_currently_exported() const
{
	return __current_instr_for_export;
}

inline void Instrument::set_currently_exported( bool isCurrentlyExported )
{
	__current_instr_for_export = isCurrentlyExported;
}

};



#endif // H2C_INSTRUMENT_H

/* vim: set softtabstop=4 noexpandtab:  */
