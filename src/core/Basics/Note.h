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

#ifndef H2C_NOTE_H
#define H2C_NOTE_H

#include <memory>

#include <core/Object.h>
#include <core/Basics/DrumkitMap.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/Sample.h>

#include <core/IO/MidiCommon.h>

#define KEY_MIN                 0
#define KEY_MAX                 11
#define KEY_INVALID             666
#define OCTAVE_MIN              -3 /* C-1 */
#define OCTAVE_MAX              3 /* C5 */
#define OCTAVE_OFFSET           3
#define OCTAVE_DEFAULT          0
#define OCTAVE_NUMBER           7
#define OCTAVE_INVALID          666
#define KEYS_PER_OCTAVE         12

#define VELOCITY_MIN            0.0f
#define VELOCITY_DEFAULT        0.8f
#define VELOCITY_MAX            1.0f
#define PAN_MIN                 -1.0f
#define PAN_DEFAULT             0.0f
#define PAN_MAX                 1.0f
#define LEAD_LAG_MIN            -1.0f
#define LEAD_LAG_DEFAULT        0.0f
#define LEAD_LAG_MAX            1.0f
#define LENGTH_ENTIRE_SAMPLE    -1
#define PITCH_DEFAULT           0.0f /* C2 */
#define PITCH_INVALID           666
#define PROBABILITY_MIN         0.0f
#define PROBABILITY_DEFAULT     1.0f
#define PROBABILITY_MAX         1.0f

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

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const;
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
	static QString KeyToQString( const Key& key );
	
		/** possible octaves */
		enum Octave { P8Z=-3, P8Y=-2, P8X=-1, P8=OCTAVE_DEFAULT, P8A=1, P8B=2, P8C=3 };
		static QString OctaveToQString( const Octave& octave );

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
	Note( std::shared_ptr<Instrument> pInstrument, int nPosition = 0,
		  float fVelocity = VELOCITY_DEFAULT, float fPan = PAN_DEFAULT,
		  int nLength = LENGTH_ENTIRE_SAMPLE, float fPitch = PITCH_DEFAULT );

		/**
		 * copy constructor with an optional parameter
		 * \param pOther 
		 * \param pInstrument if set will be used as note instrument
		 */
		Note( std::shared_ptr<Note> pOther,
			  std::shared_ptr<Instrument> pInstrument = nullptr );
		/** destructor */
		~Note();

		/*
		 * save the note within the given XMLNode
		 * \param node the XMLNode to feed
		 */
		void saveTo( XMLNode& node ) const;
		/**
		 * load a note from an XMLNode
		 * \param node the XMLDode to read from
		 * \param bSilent Whether infos, warnings, and errors should
		 * be logged.
		 * \return a new Note instance
		 */
	static std::shared_ptr<Note> loadFrom( const XMLNode& node,
											bool bSilent = false );

		/**
		 * Make the current Note work with the provided drumkit @a pDrumkit.
		 *
		 * \param pDrumkit Most likely the currently used kit.
		 * \param pOldDrumkit Optionally, the former kit the note was mapped to.
		 */
		void mapTo( std::shared_ptr<Drumkit> pDrumkit,
					std::shared_ptr<Drumkit> pOldDrumkit = nullptr );
		/** #m_pInstrument accessor */
		std::shared_ptr<Instrument> getInstrument() const;
		/**
		 * #m_nInstrumentId setter
		 * \param value the new value
		 */
		void setInstrumentId( int value );
		/** #m_nInstrumentId accessor */
		int getInstrumentId() const;

		void setType( DrumkitMap::Type sType );
		DrumkitMap::Type getType() const;

		void setSpecificCompoIdx( int value );
		int getSpecificCompoIdx() const;
		/**
		 * #m_nPosition setter
		 * \param value the new value
		 */
		void setPosition( int value );
		/** #m_nPosition accessor */
		int getPosition() const;
		/**
		 * #m_fVelocity setter
		 * \param value the new value
		 */
		void setVelocity( float value );
		/** #m_fVelocity accessor */
		float getVelocity() const;
		
		/** set pan of the note. assumes the input range in [-1;1]*/
		void setPan( float val );
		/** set pan of the note, assuming the input range in [0;1] */
		void setPanWithRangeFrom0To1( float fVal ) {
			// scale and translate into [-1;1]
			this->setPan( PAN_MIN + ( PAN_MAX - PAN_MIN ) * fVal );
		};
		/** get pan of the note. Output pan range: [-1;1] */
		float getPan() const;
		/** get pan of the note, scaling and translating the range from [-1;1] to [0;1] */
		float getPanWithRangeFrom0To1() const {
			return 0.5f * ( 1.f + m_fPan );
		}

		/**
		 * #m_fLeadLag setter
		 * \param value the new value
		 */
		void setLeadLag( float value );
		/** #m_fLeadLag accessor */
		float getLeadLag() const;
		/**
		 * #m_nLength setter
		 * \param value the new value
		 */
		void setLength( int value );
		/** #m_nLength accessor */
		int getLength() const;
		/** #m_fPitch accessor */
		float getPitch() const;
		/**
		 * #m_bNoteOff setter
		 * \param value the new value
		 */
		void setNoteOff( bool value );
		/** #m_bNoteOff accessor */
		bool getNoteOff() const;
		/** #m_nMidiMsg accessor */
		int getMidiMsg() const;

	std::shared_ptr<SelectedLayerInfo> getLayerSelected( int nIdx ) const;

		void setProbability( float value );
		float getProbability() const;

		/**
		 * #m_nHumanizeDelay setter
		 * \param value the new value
		 */
		void setHumanizeDelay( int value );
		/** #m_nHumanizeDelay accessor */
		int getHumanizeDelay() const;
		/** Filter output is sustaining note */
		bool filterSustain() const;
		/** #m_key accessor */
		Key getKey() const;
		/** #m_octave accessor */
		Octave getOctave() const;
		/** return scaled key for midi output, !!! DO NOT CHECK IF INSTRUMENT IS SET !!! */
		int getMidiKey() const;
		/** midi velocity accessor 
		 * m_fVelocity * 127
		 * \endcode */
		int getMidiVelocity() const;
		/** note key pitch accessor
		 * \code{.cpp}
		 * m_octave * KEYS_PER_OCTAVE + m_key
		 * \endcode */
		float getPitchFromKeyOctave() const;
		float getTotalPitch() const;

		/**
		 * parse str and set #m_key and #m_octave
		 * \param str the string to be parsed
		 */
		void setKeyOctave( const QString& str );
		/**
		 * set #m_key and #m_octave only if within acceptable range
		 * \param key the key to set
		 * \param octave the octave to be set
		 */
		void setKeyOctave( Key key, Octave octave );
		/**
		 * set #m_key, #m_octave and #m_nMidiMsg only if within acceptable range
		 * \param key the key to set
		 * \param octave the octave to be set
		 * \param msg
		 */
		void setMidiInfo( Key key, Octave octave, int msg );

		/** get the ADSR of the note */
		std::shared_ptr<ADSR> getAdsr() const;

		/** @returns true if instrument id and type as well as key and octave
		 * matches with internal
		 *
		 * \param nInstrumentId the instrument ID to match with #m_nInstrumentId
		 * \param sInstrumentType the instrument type to match with #m_sType
		 * \param key the key to match with #m_key
		 * \param octave the octave to match with #m_octave
		 */
		bool match( int nInstrumentId, const QString& sType, Key key,
					Octave octave ) const;

		/** Return true if two notes match in instrument, key and octave. */
		bool match( const std::shared_ptr<Note> pNote ) const;

		/** Compares two notes based on position (primary) and pitch
		 * (secundary).
		 *
		 * In contrast to compareStart() position in here only takes the actual
		 * #m_nPosition into account. Neither humanization nor lead/lag.
		 *
		 * @returns `true` in case @a pNote1 wins (larger position, larger
		 *   pitch) compared to @a pNote2. */
		static bool compare( const std::shared_ptr<Note> pNote1,
							 const std::shared_ptr<Note> pNote2 );

		/** Performs a comparison based on starting position of the note.
		 *
		 * Note that this start has to be calculated first and involves random
		 * components (humanization). It is thus something to be performed on
		 * notes already enqueued in #AudioEngine or #Sampler and not suitable
		 * for plain notes residing in #Pattern.
		 *
		 * @returns `true` in case @a pNote1 has a bigger starting position as
		 *   @a pNote2. */
		static bool compareStart( const std::shared_ptr<Note> pNote1,
								  const std::shared_ptr<Note> pNote2 );

		/**
		 * compute left and right output based on filters
		 * \param val_l the left channel value
		 * \param val_r the right channel value
		 */
		void computeLrValues( float* val_l, float* val_r );

	long long getNoteStart() const;
	float getUsedTickSize() const;

	/** 
	 * @return true if the #Sampler already started rendering this
	 * note.
	 */
	bool isPartiallyRendered() const;

	/**
	 * Calculates the #m_nNoteStart in frames corresponding to the
	 * #m_nPosition in ticks and storing the used tick size in
	 * #m_fUsedTickSize.
	 *
	 * Whenever the tempo changes and the #Timeline is not
	 * enabled, the #m_nNoteStart gets invalidated and this function
	 * needs to be rerun.
	 */
	void computeNoteStart();

	/**
	 * Add random contributions to #m_fPitch, #m_nHumanizeDelay, and
	 * #m_fVelocity.
	 */
	void humanize();

	/**
	 * Add swing contribution to #m_nHumanizeDelay.
	 *
	 * As the value applied is deterministic, it will not be handled
	 * in humanice() but separately.
	 */
	void swing();

		/** Returns a short but expressive string using which the particular
		 * note instance can be identified.
		 *
		 * Note that the ability to uniquely identify the note is only ensured
		 * if no identical notes exist in any pattern (de-duplication). */
		QString prettyName() const;
	
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
				return (Octave)( nPitch / KEYS_PER_OCTAVE );
			} else {
				return (Octave)((nPitch-11) / KEYS_PER_OCTAVE );
			}
		}
		static inline Key pitchToKey( int nPitch ) {
			return (Key)(nPitch - KEYS_PER_OCTAVE * pitchToOctave( nPitch ));
		}
		static inline int octaveKeyToPitch( Octave octave, Key key ) {
			return KEYS_PER_OCTAVE * (int)octave + (int)key;
		}

		/** Pitch / line conversions used in GUI. */
		static int lineToPitch( int nLine ) {
			return KEYS_PER_OCTAVE * ( OCTAVE_MIN + OCTAVE_NUMBER ) - 1 - nLine;
		}
		static int pitchToLine( int nPitch ) {
			return KEYS_PER_OCTAVE * ( OCTAVE_MIN + OCTAVE_NUMBER ) - 1 - nPitch;
		}

	/**
	 * Returns the sample associated with the note for a specific
	 * InstrumentComponent @a nComponentIdx.
	 *
	 * A sample of the InstrumentComponent is a possible candidate if
	 * the note velocity falls within the start and end velocity of an
	 * InstrumentLayer. In case multiple samples are possible the
	 * function will either pick the provided @a nSelectedLayer or -
	 * for @a nSelectedLayer == -1 - the selection algorithm stored in
	 * #m_pInstrument to determined a layer.
	 *
	 * The function stores the selected layer in #m_layersSelected
	 * and will reuse this parameter in every following call while
	 * disregarding the provided @a nSelectedLayer.
	 */
	std::shared_ptr<Sample> getSample( int nComponentIdx, int nSelectedLayer = -1 ) const;

	private:
		int				m_nInstrumentId;        ///< the id of the instrument played by this note
		/** Drumkit-independent identifier used to relate a note/pattern to a
		 * different kit */
		DrumkitMap::Type m_sType;
		int				m_nPosition;             ///< note position in
												///ticks inside the pattern
		float			m_fVelocity;           ///< velocity (intensity) of the note [0;1]
		float			m_fPan;		///< pan of the note, [-1;1] from
									///left to right, as requested by
									///Sampler PanLaws
	/** Length of the note in frames.
	 *
	 * If set to -1, the Note will be rendered till the end of all
	 * contained Samples is reached.
	 */
		int				m_nLength;               ///< the length of the note
		float			m_fPitch;              ///< the frequency of the note
		Key				m_key;                  ///< the key, [0;11]==[C;B]
		Octave			 m_octave;            ///< the octave [-3;3]
		std::shared_ptr<ADSR>			m_pAdsr;               ///< attack decay sustain release
		float			m_fLeadLag;           ///< lead or lag offset of the note
		/** Offset of the note start in frames.
		 * 
		 * It includes contributions of the onset humanization, the
		 * lead lag factor, and the swing. For some of these a random
		 * value will be drawn but once stored in this variable, the
		 * delay is fixed and will not change anymore.
		 *
		 * It is incorporated in the #m_nNoteStart.
		 */
		int				m_nHumanizeDelay;
		float			m_fBpfbL;             ///< left band pass filter buffer
		float			m_fBpfbR;             ///< right band pass filter buffer
		float			m_fLpfbL;             ///< left low pass filter buffer
		float			m_fLpfbR;             ///< right low pass filter buffer
		int				m_nMidiMsg;             ///< TODO
		bool			m_bNoteOff;            ///< note type on|off
		float			m_fProbability;        ///< note probability
		static const char* m_keyStr[]; ///< used to build QString
										///from #m_key an #m_octave
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

		/** Play a specific component, -1 if playing all */
		int				m_nSpecificCompoIdx;

		/** One #SelectedLayerInfo for each #InstrumentComponent in
		 * #m_pInstrument. It assumes the same order as
		 * #Instrument::__components. */
	std::vector<std::shared_ptr<SelectedLayerInfo>> m_layersSelected;

		/** the instrument to be played by this note */
		std::shared_ptr<Instrument>		m_pInstrument;
};

