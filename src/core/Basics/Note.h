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

#ifndef H2C_NOTE_H
#define H2C_NOTE_H

#include <memory>

#include <core/Object.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/Sample.h>

#include <core/IO/MidiCommon.h>

#define KEY_MIN                 0
#define KEY_MAX                 11
#define OCTAVE_MIN              -3
#define OCTAVE_MAX              3
#define OCTAVE_OFFSET           3
#define OCTAVE_DEFAULT          0
#define KEYS_PER_OCTAVE         12

#define MIDI_MIDDLE_C           60
#define MIDI_FACTOR             127

#define VELOCITY_MIN            0.0f
#define VELOCITY_MAX            1.0f
#define PAN_MIN                 0.0f
#define PAN_MAX                 0.5f
#define LEAD_LAG_MIN            -1.0f
#define LEAD_LAG_MAX            1.0f

namespace H2Core
{

class XMLNode;
class ADSR;
class Instrument;
class InstrumentList;

/** Auxiliary variables storing the rendering state of a #H2Core::Note within
 * the #H2Core::Sampler */
struct SelectedLayerInfo {
	/** Selected layer during rendering in the #H2Core::Sampler.
	 *
	 * If set to -1 (during creation), Sampler::renderNote() will determine
	 * which layer to use and overrides this variable with the corresponding
	 * value. */
	int nSelectedLayer;

	/** Stores the frame till which the #H2Core::Sample of the selected
	 * #H2Core::InstrumentLayer using #nSelectedLayer was already rendered. If
	 * several cycles of #Sampler::renderNote() are required, this variable
	 * corresponds to the starting point of each cycle.
	 *
	 * It is given in float instead of int/long - what one might expect when
	 * talking about frames - since it also serves as the fraction of the
	 * #H2Core::Sample already processed in case it has to be resampled. */
	float fSamplePosition;

	/** Frame / #fSamplePosition at which rendering of the current note is
	 * considered done.
	 *
	 * For regular notes this is the number of frames of the #H2Core::Sample
	 * corresponding to #nSelectedLayer.
	 *
	 * If, however, the user specifies the length of a note things are more
	 * complex. The length is specified in the GUI in **ticks** and this
	 * variable is the corresponding value in frames. Now, whenever manually
	 * adjusting the tempo or adding/deleting a tempo marker the length of the
	 * note in frames differs for the new speed. In case rendering did already
	 * started, it is important to not rescale the whole length of the note but
	 * just the fraction between #fSamplePosition and the former #nNoteLength.*/
	int nNoteLength;
};

/**
 * A note plays an associated instrument with a velocity left and right pan
 */
/** \ingroup docCore docDataStructure */
class Note : public H2Core::Object<Note>
{
		H2_OBJECT(Note)
	public:
		/** possible keys */
		enum Key { C=KEY_MIN, Cs, D, Ef, E, F, Fs, G, Af, A, Bf, B };
	static QString KeyToQString( Key key );
	
		/** possible octaves */
		enum Octave { P8Z=-3, P8Y=-2, P8X=-1, P8=OCTAVE_DEFAULT, P8A=1, P8B=2, P8C=3 };

		/**
		 * constructor
		 *
		 * \param pInstrument the instrument played by this note
		 * \param nPosition the position of the note within the pattern
		 * \param fVelocity it's velocity
		 * \param fFan pan
		 * \param nLength Length of the note in frames. If set to -1,
		 * the length of the #H2Core::Sample used during playback will
		 * be used instead.
		 * \param fPitch it's pitch
		 */
	Note( std::shared_ptr<Instrument> pInstrument, int nPosition = 0, float fVelocity = 0.8, float fPan = 0.0, int nLength = -1, float fPitch = 0.0 );

		/**
		 * copy constructor with an optional parameter
		 * \param pOther 
		 * \param pInstrument if set will be used as note instrument
		 */
		Note( Note* pOther, std::shared_ptr<Instrument> pInstrument = nullptr );
		/** destructor */
		~Note();

		/*
		 * save the note within the given XMLNode
		 * \param node the XMLNode to feed
		 */
		void save_to( XMLNode* node );
		/**
		 * load a note from an XMLNode
		 * \param node the XMLDode to read from
		 * \param instruments the current instrument list to search instrument into
		 * \param bSilent Whether infos, warnings, and errors should
		 * be logged.
		 * \return a new Note instance
		 */
	static Note* load_from( XMLNode* node, std::shared_ptr<InstrumentList> instruments, bool bSilent = false );

