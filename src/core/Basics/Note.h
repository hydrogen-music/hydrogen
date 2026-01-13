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

#include <map>
#include <memory>

#include <core/Basics/DrumkitMap.h>
#include <core/Basics/Instrument.h>
#include <core/Midi/Midi.h>
#include <core/Object.h>

#define OCTAVE_OFFSET 3
#define OCTAVE_NUMBER 7
#define KEYS_PER_OCTAVE 12

#define VELOCITY_MIN 0.0f
#define VELOCITY_DEFAULT 0.8f
#define VELOCITY_MAX 1.0f
#define PAN_MIN -1.0f
#define PAN_DEFAULT 0.0f
#define PAN_MAX 1.0f
#define LEAD_LAG_MIN -1.0f
#define LEAD_LAG_DEFAULT 0.0f
#define LEAD_LAG_MAX 1.0f
#define LENGTH_ENTIRE_SAMPLE -1
#define PROBABILITY_MIN 0.0f
#define PROBABILITY_DEFAULT 1.0f
#define PROBABILITY_MAX 1.0f

namespace H2Core {

class XMLNode;
class ADSR;
class InstrumentLayer;
class InstrumentList;

/** Auxiliary variables storing the rendering state of a #H2Core::Note within
 * the #H2Core::Sampler */
struct SelectedLayerInfo {
	SelectedLayerInfo();
	~SelectedLayerInfo();

	/** Selected layer during rendering in the #H2Core::Sampler. */
	std::shared_ptr<InstrumentLayer> pLayer;

	/** Stores the frame till which the #H2Core::Sample of #pLayer was already
	 * rendered. If several cycles of #Sampler::renderNote() are required, this
	 * variable corresponds to the starting point of each cycle.
	 *
	 * It is given in float instead of int/long - what one might expect when
	 * talking about frames - since it also serves as the fraction of the
	 * #H2Core::Sample already processed in case it has to be resampled. */
	float fSamplePosition;

	/** Frame / #fSamplePosition at which rendering of the current note is
	 * considered done.
	 *
	 * For regular notes this is the number of frames of the #H2Core::Sample
	 * contained in #pLayer.
	 *
	 * If, however, the user specifies a custom length, things are more complex.
	 * The length is specified in the GUI in **ticks** and this variable is the
	 * corresponding value in frames. Now, whenever manually adjusting the tempo
	 * or adding/deleting a tempo marker the length of the note in frames
	 * differs for the new speed. In case rendering did already started, it is
	 * important to not rescale the whole length of the note but just the
	 * fraction between #fSamplePosition and the former #nNoteLength.*/
	int nNoteLength;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const;
};

/**
 * Represents an instrument activation with various parameters and options.
 *
 * A note itself can not render audio. First, it has to be mapped to a
 * #H2Core::Drumkit in order to be associated with an #H2Core::Instrument. Notes
 * holding a valid #m_pInstrument can than be handed over to the
 * #H2Core::Sampler for rendering.
 */
/** \ingroup docCore docDataStructure */
class Note : public H2Core::Object<Note> {
	H2_OBJECT( Note )
   public:
	/** possible keys */
	enum class Key {
		C = 0,
		Cs = 1,
		D = 2,
		Ef = 3,
		E = 4,
		F = 5,
		Fs = 6,
		G = 7,
		Af = 8,
		A = 9,
		Bf = 10,
		B = 11,
		Invalid = 666
	};
	static QString KeyToQString( const Key& key );
	static Key QStringToKey( const QString& sKey );
	static Key keyFromInt( int nValue );
	static Key keyFromIntClamp( int nValue );
	static Key keyFrom( Midi::Note note );

	static constexpr Key KeyDefault = Key::C;
	static constexpr Key KeyMinimum = Key::C;
	static constexpr Key KeyMaximum = Key::B;