// DEFINITIONS

inline std::shared_ptr<ADSR> Note::getAdsr() const
{
	return m_pAdsr;
}

inline std::shared_ptr<Instrument> Note::getInstrument() const
{
	return m_pInstrument;
}

inline void Note::setInstrumentId( int value )
{
	m_nInstrumentId = value;
}

inline int Note::getInstrumentId() const
{
	return m_nInstrumentId;
}

inline void Note::setType( DrumkitMap::Type sType ) {
	m_sType = sType;
}
inline DrumkitMap::Type Note::getType() const {
	return m_sType;
}

inline void Note::setSpecificCompoIdx( int value )
{
	m_nSpecificCompoIdx = value;
}

inline int Note::getSpecificCompoIdx() const
{
	return m_nSpecificCompoIdx;
}

inline void Note::setPosition( int value )
{
	m_nPosition = value;
}

inline int Note::getPosition() const
{
	return m_nPosition;
}

inline float Note::getVelocity() const
{
	return m_fVelocity;
}

inline float Note::getPan() const
{
	return m_fPan;
}

inline float Note::getLeadLag() const
{
	return m_fLeadLag;
}

inline void Note::setLength( int value )
{
	m_nLength = value;
}

inline int Note::getLength() const
{
	return m_nLength;
}

inline float Note::getPitch() const
{
	return m_fPitch;
}