		/**
		 * find the corresponding instrument and point to it, or an empty instrument
		 * \param instruments the list of instrument to look into
		 */
		void map_instrument( std::shared_ptr<InstrumentList> instruments );
		/** #__instrument accessor */
		std::shared_ptr<Instrument> get_instrument();
		/** return true if #__instrument is set */
		bool has_instrument() const;
		/**
		 * #__instrument_id setter
		 * \param value the new value
		 */
		void set_instrument_id( int value );
		/** #__instrument_id accessor */
		int get_instrument_id() const;
		/**
		 * #__specific_compo_id setter
		 * \param value the new value
		 */
		void set_specific_compo_id( int value );
		/** #__specific_compo_id accessor */
		int get_specific_compo_id() const;
		/**
		 * #__position setter
		 * \param value the new value
		 */
		void set_position( int value );
		/** #__position accessor */
		int get_position() const;
		/**
		 * #__velocity setter
		 * \param value the new value
		 */
		void set_velocity( float value );
		/** #__velocity accessor */
		float get_velocity() const;
		
		/** set pan of the note. assumes the input range in [-1;1]*/
		void setPan( float val );
		/** set pan of the note, assuming the input range in [0;1] */
		void setPanWithRangeFrom0To1( float fVal ) {
			this->setPan( -1.f + 2.f * fVal ); // scale and translate into [-1;1]
		};
		/** get pan of the note. Output pan range: [-1;1] */
		float getPan() const;
		/** get pan of the note, scaling and translating the range from [-1;1] to [0;1] */
		float getPanWithRangeFrom0To1() const {
			return 0.5f * ( 1.f + m_fPan );
		}

		/**
		 * #__lead_lag setter
		 * \param value the new value
		 */
		void set_lead_lag( float value );
		/** #__lead_lag accessor */
		float get_lead_lag() const;
		/**
		 * #__length setter
		 * \param value the new value
		 */
		void set_length( int value );
		/** #__length accessor */
		int get_length() const;
		/**
		 * #__pitch setter
		 * \param value the new value
		 */
		void set_pitch( float value );
		/** #__pitch accessor */
		float get_pitch() const;
		/**
		 * #__note_off setter
		 * \param value the new value
		 */
		void set_note_off( bool value );
		/** #__note_off accessor */
		bool get_note_off() const;
		/** #__midi_msg accessor */
		int get_midi_msg() const;
		/**
		 * #__pattern_idx setter
		 * \param value the new value
		 */
		void set_pattern_idx( int value );
		/** #__pattern_idx accessor */
		int get_pattern_idx() const;
		/**
		 * #__just_recorded setter
		 * \param value the new value
		 */
		void set_just_recorded( bool value );
		/** #__just_recorded accessor */
		bool get_just_recorded() const;

		/*
		 * selected sample
		 * */
	std::shared_ptr<SelectedLayerInfo> get_layer_selected( int CompoID );
	std::map<int, std::shared_ptr<SelectedLayerInfo>> get_layers_selected() const;


		void set_probability( float value );
		float get_probability() const;

		/**
		 * #__humanize_delay setter
		 * \param value the new value
		 */
		void set_humanize_delay( int value );
		/** #__humanize_delay accessor */
		int get_humanize_delay() const;
		/** #__cut_off accessor */
		float get_cut_off() const;
		/** #__resonance accessor */
		float get_resonance() const;
		/** #__bpfb_l accessor */
		float get_bpfb_l() const;
		/** #__bpfb_r accessor */
		float get_bpfb_r() const;
		/** #__lpfb_l accessor */
		float get_lpfb_l() const;
		/** #__lpfb_r accessor */
		float get_lpfb_r() const;
		/** Filter output is sustaining note */
		bool filter_sustain() const;
		/** #__key accessor */
		Key get_key();
		/** #__octave accessor */
		Octave get_octave();
		/** return scaled key for midi output, !!! DO NOT CHECK IF INSTRUMENT IS SET !!! */
		int get_midi_key() const;
		/** midi velocity accessor 
		 * \code{.cpp}
		 * __velocity * #MIDI_FACTOR
		 * \endcode */
		int get_midi_velocity() const;
		/** note key pitch accessor
		 * \code{.cpp}
		 * __octave * KEYS_PER_OCTAVE + __key
		 * \endcode */
		float get_notekey_pitch() const;
	/**
	 *
	 * @returns
	 * \code{.cpp}
	 * __octave * 12 + __key + __pitch + __instrument->get_pitch_offset()
	 * \endcode*/ 
		float get_total_pitch() const;