	/** possible octaves */
	enum class Octave {
		P8Z = -3,
		P8Y = -2,
		P8X = -1,
		P8 = 0,
		P8A = 1,
		P8B = 2,
		P8C = 3,
		Invalid = 666
	};
	static QString OctaveToQString( const Octave& octave );
	static Octave octaveFromInt( int nValue );
	static Octave octaveFromIntClamp( int nValue );
	static Octave octaveFrom( Midi::Note note );

	static constexpr Octave OctaveDefault = Octave::P8;
	static constexpr Octave OctaveMinimum = Octave::P8Z;
	static constexpr Octave OctaveMaximum = Octave::P8C;

	/** The pitch of a note represents the resulting (fundamental) frequency
	 * with high pitches corrseponding to low frequencies and vice versa.
	 *
	 * Although drum samples are in general of indefinite pitch - the
	 * resulting sound is not made up by a fundamental frequency and
	 * harmonics for the most part but contains a large number of other
	 * frequencies (overtones) - we do still use it in here to determine the
	 * _relative_ pitch. Via resampling we can alter pitch of the underlying
	 * samples and can make them sound higher or lower pitched.
	 *
	 * The pitch is centered around C4 (termed C2 within Hydrogen. Why we
	 * start with -2 to count octaves is lost in history). Since pitch is
	 * defined on a logarithmic scale, every semitone (on the western
	 * chromatic scale used within the MIDI standard) does exactly equate to
	 * a pitch difference of `1`. Thus, a difference of #Note::Key within
	 * the same #Note::Octave is equivilant to the corresponding difference
	 * in #Note::Pitch.
	 *
	 * In Hydrogen, the pitch of a note has various contribution (depending
	 * on the context):
	 *
	 * - #Note::Key and #Note::Octave: set in #PianoRollEditor
	 * - #Instrument::m_fPitchOffset: instrument-wide pitch offset
	 * - #InstrumentLayer::m_fPitchOffset: pitch offset for the particular
	 *     sample
	 * - #Note::m_fRandomPitch: created as part of the humanization based on
	 *     #Instrument::m_fRandomPitchFactor.
	 *
	 * The range of possible values is smaller than e.g. the allowed range
	 * within the MIDI standard (TBH I do not see why. This design decision
	 * is lost as well).
	 *
	 * The value itself if based on `float` since it allows use for a more
	 * precise pitch control than semitones (`int`s). This is especially
	 * important during humanization. `double`, on the other hand, is
	 * already "too" precise since the human ear won't be able to tell those
	 * fine differences apart and other software/technologies is not
	 * designed to deal with it either. */
	class Pitch {
	   public:
		// Since we are declaring Pitch in here, we can not use it for
		// constexpr.
		static Pitch Invalid;
		static Pitch Minimum;
		static Pitch Default;
		static Pitch Maximum;

		// It would be more consistent with Note::Key, Note::Octave, Midi::Note
		// ... to have a Note::pitchFromFloat() static method. But then we
		// could not make the Pitch() constructor private (and our code more
		// secure) because we would have a chicken - egg situation with class
		// and friend method definition.
		static Pitch fromFloat( float fPitch )
		{
			if ( fPitch >= static_cast<float>( Pitch::Minimum ) &&
				 fPitch <= static_cast<float>( Pitch::Maximum ) ) {
				return static_cast<Pitch>( fPitch );
			}
			else {
				return Pitch::Invalid;
			}
		}
		static Pitch fromFloatClamp( float fPitch )
		{
			return static_cast<Pitch>( std::clamp(
				fPitch, static_cast<float>( Pitch::Minimum ),
				static_cast<float>( Pitch::Maximum )
			) );
		}
		static Pitch fromKeyOctave( Key key, Octave octave )
		{
			return fromFloatClamp(
				static_cast<float>( KEYS_PER_OCTAVE ) *
					static_cast<float>( octave ) +
				static_cast<float>( key )
			);
		}
		/** Line within the #PianoRollEditor. */
		static Pitch fromLine( int nLine )
		{
			return fromFloatClamp(
				KEYS_PER_OCTAVE * ( static_cast<int>( Note::OctaveMinimum ) +
									OCTAVE_NUMBER ) -
				1 - nLine
			);
		}
		static Pitch fromMidiNote( Midi::Note note )
		{
			const auto key = keyFrom( note );
			const auto octave = octaveFrom( note );
			if ( key == Note::Key::Invalid ||
				 octave == Note::Octave::Invalid ) {
                return Pitch::Invalid;
			}
			return fromKeyOctave( key, octave );
		}