inline void Note::setNoteOff( bool value )
{
	m_bNoteOff = value;
}

inline bool Note::getNoteOff() const
{
	return m_bNoteOff;
}

inline int Note::getMidiMsg() const
{
	return m_nMidiMsg;
}

inline float Note::getProbability() const
{
	return m_fProbability;
}

inline void Note::setProbability( float value )
{
	m_fProbability = value;
}

inline std::shared_ptr<SelectedLayerInfo> Note::getLayerSelected( int nCompoIdx ) const
{
	if ( nCompoIdx < 0 || nCompoIdx >= m_layersSelected.size() ) {
		return nullptr;
	}
	return m_layersSelected.at( nCompoIdx );
}

inline int Note::getHumanizeDelay() const
{
	return m_nHumanizeDelay;
}

inline bool Note::filterSustain() const
{
	const double fLimit = 0.001;
	return ( fabs( m_fLpfbL ) > fLimit || fabs( m_fLpfbR ) > fLimit ||
			 fabs( m_fBpfbL ) > fLimit || fabs( m_fBpfbR ) > fLimit );
}

inline Note::Key Note::getKey() const
{
	return m_key;
}

inline Note::Octave Note::getOctave() const
{
	return m_octave;
}

inline int Note::getMidiKey() const
{
	int nMidiKey = ( m_octave + OCTAVE_OFFSET ) * KEYS_PER_OCTAVE + m_key;
	if ( m_pInstrument != nullptr ) {
		nMidiKey += m_pInstrument->get_midi_out_note() -
			MidiMessage::instrumentOffset;
	}
	return nMidiKey;
}