		/** return a string representation of key-octave */
		QString key_to_string();
		/**
		 * parse str and set #__key and #__octave
		 * \param str the string to be parsed
		 */
		void set_key_octave( const QString& str );
		/**
		 * set #__key and #__octave only if within acceptable range
		 * \param key the key to set
		 * \param octave the octave to be set
		 */
		void set_key_octave( Key key, Octave octave );
		/**
		 * set #__key, #__octave and #__midi_msg only if within acceptable range
		 * \param key the key to set
		 * \param octave the octave to be set
		 * \param msg
		 */
		void set_midi_info( Key key, Octave octave, int msg );

		/** get the ADSR of the note */
		std::shared_ptr<ADSR> get_adsr() const;

		/** return true if instrument, key and octave matches with internal
		 * \param instrument the instrument to match with #__instrument
		 * \param key the key to match with #__key
		 * \param octave the octave to match with #__octave
		 */
		bool match( std::shared_ptr<Instrument> instrument, Key key, Octave octave ) const;

		/** Return true if two notes match in instrument, key and octave. */
		bool match( const Note *pNote ) const;
		bool match( const std::shared_ptr<Note> pNote ) const;

		/**
		 * compute left and right output based on filters
		 * \param val_l the left channel value
		 * \param val_r the right channel value
		 */
		void compute_lr_values( float* val_l, float* val_r );

	long long getNoteStart() const;
	float getUsedTickSize() const;

	/** 
	 * @return true if the #Sampler already started rendering this
	 * note.
	 */
	bool isPartiallyRendered() const;

	/**
	 * Calculates the #m_nNoteStart in frames corresponding to the
	 * #__position in ticks and storing the used tick size in
	 * #m_fUsedTickSize.
	 *
	 * Whenever the tempo changes and the #Timeline is not
	 * enabled, the #m_nNoteStart gets invalidated and this function
	 * needs to be rerun.
	 */
	void computeNoteStart();

	/**
	 * Add random contributions to #__pitch, #__humanize_delay, and
	 * #__velocity.
	 */
	void humanize();

	/**
	 * Add swing contribution to #__humanize_delay.
	 *
	 * As the value applied is deterministic, it will not be handled
	 * in humanice() but separately.
	 */
	void swing();
	
		/** Formatted string version for debugging purposes.
		 * \param sPrefix String prefix which will be added in front of
		 * every new line
		 * \param bShort Instead of the whole content of all classes
		 * stored as members just a single unique identifier will be
		 * displayed without line breaks.
		 *
		 * \return String presentation of current object.*/
		QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

		/** Convert a logarithmic pitch-space value in semitones to a frequency-domain value */
		static inline double pitchToFrequency( double fPitch ) {
			// Equivalent to, but quicker to compute than, pow( 2.0, ( fPitch/12 ) )
			return pow( 1.0594630943593, fPitch );
		}

		static inline Octave pitchToOctave( int nPitch ) {
			if ( nPitch >= 0 ) {
				return (Octave)(nPitch / 12);
			} else {
				return (Octave)((nPitch-11) / 12);
			}
		}
		static inline Key pitchToKey( int nPitch ) {
			return (Key)(nPitch - 12 * pitchToOctave( nPitch ));
		}
		static inline int octaveKeyToPitch( Octave octave, Key key ) {
			return 12 * (int)octave + (int)key;
		}

	/**
	 * Returns the sample associated with the note for a specific
	 * InstrumentComponent @a nComponentID.
	 *
	 * A sample of the InstrumentComponent is a possible candidate if
	 * the note velocity falls within the start and end velocity of an
	 * InstrumentLayer. In case multiple samples are possible the
	 * function will either pick the provided @a nSelectedLayer or -
	 * for @a nSelectedLayer == -1 - the selection algorithm stored in
	 * #__instrument to determined a layer.
	 *
	 * The function stores the selected layer in #__layers_selected
	 * and will reuse this parameter in every following call while
	 * disregarding the provided @a nSelectedLayer.
	 */
	std::shared_ptr<Sample> getSample( int nComponentID, int nSelectedLayer = -1 );