		/** Assumes #m_fValue to be a pitch difference in equal-tempered
		 * semitones to #Pitch::Default and calculates the corresponding ratio
		 * of target to source frequency. */
		double toFrequencyRatio() const
		{
			// Equivalent to, but quicker to compute than, pow( 2.0, ( fPitch/12
			// ) )
			return pow( 1.0594630943593, static_cast<double>( m_fValue ) );
		}
		Octave toOctave() const
		{
			if ( m_fValue >= 0 ) {
				return Note::octaveFromIntClamp( m_fValue / KEYS_PER_OCTAVE );
			}
			else {
				return Note::octaveFromIntClamp(
					( m_fValue - 11 ) / KEYS_PER_OCTAVE
				);
			}
		};
		Key toKey() const
		{
			return Note::keyFromIntClamp(
				m_fValue - KEYS_PER_OCTAVE * static_cast<int>( toOctave() )
			);
		}
		/** Line within the #PianoRollEditor. */
		int toLine() const
		{
			return KEYS_PER_OCTAVE * ( static_cast<int>( Note::OctaveMinimum ) +
									   OCTAVE_NUMBER ) -
				   1 - static_cast<int>( std::round( m_fValue ) );
		};

		operator double() const { return static_cast<double>( m_fValue ); };
		operator float() const { return m_fValue; };
		operator int() const { return static_cast<int>( m_fValue ); };

		Pitch& operator=( const Pitch& other )
		{
			m_fValue = other.m_fValue;
			return *this;
		};

		bool operator==( const Pitch& other ) const
		{
			return m_fValue == other.m_fValue;
		};
		bool operator!=( const Pitch& other ) const
		{
			return m_fValue != other.m_fValue;
		};
		bool operator<( const Pitch& other ) const
		{
			return m_fValue < other.m_fValue;
		};
		bool operator>( const Pitch& other ) const
		{
			return m_fValue > other.m_fValue;
		};
		// No +/- operators to enfore bound checks.
		// Pitch operator+( const Pitch& other ) const
		// Pitch operator-( const Pitch& other ) const

	   private:
		constexpr explicit Pitch( float fValue ) : m_fValue( fValue ) {};

		float m_fValue;
	};

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
	 */
	Note(
		std::shared_ptr<Instrument> pInstrument = nullptr,
		int nPosition = 0,
		float fVelocity = VELOCITY_DEFAULT,
		float fPan = PAN_DEFAULT,
		int nLength = LENGTH_ENTIRE_SAMPLE
	);

	Note( std::shared_ptr<Note> pOther );
	~Note();

	Note::Pitch toPitch() const;

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
	static std::shared_ptr<Note>
	loadFrom( const XMLNode& node, bool bSilent = false );

	/** #m_pInstrument accessor */
	std::shared_ptr<Instrument> getInstrument() const;
	/**
	 * #m_instrumentId setter
	 * \param value the new value
	 */
	void setInstrumentId( Instrument::Id id );
	/** #m_instrumentId accessor */
	Instrument::Id getInstrumentId() const;