inline int Note::getMidiVelocity() const
{
	return m_fVelocity * 127;
}

inline float Note::getPitchFromKeyOctave() const
{
	return m_octave * KEYS_PER_OCTAVE + m_key;
}

inline void Note::setKeyOctave( Key key, Octave octave )
{
	if( key>=KEY_MIN && key<=KEY_MAX ) m_key = key;
	if( octave>=OCTAVE_MIN && octave<=OCTAVE_MAX ) m_octave = octave;
}

inline void Note::setMidiInfo( Key key, Octave octave, int msg )
{
	if( key>=KEY_MIN && key<=KEY_MAX ) m_key = key;
	if( octave>=OCTAVE_MIN && octave<=OCTAVE_MAX ) m_octave = octave;
	m_nMidiMsg = msg;
}

inline bool Note::match( int nInstrumentId, const QString& sType, Key key,
						 Octave octave ) const
{
	return m_nInstrumentId == nInstrumentId && m_sType == sType &&
		m_key == key && m_octave==octave;
}

inline bool Note::match( const std::shared_ptr<Note> pNote ) const
{
	return match( pNote->m_nInstrumentId, pNote->m_sType, pNote->m_key,
				  pNote->m_octave );
}

inline bool Note::compareStart( const std::shared_ptr<Note> pNote1,
								const std::shared_ptr<Note> pNote2 ) {
	if ( pNote1 == nullptr || pNote2 == nullptr ) {
		return false;
	}

	return pNote1->getNoteStart() > pNote2->getNoteStart();
}