	private:
		std::shared_ptr<Instrument>		__instrument;   ///< the instrument to be played by this note
		int				__instrument_id;        ///< the id of the instrument played by this note
		int				__specific_compo_id;    ///< play a specific component, -1 if playing all
		int				__position;             ///< note position in
												///ticks inside the pattern
		float			__velocity;           ///< velocity (intensity) of the note [0;1]
		float			m_fPan;		///< pan of the note, [-1;1] from
									///left to right, as requested by
									///Sampler PanLaws
	/** Length of the note in frames.
	 *
	 * If set to -1, the Note will be rendered till the end of all
	 * contained Samples is reached.
	 */
		int				__length;               ///< the length of the note
		float			__pitch;              ///< the frequency of the note
		Key				__key;                  ///< the key, [0;11]==[C;B]
		Octave			 __octave;            ///< the octave [-3;3]
		std::shared_ptr<ADSR>			__adsr;               ///< attack decay sustain release
		float			__lead_lag;           ///< lead or lag offset of the note
		float			__cut_off;            ///< filter cutoff [0;1]
		float			__resonance;          ///< filter resonant
											  ///frequency [0;1]
		/** Offset of the note start in frames.
		 * 
		 * It includes contributions of the onset humanization, the
		 * lead lag factor, and the swing. For some of these a random
		 * value will be drawn but once stored in this variable, the
		 * delay is fixed and will not change anymore.
		 *
		 * It is incorporated in the #m_nNoteStart.
		 */
		int				__humanize_delay;
	std::map< int, std::shared_ptr<SelectedLayerInfo>> __layers_selected;
		float			__bpfb_l;             ///< left band pass filter buffer
		float			__bpfb_r;             ///< right band pass filter buffer
		float			__lpfb_l;             ///< left low pass filter buffer
		float			__lpfb_r;             ///< right low pass filter buffer
		int				__pattern_idx;          ///< index of the pattern holding this note for undo actions
		int				__midi_msg;             ///< TODO
		bool			__note_off;            ///< note type on|off
		bool			__just_recorded;       ///< used in record+delete
		float			__probability;        ///< note probability
		static const char* __key_str[]; ///< used to build QString
										///from #__key an #__octave
	/**
	 * Onset of the note in frames.
	 *
	 * This member is only used by the #AudioEngine and #Sampler
	 * during processing and not written to disk.
	*/
	long long m_nNoteStart;
	/**
	 * TransportPosition::m_fTickSize used to calculate #m_nNoteStart.
	 *
	 * If #m_nNoteStart was calculated in the presence of an active
	 * #Timeline, it will be set to -1.
	 *
	 * Used to check whether the note start has to be rescaled because
	 * of a change in speed (which occurs less often and is faster
	 * than recalculating #m_nNoteStart everywhere it is required.)
	 *
	 * This member is only used by the #AudioEngine and #Sampler
	 * during processing and not written to disk.
	 */
	float m_fUsedTickSize;
};

// DEFINITIONS

inline std::shared_ptr<ADSR> Note::get_adsr() const
{
	return __adsr;
}

inline std::shared_ptr<Instrument> Note::get_instrument()
{
	return __instrument;
}

inline bool Note::has_instrument() const
{
	return __instrument!=nullptr;
}

inline void Note::set_instrument_id( int value )
{
	__instrument_id = value;
}

inline int Note::get_instrument_id() const
{
	return __instrument_id;
}

inline void Note::set_specific_compo_id( int value )
{
	__specific_compo_id = value;
}

inline int Note::get_specific_compo_id() const
{
	return __specific_compo_id;
}

inline void Note::set_position( int value )
{
	__position = value;
}

inline int Note::get_position() const
{
	return __position;
}

inline float Note::get_velocity() const
{
	return __velocity;
}

inline float Note::getPan() const
{
	return m_fPan;
}

inline float Note::get_lead_lag() const
{
	return __lead_lag;
}

inline void Note::set_length( int value )
{
	__length = value;
}

inline int Note::get_length() const
{
	return __length;
}

inline void Note::set_pitch( float value )
{
	__pitch = value;
}

inline float Note::get_pitch() const
{
	return __pitch;
}

inline void Note::set_note_off( bool value )
{
	__note_off = value;
}

inline bool Note::get_note_off() const
{
	return __note_off;
}

inline int Note::get_midi_msg() const
{
	return __midi_msg;
}

inline void Note::set_pattern_idx( int value )
{
	__pattern_idx = value;
}

inline int Note::get_pattern_idx() const
{
	return __pattern_idx;
}

inline void Note::set_just_recorded( bool value )
{
	__just_recorded = value;
}

inline bool Note::get_just_recorded() const
{
	return __just_recorded;
}

inline float Note::get_probability() const
{
	return __probability;
}

inline void Note::set_probability( float value )
{
	__probability = value;
}

inline std::shared_ptr<SelectedLayerInfo> Note::get_layer_selected( int CompoID )
{
	return __layers_selected[ CompoID ];
}

inline std::map<int, std::shared_ptr<SelectedLayerInfo>> Note::get_layers_selected() const
{
	return __layers_selected;
}

inline int Note::get_humanize_delay() const
{
	return __humanize_delay;
}

inline float Note::get_cut_off() const
{
	return __cut_off;
}

inline float Note::get_resonance() const
{
	return __resonance;
}

inline float Note::get_bpfb_l() const
{
	return __bpfb_l;
}

inline float Note::get_bpfb_r() const
{
	return __bpfb_r;
}

inline float Note::get_lpfb_l() const
{
	return __lpfb_l;
}

inline float Note::get_lpfb_r() const
{
	return __lpfb_r;
}

inline bool Note::filter_sustain() const
{
	const double fLimit = 0.001;
	return ( fabs( __lpfb_l ) > fLimit || fabs( __lpfb_r ) > fLimit ||
			 fabs( __bpfb_l ) > fLimit || fabs( __bpfb_r ) > fLimit );
}

inline Note::Key Note::get_key()
{
	return __key;
}

inline Note::Octave Note::get_octave()
{
	return __octave;
}

inline int Note::get_midi_key() const
{
	/* TODO ???
	if( !has_instrument() ) { return (__octave + OCTAVE_OFFSET ) * KEYS_PER_OCTAVE + __key; }
	*/
	return ( __octave + OCTAVE_OFFSET ) * KEYS_PER_OCTAVE + __key +
		__instrument->get_midi_out_note() - MidiMessage::instrumentOffset;
}

inline int Note::get_midi_velocity() const
{
	return __velocity * MIDI_FACTOR;
}

inline float Note::get_notekey_pitch() const
{
	return __octave * KEYS_PER_OCTAVE + __key;
}

inline void Note::set_key_octave( Key key, Octave octave )
{
	if( key>=KEY_MIN && key<=KEY_MAX ) __key = key;
	if( octave>=OCTAVE_MIN && octave<=OCTAVE_MAX ) __octave = octave;
}

inline void Note::set_midi_info( Key key, Octave octave, int msg )
{
	if( key>=KEY_MIN && key<=KEY_MAX ) __key = key;
	if( octave>=OCTAVE_MIN && octave<=OCTAVE_MAX ) __octave = octave;
	__midi_msg = msg;
}

inline bool Note::match( std::shared_ptr<Instrument> instrument, Key key, Octave octave ) const
{
	return ( ( __instrument==instrument ) && ( __key==key ) && ( __octave==octave ) );
}

inline bool Note::match( const Note *pNote ) const
{
	return match( pNote->__instrument, pNote->__key, pNote->__octave );
}
inline bool Note::match( const std::shared_ptr<Note> pNote ) const
{
	return match( pNote->__instrument, pNote->__key, pNote->__octave );
}

inline void Note::compute_lr_values( float* val_l, float* val_r )
{
	/* TODO ???
	if( !has_instrument() ) {
		*val_l = 0.0f;
		*val_r = 0.0f;
		return;
	}
	*/
	float cut_off = __instrument->get_filter_cutoff();
	float resonance = __instrument->get_filter_resonance();
	__bpfb_l  =  resonance * __bpfb_l  + cut_off * ( *val_l - __lpfb_l );
	__lpfb_l +=  cut_off   * __bpfb_l;
	__bpfb_r  =  resonance * __bpfb_r  + cut_off * ( *val_r - __lpfb_r );
	__lpfb_r +=  cut_off   * __bpfb_r;
	*val_l = __lpfb_l;
	*val_r = __lpfb_r;
}

inline long long Note::getNoteStart() const {
	return m_nNoteStart;
}
inline float Note::getUsedTickSize() const {
	return m_fUsedTickSize;
}
};

#endif // H2C_NOTE_H

/* vim: set softtabstop=4 noexpandtab: */