	void setType( Instrument::Type sType );
	Instrument::Type getType() const;

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
	void setPanWithRangeFrom0To1( float fVal )
	{
		// scale and translate into [-1;1]
		this->setPan( PAN_MIN + ( PAN_MAX - PAN_MIN ) * fVal );
	};
	/** get pan of the note. Output pan range: [-1;1] */
	float getPan() const;
	/** get pan of the note, scaling and translating the range from [-1;1] to
	 * [0;1] */
	float getPanWithRangeFrom0To1() const { return 0.5f * ( 1.f + m_fPan ); }

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
	/**
	 * #m_bNoteOff setter
	 * \param value the new value
	 */
	void setNoteOff( bool value );
	/** #m_bNoteOff accessor */
	bool getNoteOff() const;

	bool layersAlreadySelected() const;

	/** Picks one #H2Core::InstrumentLayer for each
	 * #H2Core::InstrumentComponent in #m_pInstrument according to
	 * #H2Core::InstrumentComponent::m_selection.
	 *
	 * In order to allow for a consistent round robin selection, the layers
	 * used last for all encountered components is provided. */
	void selectLayers( const std::map<
					   std::shared_ptr<InstrumentComponent>,
					   std::shared_ptr<InstrumentLayer> >& lastUsedLayers );

	std::map<
		std::shared_ptr<InstrumentComponent>,
		std::shared_ptr<SelectedLayerInfo> >
	getAllSelectedLayerInfos() const;
	/** Returns the #H2Core::InstrumentLayer and some additional rendering
	 * meta data for a given component. If no selection took place yet,
	 * `nullptr` will be returned. */
	std::shared_ptr<SelectedLayerInfo> getSelecterLayerInfo(
		std::shared_ptr<InstrumentComponent> pComponent
	) const;
	/** Can be used for custom layer selection.
	 *
	 * Note that when using this function for one
	 * #H2Core::InstrumentComponent, you have to use it for all others too
	 * or no layer/sample will be picked for them. */
	void setSelectedLayerInfo(
		std::shared_ptr<SelectedLayerInfo> pInfo,
		std::shared_ptr<InstrumentComponent> pComponent
	);

	void mapToInstrument( std::shared_ptr<Instrument> pInstrument );

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
	Key getKey() const;
	void setKey( Key key );
	Octave getOctave() const;
	void setOctave( Octave octave );
	/** return scaled key for midi output, !!! DO NOT CHECK IF INSTRUMENT IS SET
	 * !!! */
	Midi::Note getMidiNote() const;
	Midi::Parameter getMidiVelocity() const;

	/** get the ADSR of the note */
	std::shared_ptr<ADSR> getAdsr() const;

	/** @returns true if instrument id and type as well as key and octave
	 * matches with internal
	 *
	 * \param id the instrument ID to match with #m_instrumentId
	 * \param sType the instrument type to match with #m_sType
	 * \param key the key to match with #m_key
	 * \param octave the octave to match with #m_octave
	 */
	bool match(
		Instrument::Id id,
		const Instrument::Type& sType,
		Key key,
		Octave octave
	) const;

	/** Return true if two notes match in instrument, key and octave. */
	bool match( const std::shared_ptr<Note> pNote ) const;

	/** Does not compare the whole note but just its position in rendered in
	 * the (PianoRoll) editor. */
	bool matchPosition( const std::shared_ptr<Note> pNote ) const;

	/** Compares two notes based on position (primary) and pitch
	 * (secundary).
	 *
	 * In contrast to compareStart() position in here only takes the actual
	 * #m_nPosition into account. Neither humanization nor lead/lag.
	 *
	 * @returns `true` in case @a pNote1 wins (larger position, larger
	 *   pitch) compared to @a pNote2. */
	static bool compare(
		const std::shared_ptr<Note> pNote1,
		const std::shared_ptr<Note> pNote2
	);

	/** Like compare() but in reverse order.
	 *
	 * Just negating the output of compare() doesn't not work in the macOS
	 * pipeline for some reason. */
	static bool compareAscending(
		const std::shared_ptr<Note> pNote1,
		const std::shared_ptr<Note> pNote2
	);