inline bool Note::compare( const std::shared_ptr<Note> pNote1,
						   const std::shared_ptr<Note> pNote2 ) {
	if ( pNote1 == nullptr || pNote2 == nullptr ) {
		return false;
	}

	if ( pNote1->getPosition() != pNote2->getPosition() ) {
		return pNote1->getPosition() > pNote2->getPosition();
	}
	else {
		return pNote1->getTotalPitch() > pNote2->getTotalPitch();
	}
}

inline void Note::computeLrValues( float* val_l, float* val_r )
{
	if ( m_pInstrument == nullptr ) {
		*val_l = 0.0f;
		*val_r = 0.0f;
		return;
	}
	else {
		const float fCutOff = m_pInstrument->get_filter_cutoff();
		const float fResonance = m_pInstrument->get_filter_resonance();
		m_fBpfbL  =  fResonance * m_fBpfbL  + fCutOff * ( *val_l - m_fLpfbL );
		m_fLpfbL +=  fCutOff   * m_fBpfbL;
		m_fBpfbR  =  fResonance * m_fBpfbR  + fCutOff * ( *val_r - m_fLpfbR );
		m_fLpfbR +=  fCutOff   * m_fBpfbR;
		*val_l = m_fLpfbL;
		*val_r = m_fLpfbR;
	}
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