	/** Performs a comparison based on starting position of the note.
	 *
	 * Note that this start has to be calculated first and involves random
	 * components (humanization). It is thus something to be performed on
	 * notes already enqueued in #AudioEngine or #Sampler and not suitable
	 * for plain notes residing in #Pattern.
	 *
	 * @returns `true` in case @a pNote1 has a bigger starting position as
	 *   @a pNote2. */
	static bool compareStart(
		const std::shared_ptr<Note> pNote1,
		const std::shared_ptr<Note> pNote2
	);
	struct compareStartStruct {
		bool
		operator()( std::shared_ptr<Note> pNote1, std::shared_ptr<Note> pNote2 )
		{
            return Note::compareStart( pNote1, pNote2 );
		}
	};

	/**
	 * compute left and right output based on filters
	 * \param val_l the left channel value
	 * \param val_r the right channel value
	 */
	void computeLrValues( float* val_l, float* val_r );

	long long getNoteStart() const;
	float getUsedTickSize() const;

	float getPitchHumanization() const;

	long long getMidiNoteOnSentFrame() const;
	void setMidiNoteOnSentFrame( long long nNew );

	long long getMidiNoteOffFrame() const;
	void setMidiNoteOffFrame( long long nNew );

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
	 * Add random contributions to #m_fPitchHumanization, #m_nHumanizeDelay, and
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
	QString toQString( const QString& sPrefix = "", bool bShort = true )
		const override;

   private:
	/** The ID of the instrument the note will be mapped to in case a
	 * drumkit with no or incomplete types is used (e.g. a new or legacy
	 * kit).
	 *
	 * Note that this number being set does not mean that the note is
	 * actually associated with an instrument of the (current) drumkit. The
	 * pointer #m_pInstrument will tell instead. */
	Instrument::Id m_instrumentId;
	/** Drumkit-independent identifier used to relate a note/pattern to a
	 * different kit.
	 *
	 * Note that this one being set does not mean that the note is actually
	 * associated with an instrument of the (current) drumkit. The pointer
	 * #m_pInstrument will tell instead. */
	Instrument::Type m_sType;
	int m_nPosition;	///< note position in
						/// ticks inside the pattern
	float m_fVelocity;	///< velocity (intensity) of the note [0;1]
	float m_fPan;		///< pan of the note, [-1;1] from
						/// left to right, as requested by
						/// Sampler PanLaws
	/** Length of the note in ticks.
	 *
	 * If set to -1, the Note will be rendered till the end of all
	 * contained Samples is reached.
	 */
	int m_nLength;
	Key m_key;						///< the key, [0;11]==[C;B]
	Octave m_octave;				///< the octave [-3;3]
	std::shared_ptr<ADSR> m_pAdsr;	///< attack decay sustain release
	float m_fLeadLag;				///< lead or lag offset of the note
	/** Offset of the note start in frames.
	 *
	 * It includes contributions of the onset humanization, the
	 * lead lag factor, and the swing. For some of these a random
	 * value will be drawn but once stored in this variable, the
	 * delay is fixed and will not change anymore.
	 *
	 * It is incorporated in the #m_nNoteStart.
	 */
	int m_nHumanizeDelay;
	float m_fBpfbL;					///< left band pass filter buffer
	float m_fBpfbR;					///< right band pass filter buffer
	float m_fLpfbL;					///< left low pass filter buffer
	float m_fLpfbR;					///< right low pass filter buffer
	bool m_bNoteOff;				///< note type on|off
	float m_fProbability;			///< note probability
	static const char* m_keyStr[];	///< used to build QString
									/// from #m_key an #m_octave
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

	/** Transient property caching the pitch humanization. Uses the same scale
	 * as #Note::Pitch.
	 *
	 * Not written to disk. */
	float m_fPitchHumanization;

	std::map<
		std::shared_ptr<InstrumentComponent>,
		std::shared_ptr<SelectedLayerInfo> >
		m_selectedLayerInfoMap;

	/** Transient member not written to file. Indicates whether - `-1` if not -
	 * and when a `NOTE_ON` MIDI message was sent for this note within the
	 * #Sampler. */
	long long m_nMidiNoteOnSentFrame;

	/** Transient member not written to file. Indicates at which frame
	 * #H2Core::Sampler is supposed to send a NOTE_OFF MIDI message
	 * corresponding to this note. */
	long long m_nMidiNoteOffFrame;

	/** The instrument (of the current drumkit) the note is associated with.
	 * It will be used to render the note and, if not `nullptr`, to indicate
	 * that the note is mapped to a drumkit. */
	std::shared_ptr<Instrument> m_pInstrument;
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

inline void Note::setInstrumentId( Instrument::Id id )
{
	m_instrumentId = id;
}

inline Instrument::Id Note::getInstrumentId() const
{
	return m_instrumentId;
}

inline void Note::setType( Instrument::Type sType )
{
	m_sType = sType;
}
inline Instrument::Type Note::getType() const
{
	return m_sType;
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

inline void Note::setNoteOff( bool value )
{
	m_bNoteOff = value;
}

inline bool Note::getNoteOff() const
{
	return m_bNoteOff;
}

inline std::map<
	std::shared_ptr<InstrumentComponent>,
	std::shared_ptr<SelectedLayerInfo> >
Note::getAllSelectedLayerInfos() const
{
	return m_selectedLayerInfoMap;
}

inline float Note::getProbability() const
{
	return m_fProbability;
}

inline void Note::setProbability( float value )
{
	m_fProbability = value;
}

inline int Note::getHumanizeDelay() const
{
	return m_nHumanizeDelay;
}

inline bool Note::filterSustain() const
{
	const double fLimit = 0.001;
	return (
		fabs( m_fLpfbL ) > fLimit || fabs( m_fLpfbR ) > fLimit ||
		fabs( m_fBpfbL ) > fLimit || fabs( m_fBpfbR ) > fLimit
	);
}

inline Note::Key Note::getKey() const
{
	return m_key;
}
inline void Note::setKey( Note::Key key )
{
	m_key = key;
}

inline Note::Octave Note::getOctave() const
{
	return m_octave;
}
inline void Note::setOctave( Note::Octave octave )
{
	m_octave = octave;
}

inline Midi::Note Note::getMidiNote() const
{
	int nMidiKey =
		( static_cast<int>( m_octave ) + OCTAVE_OFFSET ) * KEYS_PER_OCTAVE +
		static_cast<int>( m_key );
	if ( m_pInstrument != nullptr ) {
		nMidiKey += static_cast<int>( m_pInstrument->getMidiOutNote() ) -
					static_cast<int>( Midi::NoteOffset );
	}
	return Midi::noteFromIntClamp( nMidiKey );
}

inline Midi::Parameter Note::getMidiVelocity() const
{
	return Midi::parameterFromIntClamp( static_cast<int>(
		std::round( m_fVelocity * static_cast<float>( Midi::ParameterMaximum ) )
	) );
}

inline bool Note::match(
	Instrument::Id id,
	const Instrument::Type& sType,
	Key key,
	Octave octave
) const
{
	return m_instrumentId == id && m_sType == sType && m_key == key &&
		   m_octave == octave;
}

inline bool Note::matchPosition( const std::shared_ptr<Note> pNote ) const
{
	if ( pNote == nullptr ) {
		return false;
	}
	return m_instrumentId == pNote->m_instrumentId &&
		   m_sType == pNote->m_sType && m_nPosition == pNote->m_nPosition &&
		   m_key == pNote->m_key && m_octave == pNote->m_octave;
}

inline bool Note::match( const std::shared_ptr<Note> pNote ) const
{
	if ( pNote == nullptr ) {
		return false;
	}
	return m_instrumentId == pNote->m_instrumentId &&
		   m_sType == pNote->m_sType && m_nPosition == pNote->m_nPosition &&
		   m_fVelocity == pNote->m_fVelocity && m_fPan == pNote->m_fPan &&
		   m_nLength == pNote->m_nLength && m_fLeadLag == pNote->m_fLeadLag &&
		   m_fProbability == pNote->m_fProbability && m_key == pNote->m_key &&
		   m_octave == pNote->m_octave;
}

inline bool Note::compareStart(
	const std::shared_ptr<Note> pNote1,
	const std::shared_ptr<Note> pNote2
)
{
	if ( pNote1 == nullptr || pNote2 == nullptr ) {
		return false;
	}

	if ( pNote1->getNoteStart() == pNote2->getNoteStart() &&
		 pNote1->getNoteOff() ) {
		return true;
	}

	return pNote1->getNoteStart() > pNote2->getNoteStart();
}

inline bool Note::compare(
	const std::shared_ptr<Note> pNote1,
	const std::shared_ptr<Note> pNote2
)
{
	if ( pNote1 == nullptr || pNote2 == nullptr ) {
		return false;
	}

	if ( pNote1->getPosition() != pNote2->getPosition() ) {
		return pNote1->getPosition() > pNote2->getPosition();
	}
	else {
		return Note::Pitch::fromFloatClamp(
				   static_cast<float>( pNote1->toPitch() ) +
				   pNote1->getPitchHumanization()
			   ) >
			   Note::Pitch::fromFloatClamp(
				   static_cast<float>( pNote2->toPitch() ) +
				   pNote2->getPitchHumanization()
			   );
	}
}

inline bool Note::compareAscending(
	const std::shared_ptr<Note> pNote1,
	const std::shared_ptr<Note> pNote2
)
{
	if ( pNote1 == nullptr || pNote2 == nullptr ) {
		return false;
	}

	if ( pNote1->getPosition() != pNote2->getPosition() ) {
		return pNote1->getPosition() < pNote2->getPosition();
	}
	else {
		return Note::Pitch::fromFloatClamp(
				   static_cast<float>( pNote1->toPitch() ) +
				   pNote1->getPitchHumanization()
			   ) <
			   Note::Pitch::fromFloatClamp(
				   static_cast<float>( pNote2->toPitch() ) +
				   pNote2->getPitchHumanization()
			   );
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
		const float fCutOff = m_pInstrument->getFilterCutoff();
		const float fResonance = m_pInstrument->getFilterResonance();
		m_fBpfbL = fResonance * m_fBpfbL + fCutOff * ( *val_l - m_fLpfbL );
		m_fLpfbL += fCutOff * m_fBpfbL;
		m_fBpfbR = fResonance * m_fBpfbR + fCutOff * ( *val_r - m_fLpfbR );
		m_fLpfbR += fCutOff * m_fBpfbR;
		*val_l = m_fLpfbL;
		*val_r = m_fLpfbR;
	}
}

inline long long Note::getNoteStart() const
{
	return m_nNoteStart;
}
inline float Note::getUsedTickSize() const
{
	return m_fUsedTickSize;
}
inline float Note::getPitchHumanization() const
{
	return m_fPitchHumanization;
}
inline long long Note::getMidiNoteOnSentFrame() const
{
    return m_nMidiNoteOnSentFrame;
}
inline void Note::setMidiNoteOnSentFrame( long long nNew )
{
    m_nMidiNoteOnSentFrame = nNew;
}
inline long long Note::getMidiNoteOffFrame() const
{
    return m_nMidiNoteOffFrame;
}
inline void Note::setMidiNoteOffFrame( long long nNew )
{
    m_nMidiNoteOffFrame = nNew;
}
};	// namespace H2Core

#endif	// H2C_NOTE_H

/* vim: set softtabstop=4 noexpandtab: */
